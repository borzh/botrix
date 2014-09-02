#include <good/string_utils.h>

#include "bot.h"
#include "mod.h"
#include "players.h"
#include "server_plugin.h"
#include "type2string.h"
#include "waypoint.h"

#include "mods/css/event_css.h"
#include "mods/borzh/mod_borzh.h"
#include "mods/hl2dm/mod_hl2dm.h"
#include "mods/tf2/mod_tf2.h"

//----------------------------------------------------------------------------------------------------------------
TModId CMod::m_iModId;
good::string CMod::sModName;
IMod* CMod::pCurrentMod = NULL;

StringVector CMod::aBotNames;
good::vector<CEventPtr> CMod::m_aEvents;
good::vector< good::pair<TFrameEvent, TPlayerIndex> > CMod::m_aFrameEvents;

bool CMod::m_bMapHas[EItemTypeNotObject]; // Health, armor, weapon, ammo.

StringVector CMod::aTeamsNames;
int CMod::iUnassignedTeam = 0;
int CMod::iSpectatorTeam = 1;

StringVector CMod::aClassNames;

bool CMod::bIntelligenceInBotName = true;
bool CMod::bHeadShotDoesMoreDamage = true;

//TDeathmatchFlags CMod::iDeathmatchFlags = -1;

int CMod::iPlayerHeight = 72;
int CMod::iPlayerHeightCrouched = 36;
int CMod::iPlayerWidth = 36;
Vector CMod::vPlayerCollisionHull(iPlayerWidth, iPlayerWidth, iPlayerHeight);

int CMod::iPlayerEyeLevel = 64;
int CMod::iPlayerEyeLevelCrouched = 28;

int CMod::iPlayerMaxObstacleHeight = 18;
int CMod::iPlayerNormalJumpHeight = 20;
int CMod::iPlayerJumpCrouchHeight = 56;

int CMod::iPlayerRadius;
int CMod::iNearItemMaxDistanceSqr = SQR(312);
int CMod::iItemPickUpDistance = 100;

int CMod::iPlayerMaxSlopeGradient = 45;
int CMod::iPlayerMaxHeightNoFallDamage = 185;

int CMod::iPlayerMaxArmor = 100;
int CMod::iPlayerMaxHealth = 100;

float CMod::fMaxCrouchVelocity = 63.33f;
float CMod::fMaxWalkVelocity = 150.0f;
float CMod::fMaxRunVelocity = 190.0f;
float CMod::fMaxSprintVelocity = 327.5f;
float CMod::fMinNonStuckSpeed = fMaxCrouchVelocity / 2.0f;

int CMod::iPointTouchSquaredXY = SQR(iPlayerWidth/4);
int CMod::iPointTouchSquaredZ = SQR(iPlayerJumpCrouchHeight);
int CMod::iPointTouchLadderSquaredZ = SQR(2);


//----------------------------------------------------------------------------------------------------------------
bool CMod::Load( TModId iModId )
{
    m_iModId = iModId;
    m_aEvents.clear();
    m_aEvents.reserve(16);

    m_aFrameEvents.clear();
    m_aFrameEvents.reserve(8);

    bool bResult = true;
    switch ( iModId )
    {
#ifdef BOTRIX_TF2
    case EModId_TF2:
        bHeadShotDoesMoreDamage = false;
        bResult &= AddEvent(new CPlayerActivateEvent());
        bResult &= AddEvent(new CPlayerTeamEvent());
        bResult &= AddEvent(new CPlayerSpawnEvent());
        bResult &= AddEvent(new CPlayerHurtEvent());
        bResult &= AddEvent(new CPlayerDeathEvent());
        // bResult &= AddEvent(new CPlayerChatEvent());

        // TODO: events https://wiki.alliedmods.net/Team_Fortress_2_Events
        // player_changeclass ctf_flag_captured
        // teamplay_point_startcapture achievement_earned
        // player_calledformedic
        // teamplay_round_active

        iPlayerHeight = 83;
        iPlayerHeightCrouched = 56;
        iPlayerWidth = 49;
        vPlayerCollisionHull = Vector(iPlayerWidth, iPlayerWidth, iPlayerHeight);

        // TODO: for class  https://developer.valvesoftware.com/wiki/TF2/Team_Fortress_2_Mapper%27s_Reference
        iPlayerEyeLevel = 68;
        iPlayerEyeLevelCrouched = 48;

        iPlayerMaxObstacleHeight = 18;
        iPlayerNormalJumpHeight = 43;
        iPlayerJumpCrouchHeight = 70;

        iPlayerMaxSlopeGradient = 45;
        iPlayerMaxHeightNoFallDamage = 269;

        iPlayerMaxArmor = 0; // TODO:
        iPlayerMaxHealth = 100;

        fMaxCrouchVelocity = 800.0f;
        fMaxWalkVelocity = 800.0f;
        fMaxRunVelocity = 800.0f;
        fMaxSprintVelocity = 800.0f;
        fMinNonStuckSpeed = 30.0f;

        iPointTouchSquaredXY = SQR(iPlayerWidth/4);
        iPointTouchSquaredZ = SQR(iPlayerJumpCrouchHeight);
        iPointTouchLadderSquaredZ = SQR(5);

        pCurrentMod = new CModTF2();
        break;
#endif

#ifdef BOTRIX_HL2DM
    case EModId_HL2DM:
        // TODO: move to hl2dm mod.
        bResult &= AddEvent(new CPlayerActivateEvent());
        bResult &= AddEvent(new CPlayerTeamEvent());
        bResult &= AddEvent(new CPlayerSpawnEvent());
        bResult &= AddEvent(new CPlayerHurtEvent());
        bResult &= AddEvent(new CPlayerDeathEvent());
//        bResult &= AddEvent(new CPlayerChatEvent());

        pCurrentMod = new CModHL2DM();
        break;
#endif

#ifdef BOTRIX_BORZH
    case EModId_Borzh:
        pCurrentMod = new CModBorzh();
        break;
#endif

#ifdef BOTRIX_MOD_CSS
    case EModId_CSS:
        bResult &= AddEvent(new CRoundStartEvent());
        bResult &= AddEvent(new CWeaponFireEvent());
        bResult &= AddEvent(new CBulletImpactEvent());
        bResult &= AddEvent(new CPlayerFootstepEvent());
        bResult &= AddEvent(new CBombDroppedEvent());
        bResult &= AddEvent(new CBombPickupEvent());
        pCurrentMod = new CModCss();
        break;
#endif

    default:
        BLOG_E("Error: unknown mod.");
        BreakDebugger();
    }

    iPlayerRadius = (int)FastSqrt( SQR(iPlayerWidth>>1) + SQR(iPlayerHeight>>1) );
    return bResult;
}

//----------------------------------------------------------------------------------------------------------------
bool CMod::AddEvent( CEvent* pEvent )
{
    bool bResult = CBotrixPlugin::pGameEventManager &&
                   CBotrixPlugin::pGameEventManager->AddListener( CBotrixPlugin::instance, pEvent->GetName().c_str(), true );
    if ( bResult )
        m_aEvents.push_back( CEventPtr(pEvent) );
    return bResult;
}

//----------------------------------------------------------------------------------------------------------------
void CMod::MapLoaded()
{
    // TODO: move this to items.
    for ( TItemType iType=0; iType < EItemTypeNotObject; ++iType )
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

    if ( pCurrentMod )
        pCurrentMod->MapLoaded();
}

//----------------------------------------------------------------------------------------------------------------
const good::string& CMod::GetRandomBotName( TBotIntelligence iIntelligence )
{
    int iIdx = rand() % aBotNames.size();
    for ( int i=iIdx; i<aBotNames.size(); ++i )
        if ( !IsNameTaken(aBotNames[i], iIntelligence) )
            return aBotNames[i];
    for ( int i=iIdx-1; i>=0; --i )
        if ( !IsNameTaken(aBotNames[i], iIntelligence) )
            return aBotNames[i];
    if ( iIdx < 0 ) // All names taken.
        iIdx = rand() % aBotNames.size();
    return aBotNames[iIdx];
}

//----------------------------------------------------------------------------------------------------------------
void CMod::ExecuteEvent( void* pEvent, TEventType iType )
{
    IEventInterface *pInterface = CEvent::GetEventInterface(pEvent, iType);
    if ( pInterface == NULL )
        return;

    const char* szEventName = pInterface->GetName();
    GoodAssert( szEventName );

    for ( good::vector<CEventPtr>::iterator it = m_aEvents.begin(); it != m_aEvents.end(); ++it )
    {
        if ( (*it)->GetName() == szEventName )
        {
            (*it)->Execute(pInterface);
            break;
        }
    }
}

//----------------------------------------------------------------------------------------------------------------
bool CMod::IsNameTaken( const good::string& cName, TBotIntelligence iIntelligence )
{
    for ( TPlayerIndex iPlayer=0; iPlayer<CPlayers::Size(); ++iPlayer )
    {
        const CPlayer* pPlayer = CPlayers::Get(iPlayer);
        if ( pPlayer && good::starts_with( good::string(pPlayer->GetName()), cName ) &&
             pPlayer->IsBot() && ( iIntelligence == ((CBot*)pPlayer)->GetIntelligence()) )
            return true;
    }
    return false;
}

//----------------------------------------------------------------------------------------------------------------
void CMod::Think()
{
    if ( pCurrentMod )
        pCurrentMod->Think();

    for ( int i = 0; i < m_aFrameEvents.size(); ++i )
    {
        CPlayer* pPlayer = CPlayers::Get( m_aFrameEvents[i].second );
        if ( pPlayer == NULL )
            BLOG_E( "Player with index %d is not present to receive event %d.", m_aFrameEvents[i].second, m_aFrameEvents[i].first );
        else
        {
            switch ( m_aFrameEvents[i].first )
            {
            case EFrameEventActivated:
                pPlayer->Activated();
                break;
            case EFrameEventRespawned:
                pPlayer->Respawned();
                break;
            default:
                GoodAssert(false);
            }
        }
    }

    m_aFrameEvents.clear();
}
