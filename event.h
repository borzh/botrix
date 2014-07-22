#ifndef __BOTRIX_EVENT_H__
#define __BOTRIX_EVENT_H__


#include <good/memory.h>

#include "types.h"


//****************************************************************************************************************
/// Interface for getting event's values.
//****************************************************************************************************************
class IEventInterface
{

public:
    /// Virtual estructor.
    virtual ~IEventInterface() {}

    /// Get event name.
    virtual const char *GetName() = 0;

    /// Get event bool value for key szKey. Return bDefaultValue if szKey not found.
    virtual bool GetBool( const char *szKey, bool bDefaultValue = false ) = 0;

    /// Get event int value for key szKey. Return iDefaultValue if szKey not found.
    virtual int GetInt( const char *szKey, int iDefaultValue = 0 ) = 0;

    /// Get event float value for key szKey. Return fDefaultValue if szKey not found.
    virtual float GetFloat( const char *szKey, float fDefaultValue = 0 ) = 0;

    /// Get event string value for key szKey. Return szDefaultValue if szKey not found.
    virtual const char *GetString( const char *szKey, const char *szDefaultValue = NULL ) = 0;

};


//****************************************************************************************************************
/// Game event (like player hurt, game start, etc).
//****************************************************************************************************************
class CEvent
{

public:
    /// Constructor.
    CEvent( const char* szType ): m_sType(szType) {}

    /// Destructor.
    virtual ~CEvent() {}

    /// Return name of this event.
    const good::string& GetName() const { return m_sType; }

    /// Execute this event.
    virtual void Execute( IEventInterface* pEvent ) = 0;

    /// Get event interface from event pEvent with type iType. You will need to delete it after use.
    static IEventInterface* GetEventInterface( void* pEvent, TEventType iType );

protected:
    good::string m_sType;

};

typedef good::shared_ptr<CEvent> CEventPtr; ///< Typedef for unique_ptr of CEvent.


//================================================================================================================
/// This event is fired when player finished connecting to server.
//================================================================================================================
class CPlayerActivateEvent: public CEvent
{
public:
    CPlayerActivateEvent(): CEvent("player_activate") {}

    void Execute( IEventInterface* pEvent );
};


//================================================================================================================
/// This event is fired every time player changes team.
//================================================================================================================
class CPlayerTeamEvent: public CEvent
{
public:
    CPlayerTeamEvent(): CEvent("player_team") {}

    void Execute( IEventInterface* pEvent );
};


//================================================================================================================
/// This event is fired every time player spawns on server.
//================================================================================================================
class CPlayerSpawnEvent: public CEvent
{
public:
    CPlayerSpawnEvent(): CEvent("player_spawn") {}

    void Execute( IEventInterface* pEvent );
};


//================================================================================================================
/// This event is fired when player chats.
//================================================================================================================
class CPlayerChatEvent: public CEvent
{
public:
    CPlayerChatEvent(): CEvent("player_say") {}

    void Execute( IEventInterface* pEvent );
};


//================================================================================================================
/// This event is fired when player is hurted by another player.
//================================================================================================================
class CPlayerHurtEvent: public CEvent
{
public:
    CPlayerHurtEvent(): CEvent("player_hurt") {}

    void Execute( IEventInterface* pEvent );
};


//================================================================================================================
/// This event is fired when player is dead.
//================================================================================================================
class CPlayerDeathEvent: public CEvent
{
public:
    CPlayerDeathEvent(): CEvent("player_death") {}

    void Execute( IEventInterface* pEvent );
};


#endif // __BOTRIX_EVENT_H__
