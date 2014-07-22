#ifndef __BOTRIX_SERVER_PLUGIN_H__
#define __BOTRIX_SERVER_PLUGIN_H__


#include <good/defines.h>

#include "item.h"

#include "public/engine/iserverplugin.h"
#include "public/igameevents.h"


#define PLUGIN_VERSION "v0.0.1"

class IVEngineServer;
class IFileSystem;
class IGameEventManager;
class IGameEventManager2;
class IPlayerInfoManager;
class IVEngineServer;
class IServerPluginHelpers;
class IServerGameClients;
class IEngineTrace;
class IEffects;
class IBotManager;
class ICvar;


//****************************************************************************************************************
/// Plugin class.
//****************************************************************************************************************
class CBotrixPlugin : public IServerPluginCallbacks, public IGameEventListener, public IGameEventListener2
{

public: // Static members.
    static CBotrixPlugin* instance;                    ///< Plugin singleton.

    static IVEngineServer* pEngineServer;              ///< Interface to util engine functions.
    static IGameEventManager* pGameEventManager;       ///< Game events interface.
    static IGameEventManager2* pGameEventManager2;
    static IPlayerInfoManager* pPlayerInfoManager;     ///< Interface to interact with players.
    static IServerPluginHelpers* pServerPluginHelpers; ///< To create menues on client side.
    static IServerGameClients* pServerGameClients;
    static IEngineTrace* pEngineTrace;
    static IBotManager* pBotManager;
    static ICvar* pCvar;


public: // Methods.
    //------------------------------------------------------------------------------------------------------------
    /// Constructor.
    //------------------------------------------------------------------------------------------------------------
    CBotrixPlugin();

    /// Destructor.
    ~CBotrixPlugin();

    //------------------------------------------------------------------------------------------------------------
    // IServerPluginCallbacks implementation.
    //------------------------------------------------------------------------------------------------------------
    /// Initialize the plugin to run
    /// Return false if there is an error during startup.
    virtual bool Load(	CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory  );

    /// Called when the plugin should be shutdown
    virtual void Unload( void );

    /// called when a plugins execution is stopped but the plugin is not unloaded
    virtual void Pause( void );

    /// called when a plugin should start executing again (sometime after a Pause() call)
    virtual void UnPause( void );

    /// Returns string describing current plugin.  e.g., Admin-Mod.
    virtual const char     *GetPluginDescription( void );

    /// Called any time a new level is started (after GameInit() also on level transitions within a game)
    virtual void LevelInit( char const *pMapName );

    /// The server is about to activate
    virtual void ServerActivate( edict_t *pEdictList, int edictCount, int clientMax );

    /// The server should run physics/think on all edicts
    virtual void GameFrame( bool simulating );

    /// Called when a level is shutdown (including changing levels)
    virtual void LevelShutdown( void );

    /// Client is going active
    virtual void ClientActive( edict_t *pEntity );

    /// Client is disconnecting from server
    virtual void ClientDisconnect( edict_t *pEntity );

    /// Client is connected and should be put in the game
    virtual void ClientPutInServer( edict_t *pEntity, char const *playername );

    /// Sets the client index for the client who typed the command into their console
    virtual void SetCommandClient( int index );

    /// A player changed one/several replicated cvars (name etc)
    virtual void ClientSettingsChanged( edict_t *pEdict );

    /// Client is connecting to server ( set retVal to false to reject the connection )
    ///	You can specify a rejection message by writing it into reject
    virtual PLUGIN_RESULT ClientConnect( bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen );

    /// A user has had their network id setup and validated
    virtual PLUGIN_RESULT NetworkIDValidated( const char *pszUserName, const char *pszNetworkID );

#ifdef SOURCE_ENGINE_2006
    // The client has typed a command at the console
    virtual PLUGIN_RESULT ClientCommand( edict_t *pEntity );
#else
    /// The client has typed a command at the console
    virtual PLUGIN_RESULT ClientCommand( edict_t *pEntity, const CCommand &args );

    /// This is called when a query from IServerPluginHelpers::StartQueryCvarValue is finished.
    /// iCookie is the value returned by IServerPluginHelpers::StartQueryCvarValue.
    /// Added with version 2 of the interface.
    virtual void OnQueryCvarValueFinished( QueryCvarCookie_t, edict_t*, EQueryCvarValueStatus, const char*, const char* ) {}

    /// added with version 3 of the interface.
    virtual void OnEdictAllocated( edict_t *edict ) { if ( bMapRunning ) CItems::Allocated(edict); }
    virtual void OnEdictFreed( const edict_t *edict  ) { if ( bMapRunning ) CItems::Freed(edict); }
#endif

    //------------------------------------------------------------------------------------------------------------
    // IGameEventListener implementation.
    //------------------------------------------------------------------------------------------------------------
    // FireEvent is called by EventManager if event just occured
    // KeyValue memory will be freed by manager if not needed anymore
    virtual void FireGameEvent( KeyValues * event);

    //------------------------------------------------------------------------------------------------------------
    // IGameEventListener2 implementation.
    //------------------------------------------------------------------------------------------------------------
    // FireEvent is called by EventManager if event just occured
    // IGameEvent memory will be freed by manager if not needed anymore
    virtual void FireGameEvent( IGameEvent *event );


    // Generate say event.
    void GenerateSayEvent( edict_t* pEntity, const char* szText, bool bTeamOnly );

public: // Static methods.
    static void HudTextMessage( edict_t* pEntity, char *szTitle, char *szMessage, Color colour, int level, int time );

public: // Members.
    good::string sGameFolder;             ///< Game folder, i.e. "counter-strike source"
    good::string sModFolder;              ///< Mod folder, i.e. "cstrike"
    good::string sBotrixPath;             ///< Full path to Botrix folder, where config.ini and waypoints are.

    bool bIsLoaded;                       ///< True if this plugin was loaded (Load() was called by Source engine).
    bool bTeamPlay;                       ///< True if game is team based (like Counter-Strike), if false then it is deathmatch.

    bool bMapRunning;                     ///< True if map is currently running (LevelInit() was called by Source engine).
    good::string sMapName;                ///< Current map name (set at LevelInit()).

    static int iFPS;                      ///< Frames per second. Updated each second.
    static float fTime;                   ///< Current time.
    static float fEngineTime;             ///< Current engine time.

protected:
    static float m_fFpsEnd;               // Time of ending counting frames to calculate fps.
    static int m_iFramesCount;            // Count of frames since m_fFpsStart.

};

#endif // __BOTRIX_SERVER_PLUGIN_H__
