#include "cbase.h"

#include "bot.h"
#include "client.h"
#include "item.h"
#include "player.h"
#include "server_plugin.h"
#include "type2string.h"
#include "util.h"
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


extern IVDebugOverlay* pVDebugOverlay;

char szValueBuffer[16];

//================================================================================================================
inline int GetEntityFlags( IServerEntity* pServerEntity )
{
	// virtual bool GetKeyValue( const char *szKeyName, char *szValue, int iMaxLen );
	// virtual int	ObjectCaps( void );
	// GetMaxHealth
	// IsAlive
	CBaseEntity* pEntity = pServerEntity->GetBaseEntity();
	pEntity->GetKeyValue("effects", szValueBuffer, 16);
	int flags = atoi(szValueBuffer);
	return flags;
	//return pEntity->GetEffects();
}

inline int GetEntityHealth( IServerEntity* pServerEntity )
{
	CBaseEntity* pEntity = pServerEntity->GetBaseEntity();
	pEntity->GetKeyValue("health", szValueBuffer, 16);
	int health = atoi(szValueBuffer);
	return health;
	//return pEntity->GetHealth();
}

inline bool IsEntityOnMap( IServerEntity* pServerEntity )
{
	 // EF_BONEMERGE is set for weapon, when picked up. Now appears that you don't need it.
	return !FLAG_SOME_SET( EF_NODRAW | EF_BONEMERGE, GetEntityFlags(pServerEntity) );
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

good::vector<edict_t*> CItems::m_aOthers(1024);                      // Array of other entities.

#ifndef SOURCE_ENGINE_2006
good::vector<edict_t*> CItems::m_aNewEntities(16);
#endif

good::vector<good::pair<good::string, TEntityFlags>> CItems::m_aObjectFlagsForModels(4);
good::bitset CItems::m_aUsedItems(MAX_EDICTS);
int CItems::m_iCurrentEntity;
bool CItems::m_bMapLoaded = false;


//----------------------------------------------------------------------------------------------------------------
TEntityIndex CItems::GetNearestItem( TEntityType iEntityType, const Vector& vOrigin, const good::vector<CPickedItem>& aSkip, const CEntityClass* pClass )
{
	good::vector<CEntity>& aItems = m_aItems[iEntityType];

	TEntityIndex iResult = -1;
	float fSqrDistResult = 0.0f;

	for ( int i = 0; i < aItems.size(); ++i )
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

	m_aUsedItems.clear(iIndex);
	good::vector<CEntity>& aWeapons = m_aItems[EEntityTypeWeapon];
	for ( TEntityIndex i=0; i < aWeapons.size(); ++i )
		if ( aWeapons[i].pEdict == pEdict )
		{
			aWeapons[i].pEdict = NULL;
			m_iFreeIndex[EEntityTypeWeapon] = i;
			return;
		}
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

		for ( int i = 0; i < aClasses.size(); ++i )
		{
			aClasses[i].szEngineName = NULL; // Invalidate class name, because it was loaded in previous map.
			aItems.reserve(64);              // At least.
		}
	}

	m_aOthers.clear();
	m_aUsedItems.clear();
	m_bMapLoaded = false;
}


//----------------------------------------------------------------------------------------------------------------
void CItems::MapLoaded()
{
	m_iCurrentEntity = CPlayers::GetMaxPlayers()+1;

	// 0 is world, 1..max players are players. Other entities are from indexes above max players.
	int iCount = CBotrixPlugin::pEngineServer->GetEntityCount();
	for ( int i = m_iCurrentEntity; i < iCount; ++i ) // 
	{
		edict_t* pEdict = CBotrixPlugin::pEngineServer->PEntityOfEntIndex(i);
		if ( (pEdict == NULL) || pEdict->IsFree() )
			continue;

		DebugAssert(i < iCount);

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
	m_iCurrentEntity = (iTo == iCount) ? CPlayers::GetMaxPlayers()+1: iTo;
#else
	for ( int i = 0; i < m_aNewEntities.size(); ++i )
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

		for ( int j = 0; j < aItemClasses.size(); ++j )
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
		CEntity* pItem = AddItem( iEntityType, pEdict, pItemClass, pServerEntity );
		CUtil::Message(NULL, "Added new item on map: %s.", pEdict->GetClassName());

		if ( pItem && !m_bMapLoaded && !CWaypoint::IsValid(pItem->iWaypoint) && FLAG_CLEARED(FTaken, pItem->iFlags) ) // Item should have waypoint near.
		{
			CUtil::Message(NULL, "Warning: entity %s doesn't have waypoint close.", pEdict->GetClassName());
			int iWaypoint = CWaypoints::GetNearestWaypoint( pItem->vOrigin, NULL, true );
			if ( CWaypoint::IsValid(iWaypoint) )
				CUtil::Message(NULL, "\tNearest waypoint %d.", iWaypoint);
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
CEntity* CItems::AddItem( TEntityType iEntityType, edict_t* pEdict, CEntityClass* pItemClass, IServerEntity* pServerEntity )
{
	ICollideable* pCollidable = pServerEntity->GetCollideable();
	DebugAssert( pCollidable );

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

	CEntity cItem( pEdict, iFlags, fRadiusSqr, pItemClass, vItemOrigin, iWaypoint );

	good::vector<CEntity>& aItems = m_aItems[iEntityType];
	if ( m_bMapLoaded ) // Check if there are free space in items array.
	{
		if ( m_iFreeIndex[iEntityType] != -1 )
		{
			CEntity& cResult = aItems[m_iFreeIndex[iEntityType]];
			m_iFreeIndex[iEntityType] = -1;
			cResult = cItem;
			return &cResult;
		}

		for ( TEntityIndex i=0; i < aItems.size(); ++i )
			if ( aItems[i].pEdict == NULL )
			{
				aItems[i] = cItem;
				return &aItems[i];
			}
	}

	aItems.push_back( cItem );
	return &aItems.back();
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
		for ( int i = 0; i < m_aObjectFlagsForModels.size(); ++i )
			if ( sModel.find( m_aObjectFlagsForModels[i].first ) != good::string::npos )
			{
				FLAG_SET(m_aObjectFlagsForModels[i].second, iFlags);
				break;
			}
	}

	good::vector<CEntity>& aItems = m_aItems[EEntityTypeObject];
	CEntity cObject( pEdict, iFlags, fMaxsRadiusSqr, pObjectClass, CUtil::vZero, -1 );
	if ( m_bMapLoaded ) // Check if there are free space in items array.
	{
		if ( m_iFreeIndex[EEntityTypeObject] != -1 )
		{
			CEntity& cResult = aItems[m_iFreeIndex[EEntityTypeObject]];
			m_iFreeIndex[EEntityTypeObject] = -1;
			cResult = cObject;
			return;
		}

		for ( TEntityIndex i=0; i < aItems.size(); ++i )
			if ( aItems[i].pEdict == NULL )
			{
				aItems[i] = cObject;
				return;
			}
	}
	aItems.push_back( cObject );
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
            DebugAssert( pServerEntity );

			if ( IsEntityTaken(pServerEntity) )
				continue;

			ICollideable* pCollide = pServerEntity->GetCollideable();
			const Vector& vOrigin = pCollide->GetCollisionOrigin();

			if ( CBotrixPlugin::pEngineServer->CheckOriginInPVS( vOrigin, pvs, sizeof(pvs) ) &&
			     CUtil::IsVisible(pClient->GetHead(), vOrigin) )
			{
				if ( FLAG_SOME_SET(EItemDrawStats, pClient->iItemDrawFlags) )
				{
					int pos = 0;
					CUtil::DrawText( vOrigin, pos++, 1.0f, 0xFF, 0xFF, 0xFF, CTypeToString::EntityTypeToString(iEntityType).c_str() );
					CUtil::DrawText( vOrigin, pos++, 1.0f, 0xFF, 0xFF, 0xFF, pEdict->GetClassName() );
					CUtil::DrawText( vOrigin, pos++, 1.0f, 0xFF, 0xFF, 0xFF, IsEntityOnMap(pServerEntity) ? "alive" : "dead" );
					//CUtil::DrawText( vOrigin, pos++, 1.0f, 0xFF, 0xFF, 0xFF, IsEntityTaken(pServerEntity) ? "taken" : "not taken" );
					CUtil::DrawText( vOrigin, pos++, 1.0f, 0xFF, 0xFF, 0xFF, IsEntityBreakable(pServerEntity) ? "breakable" : "non breakable" );
					if ( iEntityType == EEntityTypeObject )
						CUtil::DrawText( vOrigin, pos++, 1.0f, 0xFF, 0xFF, 0xFF, CTypeToString::EntityClassFlagsToString(m_aItems[iEntityType][i].iFlags).c_str() );
					if ( iEntityType >= EEntityTypeObject )
						CUtil::DrawText( vOrigin, pos++, 1.0f, 0xFF, 0xFF, 0xFF, STRING( pEdict->GetIServerEntity()->GetModelName() ) );
				}

				if ( FLAG_SOME_SET(EItemDrawBoundBox, pClient->iItemDrawFlags) )
					CUtil::DrawBox(vOrigin, pCollide->OBBMins(), pCollide->OBBMaxs(), 1.0f, 0xFF, 0xFF, 0xFF, pCollide->GetCollisionAngles());

				if ( (iEntityType < EEntityTypeObject) && FLAG_SOME_SET(EItemDrawWaypoint, pClient->iItemDrawFlags) && CWaypoint::IsValid(m_aItems[iEntityType][i].iWaypoint) )
					CUtil::DrawLine(CWaypoints::Get(m_aItems[iEntityType][i].iWaypoint).vOrigin, vOrigin, 1.0f, 0xFF, 0xFF, 0);
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

		for ( int i = 0; i < aItems.size(); ++i )
		{
			CEntity& cItem = aItems[i];
			if ( cItem.iWaypoint == id )
				cItem.iWaypoint = CWaypoints::GetNearestWaypoint( cItem.vOrigin );
			else if ( cItem.iWaypoint > id )
				--cItem.iWaypoint;
		}
	}
}
