#ifdef BOTRIX_HL2DM


#include <good/string_buffer.h>
#include <good/string_utils.h>

#include "clients.h"
#include "type2string.h"

#include "mods/hl2dm/bot_hl2dm.h"
#include "mods/hl2dm/mod_hl2dm.h"


extern char* szMainBuffer;
extern int iMainBufferSize;

//----------------------------------------------------------------------------------------------------------------
CBot_HL2DM::CBot_HL2DM( edict_t* pEdict, TBotIntelligence iIntelligence ):
    CBot(pEdict, iIntelligence, 0), m_aWaypoints(CWaypoints::Size()),
    m_cItemToSearch(-1, -1), m_cSkipWeapons( CWeapons::Size() )
{
    m_bShootAtHead = false;
}


//----------------------------------------------------------------------------------------------------------------
void CBot_HL2DM::Activated()
{
    CBot::Activated();
    GoodAssert( m_pPlayerInfo && m_pController );

    CBotrixPlugin::pEngineServer->SetFakeClientConVarValue( m_pEdict, "cl_autowepswitch", "0" );
    CBotrixPlugin::pEngineServer->SetFakeClientConVarValue( m_pEdict, "cl_defaultweapon", "weapon_smg1" );

    TTeam iTeam = GetTeam();
    if ( iTeam == CMod::iUnassignedTeam ) // Deathmatch.
        iTeam = 2 + ( rand()&1 ); // 0 = deathmatch, 1 = spectator, 2 = rebels, 3 = combines.

    const good::string* pModel = ((CModHL2DM*)CMod::pCurrentMod)->GetRandomModel(iTeam);
    if ( pModel )
    {
        //CBotrixPlugin::pServerPluginHelpers->ClientCommand( m_pEdict, "cl_playermodel" + pModel->c_str() );
        CBotrixPlugin::pEngineServer->SetFakeClientConVarValue(m_pEdict, "cl_playermodel", pModel->c_str());
    }

    m_aWaypoints.reset();
    m_iFailWaypoint = EWaypointIdInvalid;

    m_iCurrentTask = EBotTaskInvalid;
    m_bNeedTaskCheck = true;
    m_bChasing = false;
}


//----------------------------------------------------------------------------------------------------------------
void CBot_HL2DM::Respawned()
{
    CBot::Respawned();
    GoodAssert( m_pPlayerInfo && m_pController );

    m_aWaypoints.reset();
    m_iFailWaypoint = EWaypointIdInvalid;

    m_iCurrentTask = EBotTaskInvalid;
    m_bNeedTaskCheck = true;
    m_bChasing = false;
}


//----------------------------------------------------------------------------------------------------------------
void CBot_HL2DM::KilledEnemy( int /*iPlayerIndex*/, CPlayer* /*pVictim*/ )
{
}


//----------------------------------------------------------------------------------------------------------------
void CBot_HL2DM::HurtBy( int iPlayerIndex, CPlayer* pAttacker, int iHealthNow )
{
    if ( pAttacker && (pAttacker != this) )
        CheckEnemy(iPlayerIndex, pAttacker, false);
    if ( iHealthNow < (CMod::iPlayerMaxHealth/2) )
        m_bNeedTaskCheck = true; // Check if need search for health.
}


//----------------------------------------------------------------------------------------------------------------
void CBot_HL2DM::Think()
{
    GoodAssert( !m_bTest );
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

    // Check if needs to add new tasks.
    if ( m_bNeedTaskCheck )
    {
        m_bNeedTaskCheck = false;
        /*if ( bForceNewTask || m_bFlee
#ifdef BOTRIX_CHAT
            || ( !m_bObjectiveChanged && (m_iObjective == EBotChatUnknown) )
#endif
            )*/
        CheckNewTasks(bForceNewTask);
    }

    if ( m_pCurrentEnemy && !m_bFlee && (m_iCurrentTask != EBotTaskEngageEnemy) )
    {
        m_iCurrentTask = EBotTaskEngageEnemy;
        m_pChasedEnemy = m_pCurrentEnemy;
        m_bNeedMove = false;
    }

    if ( m_iCurrentTask == EBotTaskEngageEnemy )
    {
        if ( !m_bNeedMove ||
            (!m_bUseNavigatorToMove && !CWaypoint::IsValid(iNextWaypoint) ) )
        {
            // TODO: come close if very far. Vector vDiff =
            if ( m_bChasing && (m_pCurrentEnemy != m_pChasedEnemy) &&    // Bot not seeing enemy, arrived where enemy was.
                (m_pChasedEnemy->iCurrentWaypoint != iCurrentWaypoint) ) // Should not be at the same waypoint.
                ChaseEnemy();
            else // Bot arrived at adyacent waypoint or is seeing enemy.
            {
                m_bNeedMove = m_bDestinationChanged = true;
                m_bUseNavigatorToMove = m_bChasing = false;
                iNextWaypoint = CWaypoints::GetRandomNeighbour(iCurrentWaypoint);
                BotMessage( "%s -> Moving to waypoint %d (current %d)", GetName(), iNextWaypoint, iCurrentWaypoint );
            }
        }
        if ( m_pCurrentEnemy != m_pChasedEnemy ) // Start/stop seeing enemy or enemy change.
        {
            if ( m_pCurrentEnemy ) // Seeing new enemy.
                m_pChasedEnemy = m_pCurrentEnemy;
            else if ( !m_bChasing ) // Lost sight of enemy, chase.
                ChaseEnemy();
        }
    }

    if ( m_bFlee )
        m_bNeedSprint = true; // Force bot to move rapidly. TODO: check if stops.
}


//----------------------------------------------------------------------------------------------------------------
void CBot_HL2DM::ReceiveChat( int /*iPlayerIndex*/, CPlayer* /*pPlayer*/, bool /*bTeamOnly*/, const char* /*szText*/ )
{

}


//----------------------------------------------------------------------------------------------------------------
bool CBot_HL2DM::DoWaypointAction()
{
    if ( m_bUseNavigatorToMove && (iCurrentWaypoint == m_iTaskDestination) )
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

    const CWeapon* pWeapon = ( m_bFeatureWeaponCheck && CWeapon::IsValid(m_iBestWeapon) ) ? m_aWeapons[m_iBestWeapon].GetBaseWeapon() : NULL;
    TBotIntelligence iWeaponPreference = m_iIntelligence;

    bool bNeedHealth = CMod::HasMapItems(EEntityTypeHealth) && ( m_pPlayerInfo->GetHealth() < CMod::iPlayerMaxHealth );
    bool bNeedHealthBad = bNeedHealth && ( m_pPlayerInfo->GetHealth() < (CMod::iPlayerMaxHealth/2) );
    bool bAlmostDead = bNeedHealthBad && ( m_pPlayerInfo->GetHealth() < (CMod::iPlayerMaxHealth/5) );
    bool bNeedWeapon = pWeapon && CMod::HasMapItems(EEntityTypeWeapon);
    bool bNeedAmmo = pWeapon && CMod::HasMapItems(EEntityTypeAmmo);

    TWeaponId iWeapon = EWeaponIdInvalid;
    bool bSecondary = false;

    const CEntityClass* pEntityClass = NULL; // Weapon or ammo class to search for.

    if ( bAlmostDead )
    {
        m_bDontAttack = (m_iIntelligence <= EBotNormal);
        m_bFlee = true;
    }

    int retries = 0;

restart_find_task: // TODO: remove gotos.
    retries++;
    if ( retries == 5 )
    {
        iNewTask = EBotTaskFindEnemy;
        pEntityClass = NULL;
        goto find_enemy;
    }

    if ( bNeedHealthBad ) // Need health pretty much.
    {
        iNewTask = EBotTaskFindHealth;
        bForce = true;
    }
    else if ( bNeedWeapon && (pWeapon->iBotPreference < iWeaponPreference) ) // Need some weapon with higher preference.
    {
        iNewTask = EBotTaskFindWeapon;
    }
    else if ( bNeedAmmo )
    {
        // Need ammunition.
        bool bNeedAmmo0 = (m_aWeapons[m_iBestWeapon].ExtraBullets(0) < pWeapon->iClipSize[0]); // Has less than 1 extra clip.
        bool bNeedAmmo1 = pWeapon->bHasSecondary && !pWeapon->bSecondaryUseSameBullets &&      // Has secondary function, but no secondary bullets.
                          !m_aWeapons[m_iBestWeapon].HasAmmoInClip(1) && !m_aWeapons[m_iBestWeapon].HasAmmoExtra(1);

        if ( bNeedAmmo0 || bNeedAmmo1 )
        {
            iNewTask = EBotTaskFindAmmo;
            // Prefer search for secondary ammo only if has extra bullets for primary.
            bSecondary = bNeedAmmo1 && (m_aWeapons[m_iBestWeapon].ExtraBullets(0) > 0);
            pEntityClass = pWeapon->aAmmos[bSecondary][ rand() % pWeapon->aAmmos[bSecondary].size() ];
        }
        else if ( bNeedHealth ) // Need health (but has more than 50%).
            iNewTask = EBotTaskFindHealth;
        else if ( CMod::HasMapItems(EEntityTypeArmor) && (m_pPlayerInfo->GetArmorValue() < CMod::iPlayerMaxArmor) ) // Need armor.
            iNewTask = EBotTaskFindArmor;
        else if ( bNeedWeapon && (pWeapon->iBotPreference < EBotPro) ) // Check if can find a better weapon.
        {
            iNewTask = EBotTaskFindWeapon;
            iWeaponPreference = pWeapon->iBotPreference+1;
        }
        else if ( !m_aWeapons[m_iBestWeapon].FullAmmo(1) ) // Check if weapon needs secondary ammo.
        {
            iNewTask = EBotTaskFindAmmo;
            bSecondary = true;
        }
        else if ( !m_aWeapons[m_iBestWeapon].FullAmmo(0) ) // Check if weapon needs primary ammo.
        {
            iNewTask = EBotTaskFindAmmo;
            bSecondary = false;
        }
        else
            iNewTask = EBotTaskFindEnemy;
    }
    else
        iNewTask = EBotTaskFindEnemy;

    switch(iNewTask)
    {
    case EBotTaskFindWeapon:
        pEntityClass = NULL;

        // Get weapon entity class to search for. Search for better weapons that actually have.
        for ( TBotIntelligence iPreference = iWeaponPreference; iPreference < EBotIntelligenceTotal; ++iPreference )
        {
            iWeapon = CWeapons::GetRandomWeapon(iPreference, m_cSkipWeapons);
            if ( iWeapon != EWeaponIdInvalid )
            {
                pEntityClass = CWeapons::Get(iWeapon).GetBaseWeapon()->pWeaponClass;
                break;
            }
        }
        if ( pEntityClass == NULL )
        {
            // None found, search for worst weapons.
            for ( TBotIntelligence iPreference = iWeaponPreference-1; iPreference >= 0; --iPreference )
            {
                iWeapon = CWeapons::GetRandomWeapon(iPreference, m_cSkipWeapons);
                if ( iWeapon != EWeaponIdInvalid )
                {
                    pEntityClass = CWeapons::Get(iWeapon).GetBaseWeapon()->pWeaponClass;
                    break;
                }
            }
        }
        if ( pEntityClass == NULL )
        {
            bNeedWeapon = false;
            goto restart_find_task;
        }
        break;

    case EBotTaskFindAmmo:
        // Get ammo entity class to search for.
        iWeapon = m_iBestWeapon;

        // Randomly search for weapon instead, as it gives same primary bullets.
        if ( !bSecondary && bNeedWeapon && CItems::ExistsOnMap(pWeapon->pWeaponClass) && (rand() & 1) )
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

    BASSERT( iNewTask != EBotTaskInvalid, return );

find_enemy:
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
                if ( (m_iCurrentTask == EBotTaskFindWeapon) && (iWeapon != EWeaponIdInvalid) )
                    m_cSkipWeapons.set(iWeapon);
                m_iCurrentTask = EBotTaskInvalid;
                goto restart_find_task;
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
    m_cSkipWeapons.reset();
}


//----------------------------------------------------------------------------------------------------------------
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


#endif // BOTRIX_HL2DM
