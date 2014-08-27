#ifndef __BOTRIX_TYPE_TO_STRING_H__
#define __BOTRIX_TYPE_TO_STRING_H__


#include "types.h"


//****************************************************************************************************************
/// Useful class to get type from string and viceversa.
//****************************************************************************************************************
class CTypeToString
{
public:

    //------------------------------------------------------------------------------------------------------------
    /// Get string from vector of strings.
    //------------------------------------------------------------------------------------------------------------
    static const good::string& StringVectorToString( const StringVector& aStrings );


    //------------------------------------------------------------------------------------------------------------
    /// Get bool from string (on/off). -1 = invalid string.
    //------------------------------------------------------------------------------------------------------------
    static int BoolFromString( const good::string& sBool );

    /// Get string from bool.
    static const good::string& BoolToString( bool b );


    //------------------------------------------------------------------------------------------------------------
    /// Get mod id from string.
    //------------------------------------------------------------------------------------------------------------
    static TModId ModFromString( const good::string& sMod );

    /// Get string from mod id.
    static const good::string& ModToString( TModId iMod );


    //------------------------------------------------------------------------------------------------------------
    /// Get access flags from string.
    //------------------------------------------------------------------------------------------------------------
    static int AccessFlagsFromString( const good::string& sFlags );

    /// Get string from access flags.
    static const good::string& AccessFlagsToString( TCommandAccessFlags iFlags );


    //------------------------------------------------------------------------------------------------------------
    /// Get waypoint flags from string.
    //------------------------------------------------------------------------------------------------------------
    static int WaypointFlagsFromString( const good::string& sFlags );

    /// Get string from waypoint flags.
    static const good::string& WaypointFlagsToString( TWaypointFlags iFlags );


    //------------------------------------------------------------------------------------------------------------
    /// Get path flags from string.
    //------------------------------------------------------------------------------------------------------------
    static int PathFlagsFromString( const good::string& sFlags );

    /// Get string from path flags.
    static const good::string& PathFlagsToString( TPathFlags iFlags );


    //------------------------------------------------------------------------------------------------------------
    /// Get waypoint draw types from string.
    //------------------------------------------------------------------------------------------------------------
    static int WaypointDrawFlagsFromString( const good::string& sFlags );

    /// Get string from waypoint draw types.
    static const good::string& WaypointDrawFlagsToString( TWaypointDrawFlags iFlags );


    //------------------------------------------------------------------------------------------------------------
    /// Get path draw types from string.
    //------------------------------------------------------------------------------------------------------------
    static int PathDrawFlagsFromString( const good::string& sFlags );

    /// Get string from waypoint draw flags.
    static const good::string& PathDrawFlagsToString( TPathDrawFlags iFlags );


    //------------------------------------------------------------------------------------------------------------
    /// Get item type from string.
    //------------------------------------------------------------------------------------------------------------
    static int EntityTypeFromString( const good::string& sType );

    /// Get string from item type.
    static const good::string& EntityTypeToString( TEntityType iType );


    //------------------------------------------------------------------------------------------------------------
    /// Get item type flag from string. Used to draw entities on map.
    //------------------------------------------------------------------------------------------------------------
    static int EntityTypeFlagsFromString( const good::string& sFlag );

    /// Get string from item type flags.
    static const good::string& EntityTypeFlagsToString( TEntityTypeFlags iItemTypeFlags );


    //------------------------------------------------------------------------------------------------------------
    /// Get item flags from string. Used to draw entities on map.
    //------------------------------------------------------------------------------------------------------------
    static int EntityClassFlagsFromString( const good::string& sFlags );

    /// Get string from item flags.
    static const good::string& EntityClassFlagsToString( TEntityFlags iItemFlags );


    //------------------------------------------------------------------------------------------------------------
    /// Get item draw flags from string.
    //------------------------------------------------------------------------------------------------------------
    static int ItemDrawFlagsFromString( const good::string& sFlags );

    /// Get string from item draw flags.
    static const good::string& ItemDrawFlagsToString( TItemDrawFlags iFlags );


    //------------------------------------------------------------------------------------------------------------
    /// Get weapon flag from string.
    //------------------------------------------------------------------------------------------------------------
    static int WeaponTypeFromString( const good::string& sType );

    /// Get string from weapon flags.
    static const good::string& WeaponTypeToString( TWeaponType iType );


    //------------------------------------------------------------------------------------------------------------
    /// Get bot preference from string.
    //------------------------------------------------------------------------------------------------------------
    static int PreferenceFromString( const good::string& sPreference );

    /// Get string from bot preference .
    static const good::string& PreferenceToString( TBotIntelligence iPreference );


    //------------------------------------------------------------------------------------------------------------
    /// Get string from bot intelligence.
    //------------------------------------------------------------------------------------------------------------
    static const good::string& IntelligenceToString( int iIntelligence );

    /// Get bot intelligence from string.
    static int IntelligenceFromString( const good::string& sIntelligence );


    //------------------------------------------------------------------------------------------------------------
    /// Get string from bot team.
    //------------------------------------------------------------------------------------------------------------
    static const good::string& TeamToString( int iTeam );

    /// Get bot class from string.
    static const good::string& TeamFlagsToString( int iTeams );

    /// Get bot class from string.
    static int TeamFromString( const good::string& sTeam );


    //------------------------------------------------------------------------------------------------------------
    /// Get string from bot class.
    //------------------------------------------------------------------------------------------------------------
    static const good::string& ClassToString( int iClass );

    /// Get bot class from string.
    static const good::string& ClassFlagsToString( int iClasses );

    /// Get bot class from string.
    static int ClassFromString( const good::string& sClass );


    //------------------------------------------------------------------------------------------------------------
    /// Get bot task name.
    //------------------------------------------------------------------------------------------------------------
    static const good::string& BotTaskToString( TBotTask iBotTask );

    //------------------------------------------------------------------------------------------------------------
    /// Get bot command from string.
    //------------------------------------------------------------------------------------------------------------
    static int BotCommandFromString( const good::string& sCommand );

    /// Get string from bot command.
    static const good::string& BotCommandToString( TBotChat iCommand );


    //------------------------------------------------------------------------------------------------------------
    /// Get log level from string.
    //------------------------------------------------------------------------------------------------------------
    static int LogLevelFromString( const good::string& sAction );

    /// Get string from log level.
    static const good::string& LogLevelToString( int iLevel );

#ifdef BOTRIX_BORZH
    //------------------------------------------------------------------------------------------------------------
    /// Get bot action from string.
    //------------------------------------------------------------------------------------------------------------
    static int BotActionFromString( const good::string& sAction );

    /// Get string from bot action.
    static const good::string& BotActionToString( int iAction );

    //------------------------------------------------------------------------------------------------------------
    /// Get string from borzh's task.
    //------------------------------------------------------------------------------------------------------------
    static const good::string& BorzhTaskToString( int iTask );
#endif

};

#endif // __BOTRIX_TYPE_TO_STRING_H__
