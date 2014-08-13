#include "bot.h"
#include "clients.h"
#include "event.h"
#include "server_plugin.h"
#include "source_engine.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//----------------------------------------------------------------------------------------------------------------
// Class for events of type KeyValues.
//----------------------------------------------------------------------------------------------------------------
class CGameEventInterface1 : public IEventInterface
{
public:
    CGameEventInterface1( KeyValues *pEvent ) { m_pEvent = pEvent; }

    const char *GetName()
    {
        return m_pEvent->GetName();
    }

    bool GetBool( const char *keyName, bool defaultValue = false )
    {
        return ( m_pEvent->GetInt(keyName, defaultValue) != 0 );
    }

    int GetInt( const char *keyName, int defaultValue = 0 )
    {
        return m_pEvent->GetInt(keyName, defaultValue);
    }

    float GetFloat( const char *keyName, float defaultValue = 0 )
    {
        return m_pEvent->GetFloat(keyName, defaultValue);
    }

    const char *GetString( const char *keyName, const char *defaultValue = NULL )
    {
        return m_pEvent->GetString(keyName, defaultValue);
    }

protected:
    KeyValues *m_pEvent;

};


//----------------------------------------------------------------------------------------------------------------
// Class for events of type IGameEvent.
//----------------------------------------------------------------------------------------------------------------
class CGameEventInterface2 : public IEventInterface
{
public:
    CGameEventInterface2( IGameEvent *pEvent ) { m_pEvent = pEvent; }

    const char *GetName() { return m_pEvent->GetName(); }

    bool GetBool( const char *keyName, bool defaultValue = false )
    {
        return m_pEvent->GetBool(keyName, defaultValue);
    }

    int GetInt( const char *keyName, int defaultValue = 0 )
    {
        return m_pEvent->GetInt(keyName, defaultValue);
    }

    float GetFloat( const char *keyName, float defaultValue = 0 )
    {
        return m_pEvent->GetFloat(keyName, defaultValue);
    }

    const char *GetString( const char *keyName, const char *defaultValue = 0 )
    {
        return m_pEvent->GetString(keyName, defaultValue);
    }

protected:
    IGameEvent *m_pEvent;
};


//----------------------------------------------------------------------------------------------------------------
IEventInterface* CEvent::GetEventInterface( void* pEvent, TEventType iType )
{
    IEventInterface *result = NULL;

    if (iType == EEventTypeKeyValues)
        result = new CGameEventInterface1( (KeyValues*)pEvent );
    else if (iType == EEventTypeIGameEvent)
        result = new CGameEventInterface2( (IGameEvent*)pEvent );

    return result;
}


//----------------------------------------------------------------------------------------------------------------
void CPlayerActivateEvent::Execute( IEventInterface* pEvent )
{
    edict_t* pActivator = CUtil::GetEntityByUserId( pEvent->GetInt("userid") );

    int iIdx = CPlayers::Get(pActivator);
    BASSERT( iIdx >= 0, return );

    CPlayer* pPlayer = CPlayers::Get(iIdx);
    if (pPlayer)
        pPlayer->Activated();
}


//----------------------------------------------------------------------------------------------------------------
void CPlayerTeamEvent::Execute( IEventInterface* pEvent )
{
    edict_t* pActivator = CUtil::GetEntityByUserId( pEvent->GetInt("userid") );
    TTeam iTeam = pEvent->GetInt("team");

    int iIdx = CPlayers::Get(pActivator);
    BASSERT( iIdx >= 0, return );

    CPlayer* pPlayer = CPlayers::Get(iIdx);
    if ( pPlayer && pPlayer->IsBot() )
        ((CBot*)pPlayer)->ChangeTeam( iTeam );
}


//----------------------------------------------------------------------------------------------------------------
void CPlayerSpawnEvent::Execute( IEventInterface* pEvent )
{
    edict_t* pActivator = CUtil::GetEntityByUserId( pEvent->GetInt("userid") );

    int iIdx = CPlayers::Get(pActivator);
    BASSERT( iIdx >= 0, return );

    CPlayer* pPlayer = CPlayers::Get(iIdx);
    if (pPlayer)
        pPlayer->Respawned();
}


//----------------------------------------------------------------------------------------------------------------
void CPlayerChatEvent::Execute( IEventInterface* pEvent )
{
    edict_t* pActivator = CUtil::GetEntityByUserId( pEvent->GetInt("userid") );
    const char* szText = pEvent->GetString("text");
    bool bTeamOnly = pEvent->GetBool("teamonly");

    CPlayers::DeliverChat(pActivator, bTeamOnly, szText);
}


//----------------------------------------------------------------------------------------------------------------
void CPlayerHurtEvent::Execute( IEventInterface* pEvent )
{
    edict_t* pActivator = CUtil::GetEntityByUserId( pEvent->GetInt("userid") );
    int iActivator = CPlayers::Get(pActivator);
    BASSERT( iActivator >= 0, return );

    edict_t* pAttacker = CUtil::GetEntityByUserId( pEvent->GetInt("attacker") );
    int iAttacker = pAttacker ? CPlayers::Get(pAttacker) : iActivator; // May hurt himself.

    CPlayer *pPlayer = CPlayers::Get(iActivator);
    CPlayer *pPlayerAttacker = iAttacker >= 0 ? CPlayers::Get(iAttacker) : NULL;
    if ( pPlayer && pPlayer->IsBot() )
        ((CBot*)pPlayer)->HurtBy( iAttacker, pPlayerAttacker, pEvent->GetInt("health") );
}


//----------------------------------------------------------------------------------------------------------------
void CPlayerDeathEvent::Execute( IEventInterface* pEvent )
{
    edict_t* pActivator = CUtil::GetEntityByUserId( pEvent->GetInt("userid") );
    edict_t* pAttacker = CUtil::GetEntityByUserId( pEvent->GetInt("attacker") );

    int iActivator = CPlayers::Get(pActivator);
    BASSERT( iActivator >= 0, return );

    CPlayer* pPlayerActivator = CPlayers::Get(iActivator);
    if ( pPlayerActivator )
        pPlayerActivator->Dead();

    if ( pAttacker )
    {
        int iAttacker = CPlayers::Get(pAttacker);
        BASSERT( iAttacker >= 0, return );

        CPlayer* pPlayerAttacker = CPlayers::Get(iAttacker);
        if ( pPlayerAttacker && pPlayerAttacker->IsBot() )
            ((CBot*)pPlayerAttacker)->KilledEnemy(iActivator, pPlayerActivator);
    }
}
