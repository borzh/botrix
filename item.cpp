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


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


extern IVDebugOverlay* pVDebugOverlay;

extern char* szMainBuffer;
extern int iMainBufferSize;

char szValueBuffer[16];

#ifndef BOTRIX_SOURCE_ENGINE_2006
    #define BOTRIX_ENTITY_USE_KEY_VALUE
#endif

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
bool CItem::IsOnMap() const
{
    return IsEntityOnMap(pEdict->GetIServerEntity());
}

//----------------------------------------------------------------------------------------------------------------
bool CItem::IsBreakable() const
{
    return IsEntityBreakable(pEdict->GetIServerEntity());
}


//================================================================================================================
good::vector<CItem> CItems::m_aItems[EItemTypeTotal];            // Array of items.
good::list<CItemClass> CItems::m_aItemClasses[EItemTypeTotal]; // Array of item classes.
TItemIndex CItems::m_iFreeIndex[EItemTypeTotal];                 // First free weapon index.
int CItems::m_iFreeEntityCount[EItemTypeTotal];                    // Count of unused entities.

good::vector<edict_t*> CItems::m_aOthers(1024);                      // Array of other entities.

#ifndef BOTRIX_SOURCE_ENGINE_2006
good::vector<edict_t*> CItems::m_aNewEntities(16);
#endif

good::vector< good::pair<good::string, TItemFlags> > CItems::m_aObjectFlagsForModels(4);
good::bitset CItems::m_aUsedItems(MAX_EDICTS);
int CItems::m_iCurrentEntity;
bool CItems::m_bMapLoaded = false;


//----------------------------------------------------------------------------------------------------------------
TItemIndex CItems::GetNearestItem( TItemType iEntityType, const Vector& vOrigin, const good::vector<CPickedItem>& aSkip, const CItemClass* pClass )
{
    good::vector<CItem>& aItems = m_aItems[iEntityType];

    TItemIndex iResult = -1;
    float fSqrDistResult = 0.0f;

    for ( int i = 0; i < aItems.size(); ++i )
    {
        CItem& cItem = aItems[i];
        if ( (cItem.pEdict == NULL) || cItem.pEdict->IsFree() || !CWaypoint::IsValid(cItem.iWaypoint) ||
             ( pClass && (pClass != cItem.pItemClass) ) ) // If item is already added, we have engine name.
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


#ifndef BOTRIX_SOURCE_ENGINE_2006

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
    GoodAssert( iIndex > 0 ); // Valve should not allow this assert.

    // Check only server entities.
    if ( !m_aUsedItems.test(iIndex) )
        return;

    m_aUsedItems.reset(iIndex);
    good::vector<CItem>& aWeapons = m_aItems[EItemTypeWeapon];
    for ( TItemIndex i=0; i < (int)aWeapons.size(); ++i )
        if ( aWeapons[i].pEdict == pEdict )
        {
            aWeapons[i].pEdict = NULL;
            m_iFreeIndex[EItemTypeWeapon] = i;
            return;
        }
    //BASSERT(false); // Only weapons are allocated/deallocated while map is running.
}
#endif // BOTRIX_SOURCE_ENGINE_2006

//----------------------------------------------------------------------------------------------------------------
void CItems::MapUnloaded()
{
    for ( TItemType iEntityType = 0; iEntityType < EItemTypeTotal; ++iEntityType )
    {
        good::list<CItemClass>& aClasses = m_aItemClasses[iEntityType];
        good::vector<CItem>& aItems = m_aItems[iEntityType];

        aItems.clear();
        m_iFreeIndex[iEntityType] = -1;      // Invalidate free entity index.

        for ( good::list<CItemClass>::iterator it = aClasses.begin(); it != aClasses.end(); ++it )
        {
            it->szEngineName = NULL; // Invalidate class name, because it was loaded in previous map.
            aItems.reserve(64);      // At least.
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
#ifdef BOTRIX_SOURCE_ENGINE_2006
    // Source engine 2007 uses IServerPluginCallbacks::OnEdictAllocated instead of checking all array of edicts.

    // Update weapons we have in items array.
    good::vector<CItem>& aWeapons = m_aItems[EItemTypeWeapon];
    for ( TItemIndex i = 0; i < aWeapons.size(); ++i )
    {
        CItem& cEntity = aWeapons[i];
        edict_t* pEdict = cEntity.pEdict;
        if ( pEdict == NULL )
        {
            m_iFreeIndex[EItemTypeWeapon] = i;
            continue;
        }

        IServerEntity* pServerEntity = pEdict->GetIServerEntity();
        if ( pEdict->IsFree() || (pServerEntity == NULL) )
        {
            cEntity.pEdict = NULL;
            m_iFreeIndex[EItemTypeWeapon] = i;
            m_aUsedItems.reset( CBotrixPlugin::pEngineServer->IndexOfEdict(pEdict) );
        }
        else if ( IsEntityTaken(pServerEntity) ) // Weapon still belongs to some player.
            FLAG_SET(FTaken, cEntity.iFlags);
        else if ( FLAG_ALL_SET_OR_0(FTaken, cEntity.iFlags) && IsEntityOnMap(pServerEntity) )
        {
            FLAG_CLEAR(FTaken, cEntity.iFlags);
            cEntity.vOrigin = cEntity.CurrentPosition();
            cEntity.iWaypoint = CWaypoints::GetNearestWaypoint( cEntity.vOrigin, NULL, true, CItem::iMaxDistToWaypoint );
        }
    }

    // 0 is world, 1..max players are players. Other entities are from indexes above max players.
    int iCount = CBotrixPlugin::pEngineServer->GetEntityCount();

    // Update weapon entities on map.
    int iTo = m_iCurrentEntity + m_iCheckEntitiesPerFrame;
    if ( iTo > iCount )
        iTo = iCount;

    for ( TItemIndex i = m_iCurrentEntity; i < iTo; ++i )
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
        CItemClass* pWeaponClass;
        TItemType iEntityType = GetEntityType(pEdict->GetClassName(), pWeaponClass, EItemTypeWeapon, EItemTypeWeapon+1);

        if ( iEntityType == EItemTypeWeapon )
            AddItem( EItemTypeWeapon, pEdict, pWeaponClass, pServerEntity );
    }
    m_iCurrentEntity = (iTo == iCount) ? CPlayers::Size()+1: iTo;
#else
    for ( int i = 0; i < m_aNewEntities.size(); ++i )
        CheckNewEntity( m_aNewEntities[i] );
    m_aNewEntities.clear();
#endif // BOTRIX_SOURCE_ENGINE_2006
}


//----------------------------------------------------------------------------------------------------------------
TItemType CItems::GetEntityType( const char* szClassName, CItemClass* & pEntityClass, TItemType iFrom, TItemType iTo, bool bFastCmp )
{
    for ( TItemType iEntityType = iFrom; iEntityType < iTo; ++iEntityType )
    {
        good::list<CItemClass>& aItemClasses = m_aItemClasses[iEntityType];

        for ( good::list<CItemClass>::iterator it = aItemClasses.begin(); it != aItemClasses.end(); ++it )
        {
            CItemClass& cEntityClass = *it;

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

    return EItemTypeOther;
}


//----------------------------------------------------------------------------------------------------------------
void CItems::CheckNewEntity( edict_t* pEdict )
{
    // Check only server entities.
    IServerEntity* pServerEntity = pEdict->GetIServerEntity();
    if ( pServerEntity == NULL )
        return;

    const char* szClassName = pEdict->GetClassName();
    CItemClass* pItemClass;
    TItemType iEntityType = GetEntityType(szClassName, pItemClass, 0, EItemTypeTotal);
    if ( iEntityType == EItemTypeOther )
        m_aOthers.push_back(pEdict);
    else if ( iEntityType == EItemTypeObject )
        AddObject( pEdict, pItemClass, pServerEntity );
    else
    {
        TItemIndex iIndex = AddItem( iEntityType, pEdict, pItemClass, pServerEntity );
        BASSERT(iIndex >= 0, return);
        CItem& cItem = m_aItems[iEntityType][iIndex];

        BLOG_D("New item: %s %d (%s), waypoint %d (%s).", CTypeToString::EntityTypeToString(iEntityType).c_str(), iIndex,
                    pEdict->GetClassName(), cItem.iWaypoint,
                    CWaypoints::IsValid(cItem.iWaypoint)
                        ? CTypeToString::WaypointFlagsToString( CWaypoints::Get(cItem.iWaypoint).iFlags ).c_str()
                        : "");

        if (  !m_bMapLoaded && FLAG_CLEARED(FTaken, cItem.iFlags) ) // Item should have waypoint near.
        {
            if ( !CWaypoint::IsValid(cItem.iWaypoint) )
            {
                const Vector& vOrigin = cItem.CurrentPosition();
                BLOG_W( "Warning: entity %s %d (%.0f %.0f %.0f) doesn't have waypoint close.", pEdict->GetClassName(), iIndex+1,
                        vOrigin.x, vOrigin.y, vOrigin.z );
                TWaypointId iWaypoint = CWaypoints::GetNearestWaypoint( cItem.vOrigin );
                if ( CWaypoint::IsValid(iWaypoint) )
                    BLOG_W("  Nearest waypoint %d.", iWaypoint);
            }
            else if ( iEntityType == EItemTypeDoor && !CWaypoint::IsValid((TWaypointId)cItem.pArguments) )
                BLOG_W("Door %d doesn't have 2 waypoints near.", iIndex);
        }

#ifdef BOTRIX_SOURCE_ENGINE_2006
        // Weapon entities are allocated / deallocated when respawned / owner killed.
        if ( iEntityType != EItemTypeWeapon )
#endif
        {
            int iIndex = CBotrixPlugin::pEngineServer->IndexOfEdict(pEdict);
            GoodAssert( iIndex > 0 ); // Valve should not allow this assert.
            m_aUsedItems.set(iIndex);
        }
    }

}


//----------------------------------------------------------------------------------------------------------------
TItemIndex CItems::InsertEntity( int iEntityType, const CItem& cEntity )
{
    good::vector<CItem>& aItems = m_aItems[iEntityType];

    if ( m_bMapLoaded ) // Check if there are free space in items array.
    {
        if ( m_iFreeIndex[iEntityType] != -1 )
        {
            int iIndex = m_iFreeIndex[iEntityType];
            m_iFreeIndex[iEntityType] = -1;
            aItems[iIndex] = cEntity;
            return iIndex;
        }

        for ( int i=0; i < aItems.size(); ++i ) // TODO: add free count.
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
void CItems::AutoWaypointPathFlagsForEntity( TItemType iEntityType, TItemIndex iIndex, CItem& cEntity )
{
    TWaypointId iWaypoint = cEntity.iWaypoint;
    if ( (iEntityType == EItemTypeButton) && (iWaypoint != EWaypointIdInvalid) )
    {
        // Set waypoint argument to button.
        CWaypoint& cWaypoint = CWaypoints::Get(iWaypoint);
        FLAG_SET(FWaypointButton, cWaypoint.iFlags);
        CWaypoint::SetButton( iIndex+1, cWaypoint.iArgument );
    }

    // Check 2nd nearest waypoint for door.
    if ( iEntityType == EItemTypeDoor )
    {
        if ( iWaypoint != EWaypointIdInvalid )
        {
            good::bitset cOmitWaypoints(CWaypoints::Size());
            cOmitWaypoints.set(iWaypoint);
            iWaypoint = CWaypoints::GetNearestWaypoint( cEntity.vOrigin, &cOmitWaypoints, true, CItem::iMaxDistToWaypoint );
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
TItemIndex CItems::AddItem( TItemType iEntityType, edict_t* pEdict, CItemClass* pItemClass, IServerEntity* pServerEntity )
{
    GoodAssert( (0 <= iEntityType) && (iEntityType < EItemTypeObject) );

    ICollideable* pCollidable = pServerEntity->GetCollideable();
    BASSERT( pCollidable, return -1 );

    const Vector& vItemOrigin = pCollidable->GetCollisionOrigin();

    float fPickupDistanceSqr = pItemClass->fPickupDistanceSqr;
    if ( fPickupDistanceSqr < 1.0f )
    {
        // Calculate object radius.
        float fMaxsRadiusSqr = pCollidable->OBBMaxs().LengthSqr();
        float fMinsRadiusSqr = pCollidable->OBBMins().LengthSqr();
        fPickupDistanceSqr = FastSqrt( MAX2(fMinsRadiusSqr, fMaxsRadiusSqr) );
        fPickupDistanceSqr += CMod::iPlayerRadius + CMod::iItemPickUpDistance;
        fPickupDistanceSqr *= fPickupDistanceSqr*2;
        pItemClass->fPickupDistanceSqr = fPickupDistanceSqr;
    }

    int iFlags = pItemClass->iFlags;
    TWaypointId iWaypoint = -1;

    if ( !IsEntityOnMap(pServerEntity) || IsEntityTaken(pServerEntity) )
        FLAG_SET(FTaken, iFlags);
    else
        iWaypoint = CWaypoints::GetNearestWaypoint( vItemOrigin, NULL, true, CItem::iMaxDistToWaypoint );

    CItem cNewEntity(pEdict, iFlags, fPickupDistanceSqr, pItemClass, vItemOrigin, iWaypoint);
    TItemIndex iIndex = InsertEntity( iEntityType, cNewEntity );

    CItem& cEntity = m_aItems[iEntityType][iIndex];
    AutoWaypointPathFlagsForEntity( iEntityType, iIndex, cEntity );

    return iIndex;
}


//----------------------------------------------------------------------------------------------------------------
void CItems::AddObject( edict_t* pEdict, const CItemClass* pObjectClass, IServerEntity* pServerEntity )
{
    // Calculate object radius.
    ICollideable* pCollidable = pServerEntity->GetCollideable();

    // TODO: why not use object class to get distance?
    float fMaxsRadiusSqr = pCollidable->OBBMaxs().LengthSqr();
    float fMinsRadiusSqr = pCollidable->OBBMins().LengthSqr();
    fMaxsRadiusSqr = FastSqrt( MAX2(fMaxsRadiusSqr, fMinsRadiusSqr) );
    fMaxsRadiusSqr += CMod::iPlayerRadius;
    fMaxsRadiusSqr *= fMaxsRadiusSqr;

    int iFlags = pObjectClass->iFlags;

    // Get object flags.
    if ( m_aObjectFlagsForModels.size() )
    {
        good::string sModel = STRING( pServerEntity->GetModelName() );
        for ( int i = 0; i < m_aObjectFlagsForModels.size(); ++i )
            if ( sModel.find( m_aObjectFlagsForModels[i].first ) != good::string::npos )
            {
                FLAG_SET(m_aObjectFlagsForModels[i].second, iFlags);
                break;
            }
    }

    good::vector<CItem>& aItems = m_aItems[EItemTypeObject];
    CItem cObject( pEdict, iFlags, fMaxsRadiusSqr, pObjectClass, pCollidable->GetCollisionOrigin(), -1 );
    if ( m_bMapLoaded ) // Check if there are free space in items array.
    {
        if ( m_iFreeIndex[EItemTypeObject] != -1 )
        {
            CItem& cResult = aItems[m_iFreeIndex[EItemTypeObject]];
            m_iFreeIndex[EItemTypeObject] = -1;
            cResult = cObject;
            return;
        }

        for ( int i=0; i < aItems.size(); ++i )
            if ( aItems[i].pEdict == NULL )
            {
                aItems[i] = cObject;
                return;
            }
    }
    aItems.push_back( cObject );
}

//----------------------------------------------------------------------------------------------------------------
bool CItems::IsDoorOpened( TItemIndex iDoor )
{
    const CItem& cDoor = m_aItems[EItemTypeDoor][iDoor];
    TWaypointId w1 = cDoor.iWaypoint, w2 = (TWaypointId)cDoor.pArguments;

    BASSERT( CWaypoints::IsValid(w1) && CWaypoints::IsValid(w2), return false); // Door should have two waypoints from each side.

    const Vector& v1 = CWaypoints::Get(w1).vOrigin;
    const Vector& v2 = CWaypoints::Get(w2).vOrigin;
    return !CUtil::IsRayHitsEntity(cDoor.pEdict, v1, v2);
}

//----------------------------------------------------------------------------------------------------------------
void CItems::Draw( CClient* pClient )
{
    static float fNextDrawTime = 0.0f;
    static unsigned char pvs[MAX_MAP_CLUSTERS/8];

    if ( (pClient->iItemDrawFlags == FItemDontDraw) || (pClient->iItemTypeFlags == 0) || (CBotrixPlugin::fTime < fNextDrawTime) )
        return;

    fNextDrawTime = CBotrixPlugin::fTime + 1.0f;

    int iClusterIndex = CBotrixPlugin::pEngineServer->GetClusterForOrigin( pClient->GetHead() );
    CBotrixPlugin::pEngineServer->GetPVSForCluster( iClusterIndex, sizeof(pvs), pvs );

    for ( TItemType iEntityType = 0; iEntityType < EItemTypeTotal+1; ++iEntityType )
    {
        if ( !FLAG_SOME_SET(1<<iEntityType, pClient->iItemTypeFlags) ) // Don't draw items of disabled item type.
            continue;

        int iSize = (iEntityType == EItemTypeOther) ? m_aOthers.size() : m_aItems[iEntityType].size();
        for ( int i = 0; i < iSize; ++i )
        {
            edict_t* pEdict = (iEntityType == EItemTypeOther) ? m_aOthers[i] : m_aItems[iEntityType][i].pEdict;

            if ( (pEdict == NULL) || pEdict->IsFree() )
                continue;

            IServerEntity* pServerEntity = pEdict->GetIServerEntity();
            BASSERT( pServerEntity, continue );

            if ( IsEntityTaken(pServerEntity) )
                continue;

            ICollideable* pCollide = pServerEntity->GetCollideable();
            const Vector& vOrigin = pCollide->GetCollisionOrigin();

            if ( CBotrixPlugin::pEngineServer->CheckOriginInPVS( vOrigin, pvs, sizeof(pvs) ) &&
                 CUtil::IsVisible(pClient->GetHead(), vOrigin) )
            {
                const CItem* pEntity = (iEntityType == EItemTypeOther) ? NULL : &m_aItems[iEntityType][i];

                if ( FLAG_SOME_SET(FItemDrawStats, pClient->iItemDrawFlags) )
                {
                    int pos = 0;

                    // Draw entity class name name with index.
                    sprintf( szMainBuffer, "%s %d", CTypeToString::EntityTypeToString(iEntityType).c_str(), i+1 );
                    CUtil::DrawText( vOrigin, pos++, 1.0f, 0xFF, 0xFF, 0xFF, szMainBuffer );

                    CUtil::DrawText( vOrigin, pos++, 1.0f, 0xFF, 0xFF, 0xFF, pEdict->GetClassName() );
                    if ( iEntityType == EItemTypeDoor )
                        CUtil::DrawText( vOrigin, pos++, 1.0f, 0xFF, 0xFF, 0xFF, IsDoorOpened(i) ? "opened" : "closed" );
                    else if ( iEntityType == EItemTypeObject )
                    {
                        sprintf( szMainBuffer, "%s %d", CTypeToString::EntityTypeToString(iEntityType).c_str(), i );
                        CUtil::DrawText( vOrigin, pos++, 1.0f, 0xFF, 0xFF, 0xFF, IsEntityOnMap(pServerEntity) ? "alive" : "dead" );
                        //CUtil::DrawText( vOrigin, pos++, 1.0f, 0xFF, 0xFF, 0xFF, IsEntityTaken(pServerEntity) ? "taken" : "not taken" ); // Taken not shown.
                        CUtil::DrawText( vOrigin, pos++, 1.0f, 0xFF, 0xFF, 0xFF, IsEntityBreakable(pServerEntity) ? "breakable" : "non breakable" );
                        CUtil::DrawText( vOrigin, pos++, 1.0f, 0xFF, 0xFF, 0xFF, CTypeToString::EntityClassFlagsToString(pEntity->iFlags).c_str() );
                    }
                    //if ( iEntityType >= EItemTypeButton ) // Draw entity name. Doesn't work.
                    //	CUtil::DrawText( vOrigin, pos++, 1.0f, 0xFF, 0xFF, 0xFF, GetEntityName(pServerEntity).ToCStr() );

                    if ( iEntityType >= EItemTypeObject ) // Draw object model.
                        CUtil::DrawText( vOrigin, pos++, 1.0f, 0xFF, 0xFF, 0xFF, STRING( pEdict->GetIServerEntity()->GetModelName() ) );
                }

                // Draw box around item.
                if ( FLAG_SOME_SET(FItemDrawBoundBox, pClient->iItemDrawFlags) )
                    CUtil::DrawBox(vOrigin, pCollide->OBBMins(), pCollide->OBBMaxs(), 1.0f, 0xFF, 0xFF, 0xFF, pCollide->GetCollisionAngles());

                if ( FLAG_SOME_SET(FItemDrawWaypoint, pClient->iItemDrawFlags) && (iEntityType < EItemTypeObject) )
                {
                    // Draw nearest waypoint from item.
                    if (CWaypoint::IsValid(pEntity->iWaypoint) )
                        CUtil::DrawLine(CWaypoints::Get(pEntity->iWaypoint).vOrigin, vOrigin, 1.0f, 0xFF, 0xFF, 0);

                    // Draw second waypoint for door.
                    if ( (iEntityType == EItemTypeDoor) && CWaypoint::IsValid( (TWaypointId)pEntity->pArguments ) )
                        CUtil::DrawLine(CWaypoints::Get((TWaypointId)pEntity->pArguments).vOrigin, vOrigin, 1.0f, 0xFF, 0xFF, 0);
                }
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------
void CItems::WaypointDeleted( TWaypointId id )
{
    for ( int iEntityType = 0; iEntityType < EItemTypeTotal; ++iEntityType )
    {
        good::vector<CItem>& aItems = m_aItems[iEntityType];

        for ( int i = 0; i < aItems.size(); ++i )
        {
            CItem& cItem = aItems[i];
            if ( cItem.iWaypoint == id )
                cItem.iWaypoint = CWaypoints::GetNearestWaypoint( cItem.vOrigin );
            else if ( cItem.iWaypoint > id )
                --cItem.iWaypoint;

            // TODO: update doors.
            //if ( iEntityType == EItemTypeDoor )
        }
    }
}
