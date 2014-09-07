#include <good/string_buffer.h>
#include <good/string_utils.h>

#include "bot.h"
#include "clients.h"
#include "config.h"
#include "console_commands.h"
#include "waypoint.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


good::unique_ptr<CMainCommand> CMainCommand::instance;

const good::string sHelp("help"); // TODO: all commands.

const good::string sAll("all");
const good::string sNone("none");
const good::string sNext("next");

const good::string sWeapon = "weapon";
const good::string sAmmo = "ammo";
const good::string sHealth = "health";
const good::string sArmor = "armor";
const good::string sButton = "button";
const good::string sFirstAngle = "first_angle";
const good::string sSecondAngle = "second_angle";

extern char* szMainBuffer;
extern int iMainBufferSize;

//----------------------------------------------------------------------------------------------------------------
// Singleton to access console variables.
//----------------------------------------------------------------------------------------------------------------
//CPluginConVarAccessor CPluginConVarAccessor::instance;
//
//bool CPluginConVarAccessor::RegisterConCommandBase( ConCommandBase *pCommand )
//{
//	// Link to engine's list.
//	CBotrixPlugin::pCvar->RegisterConCommand( pCommand );
//	return true;
//}


//----------------------------------------------------------------------------------------------------------------
// CConsoleCommand.
//----------------------------------------------------------------------------------------------------------------
int CConsoleCommand::AutoComplete( const char* partial, int partialLength, char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ], int strIndex, int charIndex )
{
    if (charIndex + partialLength >= COMMAND_COMPLETION_ITEM_LENGTH ||
        strIndex >= COMMAND_COMPLETION_ITEM_LENGTH-1)
        return 0; // Check bounds.

    int result = 0;
    int maxLength = COMMAND_COMPLETION_ITEM_LENGTH - charIndex - 1; // Save one space for trailing 0.

    if ( partialLength <= m_sCommand.size() )
    {
        if ( strncmp( m_sCommand.c_str(), partial, partialLength ) == 0 )
        {
            // Autocomplete only command name.
            strncpy( &commands[strIndex][charIndex], m_sCommand.c_str(), MIN2(maxLength, m_sCommand.size()+1) );
            commands[strIndex+result][COMMAND_COMPLETION_ITEM_LENGTH-1] = 0;
            result++;
        }
    }
    else
    {
        if ( m_cAutoCompleteArguments.size() > 0 &&
             strncmp( m_sCommand.c_str(), partial, m_sCommand.size() ) == 0 )
        {
            // Autocomplete command name with arguments.
            good::string part(partial, false, false, partialLength);

            int start = m_sCommand.size();
            if ( part[start] == ' ' )
            {
                int lastSpace = part.rfind(' ');
                if ( lastSpace != good::string::npos )
                {
                    if ( !m_bAutoCompleteOnlyOneArgument || (lastSpace == start) )
                    {
                        lastSpace++;
                        good::string partArg(&partial[lastSpace], false, false, partialLength - lastSpace);

                        maxLength = COMMAND_COMPLETION_ITEM_LENGTH - (charIndex + lastSpace) - 1; // Save one space for trailing 0.
                        if ( maxLength > 0 ) // There is still space in autocomplete field.
                        {
                            for ( int i = 0; i < m_cAutoCompleteArguments.size(); ++i )
                            {
                                const good::string& arg = m_cAutoCompleteArguments[i];
                                if ( good::starts_with(arg, partArg) )
                                {
                                    strncpy( &commands[strIndex+result][charIndex], partial, lastSpace );
                                    strncpy( &commands[strIndex+result][charIndex+lastSpace], arg.c_str(), MIN2(maxLength, arg.size()+1) );
                                    commands[strIndex+result][COMMAND_COMPLETION_ITEM_LENGTH-1] = 0;
                                    result++;
                                    if ( strIndex+result >= COMMAND_COMPLETION_ITEM_LENGTH-1 )
                                        return result; // Bound check.
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return result;
}


void CConsoleCommand::PrintCommand( edict_t* pPrintTo, int indent )
{
    bool bHasAccess = true;

    if ( pPrintTo )
    {
        CPlayer* pPlayer = CPlayers::Get( pPrintTo );
        BASSERT( pPlayer && !pPlayer->IsBot(), return );
        CClient* pClient = (CClient*)pPlayer;
        bHasAccess = HasAccess(pClient);
    }

    int i;
    for ( i = 0; i < indent*2; i ++ )
        szMainBuffer[i] = ' ';
    szMainBuffer[i]=0;

    const char* sCantUse = bHasAccess ? "" : "[can't use]";
    BULOG_I( pPrintTo, "%s[%s]%s: %s", szMainBuffer, m_sCommand.c_str(), sCantUse, m_sHelp.c_str() );
    if ( m_sDescription.length() > 0 )
        BULOG_I( pPrintTo, "%s  %s", szMainBuffer, m_sDescription.c_str() );
}



//----------------------------------------------------------------------------------------------------------------
// CConsoleCommandContainer.
//----------------------------------------------------------------------------------------------------------------
int CConsoleCommandContainer::AutoComplete( const char* partial, int partialLength, char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ], int strIndex, int charIndex )
{
    int result = 0;
    int command_size = m_sCommand.size();

    if ( command_size >= partialLength ) // only Add command to commands array
    {
        if ( strncmp( m_sCommand.c_str(), partial, partialLength ) == 0 )
        {
            strcpy( &commands[strIndex][charIndex], m_sCommand.c_str() ); // e.g. "way" -> "waypoint"
            result++;
        }
    }
    else
    {
        if ( strncmp( m_sCommand.c_str(), partial, command_size ) == 0 )
        {
            partial += command_size;
            partialLength -= command_size;
            while ( partial[0] == ' ' )
            {
                partial++; // remove root command from partial command(e.g. "botrix way" -> "way")
                partialLength--;
            }

            int charIdx = charIndex + command_size + 1; // 1 is for space

            for ( int i = 0; i < m_commands.size(); i ++ )
            {
                int count = m_commands[i]->AutoComplete(partial, partialLength, commands, strIndex, charIdx);
                for ( int j = 0; j < count; j ++ )
                {
                    strncpy(&commands[strIndex][charIndex], m_sCommand.c_str(), command_size);
                    commands[strIndex][charIndex+command_size] = ' ';
                    strIndex ++;
                    result ++;
                }
            }
        }
    }

    return result;
}

TCommandResult CConsoleCommandContainer::Execute( CClient* pClient, int argc, const char** argv )
{
    if ( argc > 0 )
    {
        for ( int i = 0; i < m_commands.size(); i ++ )
        {
            CConsoleCommand *pCommand = m_commands[i].get();

            if ( pCommand->IsCommand(argv[0]) )
            {
                if ( pCommand->HasAccess( pClient ) )
                    return pCommand->Execute( pClient, argc-1, &argv[1] );
                else
                    return ECommandRequireAccess;
            }
        }
    }

    PrintCommand( pClient ? pClient->GetEdict() : NULL );
    return ECommandNotFound;
}

void CConsoleCommandContainer::PrintCommand( edict_t* pPrintTo, int indent )
{
    int i;
    for ( i = 0; i < indent*2; i ++ )
        szMainBuffer[i] = ' ';
    szMainBuffer[i]=0;

    BULOG_I( pPrintTo, "%s[%s]", szMainBuffer, m_sCommand.c_str() );
    for ( int i = 0; i < m_commands.size(); i ++ )
        m_commands[i]->PrintCommand( pPrintTo, indent+1 );
}



//----------------------------------------------------------------------------------------------------------------
// Waypoints commands.
//----------------------------------------------------------------------------------------------------------------
CWaypointDrawFlagCommand::CWaypointDrawFlagCommand()
{
    m_sCommand = "drawtype";
    m_sHelp = "defines how to draw waypoint";
    m_sDescription = good::string("Can be 'none' / 'all' / 'next' or mix of: ") + CTypeToString::WaypointDrawFlagsToString(FWaypointDrawAll);
    m_iAccessLevel = FCommandAccessWaypoint;

    m_cAutoCompleteArguments.push_back(sNone);
    for (int i=0; i < EWaypointDrawFlagTotal; ++i)
        m_cAutoCompleteArguments.push_back( CTypeToString::WaypointDrawFlagsToString(1<<i).duplicate() );
    m_cAutoCompleteArguments.push_back(sAll);
}

TCommandResult CWaypointDrawFlagCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    if ( pClient == NULL )
    {
        BLOG_W( "Please login to server to execute this command." );
        return ECommandError;
    }

    if ( argc == 0 )
    {
        const good::string& sType = CTypeToString::WaypointDrawFlagsToString(pClient->iWaypointDrawFlags);
        BULOG_I( pClient->GetEdict(), "Waypoint draw flags: %s.", (sType.size() != 0) ? sType.c_str() : sNone.c_str() );
        return ECommandPerformed;
    }

    // Retrieve flags from string arguments.
    bool bFinished = false;
    TWaypointDrawFlags iFlags = FWaypointDrawNone;

    if ( argc == 1 )
    {
        if ( sNone == argv[0] )
            bFinished = true;
        else if ( sAll == argv[0] )
        {
            iFlags = FWaypointDrawAll;
            bFinished = true;
        }
        else if ( sNext == argv[0] )
        {
            int iNew = (pClient->iWaypointDrawFlags)  ?  pClient->iWaypointDrawFlags<< 1  :  1;
            iFlags = (iNew > FWaypointDrawAll) ? 0 : iNew;
            bFinished = true;
        }
    }

    if ( !bFinished )
    {
        for ( int i=0; i < argc; ++i )
        {
            int iAddFlag = CTypeToString::WaypointDrawFlagsFromString(argv[i]);
            if ( iAddFlag == -1 )
            {
                BULOG_E( pClient->GetEdict(), "Error, invalid draw flag(s). Can be 'none' / 'all' / 'next' or mix of: %s", CTypeToString::WaypointDrawFlagsToString(FWaypointDrawAll).c_str() );
                return ECommandError;
            }
            FLAG_SET(iAddFlag, iFlags);
        }
    }

    pClient->iWaypointDrawFlags = iFlags;
    BULOG_I(pClient->GetEdict(), "Waypoints drawing is %s.", iFlags ? "on" : "off");
    return ECommandPerformed;
}

TCommandResult CWaypointResetCommand::Execute( CClient* pClient, int /*argc*/, const char** /*argv*/ )
{
    if ( pClient == NULL )
    {
        BLOG_W( "Please login to server to execute this command." );
        return ECommandError;
    }

    Vector vOrigin( pClient->GetHead() );
    pClient->iCurrentWaypoint = CWaypoints::GetNearestWaypoint( vOrigin );
    BULOG_I(pClient->GetEdict(), "Current waypoint %d.", pClient->iCurrentWaypoint);

    return ECommandPerformed;
}

TCommandResult CWaypointCreateCommand::Execute( CClient* pClient, int /*argc*/, const char** /*argv*/ )
{
    if ( pClient == NULL )
    {
        BLOG_W( "Please login to server to execute this command." );
        return ECommandError;
    }

    if ( !pClient->IsAlive() )
    {
        BULOG_W(pClient->GetEdict(), "Error, you need to be alive to create waypoints (bots can't just fly around you know).");
        return ECommandError;
    }

    TWaypointId id = CWaypoints::Add( pClient->GetHead(), FWaypointNone );
    pClient->iCurrentWaypoint = id;

    // Check if player is crouched.
    float fHeight = pClient->GetPlayerInfo()->GetPlayerMaxs().z - pClient->GetPlayerInfo()->GetPlayerMins().z + 1;
    bool bIsCrouched = ( fHeight < CMod::iPlayerHeight );

    if (pClient->bAutoCreatePaths)
        CWaypoints::CreateAutoPaths(id, bIsCrouched);
    else if ( CWaypoint::IsValid( pClient->iDestinationWaypoint ) )
        CWaypoints::CreatePathsWithAutoFlags( pClient->iDestinationWaypoint, pClient->iCurrentWaypoint, bIsCrouched );

    BULOG_I(pClient->GetEdict(), "Waypoint %d added.", id);
    return ECommandPerformed;
}

TCommandResult CWaypointRemoveCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    if ( pClient == NULL )
    {
        BLOG_W( "Please login to server to execute this command." );
        return ECommandError;
    }

    TWaypointId id = -1;
    if (argc == 0)
        id = pClient->iCurrentWaypoint;
    else if (argc == 1)
        sscanf(argv[0], "%d", &id);

    if ( !CWaypoints::IsValid(id) )
    {
        BULOG_W(pClient->GetEdict(), "Error, invalid given or current waypoint (move closer to some waypoint).");
        return ECommandError;
    }

    CWaypoints::Remove(id);
    BULOG_I(pClient->GetEdict(), "Waypoint %d deleted.", id);

    // Invalidate current / destination waypoints for all players.
    for (int i=0; i < CPlayers::Size(); ++i)
    {
        CPlayer* pPlayer = CPlayers::Get(i);
        if ( pPlayer )
        {
            if ( pPlayer->IsBot() )
            {
                pPlayer->iCurrentWaypoint = -1; // Force bot move failure, because path can contain removed waypoint.
                pPlayer->iNextWaypoint = -1;
            }
            else
            {
                CClient* pClient = (CClient*)pPlayer;
                if ( pClient->iCurrentWaypoint == id )
                    pClient->iCurrentWaypoint = -1;
                else if ( pClient->iCurrentWaypoint > id )
                    pClient->iCurrentWaypoint--;
                if ( pClient->iDestinationWaypoint == id )
                    pClient->iDestinationWaypoint = -1;
                else if ( pClient->iDestinationWaypoint > id )
                    pClient->iDestinationWaypoint--;
            }
        }
    }

    return ECommandPerformed;
}

TCommandResult CWaypointMoveCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    if ( pClient == NULL )
    {
        BLOG_W( "Please login to server to execute this command." );
        return ECommandError;
    }

    TWaypointId id = -1;
    if (argc == 0)
        id = pClient->iDestinationWaypoint;
    else if ( (argc == 1) || (argc == 4) )
        sscanf(argv[0], "%d", &id);
    else if ( argc != 3 )
    {
        BULOG_W(pClient->GetEdict(), "Error, invalid arguments count (waypoint id & point x/y/z).");
        return ECommandError;
    }

    Vector vOrigin( pClient->GetHead() );
    if ( argc >= 3 )
    {
        int iValue = 0;
        sscanf(argv[argc - 3], "%d", &iValue); vOrigin.x = iValue;
        sscanf(argv[argc - 2], "%d", &iValue); vOrigin.y = iValue;
        sscanf(argv[argc - 1], "%d", &iValue); vOrigin.z = iValue;
    }

    if ( !CWaypoints::IsValid(id) )
    {
        BULOG_W(pClient->GetEdict(), "Error, invalid given or destination waypoint.");
        return ECommandError;
    }

    CWaypoints::Get(id).vOrigin = vOrigin;
    BULOG_I(pClient->GetEdict(), "Set new position for waypoint %d (%d, %d, %d).", id, (int)vOrigin.x, (int)vOrigin.y, (int)vOrigin.z);

    return ECommandPerformed;
}

TCommandResult CWaypointAutoCreateCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    if ( pClient == NULL )
    {
        BLOG_W( "Please login to server to execute this command." );
        return ECommandError;
    }

    if ( argc == 0 )
    {
        BULOG_I( pClient->GetEdict(), pClient->bAutoCreateWaypoints ? "Auto create waypoints is on." : "Auto create waypoints is off." );
        return ECommandPerformed;
    }

    int iValue = -1;
    if ( argc == 1 )
        iValue = CTypeToString::BoolFromString(argv[0]);

    if ( iValue == -1 )
    {
        BULOG_W(pClient->GetEdict(), "Error, invalid argument (must be 'on' or 'off').");
        return ECommandError;
    }

    pClient->bAutoCreateWaypoints = iValue != 0;
    BULOG_I(pClient->GetEdict(), iValue ? "Auto create waypoints is on." : "Auto create waypoints is off.");
    return ECommandPerformed;
}


TCommandResult CWaypointClearCommand::Execute( CClient* pClient, int /*argc*/, const char** /*argv*/ )
{
    if ( pClient == NULL )
    {
        BLOG_W( "Please login to server to execute this command." );
        return ECommandError;
    }

    // Invalidate current / next / destination waypoints for all players.
    for ( int i = 0; i < CPlayers::Size(); ++i )
    {
        CPlayer* pPlayer = CPlayers::Get(i);
        if ( pPlayer )
        {
            pPlayer->iCurrentWaypoint = pPlayer->iNextWaypoint = -1;
            if ( !pPlayer->IsBot() )
                ((CClient*)pPlayer)->iDestinationWaypoint = -1;
        }
    }

    CWaypoints::Clear();
    BULOG_I(pClient->GetEdict(), "All waypoints deleted.");
    return ECommandPerformed;
}

TCommandResult CWaypointAddTypeCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    if ( pClient == NULL )
    {
        BLOG_W( "Please login to server to execute this command." );
        return ECommandError;
    }

    if ( !CWaypoint::IsValid(pClient->iCurrentWaypoint) )
    {
        BULOG_W(pClient->GetEdict(), "Error, no waypoint nearby to add type (move closer to waypoint).");
        return ECommandError;
    }

    // Retrieve flags from string arguments.
    TWaypointFlags iFlags = FWaypointNone;
    for ( int i=0; i < argc; ++i )
    {
        int iAddFlag = CTypeToString::WaypointFlagsFromString(argv[i]);
        if ( iAddFlag == -1 )
        {
            BULOG_E( pClient->GetEdict(), "Error, invalid waypoint flag: %s. Can be one of: %s", argv[i], CTypeToString::WaypointFlagsToString(FWaypointAll).c_str() );
            return ECommandError;
        }
        FLAG_SET(iAddFlag, iFlags);
    }

    if ( iFlags == FWaypointNone )
    {
        BULOG_E(pClient->GetEdict(), "Error, specify at least one waypoint type. Can be one of: %s.", CTypeToString::WaypointFlagsToString(FWaypointAll).c_str());
        return ECommandError;
    }
    else
    {
        CWaypoint& w = CWaypoints::Get(pClient->iCurrentWaypoint);

        bool bAngle1 = FLAG_SOME_SET(FWaypointCamper | FWaypointSniper | FWaypointArmorMachine | FWaypointHealthMachine | FWaypointButton, w.iFlags);
        bool bAngle2 = FLAG_SOME_SET(FWaypointCamper | FWaypointSniper, w.iFlags);

        bool bWeapon = FLAG_SOME_SET(FWaypointAmmo | FWaypointWeapon, w.iFlags);

        bool bArmor = FLAG_SOME_SET(FWaypointArmor, w.iFlags);
        bool bHealth = FLAG_SOME_SET(FWaypointHealth, w.iFlags);

        if ( (bAngle1 && bWeapon) || ( bAngle2 && (bWeapon || bArmor || bHealth) ) )
        {
            BULOG_W(pClient->GetEdict(), "Error, you can't mix these waypoint types.");
            return ECommandError;
        }

        FLAG_SET(iFlags, w.iFlags);
        BULOG_I(pClient->GetEdict(), "Types %s (%d) added to waypoint %d.", CTypeToString::WaypointFlagsToString(iFlags).c_str(), iFlags, pClient->iCurrentWaypoint);
        return ECommandPerformed;
    }
}

TCommandResult CWaypointRemoveTypeCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    if ( pClient == NULL )
    {
        BLOG_W( "Please login to server to execute this command." );
        return ECommandError;
    }

    TWaypointId id = -1;
    if (argc == 0)
        id = pClient->iCurrentWaypoint;
    else if (argc == 1)
        sscanf(argv[0], "%d", &id);

    if ( CWaypoints::IsValid(id) )
    {
        CWaypoint& w = CWaypoints::Get(id);
        w.iFlags = FWaypointNone;
        BULOG_I(pClient->GetEdict(), "Waypoint %d has no type now.", id);
        return ECommandPerformed;
    }
    else
    {
        BULOG_W(pClient->GetEdict(), "Error, no waypoint nearby to remove type (move closer to waypoint).");
        return ECommandError;
    }
}

TCommandResult CWaypointArgumentCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    if ( pClient == NULL )
    {
        BLOG_W( "Please login to server to execute this command." );
        return ECommandError;
    }

    TWaypointId id = pClient->iCurrentWaypoint;
    if ( !CWaypoints::IsValid(id) )
    {
        BULOG_W(pClient->GetEdict(), "Error, no waypoint nearby (move closer to waypoint).");
        return ECommandError;
    }

    CWaypoint& w = CWaypoints::Get(pClient->iCurrentWaypoint);

    bool bAngle1 = FLAG_SOME_SET(FWaypointCamper | FWaypointSniper | FWaypointArmorMachine | FWaypointHealthMachine | FWaypointButton | FWaypointSeeButton, w.iFlags);
    bool bAngle2 = FLAG_SOME_SET(FWaypointCamper | FWaypointSniper, w.iFlags);

    bool bWeapon = FLAG_SOME_SET(FWaypointAmmo | FWaypointWeapon, w.iFlags);

    bool bArmor = FLAG_SOME_SET(FWaypointArmor, w.iFlags);
    bool bHealth = FLAG_SOME_SET(FWaypointHealth, w.iFlags);

    bool bButton = FLAG_SOME_SET(FWaypointButton | FWaypointSeeButton, w.iFlags);

    if ( argc == 0 )
    {
        if ( bWeapon )
            BULOG_I(pClient->GetEdict(), "Weapon index %d, subindex %d.", CWaypoint::GetWeaponIndex(w.iArgument), CWaypoint::GetWeaponSubIndex(w.iArgument));
        if ( FLAG_ALL_SET_OR_0(FWaypointAmmo, w.iFlags) )
        {
            bool bIsSecondary; int iAmmo = CWaypoint::GetAmmo(bIsSecondary, w.iArgument);
            BULOG_I(pClient->GetEdict(), "Weapon ammo %d, secondary %s.", iAmmo, bIsSecondary ? "yes" : "no");
        }
        if ( bHealth )
            BULOG_I(pClient->GetEdict(), "Health %d.", CWaypoint::GetHealth(w.iArgument));
        if ( bArmor )
            BULOG_I(pClient->GetEdict(), "Armor %d.", CWaypoint::GetArmor(w.iArgument));
        if ( bButton )
            BULOG_I(pClient->GetEdict(), "Button %d.", CWaypoint::GetButton(w.iArgument));
        if ( bAngle1 )
        {
            QAngle a1; CWaypoint::GetFirstAngle(a1, w.iArgument);
            BULOG_I(pClient->GetEdict(), "Angle 1 (pitch, yaw): (%.0f, %.0f).", a1.x, a1.y);
        }
        if ( bAngle2 )
        {
            QAngle a2; CWaypoint::GetSecondAngle(a2, w.iArgument);
            BULOG_I(pClient->GetEdict(), "Angle 2 (pitch, yaw): (%.0f, %.0f).", a2.x, a2.y);
        }
        return ECommandPerformed;
    }

    if ( sHelp == argv[0] )
    {
        BULOG_I(pClient->GetEdict(), "You can mix next arguments:");
        BULOG_I(pClient->GetEdict(), " - 'weapon' number1 number2: set weapon index/subindex (for ammo and/or weapon)");
        BULOG_I(pClient->GetEdict(), " - 'health' number: set health amount (also for health machine, 30 by default)");
        BULOG_I(pClient->GetEdict(), " - 'armor' number: set armor amount (also for armor machine, 30 by default)");
        BULOG_I(pClient->GetEdict(), " - 'first_angle': set your current angles as waypoint first angles (camper, sniper, machines)");
        BULOG_I(pClient->GetEdict(), " - 'second_angle': set your current angles as second angles (camper, sniper)");
        return ECommandPerformed;
    }

    int iArgument = w.iArgument;
    QAngle angClient;
    pClient->GetEyeAngles(angClient);
    CUtil::DeNormalizeAngle(angClient.x);
    CUtil::DeNormalizeAngle(angClient.y);
    BASSERT( -90.0f <= angClient.x && angClient.x <= 90.0f, return ECommandError );

    for ( int i=0; i < argc; ++i )
    {
        if ( sWeapon == argv[i] )
        {
            if ( i+2 >= argc )
            {
                BULOG_W(pClient->GetEdict(), "Error, you must provide 2 arguments for weapon (index and subindex).");
                return ECommandError;
            }
            if ( bAngle1 )
            {
                BULOG_W(pClient->GetEdict(), "Error, you can't mix weapon/ammo with angles.");
                return ECommandError;
            }
            if ( !bWeapon )
            {
                BULOG_W(pClient->GetEdict(), "Error, first you need to set waypoint type accordingly (weapon/ammo).");
                return ECommandError;
            }

            int i1 = -1, i2 = -1;
            sscanf(argv[++i], "%d", &i1);
            sscanf(argv[++i], "%d", &i2);

            if ( (i1 < 0) || (i1 > 15) || (i2 < 0) || (i2 > 15) )
            {
                BULOG_W(pClient->GetEdict(), "Error, invalid weapon arguments (must be from 0 to 15).");
                return ECommandError;
            }
            CWaypoint::SetWeapon(i1, i2, iArgument);
        }

        else if ( sAmmo == argv[i] )
        {
            if ( i+2 >= argc )
            {
                BULOG_W(pClient->GetEdict(), "Error, you must provide 2 arguments to ammo (ammo amount and 0=primary/1=secondary).");
                return ECommandError;
            }
            if ( bAngle1 )
            {
                BULOG_W(pClient->GetEdict(), "Error, you can't mix weapon/ammo with angles.");
                return ECommandError;
            }
            if ( !FLAG_SOME_SET(FWaypointAmmo, w.iFlags) )
            {
                BULOG_W(pClient->GetEdict(), "Error, first you need to set waypoint type accordingly (ammo).");
                return ECommandError;
            }

            int i1 = -1, i2 = -1;
            sscanf(argv[++i], "%d", &i1);
            sscanf(argv[++i], "%d", &i2);

            if ( (i1 < 0) || (i1 > 127) || (i2 < 0) || (i2 > 1) )
            {
                BULOG_W(pClient->GetEdict(), "Error, invalid ammo arguments (must be 0 .. 127 / 0 .. 1).");
                return ECommandError;
            }
            CWaypoint::SetAmmo(i1, i2 != 0, iArgument);
        }

        else if ( sHealth == argv[i] )
        {
            if ( i+1 >= argc )
            {
                BULOG_W(pClient->GetEdict(), "Error, you must provide 1 argument to health/health_machine (health amount).");
                return ECommandError;
            }
            if ( bAngle2 )
            {
                BULOG_W(pClient->GetEdict(), "Error, you can't mix health with 2 angles.");
                return ECommandError;
            }
            if ( !bHealth )
            {
                BULOG_W(pClient->GetEdict(), "Error, first you need to set waypoint type accordingly (health/health_machine).");
                return ECommandError;
            }

            int i1 = -1;
            sscanf(argv[++i], "%d", &i1);

            if ( (i1 < 0) || (i1 > 100) )
            {
                BULOG_W(pClient->GetEdict(), "Error, invalid health argument (must be from 0 to 100).");
                return ECommandError;
            }
            CWaypoint::SetHealth(i1, iArgument);
        }

        else if ( sArmor == argv[i] )
        {
            if ( i+1 >= argc )
            {
                BULOG_W(pClient->GetEdict(), "Error, you must provide 1 argument to armor/armor_machine (armor amount).");
                return ECommandError;
            }
            if ( bAngle2 )
            {
                BULOG_W(pClient->GetEdict(), "Error, you can't mix armor with 2 angles.");
                return ECommandError;
            }
            if ( !bArmor )
            {
                BULOG_W(pClient->GetEdict(), "Error, first you need to set waypoint type accordingly (armor/armor_machine).");
                return ECommandError;
            }

            int i1 = -1;
            sscanf(argv[++i], "%d", &i1);

            if ( (i1 < 0) || (i1 > 100) )
            {
                BULOG_W(pClient->GetEdict(), "Error, invalid armor argument (must be from 0 to 100).");
                return ECommandError;
            }
            CWaypoint::SetArmor(i1, iArgument);
        }

        else if ( sButton == argv[i] )
        {
            if ( i+1 >= argc )
            {
                BULOG_W(pClient->GetEdict(), "Error, you must provide 1 argument to button (button index).");
                return ECommandError;
            }
            if ( bAngle2 )
            {
                BULOG_W(pClient->GetEdict(), "Error, you can't mix button with 2 angles.");
                return ECommandError;
            }
            if ( !bButton )
            {
                BULOG_W(pClient->GetEdict(), "Error, first you need to set waypoint type accordingly (button/see_button).");
                return ECommandError;
            }

            int i1 = -1;
            sscanf(argv[++i], "%d", &i1);

            int iButtonsCount = CItems::GetItems(EItemTypeButton).size();
            if ( (i1 <= 0) || (i1 > iButtonsCount) )
            {
                BULOG_W(pClient->GetEdict(), "Error, invalid button argument (must be from 1 to %d).", iButtonsCount);
                return ECommandError;
            }
            CWaypoint::SetButton(i1, iArgument);
        }

        else if ( sFirstAngle == argv[i] )
        {
            if ( bWeapon )
            {
                BULOG_W(pClient->GetEdict(), "Error, you can't mix weapon/ammo with angles.");
                return ECommandError;
            }
            if ( !bAngle1 )
            {
                BULOG_W(pClient->GetEdict(), "Error, first you need to set waypoint type accordingly (camper/sniper/armor_machine/health_machine).");
                return ECommandError;
            }

            int iPitch = angClient.x;
            int iYaw = angClient.y;
            if ( (CWaypoint::MIN_ANGLE_PITCH <= iPitch) && (iPitch <= CWaypoint::MAX_ANGLE_PITCH) &&
                 (CWaypoint::MIN_ANGLE_PITCH <= iPitch) && (iPitch <= CWaypoint::MAX_ANGLE_PITCH) )
                 CWaypoint::SetFirstAngle(iPitch, iYaw, iArgument);
            else
            {
                BULOG_W(pClient->GetEdict(), "Error, up/down angle (pitch) must be from -64 to 63.");
                return ECommandError;
            }
        }

        else if ( sSecondAngle == argv[i] )
        {
            if ( bWeapon || bArmor || bHealth )
            {
                BULOG_W(pClient->GetEdict(), "Error, you can't mix weapon/ammo/health/armor with two angles.");
                return ECommandError;
            }
            if ( !bAngle2 )
            {
                BULOG_W(pClient->GetEdict(), "Error, first you need to set waypoint type accordingly (camper/sniper).");
                return ECommandError;
            }

            int iPitch = angClient.x;
            int iYaw = angClient.y;
            if ( (CWaypoint::MIN_ANGLE_PITCH <= iPitch) && (iPitch <= CWaypoint::MAX_ANGLE_PITCH) &&
                 (CWaypoint::MIN_ANGLE_PITCH <= iPitch) && (iPitch <= CWaypoint::MAX_ANGLE_PITCH) )
                 CWaypoint::SetSecondAngle(iPitch, iYaw, iArgument);
            else
            {
                BULOG_W(pClient->GetEdict(), "Error, up/down angle (pitch) must be from -64 to 63.");
                return ECommandError;
            }
        }

        else
        {
            BULOG_W(pClient->GetEdict(), "Error, invalid argument %s.", argv[i]);
            return ECommandError;
        }
    }

    w.iArgument = iArgument;
    return ECommandPerformed;
}

TCommandResult CWaypointInfoCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    if ( pClient == NULL )
    {
        BLOG_W( "Please login to server to execute this command." );
        return ECommandError;
    }

    TWaypointId id = -1;
    if (argc == 0)
        id = pClient->iCurrentWaypoint;
    else if (argc == 1)
        sscanf(argv[0], "%d", &id);

    if ( CWaypoints::IsValid(id) )
    {
        CWaypoint& w = CWaypoints::Get(id);
        const good::string& sFlags = CTypeToString::WaypointFlagsToString(w.iFlags);
        BULOG_I(pClient->GetEdict(), "Waypoint id %d: flags %s", id, (sFlags.size() > 0) ? sFlags.c_str() : sNone.c_str());
        BULOG_I(pClient->GetEdict(), "Origin: %.0f %.0f %.0f", w.vOrigin.x, w.vOrigin.y, w.vOrigin.z);
        return ECommandPerformed;
    }
    else
    {
        BULOG_W(pClient->GetEdict(), "Error, no waypoint nearby (move closer to waypoint).");
        return ECommandError;
    }
}

TCommandResult CWaypointDestinationCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    if ( pClient == NULL )
    {
        BLOG_W( "Please login to server to execute this command." );
        return ECommandError;
    }

    TWaypointId id = -1;
    if (argc == 0)
        id = pClient->iCurrentWaypoint;
    else if (argc == 1)
        sscanf(argv[0], "%d", &id);

    if ( CWaypoint::IsValid(id) )
    {
        pClient->bLockDestinationWaypoint = true;
        pClient->iDestinationWaypoint = id;
        BULOG_I(pClient->GetEdict(), "Path 'destination' is waypoint %d, locked.", id );
        return ECommandPerformed;
    }
    else
    {
        pClient->bLockDestinationWaypoint = false;
        BULOG_I(pClient->GetEdict(), "Error, invalid arguments or current waypoint (move closer to waypoint).");
        return ECommandError;
    }
}

TCommandResult CWaypointSaveCommand::Execute( CClient* pClient, int /*argc*/, const char** /*argv*/ )
{
    if ( pClient == NULL )
    {
        BLOG_W( "Please login to server to execute this command." );
        return ECommandError;
    }

    if ( CWaypoints::Save() )
    {
        BULOG_I(pClient->GetEdict(), "%d waypoints saved.", CWaypoints::Size());
        return ECommandPerformed;
    }
    else
    {
        BULOG_W(pClient->GetEdict(), "Error, could not save waypoints.");
        return ECommandError;
    }
}

TCommandResult CWaypointLoadCommand::Execute( CClient* pClient, int /*argc*/, const char** /*argv*/ )
{
    if ( pClient == NULL )
    {
        BLOG_W( "Please login to server to execute this command." );
        return ECommandError;
    }

    if ( CWaypoints::Load() )
    {
        BULOG_I(pClient->GetEdict(), "%d waypoints loaded for map %s.", CWaypoints::Size(), CBotrixPlugin::instance->sMapName.c_str() );
        return ECommandPerformed;
    }
    else
    {
        BULOG_E( pClient->GetEdict(), "Error, could not load waypoints for %s.", CBotrixPlugin::instance->sMapName.c_str() );
        return ECommandError;
    }
}


CWaypointVisibilityCommand::CWaypointVisibilityCommand()
{
    m_sCommand = "visibility";
    m_sHelp = "defines how to draw visible waypoints";
    m_sDescription = good::string("Can be 'none' / 'all' / 'next' or mix of: ") + CTypeToString::PathDrawFlagsToString(FPathDrawAll);
    m_iAccessLevel = FCommandAccessWaypoint;

    m_cAutoCompleteArguments.push_back(sNone);
    for (int i=0; i < EPathDrawFlagTotal; ++i)
        m_cAutoCompleteArguments.push_back( CTypeToString::PathDrawFlagsToString(1<<i).duplicate() );
    m_cAutoCompleteArguments.push_back(sAll);
    m_cAutoCompleteArguments.push_back(sNext);
}

TCommandResult CWaypointVisibilityCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    if ( pClient == NULL )
    {
        BLOG_W( "Please login to server to execute this command." );
        return ECommandError;
    }

    if ( argc == 0 )
    {
        const good::string& sType = CTypeToString::PathDrawFlagsToString(pClient->iPathDrawFlags);
        BULOG_I( pClient->GetEdict(), "Path draw flags: %s.", (sType.size() > 0) ? sType.c_str() : sNone.c_str());
        return ECommandPerformed;
    }

    // Retrieve flags from string arguments.
    bool bFinished = false;
    TPathDrawFlags iFlags = FPathDrawNone;

    if ( argc == 1 )
    {
        if ( sHelp == argv[0] )
        {
            PrintCommand( pClient->GetEdict() );
            return ECommandPerformed;
        }
        else if ( sNone == argv[0] )
            bFinished = true;
        else if ( sAll == argv[0] )
        {
            iFlags = FPathDrawAll;
            bFinished = true;
        }
        else if ( sNext == argv[0] )
        {
            int iNew = (pClient->iPathDrawFlags)  ?  pClient->iPathDrawFlags << 1  :  1;
            iFlags = (iNew > FPathDrawAll) ? 0 : iNew;
            bFinished = true;
        }
    }

    if ( !bFinished )
    {
        for ( int i=0; i < argc; ++i )
        {
            int iAddFlag = CTypeToString::PathDrawFlagsFromString(argv[i]);
            if ( iAddFlag == -1 )
            {
                BULOG_W( pClient->GetEdict(), "Error, invalid draw flag(s). Can be 'none' / 'all' / 'next' or mix of: %s.", CTypeToString::PathDrawFlagsToString(FWaypointDrawAll).c_str() );
                return ECommandError;
            }
            FLAG_SET(iAddFlag, iFlags);
        }
    }

    pClient->iVisiblesDrawFlags = iFlags;
    BULOG_I(pClient->GetEdict(), "Visible waypoints drawing is %s.", iFlags ? "on" : "off");
    return ECommandPerformed;
}


//----------------------------------------------------------------------------------------------------------------
// Waypoint area commands.
//----------------------------------------------------------------------------------------------------------------
TCommandResult CWaypointAreaRemoveCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    if ( pClient == NULL )
    {
        BLOG_W( "Please login to server to execute this command." );
        return ECommandError;
    }

    good::string_buffer sbBuffer(szMainBuffer, iMainBufferSize, false); // Don't deallocate after use.

    if ( argc < 1 )
    {
        BULOG_W(pClient->GetEdict(), "Error, 1 argument needed.");
        return ECommandError;
    }

    for ( int i=0; i < argc; ++i )
        sbBuffer << argv[i] << ' ';
    sbBuffer.erase( sbBuffer.size()-1, 1 ); // Erase last space.

    StringVector& cAreas = CWaypoints::GetAreas();
    for ( TAreaId iArea = 1; iArea < cAreas.size(); ++iArea ) // Do not take default area in account.
    {
        if ( sbBuffer == cAreas[iArea] )
        {
            for ( TWaypointId iWaypoint = 0; iWaypoint < CWaypoints::Size(); ++iWaypoint )
            {
                CWaypoint& cWaypoint = CWaypoints::Get(iWaypoint);
                if ( cWaypoint.iAreaId > iArea )
                    --cWaypoint.iAreaId;
                else if ( cWaypoint.iAreaId == iArea )
                    cWaypoint.iAreaId = 0; // Set to default.
            }

            cAreas.erase(cAreas.begin() + iArea);
            BULOG_I( pClient->GetEdict(), "Deleted area '%s'.", sbBuffer.c_str() );
            return ECommandPerformed;
        }
    }

    BULOG_W(pClient->GetEdict(), "Error, no area with such name.");
    return ECommandError;
}

TCommandResult CWaypointAreaRenameCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    if ( pClient == NULL )
    {
        BLOG_W( "Please login to server to execute this command." );
        return ECommandError;
    }

    good::string_buffer sbBuffer(szMainBuffer, iMainBufferSize, false); // Don't deallocate after use.

    if ( argc < 2 )
    {
        BULOG_W(pClient->GetEdict(), "Error, 2 arguments needed.");
        return ECommandError;
    }

    for ( int i=0; i < argc; ++i )
        sbBuffer << argv[i] << ' ';
    sbBuffer.erase( sbBuffer.size()-1, 1 ); // Erase last space.

    StringVector& cAreas = CWaypoints::GetAreas();
    for ( int i=1; i < cAreas.size(); ++i ) // Do not take default area in account.
    {
        StringVector::value_type& sArea = cAreas[i];
        if ( good::starts_with((good::string)sbBuffer, sArea) )
        {
            sbBuffer.erase(0, sArea.size());
            good::trim(sbBuffer);
            if ( sbBuffer.size() > 0 )
            {
                BULOG_I( pClient->GetEdict(), "Renamed '%s' to '%s'.", sArea.c_str(), sbBuffer.c_str() );
                sArea = sbBuffer.duplicate();
                return ECommandPerformed;
            }
            else
            {
                BULOG_W( pClient->GetEdict(), "Error, can't rename '%s' to '%s'.", cAreas[i].c_str(), sbBuffer.c_str() );
                return ECommandError;
            }
        }
    }

    BULOG_W(pClient->GetEdict(), "Error, no area with such name.");
    return ECommandError;
}

TCommandResult CWaypointAreaSetCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    if ( pClient == NULL )
    {
        BLOG_W( "Please login to server to execute this command." );
        return ECommandError;
    }

    TWaypointId iWaypoint = pClient->iCurrentWaypoint;
    int index = 0;

    if ( argc > 0 ) // Check if first argument is a waypoint number.
    {
        char c = argv[0][0];
        bool isNumber = '0' <= c && c <= '9';

        if (isNumber)
            iWaypoint = atoi(argv[index++]);
    }

    if ( index == argc ) // Only waypoint given, show waypoint area.
    {
        const CWaypoint& w = CWaypoints::Get(iWaypoint);
        BULOG_I(pClient->GetEdict(), "Waypoint %d is at area %s (%d).", iWaypoint, CWaypoints::GetAreas()[w.iAreaId].c_str(), w.iAreaId);
        return ECommandPerformed;
    }

    if ( !CWaypoints::IsValid(iWaypoint) )
    {
        BULOG_W(pClient->GetEdict(), "Error, invalid waypoint: %d.", iWaypoint);
        return ECommandError;
    }

    // Concatenate all arguments.
    good::string_buffer sbBuffer(szMainBuffer, iMainBufferSize, false); // Don't deallocate after use.
    for ( ; index < argc; ++index )
        sbBuffer << argv[index] << ' ';
    good::trim(sbBuffer);

    // Check if that area id already exists.
    TAreaId iAreaId = CWaypoints::GetAreaId(sbBuffer);
    if ( iAreaId == EAreaIdInvalid ) // If not, add it.
        iAreaId = CWaypoints::AddAreaName(sbBuffer.duplicate());

    CWaypoints::Get(iWaypoint).iAreaId = iAreaId;

    return ECommandPerformed;
}

TCommandResult CWaypointAreaShowCommand::Execute( CClient* pClient, int /*argc*/, const char** /*argv*/ )
{
    edict_t* pEdict = ( pClient ) ? pClient->GetEdict() : NULL;

    good::string_buffer sbBuffer(szMainBuffer, iMainBufferSize, false); // Don't deallocate after use.

    for ( int i=0; i < CWaypoints::GetAreas().size(); ++i )
        sbBuffer << "   - " <<  CWaypoints::GetAreas()[i] << '\n';
    sbBuffer.erase( sbBuffer.size()-1, 1 ); // Erase last \n.

    BULOG_I( pEdict, "Area names:\n%s", sbBuffer.c_str() );
    return ECommandPerformed;
}

//----------------------------------------------------------------------------------------------------------------
// Paths commands.
//----------------------------------------------------------------------------------------------------------------
/*
TCommandResult CPathSwapCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    if ( pClient == NULL )
    {
        BLOG_W( "Please login to server to execute this command." );
        return ECommandError;
    }

    if ( argc > 0 && argv[0] )
    {
        int iValue = pClient->iDestinationWaypoint;
        sscanf(argv[0], "%d", &iValue);
        if ( !CWaypoints::IsValid(iValue) )
        {
            BULOG_W(pClient->GetEdict(), "Error, invalid parameter.");
            return ECommandError;
        }
        pClient->iDestinationWaypoint = iValue;
    }

    if ( !CWaypoint::IsValid(pClient->iDestinationWaypoint) )
    {
        BULOG_W(pClient->GetEdict(), "Error, you need to set 'destination' waypoint first.");
        return ECommandError;
    }

    if ( CWaypoint::IsValid(pClient->iCurrentWaypoint) )
        good::swap(pClient->iDestinationWaypoint, pClient->iCurrentWaypoint);
    else
        pClient->iCurrentWaypoint = pClient->iDestinationWaypoint;

    if ( pClient->iDestinationWaypoint ) // Set angles to aim there.
    {
        CWaypoint& wFrom = CWaypoints::Get(pClient->iCurrentWaypoint);
    }

    CWaypoint& wTo = CWaypoints::Get(pClient->iCurrentWaypoint);
    pClient->GetPlayerInfo()->Set

    BULOG_I(pClient->GetEdict(), "Teleported to %d. Path destination now is %d", pClient->iCurrentWaypoint, pClient->iDestinationWaypoint);

    return ECommandPerformed;
}
*/

CPathDrawCommand::CPathDrawCommand()
{
    m_sCommand = "drawtype";
    m_sHelp = "defines how to draw path";
    m_sDescription = good::string("Can be 'none' / 'all' / 'next' or mix of: ") + CTypeToString::PathDrawFlagsToString(FPathDrawAll);
    m_iAccessLevel = FCommandAccessWaypoint;

    m_cAutoCompleteArguments.push_back(sNone);
    for (int i=0; i < EPathDrawFlagTotal; ++i)
        m_cAutoCompleteArguments.push_back( CTypeToString::PathDrawFlagsToString(1<<i).duplicate() );
    m_cAutoCompleteArguments.push_back(sAll);
    m_cAutoCompleteArguments.push_back(sNext);
}

TCommandResult CPathDrawCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    if ( pClient == NULL )
    {
        BLOG_W( "Please login to server to execute this command." );
        return ECommandError;
    }

    if ( argc == 0 )
    {
        const good::string& sType = CTypeToString::PathDrawFlagsToString(pClient->iPathDrawFlags);
        BULOG_I( pClient->GetEdict(), "Path draw flags: %s.", (sType.size() > 0) ? sType.c_str() : sNone.c_str());
        return ECommandPerformed;
    }

    // Retrieve flags from string arguments.
    bool bFinished = false;
    TPathDrawFlags iFlags = FPathDrawNone;

    if ( argc == 1 )
    {
        if ( sHelp == argv[0] )
        {
            PrintCommand( pClient->GetEdict() );
            return ECommandPerformed;
        }
        else if ( sNone == argv[0] )
            bFinished = true;
        else if ( sAll == argv[0] )
        {
            iFlags = FPathDrawAll;
            bFinished = true;
        }
        else if ( sNext == argv[0] )
        {
            int iNew = (pClient->iPathDrawFlags)  ?  pClient->iPathDrawFlags << 1  :  1;
            iFlags = (iNew > FPathDrawAll) ? 0 : iNew;
            bFinished = true;
        }
    }

    if ( !bFinished )
    {
        for ( int i=0; i < argc; ++i )
        {
            int iAddFlag = CTypeToString::PathDrawFlagsFromString(argv[i]);
            if ( iAddFlag == -1 )
            {
                BULOG_W( pClient->GetEdict(), "Error, invalid draw flag(s). Can be 'none' / 'all' / 'next' or mix of: %s.", CTypeToString::PathDrawFlagsToString(FWaypointDrawAll).c_str() );
                return ECommandError;
            }
            FLAG_SET(iAddFlag, iFlags);
        }
    }

    pClient->iPathDrawFlags = iFlags;
    BULOG_I(pClient->GetEdict(), "Path drawing is %s.", iFlags ? "on" : "off");
    return ECommandPerformed;
}

TCommandResult CPathCreateCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    if ( pClient == NULL )
    {
        BLOG_W( "Please login to server to execute this command." );
        return ECommandError;
    }

    TWaypointId iPathFrom = -1, iPathTo = -1;
    if (argc == 0)
    {
        iPathFrom = pClient->iCurrentWaypoint;
        iPathTo = pClient->iDestinationWaypoint;
    }
    else if (argc == 1)
    {
        iPathFrom = pClient->iCurrentWaypoint;
        sscanf(argv[0], "%d", &iPathTo);
    }
    else if (argc == 2)
    {
        sscanf(argv[0], "%d", &iPathFrom);
        sscanf(argv[1], "%d", &iPathTo);
    }

    if ( !CWaypoints::IsValid(iPathFrom) || !CWaypoints::IsValid(iPathTo) )
    {
        BULOG_W(pClient->GetEdict(), "Error, invalid parameters, current or 'destination' waypoints.");
        return ECommandError;
    }

    TPathFlags iFlags = FPathNone;
    if ( pClient->IsAlive() )
    {
        float fHeight = pClient->GetPlayerInfo()->GetPlayerMaxs().z - pClient->GetPlayerInfo()->GetPlayerMins().z + 1;
        if (fHeight < CMod::iPlayerHeight)
            iFlags = FPathCrouch;
    }

    if ( CWaypoints::AddPath(iPathFrom, iPathTo, 0, iFlags) )
    {
        BULOG_I(pClient->GetEdict(), "Path created: from %d to %d.", iPathFrom, iPathTo);
        return ECommandPerformed;
    }
    else
    {
        BULOG_W(pClient->GetEdict(), "Error creating path from %d to %d (exists or too big distance?).", iPathFrom, iPathTo);
        return ECommandError;
    }
}

TCommandResult CPathRemoveCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    if ( pClient == NULL )
    {
        BLOG_W( "Please login to server to execute this command." );
        return ECommandError;
    }

    TWaypointId iPathFrom = -1, iPathTo = -1;
    if (argc == 0)
    {
        iPathFrom = pClient->iCurrentWaypoint;
        iPathTo = pClient->iDestinationWaypoint;
    }
    else if (argc == 1)
    {
        iPathFrom = pClient->iCurrentWaypoint;
        sscanf(argv[0], "%d", &iPathTo);
    }
    else if (argc == 2)
    {
        sscanf(argv[0], "%d", &iPathFrom);
        sscanf(argv[1], "%d", &iPathTo);
    }

    if ( !CWaypoints::IsValid(iPathFrom) || !CWaypoints::IsValid(iPathTo) )
    {
        BULOG_W(pClient->GetEdict(), "Error, invalid parameters, current or 'destination' waypoints.");
        return ECommandError;
    }

    if ( CWaypoints::RemovePath(iPathFrom, iPathTo) )
    {
        BULOG_I(pClient->GetEdict(), "Path removed: from %d to %d.", iPathFrom, iPathTo);
        return ECommandPerformed;
    }
    else
    {
        BULOG_W(pClient->GetEdict(), "Error, there is no path from %d to %d.", iPathFrom, iPathTo);
        return ECommandError;
    }
}

TCommandResult CPathAutoCreateCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    if ( pClient == NULL )
    {
        BLOG_W( "Please login to server to execute this command." );
        return ECommandError;
    }

    if ( argc == 0 )
    {
        BULOG_I(pClient->GetEdict(), "Waypoint's auto paths creation is: %s.", CTypeToString::BoolToString(pClient->bAutoCreatePaths).c_str());
        return ECommandPerformed;
    }

    int iValue = -1;
    if ( argc == 1 )
        iValue = CTypeToString::BoolFromString(argv[0]);

    if ( iValue == -1 )
    {
        BULOG_W(pClient->GetEdict(), "Error, invalid argument (must be 'on' or 'off').");
        return ECommandError;
    }

    pClient->bAutoCreatePaths = iValue != 0;
    BULOG_I(pClient->GetEdict(), "Waypoint's auto paths creation is: %s.", CTypeToString::BoolToString(pClient->bAutoCreatePaths).c_str());
    return ECommandPerformed;
}

TCommandResult CPathAddTypeCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    if ( pClient == NULL )
    {
        BLOG_W( "Please login to server to execute this command." );
        return ECommandError;
    }

    // Get 'destination' and current waypoints IDs.
    TWaypointId iPathFrom = pClient->iCurrentWaypoint;
    TWaypointId iPathTo = pClient->iDestinationWaypoint;

    if ( !CWaypoints::IsValid(iPathFrom) )
    {
        BULOG_W(pClient->GetEdict(), "Error, no waypoint nearby (move closer to waypoint).");
        return ECommandError;
    }
    if ( !CWaypoints::IsValid(iPathTo) )
    {
        BULOG_W(pClient->GetEdict(), "Error, you need to set 'destination' waypoint first.");
        return ECommandError;
    }

    // Retrieve flags from string arguments.
    TPathFlags iFlags = FPathNone;
    for ( int i=0; i < argc; ++i )
    {
        int iAddFlag = CTypeToString::PathFlagsFromString(argv[i]);
        if ( iAddFlag == -1 )
        {
            BULOG_W( pClient->GetEdict(), "Error, invalid path flag: %s. Can be one of: %s", argv[i], CTypeToString::PathFlagsToString(FPathAll).c_str() );
            return ECommandError;
        }
        FLAG_SET(iAddFlag, iFlags);
    }

    if ( iFlags == FPathNone )
    {
        BULOG_W( pClient->GetEdict(), "Error, at least one path flag is needed. Can be one of: %s", CTypeToString::PathFlagsToString(FPathAll).c_str() );
        return ECommandError;
    }

    CWaypointPath* pPath = CWaypoints::GetPath(iPathFrom, pClient->iDestinationWaypoint);
    if ( pPath )
    {
        FLAG_SET(iFlags, pPath->iFlags);
        BULOG_I(pClient->GetEdict(), "Added path types %s (path from %d to %d).", CTypeToString::PathFlagsToString(iFlags).c_str(), iPathFrom, iPathTo);
        return ECommandPerformed;
    }
    else
    {
        BULOG_W(pClient->GetEdict(), "Error, no path from %d to %d.", iPathFrom, iPathTo);
        return ECommandError;
    }
}

TCommandResult CPathRemoveTypeCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    if ( pClient == NULL )
    {
        BLOG_W( "Please login to server to execute this command." );
        return ECommandError;
    }

    TWaypointId iPathFrom = -1, iPathTo = -1;
    if (argc == 0)
    {
        iPathFrom = pClient->iCurrentWaypoint;
        iPathTo = pClient->iDestinationWaypoint;
    }
    else if (argc == 1)
    {
        iPathFrom = pClient->iCurrentWaypoint;
        sscanf(argv[0], "%d", &iPathTo);
    }
    else if (argc == 2)
    {
        sscanf(argv[0], "%d", &iPathFrom);
        sscanf(argv[1], "%d", &iPathTo);
    }

    if ( !CWaypoints::IsValid(iPathFrom) || !CWaypoints::IsValid(iPathTo) )
    {
        BULOG_W(pClient->GetEdict(), "Error, invalid parameters, current or 'destination' waypoints.");
        return ECommandError;
    }

    CWaypointPath* pPath = CWaypoints::GetPath(iPathFrom, iPathTo);
    if ( pPath )
    {
        pPath->iFlags = FPathNone;
        BULOG_I(pClient->GetEdict(), "Removed all types for path from %d to %d.", iPathFrom, pClient->iDestinationWaypoint);
        return ECommandPerformed;
    }
    else
    {
        BULOG_W(pClient->GetEdict(), "Error, no path from %d to %d.", iPathFrom, pClient->iDestinationWaypoint);
        return ECommandError;
    }
}

TCommandResult CPathArgumentCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    if ( pClient == NULL )
    {
        BLOG_W( "Please login to server to execute this command." );
        return ECommandError;
    }

    if ( !CWaypoints::IsValid(pClient->iCurrentWaypoint) || !CWaypoints::IsValid(pClient->iDestinationWaypoint) )
    {
        BULOG_W(pClient->GetEdict(), "Error, invalid current or 'destination' waypoints.");
        return ECommandError;
    }

    CWaypointPath* pPath = CWaypoints::GetPath(pClient->iCurrentWaypoint, pClient->iDestinationWaypoint);
    if ( pPath == NULL )
    {
        BULOG_W(pClient->GetEdict(), "Error, no path from %d to %d.", pClient->iCurrentWaypoint, pClient->iDestinationWaypoint);
        return ECommandError;
    }

    if (argc == 0)
    {
        BULOG_I( pClient->GetEdict(), "Path (from %d to %d) action time %d, action duration %d. Time in deciseconds.",
                        pClient->iCurrentWaypoint, pClient->iDestinationWaypoint, GET_1ST_BYTE(pPath->iArgument), GET_2ND_BYTE(pPath->iArgument) );
        return ECommandPerformed;
    }
    else
    {
        TWaypointId iFirst = -1, iSecond = -1;
        if (argc == 2)
        {
            sscanf(argv[0], "%d", &iFirst);
            sscanf(argv[1], "%d", &iSecond);
        }

        if ( iFirst < 0 || iSecond < 0 || (iFirst & ~0xFF) || (iSecond & ~0xFF) )
        {
            BULOG_W(pClient->GetEdict(), "Error, invalid parameters, must be from 0 to 256.");
            return ECommandError;
        }

        pPath->iArgument = iFirst | (iSecond << 8);
        BULOG_I( pClient->GetEdict(), "Set path (from %d to %d) action time %d, action duration %d. Time in deciseconds.",
                        pClient->iCurrentWaypoint, pClient->iDestinationWaypoint, GET_1ST_BYTE(pPath->iArgument), GET_2ND_BYTE(pPath->iArgument) );
        return ECommandPerformed;
    }
}

TCommandResult CPathInfoCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    if ( pClient == NULL )
    {
        BLOG_W( "Please login to server to execute this command." );
        return ECommandError;
    }

    TWaypointId iPathFrom = -1, iPathTo = -1;
    if (argc == 0)
    {
        iPathFrom = pClient->iCurrentWaypoint;
        iPathTo = pClient->iDestinationWaypoint;
    }
    else if (argc == 1)
    {
        iPathFrom = pClient->iCurrentWaypoint;
        sscanf(argv[0], "%d", &iPathTo);
    }
    else if (argc == 2)
    {
        sscanf(argv[0], "%d", &iPathFrom);
        sscanf(argv[1], "%d", &iPathTo);
    }

    if ( !CWaypoints::IsValid(iPathFrom) || !CWaypoints::IsValid(iPathTo) )
    {
        BULOG_W(pClient->GetEdict(), "Error, invalid parameters, current or 'destination' waypoints.");
        return ECommandError;
    }

    CWaypointPath* pPath = CWaypoints::GetPath(iPathFrom, iPathTo);
    if ( pPath )
    {
        if ( pPath->HasDemo() )
            BULOG_I(pClient->GetEdict(), "Path (from %d to %d) has demo: id %d.", iPathFrom, iPathTo, pPath->DemoNumber());
        else
        {
            const good::string& sFlags = CTypeToString::PathFlagsToString(pPath->iFlags);
            BULOG_I( pClient->GetEdict(), "Path (from %d to %d) has flags: %s.", iPathFrom, iPathTo,
                            (sFlags.size() > 0) ? sFlags.c_str() : sNone.c_str() );
            if ( FLAG_SOME_SET(FPathDoor, pPath->iFlags) )
            {
                if ( pPath->iArgument )
                    BULOG_I( pClient->GetEdict(), "Door %d.", pPath->iArgument );
                else
                    BULOG_I( pClient->GetEdict(), "Door not set." );
            }
            else if ( FLAG_SOME_SET(FPathJump | FPathCrouch | FPathBreak, pPath->iFlags) )
                BULOG_I( pClient->GetEdict(), "Path action time %d, action duration %d. Time in deciseconds.", GET_1ST_BYTE(pPath->iArgument), GET_2ND_BYTE(pPath->iArgument) );
        }
        return ECommandPerformed;
    }
    else
    {
        BULOG_W(pClient->GetEdict(), "Error, no path from %d to %d.", iPathFrom, pClient->iDestinationWaypoint);
        return ECommandError;
    }
}



//----------------------------------------------------------------------------------------------------------------
// Bots commands.
//----------------------------------------------------------------------------------------------------------------
TCommandResult AllowOrForbid( bool bForbid, CClient* pClient, int argc, const char** argv )
{
    edict_t* pEdict = pClient ? pClient->GetEdict() : NULL;

    if ( argc == 0 ) // Print all weapons.
    {
        for ( TWeaponId i=0; i < CWeapons::Size(); ++i )
        {
            const CWeapon* pWeapon = CWeapons::Get(i).GetBaseWeapon();
            BULOG_I(pEdict, "%s is %s.", pWeapon->pWeaponClass->sClassName.c_str(), pWeapon->bForbidden ? "forbidden" : "allowed" );
        }
    }
    else if ( (argc == 1) && (sAll == argv[0]) ) // Apply to all weapons.
    {
        for ( TWeaponId i=0; i < CWeapons::Size(); ++i )
        {
            const CWeapon* pWeapon = CWeapons::Get(i).GetBaseWeapon();
            ((CWeapon*)pWeapon)->bForbidden = bForbid;
            BULOG_I(pEdict, "%s is %s.", pWeapon->pWeaponClass->sClassName.c_str(), pWeapon->bForbidden ? "forbidden" : "allowed" );
        }
    }
    else
    {
        for ( int i=0; i < argc; ++i )
        {
            TWeaponId iWeaponId = CWeapons::GetIdFromWeaponName( argv[i] );
            if ( CWeapon::IsValid(iWeaponId) )
            {
                const CWeapon* pWeapon = CWeapons::Get(iWeaponId).GetBaseWeapon();
                ((CWeapon*)pWeapon)->bForbidden = bForbid;
                BULOG_I(pEdict, "%s is %s.", argv[i], bForbid ? "forbidden" : "allowed" );
            }
            else
                BULOG_W(pEdict, "Warning, no such weapon: %s, skipping.", argv[i]);
        }
    }
    return ECommandPerformed;
}

TCommandResult CBotWeaponAddCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    edict_t* pEdict = ( pClient ) ? pClient->GetEdict() : NULL;

    if ( argc != 2 )
    {
        BULOG_W( pEdict, "Invalid parameters count." );
        return ECommandError;
    }

    good::string sName( argv[0] );
    bool bAll = ( sName == "all" );

    for ( TPlayerIndex i = 0; i < CPlayers::Size(); ++i )
    {
        CPlayer* pPlayer = CPlayers::Get(i);
        if ( pPlayer && pPlayer->IsBot() )
        {
            good::string sBotName = pPlayer->GetName();
            if ( bAll || good::starts_with(sBotName, sName) )
                ((CBot*)pPlayer)->AddWeapon(argv[1]);
        }
    }

    return ECommandPerformed;
}

TCommandResult CBotWeaponAllowCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    return AllowOrForbid(false, pClient, argc, argv);
}

TCommandResult CBotWeaponForbidCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    return AllowOrForbid(true, pClient, argc, argv);
}

TCommandResult CBotWeaponRemoveCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    edict_t* pEdict = ( pClient ) ? pClient->GetEdict() : NULL;

    if ( argc != 1 )
    {
        BULOG_W( pEdict, "Invalid parameters count." );
        return ECommandError;
    }

    good::string sName( argv[0] );
    bool bAll = ( sName == "all" );

    for ( TPlayerIndex i = 0; i < CPlayers::Size(); ++i )
    {
        CPlayer* pPlayer = CPlayers::Get(i);
        if ( pPlayer && pPlayer->IsBot() )
        {
            good::string sBotName = pPlayer->GetName();
            if ( bAll || good::starts_with(sBotName, sName) )
                ((CBot*)pPlayer)->RemoveWeapons();
        }
    }

    return ECommandPerformed;
}

TCommandResult CBotWeaponUnknownCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    edict_t* pEdict = ( pClient ) ? pClient->GetEdict() : NULL;

    bool bAssume;

    if ( argc == 1 )
    {
        good::string sArg( argv[0] );
        if ( sArg == "melee" )
            bAssume = true;
        else if ( sArg == "ranged" )
            bAssume = false;
        else
        {
            BULOG_W( pEdict, "Invalid parameter: %s. Should be 'melee' or 'ranged'", argv[0] );
            return ECommandError;
        }
    }
    else
    {
        BULOG_W( pEdict, "Invalid parameters count." );
        return ECommandError;
    }

    CBot::bAssumeUnknownWeaponManual = bAssume;
    return ECommandPerformed;
}

TCommandResult CBotAddCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    edict_t* pEdict = ( pClient ) ? pClient->GetEdict() : NULL;

    // 1st argument: name.
    int iArg = 0;
    const char* szName = ( argc > iArg ) ? argv[iArg++] : NULL;

    // 2nd argument: intelligence.
    TBotIntelligence iIntelligence = CBot::iDefaultIntelligence;
    if ( argc > iArg )
    {
        good::string sIntelligence( argv[iArg++] );
        iIntelligence = CTypeToString::IntelligenceFromString(sIntelligence);
        if ( (iIntelligence == -1) && (sIntelligence != "random") )
        {
            BULOG_W(pEdict, "Invalid bot intelligence: %s.", argv[1] );
            BULOG_W( pEdict, "  Must be one of: fool stupied normal smart pro." );
            // TODO: CTypeToString::AllIntelligences()
            return ECommandError;
        }
    }

    // 3rd argument: team
    TTeam iTeam = CBot::iDefaultTeam;
    if ( argc > iArg )
    {
        iTeam = CTypeToString::TeamFromString(argv[iArg]);
        if ( iTeam == -1 )
        {
            BULOG_W(pEdict, "Invalid team: %s.", argv[iArg]);
            BULOG_W( pEdict, "  Must be one of: %s.", CTypeToString::TeamFlagsToString(-1).c_str() );
            return ECommandError;
        }
        iArg++;
    }

    // Mod can have no classes.
    TClass iClass = CBot::iDefaultClass;
    if ( CMod::aClassNames.size() && (argc > iArg) )
    {
        good::string sClass( argv[iArg] );
        iClass = CTypeToString::ClassFromString(sClass);
        if ( (iClass == -1) && (sClass != "random") )
        {
            BULOG_W(pEdict, "Invalid class: %s.", argv[iArg]);
            BULOG_W( pEdict, "  Must be one of: %s.", CTypeToString::ClassFlagsToString(-1).c_str() );
            return ECommandError;
        }
        iArg++;
    }

    CPlayer* pBot = CPlayers::AddBot(szName, iTeam, iClass, iIntelligence, argc-iArg, &argv[iArg]);
    if ( pBot )
    {
        BULOG_I( pEdict, "Bot added: %s.", pBot->GetName() );
        return ECommandPerformed;
    }
    else
    {
        const good::string& sLastError = CPlayers::GetLastError();
        if ( sLastError.size() )
            BULOG_W( pEdict, sLastError.c_str() );
        return ECommandError;
    }
}

TCommandResult CBotCommandCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    edict_t* pEdict = ( pClient ) ? pClient->GetEdict() : NULL;

    if ( argc != 2 )
    {
        BULOG_W( pEdict,"Error, invalid arguments count." );
        return ECommandError;
    }
    else
    {
        good::string sName = argv[0];
        bool bAll = (sName == sAll);

        for ( TPlayerIndex i = 0; i < CPlayers::Size(); ++i )
        {
            CPlayer* pPlayer = CPlayers::Get(i);
            if ( pPlayer && pPlayer->IsBot() )
            {
                if ( bAll || good::starts_with( good::string(pPlayer->GetName()), sName ) )
                {
                    CBot* pBot = (CBot*)pPlayer;
                    pBot->ConsoleCommand(argv[1]);
                }
            }
        }
    }
    return ECommandPerformed;
}

TCommandResult CBotKickCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    edict_t* pEdict = ( pClient ) ? pClient->GetEdict() : NULL;

    // TODO: finish params
    if ( argc == 0 )
    {
        if ( !CPlayers::KickRandomBot() )
            BULOG_W(pEdict,"Error, no bots to kick");
    }
    else
    {
        int team = atoi(argv[0]);
        if ( !CPlayers::KickRandomBotOnTeam(team) )
            BULOG_W(pEdict,"Error, no bots to kick on team %d", team);
    }
    return ECommandPerformed;
}

TCommandResult CBotDebugCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    edict_t* pEdict = ( pClient ) ? pClient->GetEdict() : NULL;

    if ( argc == 0 )
    {
        BULOG_W( pEdict, "Error, you need to provide bot's name (or at least how it starts) or 'all'/'none'."); // TODO: all none.
        return ECommandError;
    }
    if ( argc > 2 )
    {
        BULOG_W( pEdict, "Error, invalid arguments count.");
        return ECommandError;
    }

    good::string sName = argv[0];

    CBot* pBot = NULL;
    for ( TPlayerIndex i = 0; i < CPlayers::Size(); ++i )
    {
        CPlayer* pPlayer = CPlayers::Get(i);
        if ( pPlayer && pPlayer->IsBot() )
        {
            good::string sBotName = pPlayer->GetName();
            if ( good::starts_with(sBotName, sName) )
            {
                pBot = (CBot*)pPlayer;
                break;
            }
        }
    }

    if ( pBot )
    {
        int bDebug = true;
        if ( argc == 2 )
        {
            bDebug = CTypeToString::BoolFromString(argv[1]);
            if ( bDebug == -1 )
            {
                BULOG_W( pEdict, "Error, unknown parameter %s, should be 'on' or 'off'.", argv[1] );
                return ECommandError;
            }
        }
        pBot->SetDebugging(bDebug != 0);
        return ECommandPerformed;
    }
    else
    {
        BULOG_W( pEdict, "Error, no such bot: %s.", argv[0] );
        return ECommandError;
    }
}

TCommandResult CBotConfigQuotaCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    edict_t* pEdict = ( pClient ) ? pClient->GetEdict() : NULL;

    TCommandResult iResult = ECommandPerformed;
    if ( argc == 0 )
    {
        if ( CPlayers::fPlayerBotRatio > 0.0f )
            BULOG_I( pEdict, "Player-Bot ratio is %.1f.", CPlayers::fPlayerBotRatio );
        else
            BULOG_I( pEdict, "Bots+Players amount: %d.", CPlayers::iBotsPlayersCount );
    }
    else if ( argc == 1 )
    {
        StringVector aSplit(2);
        good::split( good::string(argv[0]), aSplit, '-' );
        int iArg[2] = {-1, -1};
        if ( aSplit.size() > 2 )
        {
            BULOG_W( pEdict, "Error, invalid argument: %s.", argv[0] );
            return ECommandError;
        }
        for ( int i=0; i < aSplit.size(); ++i )
            if ( (sscanf(aSplit[i].c_str(), "%d", &iArg[i]) != 1) || (iArg[i] <= 0) ||
                 (CBotrixPlugin::instance->bMapRunning && (iArg[i] > CPlayers::Size())) )
            {
                BULOG_W( pEdict, "Error, invalid argument: %s.", aSplit[i].c_str() );
                return ECommandError;
            }

        if ( aSplit.size() == 2 )
        {
            CPlayers::fPlayerBotRatio = (float)iArg[1]/(float)iArg[0];
            BULOG_I( pEdict, "Player-Bot ratio is %.1f.", CPlayers::fPlayerBotRatio );
        }
        else
        {
            CPlayers::fPlayerBotRatio = 0.0f;
            CPlayers::iBotsPlayersCount = iArg[0];
            BULOG_I( pEdict, "Bots+Players amount: %d.", CPlayers::iBotsPlayersCount );
        }

        CPlayers::CheckBotsCount();
        iResult = ECommandPerformed;
    }
    else
    {
        BULOG_W( pEdict, "Error, invalid arguments count." );
        iResult = ECommandError;
    }
    return iResult;
}

TCommandResult CBotConfigIntelligenceCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    edict_t* pEdict = ( pClient ) ? pClient->GetEdict() : NULL;

    TCommandResult iResult = ECommandPerformed;
    if ( argc == 0 )
    {
        const char* szIntelligence = (CBot::iDefaultIntelligence == -1)
            ? "random"
            : CTypeToString::IntelligenceToString(CBot::iDefaultIntelligence).c_str();
        BULOG_I( pEdict, "Bot's default intelligence: %s.", szIntelligence );
    }
    else if ( argc == 1 )
    {
        good::string sIntelligence( argv[0] );
        TBotIntelligence iIntelligence = CTypeToString::IntelligenceFromString(sIntelligence);
        if ( (iIntelligence == -1) && (sIntelligence != "random") )
        {
            BULOG_W( pEdict, "Error, invalid intelligence: %s.", argv[0] );
            BULOG_W( pEdict, "Can be one of: random fool stupied normal smart pro" );
            iResult = ECommandError;
        }
        else
        {
            BULOG_I( pEdict, "Bot's default intelligence: %s.", argv[0] );
            CBot::iDefaultIntelligence = iIntelligence;
        }
    }
    else
    {
        BULOG_W( pEdict, "Error, invalid arguments count." );
        iResult = ECommandError;
    }
    return iResult;
}

TCommandResult CBotConfigTeamCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    edict_t* pEdict = ( pClient ) ? pClient->GetEdict() : NULL;

    TCommandResult iResult = ECommandPerformed;
    if ( argc == 0 )
    {
        BULOG_I( pEdict, "Bot's default team: %s.", CTypeToString::TeamToString(CBot::iDefaultTeam).c_str() );
    }
    else if ( argc == 1 )
    {
        TTeam iTeam = CTypeToString::TeamFromString(argv[0]);
        if ( iTeam == -1 )
        {
            BULOG_W( pEdict, "Error, invalid team: %s.", argv[0] );
            BULOG_W( pEdict, "Can be one of: %s", CTypeToString::TeamFlagsToString(-1).c_str() );
            iResult = ECommandError;
        }
        else
        {
            BULOG_I( pEdict, "Bot's default team: %s.", argv[0] );
            CBot::iDefaultTeam = iTeam;
        }
    }
    else
    {
        BULOG_W( pEdict, "Error, invalid arguments count." );
        iResult = ECommandError;
    }
    return iResult;
}

TCommandResult CBotConfigClassCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    edict_t* pEdict = ( pClient ) ? pClient->GetEdict() : NULL;

    TCommandResult iResult = ECommandPerformed;
    if ( argc == 0 )
    {
        const char* szClass = (CBot::iDefaultClass == -1)
            ? "random"
            : CTypeToString::ClassToString(CBot::iDefaultClass).c_str();
        BULOG_I( pEdict, "Bot's default class: %s.", szClass );
    }
    else if ( argc == 1 )
    {
        good::string sClass( argv[0] );
        TClass iClass = CTypeToString::ClassFromString(sClass);
        if ( (iClass == -1) && (sClass != "random") )
        {
            BULOG_W( pEdict, "Error, invalid class: %s.", argv[0] );
            BULOG_W( pEdict, "Can be one of: random %s.", CTypeToString::ClassFlagsToString(-1).c_str() );
            iResult = ECommandError;
        }
        else
        {
            BULOG_I( pEdict, "Bot's default class: %s.", argv[0] );
            CBot::iDefaultClass = iClass;
        }
    }
    else
    {
        BULOG_W( pEdict, "Error, invalid arguments count." );
        iResult = ECommandError;
    }
    return iResult;
}

TCommandResult CBotConfigStrategyFlagsCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    edict_t* pEdict = ( pClient ) ? pClient->GetEdict() : NULL;

    if ( argc == 0 )
        BULOG_I( pEdict, "Bot's strategy flags: %s.",
                 CTypeToString::StrategyFlagsToString(CBot::iDefaultFightStrategy).c_str() );
    else
    {
        TFightStrategyFlags iFlags = 0;
        for ( int i = 0; i < argc; ++i )
        {
            TFightStrategyFlags iFlag = CTypeToString::StrategyFlagsFromString(argv[i]);
            if ( iFlag == -1 )
            {
                BULOG_W( pEdict, "Error, invalid strategy flag: %s.", argv[i] );
                BULOG_W( pEdict, "Can be one of: %s.",
                         CTypeToString::StrategyFlagsToString(FFightStrategyAll).c_str() );
                return ECommandError;
            }
            FLAG_SET(iFlag, iFlags);
        }
        CBot::iDefaultFightStrategy = iFlags;
    }
    return ECommandPerformed;
}

TCommandResult CBotConfigStrategySetCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    edict_t* pEdict = ( pClient ) ? pClient->GetEdict() : NULL;

    if ( argc == 0 )
    {
        BULOG_I( pEdict, "Bot's strategy arguments:" );
        BULOG_I( pEdict, "  %s = %d", CTypeToString::StrategyArgToString(EFightStrategyArgNearDistance).c_str(),
                 (int)FastSqrt(CBot::fNearDistanceSqr) );
        BULOG_I( pEdict, "  %s = %d", CTypeToString::StrategyArgToString(EFightStrategyArgFarDistance).c_str(),
                 (int)FastSqrt(CBot::fFarDistanceSqr) );
    }
    else if ( argc == 2 )
    {
        TFightStrategyArg iArg = CTypeToString::StrategyArgFromString(argv[0]);
        if ( iArg == -1 )
        {
            BULOG_W( pEdict, "Error, invalid strategy argument: %s.", argv[0] );
            BULOG_W( pEdict, "Can be one of: %s.", CTypeToString::StrategyArgs().c_str() );
            return ECommandError;
        }
        int iArgValue;
        int iCount = sscanf(argv[1], "%d", &iArgValue);
        if ( (iCount != 1) || (iArgValue < 0) )
        {
            BULOG_W( pEdict, "Error, invalid number: %s.", argv[1] );
            return ECommandError;
        }
        if ( iArgValue > CUtil::iMaxMapSize )
            iArgValue = CUtil::iMaxMapSize;
        switch ( iArg )
        {
        case EFightStrategyArgNearDistance:
            CBot::fNearDistanceSqr = SQR(iArgValue);
            break;
        case EFightStrategyArgFarDistance:
            CBot::fFarDistanceSqr = SQR(iArgValue);
            break;
        default:
            GoodAssert(false);
        }
    }
    else
    {
        BULOG_W( pEdict, "Error, invalid arguments count." );
        return ECommandError;
   }
    return ECommandPerformed;
}

TCommandResult CBotDrawPathCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    edict_t* pEdict = ( pClient ) ? pClient->GetEdict() : NULL;

    if ( argc == 0 )
    {
        const good::string& sTypes = CTypeToString::PathDrawFlagsToString(CWaypointNavigator::iPathDrawFlags);
        BULOG_I( pEdict, "Bot's path draw flags: %s.", (sTypes.size() > 0) ? sTypes.c_str() : sNone.c_str() );
        return ECommandPerformed;
    }

    // Retrieve flags from string arguments.
    bool bFinished = false;
    TPathDrawFlags iFlags = FPathDrawNone;

    if ( argc == 1 )
    {
        if ( sHelp == argv[0] )
        {
            PrintCommand( pClient->GetEdict() );
            return ECommandPerformed;
        }
        else if ( sNone == argv[0] )
            bFinished = true;
        else if ( sAll == argv[0] )
        {
            iFlags = FPathDrawAll;
            bFinished = true;
        }
        else if ( sNext == argv[0] )
        {
            int iNew = (CWaypointNavigator::iPathDrawFlags)  ?  CWaypointNavigator::iPathDrawFlags << 1  :  1;
            iFlags = (iNew > FPathDrawAll) ? 0 : iNew;
            bFinished = true;
        }
    }

    if ( !bFinished )
    {
        for ( int i=0; i < argc; ++i )
        {
            int iAddFlag = CTypeToString::PathDrawFlagsFromString(argv[i]);
            if ( iAddFlag == -1 )
            {
                BULOG_W( pEdict, "Error, invalid draw type(s). Can be 'none' / 'all' / 'next' or mix of: %s", CTypeToString::PathDrawFlagsToString(FPathDrawAll).c_str() );
                return ECommandError;
            }
            FLAG_SET(iAddFlag, iFlags);
        }
    }

    CWaypointNavigator::iPathDrawFlags = iFlags;
    BULOG_I(pEdict, "Bot's path drawing is %s.", iFlags ? "on" : "off");
    return ECommandPerformed;
}

TCommandResult CBotPauseCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    edict_t* pEdict = ( pClient ) ? pClient->GetEdict() : NULL;

    if ( argc == 0 )
    {
        for ( TPlayerIndex i = 0; i < CPlayers::Size(); ++i )
        {
            CPlayer* pPlayer = CPlayers::Get(i);
            if ( pPlayer && pPlayer->IsBot() )
            {
                ((CBot*)pPlayer)->SetPaused( !((CBot*)pPlayer)->IsPaused() );
            }
        }
        return ECommandPerformed;
    }

    if ( argc > 2 )
    {
        BULOG_W( pEdict, "Error, invalid arguments count." );
        return ECommandError;
    }

    good::string sName = argv[0];

    CBot* pBot = NULL;
    for ( TPlayerIndex i = 0; i < CPlayers::Size(); ++i )
    {
        CPlayer* pPlayer = CPlayers::Get(i);
        if ( pPlayer && pPlayer->IsBot() )
        {
            good::string sBotName = pPlayer->GetName();
            if ( good::starts_with(sBotName, sName) )
            {
                pBot = (CBot*)pPlayer;
                break;
            }
        }
    }

    if ( pBot )
    {
        int bPaused = !pBot->IsPaused();
        if ( argc == 2 )
        {
            bPaused = CTypeToString::BoolFromString(argv[1]);
            if ( bPaused == -1 )
            {
                BULOG_W( pEdict, "Error, unknown parameter %s, should be 'on' or 'off'.", argv[1] );
                return ECommandError;
            }
        }
        pBot->SetPaused(bPaused ? true : false);
        return ECommandPerformed;
    }
    else
    {
        BULOG_W( pEdict, "Error, no such bot: %s.", argv[0] );
        return ECommandError;
    }
}

TCommandResult CBotTestPathCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    if ( pClient == NULL )
    {
        BLOG_W( "Please login to server to execute this command." );
        return ECommandError;
    }

    TWaypointId iPathFrom = -1, iPathTo = -1;
    if (argc == 0)
    {
        iPathFrom = pClient->iCurrentWaypoint;
        iPathTo = pClient->iDestinationWaypoint;
    }
    else if (argc == 1)
    {
        iPathFrom = pClient->iCurrentWaypoint;
        sscanf(argv[0], "%d", &iPathTo);
    }
    else if (argc == 2)
    {
        sscanf(argv[0], "%d", &iPathFrom);
        sscanf(argv[1], "%d", &iPathTo);
    }

    if ( !CWaypoints::IsValid(iPathFrom) || !CWaypoints::IsValid(iPathTo) || (iPathFrom == iPathTo) )
    {
        BULOG_W(pClient->GetEdict(), "Error, invalid parameters, current or 'destination' waypoints.");
        return ECommandError;
    }

    CPlayer* pPlayer = CPlayers::AddBot();
    if ( pPlayer )
    {
        ((CBot*)pPlayer)->TestWaypoints(iPathFrom, iPathTo);
        BULOG_I( pClient->GetEdict(), "Bot added: %s. Testing path from %d to %d.",
                 pPlayer->GetName(), iPathFrom, iPathTo );
        return ECommandPerformed;
    }
    else
    {
        BULOG_W( pClient->GetEdict(), CMod::pCurrentMod->GetLastError().c_str() );
        return ECommandError;
    }
}

//----------------------------------------------------------------------------------------------------------------
// Item commands.
//----------------------------------------------------------------------------------------------------------------
TCommandResult CItemDrawCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    if ( pClient == NULL )
    {
        BLOG_W( "Please login to server to execute this command." );
        return ECommandError;
    }

    if ( argc == 0 )
    {
        const good::string& sFlags = CTypeToString::EntityTypeFlagsToString(pClient->iItemTypeFlags);
        BULOG_I(pClient->GetEdict(), "Item types to draw: %s.", sFlags.size() ? sFlags.c_str(): "none");
        return ECommandPerformed;
    }

    // Retrieve flags from string arguments.
    bool bFinished = false;
    TItemTypeFlags iFlags = 0;

    if ( argc == 1 )
    {
        if ( sHelp == argv[0] )
        {
            PrintCommand( pClient->GetEdict() );
            return ECommandPerformed;
        }
        else if ( sNone == argv[0] )
            bFinished = true;
        else if ( sAll == argv[0] )
        {
            iFlags = EItemTypeAll;
            bFinished = true;
        }
        else if ( sNext == argv[0] )
        {
            int iNew = (pClient->iItemTypeFlags)  ?  pClient->iItemTypeFlags << 1  :  1;
            iFlags = (iNew > EItemTypeAll) ? 0 : iNew;
            bFinished = true;
        }
    }

    if ( !bFinished )
    {
        for ( int i=0; i < argc; ++i )
        {
            int iAddFlag = CTypeToString::EntityTypeFlagsFromString(argv[i]);
            if ( iAddFlag == -1 )
            {
                BULOG_W( pClient->GetEdict(), "Error, invalid item type(s). Can be 'none' / 'all' / 'next' or mix of: %s", CTypeToString::EntityTypeFlagsToString(EItemTypeAll).c_str() );
                return ECommandError;
            }
            FLAG_SET(iAddFlag, iFlags);
        }
    }

    pClient->iItemTypeFlags = iFlags;
    const good::string& sFlags = CTypeToString::EntityTypeFlagsToString(pClient->iItemTypeFlags);
    BULOG_I(pClient->GetEdict(), "Item types to draw: %s.", sFlags.size() ? sFlags.c_str(): "none");
    return ECommandPerformed;
}

TCommandResult CItemDrawTypeCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    if ( pClient == NULL )
    {
        BLOG_W( "Please login to server to execute this command." );
        return ECommandError;
    }

    if ( argc == 0 )
    {
        const good::string& sFlags = CTypeToString::ItemDrawFlagsToString(pClient->iItemDrawFlags);
        BULOG_I(pClient->GetEdict(), "Draw item flags: %s.", sFlags.size() ? sFlags.c_str(): "none");
        return ECommandPerformed;
    }

    // Retrieve flags from string arguments.
    bool bFinished = false;
    TItemDrawFlags iFlags = FItemDontDraw;

    if ( argc == 1 )
    {
        if ( sHelp == argv[0] )
        {
            PrintCommand( pClient->GetEdict() );
            return ECommandPerformed;
        }
        else if ( sNone == argv[0] )
            bFinished = true;
        else if ( sAll == argv[0] )
        {
            iFlags = FItemDrawAll;
            bFinished = true;
        }
        else if ( sNext == argv[0] )
        {
            int iNew = (pClient->iItemDrawFlags)  ?  pClient->iItemDrawFlags << 1  :  1;
            iFlags = (iNew > FItemDrawAll) ? 0 : iNew;
            bFinished = true;
        }
    }

    if ( !bFinished )
    {
        for ( int i=0; i < argc; ++i )
        {
            int iAddFlag = CTypeToString::ItemDrawFlagsFromString(argv[i]);
            if ( iAddFlag == -1 )
            {
                BULOG_W( pClient->GetEdict(), "Error, invalid draw type(s). Can be 'none' / 'all' / 'next' or mix of: %s", CTypeToString::ItemDrawFlagsToString(FItemDrawAll).c_str() );
                return ECommandError;
            }
            FLAG_SET(iAddFlag, iFlags);
        }
    }

    pClient->iItemDrawFlags = iFlags;
    BULOG_I(pClient->GetEdict(), "Items drawing is %s.", iFlags ? "on" : "off");
    return ECommandPerformed;
}

//----------------------------------------------------------------------------------------------------------------
// Config commands.
//----------------------------------------------------------------------------------------------------------------
TCommandResult CConfigEventsCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    if ( pClient == NULL )
    {
        BLOG_W( "Please login to server to execute this command." );
        return ECommandError;
    }

    if ( argc == 0 )
    {
        BULOG_I(pClient->GetEdict(), "Display game events: %s.", pClient->bDebuggingEvents ?  "on" : "off");
        return ECommandPerformed;
    }

    int iValue = -1;
    if ( argc == 1 )
        iValue = CTypeToString::BoolFromString(argv[0]);

    if ( iValue == -1 )
    {
        BULOG_W(pClient->GetEdict(), "Error, invalid argument (must be 'on' or 'off').");
        return ECommandError;
    }

    pClient->bDebuggingEvents = iValue != 0;
    CPlayers::CheckForDebugging();
    BULOG_I(pClient->GetEdict(), "Display game events: %s.", pClient->bDebuggingEvents ?  "on" : "off");
    return ECommandPerformed;
}

TCommandResult CConfigLogCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    edict_t* pEdict = ( pClient ) ? pClient->GetEdict() : NULL;
    if ( argc == 0 )
    {
        BULOG_I( pEdict, "Console log level: %s.", CTypeToString::LogLevelToString(CUtil::iLogLevel).c_str() );
        return ECommandPerformed;
    }

    good::TLogLevel iLogLevel = -1;
    if ( argc == 1 )
        iLogLevel = CTypeToString::LogLevelFromString(argv[0]);

    if ( iLogLevel == -1 )
    {
        BULOG_W( pEdict, "Error, invalid argument (must be none, trace, debug, info, warning, error)." );
        return ECommandError;
    }

    CUtil::iLogLevel = iLogLevel;
    BULOG_I( pEdict, "Console log level: %s.", argv[0] );
    return ECommandPerformed;
}

TCommandResult CConfigAdminsSetAccessCommand::Execute( CClient* pClient, int argc, const char** argv )
{
    edict_t* pEdict = ( pClient ) ? pClient->GetEdict() : NULL;

    if ( argc < 2 )
    {
        BULOG_W(pEdict, "Error, you show provide steam id and access level.");
        return ECommandError;
    }

    good::string sSteamId = argv[0];
    if ( !good::starts_with(sSteamId, "STEAM_") )
    {
        BULOG_W(pEdict, "Error, steam id should start with \"STEAM_\".");
        return ECommandError;
    }

    good::string_buffer sbBuffer(szMainBuffer, iMainBufferSize, false); // Don't deallocate after use.
    for ( int i=1; i<argc; ++i )
        sbBuffer << argv[i] << " ";
    sbBuffer.erase( sbBuffer.size()-1 ); // Erase last space.

    int iFlags = CTypeToString::AccessFlagsFromString( sbBuffer );
    if ( iFlags == -1 )
    {
        BULOG_W( pEdict, "Error, invalid access flags: %s.", sbBuffer.c_str() );
        return ECommandError;
    }

    CConfiguration::SetClientAccessLevel(sSteamId, iFlags);
    bool bFound = false;
    for (int i = 0; i < CPlayers::Size(); ++i)
    {
        CPlayer* pPlayer = CPlayers::Get(i);
        if ( pPlayer && !pPlayer->IsBot() )
        {
            CClient* pClient = (CClient*)pPlayer;
            if ( sSteamId == pClient->GetSteamID() )
            {
                pClient->iCommandAccessFlags = iFlags;
                BULOG_I( pEdict, "Set access flags: '%s' for %s.", sbBuffer.c_str(), pClient->GetName() );
                bFound = true;
                break;
            }
        }
    }

    if ( !bFound )
        BULOG_I( pEdict, "Set access flags: '%s' for %s.", sbBuffer.c_str(), sSteamId.c_str() );

    return ECommandPerformed;
}

TCommandResult CConfigAdminsShowCommand::Execute( CClient* pClient, int /*argc*/, const char** /*argv*/ )
{
    edict_t* pEdict = ( pClient ) ? pClient->GetEdict() : NULL;

    for (int i = 0; i < CPlayers::Size(); ++i)
    {
        CPlayer* pPlayer = CPlayers::Get(i);
        if ( pPlayer && !pPlayer->IsBot() )
        {
            CClient* pClient = (CClient*)pPlayer;
            if ( pClient->iCommandAccessFlags )
                BULOG_I( pEdict, "Name: %s, access: %s, steam ID: %s.", pClient->GetName(),
                         CTypeToString::AccessFlagsToString(pClient->iCommandAccessFlags).c_str(), pClient->GetSteamID().c_str() );
        }
    }

    return ECommandPerformed;
}


//----------------------------------------------------------------------------------------------------------------
// Static "botrix" command (server side).
//----------------------------------------------------------------------------------------------------------------
#define MAIN_COMMAND "botrix"

#ifdef SOURCE_ENGINE_2006

void bbotCommandCallback()
{
    int argc = MIN2(CBotrixPlugin::pEngineServer->Cmd_Argc(), 16);

    static const char* argv[16];
    for (int i = 0; i < argc; ++i)
        argv[i] = CBotrixPlugin::pEngineServer->Cmd_Argv(i);

#else

void bbotCommandCallback( const CCommand &command )
{
    int argc = command.ArgC();
    const char** argv = command.ArgV();

#endif

    CClient* pClient = CPlayers::GetListenServerClient();

    TCommandResult result = CMainCommand::instance->Execute( pClient, argc-1, &argv[1] );
    if (result == ECommandRequireAccess)
        BULOG_W(pClient ? pClient->GetEdict() : NULL, "Error, you don't have access to this command.");
    else if (result == ECommandNotFound)
        BULOG_W(pClient ? pClient->GetEdict() : NULL, "Error, command not found.");
    else if (result == ECommandError)
        BULOG_W(pClient ? pClient->GetEdict() : NULL, "Command error.");
}


int bbotCompletion( const char* partial, char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ] )
{
    int len = strlen(partial);
    return CMainCommand::instance->AutoComplete(partial, len, commands, 0, 0);
}


ConCommand botrix(MAIN_COMMAND, bbotCommandCallback, "Botrix plugin's commands. " PLUGIN_VERSION " Beta(BUILD " __DATE__ ")\n", FCVAR_NONE, bbotCompletion);


//----------------------------------------------------------------------------------------------------------------
// Main "botrix" command.
//----------------------------------------------------------------------------------------------------------------
CMainCommand::CMainCommand()
{
    m_sCommand = "botrix";
    if ( CBotrixPlugin::instance->IsEnabled() )
    {
        Add(new CBotCommand);
        Add(new CConfigCommand);
        Add(new CItemCommand);
        Add(new CWaypointCommand);
        Add(new CPathCommand);
        Add(new CVersionCommand);
        Add(new CDisableCommand);
    }
    else
        Add(new CEnableCommand);

#ifndef DONT_USE_VALVE_FUNCTIONS
  #ifdef SOURCE_ENGINE_2006
    CBotrixPlugin::pCvar->RegisterConCommandBase( &botrix );
  #else
    CBotrixPlugin::pCVar->RegisterConCommand( &botrix );
  #endif
#endif
}

CMainCommand::~CMainCommand()
{
#if !defined(SOURCE_ENGINE_2006) && !defined(DONT_USE_VALVE_FUNCTIONS)
    CBotrixPlugin::pCVar->UnregisterConCommand( &botrix );
#endif
}
