#ifndef __BOTRIX_CONSOLE_COMMANDS_H__
#define __BOTRIX_CONSOLE_COMMANDS_H__

#include <good/memory.h>

#include "types.h"

#include "clients.h"
#include "item.h"
#include "type2string.h"

#include "public/tier1/convar.h"
#include "public/edict.h"


//****************************************************************************************************************
/// Console command.
//****************************************************************************************************************
class CConsoleCommand
{
public:

    CConsoleCommand( char *szCommand, TCommandAccessFlags iCommandAccessFlags = FCommandAccessNone ):
        m_sCommand(szCommand), m_iAccessLevel(iCommandAccessFlags) {}

    virtual ~CConsoleCommand() {}

    bool IsCommand( const char* szCommand ) { return m_sCommand == szCommand; }

    bool HasAccess( CClient* pClient )
    {
        TCommandAccessFlags access = pClient ? pClient->iCommandAccessFlags : FCommandAccessAll;
        return FLAG_ALL_SET(m_iAccessLevel, access);
    }

    virtual TCommandResult Execute( CClient* pClient, int argc, const char** argv ) = 0;

    virtual int AutoComplete( const char* partial, int partialLength, char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ], int strIndex, int charIndex );
    virtual void PrintCommand( edict_t* pPrintTo, int indent = 0);

protected:
    CConsoleCommand() : m_iAccessLevel(0), m_bAutoCompleteOnlyOneArgument(false) {}

    good::string m_sCommand;
    int m_iAccessLevel;

    good::string m_sHelp;
    good::string m_sDescription;
    StringVector m_cAutoCompleteArguments;
    bool m_bAutoCompleteOnlyOneArgument;
};


//****************************************************************************************************************
/// Container of commands.
//****************************************************************************************************************
class CConsoleCommandContainer: public CConsoleCommand
{
public:
    TCommandResult Execute( CClient* pClient, int argc, const char** argv );

    void Add( CConsoleCommand* newCommand ) { m_commands.push_back( good::unique_ptr<CConsoleCommand>(newCommand) ); }

    virtual int AutoComplete( const char* partial, int partialLength, char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ], int strIndex, int charIndex );
    virtual void PrintCommand( edict_t* pPrintTo, int indent = 0);

protected:
    good::vector< good::unique_ptr<CConsoleCommand> > m_commands;
};



//****************************************************************************************************************
// Waypoint commands.
//****************************************************************************************************************
class CWaypointDrawFlagCommand: public CConsoleCommand
{
public:
    CWaypointDrawFlagCommand();
    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CWaypointResetCommand: public CConsoleCommand
{
public:
    CWaypointResetCommand()
    {
        m_sCommand = "reset";
        m_sHelp = "reset current waypoint to nearest";
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CWaypointCreateCommand: public CConsoleCommand
{
public:
    CWaypointCreateCommand()
    {
        m_sCommand = "create";
        m_sHelp = "create new waypoint at current player's position";
        m_iAccessLevel = FCommandAccessWaypoint;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CWaypointRemoveCommand: public CConsoleCommand
{
public:
    CWaypointRemoveCommand()
    {
        m_sCommand = "remove";
        m_sHelp = "delete given or current waypoint";
        m_iAccessLevel = FCommandAccessWaypoint;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CWaypointMoveCommand: public CConsoleCommand
{
public:
    CWaypointMoveCommand()
    {
        m_sCommand = "move";
        m_sHelp = "moves destination or given waypoint to current player's position";
        m_iAccessLevel = FCommandAccessWaypoint;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CWaypointAutoCreateCommand: public CConsoleCommand
{
public:
    CWaypointAutoCreateCommand()
    {
        m_sCommand = "autocreate";
        m_sHelp = "automatically create new waypoints ('off' - disable, 'on' - enable)";
        m_sDescription = "Waypoint will be added when player goes too far from current one.";
        m_iAccessLevel = FCommandAccessWaypoint;
        m_cAutoCompleteArguments.push_back("on");
        m_cAutoCompleteArguments.push_back("off");
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CWaypointInfoCommand: public CConsoleCommand
{
public:
    CWaypointInfoCommand()
    {
        m_sCommand = "info";
        m_sHelp = "display info of current waypoint at console";
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CWaypointDestinationCommand: public CConsoleCommand
{
public:
    CWaypointDestinationCommand()
    {
        m_sCommand = "destination";
        m_sHelp = "set given or current waypoint as path 'destination'";
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CWaypointSaveCommand: public CConsoleCommand
{
public:
    CWaypointSaveCommand()
    {
        m_sCommand = "save";
        m_sHelp = "save waypoints";
        m_iAccessLevel = FCommandAccessWaypoint;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CWaypointLoadCommand: public CConsoleCommand
{
public:
    CWaypointLoadCommand()
    {
        m_sCommand = "load";
        m_sHelp = "load waypoints";
        m_iAccessLevel = FCommandAccessWaypoint;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CWaypointClearCommand: public CConsoleCommand
{
public:
    CWaypointClearCommand()
    {
        m_sCommand = "clear";
        m_sHelp = "delete all waypoints";
        m_iAccessLevel = FCommandAccessWaypoint;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CWaypointAddTypeCommand: public CConsoleCommand
{
public:
    CWaypointAddTypeCommand()
    {
        m_sCommand = "addtype";
        m_sHelp = "add type to waypoint";
        m_sDescription = good::string("Can be mix of: ") + CTypeToString::WaypointFlagsToString(FWaypointAll);
        m_iAccessLevel = FCommandAccessWaypoint;

        for (int i=0; i < FWaypointAll; ++i)
            m_cAutoCompleteArguments.push_back( CTypeToString::WaypointFlagsToString(1<<i).duplicate() );
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CWaypointRemoveTypeCommand: public CConsoleCommand
{
public:
    CWaypointRemoveTypeCommand()
    {
        m_sCommand = "removetype";
        m_sHelp = "remove all waypoint types";
        m_iAccessLevel = FCommandAccessWaypoint;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CWaypointArgumentCommand: public CConsoleCommand
{
public:
    CWaypointArgumentCommand()
    {
        m_sCommand = "argument";
        m_sHelp = "set waypoint arguments (angles, ammo count, weapon index/subindex, armor count, health count)";
        m_iAccessLevel = FCommandAccessWaypoint; // TODO: autocomplete
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CWaypointVisibilityCommand: public CConsoleCommand
{
public:
    CWaypointVisibilityCommand();
    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};


//****************************************************************************************************************
// Area waypoint commands.
//****************************************************************************************************************
class CWaypointAreaRemoveCommand: public CConsoleCommand
{
public:
    CWaypointAreaRemoveCommand()
    {
        m_sCommand = "remove";
        m_sHelp = "delete waypoint area";
        m_iAccessLevel = FCommandAccessWaypoint;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CWaypointAreaRenameCommand: public CConsoleCommand
{
public:
    CWaypointAreaRenameCommand()
    {
        m_sCommand = "rename";
        m_sHelp = "rename waypoint area";
        m_iAccessLevel = FCommandAccessWaypoint;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CWaypointAreaSetCommand: public CConsoleCommand
{
public:
    CWaypointAreaSetCommand()
    {
        m_sCommand = "set";
        m_sHelp = "set waypoint area";
        m_iAccessLevel = FCommandAccessWaypoint;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CWaypointAreaShowCommand: public CConsoleCommand
{
public:
    CWaypointAreaShowCommand()
    {
        m_sCommand = "show";
        m_sHelp = "print all waypoint areas";
        m_iAccessLevel = FCommandAccessWaypoint;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

//****************************************************************************************************************
// Path waypoint commands.
//****************************************************************************************************************
class CPathDrawCommand: public CConsoleCommand
{
public:
    CPathDrawCommand();
    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CPathCreateCommand: public CConsoleCommand
{
public:
    CPathCreateCommand()
    {
        m_sCommand = "create";
        m_sHelp = "create path (from current waypoint to 'destination')";
        m_iAccessLevel = FCommandAccessWaypoint;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CPathRemoveCommand: public CConsoleCommand
{
public:
    CPathRemoveCommand()
    {
        m_sCommand = "remove";
        m_sHelp = "remove path (from current waypoint to 'destination')";
        m_iAccessLevel = FCommandAccessWaypoint;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CPathAutoCreateCommand: public CConsoleCommand
{
public:
    CPathAutoCreateCommand()
    {
        m_sCommand = "autocreate";
        m_sHelp = "enable auto path creation for new waypoints ('off' - disable, 'on' - enable)";
        m_sDescription = "If disabled, only path from 'destination' to new waypoint will be added";
        m_iAccessLevel = FCommandAccessWaypoint;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

/*
class CPathSwapCommand: public CConsoleCommand
{
public:
    CPathSwapCommand()
    {
        m_sCommand = "swap";
        m_sHelp = "set current waypoint as 'destination' and teleport to old 'destination'";
        m_sDescription = "If argument is provided, then teleport there";
        m_iAccessLevel = FCommandAccessWaypoint;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};
*/

class CPathAddTypeCommand: public CConsoleCommand
{
public:
    CPathAddTypeCommand()
    {
        m_sCommand = "addtype";
        m_sHelp = "add path type (from current waypoint to 'destination').";
        m_sDescription = good::string("Can be mix of: ") + CTypeToString::PathFlagsToString(FPathAll);
        m_iAccessLevel = FCommandAccessWaypoint;

        for (int i=0; i < FPathTotal; ++i)
            m_cAutoCompleteArguments.push_back( CTypeToString::PathFlagsToString(1<<i).duplicate() );
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CPathRemoveTypeCommand: public CConsoleCommand
{
public:
    CPathRemoveTypeCommand()
    {
        m_sCommand = "removetype";
        m_sHelp = "remove path type (from current waypoint to 'destination')";
        m_iAccessLevel = FCommandAccessWaypoint;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CPathArgumentCommand: public CConsoleCommand
{
public:
    CPathArgumentCommand()
    {
        m_sCommand = "argument";
        m_sHelp = "set path arguments.";
        m_sDescription = "First parameter is time to wait before action, and second is action duration.";
        m_iAccessLevel = FCommandAccessWaypoint;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CPathInfoCommand: public CConsoleCommand
{
public:
    CPathInfoCommand()
    {
        m_sCommand = "info";
        m_sHelp = "display path info on console (from current waypoint to 'destination')";
        m_iAccessLevel = FCommandAccessWaypoint;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};


//****************************************************************************************************************
// Weapon commands.
//****************************************************************************************************************
class CBotWeaponAddCommand: public CConsoleCommand
{
public:
    CBotWeaponAddCommand()
    {
        m_sCommand = "add";
        m_sHelp = "add a weapon to bot";
        m_iAccessLevel = FCommandAccessBot;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CBotWeaponAllowCommand: public CConsoleCommand
{
public:
    CBotWeaponAllowCommand()
    {
        m_sCommand = "allow";
        m_sHelp = "allow bots to use given weapons";
        m_iAccessLevel = FCommandAccessBot;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CBotWeaponForbidCommand: public CConsoleCommand
{
public:
    CBotWeaponForbidCommand()
    {
        m_sCommand = "forbid";
        m_sHelp = "forbid bots to use given weapons";
        m_iAccessLevel = FCommandAccessBot;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CBotWeaponRemoveCommand: public CConsoleCommand
{
public:
    CBotWeaponRemoveCommand()
    {
        m_sCommand = "remove";
        m_sHelp = "remove all weapons from bot";
        m_iAccessLevel = FCommandAccessBot;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CBotWeaponUnknownCommand: public CConsoleCommand
{
public:
    CBotWeaponUnknownCommand()
    {
        m_sCommand = "unknown";
        m_sHelp = "bot assumption about unknown weapons ('melee' or 'ranged')";
        m_sDescription = "If bot grabs or respawns with unknown weapon, choose it to be marked as melee or ranged";
        m_iAccessLevel = FCommandAccessBot;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};


//****************************************************************************************************************
// Bot commands.
//****************************************************************************************************************
class CBotAddCommand: public CConsoleCommand
{
public:
    CBotAddCommand()
    {
        m_sCommand = "add";
        m_sHelp = "add random bot";
        m_iAccessLevel = FCommandAccessBot;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CBotKickCommand: public CConsoleCommand
{
public:
    CBotKickCommand()
    {
        m_sCommand = "kick";
        m_sHelp = "kick random bot or bot on team (given argument)";
        m_iAccessLevel = FCommandAccessBot;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CBotDebugCommand: public CConsoleCommand
{
public:
    CBotDebugCommand()
    {
        m_sCommand = "debug";
        m_sHelp = "show bot debug messages on server (arguments are bot name and optionally 'on' or 'off')";
        m_iAccessLevel = FCommandAccessBot;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CBotDefaultIntelligenceCommand: public CConsoleCommand
{
public:
    CBotDefaultIntelligenceCommand()
    {
        m_sCommand = "intelligence";
        m_sHelp = "set default bot intelligence";
        m_sDescription = "Can be one of: random fool stupied normal smart pro";// TODO: intelligence flags to string.
        m_iAccessLevel = FCommandAccessBot;

        m_cAutoCompleteArguments.push_back("random");
        for (int i=0; i < EBotIntelligenceTotal; ++i)
            m_cAutoCompleteArguments.push_back( CTypeToString::IntelligenceToString(i).duplicate() );
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CBotDefaultTeamCommand: public CConsoleCommand
{
public:
    CBotDefaultTeamCommand()
    {
        m_sCommand = "team";
        m_sHelp = "set default bot team";
        m_sDescription = good::string("Can be one of: ") + CTypeToString::TeamFlagsToString(-1);
        m_iAccessLevel = FCommandAccessBot;

        m_cAutoCompleteArguments.push_back("random");
        for (int i=0; i < CMod::aTeamsNames.size(); ++i)
            m_cAutoCompleteArguments.push_back( CTypeToString::TeamToString(i).duplicate() );
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CBotDefaultClassCommand: public CConsoleCommand
{
public:
    CBotDefaultClassCommand()
    {
        m_sCommand = "class";
        m_sHelp = "set default bot class";
        m_sDescription = good::string("Can be one of: random ") + CTypeToString::ClassFlagsToString(-1);
        m_iAccessLevel = FCommandAccessBot;

        m_cAutoCompleteArguments.push_back("random");
        for (int i=0; i < CMod::aClassNames.size(); ++i)
            m_cAutoCompleteArguments.push_back( CTypeToString::ClassToString(i).duplicate() );
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CBotDefaultCommand: public CConsoleCommandContainer
{
public:
    CBotDefaultCommand()
    {
        m_sCommand = "default";
        Add(new CBotDefaultIntelligenceCommand());
        Add(new CBotDefaultTeamCommand());
        if ( CMod::aClassNames.size() )
            Add(new CBotDefaultClassCommand());
    }
};

class CBotDrawPathCommand: public CConsoleCommand
{
public:
    CBotDrawPathCommand()
    {
        m_sCommand = "drawpath";
        m_sHelp = "defines how to draw bot's path";
        m_sDescription = good::string("Can be 'none' / 'all' / 'next' or mix of: ") + CTypeToString::PathDrawFlagsToString(FPathDrawAll);
        m_iAccessLevel = FCommandAccessBot;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CBotPauseCommand: public CConsoleCommand
{
public:
    CBotPauseCommand()
    {
        m_sCommand = "pause";
        m_sHelp = "pause/resume given or all bots";
        m_iAccessLevel = FCommandAccessBot;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CBotTestPathCommand: public CConsoleCommand
{
public:
    CBotTestPathCommand()
    {
        m_sCommand = "test";
        m_sHelp = "create bot to test path from given (current) to given (destination) waypoints";
        m_iAccessLevel = FCommandAccessBot;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CBotWeaponCommand: public CConsoleCommandContainer
{
public:
    CBotWeaponCommand()
    {
        m_sCommand = "weapon";
        Add(new CBotWeaponAddCommand());
        Add(new CBotWeaponAllowCommand());
        Add(new CBotWeaponForbidCommand());
        //Add(new CBotWeaponRemoveCommand());
        Add(new CBotWeaponUnknownCommand());
    }
};


//****************************************************************************************************************
// Item commands.
//****************************************************************************************************************
class CItemDrawCommand: public CConsoleCommand
{
public:
    CItemDrawCommand()
    {
        m_sCommand = "draw";
        m_sHelp = "defines which items to draw";
        m_sDescription = good::string("Can be 'none' / 'all' / 'next' or mix of: ") + CTypeToString::EntityTypeFlagsToString(EItemTypeAll);
        m_iAccessLevel = FCommandAccessWaypoint; // User doesn't have control over items, he only can draw them.
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};


class CItemDrawTypeCommand: public CConsoleCommand
{
public:
    CItemDrawTypeCommand()
    {
        m_sCommand = "drawtype";
        m_sHelp = "defines how to draw items";
        m_sDescription = good::string("Can be 'none' / 'all' / 'next' or mix of: ") + CTypeToString::ItemDrawFlagsToString(EItemDrawAll);
        m_iAccessLevel = FCommandAccessWaypoint;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};


class CItemReloadCommand: public CConsoleCommand
{
public:
    CItemReloadCommand()
    {
        m_sCommand = "reload";
        m_sHelp = "reload all items (will recalculate nearest waypoints)";
        m_iAccessLevel = FCommandAccessWaypoint;
    }

    TCommandResult Execute( CClient* pClient, int /*argc*/, const char** /*argv*/ )
    {
        if ( pClient == NULL )
            return ECommandError;

        CItems::MapUnloaded();
        CItems::MapLoaded();
        return ECommandPerformed;
    }
};


//****************************************************************************************************************
// Config commands.
//****************************************************************************************************************
class CConfigAdminsSetAccessCommand: public CConsoleCommand
{
public:
    CConfigAdminsSetAccessCommand()
    {
        m_sCommand = "access";
        m_sHelp = "set access flags for given admin";
        m_sDescription = good::string("Arguments: <Steam ID> <Access Flags>. Can be none / all / mix of: ") +
                         CTypeToString::AccessFlagsToString(FCommandAccessAll);
        m_iAccessLevel = FCommandAccessConfig;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );

};

class CConfigAdminsShowCommand: public CConsoleCommand
{
public:
    CConfigAdminsShowCommand()
    {
        m_sCommand = "show";
        m_sHelp = "show admins currently on server";
        m_iAccessLevel = FCommandAccessConfig;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );

};

class CConfigEventsCommand: public CConsoleCommand
{
public:
    CConfigEventsCommand()
    {
        m_sCommand = "event";
        m_sHelp = "display events on console ('off' - disable, 'on' - enable)";
        m_iAccessLevel = FCommandAccessConfig;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CConfigLogCommand: public CConsoleCommand
{
public:
    CConfigLogCommand()
    {
        m_sCommand = "log";
        m_sHelp = "set console log level (none, trace, debug, info, warning, error).";
        m_iAccessLevel = FCommandAccessConfig;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};


//****************************************************************************************************************
// Admins: show admins and set admin flags.
//****************************************************************************************************************
class CConfigAdminsCommand: public CConsoleCommandContainer
{
public:
    CConfigAdminsCommand()
    {
        m_sCommand = "admins";
        Add(new CConfigAdminsSetAccessCommand());
        Add(new CConfigAdminsShowCommand());
    }
};


//****************************************************************************************************************
// Waypoint areas: show/set/rename waypoint area.
//****************************************************************************************************************
class CWaypointAreaCommand: public CConsoleCommandContainer
{
public:
    CWaypointAreaCommand()
    {
        m_sCommand = "area";
        Add(new CWaypointAreaRemoveCommand());
        Add(new CWaypointAreaRenameCommand());
        Add(new CWaypointAreaSetCommand());
        Add(new CWaypointAreaShowCommand());
    }
};


//****************************************************************************************************************
// Container of all commands starting with "waypoint".
//****************************************************************************************************************
class CWaypointCommand: public CConsoleCommandContainer
{
public:
    CWaypointCommand()
    {
        m_sCommand = "waypoint";
        Add(new CWaypointAddTypeCommand());
        Add(new CWaypointAreaCommand());
        Add(new CWaypointArgumentCommand());
        Add(new CWaypointAutoCreateCommand());
        Add(new CWaypointClearCommand());
        Add(new CWaypointCreateCommand());
        Add(new CWaypointDestinationCommand()); // Todo: change waypoint by look.
        Add(new CWaypointDrawFlagCommand());
        Add(new CWaypointInfoCommand());
        Add(new CWaypointLoadCommand());
        Add(new CWaypointMoveCommand());
        Add(new CWaypointRemoveCommand());
        Add(new CWaypointRemoveTypeCommand());
        Add(new CWaypointResetCommand());
        Add(new CWaypointSaveCommand());
        Add(new CWaypointVisibilityCommand());
    }
};


//****************************************************************************************************************
// Container of all commands starting with "pathwaypoint".
//****************************************************************************************************************
class CPathCommand: public CConsoleCommandContainer
{
public:
    CPathCommand()
    {
        m_sCommand = "path";
        Add(new CPathAutoCreateCommand());
        Add(new CPathAddTypeCommand());
        Add(new CPathArgumentCommand());
        Add(new CPathCreateCommand());
        Add(new CPathDrawCommand());
        Add(new CPathInfoCommand());
        //Add(new CPathSwapCommand());
        Add(new CPathRemoveCommand());
        Add(new CPathRemoveTypeCommand());
    }
};


//****************************************************************************************************************
// Container of all commands starting with "item".
//****************************************************************************************************************
class CItemCommand: public CConsoleCommandContainer
{
public:
    CItemCommand()
    {
        m_sCommand = "item";
        Add(new CItemDrawCommand());
        Add(new CItemDrawTypeCommand());
        Add(new CItemReloadCommand());
    }
};


//****************************************************************************************************************
// Container of all commands starting with "bot".
//****************************************************************************************************************
class CBotCommand: public CConsoleCommandContainer
{
public:
    CBotCommand()
    {
        m_sCommand = "bot";
        Add(new CBotAddCommand());
        Add(new CBotDebugCommand());
        Add(new CBotDrawPathCommand());
        Add(new CBotDefaultCommand());
        Add(new CBotKickCommand());
        Add(new CBotPauseCommand());
        Add(new CBotTestPathCommand());
        Add(new CBotWeaponCommand());
    }
};


//****************************************************************************************************************
// Container of all commands starting with "config".
//****************************************************************************************************************
class CConfigCommand: public CConsoleCommandContainer
{
public:
    CConfigCommand()
    {
        m_sCommand = "config";
        Add(new CConfigAdminsCommand());
        Add(new CConfigEventsCommand);
        Add(new CConfigLogCommand);
    }
};


//****************************************************************************************************************
// Commmand "version".
//****************************************************************************************************************
class CVersionCommand: public CConsoleCommand
{
public:
    CVersionCommand()
    {
        m_sCommand = "version";
        m_sHelp = "display plugin version";
        m_iAccessLevel = FCommandAccessConfig;
    }

    TCommandResult Execute( CClient* pClient, int /*argc*/, const char** /*argv*/ )
    {
        edict_t* pEdict = pClient ? pClient->GetEdict() : NULL;
        BULOG_I( pEdict, "Version " PLUGIN_VERSION );
        return ECommandPerformed;
    }
};


//****************************************************************************************************************
// Container of all commands starting with "botrix".
//****************************************************************************************************************
class CMainCommand: public CConsoleCommandContainer
{
public:
    CMainCommand();
    ~CMainCommand();

    static good::unique_ptr<CMainCommand> instance; ///< Singleton of this class.
};

#endif // __BOTRIX_CONSOLE_COMMANDS_H__
