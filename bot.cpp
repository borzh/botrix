#include <stdio.h>
#include <math.h>

#include <good/string.h>
#include <good/string_buffer.h>

#include "clients.h"
#include "chat.h"
#include "waypoint_navigator.h"
#include "server_plugin.h"

#include "bot.h"

#include "game/shared/in_buttons.h"
#include "public/irecipientfilter.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "public/tier0/memdbgon.h"


//----------------------------------------------------------------------------------------------------------------
extern char* szMainBuffer;
extern int iMainBufferSize;


//----------------------------------------------------------------------------------------------------------------
float CBot::m_fTimeIntervalCheckUsingMachines = 0.5f;
int CBot::m_iCheckEntitiesPerFrame = 4;

//----------------------------------------------------------------------------------------------------------------
CBot::CBot( edict_t* pEdict, TPlayerIndex iIndex, TBotIntelligence iIntelligence ):
    CPlayer(pEdict, iIndex, true),
    m_iIntelligence(iIntelligence), r( rand()&0xFF ), g( rand()&0xFF ), b( rand()&0xFF ),
    m_aNearPlayers(CPlayers::Size()), m_aSeenEnemies(CPlayers::Size()), m_aEnemies(CPlayers::Size()),
    m_bPaused(false),
#if defined(DEBUG) || defined(_DEBUG)
    m_bDebugging(true),
#else
    m_bDebugging(false),
#endif
    m_bDontBreakObjects(false), m_bDontThrowObjects(false)
{
    m_aPickedItems.reserve(16);
    for ( TEntityType i=0; i < EEntityTypeTotal; ++i )
    {
        int iSize = CItems::GetItems(i).size() >> 4;
        if ( iSize == 0 )
            iSize = 2;
        m_aNearItems[i].reserve( iSize );
        m_aNearestItems[i].reserve( 4 );
    }
}

//----------------------------------------------------------------------------------------------------------------
void CBot::TestWaypoints( TWaypointId iFrom, TWaypointId iTo )
{
    DebugAssert( CWaypoints::IsValid(iFrom) && CWaypoints::IsValid(iTo) );
    CWaypoint& wFrom = CWaypoints::Get(iFrom);

    Vector vSetOrigin = wFrom.vOrigin;
    vSetOrigin.z -= CUtil::iPlayerEyeLevel; // Make bot appear on the ground (waypoints are at eye level).

    m_pController->SetAbsOrigin(vSetOrigin);

    iCurrentWaypoint = iFrom;
    m_iDestinationWaypoint = iTo;

    m_bTest = true;
}


//----------------------------------------------------------------------------------------------------------------
static char szBotBuffer[2048];

//----------------------------------------------------------------------------------------------------------------
void CBot::ConsoleCommand(const char* szFormat, ...)
{
    va_list vaList;
    va_start(vaList, szFormat);
    vsprintf(szBotBuffer, szFormat, vaList);
    va_end(vaList);

    CBotrixPlugin::pServerPluginHelpers->ClientCommand(m_pEdict, szBotBuffer);
}

//----------------------------------------------------------------------------------------------------------------
// Class used to send say messages to server.
//----------------------------------------------------------------------------------------------------------------
class UserRecipientFilter: public IRecipientFilter
{
public:
    UserRecipientFilter(int iUserIndex): m_iUserIndex(iUserIndex) {}

    virtual bool	IsReliable() const { return true; }
    virtual bool	IsInitMessage() const { return false; }

    virtual int		GetRecipientCount() const { return 1; }
    virtual int		GetRecipientIndex( int ) const { return m_iUserIndex;}
protected:
    int m_iUserIndex;
};

//----------------------------------------------------------------------------------------------------------------
void CBot::Say(bool bTeamOnly, const char* szFormat, ...)
{
    good::string_buffer sBuffer(szBotBuffer, 2048, false);
    if ( bTeamOnly )
        sBuffer << "(TEAM) " << GetName() << ": ";
    int iSize = sBuffer.size();

    va_list vaList;
    va_start(vaList, szFormat);
    vsprintf(&szBotBuffer[iSize], szFormat, vaList);
    va_end(vaList);

    //CBotrixPlugin::pServerPluginHelpers->ClientCommand(m_pEdict, szBotBuffer);

#ifndef BOTRIX_DONT_SEND_BOT_CHAT
    for ( int i = 0; i <= CPlayers::GetPlayersCount(); i++ )
    {
        CPlayer* pPlayer = CPlayers::Get(i);

        if ( !pPlayer || pPlayer->IsBot() )
            continue;

        if ( bTeamOnly && (pPlayer->GetTeam() != 0) && (pPlayer->GetTeam() != GetTeam()) )
            continue;

        UserRecipientFilter user( pPlayer->GetIndex() + 1 );
        bf_write* bf = CBotrixPlugin::pEngineServer->UserMessageBegin(&user, 3);
        if ( bf )
        {
            bf->WriteByte( m_iIndex );
            bf->WriteString( szBotBuffer );
            bf->WriteByte( true );
        }
        CBotrixPlugin::pEngineServer->MessageEnd();
    }

    // Echo to server console.
    if ( CBotrixPlugin::pEngineServer->IsDedicatedServer() )
         Msg( "%s", szBotBuffer );
#endif

    CBotrixPlugin::instance->GenerateSayEvent(m_pEdict, &szBotBuffer[iSize], bTeamOnly);
}
//----------------------------------------------------------------------------------------------------------------
void CBot::Activated()
{
    CPlayer::Activated();

    m_pPlayerInfo = CBotrixPlugin::pPlayerInfoManager->GetPlayerInfo(m_pEdict);
    m_pController = CBotrixPlugin::pBotManager->GetBotController(m_pEdict);

#ifndef BOTRIX_HL2DM_MOD
    CBotrixPlugin::pEngineServer->SetFakeClientConVarValue( m_pEdict, "cl_team", "default" );
    const good::string* sModel = CMod::GetRandomModel( m_pPlayerInfo->GetTeamIndex() );
    if ( sModel )
        CBotrixPlugin::pEngineServer->SetFakeClientConVarValue(m_pEdict, "cl_playermodel", sModel->c_str());
#endif

    m_bFirstRespawn = true;

#ifdef BOTRIX_CHAT
    m_iPrevChatMate = m_iPrevTalk = -1;
    m_bTalkStarted = false;

    m_cChat.iSpeaker = m_iIndex;
#endif
}

//----------------------------------------------------------------------------------------------------------------
void CBot::Respawned()
{
    CPlayer::Respawned();

    m_cCmd.Reset();
    m_cCmd.viewangles = m_pController->GetLocalAngles();

    m_pNavigator.Stop();

    //m_vLastVelocity.x = m_vLastVelocity.y = m_vLastVelocity.z = 0.0f;

    m_fPrevThinkTime = m_fStuckCheckTime = 0.0f;

    for ( TEntityType i=0; i < EEntityTypeTotal; ++i )
    {
        m_iNextNearItem[i] = 0;
        m_aNearItems[i].clear();
        m_aNearestItems[i].clear();
    }
    m_aAvoidAreas.clear();

    m_iNextCheckPlayer = 0;
    m_aNearPlayers.reset();
    m_aSeenEnemies.reset();
    m_pCurrentEnemy = NULL;
    m_iObjective = EBotChatUnknown;

    // Don't clear picked items, as bot still know which were taken previously.
    m_iCurrentPickedItem = 0;

    CWeapons::GetRespawnWeapons( m_aWeapons, m_pPlayerInfo->GetTeamIndex() );

    // Check if bot has physcannon / manual weapon.
    m_iPhyscannon = m_iManualWeapon = -1;
    for ( size_t i=0; i < m_aWeapons.size(); ++i)
    {
        if ( m_aWeapons[i].IsPresent() )
        {
            if ( m_aWeapons[i].IsPhysics() )
                m_iPhyscannon = i;
            else if ( m_aWeapons[i].IsManual() )
                m_iManualWeapon = i;
        }
    }

    // Set bot's best weapon.
    m_iWeapon = CWeapons::GetBestRangedWeapon(m_aWeapons);
    if ( m_iWeapon == -1 )
        m_iWeapon = m_iManualWeapon; // Get manual weapon.
    if ( m_iWeapon == -1 )
        m_iWeapon = 0; // Get first weapon.

    m_iBestWeapon = m_iWeapon;

    const good::string& sWeaponName = m_aWeapons[m_iWeapon].GetName();
    m_pController->SetActiveWeapon( sWeaponName.c_str() );
    DebugAssert( sWeaponName == m_pPlayerInfo->GetWeaponName() );

    // Set default flags.
#ifdef BOTRIX_CHAT
    m_bPerformingRequest = false;
#endif
    m_bObjectiveChanged = m_bUnderAttack = m_bDontAttack = m_bFlee = m_bNeedSetWeapon = m_bNeedReload = m_bAttackDuck = false;

    m_bNeedAim = m_bUseSideLook = false;
    m_bDontAttack = m_bDestinationChanged = m_bNeedMove = m_bLastNeedMove = m_bUseNavigatorToMove = m_bTest;

    m_bLockAim = m_bLockNavigatorMove = m_bLockMove = m_bLockAll = false;
    m_bMoveFailure = false;

    m_bStuck = m_bNeedCheckStuck = m_bStuckBreakObject = m_bStuckUsePhyscannon = false;
    m_bStuckTryingSide = m_bStuckTryGoLeft = m_bStuckGotoCurrent = m_bStuckGotoPrevious = m_bRepeatWaypointAction = false;

    m_bLadderMove = m_bNeedStop = m_bNeedDuck = m_bNeedWalk = m_bNeedSprint = false;
    m_bNeedFlashlight = m_bUsingFlashlight = false;
    m_bNeedUse = m_bAlreadyUsed = m_bUsingHealthMachine = false;
    m_bNeedAttack = m_bNeedAttack2 = m_bNeedJumpDuck = m_bNeedJump = false;

    m_fNextDrawNearObjectsTime = 0.0f;

    // Check near items (skip objects).
    Vector vFoot = m_pController->GetLocalOrigin();
    for ( TEntityType iType = 0; iType < EEntityTypeObject; ++iType )
    {
        good::vector<TEntityIndex>& aNear = m_aNearItems[iType];
        good::vector<TEntityIndex>& aNearest = m_aNearestItems[iType];

        const good::vector<CEntity>& aItems = CItems::GetItems(iType);
        if ( aItems.size() == 0)
            continue;

        for ( TEntityIndex i = 0; i < (int)aItems.size(); ++i )
        {
            const CEntity* pItem = &aItems[i];

            if ( pItem->IsFree() || !pItem->IsOnMap() ) // Item is picked or broken.
                continue;

            float fDistSqr = vFoot.DistToSqr( pItem->CurrentPosition() );
            if ( fDistSqr <= pItem->fRadiusSqr*4 )
                aNearest.push_back(i);
            else if ( fDistSqr <= CUtil::iNearItemMaxDistanceSqr ) // Item is close.
                aNear.push_back(i);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------
void CBot::Dead()
{
    CPlayer::Dead();
    if ( m_bTest )
        CPlayers::KickBot(this);
}

//----------------------------------------------------------------------------------------------------------------
void CBot::PlayerDisconnect( int iPlayerIndex, CPlayer* pPlayer )
{
    m_aNearPlayers.reset(iPlayerIndex);
    m_aSeenEnemies.reset(iPlayerIndex);

    if ( pPlayer == m_pCurrentEnemy )
    {
        m_pCurrentEnemy = NULL;
        m_bUnderAttack = m_bAttackDuck = false;
        //CheckEnemy();
    }
}

//----------------------------------------------------------------------------------------------------------------
#ifdef BOTRIX_CHAT
void CBot::ReceiveChatRequest( const CBotChat& cRequest )
{
    DebugAssert( (cRequest.iDirectedTo == m_iIndex) && (cRequest.iSpeaker != -1));
    if (iChatMate == -1)
        iChatMate = cRequest.iSpeaker;

    CBotChat cResponse(EBotChatUnknown, m_iIndex, cRequest.iSpeaker); // TODO: m_cChat.
    if ( iChatMate == cRequest.iSpeaker )
    {
        // If conversation is not started, decide whether to help teammate or not (random).
        if ( !m_bTalkStarted )
        {
            m_bTalkStarted = true;
            m_iPrevTalk = -1;
            m_bHelpingMate = rand()&1;
            if ( !m_bHelpingMate ) // Force to help admin of the server.
            {
                CPlayer* pSpeaker = CPlayers::Get(cRequest.iSpeaker);
                if ( !pSpeaker->IsBot() )
                {
                    CClient* pClient = (CClient*)pSpeaker;
                    if ( FLAG_ALL_SET(FCommandAccessBot, pClient->iCommandAccessFlags) )
                        m_bHelpingMate = true;
                }
            }
        }

        if ( m_iPrevTalk == -1 ) // This is a request from other player, generate response.
        {
            // We want to know what bot can answer.
            const good::vector<TBotChat>& aPossibleAnswers = CChat::PossibleAnswers(cRequest.iBotChat);
            if ( aPossibleAnswers.size() == 1 )
                cResponse.iBotChat = aPossibleAnswers[0];
            else if ( aPossibleAnswers.size() > 0 )
            {
                bool bAffirmativeNegativeAnswer = good::find(aPossibleAnswers.begin(), aPossibleAnswers.end(), EBotChatAffirm) != aPossibleAnswers.end();
                if ( bAffirmativeNegativeAnswer )
                {
                    bool bYesNoAnswer = good::find(aPossibleAnswers.begin(), aPossibleAnswers.end(), EBotChatAffirmative) != aPossibleAnswers.end();
                    cResponse.iBotChat = bYesNoAnswer ? ( (rand()&1) ? EBotChatAffirmative : EBotChatAffirm ) : EBotChatAffirm;
                    if ( m_bHelpingMate )
                        StartPerformingChatRequest(cRequest);
                    else
                        cResponse.iBotChat += 1; // Negate request (EBotChatAffirmative + 1 = EBotChatNegative).
                }
                else
                    cResponse.iBotChat = aPossibleAnswers[ rand() % aPossibleAnswers.size() ];
            }
        }
        else // This is a response to this bot's request.
        {
            // TODO: this part is not implemented yet.
            switch ( cRequest.iBotChat )
            {
            case EBotChatAffirm:
            case EBotChatAffirmative:

            case EBotChatNegative:
            case EBotChatNegate:
            case EBotChatBusy:
                // Ask another bot?
            default:;
            }
        }

        if ( cRequest.iBotChat == EBotChatBye )
        {
            m_iPrevChatMate = iChatMate;
            m_iPrevTalk = EBotChatBye;
            iChatMate = -1;
        }
    }
    else if ( cRequest.iBotChat == EBotChatBye )
    {
        if ( (m_iPrevTalk != EBotChatBye) || (m_iPrevChatMate != cRequest.iSpeaker) )
            cResponse.iBotChat = EBotChatBye;
    }
    else
        cResponse.iBotChat = EBotChatBusy;

    // Generate chat.
    if ( cResponse.iBotChat != -1 )
    {
        const good::string& sText = CChat::ChatToText(cResponse);
        Say(false, sText.c_str());
    }
}

//----------------------------------------------------------------------------------------------------------------
void CBot::StartPerformingChatRequest( const CBotChat& cRequest )
{
    m_bPerformingRequest = true;
    m_iObjective = cRequest.iBotChat;

    switch ( m_iObjective )
    {
    case EBotChatStop:
        m_bNeedMove = false;
        m_bRequestTimeout = true;
        m_fEndTalkActionTime = CBotrixPlugin::fTime + 30.0f;
        break;
    case EBotChatCome:
    case EBotChatFollow:
    case EBotChatAttack:
    case EBotChatNoKill:
    case EBotChatSitDown:
    case EBotChatStandUp:
    case EBotChatJump:
    case EBotChatLeave:

    default:
        DebugAssert(false); // Should not enter.
    }
}

//----------------------------------------------------------------------------------------------------------------
void CBot::EndPerformingChatRequest( bool bSayGoodbye )
{
    m_bPerformingRequest = m_bRequestTimeout = false;
    if ( bSayGoodbye )
    {
        m_cChat.cMap.clear();
        m_cChat.iBotChat = EBotChatBye;
        if ( rand()&1 )
        {
            m_cChat.iDirectedTo = iChatMate;
            m_cChat.cMap.push_back( CChatVarValue(CChat::iPlayerVar, 0, iChatMate) );
        }
        else
            m_cChat.iDirectedTo = -1;

        bool bTeam = ( CPlayers::Get(iChatMate)->GetTeam() == GetTeam() );
        Say( bTeam, CChat::ChatToText(m_cChat).c_str() );
    }

    switch ( m_iObjective )
    {
    case EBotChatStop:
        m_bNeedMove = true;
        break;
    case EBotChatCome:
    case EBotChatFollow:
    case EBotChatAttack:
    case EBotChatNoKill:
    case EBotChatSitDown:
    case EBotChatStandUp:
    case EBotChatJump:
    case EBotChatLeave:

    default:
        DebugAssert(false); // Should not enter.
    }
}
#endif // BOTRIX_CHAT

//----------------------------------------------------------------------------------------------------------------
void CBot::PreThink()
{
    if ( m_bPaused )
        return;

    if ( m_bFirstRespawn )
    {
        Respawned(); // Force respawn, as first time bot appeares on map, Respawned() is not called.
        m_bFirstRespawn = false;
    }

    Vector vPrevOrigin = m_vHead;
    int iPrevCurrWaypoint = iCurrentWaypoint;

    CPlayer::PreThink();

    if ( m_pPlayerInfo->IsDead() ) // CBasePlayer::IsDead() returns true only when player became dead,  but when
        m_bAlive = false;          // player is respawnable (but still dead) it returns false.

    // Reset bot's command.
    m_cCmd.forwardmove = 0;
    m_cCmd.sidemove = 0;
    m_cCmd.upmove = 0;
    m_cCmd.buttons = 0;
    m_cCmd.impulse = 0;

    if ( m_bAlive ) // Update near objects, items, players and current weapon.
        UpdateWorld();

    if ( !m_bTest )
        Think(); // Mod's think.

    if ( m_bAlive )
    {
        PerformMove( iPrevCurrWaypoint, vPrevOrigin );

        if ( m_bNeedMove && m_bUseNavigatorToMove && m_pNavigator.SearchEnded() )
            m_pNavigator.DrawPath(r, g, b, m_vHead);

        // Show near items in white and nearest in red.
        if ( m_bDebugging )
        {
            static const float fDrawNearObjectsTime = 0.1f;
            if ( CBotrixPlugin::fTime > m_fNextDrawNearObjectsTime )
            {
                m_fNextDrawNearObjectsTime = CBotrixPlugin::fTime + fDrawNearObjectsTime;
                for ( TEntityType iType=0; iType < EEntityTypeTotal; ++iType)
                {
                    const good::vector<CEntity>& aItems = CItems::GetItems(iType);
                    for ( size_t i=0; i < m_aNearItems[iType].size(); ++i) // Draw near items with white color.
                    {
                        ICollideable* pCollide = aItems[ m_aNearItems[iType][i] ].pEdict->GetCollideable();
                        CUtil::DrawBox(pCollide->GetCollisionOrigin(), pCollide->OBBMins(), pCollide->OBBMaxs(),
                                       fDrawNearObjectsTime, 0xFF, 0xFF, 0xFF, pCollide->GetCollisionAngles());
                    }

                    for ( size_t i=0; i < m_aNearestItems[iType].size(); ++i) // Draw nearest items with red color.
                    {
                        ICollideable* pCollide = aItems[ m_aNearestItems[iType][i] ].pEdict->GetCollideable();
                        CUtil::DrawBox(pCollide->GetCollisionOrigin(), pCollide->OBBMins(), pCollide->OBBMaxs(),
                                       fDrawNearObjectsTime, 0xFF, 0x00, 0x00, pCollide->GetCollisionAngles());
                    }
                }
            }
        }
    }

    m_pController->RunPlayerMove(&m_cCmd);
#ifndef BOTRIX_SOURCE_2013
    m_pController->PostClientMessagesSent();
#endif

    m_fPrevThinkTime = CBotrixPlugin::fTime;

    // Bot created for testing and couldn't find path or reached destination and finished using health/armor machine.
    if ( m_bTest && ( m_bMoveFailure || (!m_bNeedMove && !m_bNeedUse) ) )
        CPlayers::KickBot(this);
}







//================================================================================================================
// CBot virtual protected methods.
//================================================================================================================
void CBot::CurrentWaypointJustChanged()
{
    if ( m_bNeedMove && m_bUseNavigatorToMove )
    {
        if ( m_pNavigator.SearchEnded() )
        {
            if ( iCurrentWaypoint == iNextWaypoint ) // Bot becomes closer to next waypoint in path.
            {
                if ( CWaypoint::IsValid(m_iAfterNextWaypoint) )
                {
                    // Set forward look to next waypoint in path.
                    m_vForward = CWaypoints::Get(m_iAfterNextWaypoint).vOrigin;

                     // Don't change look while using actions.
                    if ( m_bUnderAttack || m_bLadderMove || m_bNeedStop || m_bNeedWalk || m_bNeedDuck || m_bNeedSprint ||
                         m_bNeedAttack || m_bNeedAttack2 || m_bNeedJump || m_bNeedJumpDuck )
                    {
                        //m_bLockAim = true;
                    }
                    else
                    {
                        m_vLook = m_vForward;
                        m_bNeedAim = true;
                        m_fEndAimTime = CBotrixPlugin::fTime + GetEndLookTime();
                    }
                }
            }
            else // Something went wrong (bot falls or pushed?).
            {
#if defined(DEBUG) || defined(_DEBUG)
                BotMessage("%s -> invalid current waypoint %d (should be %d).", GetName(), iCurrentWaypoint, iNextWaypoint);
#endif
                m_bMoveFailure = true;
                iCurrentWaypoint = EWaypointIdInvalid;
                m_pNavigator.Stop();
            }
        }
        else
            m_bDestinationChanged = true; // Search not ended, but waypoint changed (?), restart search from current waypoint.
    }
}

//----------------------------------------------------------------------------------------------------------------
bool CBot::DoWaypointAction()
{
    if ( !m_bNeedMove )
        return false;

    DebugAssert( m_bUseNavigatorToMove );
    DebugAssert( CWaypoint::IsValid(iCurrentWaypoint) );
    CWaypoint& w = CWaypoints::Get(iCurrentWaypoint);

    if ( m_bAlreadyUsed )
    {
        // When coming back to current waypoint (if stucked) don't use it again, but make this variable false at next waypoint.
        m_bAlreadyUsed = FLAG_SOME_SET(FWaypointHealthMachine | FWaypointArmorMachine, w.iFlags);
    }
    else
    {
        // Check if need health/armor.
        if ( FLAG_SOME_SET(FWaypointHealthMachine, w.iFlags) )
        {
            m_iLastHealthArmor = m_pPlayerInfo->GetHealth();
            m_bNeedUse = m_iLastHealthArmor < m_pPlayerInfo->GetMaxHealth();
            m_bUsingHealthMachine = true;
        }

        else if ( FLAG_SOME_SET(FWaypointArmorMachine, w.iFlags) )
        {
            m_iLastHealthArmor = m_pPlayerInfo->GetArmorValue();
            m_bNeedUse = m_iLastHealthArmor < CUtil::iPlayerMaxArmor;
            m_bUsingHealthMachine = false;
        }

        if ( m_bNeedUse )
        {
            QAngle angNeeded;
            CWaypoint::GetFirstAngle(angNeeded, w.iArgument);
            CUtil::NormalizeAngle(angNeeded.x);
            CUtil::NormalizeAngle(angNeeded.y);

            AngleVectors(angNeeded, &m_vForward);
            m_vForward *= 1000.0f;
            m_vForward += m_vHead;
            m_vLook = m_vForward;

            m_fStartActionTime = m_fEndAimTime = CBotrixPlugin::fTime + GetEndLookTime();
            m_fEndActionTime = m_fStartActionTime + m_fTimeIntervalCheckUsingMachines;

            m_bLockNavigatorMove = m_bLockAim = m_bNeedAim = true;
        }
    }

    // To stop, bot must not start from current waypoint.
    //if ( CWaypoint::IsValid(iPrevWaypoint) && (iCurrentWaypoint != iPrevWaypoint) &&
    //     FLAG_SOME_SET(FWaypointStop, CWaypoints::Get(iCurrentWaypoint).iFlags) )
    //	m_bNeedStop = true;

    m_bNeedStop = FLAG_SOME_SET(FWaypointStop, CWaypoints::Get(iCurrentWaypoint).iFlags);

    return m_bNeedUse;
}

//----------------------------------------------------------------------------------------------------------------
void CBot::ApplyPathFlags()
{
    //DebugAssert( m_bNeedMove && m_bUseNavigatorToMove );
    //BotMessage("%s -> waypoint %d", m_pPlayerInfo->GetName(), iCurrentWaypoint);

    // Release buttons and locks.
    m_bNeedFlashlight = m_bAlreadyUsed = m_bNeedUse = false;
    m_bNeedJumpDuck = m_bNeedJump = m_bNeedAttack = m_bNeedAttack2 = m_bNeedSprint = m_bNeedDuck = m_bNeedWalk = false;
    m_bLockNavigatorMove = m_bLockAim = false;

    m_bLockAim = false;

    if ( CWaypoint::IsValid(iNextWaypoint) )
    {
        CWaypoint& wNext = CWaypoints::Get(iNextWaypoint);
        m_vDestination = wNext.vOrigin;

        if ( iCurrentWaypoint == iNextWaypoint ) // Happens when bot stucks.
        {
            if ( CWaypoint::IsValid(m_iAfterNextWaypoint) )
                m_vForward = CWaypoints::Get(m_iAfterNextWaypoint).vOrigin;
            else
                m_vForward = wNext.vOrigin;
        }
        else
        {
            m_vForward = wNext.vOrigin;

            CWaypointPath* pCurrentPath = CWaypoints::GetPath(iCurrentWaypoint, iNextWaypoint);
            DebugAssert( pCurrentPath );
            if ( pCurrentPath == NULL )
                return;

            m_bLadderMove = FLAG_ALL_SET(FPathLadder, pCurrentPath->iFlags);

            if ( FLAG_ALL_SET(FPathStop, pCurrentPath->iFlags) )
                m_bNeedStop = true;

            if ( FLAG_ALL_SET(FPathSprint, pCurrentPath->iFlags) )
                m_bNeedSprint = true;

            if ( FLAG_ALL_SET(FPathFlashlight, pCurrentPath->iFlags) )
                m_bNeedFlashlight = true;

            if ( FLAG_ALL_SET(FPathCrouch, pCurrentPath->iFlags) &&
                !FLAG_ALL_SET(FPathJump, pCurrentPath->iFlags) )
                m_bNeedDuck = true; // Crouch only when not jumping.

            //m_bLockAim = FLAG_SOME_SET( FPathJump | FPathBreak | FPathSprint | FPathLadder | FPathStop, pCurrentPath->iFlags);
        }
    }

    if ( m_bLockAim && !m_bUnderAttack )
    {
        m_bNeedAim = true;
        m_vLook = m_vForward; // Always look to next waypoint when aim is locked.
        m_fEndAimTime = CBotrixPlugin::fTime + GetEndLookTime();
    }
}

//----------------------------------------------------------------------------------------------------------------
void CBot::DoPathAction()
{
    DebugAssert( CWaypoint::IsValid(iCurrentWaypoint) );

    if ( CWaypoint::IsValid(iNextWaypoint) && (iCurrentWaypoint != iNextWaypoint) )
    {
        CWaypointPath* pCurrentPath = CWaypoints::GetPath(iCurrentWaypoint, iNextWaypoint);
        DebugAssert( pCurrentPath );

        if ( FLAG_ALL_SET(FPathBreak, pCurrentPath->iFlags) )
            m_bNeedAttack = true;

        if ( FLAG_SOME_SET(FPathJump | FPathTotem, pCurrentPath->iFlags) )
            m_bNeedJump = true;

        if ( FLAG_ALL_SET(FPathCrouch | FPathJump, pCurrentPath->iFlags) || FLAG_SOME_SET(FPathTotem, pCurrentPath->iFlags) )
            m_bNeedJumpDuck = true;

        if ( m_bNeedAttack || m_bNeedJump || m_bNeedJumpDuck )
        {
            int iStartTime = GET_1ST_BYTE(pCurrentPath->iArgument); // Action (jump/ jump with duck / attack) start time in deciseconds at first byte.
            int iDuration = GET_2ND_BYTE(pCurrentPath->iArgument);  // Action duration (duck for jump / attack for break) in deciseconds at second byte.
            if (iDuration == 0)
                iDuration = 10; // Duration 1 second by default. Good for either duck while jumping or attack (to break something).

            m_fStartActionTime = CBotrixPlugin::fTime + (iStartTime / 10.0f);
            m_fEndActionTime = CBotrixPlugin::fTime + ( (iStartTime + iDuration) / 10.0f);
        }
    }

    m_bRepeatWaypointAction = false;
}

//----------------------------------------------------------------------------------------------------------------
void CBot::PickItem( const CEntity& cItem, TEntityType iEntityType, TEntityIndex iIndex )
{
    switch ( iEntityType )
    {
    case EEntityTypeHealth:
        BotMessage("%s -> Picked %s. Health now %d.", GetName(), cItem.pItemClass->sClassName.c_str(), m_pPlayerInfo->GetHealth());
        break;
    case EEntityTypeArmor:
        BotMessage("%s -> Picked %s. Armor now %d.", GetName(), cItem.pItemClass->sClassName.c_str(), m_pPlayerInfo->GetArmorValue());
        break;
    case EEntityTypeWeapon:
    {
        TWeaponId iWeaponId = CWeapons::GetIdFromWeaponName(cItem.pItemClass->sClassName);
        DebugAssert( iWeaponId >= 0 );
        m_aWeapons[iWeaponId].AddWeapon();

        BotMessage("%s -> Picked %s (%d, %d).", GetName(), cItem.pItemClass->sClassName.c_str(), m_aWeapons[iWeaponId].ExtraBullets(0), m_aWeapons[iWeaponId].ExtraBullets(1));

        CheckWeapon();
        break;
    }
    case EEntityTypeAmmo:
    {
        bool bSec;
        int iAmmo;
        TWeaponId iWeaponId = CWeapons::GetIdFromAmmo(cItem.pItemClass, bSec, iAmmo);
        DebugAssert( iWeaponId >= 0 );
        m_aWeapons[iWeaponId].AddBullets(iAmmo, bSec);

        BotMessage("%s -> Picked %s (%d, %d).", GetName(), cItem.pItemClass->sClassName.c_str(), m_aWeapons[iWeaponId].ExtraBullets(0), m_aWeapons[iWeaponId].ExtraBullets(1));

        CheckWeapon();
        break;
    }
    case EEntityTypeObject:
        BotMessage("%s -> Breaked object %s", GetName(), cItem.pItemClass->sClassName.c_str());
        break;
    default:
        DebugAssert(0);
        break;
    }

    if ( iEntityType != EEntityTypeObject )
    {
        CPickedItem cPickedItem( iEntityType, iIndex, CBotrixPlugin::fTime );

        // If item is not respawnable (or just bad configuration), force to not to search for it again right away, but in 1 minute at least.
        // TODO: check if item is respawnable.
        cPickedItem.fRemoveTime += /*FLAG_ALL_SET(FEntityRespawnable, cItem.iFlags) ? cItem.pItemClass->GetArgument() : */60.0f;
        m_aPickedItems.push_back( cPickedItem );
    }
}




//================================================================================================================
// CBot protected methods.
//================================================================================================================
#ifdef BOTRIX_CHAT
void CBot::Speak( bool bTeamSay )
{
    TPlayerIndex iPlayerIndex = m_cChat.iDirectedTo;
    if ( iPlayerIndex == EPlayerIndexInvalid )
        iPlayerIndex = m_iPrevChatMate;
    if ( iPlayerIndex != EPlayerIndexInvalid )
        m_cChat.cMap.push_back( CChatVarValue(CChat::iPlayerVar, 0, iPlayerIndex) );
    const good::string& sText = CChat::ChatToText(m_cChat);
    CUtil::Message( NULL, "%s: %s %s", GetName(), bTeamSay? "say_team" : "say", sText.c_str() );
    Say( bTeamSay, sText.c_str() );
}
#else
void CBot::Speak( bool ) {}
#endif


//----------------------------------------------------------------------------------------------------------------
bool CBot::IsVisible( CPlayer* pPlayer ) const
{
    // Check PVS first. Get visible clusters from player's position.
    Vector vAim(pPlayer->GetHead());

    CUtil::SetPVSForVector(m_vHead);
    if ( !CUtil::IsVisiblePVS(vAim) )
        return false;

    // First check if other player is in bot's view cone.
    static const float fFovHorizontal = 60.0f;              // Giving 120 degree of view horizontally.
    static const float fFovVertical = fFovHorizontal * 3/4; // Normal monitor has 4:3 aspect ratio.

    vAim -= m_vHead; // vAim contains vector to enemy relative from bot.

    QAngle angPlayer;
    VectorAngles(vAim, angPlayer);

    CUtil::GetAngleDifference(angPlayer, m_cCmd.viewangles, angPlayer);
    if ( (-fFovHorizontal <= angPlayer.x) && (angPlayer.x <= fFovHorizontal) &&
         (-fFovVertical <= angPlayer.y) && (angPlayer.y <= fFovVertical) )
         return CUtil::IsVisible( m_vHead, pPlayer->GetEdict() ); // CUtil::IsVisible( m_vHead, vAim, FVisibilityAll );
    else
        return false;
}


//----------------------------------------------------------------------------------------------------------------
float CBot::GetEndLookTime()
{
    // angle 180 degrees = aAimSpeed time   ->   Y time = angle X * aAimSpeed / 180 degrees
    // angle X           = Y time
    //static const float aAimSpeed[EBotIntelligenceTotal] = { 1.20f, 0.85f, 0.67f, 0.57f, 1.42f };
    static const int iAngDiff = 40; // 30 degrees to make time difference in bot's aim.
    static const int iEndAimSize = 180/iAngDiff + 1;
    static const float aEndAimTime[EBotIntelligenceTotal][iEndAimSize] =
    {
        //  40    80     120    160    180
        { 0.50f, 0.70f, 0.90f, 1.10f, 1.30f },
        { 0.50f, 0.60f, 0.70f, 0.80f, 0.90f },
        { 0.50f, 0.55f, 0.60f, 0.65f, 0.70f },
        { 0.40f, 0.45f, 0.50f, 0.55f, 0.60f },
        { 0.25f, 0.30f, 0.35f, 0.40f, 0.45f },
    };

    Vector vDestinationAim( m_vLook );
    vDestinationAim -= m_vHead; // Get abs vector.

    QAngle angNeeded;
    VectorAngles(vDestinationAim, angNeeded);

    CUtil::GetAngleDifference(m_cCmd.viewangles, angNeeded, angNeeded);

    int iPitch = abs( (int)angNeeded.x );
    int iYaw = abs( (int)angNeeded.y );

    iPitch /= iAngDiff;
    iYaw /= iAngDiff;

    DebugAssert( iPitch <= iEndAimSize );
    DebugAssert( iYaw <= iEndAimSize );

    if ( iPitch < iYaw)
        iPitch = iYaw; // iPitch = MAX2( iPitch, iYaw );

    float fAimTime = aEndAimTime[m_iIntelligence][iPitch];
    DebugAssert( fAimTime < 2.0f );

    return fAimTime;
    //return CBotrixPlugin::fTime + iPitch * aAimSpeed[m_iIntelligence] / 180.0f;
}


//----------------------------------------------------------------------------------------------------------------
void CBot::UpdateWorld()
{
    // Update picked items.
    if ( m_aPickedItems.size() )
    {
        if ( CBotrixPlugin::fTime >= m_aPickedItems[m_iCurrentPickedItem].fRemoveTime )
            m_aPickedItems.erase(m_aPickedItems.begin() + m_iCurrentPickedItem);
        else
            ++m_iCurrentPickedItem;
        if ( m_iCurrentPickedItem >= m_aPickedItems.size() )
            m_iCurrentPickedItem = 0;
    }

    // Update bot's weapon.
    m_aWeapons[m_iWeapon].GameFrame();
    if ( m_aWeapons[m_iWeapon].GetName() != m_pPlayerInfo->GetWeaponName() ) // Happens when out of bullets automatically.
    {
        BotMessage( "%s -> Current weapon %s, should be %s.", GetName(), m_pPlayerInfo->GetWeaponName(), m_aWeapons[m_iWeapon].GetName().c_str() );
        m_pController->SetActiveWeapon(m_aWeapons[m_iWeapon].GetName().c_str());
    }

    // Get near items.
    Vector vFoot = m_pController->GetLocalOrigin();
    for ( TEntityType iType=0; iType < EEntityTypeTotal; ++iType )
    {
        const good::vector<CEntity>& aItems = CItems::GetItems(iType);
        if ( aItems.size() == 0)
            continue;

        good::vector<TEntityIndex>& aNearest = m_aNearestItems[iType];
        int iNearestSize = aNearest.size();

        good::vector<TEntityIndex>& aNear = m_aNearItems[iType];
        int iNearSize = aNear.size();

        // Update nearest items.
        for ( int i = 0; i < iNearestSize; )
        {
            const CEntity& cItem = aItems[ aNearest[i] ];
            if ( cItem.IsFree() ) // Remove object if it is removed from game.
            {
                aNearest.erase(aNearest.begin() + i);
                --iNearestSize;
            }
            else if ( !cItem.IsOnMap() ) // Was on map before, but disappeared, bot could grab it or break it.
            {
                PickItem( cItem, iType, aNearest[i] );
                aNearest.erase(aNearest.begin() + i);
                --iNearestSize;
            }
            else if ( vFoot.DistToSqr(cItem.CurrentPosition()) > cItem.fRadiusSqr ) // Item becomes far.
            {
                aNear.push_back(aNearest[i]);
                aNearest.erase(aNearest.begin() + i);
                --iNearestSize;
            }
            else
                 ++i;
        }

        // Check if bot becomes too close to near items, to pass it to nearest items (and viceversa).
        for ( int i = 0; i < iNearSize; )
        {
            const CEntity& cItem = aItems[ aNear[i] ];
            if ( cItem.IsFree() || !cItem.IsOnMap() )
            {
                aNear.erase(aNear.begin() + i);
                --iNearSize;
            }
            else
            {
                float fDistSqr = vFoot.DistToSqr( cItem.CurrentPosition() );

                if ( fDistSqr > CUtil::iNearItemMaxDistanceSqr ) // Item becomes far.
                {
                    aNear.erase(aNear.begin() + i);
                    --iNearSize;
                }
                else
                {
                    if ( fDistSqr <= cItem.fRadiusSqr ) // Can pick up.
                    {
                        aNearest.push_back(aNear[i]);
                        aNear.erase(aNear.begin() + i);
                        --iNearSize;
                    }
                    else
                        ++i;
                }
            }
        }

        // Check if bot becomes close to items to consider them near (m_iCheckEntitiesPerFrame items max).
        size_t iCheckTo = m_iNextNearItem[iType] + m_iCheckEntitiesPerFrame;
        if ( iCheckTo > aItems.size() )
            iCheckTo = aItems.size();

        for ( size_t i = m_iNextNearItem[iType]; i < iCheckTo; ++i )
        {
            const CEntity* pItem = &aItems[i];

            if (   pItem->IsFree() || !pItem->IsOnMap() ||                           // Item is picked or broken.
                 ( find( aNear.begin(), aNear.end(), i ) != aNear.end() ) ||         // Item is near, all checks were already made before for all near items.
                 ( find( aNearest.begin(), aNearest.end(), i ) != aNearest.end() ) ) // Item is nearest, all checks were already made before for all nearest items.
                continue;

            float fDistSqr = vFoot.DistToSqr( pItem->CurrentPosition() );
            if ( fDistSqr <= pItem->fRadiusSqr )
                aNearest.push_back(i);
            else if ( fDistSqr <= CUtil::iNearItemMaxDistanceSqr ) // Item is close.
                aNear.push_back(i);
        }
        m_iNextNearItem[iType] = ( iCheckTo == aItems.size() ) ? 0 : iCheckTo;
    }

    // Check only 1 player per frame.
    CPlayer* pEnemy = NULL;
    while ( m_iNextCheckPlayer < CPlayers::Size() )
    {
        CPlayer* pPlayer = CPlayers::Get(m_iNextCheckPlayer);
        if ( pPlayer && (this != pPlayer) )
        {
            pEnemy = pPlayer;
            break;
        }
        m_iNextCheckPlayer++;
    }

    if ( pEnemy )
    {
        if ( pEnemy->IsAlive() )
        {
            float fDistSqr = m_vHead.DistToSqr( pEnemy->GetHead() );
            if ( fDistSqr <= CUtil::iNearItemMaxDistanceSqr )
            {
                m_aNearPlayers.set(m_iNextCheckPlayer);

                // Check if players are not stucked with each other.
                if ( !m_bStuckTryingSide && ( fDistSqr <= (SQR(CUtil::iPlayerRadius) << 2) ) )
                {
                    Vector vNeeded(m_vDestination), vOther( pEnemy->GetHead() );
                    vNeeded -= m_vHead;
                    vOther -= m_vHead;

                    QAngle angNeeded, angOther;
                    VectorAngles(vNeeded, angNeeded);
                    VectorAngles(vOther, angOther);

                    float fYaw = angNeeded.y - angOther.y;
                    CUtil::DeNormalizeAngle(fYaw);

                    if ( (-45.0f < fYaw) && (fYaw < 45.0f) )
                    {
                        // Player stucked with this one (move right for 1/4 seconds).
                        m_bStuckTryingSide = true;
                        m_bStuckTryGoLeft = false;
                        m_fEndActionTime = CBotrixPlugin::fTime + 0.25f;
                    }
                }
            }
            else
                m_aNearPlayers.reset(m_iNextCheckPlayer);
        }
        else
            m_aNearPlayers.reset(m_iNextCheckPlayer);

        // Check if this enemy can be seen / should be attacked.
        if ( IsEnemy(pEnemy) )
            CheckEnemy(m_iNextCheckPlayer, pEnemy, true);
    }

    m_iNextCheckPlayer++;
    if ( m_iNextCheckPlayer >= CPlayers::Size() )
        m_iNextCheckPlayer = 0;
}


//----------------------------------------------------------------------------------------------------------------
void CBot::CheckEnemy( int iPlayerIndex, CPlayer* pPlayer, bool bCheckVisibility )
{
    DebugAssert( m_pCurrentEnemy != this );

    if ( m_bDontAttack )
    {
        m_bUnderAttack = m_bAttackDuck = false;
        return;
    }

    bool bEnemyChanged = false;

    if ( pPlayer->IsAlive() && ( !bCheckVisibility || IsVisible(pPlayer) ) ) // Currently seeing this player.
    {
        m_aSeenEnemies.set(iPlayerIndex);

        float fDistanceSqr = m_vHead.DistToSqr( pPlayer->GetHead() );
        bEnemyChanged = ( (m_pCurrentEnemy == NULL) ||
                        ( (m_pCurrentEnemy != pPlayer) && (fDistanceSqr < m_fDistanceSqrToEnemy + 256) ) ); // Add 16^2 to not to change enemy too often.
        if ( bEnemyChanged )
        {
            m_pCurrentEnemy = pPlayer;
            m_fDistanceSqrToEnemy = fDistanceSqr;
        }
    }
    else // Can't see this player anymore or player is dead.
    {
        m_aSeenEnemies.reset(iPlayerIndex);
        if ( m_pCurrentEnemy == pPlayer )
        {
            m_pCurrentEnemy = NULL;
            m_bUnderAttack = m_bAttackDuck = false;
        }
    }

    if ( bEnemyChanged ) // TODO: Don't shoot on stairs.
    {
        m_bUnderAttack = true;
        m_bEnemyAimed = false; // Still need to aim at enemy before shooting.
        m_bStuckBreakObject = m_bStuckUsePhyscannon = false; // Don't change weapon while under attack.

        EnemyAim();
    }

    if ( m_bUnderAttack && (m_pCurrentEnemy == pPlayer) ) // Check if it is close attack.
    {
        if ( m_fDistanceSqrToEnemy <= SQR(512) )
        {
            m_bCloseAttack = true;
            // Fool and stupied bots attack while standing, normal ducks and smart/pro will move left/right.
            if ( m_iIntelligence >= EBotNormal )
            {
                if ( !m_bNeedDuck && !m_bAttackDuck ) // Duck only if not ducking already.
                {
                    Vector vSrc(m_vHead);
                    vSrc.z -= CUtil::iPlayerEyeLevel - CUtil::iPlayerEyeLevelCrouched;
                    m_bAttackDuck = CUtil::IsVisible( vSrc, m_pCurrentEnemy->GetHead()); // Duck, if enemy is visible while ducking.
                }
            }
            //else if ( m_iIntelligence >= EBotSmart ) // Bot is smart.
            //{
            //	if ( fDistanceSqr <= SQR(256) ) // TODO: parametrize this.
            //}
        }
        else
            m_bCloseAttack = false;
    }
}


//----------------------------------------------------------------------------------------------------------------
void CBot::EnemyAim()
{
    DebugAssert( m_pCurrentEnemy );
    //Vector vOldLook(m_vLook);

    Vector vEnemyCenter;
    m_pCurrentEnemy->GetCenter( vEnemyCenter );

    if ( !m_bShootAtHead && CUtil::IsVisible(m_vHead, vEnemyCenter) )
        m_vLook = vEnemyCenter;
    else
        m_vLook = m_pCurrentEnemy->GetHead();

    //float fRand = fDistanceToEnemy / CUtil::iMaxMapSize;
    m_vLook.x += 10 - (rand()%20); // -10 .. + 10
    m_vLook.y += 10 - (rand()%20); // -10 .. + 10
    m_vLook.z += 10 - (rand()%20); // -10 .. + 10

    m_fEndAimTime = GetEndLookTime();

    // Take player's speed into account.
    if ( m_iIntelligence >= EBotNormal )
    {
        Vector vInc( m_pCurrentEnemy->GetHead() );
        vInc -= m_pCurrentEnemy->GetPreviousHead();

        float fFramesLeft = m_fEndAimTime * CBotrixPlugin::iFPS; // How many frames left to aim?
        vInc *= fFramesLeft;

        m_vLook += vInc;
    }

    m_fEndAimTime += CBotrixPlugin::fTime;
    m_bNeedAim = true;
}


//----------------------------------------------------------------------------------------------------------------
void CBot::CheckWeapon()
{
    CWeaponWithAmmo& cWeapon = m_aWeapons[m_iWeapon];
    if ( !cWeapon.IsPresent() )
        return; // No weapons?

    if ( m_pCurrentEnemy && cWeapon.CanUse(m_fDistanceSqrToEnemy) )
        return;

    if ( !cWeapon.CanChange() ) // Currently shooting, can't change.
        return;

    // If not engaging enemy, reload some weapons.
    if ( (m_pCurrentEnemy == NULL) && m_bNeedReload )
    {
        if ( !cWeapon.CanUse() ) // Currently reloading or changing.
            return;

        if ( cWeapon.NeedReload(0) )
        {
            Reload();
            return;
        }

        int iIdx = -1;
        for ( size_t i = 0; i < m_aWeapons.size(); ++i )
            if ( m_aWeapons[i].IsPresent() && (m_aWeapons[i].NeedReload(0) || m_aWeapons[i].NeedReload(1)) )
            {
                iIdx = i;
                break;
            }

        if ( iIdx == -1 )
            m_bNeedReload = false; // Continue to select best weapon.
        else
        {
            SetActiveWeapon(iIdx);
            return;
        }
    }

    // Choose best ranged weapon.
    m_iBestWeapon = CWeapons::GetBestRangedWeapon(m_aWeapons);

    // Choose manual weapon.
    if ( m_iBestWeapon == -1 )
        m_iBestWeapon = m_iManualWeapon;

    if ( m_iBestWeapon == m_iWeapon )
    {
        if ( !m_aWeapons[m_iWeapon].HasAmmoInClip(0) )
            m_bStayReloading = true;
    }
    else
        SetActiveWeapon(m_iBestWeapon);

    m_bNeedSetWeapon = false;

    //m_bFlee = (m_pCurrentEnemy != NULL); // Run away if enemy is seen.
}

//----------------------------------------------------------------------------------------------------------------
void CBot::SetActiveWeapon( int iIndex )
{
    if ( iIndex == m_iWeapon )
        return;

    CWeaponWithAmmo& cOld = m_aWeapons[m_iWeapon];
    CWeaponWithAmmo& cNew = m_aWeapons[iIndex];
    DebugAssert( cOld.IsPresent() && cNew.IsPresent() );

    const good::string& sWeaponName = cNew.GetName();
    m_pController->SetActiveWeapon( sWeaponName.c_str() );

    if ( sWeaponName == m_pPlayerInfo->GetWeaponName() )
    {
        cOld.Holster( cNew );
        BotMessage( "%s -> Set weapon %s", GetName(), sWeaponName.c_str() );
        m_iWeapon = iIndex;
    }
}

//----------------------------------------------------------------------------------------------------------------
void CBot::Shoot( bool bSecondary )
{
    CWeaponWithAmmo& cWeapon = m_aWeapons[m_iWeapon];
    DebugAssert( cWeapon.IsPresent() );

    if ( !cWeapon.CanUse() )
        return;
    BotMessage( "%s -> Shoot %s %s, ammo %d/%d.", GetName(), bSecondary ? "secondary" : "primary", cWeapon.GetName().c_str(),
                  cWeapon.Bullets(bSecondary), cWeapon.ExtraBullets(bSecondary) );

    cWeapon.Shoot(bSecondary);
    FLAG_SET(bSecondary ? IN_ATTACK2 : IN_ATTACK, m_cCmd.buttons);

    if ( cWeapon.IsRanged() )
        m_bNeedReload = true;
}


//----------------------------------------------------------------------------------------------------------------
void CBot::CheckSideLook( bool bIsMoving, bool /*bNewDestination*/ )
{
    if ( m_bUnderAttack || !bIsMoving )
        return;

    /*if ( m_bUseSideLook && !m_bLockAim )
    {
        // Check if need to change look forward/backward/left/right while moving.
        bool bIsTime = CBotrixPlugin::fTime >= m_fRandomSideLookTime;

        // Bot needs to change look when it is moving and:
        //  - either it is time to change side look.
        //  - either waypoint is reached while looking forward.
        if ( bIsTime || (bNewDestination && (m_iCurrentSideLook == 0)) )
        {
            m_bNeedAim = true;

            Vector v[4]; // Forward, left, right, back.
            v[0] = m_vForward - m_vHead; // Forward.
            VectorVectors(v[0], v[2], v[3]); // Get right side.
            v[1] = -v[2]; // Left = -Right.
            v[3] = -v[0]; // Back = -Forward.

            // Get time to finish changing side look.
            //m_fEndAimTime = CBotrixPlugin::fTime + aEndAimTime[m_iIntelligence];

            if ( bIsTime )
            {
                static const float aBaseTime[4][EBotIntelligenceTotal] =
                {
                    { 11.0f, 9.0f, 7.0f, 5.0f, 3.0f }, // Base time to stay looking forward.
                    { 2.5f,  2.0f, 1.5f, 1.0f, 0.5f }, // Base time to stay looking left.
                    { 2.5f,  2.0f, 1.5f, 1.0f, 0.5f }, // Base time to stay looking right.
                    { 2.5f,  2.0f, 1.5f, 1.0f, 0.5f }, // Base time to stay looking back.
                };
                static const int aRandTime[4][EBotIntelligenceTotal] =
                {
                    { 7, 6, 5, 4, 3 }, // Random time to stay looking forward.
                    { 4, 4, 3, 3, 2 }, // Random time to stay looking left.
                    { 4, 4, 3, 3, 2 }, // Random time to stay looking right.
                    { 3, 3, 2, 2, 1 }, // Random time to stay looking back.
                };

                // Bot changes side look when time comes or destination is changed and it is looking forward.
                if (m_iCurrentSideLook == 0) // We were looking forward.
                {
                    do {
                        m_iCurrentSideLook = rand() & 3;
                    } while (m_iCurrentSideLook == 0);
                }
                else
                    m_iCurrentSideLook = 0; // Always look forward after looking other side.

                // Get time to start changing side look again.
                m_fRandomSideLookTime = aBaseTime[m_iCurrentSideLook][m_iIntelligence]  +  (rand() % aRandTime[m_iCurrentSideLook][m_iIntelligence]);

                // TODO: erase me.
                switch (m_iCurrentSideLook)
                {
                case 0: BotMessage("Forward %.1f", m_fRandomSideLookTime); break;
                case 1: BotMessage("Left %.1f", m_fRandomSideLookTime); break;
                case 2: BotMessage("Right %.1f", m_fRandomSideLookTime); break;
                case 3: BotMessage("Back %.1f", m_fRandomSideLookTime); break;
                }
            }

            // Make vector larger, so bot will not change it when pass it.
            v[m_iCurrentSideLook] *= 1000.0f;
            m_vLook = m_vHead;
            m_vLook += v[m_iCurrentSideLook];

            m_fEndAimTime = CBotrixPlugin::fTime + GetEndLookTime();

            if ( bIsTime )
                m_fRandomSideLookTime += m_fEndAimTime;
        }
    }

    else // Not using side look.*/
    {
        // Bot is moving using navigator, but doesn't need to change look, look at next waypoint.
        if ( m_bUseNavigatorToMove && !m_bNeedAim && !m_bLockAim )
        {
            m_bNeedAim = true;
            m_vLook = m_vForward;
            m_vLook -= m_vHead;
            m_vLook *= 1000.0f;
            m_vLook += m_vHead; // m_vLook = m_vHead + (m_vForward - m_vHead)*1000.0f
            m_fEndAimTime = CBotrixPlugin::fTime + GetEndLookTime();
        }
    }
}

//----------------------------------------------------------------------------------------------------------------
bool CBot::ResolveStuckMove()
{
    TWaypointId iPrevCurrWaypoint = iCurrentWaypoint;

    // Reset waypoint.
    iCurrentWaypoint = CWaypoints::GetNearestWaypoint(m_vHead);
    if ( !CWaypoint::IsValid(iCurrentWaypoint) )
    {
        m_bMoveFailure = true;
        m_pNavigator.Stop();
        return false;
    }

    const CEntity* pObject = NULL;
    CWaypoint& wCurr = CWaypoints::Get(iCurrentWaypoint);

    if ( m_aNearestItems[EEntityTypeObject].size() )
    {
        pObject = &CItems::GetItems(EEntityTypeObject)[ m_aNearestItems[EEntityTypeObject][0] ];
        // If object is behind, it doesn't disturb.
        Vector vObject = pObject->CurrentPosition();
        vObject -= m_vHead;

        Vector vGoing( CWaypoints::Get(iNextWaypoint).vOrigin );
        vGoing -= m_vHead;

        QAngle angGoing, angObject;
        VectorAngles(vObject, angObject);
        VectorAngles(vGoing, angGoing);

        CUtil::GetAngleDifference(angGoing, angObject, angGoing);

        if ( angGoing.y <= -90.0f || angGoing.y >= 90.0f )
            pObject = NULL;
    }

    if ( pObject )
    {
        if ( !m_bDontBreakObjects && CWeapon::IsValid(m_iManualWeapon) && m_aWeapons[m_iManualWeapon].IsPresent() &&
             pObject->IsBreakable() && !pObject->IsExplosive() )
        {
            // Look at origin of the object.
            m_vLook = pObject->CurrentPosition();
            m_fStartActionTime = m_fEndAimTime = CBotrixPlugin::fTime + GetEndLookTime(); // When start to attract object.

            m_bStuckBreakObject = m_bLockAim = true;
        }
        else if ( !m_bDontThrowObjects && CWeapon::IsValid(m_iPhyscannon) && m_aWeapons[m_iPhyscannon].IsPresent() && !pObject->IsHeavy() )
        {
            // Look at origin of the object.
            m_vLook = m_vDisturbingObjectPosition = pObject->CurrentPosition();

            m_fStartActionTime = m_fEndAimTime = CBotrixPlugin::fTime + GetEndLookTime();
            m_fEndActionTime = m_fStartActionTime + 2.0f; // 1 second: half to attract object and 1.5 to look back and shoot it.

            m_bStuckUsePhyscannon = m_bLockAim = true;
        }
        else
        {
            Vector vMins, vMaxs;
            pObject->pEdict->GetCollideable()->WorldSpaceSurroundingBounds(&vMins, &vMaxs);
            float zDistance = vMaxs.z - vMins.z;
            if ( zDistance <= CUtil::iPlayerJumpCrouchHeight ) // Can jump on it.
            {
                m_bNeedJump = m_bNeedJumpDuck = true;
                m_fStartActionTime = CBotrixPlugin::fTime;
                m_fEndActionTime = CBotrixPlugin::fTime + 0.25f;
            }
            else // Try going left/right and then jump.
            {
                m_bStuckTryingSide = true;
                m_bStuckTryGoLeft = !m_bStuckTryGoLeft;
                BotMessage("%s -> try going %s.", GetName(), m_bStuckTryGoLeft ? "left" : "right");
                m_bNeedJump = m_bNeedJumpDuck = true;
                m_fStartActionTime = CBotrixPlugin::fTime + 0.75f;
                m_fEndActionTime = CBotrixPlugin::fTime + 1.0f;
            }
        }
    }

    else if ( m_bUseNavigatorToMove )
    {
        if ( iCurrentWaypoint == iPrevCurrWaypoint ) // Got stucked because action didn't work?
        {
            m_bStuck = false;

            // First test if we are touching current waypoint.
            bool bTouch = wCurr.IsTouching(m_vHead, m_bLadderMove);
            if ( bTouch || m_bStuckGotoCurrent )
            {
                // Force to make action again.
                m_pNavigator.SetPreviousPathPosition();
                m_pNavigator.SetPreviousPathPosition();
                m_pNavigator.GetNextWaypoints(iNextWaypoint, m_iAfterNextWaypoint);

                m_bRepeatWaypointAction = true;

                BotMessage("%s -> stucked, but not lost, go to previous waypoint %d and touch it.", GetName(), iNextWaypoint);

                if ( !bTouch )
                {
                    ApplyPathFlags();       // Apply path flags from previous waypoint (to go back to current waypoint again).
                    m_bNeedJump = rand()&1; // Jump for just in case sometimes.
                }

                if ( m_bStuckGotoCurrent )
                    m_bStuckGotoCurrent = false; // Try to move left right next time.
            }
            else
            {
                // Check if next waypoint is more on left or right.
                CWaypoint& wNext = CWaypoints::Get(iNextWaypoint);
                Vector wNeed( wNext.vOrigin );
                wNeed -= wCurr.vOrigin;
                Vector wNow( m_vHead );
                wNow -= wCurr.vOrigin;

                QAngle angNeed, angNow;
                VectorAngles(wNeed, angNeed);
                VectorAngles(wNow, angNow);

                CUtil::GetAngleDifference(angNow, angNeed, angNow);

                m_bStuckTryGoLeft = ( angNow.y >= 0.0f ); // Need to increment angles, move left.
                m_bStuckTryingSide = true;
                m_bNeedJump = m_bNeedJumpDuck = true;
                m_fStartActionTime = CBotrixPlugin::fTime + 0.75f;
                m_fEndActionTime = CBotrixPlugin::fTime + 1.0f;

                BotMessage("%s -> stucked, but not lost, try to move %s.", GetName(), m_bStuckTryGoLeft ? "left" : "right");
                m_bStuckGotoCurrent = true; // Try to go back to current waypoint next time.
            }
            return true;
        }
        else // Got stucked, because bot falled down or someone pushed it.
        {
            m_bLockAim = m_bLockNavigatorMove = false; // Release locks.
            m_bNeedJumpDuck = m_bNeedJump = m_bNeedAttack2 = m_bNeedAttack = m_bNeedSprint = m_bNeedDuck = m_bNeedWalk = false;

            BotMessage("Stucked and lost, because of failure following path. Trying again...");

            if ( m_bTest )
            {
                m_bStuck = false;
                m_bDestinationChanged = true;
            }
            else // Lost path and stucked, move failure.
                m_bMoveFailure = true; // Let mod decide what to do.

            m_pNavigator.Stop();
        }
    }
    else // Not using navigator and stucked?
    {
        m_bMoveFailure = true; // Let mod decide what to do.
        m_pNavigator.Stop();
    }
    return false;
}

//----------------------------------------------------------------------------------------------------------------
bool CBot::NavigatorMove()
{
    bool bArrived = false;

    if ( m_bDestinationChanged )
    {
        // Destination changed, make sure to start new path search.
        m_bDestinationChanged = false;
        m_pNavigator.Stop();

        DebugAssert( CWaypoint::IsValid(iCurrentWaypoint) && CWaypoint::IsValid(m_iDestinationWaypoint) && (m_iDestinationWaypoint != iCurrentWaypoint) );
        m_bMoveFailure = !m_pNavigator.SearchSetup( iCurrentWaypoint, m_iDestinationWaypoint, m_aAvoidAreas );
        iPrevWaypoint = iCurrentWaypoint;
    }

    // Check if bot arrived to destination. Set up new destination to next waypoint in path.
    else if ( m_pNavigator.PathFound() )
    {
        if ( !CWaypoints::Get(iNextWaypoint).IsTouching(m_vHead, m_bLadderMove) )
            return false;

        iCurrentWaypoint = iNextWaypoint; // Force current waypoint become the one we just reached.

        bool bDoingAction = DoWaypointAction();

        m_bNeedMove = m_bNeedMove && m_pNavigator.HasMoreCoords();
        if ( m_bNeedMove )
        {
            m_pNavigator.GetNextWaypoints(iNextWaypoint, m_iAfterNextWaypoint);
            if ( !bDoingAction ) // If not doing some waypoint action.
            {
                ApplyPathFlags();
                DoPathAction();
            }
        }
        else
            m_iAfterNextWaypoint = iNextWaypoint = -1;

        return true; // We arrived to next waypoint.
    }

    if ( !m_bMoveFailure )
    {
        // Here search is still not finished.
        bArrived = m_pNavigator.SearchStep();

        if ( bArrived ) // Ended search just now.
        {
            m_bMoveFailure = !m_pNavigator.PathFound();

            if ( m_bMoveFailure )
            {
                BotMessage("%s -> can't reach waypoint %d.", GetName(), m_iDestinationWaypoint);
            }
            else
            {
                DebugAssert( m_pNavigator.PathFound() && m_pNavigator.HasMoreCoords() );
                m_pNavigator.GetNextWaypoints(iNextWaypoint, m_iAfterNextWaypoint);
                ApplyPathFlags();
                DebugAssert( iCurrentWaypoint == iNextWaypoint ); // First coord in path must be current waypoint.

                // If lost and iCurrentWaypoint == iNextWaypoint, perform waypoint 'touch'.
                if ( CWaypoints::Get(iNextWaypoint).IsTouching(m_vHead, m_bLadderMove) )
                {
                    m_pNavigator.GetNextWaypoints(iNextWaypoint, m_iAfterNextWaypoint);
                    if ( DoWaypointAction() == false ) // No need to perform waypoint action.
                    {
                        ApplyPathFlags();
                        DoPathAction();
                    }
                }
            }
        }
    }
    return bArrived;
}

//----------------------------------------------------------------------------------------------------------------
bool CBot::NormalMove()
{
    bool bArrived = CUtil::IsPointTouch3d(m_vHead, m_vDestination);
    return bArrived;
}

//----------------------------------------------------------------------------------------------------------------
void CBot::PerformMove( TWaypointId iPrevCurrentWaypoint, Vector const& vPrevOrigin )
{
    //m_cCmd.viewangles = m_pController->GetLocalAngles(); // WTF?
    //m_cCmd.viewangles = m_pPlayerInfo->GetAbsAngles(); // WTF?

    // Don't perform navigator moves when under attack.
    //if ( m_bUnderAttack )
    //{
    //	if ( m_iIntelligence >= EBotSmart)
    //	{
    //		// Move left/stop-shoot/right/stop-shoot.
    //	}
    //}

    if  ( CheckMoveFailure() )
    {
        m_pNavigator.Stop();
        return;
    }

    if ( !m_bLastNeedMove && m_bNeedMove ) // Bot started to move just now, update stuck check time.
    {
        m_fStuckCheckTime = CBotrixPlugin::fTime + 1.0f;
        m_bLastNeedMove = m_bNeedMove;
    }

    // Waypoint just changed from previous and valid waypoint.
    bool bCurrentWaypointChanged = CWaypoint::IsValid(iPrevCurrentWaypoint) && (iPrevCurrentWaypoint != iCurrentWaypoint);
    if ( bCurrentWaypointChanged )
        CurrentWaypointJustChanged();

    bool bTouchedWaypoint = false; // Set to true if reached next waypoint in path or path searching just finished (then touched start waypoint).
    if ( m_bNeedMove && !m_bMoveFailure )
        bTouchedWaypoint = ( m_bUseNavigatorToMove ) ? NavigatorMove() : NormalMove();

    bool bMove = m_bNeedMove && !m_bMoveFailure && !m_bStuckBreakObject && !m_bStuckUsePhyscannon;
    if ( m_bUseNavigatorToMove && ( !m_pNavigator.SearchEnded() || m_bLockNavigatorMove ) )
        bMove = false; // Check if search is finished (m_bMoveFailure will be set if search fails).

    // Check if need to look left/right/back/forward.
    CheckSideLook(bMove, bTouchedWaypoint);

    // Need to aim somewhere? TODO: faster...
    if ( m_bNeedAim )
    {
        //QAngle angOld = m_cCmd.viewangles;

        // Calculate how much frames left to m_fEndAimTime.
        float fTimeLeft = m_fEndAimTime - CBotrixPlugin::fTime;
        int iFramesLeft = (int)((float)CBotrixPlugin::iFPS * fTimeLeft);

        if (iFramesLeft > 0)
        {
            // Make smoother angle change (break in several angle changes).
            // https://developer.valvesoftware.com/wiki/QAngle
            Vector vDestinationAim( m_vLook );
            vDestinationAim -= m_vHead; // Get abs vector.

            QAngle angNeeded;
            VectorAngles(vDestinationAim, angNeeded);

            CUtil::GetAngleDifference(m_cCmd.viewangles, angNeeded, angNeeded);
            angNeeded /= iFramesLeft;

            m_cCmd.viewangles += angNeeded;
        }
        else
        {
            Vector vAbs(m_vLook);
            vAbs -= m_vHead;
            VectorAngles(vAbs, m_cCmd.viewangles); // Convert vector to angles.
            m_bNeedAim = false;
        }

        CUtil::DeNormalizeAngle(m_cCmd.viewangles.x);
        CUtil::DeNormalizeAngle(m_cCmd.viewangles.y);

        if ( !m_bUnderAttack )
        {
            if ( m_cCmd.viewangles.x > 60.0f )
                m_cCmd.viewangles.x = 60.0f;
            else if ( m_cCmd.viewangles.x < -60.0f )
                m_cCmd.viewangles.x = -60.0f;
        }
        //CUtil::DrawLine( m_vHead, m_vLook, 0.1f, 0xFF, 0xFF, 0xFF );
    }

    // Check if need moving.
    if ( bMove && ( !m_bUnderAttack || !m_bCloseAttack ) )
    {
        float fDeltaTime = CBotrixPlugin::fTime - m_fPrevThinkTime;

        // Calculate distance from last frame.
        Vector vSpeed(m_vHead);
        vSpeed -= vPrevOrigin;
        vSpeed /= fDeltaTime; // v = (x - x0)/t

        // Cancel stuck check if bot needs to stop, or use, or perform action, or it is stucked already.
        if ( m_bUnderAttack || m_bNeedStop || m_bNeedUse || m_bStuck ||
             m_bStuckBreakObject || m_bStuckUsePhyscannon || m_bStuckTryingSide ||
            (CBotrixPlugin::fTime <= m_fEndActionTime) )
        {
            m_bNeedCheckStuck = false;
        }
        else if ( !m_bNeedCheckStuck ) // Bot starts to move, set up next stuck check time.
        {
            // Check if bot is stucked after one second.
            m_bNeedCheckStuck = true;
            m_vStuckCheck = m_vHead;
            m_fStuckCheckTime = CBotrixPlugin::fTime + 1.0f;
        }
        else if ( CBotrixPlugin::fTime >= m_fStuckCheckTime )
        {
            m_bNeedCheckStuck = false;

            // Distance that bot moves since m_fStuckCheckTime.
            if ( m_vStuckCheck.DistToSqr(m_vHead) < SQR(CUtil::fMinNonStuckSpeed) )
            {
                m_bNeedCheckStuck = true;
                m_fStuckCheckTime = CBotrixPlugin::fTime + 5.0f; // Try again in 5 secs.
                if ( !ResolveStuckMove() )
                    return;
            }
        }

        // Calculate forward and side velocities to reach destination.
        //Vector vAccelerate(vSpeed);
        //vAccelerate -= m_vLastVelocity;
        //vAccelerate /= fDeltaTime;   // a = (v - v0)/t

        //m_vLastVelocity = vSpeed;
        Vector vNeededVelocity(m_vDestination);
        float fSpeed;

        // Need to stop before next move.
        if ( m_bNeedStop && (vSpeed.x == 0) && (vSpeed.y == 0) && (vSpeed.z == 0) )
            m_bNeedStop = false;

        if ( m_bNeedStop )
        {
            vNeededVelocity = CWaypoints::Get(iCurrentWaypoint).vOrigin; // Move to current waypoint instead of next one.
            vNeededVelocity -= m_vHead; // Abs vector.
            vNeededVelocity -= vSpeed;
            fSpeed = vNeededVelocity.Length();
        }
        else
        {
            //m_bNeedSprint = true;
            fSpeed = (m_bNeedSprint) ? CUtil::fMaxSprintVelocity :
                     (m_bNeedWalk) ? CUtil::fMaxWalkVelocity : CUtil::fMaxRunVelocity;

            vNeededVelocity -= m_vHead; // Destination - head (absolute vector).

            if ( m_bStuckTryingSide ) // Currently stucked, trying to move left/right.
            {
                if ( CBotrixPlugin::fTime < m_fEndActionTime )
                {
                    // If bot was stucked then move left or right accordingly.
                    Vector vUp, vRight;
                    VectorVectors(vNeededVelocity, vRight, vUp);
                    if ( m_bStuckTryGoLeft )
                        vRight.Negate();
                    vNeededVelocity = vRight;
                    //BotMessage("Stucked, trying side until %.1f, now %.1f", m_fEndActionTime, CBotrixPlugin::fTime);
                }
                else
                    m_bStuckTryingSide = false;
            }

            vNeededVelocity.NormalizeInPlace();
            vNeededVelocity *= fSpeed;

            vNeededVelocity -= vSpeed;
        }

        QAngle angNeeded;
        VectorAngles(vNeededVelocity, angNeeded);

        CUtil::NormalizeAngle(angNeeded.y);
        CUtil::GetAngleDifference(m_cCmd.viewangles, angNeeded, angNeeded);

        //CUtil::DeNormalizeAngle(angNeeded.y);
        //float fYaw = (angNeeded.y * 3.14159265f) / 180.0f; // Degree to radians.
        //m_cCmd.forwardmove = fSpeed * cosf(fYaw);
        //m_cCmd.sidemove = -fSpeed * sinf(fYaw);

        //Vector vMove(m_cCmd.forwardmove, m_cCmd.sidemove, 0);
        //vMove += m_vHead;
        //CUtil::DrawLine( m_vHead, vMove, 0.1f, 0xFF, 0xFF, 0xFF );

        // Check forward/backward move.
        if ( (-45.0f <= angNeeded.y) && (angNeeded.y <= 45.0f) )
            m_cCmd.forwardmove = fSpeed;
        else if ( (angNeeded.y <= -135.0f) || (angNeeded.y >= 135.0f) )
            m_cCmd.forwardmove = -fSpeed;

        // Check left/right move.
        if ( (45.0f <= angNeeded.y) && (angNeeded.y <= 135.0f) )
            m_cCmd.sidemove = -fSpeed;
        else if ( (-135.0f <= angNeeded.y) && (angNeeded.y <= -45.0f) )
            m_cCmd.sidemove = fSpeed;

        //CUtil::Message(NULL,"Forward %f, side %f", m_cCmd.forwardmove, m_cCmd.sidemove);
    }
    else // No need to move.
    {
        m_bNeedCheckStuck = false;
        /*
        m_vLastVelocity = m_vLastOrigin;
        m_vLastVelocity -= m_vHead;
        m_vLastVelocity /= fDeltaTime;
        */
    }


    //---------------------------------------------------------------
    if ( !m_bDontAttack )
    {
        CWeaponWithAmmo& cWeapon = m_aWeapons[m_iWeapon];
        if ( m_bUnderAttack ) // Check if can shoot.
        {
            if ( m_pCurrentEnemy )
            {
                DebugAssert( m_pCurrentEnemy != this );
                if ( !m_bNeedAim )
                {
                    // Aim again.
                    EnemyAim();

                    if ( !m_bEnemyAimed ) // First time aimed at enemy.
                    {
                        m_bEnemyAimed = true;
                        m_bStayReloading = false; // At reload time, check if it is better to change weapon.
                    }

                }

                if ( cWeapon.CanUse() )
                {
                    if ( m_bEnemyAimed ) // Stay shooting after first time aimed at enemy.
                    {
                        if ( cWeapon.IsSniper()  ) // Check if need to zoom or out.
                        {
                            bool bNeedZoom = cWeapon.ShouldZoom(m_fDistanceSqrToEnemy);
                            if ( (  bNeedZoom && !cWeapon.IsUsingZoom() ) ||
                                 ( !bNeedZoom &&  cWeapon.IsUsingZoom() ) )
                                ToggleZoom();
                        }

                        // Prefer secondary attack.
                        if ( cWeapon.HasAmmoInClip(1) && cWeapon.IsDistanceSafe(m_fDistanceSqrToEnemy, 1) )
                            Shoot(1);
                        else if ( cWeapon.HasAmmoInClip(0) && cWeapon.IsDistanceSafe(m_fDistanceSqrToEnemy, 0) )
                            Shoot(0);
                        else
                            CheckWeapon(); // No more bullets, select another weapon.
                    }
                }
            }
            else
                m_bUnderAttack = m_bCloseAttack = m_bAttackDuck = false; // Bot is not under attack now.
        }
        else if ( cWeapon.IsSniper() && cWeapon.IsUsingZoom() && cWeapon.CanUse() )
            ToggleZoom();
        else if ( m_bNeedReload && !m_bStuckUsePhyscannon && !m_bStuckBreakObject )
            CheckWeapon();
    }

    //---------------------------------------------------------------
    // Aim finished and need start to use IN_USE button.
    if ( m_bNeedUse && !m_bUnderAttack && !m_bNeedAim )
    {
        FLAG_SET(IN_USE, m_cCmd.buttons);
        if ( CBotrixPlugin::fTime >= m_fEndActionTime )
        {
            // Ended using machine, check if need to use again.
            int iHealthArmor = m_bUsingHealthMachine ? m_pPlayerInfo->GetHealth() : m_pPlayerInfo->GetArmorValue();
            if ( iHealthArmor == m_iLastHealthArmor )
            {
                m_bNeedUse = false;
                m_bAlreadyUsed = true;
                ApplyPathFlags();
                DoPathAction();
            }
            else
            {
                //BotMessage("Incremented %d of %s, now %d.", iHealthArmor - m_iLastHealthArmor, m_bUsingHealthMachine ? "health" : "armor", iHealthArmor);
                m_iLastHealthArmor = iHealthArmor;
                m_fEndActionTime = CBotrixPlugin::fTime + m_fTimeIntervalCheckUsingMachines;
            }
        }
    }

    // If bot is on ladder forwardmove and sidemove are not used, but IN_FORWARD & IN_BACK used instead.
    if ( m_bLadderMove )
        FLAG_SET(IN_FORWARD, m_cCmd.buttons); // Should be looking at next waypoint.

    // Jump only once.
    if ( m_bNeedJump && (CBotrixPlugin::fTime >= m_fStartActionTime) )
    {
        FLAG_SET(IN_JUMP, m_cCmd.buttons);
        m_bNeedJump = false;
    }

    // Start holding duck for jump after m_fStartActionTime.
    if ( m_bNeedJumpDuck && (CBotrixPlugin::fTime >= m_fStartActionTime + 0.1f) )
    {
        // Stop holding duck for jump after m_fEndActionTime.
        if ( CBotrixPlugin::fTime >= m_fEndActionTime )
            m_bNeedJumpDuck = false;
        else
            FLAG_SET(IN_DUCK, m_cCmd.buttons);
    }

    // Start holding attack after m_fStartActionTime.
    if ( m_bNeedAttack )
    {
        // Set manual weapon.
        if ( (m_iWeapon != m_iManualWeapon) && CWeapon::IsValid(m_iManualWeapon) )
        {
            if ( m_aWeapons[m_iWeapon].CanChange() )
                SetActiveWeapon(m_iManualWeapon);
        }
        else
        {
            // Check if some object is near.
            const CEntity* pObject = NULL;
            if ( m_aNearestItems[EEntityTypeObject].size() )
                pObject = &CItems::GetItems(EEntityTypeObject)[ m_aNearestItems[EEntityTypeObject][0] ];

            if ( pObject && pObject->IsBreakable() && !pObject->IsExplosive() )
            {
                m_bStuckBreakObject = true; // Try to break the object, instead of just hitting blindly.
                m_bNeedAttack = false;
            }
            else if ( CBotrixPlugin::fTime < m_fEndActionTime ) // Stop attacking after m_fEndActionTime.
                Shoot(0);
            else
            {
                m_bNeedAttack = false;
                m_bNeedSetWeapon = true;
            }
        }
    }

    // Aim at object (at m_fStartActionTime), press ATTACK2 (physcannon) for a second, aim back, press ATTACK1 once.
    if ( m_bStuckUsePhyscannon )
    {
        DebugAssert( m_bTest || !m_bUnderAttack );

        const CEntity* pObject = NULL;
        if ( m_aNearestItems[EEntityTypeObject].size() )
            pObject = &CItems::GetItems(EEntityTypeObject)[ m_aNearestItems[EEntityTypeObject][0] ];

        // Still not finished throwing object.
        if ( CBotrixPlugin::fTime < m_fEndActionTime )
        {
            if ( (m_iWeapon != m_iPhyscannon) ) // Still need to switch weapon to physcannon.
            {
                if ( m_aWeapons[m_iWeapon].CanChange() )
                {
                    SetActiveWeapon(m_iPhyscannon);
                    float fPhyscannonUseTime = m_aWeapons[m_iWeapon].GetEndTime();
                    if ( fPhyscannonUseTime > m_fEndAimTime )
                    {
                        // Time we can use physcannon is after aim time, so re calculate action times.
                        m_fStartActionTime = m_fEndAimTime = fPhyscannonUseTime;
                        m_fEndActionTime = m_fStartActionTime + 2.0f;
                    }
                }
            }
            else // Current weapon is physcannon.
            {
                if ( (CBotrixPlugin::fTime >= m_fEndAimTime) ) // Aimed either at object, can use physcannon now.
                {
                    if ( CBotrixPlugin::fTime < m_fStartActionTime + 0.5f )
                        Shoot(1); // Attract object for half second.
                    else
                    {
                        // Bot should be holding object.
                        if ( pObject && (pObject->CurrentPosition() == m_vDisturbingObjectPosition) )
                        {
                            // Object position didn't change, so bot cant pick it up.
                            FLAG_SET(FObjectHeavy, ((CEntity*)pObject)->iFlags);
                            m_bStuckUsePhyscannon = m_bLockAim = false;
                            m_bNeedSetWeapon = true;
                        }
                        else
                        {
                            // Look back while holding object.
                            //m_bStuckPhyscannonHoldingObject = true;
                            m_bNeedAim = true;
                            m_vLook = m_vHead;
                            m_vLook -= CWaypoints::Get(iNextWaypoint).vOrigin;
                            m_vLook += m_vHead; // back = (head - waypoint) + head.
                            m_fEndActionTime = m_fEndAimTime = CBotrixPlugin::fTime + GetEndLookTime();
                        }
                    }
                }
            }
        }
        else // Throw object and restore previous weapon.
        {
            Shoot(0);
            m_bStuckUsePhyscannon = m_bLockAim = false;
            m_bNeedSetWeapon = true;
        }
    }

    // Start holding attack after m_fStartActionTime until break object or object moves far away.
    else if ( m_bStuckBreakObject )
    {
        DebugAssert( m_bTest || !m_bUnderAttack );

        if ( (m_iWeapon != m_iManualWeapon) && m_aWeapons[m_iWeapon].CanChange() )
            SetActiveWeapon(m_iManualWeapon);

        if ( CBotrixPlugin::fTime >= m_fStartActionTime )
        {
            const CEntity* pObject = NULL;
            if ( m_aNearestItems[EEntityTypeObject].size() )
                pObject = &CItems::GetItems(EEntityTypeObject)[ m_aNearestItems[EEntityTypeObject][0] ];

            if ( pObject && pObject->IsBreakable() && !pObject->IsExplosive() ) // Object still there.
            {
                Shoot(0);

                if ( (CBotrixPlugin::fTime >= m_fEndAimTime + 1.0f) ) // Maybe object moved.
                {
                    m_bNeedAim = true;
                    m_vLook = pObject->CurrentPosition();
                    int r = 4 - (rand() & 0x7); // Maybe object has holes: add random from -3 to 4.
                    m_vLook.x += r;
                    m_vLook.y += -r;
                    m_vLook.z += r;
                    m_fEndAimTime = CBotrixPlugin::fTime + GetEndLookTime();
                }
            }
            else // Object broken, restore previous weapon.
            {
                m_bStuckBreakObject = m_bLockAim = false;
                m_bNeedSetWeapon = true;
            }
        }
    }

    // Duck until bot reaches next waypoint.
    if ( m_bNeedDuck || m_bAttackDuck )
        FLAG_SET(IN_DUCK, m_cCmd.buttons);

    // Sprint until bot reaches next waypoint. Not working.
    if ( m_bNeedSprint )
        FLAG_SET(IN_SPEED, m_cCmd.buttons);

    // Toggle flashlight.
    if ( m_bNeedFlashlight ^ m_bUsingFlashlight )
    {
        m_bUsingFlashlight = !m_bUsingFlashlight;
        m_cCmd.impulse = 100;
    }

    if ( m_bNeedSetWeapon )
        CheckWeapon();
}
