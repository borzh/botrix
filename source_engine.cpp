#include <good/mutex.h>
#include <good/file.h>

#include "clients.h"
#include "console_commands.h"
#include "server_plugin.h"
#include "source_engine.h"
#include "waypoint.h"

#include "cbase.h"
#include "IEffects.h"
#include "ndebugoverlay.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



//----------------------------------------------------------------------------------------------------------------
extern IVDebugOverlay* pVDebugOverlay;
extern IEffects* pEffects;

extern char* szMainBuffer;
extern int iMainBufferSize;

const int iTextTime = 10; // Time in seconds to show text in CUtil::GetReachableInfoFromTo().



//----------------------------------------------------------------------------------------------------------------
class CTraceFilterWorldAndOtherProps: public ITraceFilter
{
public:
    int iTraceFlags;
    TVisibility iVisibility;

    CTraceFilterWorldAndOtherProps( TVisibility iVisibility ): iVisibility( iVisibility )
    {
        switch ( iVisibility )
        {
            case EVisibilityWorld:
                iTraceFlags = MASK_SOLID_BRUSHONLY;
                break;
            case EVisibilitySeeAndShoot:
                iTraceFlags = MASK_VISIBLE | MASK_SHOT;
                break;
            case EVisibilityOtherProps:
                iTraceFlags = MASK_PLAYERSOLID; // MASK_SOLID_BRUSHONLY?
                break;
            default:
                BASSERT(false);
        }
    }

    virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
    {
        int index = pServerEntity->GetRefEHandle().GetEntryIndex();
        if ( index >= MAX_EDICTS )
            return false;

        GoodAssert( index != 0 ); // Shouldn't pass in the world's entity.

        // Should trace players only if the visibility is for shooting.
        if ( index < 1 + CPlayers::Size() )
            return iVisibility == EVisibilitySeeAndShoot; 

        edict_t *pEdict = CBotrixPlugin::instance->pEngineServer->PEntityOfEntIndex( index );
        if ( pEdict == NULL )
            return false;

        CItemClass* test;
        const char* szClassName = pEdict->GetClassName();
        TItemType iType = CItems::GetEntityType( szClassName, test, 0, EItemTypeCollisionTotal );
        return iType == EItemTypeOther;
    }

    virtual TraceType_t	GetTraceType() const
    {
        switch ( iVisibility )
        {
            case EVisibilityWorld:
                return TRACE_WORLD_ONLY;
            case EVisibilityOtherProps:
            case EVisibilitySeeAndShoot:
                return TRACE_EVERYTHING_FILTER_PROPS;
            default:
                BASSERT( false );
        }
        return TRACE_WORLD_ONLY;
    }
};

//----------------------------------------------------------------------------------------------------------------
class COnlyOneEntityTraceFilter: public ITraceFilter
{
public:
    COnlyOneEntityTraceFilter( edict_t* pEntity )
    {
        pHandle = pEntity->GetIServerEntity()->GetNetworkable()->GetEntityHandle();
    }

    virtual TraceType_t	GetTraceType() const { return TRACE_ENTITIES_ONLY; }
    virtual bool ShouldHitEntity( IHandleEntity *pEntity, int /*contentsMask*/ ) { return pHandle == pEntity; }

protected:
    const IHandleEntity* pHandle;
};

CTraceFilterWorldAndOtherProps cWorldTraceFilter( EVisibilityWorld );
CTraceFilterWorldAndOtherProps cTraceFilter( EVisibilityOtherProps );
CTraceFilterWorldAndOtherProps cVisibilityTraceFilter(EVisibilitySeeAndShoot);

inline CTraceFilterWorldAndOtherProps& GetFilter( TVisibility iVisibility )
{
    switch ( iVisibility )
    {
        case EVisibilityWorld:
            return cWorldTraceFilter;
        case EVisibilityOtherProps:
            return cTraceFilter;
        case EVisibilitySeeAndShoot:
            return cVisibilityTraceFilter;
        default:
            BASSERT(false);
    }
    return cWorldTraceFilter;
}

//----------------------------------------------------------------------------------------------------------------
// If slope is less that 45 degrees (for HL2) then player can move forward (returns EReachReachable).
// If not and vSrc is higher, then player can take damage trying to get to lower ground (returns EReachFallDamage).
// If vSrc is lower in Z, and slope is more than 45 degrees then player can't reach destination (returns
// EReachNotReachable).
//----------------------------------------------------------------------------------------------------------------
TReach CanClimbSlope( const Vector& vSrc, const Vector& vDest )
{
    Vector vDiff = vDest - vSrc;
    QAngle ang;
    VectorAngles(vDiff, ang); // Get pitch to know if gradient is too big.

    float slope = CMod::GetVar( EModVarSlopeGradientToSlideOff );
    if ( ang.x < -slope ) // Destination is higher and slope is more than 45 degrees, can't climb it.
        return EReachNotReachable;
    else if ( ( ang.x > slope ) &&       // Slope is more than 45 degrees.
              ( vDiff.z > CMod::GetVar( EModVarHeightForFallDamage ) ) ) // Source is higher.
        return EReachFallDamage; // Can take damage at fall.
    else
        return EReachReachable; // Slope gradient is less than 45 degrees.
}

//----------------------------------------------------------------------------------------------------------------
// Returns true if can move forward when standing at vGround performing a jump (normal, with crouch o maybe just
// walking). At return vHit contains new coord which is where player can get after jump.
//----------------------------------------------------------------------------------------------------------------
bool CanPassOrJump( Vector& vStart, Vector& vEnd, Vector& vHit, Vector& vDirectionInc, bool& bNeedJump )
{
    float zDist = vHit.z - vEnd.z; // Distance from ground to hit position.

    vHit = vEnd;
    vHit += vDirectionInc;

    float fMaxWalkHeight = CMod::GetVar( EModVarPlayerObstacleToJump );
    if ( zDist <= fMaxWalkHeight ) // Can walk?
    {
        vHit.z = vEnd.z + fMaxWalkHeight + 1;
        CUtil::TraceLine(vStart, vHit, cTraceFilter.iTraceFlags, &cTraceFilter); // Trace again.
        if ( !CUtil::IsTraceHitSomething() ) // We can stand on vHit after jump.
        {
            bNeedJump = false;
            return true;
        }
    }
    float fJumpCrouched = CMod::GetVar( EModVarPlayerJumpHeightCrouched );
    if ( zDist <= fJumpCrouched ) // Can perform jump?
    {
        vHit.z = vEnd.z + fJumpCrouched + 1;
        CUtil::TraceLine(vStart, vHit, cTraceFilter.iTraceFlags, &cTraceFilter ); // Trace again.
        if ( !CUtil::IsTraceHitSomething() ) // We can stand on vHit after jump.
        {
            bNeedJump = true;
            return true;
        }
    }

    return false; // Can't jump over.
}



//****************************************************************************************************************
good::TLogLevel CUtil::iLogLevel = good::ELogLevelInfo;

const Vector CUtil::vZero(0, 0, 0);
const QAngle CUtil::angZero(0, 0, 0);

trace_t CUtil::m_TraceResult;



//----------------------------------------------------------------------------------------------------------------
bool CUtil::IsRayHitsEntity( edict_t* pEntity, const Vector& vSrc, const Vector& vDest )
{
    COnlyOneEntityTraceFilter filter(pEntity);
    TraceLine(vSrc, vDest, MASK_OPAQUE, &filter);
    return m_TraceResult.fraction >= 0.95f;
}

//----------------------------------------------------------------------------------------------------------------
bool CUtil::IsVisible( const Vector& vSrc, const Vector& vDest, TVisibility iVisibility )
{
    CUtil::SetPVSForVector( vSrc );
    if ( !CUtil::IsVisiblePVS( vDest ) )
        return false;

    CTraceFilterWorldAndOtherProps* pTraceFilter = &GetFilter( iVisibility );
    TraceLine(vSrc, vDest, pTraceFilter->iTraceFlags, pTraceFilter );
    return m_TraceResult.fraction >= 0.95f;
}

//----------------------------------------------------------------------------------------------------------------
bool CUtil::IsVisible( const Vector& vSrc, edict_t* pDest )
{
    Vector v;
    EntityHead(pDest, v);
    
    return IsVisible(vSrc, v, EVisibilitySeeAndShoot);
}

//----------------------------------------------------------------------------------------------------------------
TReach CUtil::GetReachableInfoFromTo( const Vector& vSrc, Vector& vDest, bool& bCrouch, float fDistanceSqr, float fMaxDistanceSqr, bool bShowHelp )
{
    static int iRandom = 0;      // This function may be called 2 times with (v1, v2) and (v2,v1)
    iRandom = (iRandom + 1) & 1; // so iRandom is to draw text higher that previous time.

    if ( fDistanceSqr <= 0.0f )
        fDistanceSqr = vSrc.AsVector2D().DistToSqr(vDest.AsVector2D());

    if ( fDistanceSqr > fMaxDistanceSqr )
        return EReachNotReachable;

    if ( !CUtil::IsVisible(vSrc, vDest, EVisibilityOtherProps ) )
        return EReachNotReachable;

    // Check if can swim there first.
    int iSrcContent = CBotrixPlugin::pEngineTrace->GetPointContents( vSrc );
    int iDestContent = CBotrixPlugin::pEngineTrace->GetPointContents( vDest );
    if ( iSrcContent == CONTENTS_WATER && iDestContent  == CONTENTS_WATER)
        return EReachReachable;

    Vector vMinZ(0, 0, -iHalfMaxMapSize);

    // Get ground positions.
    TraceLine( vSrc, vSrc + vMinZ, cTraceFilter.iTraceFlags, &cTraceFilter );
    Vector vSrcGround = TraceResult().endpos;
    GoodAssert(vSrcGround.z > vMinZ.z);
    vSrcGround.z++;

    TraceLine( vDest, vDest + vMinZ, cTraceFilter.iTraceFlags, &cTraceFilter );
    Vector vDestGround = TraceResult().endpos;
    if ( vDestGround.z == vMinZ.z )
        return EReachNotReachable;
    vDestGround.z++;

    // Try to get up if needed.
    float fPlayerEye = CMod::GetVar( EModVarPlayerEye );
    float fPlayerEyeCrouched = CMod::GetVar( EModVarPlayerEyeCrouched );

    float zDiff = vDest.z - fPlayerEye - vDestGround.z;
    if ( zDiff >= CMod::GetVar( EModVarHeightForFallDamage ) )
        return EReachNotReachable;

    zDiff = vDest.z - vDestGround.z;
    if ( zDiff != fPlayerEye && zDiff != fPlayerEyeCrouched )
    {
        if ( !bCrouch )
        {
            // Try to stand up.
            vDest.z = vDestGround.z + fPlayerEye;
            TraceLine( vDest, vDestGround, cTraceFilter.iTraceFlags, &cTraceFilter );
            bCrouch = IsTraceHitSomething();
        }
        
        if ( bCrouch )
        {
            vDest.z = vDestGround.z + fPlayerEyeCrouched;
            TraceLine( vDest, vDestGround, cTraceFilter.iTraceFlags, &cTraceFilter );
            if ( IsTraceHitSomething() )
                return EReachNotReachable;
        }
        else
            bCrouch = false;
    }

    // Draw waypoints until ground.
    if ( bShowHelp )
    {
        DrawLine( vSrc, vSrcGround, iTextTime, 0xFF, 0xFF, 0xFF );
        DrawLine( vDest, vDestGround, iTextTime, 0xFF, 0xFF, 0xFF );
    }

    bool bAlreadyJumped = false;

    bool bVisible = CUtil::IsVisible( vSrcGround, vDestGround, EVisibilityOtherProps );
    Vector vHit = TraceResult().endpos;

    TReach iResult = EReachReachable;
    if ( bVisible ) // Check if can climb up slope.
    {
        if ( bShowHelp )
            DrawLine(vSrcGround, vDestGround, iTextTime, 0xFF, 0xFF, 0xFF);
        iResult = CanClimbSlope(vSrcGround, vDestGround);
    }
    else
    {
        // Need to trace several times to know if can jump up all obstacles.
        Vector vDirection = vDestGround - vSrcGround, vStart = vSrcGround;
        vDirection.z = 0.0f; // We need only X-Y direction.
        vDirection.NormalizeInPlace();
        //vDirection *= CMod::GetVar( EModVarPlayerWidth );

        bool bNeedJump = false;
        Vector vLastHit = vStart;

        while ( vHit != vDestGround )
        {
            // Trace from hit point to the floor.
            vSrcGround = vHit;
            vSrcGround.z = -CUtil::iHalfMaxMapSize;
            TraceLine( vHit, vSrcGround, cTraceFilter.iTraceFlags, &cTraceFilter );
            vSrcGround = TraceResult().endpos;

            bool bCanPassOrJump = CanPassOrJump(vStart, vSrcGround, vHit, vDirection, bNeedJump);

            if ( !bCanPassOrJump )
            {
                if ( bShowHelp )
                {
                    DrawLine( vStart, vHit, iTextTime, 0xFF, 0xFF, 0xFF );
                    DrawText( vHit, 0, iTextTime, 0xFF, 0xFF, 0xFF, "Too high to jump" );
                }
                return EReachNotReachable;
            }

            if ( bNeedJump )
            {
                if ( bAlreadyJumped )
                {
                    if ( bShowHelp )
                        CUtil::DrawText( vHit, 0, iTextTime, 0xFF, 0xFF, 0xFF, "Can't jump twice" );
                    return EReachNotReachable;
                }
                bAlreadyJumped = true;
            }

            // Trace from jumped obstacle to the ground.
            vSrcGround = vHit;
            vSrcGround.z = -CUtil::iHalfMaxMapSize;
            TraceLine( vHit, vSrcGround, cTraceFilter.iTraceFlags, &cTraceFilter );

            if ( bShowHelp )
                CUtil::DrawLine( vStart, TraceResult().endpos, iTextTime, 0xFF, 0xFF, 0xFF );
            vStart = TraceResult().endpos;

            // Trace from new origin to destination ground.
            TraceLine( vStart, vDestGround, cTraceFilter.iTraceFlags, &cTraceFilter );
            vHit = TraceResult().endpos;

            if ( vHit == vLastHit )
                return EReachNotReachable;
            vLastHit = vHit;
        }

        vSrcGround = vStart;
        //iResult = CanClimbSlope(vSrcGround, vDestGround); // ?
    }

    // Set text position.
    Vector vText = (vSrcGround + vDestGround) / 2;
    vText.z += iRandom * 10;

    switch (iResult)
    {
    case EReachReachable:
        if (bAlreadyJumped)
        {
            iResult = EReachNeedJump;
            if ( bShowHelp )
                CUtil::DrawText( vText, 0, iTextTime, 0xFF, 0xFF, 0xFF, "Reachable with one jump" );
        }
        else if ( bShowHelp )
            CUtil::DrawText(vText, 0, iTextTime, 0xFF, 0xFF, 0xFF, "Walkable");
        break;

    case EReachFallDamage:
        if ( bShowHelp )
            CUtil::DrawText(vText, 0, iTextTime, 0xFF, 0xFF, 0xFF, "Falling risk");
        break;

    case EReachNotReachable:
        if ( bShowHelp )
            CUtil::DrawText(vText, 0, iTextTime, 0xFF, 0xFF, 0xFF, "Too high gradient");
        break;

    default:
        BASSERT(false);
    }

    return iResult;
}

//----------------------------------------------------------------------------------------------------------------
void CUtil::TraceLine(const Vector& vSrc, const Vector& vDest, int mask, ITraceFilter *pFilter)
{
    Ray_t ray;
    memset(&m_TraceResult, 0, sizeof(trace_t));
    ray.Init( vSrc, vDest );
    CBotrixPlugin::pEngineTrace->TraceRay( ray, mask, pFilter, &m_TraceResult );
}

//----------------------------------------------------------------------------------------------------------------
void CUtil::TraceHull( const Vector& vSrc, const Vector& vDest, const Vector& vMins, const Vector& vMaxs, int mask, ITraceFilter *pFilter )
{
    Ray_t ray;
    memset( &m_TraceResult, 0, sizeof( trace_t ) );
    ray.Init( vSrc, vDest, vMins, vMaxs );
    CBotrixPlugin::pEngineTrace->TraceRay( ray, mask, pFilter, &m_TraceResult );
}

//----------------------------------------------------------------------------------------------------------------
Vector& CUtil::GetGroundVec( const Vector& vSrc, const Vector& vHullMins, const Vector& vHullMaxs )
{
    Ray_t ray;
    memset( &m_TraceResult, 0, sizeof( trace_t ) );
    Vector vDest = vSrc;
    vDest.z = -iHalfMaxMapSize;
    ray.Init( vSrc, vDest, vHullMins, vHullMaxs );
    CBotrixPlugin::pEngineTrace->TraceRay( ray, cTraceFilter.iTraceFlags, &cTraceFilter, &m_TraceResult );

    return m_TraceResult.endpos;
}

//----------------------------------------------------------------------------------------------------------------
edict_t* CUtil::GetEntityByUserId( int iUserId )
{
    for ( int i = 1; i <= CPlayers::Size(); i ++ )
    {
        edict_t* pEdict = CBotrixPlugin::pEngineServer->PEntityOfEntIndex(i);

        if ( pEdict && (CBotrixPlugin::pEngineServer->GetPlayerUserId(pEdict) == iUserId) )
            return pEdict;
    }
    return NULL;
}

//----------------------------------------------------------------------------------------------------------------
unsigned char pvs[MAX_MAP_CLUSTERS/8];

void CUtil::SetPVSForVector( const Vector& v )
{
    // Get visible clusters from player's position.
    int iClusterIndex = CBotrixPlugin::pEngineServer->GetClusterForOrigin( v );
    CBotrixPlugin::pEngineServer->GetPVSForCluster( iClusterIndex, sizeof(pvs), pvs );
}

bool CUtil::IsVisiblePVS( const Vector& v )
{
    return CBotrixPlugin::pEngineServer->CheckOriginInPVS( v, pvs, sizeof(pvs) );
}

//----------------------------------------------------------------------------------------------------------------
bool CUtil::IsNetworkable( edict_t* pEntity )
{
    IServerEntity* pServerEnt = pEntity->GetIServerEntity();
    return ( pServerEnt && (pServerEnt->GetNetworkable() != NULL) );
}

//----------------------------------------------------------------------------------------------------------------
void CUtil::EntityCenter( edict_t* pEntity, Vector& v )
{
    static float* fOrigin;
    BASSERT( IsNetworkable(pEntity), return );
    fOrigin = pEntity->GetIServerEntity()->GetNetworkable()->GetPVSInfo()->m_vCenter;

    v.x = fOrigin[0]; v.y = fOrigin[1]; v.z = fOrigin[2];
}


//================================================================================================================
bool CUtil::IsTouchBoundingBox2d( const Vector2D &a1, const Vector2D &a2, const Vector2D &bmins, const Vector2D &bmaxs )
{
    Vector2D amins = Vector2D(MIN2(a1.x,a2.x),MIN2(a1.y,a2.y));
    Vector2D amaxs = Vector2D(MAX2(a1.x,a2.x),MAX2(a1.y,a2.y));

    return (((bmins.x >= amins.x) && (bmins.y >= amins.y) && (bmins.x <= amaxs.x) && (bmins.y <= amaxs.y)) ||
            ((bmaxs.x >= amins.x) && (bmaxs.y >= amins.y) && (bmaxs.x <= amaxs.x) && (bmaxs.y <= amaxs.y)));
}

//----------------------------------------------------------------------------------------------------------------
bool CUtil::IsTouchBoundingBox3d( const Vector& a1, const Vector& a2, const Vector& bmins, const Vector& bmaxs )
{
    Vector amins = Vector(MIN2(a1.x,a2.x),MIN2(a1.y,a2.y),MIN2(a1.z,a2.z));
    Vector amaxs = Vector(MAX2(a1.x,a2.x),MAX2(a1.y,a2.y),MAX2(a1.z,a2.z));

    return (((bmins.x >= amins.x) && (bmins.y >= amins.y) && (bmins.z >= amins.z) && (bmins.x <= amaxs.x) && (bmins.y <= amaxs.y) && (bmins.z <= amaxs.z)) ||
            ((bmaxs.x >= amins.x) && (bmaxs.y >= amins.y) && (bmaxs.z >= amins.z) && (bmaxs.x <= amaxs.x) && (bmaxs.y <= amaxs.y) && (bmaxs.z <= amaxs.z)));
}

//----------------------------------------------------------------------------------------------------------------
bool CUtil::IsOnOppositeSides2d( const Vector2D &amins, const Vector2D &amaxs, const Vector2D &bmins, const Vector2D &bmaxs )
{
  float g = (amaxs.x - amins.x) * (bmins.y - amins.y) -
            (amaxs.y - amins.y) * (bmins.x - amins.x);

  float h = (amaxs.x - amins.x) * (bmaxs.y - amins.y) -
           ( amaxs.y - amins.y) * (bmaxs.x - amins.x);

  return (g * h) <= 0.0f;
}

//----------------------------------------------------------------------------------------------------------------
bool CUtil::IsOnOppositeSides3d( const Vector& amins, const Vector& amaxs, const Vector& bmins, const Vector& bmaxs )
{
    amins.Cross(bmins);
    amaxs.Cross(bmaxs);

  float g =(amaxs.x - amins.x) * (bmins.y - amins.y) * (bmins.z - amins.z) -
           (amaxs.z - amins.z) * (amaxs.y - amins.y) * (bmins.x - amins.x);

  float h =(amaxs.x - amins.x) * (bmaxs.y - amins.y) * (bmaxs.z - amins.z) -
           (amaxs.z - amins.z) * (amaxs.y - amins.y) * (bmaxs.x - amins.x);

  return (g * h) <= 0.0f;
}

//----------------------------------------------------------------------------------------------------------------
bool CUtil::IsLineTouch2d( const Vector2D &amins, const Vector2D &amaxs, const Vector2D &bmins, const Vector2D &bmaxs )
{
    return IsOnOppositeSides2d(amins,amaxs,bmins,bmaxs) && IsTouchBoundingBox2d(amins,amaxs,bmins,bmaxs);
}

//----------------------------------------------------------------------------------------------------------------
bool CUtil::IsLineTouch3d(const Vector& amins, const Vector& amaxs, const Vector& bmins, const Vector& bmaxs )
{
    return IsOnOppositeSides3d(amins,amaxs,bmins,bmaxs) && IsTouchBoundingBox3d(amins,amaxs,bmins,bmaxs);
}

//================================================================================================================
void CUtil::Message( good::TLogLevel iLevel, edict_t* pEntity, const char* szMsg )
{
    if ( pEntity && ( CBotrixPlugin::pEngineServer->IsDedicatedServer() ||
                      CPlayers::Get(pEntity) != CPlayers::GetListenServerClient() ) )
        CBotrixPlugin::pEngineServer->ClientPrintf(pEntity, szMsg);

    if ( iLevel >= iLogLevel )
    {
        switch ( iLevel )
        {
        case good::ELogLevelTrace:
        case good::ELogLevelDebug:
        case good::ELogLevelInfo:
            Msg(szMsg);
//            fprintf(stdout, "%s", szMsg);
            break;
        case good::ELogLevelWarning:
        case good::ELogLevelError:
            Warning(szMsg);
//            fprintf(stdout, "%s", szMsg);
            break;
        }
/*#ifdef GOOD_LOG_FLUSH
        fflush(stdout);
        fflush(stderr);
#endif*/
    }
}

//----------------------------------------------------------------------------------------------------------------
good::mutex cMessagesMutex;
int iQueueMessageStringSize = 0;
char szQueueMessageString[64*1024];

void CUtil::PutMessageInQueue( const char* fmt, ... )
{
    va_list argptr;

    va_start(argptr, fmt);
    cMessagesMutex.lock();

    int iSize = vsprintf( &szQueueMessageString[iQueueMessageStringSize], fmt, argptr );
    BASSERT( iSize >= 0, szQueueMessageString[iQueueMessageStringSize] = 0; return );
    iQueueMessageStringSize += iSize;

    cMessagesMutex.unlock();
    va_end(argptr);
}


//----------------------------------------------------------------------------------------------------------------
void CUtil::PrintMessagesInQueue()
{
    if ( (iQueueMessageStringSize > 0) && cMessagesMutex.try_lock() )
    {
        Message(good::ELogLevelInfo, NULL, szQueueMessageString);
        iQueueMessageStringSize = 0;
        cMessagesMutex.unlock();
    }
}

//----------------------------------------------------------------------------------------------------------------
FILE *CUtil::OpenFile( const good::string& szFile, const char *szMode )
{
    FILE *fp = fopen(szFile.c_str(), szMode);

    if ( fp == NULL )
    {
        good::file::make_folders( szFile.c_str() );
        fp = fopen(szFile.c_str(), szMode);
    }

    return fp;
}

//----------------------------------------------------------------------------------------------------------------
const good::string& CUtil::BuildFileName( const good::string& sFolder, const good::string& sFile, const good::string& sExtension )
{
    static good::string_buffer sbResult(szMainBuffer, iMainBufferSize, false);
    sbResult.erase();

    sbResult << CBotrixPlugin::instance->sBotrixPath;
    sbResult << PATH_SEPARATOR
             << sFolder << PATH_SEPARATOR << sFile;

    if ( sExtension.length() )
        sbResult << "." << sExtension;

    return sbResult;
}


//----------------------------------------------------------------------------------------------------------------
// Draw functions.
//----------------------------------------------------------------------------------------------------------------
void CUtil::DrawBeam( const Vector& v1, const Vector& v2, unsigned char iWidth, float fDrawTime, unsigned char r, unsigned char g, unsigned char b )
{
    pEffects->Beam( v1, v2, CWaypoint::iWaypointTexture,
        0, 0, 1, fDrawTime,  // haloIndex, frameStart, frameRate, LifeTime
        iWidth, iWidth, 255, // width, endWidth, fadeLength
        1, r, g, b, 200, 10  // noise, R, G, B, brightness, speed
    );
}

//----------------------------------------------------------------------------------------------------------------
void CUtil::DrawLine( const Vector& v1, const Vector& v2, float fDrawTime, unsigned char r, unsigned char g, unsigned char b )
{
    if (pVDebugOverlay)
        pVDebugOverlay->AddLineOverlay(v1, v2, r, g, b, false, fDrawTime);
}

//----------------------------------------------------------------------------------------------------------------
void CUtil::DrawBox( const Vector& vOrigin, const Vector& vMins, const Vector& vMaxs, float fDrawTime, unsigned char r, unsigned char g, unsigned char b, const QAngle& ang )
{
    if (pVDebugOverlay)
        pVDebugOverlay->AddBoxOverlay(vOrigin, vMins, vMaxs, ang, r, g, b, 0, fDrawTime);
}

//----------------------------------------------------------------------------------------------------------------
void CUtil::DrawText( const Vector& vOrigin, int iLine, float fDrawTime, unsigned char, unsigned char, unsigned char, const char* szText )
{
    if (pVDebugOverlay)
        pVDebugOverlay->AddTextOverlay(vOrigin, iLine, fDrawTime, szText );
}
