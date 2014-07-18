#include <good/string_buffer.h>
#include <good/string_utils.h>

#include "chat.h"
#include "clients.h"
#include "mod.h"
#include "players.h"
#include "server_plugin.h"
#include "type2string.h"
#include "waypoint.h"
#include "mods/borzh/bot_borzh.h"
#include "mods/hl2dm/bot_hl2dm.h"

extern char* szMainBuffer;
extern int iMainBufferSize;


//----------------------------------------------------------------------------------------------------------------
std::vector<CPlayerPtr> CPlayers::m_aPlayers(16);
CClient* CPlayers::m_pListenServerClient = NULL;

int CPlayers::m_iClientsCount = 0;
int CPlayers::m_iBotsCount = 0;

#if defined(DEBUG) || defined(_DEBUG)
    bool CPlayers::m_bClientDebuggingEvents = true;
#else
    bool CPlayers::m_bClientDebuggingEvents = false;
#endif

static bool bAddingBot = false;



//----------------------------------------------------------------------------------------------------------------
void CPlayer::Activated()
{
    m_pPlayerInfo = CBotrixPlugin::pPlayerInfoManager->GetPlayerInfo( m_pEdict );
    m_sName.assign(GetName(), good::string::npos, true);
    good::lower_case(m_sName);
    DebugAssert( m_pPlayerInfo );
}

//----------------------------------------------------------------------------------------------------------------
void CPlayer::Respawned()
{
#if DRAW_PLAYER_HULL
    m_fNextDrawHullTime = 0.0f;
#endif
    CBotrixPlugin::pServerGameClients->ClientEarPosition(m_pEdict, &m_vHead);
    iCurrentWaypoint = CWaypoints::GetNearestWaypoint( m_vHead );
    m_bAlive = true;
}

//----------------------------------------------------------------------------------------------------------------
void CPlayer::PreThink()
{
    if ( !m_bAlive && m_bBot ) // Don't update current waypoint for dead bots.
        return;

#if DRAW_PLAYER_HULL
    static const float fDrawTime = 0.1f;
    if ( CBotrixPlugin::fTime >= m_fNextDrawHullTime )
    {
        m_fNextDrawHullTime = CBotrixPlugin::fTime + fDrawTime;
        Vector vPos = m_pPlayerInfo->GetAbsOrigin();
        Vector vMins = m_pPlayerInfo->GetPlayerMins();
        Vector vMaxs = m_pPlayerInfo->GetPlayerMaxs();
        CUtil::DrawBox(vPos, vMins, vMaxs, fDrawTime, 0xFF, 0xFF, 0xFF);
    }
#endif

    m_vPrevHead = m_vHead;
    CBotrixPlugin::pServerGameClients->ClientEarPosition(m_pEdict, &m_vHead); // borzh

    // If waypoint is not valid or we are too far, recalculate current waypoint.
    if ( !CWaypoint::IsValid(iCurrentWaypoint) ||
        m_vHead.DistToSqr( CWaypoints::Get(iCurrentWaypoint).vOrigin ) > SQR(CWaypoint::MAX_RANGE))
    {
        iPrevWaypoint = iCurrentWaypoint = CWaypoints::GetNearestWaypoint( m_vHead );
        return;
    }

    // Check if we are moved too far away from current waypoint as to be near to one of its neighbour.
    TWaypointId iNewWaypoint = -1;

    CWaypoints::WaypointNode& w = CWaypoints::GetNode(iCurrentWaypoint);
    float fDist = m_vHead.DistToSqr(w.vertex.vOrigin);

    if ( CWaypoint::IsValid(iNextWaypoint) ) // There is next waypoint in path, check if we are closer to it.
    {
        CWaypoint& n = CWaypoints::Get(iNextWaypoint);
        float fNewDist = m_vHead.DistToSqr(n.vOrigin);
        if ( fNewDist < fDist )
            iNewWaypoint = iNextWaypoint;
    }
    else
    {
        for (int i = 0; i < (int)w.neighbours.size(); ++i)
        {
            TWaypointId id = w.neighbours[i].target;
            CWaypoint& n = CWaypoints::Get(id);
            float fNewDist = m_vHead.DistToSqr(n.vOrigin);
            if ( fNewDist < fDist )
            {
                iNewWaypoint = id;
                fDist = fNewDist;
            }
        }
    }

    if ( iNewWaypoint >= 0 )
    {
        iPrevWaypoint = iCurrentWaypoint;
        iCurrentWaypoint = iNewWaypoint;
    }
}








//********************************************************************************************************************
void CPlayers::Init( int iMaxPlayers )
{
    m_aPlayers.clear();
    m_aPlayers.resize( iMaxPlayers );
    // TODO: Save bots config when changing map.
    //for (int i = 0; i < CPlayers::Size(); ++i)
    //{
    //	if ( m_aPlayers[i].get() )
    //		m_aPlayers[i]->Respawned();
    //}
}

//----------------------------------------------------------------------------------------------------------------
void CPlayers::Clear()
{
    for (int i = 0; i < CPlayers::Size(); ++i)
        m_aPlayers[i].reset();
    m_pListenServerClient = 0;

    m_iClientsCount = m_iBotsCount = 0;
    m_bClientDebuggingEvents = false;
}


//----------------------------------------------------------------------------------------------------------------
CPlayer* CPlayers::AddBot( TBotIntelligence iIntelligence )
{
    if ( GetPlayersCount() == Size() )
        return NULL;

    TModId iModId = CMod::GetModId();
    if ( iModId == EModId_Invalid )
    {
        CUtil::Message(NULL, "Error, can't create bot: unknown mod.");
        return NULL;
    }

    const good::string& sName = CMod::GetRandomBotName();
    const char* szName;

    if ( CMod::bIntelligenceInBotName )
    {
        good::string_buffer sbNameWithIntelligence(szMainBuffer, iMainBufferSize, false); // Don't deallocate.
        sbNameWithIntelligence = sName;
        sbNameWithIntelligence.append(' ');
        sbNameWithIntelligence.append('(');
        sbNameWithIntelligence.append(CTypeToString::IntelligenceToString(iIntelligence));
        sbNameWithIntelligence.append(')');
        szName = sbNameWithIntelligence.c_str();
    }
    else
        szName = sName.c_str();

    bAddingBot = true;
    edict_t* pEdict = CBotrixPlugin::pBotManager->CreateBot( szName );
    bAddingBot = false;

    if ( !pEdict )
        return NULL;

    TPlayerIndex iIdx = CBotrixPlugin::pEngineServer->IndexOfEdict(pEdict)-1;
    DebugAssert(iIdx >= 0);

    CPlayer* pPlayer = NULL;
    switch ( iModId )
    {
        case EModId_HL2DM:
            pPlayer = new CBot_HL2DM(pEdict, iIdx, iIntelligence);
            break;
        case EModId_Borzh:
#ifdef BOTRIX_BORZH_MOD
            pPlayer = new CBotBorzh(pEdict, iIdx, iIntelligence);
#else
            DebugAssert(false);
#endif
            break;
        default:
            DebugAssert(false);
            return NULL;
    }

    pPlayer->Activated(); // Event active is not created for bots.

    m_aPlayers[iIdx] = CPlayerPtr(pPlayer);
    m_iBotsCount++;

#ifdef BOTRIX_CHAT
    if ( CChat::iPlayerVar != EChatVariableInvalid )
    {
        good::string sName(pPlayer->GetName(), true, true); // Copy and deallocate.
        sName.lower_case();
        CChat::SetVariableValue( CChat::iPlayerVar, iIdx, sName );
    }
#endif

    return pPlayer;
}

//----------------------------------------------------------------------------------------------------------------
void CPlayers::KickBot( CPlayer* pPlayer )
{
    DebugAssert( pPlayer && pPlayer->IsBot() );
    char szCommand[16];
    sprintf( szCommand,"kickid %d\n", pPlayer->GetPlayerInfo()->GetUserID() ); // \n ???
    CBotrixPlugin::pEngineServer->ServerCommand(szCommand);
}

//----------------------------------------------------------------------------------------------------------------
bool CPlayers::KickRandomBot()
{
    if ( m_iBotsCount == 0 )
    {
        CUtil::Message(NULL,"No bots to kick");
        return false;
    }

    int index = rand() % m_aPlayers.size();

    // Search for used bot from index to left.
    CBot* toKick = NULL;
    int i;

    for (i = index; i >= 0; --i)
    {
        CPlayer* pPlayer = m_aPlayers[i].get();
        if ( pPlayer && pPlayer->IsBot() )
        {
            toKick = (CBot*)pPlayer;
            break;
        }
    }

    // Search for used bot from index to right.
    if (toKick == NULL)
        for (i = index+1; i < (int)m_aPlayers.size(); ++i)
        {
            CPlayer* pPlayer = m_aPlayers[i].get();
            if ( pPlayer && pPlayer->IsBot() )
            {
                toKick = (CBot*)pPlayer;
                break;
            }
        }

    KickBot(toKick);
    return true;
}

//----------------------------------------------------------------------------------------------------------------
bool CPlayers::KickRandomBotOnTeam( int /*iTeam*/ )
{
    // TODO:
    if ( m_iBotsCount == 0 )
    {
        CUtil::Message(NULL,"No bots to kick");
        return false;
    }

    return true;
}

//----------------------------------------------------------------------------------------------------------------
void CPlayers::PlayerConnected( edict_t* pEdict )
{
    TPlayerIndex iIdx = CBotrixPlugin::pEngineServer->IndexOfEdict(pEdict)-1;
    DebugAssert( iIdx >= 0 );

    if ( !bAddingBot )
    {
        DebugAssert( m_aPlayers[iIdx].get() == NULL );

        CPlayer* pPlayer = new CClient(pEdict, iIdx);
        pPlayer->Activated();

        if ( !CBotrixPlugin::pEngineServer->IsDedicatedServer() && (m_pListenServerClient == NULL) )
        {
            // Give listenserver client all access to bot commands.
            m_pListenServerClient = (CClient*)pPlayer;
            ((CClient*)pPlayer)->iCommandAccessFlags = FCommandAccessAll;
        }

        m_aPlayers[iIdx] = CPlayerPtr(pPlayer);
        m_iClientsCount++;

#ifdef BOTRIX_CHAT
        if ( CChat::iPlayerVar != EChatVariableInvalid )
        {
            good::string sName(pPlayer->GetName(), true, true); // Copy and deallocate.
            sName.lower_case();
            CChat::SetVariableValue( CChat::iPlayerVar, iIdx, sName );
        }
#endif
    }
}


//----------------------------------------------------------------------------------------------------------------
void CPlayers::PlayerDisconnected( edict_t* pEdict )
{
    int iIdx = CBotrixPlugin::pEngineServer->IndexOfEdict(pEdict)-1;
    DebugAssert(iIdx >= 0);

    if ( m_aPlayers[iIdx].get() == NULL )
        return; // Happens when starting new map and pressing cancel button.

    CPlayer* pPlayer = m_aPlayers[iIdx].get();
    DebugAssert( pPlayer );

#ifdef BOTRIX_CHAT
    if ( CChat::iPlayerVar != EChatVariableInvalid )
        CChat::SetVariableValue( CChat::iPlayerVar, iIdx, "" );
#endif

    // Notify bots that player is disconnected.
    for ( int i=0; i < Size(); ++i)
    {
        CPlayer* pBot = m_aPlayers[i].get();
        if ( pBot && pBot->IsBot() && pBot->IsAlive() )
            ((CBot*)pBot)->PlayerDisconnect(iIdx, pPlayer);
    }

    bool bIsBot = pPlayer->IsBot();
    m_aPlayers[iIdx].reset();

    if ( bIsBot )
        m_iBotsCount--;
    else
    {
        m_iClientsCount--;
        if ( !CBotrixPlugin::pEngineServer->IsDedicatedServer() && (m_pListenServerClient == pPlayer) )
            m_pListenServerClient = NULL;

        CheckForDebugging();
    }

    DebugAssert( (m_iBotsCount >= 0) && (m_iClientsCount >= 0) );
}


//----------------------------------------------------------------------------------------------------------------
void CPlayers::PreThink()
{
    for (std::vector<CPlayerPtr>::iterator it = m_aPlayers.begin(); it != m_aPlayers.end(); ++it)
        if ( it->get() )
            it->get()->PreThink();
}


//----------------------------------------------------------------------------------------------------------------
void CPlayers::DebugEvent( const char *szFormat, ... )
{
    static char buffer[128];
    va_list args;
    va_start(args, szFormat);
    vsprintf(buffer, szFormat, args);
    va_end(args);

    for ( std::vector<CPlayerPtr>::const_iterator it = m_aPlayers.begin(); it != m_aPlayers.end(); ++it )
    {
        const CPlayer* pPlayer = it->get();
        if ( pPlayer  &&  !pPlayer->IsBot()  &&  ((CClient*)pPlayer)->bDebuggingEvents )
            CUtil::Message(pPlayer->GetEdict(), buffer);
    }
}


//----------------------------------------------------------------------------------------------------------------
void CPlayers::CheckForDebugging()
{
    m_bClientDebuggingEvents = false;
    for (std::vector<CPlayerPtr>::iterator it = m_aPlayers.begin(); it != m_aPlayers.end(); ++it)
    {
        const CPlayer* pPlayer = it->get();
        if ( pPlayer  &&  !pPlayer->IsBot()  &&  ((CClient*)pPlayer)->bDebuggingEvents )
        {
            m_bClientDebuggingEvents = true;
            break;
        }
    }
}

//----------------------------------------------------------------------------------------------------------------
#ifdef BOTRIX_CHAT
void CPlayers::DeliverChat( edict_t* pFrom, bool bTeamOnly, const char* szText )
{
    int iTeam = 0;

    int iIdx = CPlayers::Get(pFrom);
    DebugAssert( iIdx >= 0 );
    CPlayer* pSpeaker = CPlayers::Get(iIdx);

    if ( bTeamOnly )
        iTeam = pSpeaker->GetTeam();

    // Get command from text.
    if ( GetBotsCount() > 0 )
    {
        static CBotChat cChat;
        cChat.iBotChat = EBotChatUnknown;
        cChat.iDirectedTo = EPlayerIndexInvalid;
        cChat.iSpeaker = iIdx;

        float fPercentage = CChat::ChatFromText( szText, cChat );
        bool bIsRequest = (fPercentage >= 6.0f);

/*
        if ( cChat.iDirectedTo == -1 )
            cChat.iDirectedTo = pSpeaker->iChatMate;
        else
            pSpeaker->iChatMate = cChat.iDirectedTo; // TODO: SetChatMate();
*/
        int iFrom = 0, iTo = CPlayers::Size();
        if ( bIsRequest && (cChat.iDirectedTo != -1) )
        {
            DebugAssert( cChat.iDirectedTo != iIdx );
            iFrom = cChat.iDirectedTo;
            iTo = iFrom + 1;
        }
        //else
        //	DebugMessage("Can't detect player, chat is directed to.");


        // Deliver chat for all bots.
        for ( TPlayerIndex iPlayer = iFrom; iPlayer < iTo; ++iPlayer )
        {
            CPlayer* pReceiver = CPlayers::Get(iPlayer);
            if ( pReceiver && pReceiver->IsBot() && (iIdx != iPlayer) )
            {
                CBot* pBot = (CBot*)pReceiver;
                if ( !bTeamOnly || (pBot->GetTeam() == iTeam) ) // Should be on same iTeam if chat is for iTeam only.
                {
                    if ( bIsRequest )
                        pBot->ReceiveChatRequest(cChat);
                    else
                        pBot->ReceiveChat( iIdx, pSpeaker, bTeamOnly, szText );
                }
            }
        }
    }
}
#else
void CPlayers::DeliverChat( edict_t*, bool, const char* ) {}
#endif // BOTRIX_CHAT
