#ifdef BOTRIX_MOD_CSS
#   include "mods/css/event_css.h"
#elif BOTRIX_MOD_BORZH
#   include "mods/borzh/mod_borzh.h"
#endif

#include "mod.h"
#include "players.h"
#include "server_plugin.h"
#include "type2string.h"
#include "waypoint.h"


//----------------------------------------------------------------------------------------------------------------
TModId CMod::m_iModId;
good::string CMod::sModName;
IMod* CMod::pCurrentMod = NULL;

StringVector CMod::m_aBotNames;
good::vector<StringVector> CMod::m_aModels;
good::vector<CEventPtr> CMod::m_aEvents;

bool CMod::m_bMapHas[EEntityTypeTotal-1]; // Health, armor, weapon, ammo.

StringVector CMod::aTeamsNames;
int CMod::iUnassignedTeam = 0;
int CMod::iSpectatorTeam = 1;
bool CMod::bIntelligenceInBotName = true;


//----------------------------------------------------------------------------------------------------------------
void CMod::Load( TModId iModId )
{
    m_iModId = iModId;
    m_aEvents.clear();

    switch ( iModId )
    {
    case EModId_HL2DM:
        // TODO: move to hl2dm mod.
        AddEvent(new CPlayerActivateEvent());
        AddEvent(new CPlayerTeamEvent());
        AddEvent(new CPlayerSpawnEvent());

        AddEvent(new CPlayerHurtEvent());
        AddEvent(new CPlayerDeathEvent());

        AddEvent(new CPlayerChatEvent());
        break;

#ifdef BOTRIX_MOD_BORZH
    case EModId_Borzh:
        pCurrentMod = new CModBorzh();
        break;
#endif

#ifdef BOTRIX_MOD_CSS
    case EModId_CSS:
        AddEvent(new CRoundStartEvent());
        AddEvent(new CWeaponFireEvent());
        AddEvent(new CBulletImpactEvent());
        AddEvent(new CPlayerFootstepEvent());
        AddEvent(new CBombDroppedEvent());
        AddEvent(new CBombPickupEvent());
        break;
#endif

    default:
        CUtil::Message(NULL, "Error: unknown mod.");
        BreakDebugger();
    }
}

//----------------------------------------------------------------------------------------------------------------
void CMod::AddEvent( CEvent* pEvent )
{
    if (CBotrixPlugin::pGameEventManager)
        CBotrixPlugin::pGameEventManager->AddListener( CBotrixPlugin::instance, pEvent->GetName().c_str(), true );
    if (CBotrixPlugin::pGameEventManager2)
        CBotrixPlugin::pGameEventManager2->AddListener( CBotrixPlugin::instance, pEvent->GetName().c_str(), true );
    m_aEvents.push_back( CEventPtr(pEvent) );
}

//----------------------------------------------------------------------------------------------------------------
void CMod::MapLoaded()
{
    if ( !pCurrentMod )
        return;

    // TODO: move this to items.
    for ( TEntityType iType=0; iType < EEntityTypeTotal-1; ++iType )
    {
        m_bMapHas[iType] = false;

        if ( CItems::GetItems(iType).size() > 0 ) // Has items.
            m_bMapHas[iType] = true;
        else
        {
            TWaypointFlags iFlags = CWaypoint::GetFlagsFor(iType);

            // Check if map has waypoints of given type.
            for ( TWaypointId id=0; id < CWaypoints::Size(); ++id )
                if ( FLAG_SOME_SET(CWaypoints::Get(id).iFlags, iFlags) )
                {
                    m_bMapHas[iType] = true;
                    break;
                }
        }
    }

    pCurrentMod->MapLoaded();
}

//----------------------------------------------------------------------------------------------------------------
const good::string& CMod::GetRandomBotName()
{
    int iIdx = rand() % m_aBotNames.size();
    for ( size_t i=iIdx; i<m_aBotNames.size(); ++i )
        if ( !IsNameTaken(m_aBotNames[i]) )
            return m_aBotNames[i];
    for ( int i=iIdx-1; i>=0; --i )
        if ( !IsNameTaken(m_aBotNames[i]) )
            return m_aBotNames[i];
    return m_aBotNames[iIdx];
}
//----------------------------------------------------------------------------------------------------------------
void CMod::ExecuteEvent( void* pEvent, TEventType iType )
{
    IEventInterface *pInterface = CEvent::GetEventInterface(pEvent, iType);
    if ( pInterface == NULL )
        return;

    const char* szEventName = pInterface->GetName();

    for ( good::vector<CEventPtr>::iterator it = m_aEvents.begin(); it != m_aEvents.end(); ++it )
    {
        if ( (*it)->GetName() == szEventName )
        {
            (*it)->Execute(pInterface);
            break;
        }
    }

    delete pInterface;
}

//----------------------------------------------------------------------------------------------------------------
bool CMod::IsNameTaken(const good::string& cName)
{
    for ( TPlayerIndex iPlayer=0; iPlayer<CPlayers::Size(); ++iPlayer )
    {
        const CPlayer* pPlayer = CPlayers::Get(iPlayer);
        if ( pPlayer && (cName == pPlayer->GetName()) )
            return true;
    }
    return false;
}

