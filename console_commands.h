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
    virtual void PrintCommand( edict_t* pPrintTo, int indent = 0);

#if defined(BOTRIX_NO_COMMAND_COMPLETION)
#elif defined(BOTRIX_OLD_COMMAND_COMPLETION)
    virtual int AutoComplete( const char* partial, int partialLength,
                              char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ],
                              int strIndex, int charIndex );
#else
    virtual int AutoComplete( char* partial, int partialLength, CUtlVector<CUtlString>& cCommands, int charIndex );
#endif

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

    void Add( CConsoleCommand* newCommand ) { m_aCommands.push_back( good::unique_ptr<CConsoleCommand>(newCommand) ); }
    virtual void PrintCommand( edict_t* pPrintTo, int indent = 0);

#if defined(BOTRIX_NO_COMMAND_COMPLETION)
#elif defined(BOTRIX_OLD_COMMAND_COMPLETION)
    virtual int AutoComplete( const char* partial, int partialLength,
                              char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ],
                              int strIndex, int charIndex );
#else
    virtual int AutoComplete( char* partial, int partialLength, CUtlVector< CUtlString > &commands, int charIndex );
#endif

protected:
    good::vector< good::unique_ptr<CConsoleCommand> > m_aCommands;
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
        m_sHelp = "delete current, destination or given waypoint";
        m_iAccessLevel = FCommandAccessWaypoint;

		m_bAutoCompleteOnlyOneArgument = true;
		m_cAutoCompleteArguments.push_back("current");
		m_cAutoCompleteArguments.push_back("destination");
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
		m_bAutoCompleteOnlyOneArgument = true;
		m_cAutoCompleteArguments.push_back(CTypeToString::BoolToString(true, BoolStringOnOff).duplicate());
		m_cAutoCompleteArguments.push_back(CTypeToString::BoolToString(false, BoolStringOnOff).duplicate());
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
        m_sHelp = "lock given or current waypoint as path 'destination'";
        m_sDescription = "Set to -1 to unlock.";
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

        for ( int i=0; i < EWaypointFlagTotal; ++i )
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
    CWaypointArgumentCommand();
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
class CPathDistanceCommand: public CConsoleCommand
{
public:
    CPathDistanceCommand()
    {
        m_sCommand = "distance";
        m_sHelp = "set distance to add default paths & auto add waypoints";
        m_iAccessLevel = FCommandAccessWaypoint;
    }
    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

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
		m_bAutoCompleteOnlyOneArgument = true;
		m_cAutoCompleteArguments.push_back(CTypeToString::BoolToString(true, BoolStringOnOff).duplicate());
		m_cAutoCompleteArguments.push_back(CTypeToString::BoolToString(false, BoolStringOnOff).duplicate());
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

        for ( int i=0; i < EPathFlagTotal; ++i )
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
        m_sHelp = "add bot";
        if ( CMod::aClassNames.size() )
            m_sDescription = "Optional parameters: <bot-name> <intelligence> <team> <class>. ";
        else
            m_sDescription = "Optional parameters: <bot-name> <intelligence> <team>. ";
        m_iAccessLevel = FCommandAccessBot;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CBotCommandCommand: public CConsoleCommand
{
public:
    CBotCommandCommand()
    {
        m_sCommand = "command";
        m_sHelp = "execute console command by bot";
        m_sDescription = "Parameters: <bot-name> <command>. Example: 'botrix bot command all kill'.";
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
        m_sHelp = "kick bot";
        m_sDescription = "Parameters: <empty/bot-name/all> will kick random/selected/all bots.";
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
        m_sHelp = "show bot debug messages on server";
        m_sDescription = "Parameters: <bot-name> <on/off>.";
        m_iAccessLevel = FCommandAccessBot;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CBotConfigQuotaCommand: public CConsoleCommand
{
public:
    CBotConfigQuotaCommand()
    {
        m_sCommand = "quota";
        m_sHelp = "set bots+players quota.";
        m_sDescription = "You can use 'n-m' to have m bots per n players. Set to 0 to disable quota.";
        m_iAccessLevel = FCommandAccessBot;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CBotConfigIntelligenceCommand: public CConsoleCommand
{
public:
    CBotConfigIntelligenceCommand()
    {
        m_sCommand = "intelligence";
        m_sHelp = "set min/max bot intelligence";
        m_sDescription = "Arguments: <min> <max>. Can be one of: random fool stupied normal smart pro";// TODO: intelligence flags to string.
        m_iAccessLevel = FCommandAccessBot;

		m_bAutoCompleteOnlyOneArgument = true;
        m_cAutoCompleteArguments.push_back("random");
        for ( int i=0; i < EBotIntelligenceTotal; ++i )
            m_cAutoCompleteArguments.push_back( CTypeToString::IntelligenceToString(i).duplicate() );
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CBotConfigTeamCommand: public CConsoleCommand
{
public:
    CBotConfigTeamCommand()
    {
        m_sCommand = "team";
        m_sHelp = "set default bot team";
        m_sDescription = good::string("Can be one of: ") + CTypeToString::TeamFlagsToString(-1);
        m_iAccessLevel = FCommandAccessBot;

        m_cAutoCompleteArguments.push_back("random");
        for ( int i=0; i < CMod::aTeamsNames.size(); ++i )
            m_cAutoCompleteArguments.push_back( CTypeToString::TeamToString(i).duplicate() );
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CBotConfigClassCommand: public CConsoleCommand
{
public:
    CBotConfigClassCommand()
    {
        m_sCommand = "class";
        m_sHelp = "set default bot class";
        m_sDescription = good::string("Can be one of: random ") + CTypeToString::ClassFlagsToString(-1);
        m_iAccessLevel = FCommandAccessBot;

        m_cAutoCompleteArguments.push_back("random");
        for ( int i=0; i < CMod::aClassNames.size(); ++i )
            m_cAutoCompleteArguments.push_back( CTypeToString::ClassToString(i).duplicate() );
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CBotConfigChangeClassCommand: public CConsoleCommand
{
public:
    CBotConfigChangeClassCommand()
    {
        m_sCommand = "change-class";
        m_sHelp = "change bot class to another random class after x rounds.";
        m_sDescription = "Set to 0 to disable.";
        m_iAccessLevel = FCommandAccessBot;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CBotConfigSuicideCommand: public CConsoleCommand
{
public:
    CBotConfigSuicideCommand()
    {
        m_sCommand = "suicide";
        m_sHelp = "when staying far from waypoints for this time (in seconds), suicide";
        m_sDescription = "Set to 0 to disable.";
        m_iAccessLevel = FCommandAccessBot;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CBotConfigStrategyFlagsCommand: public CConsoleCommand
{
public:
    CBotConfigStrategyFlagsCommand()
    {
        m_sCommand = "flags";
        m_sHelp = "set bot fight strategy flags";
        m_sDescription = good::string("Can be mix of: ") + CTypeToString::StrategyFlagsToString(FFightStrategyAll);
        m_iAccessLevel = FCommandAccessBot;

        for ( int i=0; i < EFightStrategyFlagTotal; ++i )
            m_cAutoCompleteArguments.push_back( CTypeToString::StrategyFlagsToString(1<<i).duplicate() );
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CBotConfigStrategySetCommand: public CConsoleCommand
{
public:
    CBotConfigStrategySetCommand()
    {
        m_sCommand = "set";
        m_sHelp = "set bot fight strategy argument";
        m_sDescription = good::string("Can be one of: ") + CTypeToString::StrategyArgs();
        m_iAccessLevel = FCommandAccessBot;

		m_bAutoCompleteOnlyOneArgument = true;
        for ( int i=0; i < EFightStrategyArgTotal; ++i )
            m_cAutoCompleteArguments.push_back( CTypeToString::StrategyArgToString(i).duplicate() );
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CBotConfigStrategyCommand: public CConsoleCommandContainer
{
public:
    CBotConfigStrategyCommand()
    {
        m_sCommand = "strategy";
        Add(new CBotConfigStrategyFlagsCommand);
        Add(new CBotConfigStrategySetCommand);
    }
};

class CBotConfigCommand: public CConsoleCommandContainer
{
public:
    CBotConfigCommand()
    {
        m_sCommand = "config";
        Add(new CBotConfigQuotaCommand);
        Add(new CBotConfigIntelligenceCommand);
        Add(new CBotConfigTeamCommand);
        if ( CMod::aClassNames.size() )
        {
            Add(new CBotConfigClassCommand);
            Add(new CBotConfigChangeClassCommand);
        }
        Add(new CBotConfigSuicideCommand);
        Add(new CBotConfigStrategyCommand);
    }
};

class CBotAllyCommand: public CConsoleCommand
{
public:
    CBotAllyCommand()
    {
        m_sCommand = "ally";
        m_sHelp = "will not attack another user/bot";
        m_sDescription = "";
        m_iAccessLevel = FCommandAccessBot;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CBotAttackCommand: public CConsoleCommand
{
public:
    CBotAttackCommand()
    {
        m_sCommand = "attack";
        m_sHelp = "forces bot to start/stop attacking";
        m_iAccessLevel = FCommandAccessBot;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CBotMoveCommand: public CConsoleCommand
{
public:
    CBotMoveCommand()
    {
        m_sCommand = "move";
        m_sHelp = "forces bot to start/stop moving";
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

class CBotWeaponCommand: public CConsoleCommandContainer
{
public:
    CBotWeaponCommand()
    {
        m_sCommand = "weapon";
        Add(new CBotWeaponAddCommand);
        Add(new CBotWeaponAllowCommand);
        Add(new CBotWeaponForbidCommand);
        //Add(new CBotWeaponRemoveCommand);
        Add(new CBotWeaponUnknownCommand);
    }
};

class CBotDrawPathCommand: public CConsoleCommand
{
public:
	CBotDrawPathCommand();
    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CBotTestPathCommand: public CConsoleCommand
{
public:
    CBotTestPathCommand()
    {
        m_sCommand = "test";
        m_sHelp = "create bot to test path from given (or current) to given (or destination) waypoints";
        m_iAccessLevel = FCommandAccessBot;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};


//****************************************************************************************************************
// Item commands.
//****************************************************************************************************************
class CItemDrawCommand: public CConsoleCommand
{
public:
	CItemDrawCommand();
    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};


class CItemDrawTypeCommand: public CConsoleCommand
{
public:
	CItemDrawTypeCommand();
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
		CItems::MapLoaded(true);
        return ECommandPerformed;
    }
};


//****************************************************************************************************************
// Config commands.
//****************************************************************************************************************
class CConfigAdminsSetAccessCommand: public CConsoleCommand
{
public:
	CConfigAdminsSetAccessCommand();
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
		m_bAutoCompleteOnlyOneArgument = true;
		m_cAutoCompleteArguments.push_back(CTypeToString::BoolToString(false, BoolStringOnOff));
		m_cAutoCompleteArguments.push_back(CTypeToString::BoolToString(true, BoolStringOnOff));
	}

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CConfigLogCommand: public CConsoleCommand
{
public:
	CConfigLogCommand();
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
        Add(new CConfigAdminsSetAccessCommand);
        Add(new CConfigAdminsShowCommand);
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
        Add(new CWaypointAreaRemoveCommand);
        Add(new CWaypointAreaRenameCommand);
        Add(new CWaypointAreaSetCommand);
        Add(new CWaypointAreaShowCommand);
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
        Add(new CWaypointAddTypeCommand);
        Add(new CWaypointAreaCommand);
        Add(new CWaypointArgumentCommand);
        Add(new CWaypointAutoCreateCommand);
        Add(new CWaypointClearCommand);
        Add(new CWaypointCreateCommand);
        Add(new CWaypointDestinationCommand);
        Add(new CWaypointDrawFlagCommand);
        Add(new CWaypointInfoCommand);
        Add(new CWaypointLoadCommand);
        Add(new CWaypointMoveCommand);
        Add(new CWaypointRemoveCommand);
        Add(new CWaypointRemoveTypeCommand);
        Add(new CWaypointResetCommand);
        Add(new CWaypointSaveCommand);
        Add(new CWaypointVisibilityCommand);
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
        Add(new CPathAutoCreateCommand);
        Add(new CPathAddTypeCommand);
        Add(new CPathArgumentCommand);
        Add(new CPathCreateCommand);
        Add(new CPathDistanceCommand);
        Add(new CPathDrawCommand);
        Add(new CPathInfoCommand);
        //Add(new CPathSwapCommand);
        Add(new CPathRemoveCommand);
        Add(new CPathRemoveTypeCommand);
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
        Add(new CItemDrawCommand);
        Add(new CItemDrawTypeCommand);
        Add(new CItemReloadCommand);
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
        Add(new CBotAddCommand);
        Add(new CBotAllyCommand);
        Add(new CBotAttackCommand);
        Add(new CBotCommandCommand);
        Add(new CBotConfigCommand);
        Add(new CBotDebugCommand);
        Add(new CBotDrawPathCommand);
        Add(new CBotKickCommand);
        Add(new CBotMoveCommand);
        Add(new CBotPauseCommand);
        if ( CMod::GetModId() != EModId_TF2 ) // TF2 bots can't be spawned after round has started.
            Add(new CBotTestPathCommand);
        Add(new CBotWeaponCommand);
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
        Add(new CConfigAdminsCommand);
        Add(new CConfigEventsCommand);
        Add(new CConfigLogCommand);
    }
};


//****************************************************************************************************************
// Command "enable".
//****************************************************************************************************************
class CEnableCommand: public CConsoleCommand
{
public:
    CEnableCommand()
    {
        m_sCommand = "enable";
        m_sHelp = "enable plugin";
        m_iAccessLevel = FCommandAccessConfig;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** /*argv*/ )
    {
        edict_t* pEdict = pClient ? pClient->GetEdict() : NULL;
        if ( argc )
        {
            BULOG_W( pEdict, "Error, invalid arguments count." );
            return ECommandError;
        }
        else
        {
            CBotrixPlugin::instance->Enable(true);
            BULOG_I( pEdict, "Plugin enabled." );
            return ECommandPerformed;
        }
    }
};


// Command "disable".
class CDisableCommand: public CConsoleCommand
{
public:
    CDisableCommand()
    {
        m_sCommand = "disable";
        m_sHelp = "disable plugin";
        m_iAccessLevel = FCommandAccessConfig;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** /*argv*/ )
    {
        edict_t* pEdict = pClient ? pClient->GetEdict() : NULL;
        if ( argc )
        {
            BULOG_W( pEdict, "Error, invalid arguments count." );
            return ECommandError;
        }
        else
        {
            CBotrixPlugin::instance->Enable(false);
            BULOG_I( pEdict, "Plugin disabled." );
            return ECommandPerformed;
        }
    }
};


//****************************************************************************************************************
// Command "version".
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
/// Container of all commands starting with "botrix".
//****************************************************************************************************************
class CBotrixCommand: public CConsoleCommandContainer
#if !defined(BOTRIX_NO_COMMAND_COMPLETION) && !defined(BOTRIX_OLD_COMMAND_COMPLETION)
    , public ICommandCompletionCallback, public ICommandCallback
#endif
{
public:
    /// Contructor.
    CBotrixCommand();

    /// Destructor.
    ~CBotrixCommand();

#if !defined(BOTRIX_NO_COMMAND_COMPLETION) && !defined(BOTRIX_OLD_COMMAND_COMPLETION)
    /// Execute "botrix" command on server (not as client).
    virtual void CommandCallback( const CCommand &command );

    /// Autocomplete "botrix" command on server (not as client).
    virtual int CommandCompletionCallback( const char *pPartial, CUtlVector< CUtlString > &commands )
    {
        good::string sPartial(pPartial, true, true);
        return AutoComplete((char*)sPartial.c_str(), sPartial.size(), commands, 0);
    }
#endif

    static good::unique_ptr<CBotrixCommand> instance; ///< Singleton of this class.

protected:
    ConCommand m_cServerCommand;
};


#endif // __BOTRIX_CONSOLE_COMMANDS_H__
