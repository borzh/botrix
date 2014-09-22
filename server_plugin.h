/**
* @mainpage Botrix plugin.
* @author Botrix ( botrix.plugin\@gmail.com )
*
* @version 0.0.4
*
* <h2>Bots plugin for Valve Games made with Source SDK.</h2>
*
* @section Changelog
*
* 0.0.5
* @li
*
* 0.0.4
* @li Fix crash autocompleting commands on listen server. No command completion for now.
* @li Added new commands:
*     -# botrix bot ally <bot-name> <player-name> <yes/no>
*     -# botrix bot attack <bot-name> <yes/no>
*     -# botrix bot move <bot-name> <yes/no>
*     -# botrix path distance
*
* 0.0.3
* @li Plugin is working with Team Fortress 2.
* @li Added new weapons handling in config.ini for TF2.
* @li Escaping TF2 steam ids in config.ini, for example \\[U:1:12345678].
* @li Bug fix: correct handling of plugin_pause/plugin_unpause commands.
* @li Bug fix: sometimes bot wasn't aware it picked up item.
* @li Bug fix: sometimes bot was shooting at spectator.
* @li Bug fix: when bot was trying to use weapon he actually was creating it.
* @li Bug fix: waypoints wasn't loading for maps with different case (linux).
* @li Now bots can use melee weapons.
* @li Now bots can use unknown weapons.
* @li Added logic for bot to pursue enemy.
* @li Bots will run randomly near engaged enemy, preferently using visible areas.
* @li Added new commands:
*     -# botrix bot config quota <#number/#player-#bot quota>
*     -# botrix bot config intelligence <bot-intelligence>
*     -# botrix bot config team <bot-team>
*     -# botrix bot config class <bot-class>
*     -# botrix bot config change-class <round-limit>
*     -# botrix bot config strategy flags
*     -# botrix bot config strategy set
*     -# botrix bot config suicide <time>
*     -# botrix bot command <bot-name> <command>
*     -# botrix bot weapon add <bot-name> <weapon-name>
*     -# botrix bot weapon unknown <melee/ranged>.
*     -# botrix config log <log-level>
*     -# botrix enable/disable
* @li Fixed commands:
*     -# botrix bot kick <bot-name/all>
* @li Waypoint editing: aim at waypoint to select 'path destination'.
*
* 0.0.2
* @li Default base folder location is <MOD DIRECTORY>/addons/botrix.
* @li Searching for base folder in 4 different locations.
* @li Log with levels (none, trace, debug, info, warning, error).
* @li Added command "version".
* @li Don't repeat bot names.
* @li Fixed several crashes.
*
* Originated from botman's bot template (aka HPB_bot).
*/

// TODO: Smart bots will keep distance according to the weapon they have.

#ifndef __BOTRIX_SERVER_PLUGIN_H__
#define __BOTRIX_SERVER_PLUGIN_H__


#include <good/string.h>

#include "public/engine/iserverplugin.h"
#include "public/igameevents.h"


#define PLUGIN_VERSION "0.0.5"

class IVEngineServer;
class IFileSystem;
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
class CBotrixPlugin : public IServerPluginCallbacks
{

public: // Static members.
    static CBotrixPlugin* instance;                    ///< Plugin singleton.

    static IVEngineServer* pEngineServer;              ///< Interface to util engine functions.
    static IGameEventManager2* pGameEventManager;      ///< Game events interface.
    static IPlayerInfoManager* pPlayerInfoManager;     ///< Interface to interact with players.
    static IServerPluginHelpers* pServerPluginHelpers; ///< To create menues on client side.
    static IServerGameClients* pServerGameClients;
    static IEngineTrace* pEngineTrace;
    static IBotManager* pBotManager;
    static ICvar* pCVar;


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
    virtual void OnEdictAllocated( edict_t *edict );
    virtual void OnEdictFreed( const edict_t *edict  );
#endif

    // Generate say event.
    void GenerateSayEvent( edict_t* pEntity, const char* szText, bool bTeamOnly );

public: // Static methods.
//    static void HudTextMessage( edict_t* pEntity, char *szTitle, char *szMessage,
//                                Color colour, int level, int time );

    /// Return true if plugin is enabled.
    bool IsEnabled() const { return m_bEnabled; }

    /// Enable/disable plugin.
    void Enable( bool bEnable );

public: // Members.
    good::string sGameFolder;             ///< Game folder, i.e. "counter-strike source"
    good::string sModFolder;              ///< Mod folder, i.e. "cstrike"
    good::string sBotrixPath;             ///< Full path to Botrix folder, where config.ini and waypoints are.

    bool bTeamPlay;                       ///< True if game is team based (like Counter-Strike), if false then it is deathmatch.

    bool bMapRunning;                     ///< True if map is currently running (LevelInit() was called by Source engine).
    good::string sMapName;                ///< Current map name (set at LevelInit()).

    static int iFPS;                      ///< Frames per second. Updated each second.
    static float fTime;                   ///< Current time.
    static float fEngineTime;             ///< Current engine time.

protected:

    void PrepareLevel( const char* szMapName );
    void ActivateLevel( int iMaxPlayers );

    bool m_bEnabled;                      ///< True if this plugin is enabled.
    bool m_bPaused;                       ///< True if this plugin is paused.
    static float m_fFpsEnd;               ///< Time of ending counting frames to calculate fps.
    static int m_iFramesCount;            ///< Count of frames since m_fFpsStart.

};

#endif // __BOTRIX_SERVER_PLUGIN_H__
