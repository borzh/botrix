#include "cbase.h"

// Ugly fix for Source Engine.
#undef MINMAX_H
#undef min
#undef max
#include "bot.h"
#include "clients.h"
#include "item.h"
#include "players.h"
#include "server_plugin.h"
#include "source_engine.h"
#include "type2string.h"
#include "waypoint.h"

//#if !defined(DEBUG) && !defined(_DEBUG)
//#include "public/isaverestore.h"
//#include "game/shared/ehandle.h"
//#include "game/shared/groundlink.h"
//#include "game/shared/touchlink.h"
//#include "game/shared/takedamageinfo.h"
//#include "game/shared/predictableid.h"
//#include "game/shared/predictable_entity.h"
//#include "public/vmatrix.h"
//#include "game/shared/shareddefs.h"
//#include "game/server/util.h"
//#include "game/server/variant_t.h"
//#include "game/shared/baseentity_shared.h"
//#include "public/networkvar.h"
//#include "game/server/BaseEntity.h"
//#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#if defined(DEBUG) || defined (_DEBUG)
#	define ItemMessage(...) CUtil::Message(NULL, __VA_ARGS__)
#else
#	define ItemMessage(...)
#endif

extern IVDebugOverlay* pVDebugOverlay;

extern char* szMainBuffer;
extern int iMainBufferSize;

char szValueBuffer[16];

#define BOTRIX_ENTITY_USE_KEY_VALUE

//================================================================================================================
inline int GetEntityFlags( IServerEntity* pServerEntity )
{
    // ObjectCaps(), GetMaxHealth(), IsAlive().
    CBaseEntity* pEntity = pServerEntity->GetBaseEntity();
#ifdef BOTRIX_ENTITY_USE_KEY_VALUE
    pEntity->GetKeyValue("effects", szValueBuffer, 16);
    int flags = atoi(szValueBuffer);
    return flags;
#else
    return pEntity->GetEffects();
#endif
}

inline int GetEntityHealth( IServerEntity* pServerEntity )
{
    CBaseEntity* pEntity = pServerEntity->GetBaseEntity();
#ifdef BOTRIX_ENTITY_USE_KEY_VALUE
    pEntity->GetKeyValue("health", szValueBuffer, 16);
    int health = atoi(szValueBuffer);
    return health;
#else
    return pEntity->GetHealth();
#endif
}

inline bool IsEntityOnMap( IServerEntity* pServerEntity )
{
     // EF_BONEMERGE is set for weapon, when picked up.
    return FLAG_CLEARED( EF_NODRAW | EF_BONEMERGE, GetEntityFlags(pServerEntity) );
    //return !FLAG_SOME_SET( EF_NODRAW | EF_BONEMERGE, GetEntityFlags(pServerEntity) );
}

inline bool IsEntityTaken( IServerEntity* pServerEntity )
{
     // EF_BONEMERGE is set for weapon, when picked up.
    return FLAG_SOME_SET( EF_BONEMERGE, GetEntityFlags(pServerEntity) );
}

inline bool IsEntityBreakable( IServerEntity* pServerEntity )
{
    return GetEntityHealth(pServerEntity) != 0;
}

inline string_t GetEntityName( IServerEntity* pServerEntity )
{
    CBaseEntity* pEntity = pServerEntity->GetBaseEntity();
    return pEntity->GetEntityName();
}


//================================================================================================================
bool CEntity::IsOnMap() const
{
    return IsEntityOnMap(pEdict->GetIServerEntity());
}

//----------------------------------------------------------------------------------------------------------------
bool CEntity::IsBreakable() const
{
    return IsEntityBreakable(pEdict->GetIServerEntity());
}


//================================================================================================================
good::vector<CEntity> CItems::m_aItems[EEntityTypeTotal];            // Array of items.
good::vector<CEntityClass> CItems::m_aItemClasses[EEntityTypeTotal]; // Array of item classes.
TEntityIndex CItems::m_iFreeIndex[EEntityTypeTotal];                 // First free weapon index.
int CItems::m_iFreeEntityCount[EEntityTypeTotal];                    // Count of unused entities.

good::vector<edict_t*> CItems::m_aOthers(1024);                      // Array of other entities.

#ifndef SOURCE_ENGINE_2006
good::vector<edict_t*> CItems::m_aNewEntities(16);
#endif

good::vector< good::pair<good::string, TEntityFlags> > CItems::m_aObjectFlagsForModels(4);
good::bitset CItems::m_aUsedItems(MAX_EDICTS);
int CItems::m_iCurrentEntity;
bool CItems::m_bMapLoaded = false;


//----------------------------------------------------------------------------------------------------------------
TEntityIndex CItems::GetNearestItem( TEntityType iEntityType, const Vector& vOrigin, const good::vector<CPickedItem>& aSkip, const CEntityClass* pClass )
{
    good::vector<CEntity>& aItems = m_aItems[iEntityType];

    TEntityIndex iResult = -1;
    float fSqrDistResult = 0.0f;

    for ( size_t i = 0; i < aItems.size(); ++i )
    {
        CEntity& cItem = aItems[i];
        if ( (cItem.pEdict == NULL) || !CWaypoint::IsValid(cItem.iWaypoint) )
            continue;

        if ( pClass && (pClass != cItem.pItemClass) ) // If item is already added, we have engine name.
            continue;

        CPickedItem cPickedItem( iEntityType, i );
        if ( find(aSkip.begin(), aSkip.end(), cPickedItem) != aSkip.end() ) // Skip this item.
            continue;

        float fSqrDist = vOrigin.DistToSqr( cItem.CurrentPosition() );
        if ( (iResult == -1) || (fSqrDist < fSqrDistResult) )
        {
            iResult = i;
            fSqrDistResult = fSqrDist;
        }
    }

    return iResult;
}


#ifndef SOURCE_ENGINE_2006

//----------------------------------------------------------------------------------------------------------------
void CItems::Allocated( edict_t* pEdict )
{
    if ( m_bMapLoaded )
        m_aNewEntities.push_back(pEdict);
}


//----------------------------------------------------------------------------------------------------------------
void CItems::Freed( const edict_t* pEdict )
{
    if ( !m_bMapLoaded )
        return;

    int iIndex = CBotrixPlugin::pEngineServer->IndexOfEdict(pEdict);
    // Check only server entities.
    if ( !m_aUsedItems.test(iIndex) )
        return;

    m_aUsedItems.reset(iIndex);
    good::vector<CEntity>& aWeapons = m_aItems[EEntityTypeWeapon];
    for ( TEntityIndex i=0; i < (int)aWeapons.size(); ++i )
        if ( aWeapons[i].pEdict == pEdict )
        {
            aWeapons[i].pEdict = NULL;
            m_iFreeIndex[EEntityTypeWeapon] = i;
            return;
        }
    DebugAssert(false); // Only weapons are allocated/deallocated while map is running.
}
#endif // SOURCE_ENGINE_2006

//----------------------------------------------------------------------------------------------------------------
void CItems::MapUnloaded()
{
    for ( TEntityType iEntityType = 0; iEntityType < EEntityTypeTotal; ++iEntityType )
    {
        good::vector<CEntityClass>& aClasses = m_aItemClasses[iEntityType];
        good::vector<CEntity>& aItems = m_aItems[iEntityType];

        aItems.clear();
        m_iFreeIndex[iEntityType] = -1;      // Invalidate free entity index.

        for ( size_t i = 0; i < aClasses.size(); ++i )
        {
            aClasses[i].szEngineName = NULL; // Invalidate class name, because it was loaded in previous map.
            aItems.reserve(64);              // At least.
        }
    }

    m_aOthers.clear();
    m_aUsedItems.reset();
    m_bMapLoaded = false;
}


//----------------------------------------------------------------------------------------------------------------
void CItems::MapLoaded()
{
    m_iCurrentEntity = CPlayers::Size()+1;

    // 0 is world, 1..max players are players. Other entities are from indexes above max players.
    int iCount = CBotrixPlugin::pEngineServer->GetEntityCount();
    for ( int i = m_iCurrentEntity; i < iCount; ++i ) //
    {
        edict_t* pEdict = CBotrixPlugin::pEngineServer->PEntityOfEntIndex(i);
        if ( (pEdict == NULL) || pEdict->IsFree() )
            continue;

        // Check items and objects.
        CheckNewEntity(pEdict);
    }
    m_bMapLoaded = true;
}


//----------------------------------------------------------------------------------------------------------------
void CItems::Update()
{
#ifdef SOURCE_ENGINE_2006
    // Source engine 2007 uses IServerPluginCallbacks::OnEdictAllocated instead of checking all array of edicts.

    // Update weapons we have in items array.
    good::vector<CEntity>& aWeapons = m_aItems[EEntityTypeWeapon];
    for ( TEntityIndex i = 0; i < aWeapons.size(); ++i )
    {
        CEntity& cEntity = aWeapons[i];
        edict_t* pEdict = cEntity.pEdict;
        if ( pEdict == NULL )
        {
            m_iFreeIndex[EEntityTypeWeapon] = i;
            continue;
        }

        IServerEntity* pServerEntity = pEdict->GetIServerEntity();
        if ( pEdict->IsFree() || (pServerEntity == NULL) )
        {
            cEntity.pEdict = NULL;
            m_iFreeIndex[EEntityTypeWeapon] = i;
            m_aUsedItems.clear( CBotrixPlugin::pEngineServer->IndexOfEdict(pEdict) );
        }
        else if ( IsEntityTaken(pServerEntity) ) // Weapon still belongs to some player.
            FLAG_SET(FTaken, cEntity.iFlags);
        else if ( FLAG_ALL_SET(FTaken, cEntity.iFlags) && IsEntityOnMap(pServerEntity) )
        {
            FLAG_CLEAR(FTaken, cEntity.iFlags);
            cEntity.vOrigin = cEntity.CurrentPosition();
            cEntity.iWaypoint = CWaypoints::GetNearestWaypoint( cEntity.vOrigin, true, CEntity::iMaxDistToWaypoint );
        }
    }

    // 0 is world, 1..max players are players. Other entities are from indexes above max players.
    int iCount = CBotrixPlugin::pEngineServer->GetEntityCount();

    // Update weapon entities on map.
    int iTo = m_iCurrentEntity + m_iCheckEntitiesPerFrame;
    if ( iTo > iCount )
        iTo = iCount;

    for ( TEntityIndex i = m_iCurrentEntity; i < iTo; ++i )
    {
        if ( m_aUsedItems.test(i) )
            continue;

        edict_t* pEdict = CBotrixPlugin::pEngineServer->PEntityOfEntIndex(i);
        if ( (pEdict == NULL) || pEdict->IsFree() )
            continue;

        // Check only server entities.
        IServerEntity* pServerEntity = pEdict->GetIServerEntity();
        if ( pServerEntity == NULL )
            continue;

        // Check only for new weapons, because new weapon instance is created when weapon is picked up.
        CEntityClass* pWeaponClass;
        TEntityType iEntityType = GetEntityType(pEdict->GetClassName(), pWeaponClass, EEntityTypeWeapon, EEntityTypeWeapon+1);

        if ( iEntityType == EEntityTypeWeapon )
            AddItem( EEntityTypeWeapon, pEdict, pWeaponClass, pServerEntity );
    }
    m_iCurrentEntity = (iTo == iCount) ? CPlayers::Size()+1: iTo;
#else
    for ( size_t i = 0; i < m_aNewEntities.size(); ++i )
        CheckNewEntity( m_aNewEntities[i] );
    m_aNewEntities.clear();
#endif // SOURCE_ENGINE_2006
}


//----------------------------------------------------------------------------------------------------------------
TEntityType CItems::GetEntityType( const char* szClassName, CEntityClass* & pEntityClass, TEntityType iFrom, TEntityType iTo, bool bFastCmp )
{
    for ( TEntityType iEntityType = iFrom; iEntityType < iTo; ++iEntityType )
    {
        good::vector<CEntityClass>& aItemClasses = m_aItemClasses[iEntityType];

        for ( size_t j = 0; j < aItemClasses.size(); ++j )
        {
            CEntityClass& cEntityClass = aItemClasses[j];

            if ( cEntityClass.szEngineName ) // Fast compare?
            {
                if ( cEntityClass.szEngineName == szClassName ) // Compare by pointer, faster.
                {
                    pEntityClass = &cEntityClass;
                    return iEntityType;
                }
            }
            else if ( !bFastCmp && (cEntityClass.sClassName == szClassName) ) // Full string content comparison.
            {
                cEntityClass.szEngineName = szClassName; // Save engine string.
                pEntityClass = &cEntityClass;
                return iEntityType;
            }
        }
    }

    return EOtherEntityType;
}


//----------------------------------------------------------------------------------------------------------------
void CItems::CheckNewEntity( edict_t* pEdict )
{
    // Check only server entities.
    IServerEntity* pServerEntity = pEdict->GetIServerEntity();
    if ( pServerEntity == NULL )
        return;

    const char* szClassName = pEdict->GetClassName();
    CEntityClass* pItemClass;
    TEntityType iEntityType = GetEntityType(szClassName, pItemClass, 0, EEntityTypeTotal);
    if ( iEntityType == EOtherEntityType )
        m_aOthers.push_back(pEdict);
    else if ( iEntityType == EEntityTypeObject )
        AddObject( pEdict, pItemClass, pServerEntity );
    else
    {
        TEntityIndex iIndex = AddItem( iEntityType, pEdict, pItemClass, pServerEntity );
        DebugAssert(iIndex >= 0, return);
        CEntity& cItem = m_aItems[iEntityType][iIndex];

        ItemMessage("New item: %s %d (%s), waypoint %d (%s).", CTypeToString::EntityTypeToString(iEntityType).c_str(), iIndex,
                    pEdict->GetClassName(), cItem.iWaypoint,
                    CWaypoints::IsValid(cItem.iWaypoint)
                        ? CTypeToString::WaypointFlagsToString( CWaypoints::Get(cItem.iWaypoint).iFlags ).c_str()
                        : "");

        if (  !m_bMapLoaded && FLAG_CLEARED(FTaken, cItem.iFlags) ) // Item should have waypoint near.
        {
            if ( !CWaypoint::IsValid(cItem.iWaypoint) )
            {
                CUtil::Message(NULL, "Warning: entity %s %d doesn't have waypoint close.", pEdict->GetClassName(), iIndex+1);
                TWaypointId iWaypoint = CWaypoints::GetNearestWaypoint( cItem.vOrigin );
                if ( CWaypoint::IsValid(iWaypoint) )
                    CUtil::Message(NULL, "\tNearest waypoint %d.", iWaypoint);
            }
            else if ( iEntityType == EEntityTypeDoor && !CWaypoint::IsValid((TWaypointId)cItem.pArguments) )
                CUtil::Message(NULL, "Door %d doesn't have 2 waypoints near.", iIndex);
        }

#ifdef SOURCE_ENGINE_2006
        // Weapon entities are allocated / deallocated when respawned / owner killed.
        if ( iEntityType != EEntityTypeWeapon )
#endif
        {
            int iIndex = CBotrixPlugin::pEngineServer->IndexOfEdict(pEdict);
            m_aUsedItems.set(iIndex);
        }
    }

}


//----------------------------------------------------------------------------------------------------------------
TEntityIndex CItems::InsertEntity( int iEntityType, const CEntity& cEntity )
{
    good::vector<CEntity>& aItems = m_aItems[iEntityType];

    if ( m_bMapLoaded ) // Check if there are free space in items array.
    {
        if ( m_iFreeIndex[iEntityType] != -1 )
        {
            int iIndex = m_iFreeIndex[iEntityType];
            m_iFreeIndex[iEntityType] = -1;
            aItems[iIndex] = cEntity;
            return iIndex;
        }

        for ( size_t i=0; i < aItems.size(); ++i ) // TODO: add free count.
            if ( aItems[i].pEdict == NULL )
            {
                aItems[i] = cEntity;
                return i;
            }
    }

    aItems.push_back( cEntity );
    return aItems.size() - 1;
}

//----------------------------------------------------------------------------------------------------------------
void CItems::AutoWaypointPathFlagsForEntity( TEntityType iEntityType, TEntityIndex iIndex, CEntity& cEntity )
{
    TWaypointId iWaypoint = cEntity.iWaypoint;
    if ( (iEntityType == EEntityTypeButton) && (iWaypoint != EWaypointIdInvalid) )
    {
        // Set waypoint argument to button.
        CWaypoint& cWaypoint = CWaypoints::Get(iWaypoint);
        FLAG_SET(FWaypointButton, cWaypoint.iFlags);
        CWaypoint::SetButton( iIndex+1, cWaypoint.iArgument );
    }

    // Check 2nd nearest waypoint for door.
    if ( iEntityType == EEntityTypeDoor )
    {
        if ( iWaypoint != EWaypointIdInvalid )
        {
            good::bitset cOmitWaypoints(CWaypoints::Size());
            cOmitWaypoints.set(iWaypoint);
            iWaypoint = CWaypoints::GetNearestWaypoint( cEntity.vOrigin, &cOmitWaypoints, true, CEntity::iMaxDistToWaypoint );
        }
        cEntity.pArguments = (void*)iWaypoint;

        // Set door for paths between these two waypoints.
        if ( iWaypoint != EWaypointIdInvalid )
        {
            CWaypointPath* pPath = CWaypoints::GetPath(iWaypoint, cEntity.iWaypoint); // From -> To.
            if ( pPath )
            {
                FLAG_SET(FPathDoor, pPath->iFlags);
                pPath->iArgument = iIndex+1;
            }

            pPath = CWaypoints::GetPath(cEntity.iWaypoint, iWaypoint); // To -> From.
            if ( pPath )
            {
                FLAG_SET(FPathDoor, pPath->iFlags);
                pPath->iArgument = iIndex+1;
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------
TEntityIndex CItems::AddItem( TEntityType iEntityType, edict_t* pEdict, CEntityClass* pItemClass, IServerEntity* pServerEntity )
{
    ICollideable* pCollidable = pServerEntity->GetCollideable();
    DebugAssert( pCollidable, return -1 );

    const Vector& vItemOrigin = pCollidable->GetCollisionOrigin();

    float fRadiusSqr = pItemClass->fRadiusSqr;

    if ( fRadiusSqr == 0.0f )
    {
        // Calculate object radius.
        float fMaxsRadiusSqr = pCollidable->OBBMaxs().LengthSqr();
        float fMinsRadiusSqr = pCollidable->OBBMins().LengthSqr();
        fRadiusSqr = powf( MAX2(fMaxsRadiusSqr, fMinsRadiusSqr), 0.5f );
        fRadiusSqr += CUtil::iPlayerRadius + CUtil::iItemPickUpDistance;
        fRadiusSqr *= fRadiusSqr;
        pItemClass->fRadiusSqr = fRadiusSqr;
    }

    int iFlags = pItemClass->iFlags;
    TWaypointId iWaypoint = -1;

    if ( !IsEntityOnMap(pServerEntity) || IsEntityTaken(pServerEntity) )
        FLAG_SET(FTaken, iFlags);
    else
        iWaypoint = CWaypoints::GetNearestWaypoint( vItemOrigin, NULL, true, CEntity::iMaxDistToWaypoint );

    TEntityIndex iIndex = InsertEntity( iEntityType, CEntity(pEdict, iFlags, fRadiusSqr, pItemClass, vItemOrigin, iWaypoint) );

    CEntity& cEntity = m_aItems[iEntityType][iIndex];
    AutoWaypointPathFlagsForEntity( iEntityType, iIndex, cEntity );

    return iIndex;
}


//----------------------------------------------------------------------------------------------------------------
void CItems::AddObject( edict_t* pEdict, const CEntityClass* pObjectClass, IServerEntity* pServerEntity )
{
    // Calculate object radius.
    ICollideable* pCollidable = pServerEntity->GetCollideable();

    float fMaxsRadiusSqr = pCollidable->OBBMaxs().LengthSqr();
    float fMinsRadiusSqr = pCollidable->OBBMins().LengthSqr();
    fMaxsRadiusSqr = powf( MAX2(fMaxsRadiusSqr, fMinsRadiusSqr), 0.5f );
    fMaxsRadiusSqr += CUtil::iPlayerRadius;
    fMaxsRadiusSqr *= fMaxsRadiusSqr;

    int iFlags = pObjectClass->iFlags;

    // Get object flags.
    if ( m_aObjectFlagsForModels.size() )
    {
        good::string sModel = STRING( pServerEntity->GetModelName() );
        for ( size_t i = 0; i < m_aObjectFlagsForModels.size(); ++i )
            if ( sModel.find( m_aObjectFlagsForModels[i].first ) != good::string::npos )
            {
                FLAG_SET(m_aObjectFlagsForModels[i].second, iFlags);
                break;
            }
    }

    good::vector<CEntity>& aItems = m_aItems[EEntityTypeObject];
    CEntity cObject( pEdict, iFlags, fMaxsRadiusSqr, pObjectClass, pCollidable->GetCollisionOrigin(), -1 );
    if ( m_bMapLoaded ) // Check if there are free space in items array.
    {
        if ( m_iFreeIndex[EEntityTypeObject] != -1 )
        {
            CEntity& cResult = aItems[m_iFreeIndex[EEntityTypeObject]];
            m_iFreeIndex[EEntityTypeObject] = -1;
            cResult = cObject;
            return;
        }

        for ( size_t i=0; i < aItems.size(); ++i )
            if ( aItems[i].pEdict == NULL )
            {
                aItems[i] = cObject;
                return;
            }
    }
    aItems.push_back( cObject );
}

//----------------------------------------------------------------------------------------------------------------
bool CItems::IsDoorOpened( TEntityIndex iDoor )
{
    const CEntity& cDoor = m_aItems[EEntityTypeDoor][iDoor];
    TWaypointId w1 = cDoor.iWaypoint, w2 = (TWaypointId)cDoor.pArguments;

    DebugAssert( CWaypoints::IsValid(w1) && CWaypoints::IsValid(w2), return false); // Door should have two waypoints from each side.

    const Vector& v1 = CWaypoints::Get(w1).vOrigin;
    const Vector& v2 = CWaypoints::Get(w2).vOrigin;
    return !CUtil::IsRayHitsEntity(cDoor.pEdict, v1, v2);
}

//----------------------------------------------------------------------------------------------------------------
void CItems::Draw( CClient* pClient )
{
    static float fNextDrawTime = 0.0f;
    static unsigned char pvs[MAX_MAP_CLUSTERS/8];

    if ( (pClient->iItemDrawFlags == EItemDontDraw) || (pClient->iItemTypeFlags == 0) || (CBotrixPlugin::fTime < fNextDrawTime) )
        return;

    fNextDrawTime = CBotrixPlugin::fTime + 1.0f;

    int iClusterIndex = CBotrixPlugin::pEngineServer->GetClusterForOrigin( pClient->GetHead() );
    CBotrixPlugin::pEngineServer->GetPVSForCluster( iClusterIndex, sizeof(pvs), pvs );

    for ( TEntityType iEntityType = 0; iEntityType < EEntityTypeTotal+1; ++iEntityType )
    {
        if ( !FLAG_SOME_SET(1<<iEntityType, pClient->iItemTypeFlags) ) // Don't draw items of disabled item type.
            continue;

        int iSize = (iEntityType == EOtherEntityType) ? m_aOthers.size() : m_aItems[iEntityType].size();
        for ( int i = 0; i < iSize; ++i )
        {
            edict_t* pEdict = (iEntityType == EOtherEntityType) ? m_aOthers[i] : m_aItems[iEntityType][i].pEdict;

            if ( (pEdict == NULL) || pEdict->IsFree() )
                continue;

            IServerEntity* pServerEntity = pEdict->GetIServerEntity();
            DebugAssert( pServerEntity, continue );

            if ( IsEntityTaken(pServerEntity) )
                continue;

            ICollideable* pCollide = pServerEntity->GetCollideable();
            const Vector& vOrigin = pCollide->GetCollisionOrigin();

            if ( CBotrixPlugin::pEngineServer->CheckOriginInPVS( vOrigin, pvs, sizeof(pvs) ) &&
                 CUtil::IsVisible(pClient->GetHead(), vOrigin) )
            {
                const CEntity* pEntity = (iEntityType == EOtherEntityType) ? NULL : &m_aItems[iEntityType][i];

                if ( FLAG_SOME_SET(EItemDrawStats, pClient->iItemDrawFlags) )
                {
                    int pos = 0;

                    // Draw entity class name name with index.
                    sprintf( szMainBuffer, "%s %d", CTypeToString::EntityTypeToString(iEntityType).c_str(), i+1 );
                    CUtil::DrawText( vOrigin, pos++, 1.0f, 0xFF, 0xFF, 0xFF, szMainBuffer );

                    CUtil::DrawText( vOrigin, pos++, 1.0f, 0xFF, 0xFF, 0xFF, pEdict->GetClassName() );
                    if ( iEntityType == EEntityTypeDoor )
                        CUtil::DrawText( vOrigin, pos++, 1.0f, 0xFF, 0xFF, 0xFF, IsDoorOpened(i) ? "opened" : "closed" );
                    else if ( iEntityType == EEntityTypeObject )
                    {
                        sprintf( szMainBuffer, "%s %d", CTypeToString::EntityTypeToString(iEntityType).c_str(), i );
                        CUtil::DrawText( vOrigin, pos++, 1.0f, 0xFF, 0xFF, 0xFF, IsEntityOnMap(pServerEntity) ? "alive" : "dead" );
                        //CUtil::DrawText( vOrigin, pos++, 1.0f, 0xFF, 0xFF, 0xFF, IsEntityTaken(pServerEntity) ? "taken" : "not taken" ); // Taken not shown.
                        CUtil::DrawText( vOrigin, pos++, 1.0f, 0xFF, 0xFF, 0xFF, IsEntityBreakable(pServerEntity) ? "breakable" : "non breakable" );
                        CUtil::DrawText( vOrigin, pos++, 1.0f, 0xFF, 0xFF, 0xFF, CTypeToString::EntityClassFlagsToString(pEntity->iFlags).c_str() );
                    }
                    //if ( iEntityType >= EEntityTypeButton ) // Draw entity name. Doesn't work.
                    //	CUtil::DrawText( vOrigin, pos++, 1.0f, 0xFF, 0xFF, 0xFF, GetEntityName(pServerEntity).ToCStr() );

                    if ( iEntityType >= EEntityTypeObject ) // Draw object model.
                        CUtil::DrawText( vOrigin, pos++, 1.0f, 0xFF, 0xFF, 0xFF, STRING( pEdict->GetIServerEntity()->GetModelName() ) );
                }

                // Draw box around item.
                if ( FLAG_SOME_SET(EItemDrawBoundBox, pClient->iItemDrawFlags) )
                    CUtil::DrawBox(vOrigin, pCollide->OBBMins(), pCollide->OBBMaxs(), 1.0f, 0xFF, 0xFF, 0xFF, pCollide->GetCollisionAngles());

                if ( FLAG_SOME_SET(EItemDrawWaypoint, pClient->iItemDrawFlags) && (iEntityType < EEntityTypeObject) )
                {
                    // Draw nearest waypoint from item.
                    if (CWaypoint::IsValid(pEntity->iWaypoint) )
                        CUtil::DrawLine(CWaypoints::Get(pEntity->iWaypoint).vOrigin, vOrigin, 1.0f, 0xFF, 0xFF, 0);

                    // Draw second waypoint for door.
                    if ( (iEntityType == EEntityTypeDoor) && CWaypoint::IsValid( (TWaypointId)pEntity->pArguments ) )
                        CUtil::DrawLine(CWaypoints::Get((TWaypointId)pEntity->pArguments).vOrigin, vOrigin, 1.0f, 0xFF, 0xFF, 0);
                }
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------
void CItems::WaypointDeleted( TWaypointId id )
{
    for ( int iEntityType = 0; iEntityType < EEntityTypeTotal; ++iEntityType )
    {
        good::vector<CEntity>& aItems = m_aItems[iEntityType];

        for ( size_t i = 0; i < aItems.size(); ++i )
        {
            CEntity& cItem = aItems[i];
            if ( cItem.iWaypoint == id )
                cItem.iWaypoint = CWaypoints::GetNearestWaypoint( cItem.vOrigin );
            else if ( cItem.iWaypoint > id )
                --cItem.iWaypoint;

            // TODO: update doors.
            //if ( iEntityType == EEntityTypeDoor )
        }
    }
}
