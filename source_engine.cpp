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
FILE* fLog = NULL;        // Log file.

//----------------------------------------------------------------------------------------------------------------
// If slope is less that 45 degrees (for HL2) then player can move forward (returns EReachReachable).
// If not and vSrc is higher, then player can take damage trying to get to lower ground (returns EReachFallDamage).
// If vSrc is lower in Z, and slope is more than 45 degrees then player can't reach destination (returns
// EReachNotReachable).
//----------------------------------------------------------------------------------------------------------------
TReach CanClimbSlope( Vector const& vSrc, Vector const& vDest )
{
    Vector vDiff = vDest - vSrc;
    QAngle ang;
    VectorAngles(vDiff, ang); // Get pitch to know if gradient is too big.

    if (ang.x < -CUtil::iPlayerMaxSlopeGradient) // Destination is higher and slope is more than 45 degrees, can't climb it.
        return EReachNotReachable;
    else if ( (ang.x > CUtil::iPlayerMaxSlopeGradient) &&       // Slope is more than 45 degrees.
              (vDiff.z > CUtil::iPlayerMaxHeightNoFallDamage) ) // Source is higher.
        return EReachFallDamage; // Can take damage at fall.
    else
        return EReachReachable; // Slope gradient is less than 45 degrees.
}

//----------------------------------------------------------------------------------------------------------------
// Returns true if can move forward when standing at vGround performing a jump (normal, with crouch o maybe just
// walking). At return vHit contains new coord which is where player can get after jump.
//----------------------------------------------------------------------------------------------------------------
bool CanPassOrJump( Vector& vStart, Vector& vGround, Vector& vHit, Vector& vDirectionInc, bool& bNeedJump )
{
    CTraceFilterWorldAndPropsOnly filter;
    float zDist = vHit.z - vGround.z; // Distance from ground to hit position.

    vHit = vGround;
    vHit += vDirectionInc;

    if ( zDist <= CUtil::iPlayerMaxObstacleHeight ) // Can walk?
    {
        vHit.z = vGround.z + CUtil::iPlayerMaxObstacleHeight + 1;
        CUtil::TraceLine(vStart, vHit, MASK_SOLID_BRUSHONLY, &filter); // Trace again.
        if ( !CUtil::IsTraceHitSomething() ) // We can stand on vHit after jump.
        {
            bNeedJump = false;
            return true;
        }
    }

    if ( zDist <= CUtil::iPlayerJumpCrouchHeight ) // Can perform jump?
    {
        vHit.z = vGround.z + CUtil::iPlayerJumpCrouchHeight + 1;
        CUtil::TraceLine(vStart, vHit, MASK_SOLID_BRUSHONLY, &filter); // Trace again.
        if ( !CUtil::IsTraceHitSomething() ) // We can stand on vHit after jump.
        {
            bNeedJump = true;
            return true;
        }
    }

    return false; // Can't jump over.
}










//****************************************************************************************************************
bool CUtil::bLogMessageToFile = false;
int CUtil::iPlayerHeight = 72;
int CUtil::iPlayerHeightCrouched = 36;
int CUtil::iPlayerWidth = 36;
Vector CUtil::vPlayerCollisionHull(iPlayerWidth, iPlayerWidth, iPlayerHeight);

int CUtil::iPlayerEyeLevel = 64;
int CUtil::iPlayerEyeLevelCrouched = 28;

int CUtil::iPlayerMaxObstacleHeight = 18;
int CUtil::iPlayerNormalJumpHeight = 20;
int CUtil::iPlayerJumpCrouchHeight = 56;

int CUtil::iPlayerRadius = ceilf( powf(2 * SQR(iPlayerWidth/2), 0.5f) );
int CUtil::iNearItemMaxDistanceSqr = SQR(256);
int CUtil::iItemPickUpDistance = 40;

int CUtil::iPlayerMaxSlopeGradient = 45;
int CUtil::iPlayerMaxHeightNoFallDamage = 185;

int CUtil::iPlayerMaxArmor = 100;
int CUtil::iPlayerMaxHealth = 100;

float CUtil::fMaxCrouchVelocity = 63.33f;
float CUtil::fMaxWalkVelocity = 150.0f;
float CUtil::fMaxRunVelocity = 190.0f;
float CUtil::fMaxSprintVelocity = 327.5f;
float CUtil::fMinNonStuckSpeed = fMaxCrouchVelocity / 2.0f;

int CUtil::iPointTouchSquaredXY = SQR(iPlayerWidth/4);
int CUtil::iPointTouchSquaredZ = SQR(iPlayerJumpCrouchHeight);
int CUtil::iPointTouchLadderSquaredZ = SQR(2);

const Vector CUtil::vZero(0, 0, 0);
const QAngle CUtil::angZero(0, 0, 0);


bool CUtil::m_bMessageUseTag = true;
trace_t CUtil::m_TraceResult;


class CVisibilityTraceFilter: public ITraceFilter
{
public:
    CVisibilityTraceFilter( TVisibilityFlags iFlags ): m_iFlags(iFlags)
    {
        m_bShouldHitEntity = FLAG_SOME_SET(FVisibilityEntity | FVisibilityProps, iFlags); // Props are entities.
        switch (iFlags)
        {
        case FVisibilityWorld:
            m_TraceType = TRACE_WORLD_ONLY;
            iTraceFlags = MASK_OPAQUE;
            break;
        case FVisibilityProps:
            m_TraceType = TRACE_EVERYTHING_FILTER_PROPS;
            iTraceFlags = MASK_OPAQUE;
            break;
        case FVisibilityEntity:
            m_TraceType = TRACE_ENTITIES_ONLY;
            iTraceFlags = MASK_NPCSOLID | MASK_PLAYERSOLID;
            break;
        default:
            m_TraceType = TRACE_EVERYTHING;
            if ( FLAG_SOME_SET(FVisibilityEntity, iFlags) )
                iTraceFlags = MASK_NPCSOLID | MASK_PLAYERSOLID;
            else
                iTraceFlags = MASK_SOLID_BRUSHONLY;
            break;
        }
    }

    virtual TraceType_t	GetTraceType() const { return m_TraceType; }
    virtual bool ShouldHitEntity( IHandleEntity */*pEntity*/, int /*contentsMask*/ ) { return m_bShouldHitEntity; }

    int iTraceFlags;

protected:
    TVisibilityFlags m_iFlags;
    TraceType_t m_TraceType;
    bool m_bShouldHitEntity;
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

//----------------------------------------------------------------------------------------------------------------
bool CUtil::IsRayHitsEntity( edict_t* pEntity, Vector const& vSrc, Vector const& vDest )
{
    COnlyOneEntityTraceFilter filter(pEntity);
    TraceLine(vSrc, vDest, MASK_OPAQUE, &filter);
    return m_TraceResult.fraction != 1.0f;
}

//----------------------------------------------------------------------------------------------------------------
bool CUtil::IsVisible( Vector const& vSrc, Vector const& vDest, TVisibilityFlags iFlags )
{
    CVisibilityTraceFilter filter(iFlags);
    TraceLine(vSrc, vDest, filter.iTraceFlags, &filter);
    return m_TraceResult.fraction == 1.0f;
}

//----------------------------------------------------------------------------------------------------------------
bool CUtil::IsVisible( Vector const& vSrc, edict_t* pDest )
{
    //CTraceFilterHitAll filter;
    CTraceFilterWorldAndPropsOnly filter;
    Vector v;
    EntityHead(pDest, v);
    TraceLine(vSrc, v, MASK_SOLID_BRUSHONLY, &filter);
    return m_TraceResult.fraction == 1.0f;
}

//----------------------------------------------------------------------------------------------------------------
TReach CUtil::GetReachableInfoFromTo( Vector const& vSrc, Vector const& vDest, float fDistance )
{
    static int iRandom = 0;      // This function may be called 2 times with (v1, v2) and (v2,v1)
    iRandom = (iRandom + 1) & 1; // so iRandom is to draw text higher that previous time.

    if ( fDistance == 0.0f )
        fDistance = vSrc.DistTo(vDest);

    if ( fDistance > CWaypoint::MAX_RANGE )
        return EReachNotReachable;

    if ( !CUtil::IsVisible(vSrc, vDest) )
        return EReachNotReachable;

    // Check if can swim there first.
    if ( (CBotrixPlugin::pEngineTrace->GetPointContents(vSrc) == CONTENTS_WATER) &&
         (CBotrixPlugin::pEngineTrace->GetPointContents(vDest) == CONTENTS_WATER) )
        return EReachReachable;

    TReach iResult = EReachReachable;

    Vector vMinZ(0, 0, -iHalfMaxMapSize);
    CTraceFilterWorldAndPropsOnly filter;

    // Get ground positions.
    TraceLine(vSrc, vSrc + vMinZ, MASK_SOLID_BRUSHONLY, &filter);
    Vector vSrcGround = TraceResult().endpos;
    vSrcGround.z++;

    TraceLine(vDest, vDest + vMinZ, MASK_SOLID_BRUSHONLY, &filter);
    Vector vDestGround = TraceResult().endpos;
    vDestGround.z++;

    // Draw waypoints until ground.
    DrawLine(vSrc, vSrcGround, iTextTime, 0xFF, 0xFF, 0xFF);
    DrawLine(vDest, vDestGround, iTextTime, 0xFF, 0xFF, 0xFF);

    bool bAlreadyJumped = false;

    bool bVisible = CUtil::IsVisible(vSrcGround, vDestGround);
    Vector vHit = TraceResult().endpos, vT;

    if ( bVisible ) // Check if can climb up slope.
    {
        DrawLine(vSrcGround, vDestGround, iTextTime, 0xFF, 0xFF, 0xFF);
        iResult = CanClimbSlope(vSrcGround, vDestGround);
    }
    else
    {
        // Need to trace several times to know if can jump up all obstacles.
        Vector vDirection = vDestGround - vSrcGround, vStart = vSrcGround;
        vDirection.z = 0.0f; // We need only X-Y direction.
        vDirection.NormalizeInPlace();

        bool bNeedJump = false, bSourceHigher = vDirection.z < 0;

        while ( vHit != vDestGround )
        {
            // Trace from hit point to the floor.
            vSrcGround = vHit;
            vSrcGround.z = -CUtil::iHalfMaxMapSize;
            TraceLine(vHit, vSrcGround, MASK_SOLID_BRUSHONLY, &filter);
            vSrcGround = TraceResult().endpos;

            if ( bSourceHigher )
            {
            }
            else
            {
            }

            bool bCanPassOrJump = CanPassOrJump(vStart, vSrcGround, vHit, vDirection, bNeedJump);

            if ( !bCanPassOrJump )
            {
                DrawLine(vStart, vHit, iTextTime, 0xFF, 0xFF, 0xFF);
                DrawText(vHit, 0, iTextTime, 0xFF, 0xFF, 0xFF, "Too high to jump");
                return EReachNotReachable;
            }

            if ( bNeedJump )
            {
                if ( bAlreadyJumped )
                {
                    CUtil::DrawText(vHit, 0, iTextTime, 0xFF, 0xFF, 0xFF, "Can't jump twice");
                    return EReachNotReachable;
                }
                bAlreadyJumped = true;
            }

            // Trace from jumped obstacle to the ground.
            vSrcGround = vHit;
            vSrcGround.z = -CUtil::iHalfMaxMapSize;
            TraceLine(vHit, vSrcGround, MASK_SOLID_BRUSHONLY, &filter);

            CUtil::DrawLine(vStart, TraceResult().endpos, iTextTime, 0xFF, 0xFF, 0xFF);
            vStart = TraceResult().endpos;

            // Trace from new origin to destination ground.
            TraceLine(vStart, vDestGround, MASK_SOLID_BRUSHONLY, &filter);
            vHit = TraceResult().endpos;
        }

        vSrcGround = vStart;
        iResult = CanClimbSlope(vSrcGround, vDestGround);

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
            CUtil::DrawText(vText, 0, iTextTime, 0xFF, 0xFF, 0xFF, "Reachable with one jump");
        }
        else
            CUtil::DrawText(vText, 0, iTextTime, 0xFF, 0xFF, 0xFF, "Walkable");
        break;

    case EReachFallDamage:
        CUtil::DrawText(vText, 0, iTextTime, 0xFF, 0xFF, 0xFF, "Falling risk");
        break;

    case EReachNotReachable:
        CUtil::DrawText(vText, 0, iTextTime, 0xFF, 0xFF, 0xFF, "Too high gradient");
        break;

    default:
        DebugAssert(false);
    }

    return iResult;
}

//----------------------------------------------------------------------------------------------------------------
void CUtil::TraceLine(Vector const& vSrc, Vector const& vDest, int mask, ITraceFilter *pFilter)
{
    Ray_t ray;
    memset(&m_TraceResult, 0, sizeof(trace_t));
    ray.Init( vSrc, vDest );
    CBotrixPlugin::pEngineTrace->TraceRay( ray, mask, pFilter, &m_TraceResult );
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
    DebugAssert( IsNetworkable(pEntity), return );
    fOrigin = pEntity->GetIServerEntity()->GetNetworkable()->GetPVSInfo()->m_vCenter;

    v.x = fOrigin[0]; v.y = fOrigin[1]; v.z = fOrigin[2];
}


//================================================================================================================
bool CUtil::IsTouchBoundingBox2d( const Vector2D &a1, const Vector2D &a2, const Vector2D &bmins, const Vector2D &bmaxs )
{
    Vector2D amins = Vector2D(min(a1.x,a2.x),min(a1.y,a2.y));
    Vector2D amaxs = Vector2D(max(a1.x,a2.x),max(a1.y,a2.y));

    return (((bmins.x >= amins.x) && (bmins.y >= amins.y) && (bmins.x <= amaxs.x) && (bmins.y <= amaxs.y)) ||
            ((bmaxs.x >= amins.x) && (bmaxs.y >= amins.y) && (bmaxs.x <= amaxs.x) && (bmaxs.y <= amaxs.y)));
}

//----------------------------------------------------------------------------------------------------------------
bool CUtil::IsTouchBoundingBox3d( Vector const& a1, Vector const& a2, Vector const& bmins, Vector const& bmaxs )
{
    Vector amins = Vector(min(a1.x,a2.x),min(a1.y,a2.y),min(a1.z,a2.z));
    Vector amaxs = Vector(max(a1.x,a2.x),max(a1.y,a2.y),max(a1.z,a2.z));

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
bool CUtil::IsOnOppositeSides3d( Vector const& amins, Vector const& amaxs, Vector const& bmins, Vector const& bmaxs )
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
bool CUtil::IsLineTouch3d(Vector const& amins, Vector const& amaxs, Vector const& bmins, Vector const& bmaxs )
{
    return IsOnOppositeSides3d(amins,amaxs,bmins,bmaxs) && IsTouchBoundingBox3d(amins,amaxs,bmins,bmaxs);
}

//================================================================================================================
char szMessageString[16*1024];
const int iSplitSize = 4*1024;
void CUtil::Message( edict_t* pEntity, const char* fmt, ... )
{
    int iStart = 0;
    if ( m_bMessageUseTag )
    {
        strcpy(szMessageString, "[Botrix] ");
        iStart = 9; // 9 is length of "[Botrix ]".
    }

    va_list argptr;

    va_start(argptr, fmt);
    int iTotal = vsprintf(&szMessageString[iStart], fmt, argptr);
    va_end(argptr);

    // Log to file.
    if ( bLogMessageToFile )
    {
        if ( fLog == NULL )
        {
            fLog = fopen("botrix.log", "a");
            if ( fLog )
                fprintf(fLog, "\n--------------------------------------------------------------------------------------------\n");
        }
        if ( fLog )
        {
            fprintf(fLog, "%s\n", szMessageString);
        }
    }

#if (defined(DEBUG) || defined(_DEBUG)) && defined(BOTRIX_MESSAGE_USE_TIME)
    static char sTime[24];
#endif

    if ( pEntity )
    {
        if ( m_bMessageUseTag )
        {
#if (defined(DEBUG) || defined(_DEBUG)) && defined(BOTRIX_MESSAGE_USE_TIME)
            sprintf(sTime, "%.5f: ", CBotrixPlugin::fTime);
            CBotrixPlugin::pEngineServer->ClientPrintf(pEntity, sTime);
#endif
        }
        int i = 0;
        while ( i < iTotal )
        {
            int iEnd = i + iSplitSize;
            if ( iEnd < iTotal-1 )
            {
                char c = szMessageString[iEnd];
                szMessageString[iEnd] = 0;
                CBotrixPlugin::pEngineServer->ClientPrintf(pEntity, &szMessageString[i]);
                szMessageString[iEnd] = c;
            }
            else
                CBotrixPlugin::pEngineServer->ClientPrintf(pEntity, &szMessageString[i]);
            i = iEnd;
        }

        CBotrixPlugin::pEngineServer->ClientPrintf(pEntity, "\n");
    }
    else
    {
        if ( m_bMessageUseTag )
        {
#if (defined(DEBUG) || defined(_DEBUG)) && defined(BOTRIX_MESSAGE_USE_TIME)
            sprintf(sTime, "%.5f: ", CBotrixPlugin::fTime);
            Msg(sTime);
#endif
        }
        int i = 0;
        while ( i < iTotal )
        {
            int iEnd = i + iSplitSize;
            if ( iEnd < iTotal-1 )
            {
                char c = szMessageString[iEnd];
                szMessageString[iEnd] = 0;
                Msg("%s", &szMessageString[i]);
                szMessageString[iEnd] = c;
            }
            else
                Msg("%s", &szMessageString[i]);
            i = iEnd;
        }
        Msg("\n");
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
    DebugAssert( iSize >= 0, szQueueMessageString[iQueueMessageStringSize] = 0; return );
    iQueueMessageStringSize += iSize;

    cMessagesMutex.unlock();
    va_end(argptr);
}


//----------------------------------------------------------------------------------------------------------------
void CUtil::PrintMessagesInQueue()
{
    if ( (iQueueMessageStringSize > 0) && cMessagesMutex.try_lock() )
    {
        Message(NULL, szQueueMessageString);
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

    sbResult << CBotrixPlugin::instance->sBotrixPath << PATH_SEPARATOR << sFolder << PATH_SEPARATOR
             << CBotrixPlugin::instance->sGameFolder << PATH_SEPARATOR
             << CBotrixPlugin::instance->sModFolder  << PATH_SEPARATOR << sFile;

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
    pVDebugOverlay->AddLineOverlay(v1, v2, r, g, b, false, fDrawTime);
}

//----------------------------------------------------------------------------------------------------------------
void CUtil::DrawBox( const Vector& vOrigin, const Vector& vMins, const Vector& vMaxs, float fDrawTime, unsigned char r, unsigned char g, unsigned char b, const QAngle& ang )
{
    pVDebugOverlay->AddBoxOverlay(vOrigin, vMins, vMaxs, ang, r, g, b, 0, fDrawTime);
}

//----------------------------------------------------------------------------------------------------------------
void CUtil::DrawText( const Vector& vOrigin, int iLine, float fDrawTime, unsigned char, unsigned char, unsigned char, const char* szText )
{
    pVDebugOverlay->AddTextOverlay(vOrigin, iLine, fDrawTime, szText );
}
