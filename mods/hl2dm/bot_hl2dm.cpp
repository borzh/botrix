#include "bot_hl2dm.h"
#include "client.h"
#include "type2string.h"


//----------------------------------------------------------------------------------------------------------------
CBot_HL2DM::CBot_HL2DM( edict_t* pEdict, TPlayerIndex iIndex, TBotIntelligence iIntelligence ):
	CBot(pEdict, iIndex, iIntelligence), m_cItemToSearch(-1, -1), m_aWaypoints(CWaypoints::Size())
{
	CBotrixPlugin::pEngineServer->SetFakeClientConVarValue(pEdict, "cl_autowepswitch", "0");	
	CBotrixPlugin::pEngineServer->SetFakeClientConVarValue(pEdict, "cl_defaultweapon", "weapon_smg1");	
	m_bShootAtHead = false;
}
		
//----------------------------------------------------------------------------------------------------------------
void CBot_HL2DM::Respawned()
{
	CBot::Respawned();
	m_aWaypoints.clear();
	m_iFailWaypoint = EInvalidWaypointId;
	
	m_iCurrentTask = EBotTaskInvalid;
	m_bNeedTaskCheck = true;
	m_bCheckWeapon = CMod::HasMapItems(EEntityTypeWeapon);
}

//----------------------------------------------------------------------------------------------------------------
void CBot_HL2DM::KilledEnemy( int iPlayerIndex, CPlayer* pVictim )
{
}

//----------------------------------------------------------------------------------------------------------------
void CBot_HL2DM::HurtBy( int iPlayerIndex, CPlayer* pAttacker, int iHealthNow )
{
	if ( pAttacker && (pAttacker != this) )
		CheckEnemy(iPlayerIndex, pAttacker, false);
	if ( iHealthNow < (CUtil::iPlayerMaxHealth/2) )
		m_bNeedTaskCheck = true; // Check if need search for health.
}

//----------------------------------------------------------------------------------------------------------------
void CBot_HL2DM::Move()
{
	if ( !m_bAlive )
	{
		m_cCmd.buttons = rand() & IN_ATTACK; // Force bot to respawn by hitting randomly attack button.
		return;
	}

	bool bForceNewTask = false;

	// Check for move failure.
	if ( m_bMoveFailure || m_bStuck )
	{
		if ( iCurrentWaypoint == m_iFailWaypoint )
			m_iFailsCount++;
		else
		{
			m_iFailWaypoint = iCurrentWaypoint;
			m_iFailsCount = 1;
		}
	
		if ( m_iFailsCount >= 3 )
		{
			BotMessage("Failed to follow path on same waypoint %d 3 times, marking task as finished.", iCurrentWaypoint);
			TaskFinished();
			m_bNeedTaskCheck = bForceNewTask = true;
			m_iFailsCount = 0;
			m_iFailWaypoint = -1;
		}
		else if ( m_bMoveFailure )
		{
			// Recalculate the route.
			m_iDestinationWaypoint = m_iTaskDestination;
			m_bNeedMove = m_bUseNavigatorToMove = m_bDestinationChanged = true;
		}		

		m_bMoveFailure = m_bStuck = false;
	}

	// Check if needs to add new tasks. Objectives have more priority than task, but not when bot flees.
	if ( m_bNeedTaskCheck )
	{
		m_bNeedTaskCheck = false;
		if ( bForceNewTask || m_bFlee || (!m_bObjectiveChanged && (m_iObjective == EBotChatUnknown)) )
			CheckNewTasks(bForceNewTask);
	}

	if ( m_bFlee )
		m_bNeedSprint = true; // Force bot to move rapidly. TODO: check if stops.
}

//----------------------------------------------------------------------------------------------------------------
void CBot_HL2DM::ReceiveChat( int iPlayerIndex, CPlayer* pPlayer, bool bTeamOnly, const char* szText )
{

}

//----------------------------------------------------------------------------------------------------------------
bool CBot_HL2DM::DoWaypointAction()
{
	if ( iCurrentWaypoint == m_iTaskDestination )
	{
		m_iCurrentTask = EBotTaskInvalid;
		m_bNeedTaskCheck = true;
	}
	return CBot::DoWaypointAction();
}

//----------------------------------------------------------------------------------------------------------------
void CBot_HL2DM::CheckNewTasks( bool bForceTaskChange )
{
	TBotTaskHL2DM iNewTask = EBotTaskInvalid;
	bool bForce = bForceTaskChange || (m_iCurrentTask == EBotTaskInvalid);

	const CWeapon* pWeapon = m_aWeapons[m_iBestWeapon].GetBaseWeapon();
	TBotIntelligence iWeaponPreference = m_iIntelligence;

	bool bNeedHealth = CMod::HasMapItems(EEntityTypeHealth) && ( m_pPlayerInfo->GetHealth() < CUtil::iPlayerMaxHealth );
	bool bNeedHealthBad = CMod::HasMapItems(EEntityTypeHealth) && ( m_pPlayerInfo->GetHealth() < (CUtil::iPlayerMaxHealth/2) );
	bool bAlmostDead = ( m_pPlayerInfo->GetHealth() < (CUtil::iPlayerMaxHealth/5) );
	
	TWeaponId iWeapon;
	bool bSecondary = false;

	const CEntityClass* pEntityClass = NULL; // Weapon or ammo class to search for.

	if ( bAlmostDead )
	{
		m_bDontAttack = true; // TODO: try to not to pass to zones with enemy.
		m_bFlee = true;
	}

	if ( bNeedHealthBad && CMod::HasMapItems(EEntityTypeHealth) ) // Need health pretty much.
	{
		iNewTask = EBotTaskFindHealth;
		bForce = true;
	}
	else if ( m_bCheckWeapon && (pWeapon->iBotPreference < iWeaponPreference) ) // Need some weapon with higher preference.
	{
		iNewTask = EBotTaskFindWeapon;
	}
	else 
	{
		// Need ammunition.
		bool bNeedAmmo0 = CMod::HasMapItems(EEntityTypeAmmo) && (m_aWeapons[m_iBestWeapon].ExtraBullets(0) < pWeapon->iClipSize[0]); // Has less than 1 extra clip.
		bool bNeedAmmo1 = CMod::HasMapItems(EEntityTypeAmmo) && pWeapon->bHasSecondary && !pWeapon->bSecondaryUseSameBullets &&      // Has secondary function,
		                 !m_aWeapons[m_iBestWeapon].HasAmmoInClip(1) && !m_aWeapons[m_iBestWeapon].HasAmmoExtra(1);                  // but no secondary bullets.

		if ( bNeedAmmo0 || bNeedAmmo1 )
		{
			iNewTask = EBotTaskFindAmmo;
			// Prefer search for secondary ammo only if has extra bullets for primary.
			bSecondary = bNeedAmmo1 && (m_aWeapons[m_iBestWeapon].ExtraBullets(0) > 0);
			pEntityClass = pWeapon->aAmmos[bSecondary][ rand() % pWeapon->aAmmos[bSecondary].size() ];
		}
		else if ( bNeedHealth ) // Need health (but has more than 50%).
			iNewTask = EBotTaskFindHealth;
		else if ( CMod::HasMapItems(EEntityTypeArmor) && (m_pPlayerInfo->GetArmorValue() < CUtil::iPlayerMaxArmor) ) // Need armor.
			iNewTask = EBotTaskFindArmor;
		else if ( m_bCheckWeapon && (pWeapon->iBotPreference < EBotPro) ) // Check if can find a better weapon.
		{
			iNewTask = EBotTaskFindWeapon;
			iWeaponPreference = pWeapon->iBotPreference+1;
		}
		else if ( CMod::HasMapItems(EEntityTypeAmmo) && !m_aWeapons[m_iBestWeapon].FullAmmo(1) ) // Check if weapon needs secondary ammo.
		{
			iNewTask = EBotTaskFindAmmo;
			bSecondary = true;
		}
		else if ( CMod::HasMapItems(EEntityTypeAmmo) && !m_aWeapons[m_iBestWeapon].FullAmmo(0) ) // Check if weapon needs primary ammo.
		{
			iNewTask = EBotTaskFindAmmo;
			bSecondary = false;
		}
		else
			iNewTask = EBotTaskFindEnemy;
	}

	switch(iNewTask)
	{
	case EBotTaskFindWeapon:
		// Get weapon entity class to search for. Search for better weapons that actually have.
		for ( TBotIntelligence iPreference = iWeaponPreference; iPreference < EBotIntelligenceTotal; ++iPreference )
		{
			iWeapon = CWeapons::GetRandomWeapon(iPreference);
			if ( iWeapon != -1 )
			{
				pEntityClass = CWeapons::Get(iWeapon)->pWeaponClass;
				break;
			}
		}
		if ( pEntityClass == NULL )
		{
			// None found, search for worst weapons.
			for ( TBotIntelligence iPreference = iWeaponPreference-1; iPreference >= 0; --iPreference )
			{
				iWeapon = CWeapons::GetRandomWeapon(iPreference);
				if ( iWeapon != -1 )
				{
					pEntityClass = CWeapons::Get(iWeapon)->pWeaponClass;
					break;
				}
			}
		}
		DebugAssert( pEntityClass != NULL ); // No weapons/ammo on this map ??? Should not enter here.
		break;

	case EBotTaskFindAmmo:
		// Get ammo entity class to search for.
		iWeapon = m_iBestWeapon;
	
		// Randomly search for weapon instead, as it gives same primary bullets.
		if ( !bSecondary && CItems::ExistsOnMap(pWeapon->pWeaponClass) && (rand() & 1) )
		{
			iNewTask = EBotTaskFindWeapon;
			pEntityClass = pWeapon->pWeaponClass;
		}
		else
		{
			int iIdx = rand() % pWeapon->aAmmos[bSecondary].size();
			pEntityClass = pWeapon->aAmmos[bSecondary][ iIdx ]; // Randomly search for ammo.
		}
		
		if ( !CItems::ExistsOnMap(pEntityClass) ) // There are no such weapon/ammo on the map.
		{
			iNewTask = EBotTaskFindEnemy; // Just find enemy.
			pEntityClass = NULL;
		}
		break;

	case EBotTaskFindHealth:
	case EBotTaskFindArmor:
		pEntityClass = CItems::GetRandomItemClass(EEntityTypeHealth + (iNewTask - EBotTaskFindHealth));
		break;
	}
	
	DebugAssert( iNewTask != EBotTaskInvalid );

	// Check if need task switch.
	if ( bForce || (m_iCurrentTask != iNewTask) )
	{
		m_iCurrentTask = iNewTask;

		if ( pEntityClass ) // Health, armor, weapon, ammo.
		{
			TEntityType iType = EEntityTypeHealth + (iNewTask - EBotTaskFindHealth);
			TEntityIndex iItemToSearch = CItems::GetNearestItem( iType, GetHead(), m_aPickedItems, pEntityClass );

			if ( iItemToSearch == -1 )
			{
				m_aWaypoints.clear();
				m_aWaypoints.set(iCurrentWaypoint);

				// Try to get to waypoint with flags.
				m_iTaskDestination = CWaypoints::GetNearestWaypoint( m_vHead, &m_aWaypoints, false, CUtil::MAX_MAP_SIZE, CWaypoint::GetFlagsFor(iType));
				m_cItemToSearch.iType = -1;
				m_cItemToSearch.iIndex = m_iTaskDestination;
			}
			else
			{
				m_iTaskDestination = CItems::GetItems(iType)[iItemToSearch].iWaypoint;
				m_cItemToSearch.iType = iType;
				m_cItemToSearch.iIndex = iItemToSearch;
			}
		}
		else if (m_iCurrentTask == EBotTaskFindEnemy)
		{
			// Just go to some random waypoint.
			m_iTaskDestination = -1;
			if ( CWaypoints::Size() >= 2 )
			{
				do {
					m_iTaskDestination = rand() % CWaypoints::Size();
				} while ( m_iTaskDestination == iCurrentWaypoint );
			}
		}

		// Check if waypoint to go to is valid.
		if ( (m_iTaskDestination == -1) || (m_iTaskDestination == iCurrentWaypoint) )
		{
			BotMessage( "%s -> task %s, invalid waypoint %d (current %d), recalculate task.", GetName(), 
			            CTypeToString::BotTaskToString(m_iCurrentTask).c_str(), m_iTaskDestination, iCurrentWaypoint );
			m_iCurrentTask = -1;
			m_bNeedTaskCheck = true; // Check new task in next frame.
			m_bNeedMove = m_bUseNavigatorToMove = m_bDestinationChanged = false;

			// Bot searched for item at current waypoint, but item was not there. Add it to array of picked items.
			TaskFinished();
		}
		else
		{
			BotMessage( "%s -> new task: %s %s, waypoint %d (current %d).", GetName(), CTypeToString::BotTaskToString(m_iCurrentTask).c_str(),
			            pEntityClass ? pEntityClass->sClassName.c_str() : "", m_iTaskDestination, iCurrentWaypoint );

			m_iDestinationWaypoint = m_iTaskDestination;
			m_bNeedMove = m_bUseNavigatorToMove = m_bDestinationChanged = true;
		}
	}
}

void CBot_HL2DM::TaskFinished()
{
	if ( (EBotTaskFindHealth <= m_cItemToSearch.iType) && (m_cItemToSearch.iType <= EBotTaskFindAmmo) )
	{
		const CEntity& cItem = CItems::GetItems(m_cItemToSearch.iType)[m_cItemToSearch.iIndex];

		// If item is not respawnable (or just bad configuration), force to not to search for it again right away, but in 1 minute.
		m_cItemToSearch.fRemoveTime = CBotrixPlugin::fTime;
		m_cItemToSearch.fRemoveTime += FLAG_ALL_SET(FEntityRespawnable, cItem.iFlags) ? cItem.pItemClass->GetArgument() : 60.0f;

		m_aPickedItems.push_back(m_cItemToSearch);
	}
	else // Bot was heading to waypoint, not item.
	{
		//m_aWaypoints.set(m_cItemToSearch.iIndex);

		m_cItemToSearch.fRemoveTime = CBotrixPlugin::fTime + 60.0f; // Do not go at that waypoint again at least for 1 minute.
		m_aPickedItems.push_back(m_cItemToSearch);
	}


}
