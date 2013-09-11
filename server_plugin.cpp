// Standard headers.
#include <stdlib.h> // rand().
#include <time.h> // time()

// Plugin headers.
#include "bot.h"
#include "clients.h"
#include "config.h"
#include "console_commands.h"
#include "item.h"
#include "event.h"
#include "icvar.h"
#include "server_plugin.h"
#include "source_engine.h"
#include "mod.h"
#include "waypoint.h"

// Good headers.
#include "good/file.h"

// Source headers.
#include "cbase.h"
#include "filesystem.h"
#include "interface.h"
#include "eiface.h"
#include "iplayerinfo.h"
#include "convar.h"
#include "IEngineTrace.h"
#include "IEffects.h"
#include "ndebugoverlay.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


int iMainBufferSize = 2048;
char szMainBufferArray[2048]; // Static buffer for different string purposes.

char* szMainBuffer = szMainBufferArray;

//----------------------------------------------------------------------------------------------------------------
// The plugin is a static singleton that is exported as an interface.
//----------------------------------------------------------------------------------------------------------------
static CBotrixPlugin g_cBotrixPlugin;
CBotrixPlugin* CBotrixPlugin::instance = &g_cBotrixPlugin;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CBotrixPlugin, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, g_cBotrixPlugin);

//----------------------------------------------------------------------------------------------------------------
// CBotrixPlugin static members.
//----------------------------------------------------------------------------------------------------------------
int CBotrixPlugin::iFPS = 60;
float CBotrixPlugin::fTime = 0.0f;
float CBotrixPlugin::fEngineTime = 0.0f;
float CBotrixPlugin::m_fFpsEnd = 0.0f;
int CBotrixPlugin::m_iFramesCount = 60;


//----------------------------------------------------------------------------------------------------------------
// Interfaces from the CBotrixPlugin::pEngineServer.
//----------------------------------------------------------------------------------------------------------------
IVEngineServer* CBotrixPlugin::pEngineServer = NULL;
IFileSystem* pFileSystem = NULL;
IGameEventManager2* CBotrixPlugin::pGameEventManager2 = NULL;
IGameEventManager* CBotrixPlugin::pGameEventManager = NULL;
IPlayerInfoManager* CBotrixPlugin::pPlayerInfoManager = NULL;
IServerPluginHelpers* CBotrixPlugin::pServerPluginHelpers = NULL;
IServerGameClients* CBotrixPlugin::pServerGameClients = NULL;
//IServerGameEnts* CBotrixPlugin::pServerGameEnts;
IEngineTrace* CBotrixPlugin::pEngineTrace = NULL;
IEffects* pEffects = NULL;
IBotManager* CBotrixPlugin::pBotManager = NULL;
//CGlobalVars* CBotrixPlugin::pGlobalVars = NULL;
ICvar* CBotrixPlugin::pCvar = NULL;

IVDebugOverlay* pVDebugOverlay = NULL;

//----------------------------------------------------------------------------------------------------------------
// Plugin functions.
//----------------------------------------------------------------------------------------------------------------
#define LOAD_INTERFACE(var,type,version) \
	if ((var =(type*)pInterfaceFactory(version, NULL)) == NULL ) {\
		Warning("[Botrix] Cannot open interface "## #version ##" "## #type ##" "## #var ##"\n");\
		return false;\
	}

#define LOAD_INTERFACE_IGNORE_ERROR(var,type,version) \
	if ((var =(type*)pInterfaceFactory(version, NULL)) == NULL ) {\
		Warning("[Botrix] Cannot open interface "## #version ##" "## #type ##" "## #var ##"\n");\
	}

#define LOAD_GAME_SERVER_INTERFACE(var, type, version) \
	if ((var =(type*)pGameServerFactory(version, NULL)) == NULL ) {\
		Warning("[Botrix] Cannot open game server interface "## #version ##" "## #type ##" "## #var ##"\n");\
		return false;\
	}

//----------------------------------------------------------------------------------------------------------------
CBotrixPlugin::CBotrixPlugin(): bIsLoaded(false) {}

//----------------------------------------------------------------------------------------------------------------
CBotrixPlugin::~CBotrixPlugin() {}

//----------------------------------------------------------------------------------------------------------------
// Called when the plugin is loaded. Load all needed interfaces using engine.
//----------------------------------------------------------------------------------------------------------------
bool CBotrixPlugin::Load( CreateInterfaceFn pInterfaceFactory, CreateInterfaceFn pGameServerFactory )
{
	LOAD_GAME_SERVER_INTERFACE(pPlayerInfoManager,IPlayerInfoManager,INTERFACEVERSION_PLAYERINFOMANAGER);

	//pGlobalVars = pPlayerInfoManager->GetGlobalVars();	

	LOAD_INTERFACE(pEngineServer, IVEngineServer, INTERFACEVERSION_VENGINESERVER);
	LOAD_INTERFACE(pServerPluginHelpers, IServerPluginHelpers, INTERFACEVERSION_ISERVERPLUGINHELPERS);
	LOAD_INTERFACE(pEngineTrace, IEngineTrace, INTERFACEVERSION_ENGINETRACE_SERVER);
	LOAD_GAME_SERVER_INTERFACE(pEffects, IEffects, IEFFECTS_INTERFACE_VERSION);
	LOAD_GAME_SERVER_INTERFACE(pBotManager, IBotManager, INTERFACEVERSION_PLAYERBOTMANAGER);

	LOAD_INTERFACE(pVDebugOverlay, IVDebugOverlay, VDEBUG_OVERLAY_INTERFACE_VERSION);
	LOAD_INTERFACE(pGameEventManager, IGameEventManager, INTERFACEVERSION_GAMEEVENTSMANAGER)
	LOAD_INTERFACE(pGameEventManager2, IGameEventManager2, INTERFACEVERSION_GAMEEVENTSMANAGER2)

	LOAD_GAME_SERVER_INTERFACE(pServerGameClients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
	LOAD_INTERFACE_IGNORE_ERROR(pFileSystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);
	//LOAD_GAME_SERVER_INTERFACE(pServerGameEnts, IServerGameEnts, INTERFACEVERSION_SERVERGAMECLIENTS);

#ifdef SOURCE_ENGINE_2006
	LOAD_INTERFACE(pCvar, ICvar, VENGINE_CVAR_INTERFACE_VERSION);
#else
	LOAD_INTERFACE(pCvar, ICvar, CVAR_INTERFACE_VERSION);
#endif

	// Get game/mod directories.
	pEngineServer->GetGameDir(szMainBuffer, iMainBufferSize);	
	sModFolder = szMainBuffer;
	int modPos = sModFolder.rfind(PATH_SEPARATOR);
	sModFolder = good::string(&szMainBuffer[modPos+1], true, true);

	if ( pFileSystem )
		pFileSystem->GetCurrentDirectory(szMainBuffer, iMainBufferSize);
	else
		szMainBuffer[modPos] = 0; // Remove trailing mod name (for example hl2dm).

	sGameFolder = szMainBuffer;
	int gamePos = sGameFolder.rfind(PATH_SEPARATOR);
	szMainBuffer[gamePos] = 0;
	sGameFolder = good::string(&szMainBuffer[gamePos+1], true, true);

	sBotrixPath = good::file::append_path(szMainBuffer, "Botrix");

	// Load configuration file.
	good::string iniName( good::file::append_path(sBotrixPath, "config.ini") );
	TModId iModId = CConfiguration::Load(iniName, sGameFolder, sModFolder);

	// Create console command instance.
	CMainCommand::instance = new CMainCommand();

	// Load mod configuration.
	CMod::Load(iModId);

#ifdef SOURCE_ENGINE_2006
	MathLib_Init(1.0, 1.0, 1.0, 1.0, false, true, false, true);
#else
	MathLib_Init();
#endif
	srand( time(NULL) );

	bIsLoaded = true;
	CUtil::Message(NULL, "Botrix loaded. Current mod: %s.\n", CMod::sModName.c_str());

	return true;
}

//----------------------------------------------------------------------------------------------------------------
// Called when the plugin is unloaded
//----------------------------------------------------------------------------------------------------------------
void CBotrixPlugin::Unload( void )
{
	CConfiguration::Save();
	CConfiguration::Unload();

	CMainCommand::instance.reset();

	if ( pGameEventManager )
		pGameEventManager->RemoveListener( this ); // make sure we are unloaded from the event system
	if ( pGameEventManager2 )
		pGameEventManager2->RemoveListener( this );

	bIsLoaded = false;
}

//----------------------------------------------------------------------------------------------------------------
// Called when the plugin is paused(i.e should stop running but isn't unloaded)
//----------------------------------------------------------------------------------------------------------------
void CBotrixPlugin::Pause( void )
{
}

//----------------------------------------------------------------------------------------------------------------
// Called when the plugin is unpaused(i.e should start executing again)
//----------------------------------------------------------------------------------------------------------------
void CBotrixPlugin::UnPause( void )
{
}

//----------------------------------------------------------------------------------------------------------------
// the name of this plugin, returned in "plugin_print" command
//----------------------------------------------------------------------------------------------------------------
const char* CBotrixPlugin::GetPluginDescription( void )
{
	return "Botrix plugin " PLUGIN_VERSION " (c) 2012 by Borzh. BUILD " __DATE__;
}

//----------------------------------------------------------------------------------------------------------------
// Called on level start
//----------------------------------------------------------------------------------------------------------------
void CBotrixPlugin::LevelInit( const char* pMapName )
{
	sMapName = pMapName;
	bMapRunning = true;

	ConVar *pTeamplay = pCvar->FindVar("mp_teamplay");
	bTeamPlay = pTeamplay ? pTeamplay->GetBool() : false;

	pGameEventManager->AddListener( this, true );	

	CWaypoints::Load();
	CWaypoint::iWaypointTexture = CBotrixPlugin::pEngineServer->PrecacheModel( "sprites/lgtning.vmt" );
}

//----------------------------------------------------------------------------------------------------------------
// Called on level start, when the server is ready to accept client connections
// edictCount is the number of entities in the level, clientMax is the max client count
//----------------------------------------------------------------------------------------------------------------
void CBotrixPlugin::ServerActivate( edict_t* pEdictList, int edictCount, int clientMax )
{
	CPlayers::Init(clientMax);
	CItems::MapLoaded();
	CMod::MapLoaded();

	CUtil::Message(NULL, "Level \"%s\" has been loaded", sMapName.c_str());
	CUtil::Message(NULL, "Max clients: %d\n", clientMax);
}

//----------------------------------------------------------------------------------------------------------------
// Called once per server frame, do recurring work here(like checking for timeouts)
//----------------------------------------------------------------------------------------------------------------
void CBotrixPlugin::GameFrame( bool simulating )
{
	CUtil::PrintMessagesInQueue();

	float fPrevEngineTime = fEngineTime;
	fEngineTime = pEngineServer->Time();

	float fDiff = fEngineTime - fPrevEngineTime;
	if ( fDiff > 1.0f ) // Too low fps, possibly debugging.
		fTime += 0.1f;
	else
		fTime += fDiff;

	// FPS counting. Used in draw waypoints.
	/*m_iFramesCount++;
	if (fEngineTime >= m_fFpsEnd)
	{
		iFPS = m_iFramesCount;
		m_iFramesCount = 0;
		m_fFpsEnd = fEngineTime + 1.0f;
		//CUtil::Message(NULL, "FPS: %d", iFPS);
	}*/

	if ( bMapRunning )
	{
		CMod::pCurrentMod->Think();

		// Show fps.
		//m_iFramesCount++;
		//if (fTime >= m_fFpsEnd)
		//{
		//	iFPS = m_iFramesCount;
		//	m_iFramesCount = 0;
		//	m_fFpsEnd = fTime + 1.0f;
		//	CUtil::Message(NULL, "FPS: %d", iFPS);
		//}

		CItems::Update();
		CPlayers::PreThink();
		//CUtil::Message(NULL, "Players think time: %.5f", pEngineServer->Time() - fTime);

		//if ( CWaypoints::NeedToWorkVisibility() )
		//	CWaypoints::WorkVisibility();
	}
}

//----------------------------------------------------------------------------------------------------------------
// Called on level end (as the server is shutting down or going to a new map)
//----------------------------------------------------------------------------------------------------------------
void CBotrixPlugin::LevelShutdown( void ) // !!!!this can get called multiple times per map change
{
	bMapRunning = false;

	CPlayers::Clear();
	CWaypoints::Clear();
	CItems::MapUnloaded();

	pGameEventManager->RemoveListener( this );
	if ( pGameEventManager2 )
		pGameEventManager2->RemoveListener( this );

	CConfiguration::Save();
}

//----------------------------------------------------------------------------------------------------------------
// Called when a client spawns into a server (i.e as they begin to play)
//----------------------------------------------------------------------------------------------------------------
void CBotrixPlugin::ClientActive( edict_t* pEntity )
{
	CPlayers::PlayerConnected(pEntity);
}

//----------------------------------------------------------------------------------------------------------------
// Called when a client leaves a server(or is timed out)
//----------------------------------------------------------------------------------------------------------------
void CBotrixPlugin::ClientDisconnect( edict_t* pEntity )
{
	CPlayers::PlayerDisconnected(pEntity);
}

//----------------------------------------------------------------------------------------------------------------
// Called on client being added to this server
//----------------------------------------------------------------------------------------------------------------
void CBotrixPlugin::ClientPutInServer( edict_t* pEntity, const char* playername )
{
}

//----------------------------------------------------------------------------------------------------------------
// Sets the client index for the client who typed the command into their console
//----------------------------------------------------------------------------------------------------------------
void CBotrixPlugin::SetCommandClient( int index )
{
	//m_iClientCommandIndex = index;
}

//----------------------------------------------------------------------------------------------------------------
// A player changed one/several replicated cvars (name etc)
//----------------------------------------------------------------------------------------------------------------
void CBotrixPlugin::ClientSettingsChanged( edict_t* pEdict )
{
	//const char* name = pEngineServer->GetClientConVarValue( pEngineServer->IndexOfEdict(pEdict), "name" );
}

//----------------------------------------------------------------------------------------------------------------
// Called when a client joins a server.
//----------------------------------------------------------------------------------------------------------------
PLUGIN_RESULT CBotrixPlugin::ClientConnect( bool *bAllowConnect, edict_t* pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen )
{
	return PLUGIN_CONTINUE;
}

//----------------------------------------------------------------------------------------------------------------
// Called when a client types in a command (only a subset of commands however, not CON_COMMAND's)
//----------------------------------------------------------------------------------------------------------------
#ifdef SOURCE_ENGINE_2006
PLUGIN_RESULT CBotrixPlugin::ClientCommand( edict_t* pEntity )
#else
PLUGIN_RESULT CBotrixPlugin::ClientCommand( edict_t* pEntity, const CCommand &args )
#endif
{
	DebugAssert( pEntity && !pEntity->IsFree() );

#ifdef SOURCE_ENGINE_2006
	int argc = MIN2(CBotrixPlugin::pEngineServer->Cmd_Argc(), 16);

	if ( (argc == 0) || !CMainCommand::instance->IsCommand( CBotrixPlugin::pEngineServer->Cmd_Argv(0) ) )
		return PLUGIN_CONTINUE;

	static const char* argv[16];
	for (int i = 0; i < argc; ++i)
		argv[i] = CBotrixPlugin::pEngineServer->Cmd_Argv(i);
#else
	int argc = args.ArgC();
	const char** argv = args.ArgV();

	if ( (argc == 0) || !CMainCommand::instance->IsCommand( argv[0] ) )
		return PLUGIN_CONTINUE;
#endif

	int iIdx = CPlayers::Get(pEntity);
	DebugAssert(iIdx >= 0);

	CPlayer* pPlayer = CPlayers::Get(iIdx);
	DebugAssert( pPlayer && !pPlayer->IsBot() );

	CClient* pClient = (CClient*)pPlayer;

	TCommandResult iResult = CMainCommand::instance->Execute( pClient, argc-1, &argv[1] );

	if (iResult != ECommandPerformed)
	{
		if (iResult == ECommandRequireAccess)
			CUtil::Message(pClient ? pClient->GetEdict() : NULL, "Sorry, you don't have access to this command.");
		else if (iResult == ECommandNotFound)
			CUtil::Message(pClient ? pClient->GetEdict() : NULL, "Bot's command not found.");
		else if (iResult == ECommandError)
			CUtil::Message(pClient ? pClient->GetEdict() : NULL, "Command error.");	
	}
	return PLUGIN_STOP; // We handled this command.
}

//----------------------------------------------------------------------------------------------------------------
// Called when a client is authenticated
//----------------------------------------------------------------------------------------------------------------
PLUGIN_RESULT CBotrixPlugin::NetworkIDValidated( const char *pszUserName, const char *pszNetworkID )
{
	return PLUGIN_CONTINUE;
}

//----------------------------------------------------------------------------------------------------------------
// Called when an event is fired
//----------------------------------------------------------------------------------------------------------------
void CBotrixPlugin::FireGameEvent( KeyValues* event )
{
	good::string_buffer sbBuffer(szMainBuffer, iMainBufferSize, false);

	const char* type = event->GetName();

	CMod::ExecuteEvent( (void*)event, EEventTypeKeyValues );

	if ( CPlayers::IsDebuggingEvents() )
	{
		KeyValues* copy = event->MakeCopy(); // Source bug? Heap corrupted if iterate directly on event.

		sbBuffer.append("Event: ");
		sbBuffer.append(type);
		sbBuffer.append('\n');

		for ( KeyValues *pk = copy->GetFirstTrueSubKey(); pk; pk = pk->GetNextTrueSubKey() )
		{
			sbBuffer.append(pk->GetName());
			sbBuffer.append('\n');
		}

		for ( KeyValues *pk = copy->GetFirstValue(); pk; pk = pk->GetNextValue() )
		{
			sbBuffer.append(pk->GetName());
			sbBuffer.append(" = ");
			sbBuffer.append(pk->GetString());
			sbBuffer.append('\n');
		}

		CPlayers::DebugEvent(sbBuffer.c_str());

		copy->deleteThis();
	}
}

void CBotrixPlugin::FireGameEvent( IGameEvent * event )
{
	CMod::ExecuteEvent( (void*)event, EEventTypeKeyValues );
}

void CBotrixPlugin::HudTextMessage( edict_t* pEntity, char *szMsgName, char *szTitle, char *szMessage, Color colour, int level, int time )
{
	KeyValues *kv = new KeyValues( "menu" );
	kv->SetString( "title", szMessage );
	//kv->SetString( "msg", szMessage );
	kv->SetColor( "color", colour);
	kv->SetInt( "level", level);
	kv->SetInt( "time", time);
	//DIALOG_TEXT
	pServerPluginHelpers->CreateMessage( pEntity, DIALOG_MSG, kv, instance );

	kv->deleteThis();
}
