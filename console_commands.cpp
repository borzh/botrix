#include "bot.h"
#include "client.h"
#include "console_commands.h"
#include "waypoint.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


good::auto_ptr<CMainCommand> CMainCommand::instance;

const good::string sHelp("help"); // TODO: all commands.

const good::string sAll("all");
const good::string sNone("none");
const good::string sNext("next");

const good::string sWeapon = "weapon";
const good::string sAmmo = "ammo";
const good::string sHealth = "health";
const good::string sArmor = "armor";
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
	int result = 0;
	if ( m_sCommand.size() >= partialLength )
	{
		if ( strncmp( m_sCommand.c_str(), partial, partialLength ) == 0 )
		{
			strcpy( &commands[strIndex][charIndex], m_sCommand.c_str() );
			result++;
		}
	}
	return result;
}


void CConsoleCommand::PrintCommand( edict_t* pPrintTo, int indent )
{
	CUtil::SetMessageUseTag(false);

	bool bHasAccess = true;

	if ( pPrintTo )
	{
		int iIdx = CPlayers::Get( pPrintTo );
		CPlayer* pPlayer = CPlayers::Get( iIdx );
		DebugAssert( pPlayer && !pPlayer->IsBot() );
		CClient* pClient = (CClient*)pPlayer;
		bHasAccess = HasAccess(pClient);
	}

	int i;
	for ( i = 0; i < indent*2; i ++ )
		szMainBuffer[i] = ' ';
	szMainBuffer[i]=0;

	const char* sCantUse = bHasAccess ? "" : "[can't use]";
	CUtil::Message( pPrintTo, "%s[%s]%s: %s", szMainBuffer, m_sCommand.c_str(), sCantUse, m_sHelp.c_str() );
	if ( m_sDescription.length() > 0 )
		CUtil::Message( pPrintTo, "%s  %s", szMainBuffer, m_sDescription.c_str() );

	CUtil::SetMessageUseTag(true);
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
	if (argc > 0)
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

	CUtil::SetMessageUseTag(false);
	CUtil::Message( pPrintTo, "%s[%s]", szMainBuffer, m_sCommand.c_str() );

	for ( int i = 0; i < m_commands.size(); i ++ )
		m_commands[i]->PrintCommand( pPrintTo, indent+1 );

	CUtil::SetMessageUseTag(false);
	CUtil::Message( pPrintTo, "" );
	CUtil::SetMessageUseTag(true);
}



//----------------------------------------------------------------------------------------------------------------
// Waypoints commands.
//----------------------------------------------------------------------------------------------------------------
TCommandResult CWaypointDrawFlagCommand::Execute( CClient* pClient, int argc, const char** argv )
{
	if ( pClient == NULL )
		return ECommandError;

	if ( argc == 0 )
	{
		const good::string& sType = CTypeToString::WaypointDrawFlagsToString(pClient->iWaypointDrawFlags);
		CUtil::Message( pClient->GetEdict(), "Waypoint draw flags: %s.", (sType.size() != 0) ? sType.c_str() : sNone.c_str() );
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
				CUtil::Message( pClient->GetEdict(), "Error, invalid draw flag(s). Can be 'none' / 'all' / 'next' or mix of: %s", CTypeToString::WaypointDrawFlagsToString(FWaypointDrawAll).c_str() );
				return ECommandError;
			}
			FLAG_SET(iAddFlag, iFlags);
		}
	}

	pClient->iWaypointDrawFlags = iFlags;
	CUtil::Message(pClient->GetEdict(), "Waypoints drawing is %s.", iFlags ? "on" : "off");
	return ECommandPerformed;
}

TCommandResult CWaypointResetCommand::Execute( CClient* pClient, int argc, const char** argv )
{
	if ( pClient == NULL )
		return ECommandError;

	Vector vOrigin( pClient->GetHead() );
	pClient->iCurrentWaypoint = CWaypoints::GetNearestWaypoint( vOrigin );
	CUtil::Message(pClient->GetEdict(), "Current waypoint %d.", pClient->iCurrentWaypoint);

	return ECommandPerformed;
}

TCommandResult CWaypointCreateCommand::Execute( CClient* pClient, int argc, const char** argv )
{
	if ( pClient == NULL )
		return ECommandError;

	if ( !pClient->IsAlive() )
	{
		CUtil::Message(pClient->GetEdict(), "Error, you need to be alive to create waypoints (bots can't just fly around you know).");
		return ECommandError;
	}

	TWaypointId id = CWaypoints::Add( pClient->GetHead(), FWaypointNone );
	pClient->iCurrentWaypoint = id;

	// Check if player is crouched.
	float height = pClient->GetPlayerInfo()->GetPlayerMaxs().z - pClient->GetPlayerInfo()->GetPlayerMins().z;
	bool bIsCrouched = ( height < CUtil::iPlayerHeight );

	if (pClient->bAutoCreatePaths)
		CWaypoints::CreateAutoPaths(id, bIsCrouched);
	else if ( CWaypoint::IsValid( pClient->iDestinationWaypoint ) )
		CWaypoints::CreatePathsWithAutoFlags( pClient->iDestinationWaypoint, pClient->iCurrentWaypoint, bIsCrouched );

	CUtil::Message(pClient->GetEdict(), "Waypoint %d added.", id);
	return ECommandPerformed;
}

TCommandResult CWaypointRemoveCommand::Execute( CClient* pClient, int argc, const char** argv )
{
	if ( pClient == NULL )
		return ECommandError;

	TWaypointId id = -1;
	if (argc == 0)
		id = pClient->iCurrentWaypoint;
	else if (argc == 1)
		sscanf(argv[0], "%d", &id);

	if ( !CWaypoints::IsValid(id) )
	{
		CUtil::Message(pClient->GetEdict(), "Error, invalid given or current waypoint (move closer to some waypoint).");
		return ECommandError;
	}

	CWaypoints::Remove(id);
	CUtil::Message(pClient->GetEdict(), "Waypoint %d deleted.", id);

	// Invalidate current / destination waypoints for all players.
	for (int i=0; i < CPlayers::GetMaxPlayers(); ++i)
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
		return ECommandError;

	TWaypointId id = -1;
	if (argc == 0)
		id = pClient->iDestinationWaypoint;
	else if ( (argc == 1) || (argc == 4) )
		sscanf(argv[0], "%d", &id);
	else if ( argc != 3 )
	{
		CUtil::Message(pClient->GetEdict(), "Error, invalid arguments count, must be 1 or 3/4 (with given point x/y/z).");
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
		CUtil::Message(pClient->GetEdict(), "Error, invalid given or destination waypoint.");
		return ECommandError;
	}

	CWaypoints::Get(id).vOrigin = vOrigin;
	CUtil::Message(pClient->GetEdict(), "Set new position for waypoint %d (%d, %d, %d).", id, (int)vOrigin.x, (int)vOrigin.y, (int)vOrigin.z);

	return ECommandPerformed;
}

TCommandResult CWaypointAutoCreateCommand::Execute( CClient* pClient, int argc, const char** argv )
{
	if ( pClient == NULL )
		return ECommandError;

	if ( argc == 0 )
	{
		CUtil::Message( pClient->GetEdict(), pClient->bAutoCreateWaypoints ? "Auto create waypoints is on." : "Auto create waypoints is off." );
		return ECommandPerformed;
	}

	int iValue = -1;
	if ( argc == 1 )
		iValue = CTypeToString::BoolFromString(argv[0]);

	if ( iValue == -1 )
	{
		CUtil::Message(pClient->GetEdict(), "Error, invalid argument (must be 'on' or 'off').");
		return ECommandError;
	}
	
	pClient->bAutoCreateWaypoints = iValue != 0;
	CUtil::Message(pClient->GetEdict(), iValue ? "Auto create waypoints is on." : "Auto create waypoints is off.");
	return ECommandPerformed;
}


TCommandResult CWaypointClearCommand::Execute( CClient* pClient, int argc, const char** argv )
{
	if ( pClient == NULL )
		return ECommandError;

	// Invalidate current / next / destination waypoints for all players.
	for ( int i = 0; i < CPlayers::GetMaxPlayers(); ++i )
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
	CUtil::Message(pClient->GetEdict(), "All waypoints deleted.");
	return ECommandPerformed;
}

TCommandResult CWaypointAddTypeCommand::Execute( CClient* pClient, int argc, const char** argv )
{
	if ( pClient == NULL )
		return ECommandError;

	if ( !CWaypoint::IsValid(pClient->iCurrentWaypoint) )
	{
		CUtil::Message(pClient->GetEdict(), "Error, no waypoint nearby to add type (move closer to waypoint).");
		return ECommandError;
	}

	// Retrieve flags from string arguments.
	TWaypointFlags iFlags = FWaypointNone;
	for ( int i=0; i < argc; ++i )
	{
		int iAddFlag = CTypeToString::WaypointFlagsFromString(argv[i]);
		if ( iAddFlag == -1 )
		{
			CUtil::Message( pClient->GetEdict(), "Error, invalid waypoint flag: %s. Can be one of: %s", argv[i], CTypeToString::WaypointFlagsToString(FWaypointAll).c_str() );
			return ECommandError;
		}
		FLAG_SET(iAddFlag, iFlags);
	}

	if ( iFlags == FWaypointNone )
	{
		CUtil::Message(pClient->GetEdict(), "Error, specify at least one waypoint type. Can be one of: %s.", CTypeToString::WaypointFlagsToString(FWaypointAll).c_str());
		return ECommandError;
	}
	else
	{
		CWaypoint& w = CWaypoints::Get(pClient->iCurrentWaypoint);

		bool bAngle1 = FLAG_SOME_SET(FWaypointCamper | FWaypointSniper | FWaypointArmorMachine | FWaypointHealthMachine, w.iFlags);
		bool bAngle2 = FLAG_SOME_SET(FWaypointCamper | FWaypointSniper, w.iFlags);

		bool bWeapon = FLAG_SOME_SET(FWaypointAmmo | FWaypointWeapon, w.iFlags);

		bool bArmor = FLAG_SOME_SET(FWaypointArmor, w.iFlags);
		bool bHealth = FLAG_SOME_SET(FWaypointHealth, w.iFlags);

		if ( (bAngle1 && bWeapon) || ( bAngle2 && (bWeapon || bArmor || bHealth) ) )
		{
			CUtil::Message(pClient->GetEdict(), "Error, you can't mix these waypoint types.");
			return ECommandError;
		}

		FLAG_SET(iFlags, w.iFlags);
		CUtil::Message(pClient->GetEdict(), "Types %s (%d) added to waypoint %d.", CTypeToString::WaypointFlagsToString(iFlags).c_str(), iFlags, pClient->iCurrentWaypoint);
		return ECommandPerformed;
	}
}

TCommandResult CWaypointRemoveTypeCommand::Execute( CClient* pClient, int argc, const char** argv )
{
	if ( pClient == NULL )
		return ECommandError;

	TWaypointId id = -1;
	if (argc == 0)
		id = pClient->iCurrentWaypoint;
	else if (argc == 1)
		sscanf(argv[0], "%d", &id);

	if ( CWaypoints::IsValid(id) )
	{
		CWaypoint& w = CWaypoints::Get(id);
		w.iFlags = FWaypointNone;
		CUtil::Message(pClient->GetEdict(), "Waypoint %d has no type now.", id);
		return ECommandPerformed;
	}
	else
	{
		CUtil::Message(pClient->GetEdict(), "Error, no waypoint nearby to remove type (move closer to waypoint).");
		return ECommandError;
	}
}

TCommandResult CWaypointArgumentCommand::Execute( CClient* pClient, int argc, const char** argv )
{
	if ( pClient == NULL )
		return ECommandError;

	TWaypointId id = pClient->iCurrentWaypoint;
	if ( !CWaypoints::IsValid(id) )
	{
		CUtil::Message(pClient->GetEdict(), "Error, no waypoint nearby (move closer to waypoint).");
		return ECommandError;
	}

	CWaypoint& w = CWaypoints::Get(pClient->iCurrentWaypoint);

	bool bAngle1 = FLAG_SOME_SET(FWaypointCamper | FWaypointSniper | FWaypointArmorMachine | FWaypointHealthMachine, w.iFlags);
	bool bAngle2 = FLAG_SOME_SET(FWaypointCamper | FWaypointSniper, w.iFlags);

	bool bWeapon = FLAG_SOME_SET(FWaypointAmmo | FWaypointWeapon, w.iFlags);

	bool bArmor = FLAG_SOME_SET(FWaypointArmor, w.iFlags);
	bool bHealth = FLAG_SOME_SET(FWaypointHealth, w.iFlags);

	if ( argc == 0 )
	{
		if ( bWeapon )
			CUtil::Message(pClient->GetEdict(), "Weapon index %d, subindex %d.", CWaypoint::GetWeaponIndex(w.iArgument), CWaypoint::GetWeaponSubIndex(w.iArgument));
		if ( FLAG_ALL_SET(FWaypointAmmo, w.iFlags) )
		{
			bool bIsSecondary; int iAmmo = CWaypoint::GetAmmo(bIsSecondary, w.iArgument);
			CUtil::Message(pClient->GetEdict(), "Weapon ammo %d, secondary %s.", iAmmo, bIsSecondary ? "yes" : "no");
		}
		if ( bHealth )
			CUtil::Message(pClient->GetEdict(), "Health %d.", CWaypoint::GetHealth(w.iArgument));
		if ( bArmor )
			CUtil::Message(pClient->GetEdict(), "Armor %d.", w.GetArmor(w.iArgument));
		if ( bAngle1 )
		{
			QAngle a1; CWaypoint::GetFirstAngle(a1, w.iArgument);
			CUtil::Message(pClient->GetEdict(), "Angle 1 (pitch, yaw): (%.0f, %.0f).", a1.x, a1.y);
		}
		if ( bAngle2 )
		{
			QAngle a2; CWaypoint::GetSecondAngle(a2, w.iArgument);
			CUtil::Message(pClient->GetEdict(), "Angle 2 (pitch, yaw): (%.0f, %.0f).", a2.x, a2.y);
		}
		return ECommandPerformed;
	}

	if ( sHelp == argv[0] )
	{
		CUtil::Message(pClient->GetEdict(), "You can mix next arguments:");
		CUtil::Message(pClient->GetEdict(), " - 'weapon' number1 number2: set weapon index/subindex (for ammo and/or weapon)");
		CUtil::Message(pClient->GetEdict(), " - 'health' number: set health amount (also for health machine, 30 by default)");
		CUtil::Message(pClient->GetEdict(), " - 'armor' number: set armor amount (also for armor machine, 30 by default)");
		CUtil::Message(pClient->GetEdict(), " - 'first_angle': set your current angles as waypoint first angles (camper, sniper, machines)");
		CUtil::Message(pClient->GetEdict(), " - 'second_angle': set your current angles as second angles (camper, sniper)");
		return ECommandPerformed;
	}

	int iArgument = w.iArgument;
	QAngle angClient;
	pClient->GetEyeAngles(angClient);
	CUtil::DeNormalizeAngle(angClient.x);
	CUtil::DeNormalizeAngle(angClient.y);
	DebugAssert( -90.0f <= angClient.x && angClient.x <= 90.0f);

	for ( int i=0; i < argc; ++i )
	{
		if ( sWeapon == argv[i] )
		{
			if ( i+2 >= argc )
			{
				CUtil::Message(pClient->GetEdict(), "Error, you must provide 2 arguments for weapon (index and subindex).");
				return ECommandError;
			}
			if ( bAngle1 )
			{
				CUtil::Message(pClient->GetEdict(), "Error, you can't mix weapon/ammo with angles.");
				return ECommandError;
			}
			if ( !bWeapon )
			{
				CUtil::Message(pClient->GetEdict(), "Error, first you need to set waypoint type accordingly (weapon/ammo).");
				return ECommandError;
			}

			int i1 = -1, i2 = -1;
			sscanf(argv[++i], "%d", &i1);
			sscanf(argv[++i], "%d", &i2);

			if ( (i1 < 0) || (i1 > 15) || (i2 < 0) || (i2 > 15) )
			{
				CUtil::Message(pClient->GetEdict(), "Error, invalid weapon arguments (must be from 0 to 15).");
				return ECommandError;
			}
			CWaypoint::SetWeapon(i1, i2, iArgument);
		}

		else if ( sAmmo == argv[i] )
		{
			if ( i+2 >= argc )
			{
				CUtil::Message(pClient->GetEdict(), "Error, you must provide 2 arguments to ammo (ammo amount and 0=primary/1=secondary).");
				return ECommandError;
			}
			if ( bAngle1 )
			{
				CUtil::Message(pClient->GetEdict(), "Error, you can't mix weapon/ammo with angles.");
				return ECommandError;
			}
			if ( !FLAG_SOME_SET(FWaypointAmmo, w.iFlags) )
			{
				CUtil::Message(pClient->GetEdict(), "Error, first you need to set waypoint type accordingly (ammo).");
				return ECommandError;
			}

			int i1 = -1, i2 = -1;
			sscanf(argv[++i], "%d", &i1);
			sscanf(argv[++i], "%d", &i2);

			if ( (i1 < 0) || (i1 > 127) || (i2 < 0) || (i2 > 1) )
			{
				CUtil::Message(pClient->GetEdict(), "Error, invalid ammo arguments (must be 0 .. 127 / 0 .. 1).");
				return ECommandError;
			}
			CWaypoint::SetAmmo(i1, i2 != 0, iArgument);
		}

		else if ( sHealth == argv[i] )
		{
			if ( i+1 >= argc )
			{
				CUtil::Message(pClient->GetEdict(), "Error, you must provide 1 argument to health/health_machine (health amount).");
				return ECommandError;
			}
			if ( bAngle2 )
			{
				CUtil::Message(pClient->GetEdict(), "Error, you can't mix health with 2 angles.");
				return ECommandError;
			}
			if ( !bHealth )
			{
				CUtil::Message(pClient->GetEdict(), "Error, first you need to set waypoint type accordingly (health/health_machine).");
				return ECommandError;
			}

			int i1 = -1;
			sscanf(argv[++i], "%d", &i1);

			if ( (i1 < 0) || (i1 > 100) )
			{
				CUtil::Message(pClient->GetEdict(), "Error, invalid health argument (must be from 0 to 100).");
				return ECommandError;
			}
			CWaypoint::SetHealth(i1, iArgument);
		}

		else if ( sArmor == argv[i] )
		{
			if ( i+1 >= argc )
			{
				CUtil::Message(pClient->GetEdict(), "Error, you must provide 1 argument to armor/armor_machine (armor amount).");
				return ECommandError;
			}
			if ( bAngle2 )
			{
				CUtil::Message(pClient->GetEdict(), "Error, you can't mix armor with 2 angles.");
				return ECommandError;
			}
			if ( !bArmor )
			{
				CUtil::Message(pClient->GetEdict(), "Error, first you need to set waypoint type accordingly (armor/armor_machine).");
				return ECommandError;
			}

			int i1 = -1;
			sscanf(argv[++i], "%d", &i1);

			if ( (i1 < 0) || (i1 > 100) )
			{
				CUtil::Message(pClient->GetEdict(), "Error, invalid armor argument (must be from 0 to 100).");
				return ECommandError;
			}
			CWaypoint::SetArmor(i1, iArgument);
		}

		else if ( sFirstAngle == argv[i] )
		{
			if ( bWeapon )
			{
				CUtil::Message(pClient->GetEdict(), "Error, you can't mix weapon/ammo with angles.");
				return ECommandError;
			}
			if ( !bAngle1 )
			{
				CUtil::Message(pClient->GetEdict(), "Error, first you need to set waypoint type accordingly (camper/sniper/armor_machine/health_machine).");
				return ECommandError;
			}
			
			int iPitch = angClient.x;
			int iYaw = angClient.y;
			if ( (CWaypoint::MIN_ANGLE_PITCH <= iPitch) && (iPitch <= CWaypoint::MAX_ANGLE_PITCH) &&
				 (CWaypoint::MIN_ANGLE_PITCH <= iPitch) && (iPitch <= CWaypoint::MAX_ANGLE_PITCH) )
				 CWaypoint::SetFirstAngle(iPitch, iYaw, iArgument);
			else
			{
				CUtil::Message(pClient->GetEdict(), "Error, up/down angle (pitch) must be from -64 to 63.");
				return ECommandError;
			}
		}

		else if ( sSecondAngle == argv[i] )
		{
			if ( bWeapon || bArmor || bHealth )
			{
				CUtil::Message(pClient->GetEdict(), "Error, you can't mix weapon/ammo/health/armor with two angles.");
				return ECommandError;
			}
			if ( !bAngle2 )
			{
				CUtil::Message(pClient->GetEdict(), "Error, first you need to set waypoint type accordingly (camper/sniper).");
				return ECommandError;
			}
			
			int iPitch = angClient.x;
			int iYaw = angClient.y;
			if ( (CWaypoint::MIN_ANGLE_PITCH <= iPitch) && (iPitch <= CWaypoint::MAX_ANGLE_PITCH) &&
				 (CWaypoint::MIN_ANGLE_PITCH <= iPitch) && (iPitch <= CWaypoint::MAX_ANGLE_PITCH) )
				 CWaypoint::SetSecondAngle(iPitch, iYaw, iArgument);
			else
			{
				CUtil::Message(pClient->GetEdict(), "Error, up/down angle (pitch) must be from -64 to 63.");
				return ECommandError;
			}
		}

		else
		{
			CUtil::Message(pClient->GetEdict(), "Error, invalid argument %s.", argv[i]);
			return ECommandError;
		}
	}

	w.iArgument = iArgument;
	return ECommandPerformed;
}

TCommandResult CWaypointInfoCommand::Execute( CClient* pClient, int argc, const char** argv )
{
	if ( pClient == NULL )
		return ECommandError;

	TWaypointId id = -1;
	if (argc == 0)
		id = pClient->iCurrentWaypoint;
	else if (argc == 1)
		sscanf(argv[0], "%d", &id);

	if ( CWaypoints::IsValid(id) )
	{
		CWaypoint& w = CWaypoints::Get(id);
		const good::string& sFlags = CTypeToString::WaypointFlagsToString(w.iFlags);
		CUtil::Message(pClient->GetEdict(), "Waypoint id %d: flags %s", id, (sFlags.size() > 0) ? sFlags.c_str() : sNone.c_str());
		CUtil::Message(pClient->GetEdict(), "Origin: %.0f %.0f %.0f", w.vOrigin.x, w.vOrigin.y, w.vOrigin.z);
		return ECommandPerformed;
	}
	else
	{
		CUtil::Message(pClient->GetEdict(), "Error, no waypoint nearby (move closer to waypoint).");
		return ECommandError;
	}
}

TCommandResult CWaypointDestinationCommand::Execute( CClient* pClient, int argc, const char** argv )
{
	if ( pClient == NULL )
		return ECommandError;

	TWaypointId id = -1;
	if (argc == 0)
		id = pClient->iCurrentWaypoint;
	else if (argc == 1)
		sscanf(argv[0], "%d", &id);

	if ( CWaypoint::IsValid(id) )
	{
		pClient->iDestinationWaypoint = id;
		CUtil::Message(pClient->GetEdict(), "Path 'destination' is waypoint %d.", id );
		return ECommandPerformed;
	}
	else
	{
		CUtil::Message(pClient->GetEdict(), "Error, invalid given or current waypoint (move closer to waypoint).");
		return ECommandError;
	}
}

TCommandResult CWaypointSaveCommand::Execute( CClient* pClient, int argc, const char** argv )
{
	if ( pClient == NULL )
		return ECommandError;

	if ( CWaypoints::Save() )
	{
		CUtil::Message(pClient->GetEdict(), "%d waypoints saved.", CWaypoints::Size());
		return ECommandPerformed;
	}
	else
	{
		CUtil::Message(pClient->GetEdict(), "Error, could not save waypoints.");
		return ECommandError;
	}
}

TCommandResult CWaypointLoadCommand::Execute( CClient* pClient, int argc, const char** argv )
{
	if ( pClient == NULL )
		return ECommandError;

	if ( CWaypoints::Load() )
	{
		CUtil::Message(pClient->GetEdict(), "%d waypoints loaded.", CWaypoints::Size());
		return ECommandPerformed;
	}
	else
	{
		CUtil::Message(pClient->GetEdict(), "Error, could not load waypoints.");
		return ECommandError;
	}
}


//----------------------------------------------------------------------------------------------------------------
// Paths commands.
//----------------------------------------------------------------------------------------------------------------
/*
TCommandResult CPathSwapCommand::Execute( CClient* pClient, int argc, const char** argv )
{
	if ( pClient == NULL )
		return ECommandError;

	if ( argc > 0 && argv[0] )
	{
		int iValue = pClient->iDestinationWaypoint;
		sscanf(argv[0], "%d", &iValue);
		if ( !CWaypoints::IsValid(iValue) )
		{
			CUtil::Message(pClient->GetEdict(), "Error, invalid parameter.");
			return ECommandError;
		}
		pClient->iDestinationWaypoint = iValue;
	}

	if ( !CWaypoint::IsValid(pClient->iDestinationWaypoint) )
	{
		CUtil::Message(pClient->GetEdict(), "Error, you need to set 'destination' waypoint first.");
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

	CUtil::Message(pClient->GetEdict(), "Teleported to %d. Path destination now is %d", pClient->iCurrentWaypoint, pClient->iDestinationWaypoint);

	return ECommandPerformed;
}
*/
TCommandResult CPathDrawCommand::Execute( CClient* pClient, int argc, const char** argv )
{
	if ( pClient == NULL )
		return ECommandError;

	if ( argc == 0 )
	{
		const good::string& sType = CTypeToString::PathDrawFlagsToString(pClient->iPathDrawFlags);
		CUtil::Message( pClient->GetEdict(), "Path draw flags: %s.", (sType.size() > 0) ? sType.c_str() : sNone.c_str());
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
				CUtil::Message( pClient->GetEdict(), "Error, invalid draw flag(s). Can be 'none' / 'all' / 'next' or mix of: %s", CTypeToString::PathDrawFlagsToString(FWaypointDrawAll).c_str() );
				return ECommandError;
			}
			FLAG_SET(iAddFlag, iFlags);
		}
	}

	pClient->iPathDrawFlags = iFlags;
	CUtil::Message(pClient->GetEdict(), "Path drawing is %s.", iFlags ? "on" : "off");
	return ECommandPerformed;
}

TCommandResult CPathCreateCommand::Execute( CClient* pClient, int argc, const char** argv )
{
	if ( pClient == NULL )
		return ECommandError;

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
		CUtil::Message(pClient->GetEdict(), "Error, invalid parameters, current or 'destination' waypoints.");
		return ECommandError;
	}

	TPathFlags iFlags = FPathNone;
	if ( pClient->IsAlive() )
	{
		float zDist = pClient->GetPlayerInfo()->GetPlayerMaxs().z - pClient->GetPlayerInfo()->GetPlayerMins().z;
		if (zDist < CUtil::iPlayerHeight)
			iFlags = FPathCrouch;
	}

	if ( CWaypoints::AddPath(iPathFrom, iPathTo, 0, iFlags) )
	{
		CUtil::Message(pClient->GetEdict(), "Path created: from %d to %d.", iPathFrom, iPathTo);
		return ECommandPerformed;
	}
	else
	{
		CUtil::Message(pClient->GetEdict(), "Error creating path from %d to %d (exists or too big distance?).", iPathFrom, iPathTo);
		return ECommandError;
	}
}

TCommandResult CPathRemoveCommand::Execute( CClient* pClient, int argc, const char** argv )
{
	if ( pClient == NULL )
		return ECommandError;

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
		CUtil::Message(pClient->GetEdict(), "Error, invalid parameters, current or 'destination' waypoints.");
		return ECommandError;
	}

	if ( CWaypoints::RemovePath(iPathFrom, iPathTo) )
	{
		CUtil::Message(pClient->GetEdict(), "Path removed: from %d to %d.", iPathFrom, iPathTo);
		return ECommandPerformed;	
	}
	else
	{
		CUtil::Message(pClient->GetEdict(), "Error, there is no path from %d to %d.", iPathFrom, iPathTo);
		return ECommandError;
	}
}

TCommandResult CPathAutoCreateCommand::Execute( CClient* pClient, int argc, const char** argv )
{
	if ( pClient == NULL )
		return ECommandError;

	if ( argc == 0 )
	{
		CUtil::Message(pClient->GetEdict(), "Waypoint's auto paths creation is: %s.", CTypeToString::BoolToString(pClient->bAutoCreatePaths).c_str());
		return ECommandPerformed;
	}

	int iValue = -1;
	if ( argc == 1 )
		iValue = CTypeToString::BoolFromString(argv[0]);

	if ( iValue == -1 )
	{
		CUtil::Message(pClient->GetEdict(), "Error, invalid argument (must be 'on' or 'off').");
		return ECommandError;
	}
	
	pClient->bAutoCreatePaths = iValue != 0;
	CUtil::Message(pClient->GetEdict(), "Waypoint's auto paths creation is: %s.", CTypeToString::BoolToString(pClient->bAutoCreatePaths).c_str());
	return ECommandPerformed;
}

TCommandResult CPathAddTypeCommand::Execute( CClient* pClient, int argc, const char** argv )
{
	if ( pClient == NULL )
		return ECommandError;

	// Get 'destination' and current waypoints IDs.
	TWaypointId iPathFrom = pClient->iCurrentWaypoint;
	TWaypointId iPathTo = pClient->iDestinationWaypoint;

	if ( !CWaypoints::IsValid(iPathFrom) )
	{
		CUtil::Message(pClient->GetEdict(), "Error, no waypoint nearby (move closer to waypoint).");
		return ECommandError;
	}
	if ( !CWaypoints::IsValid(iPathTo) )
	{
		CUtil::Message(pClient->GetEdict(), "Error, you need to set 'destination' waypoint first.");
		return ECommandError;
	}

	// Retrieve flags from string arguments.
	TPathFlags iFlags = FPathNone;
	for ( int i=0; i < argc; ++i )
	{
		int iAddFlag = CTypeToString::PathFlagsFromString(argv[i]);
		if ( iAddFlag == -1 )
		{
			CUtil::Message( pClient->GetEdict(), "Error, invalid path flag: %s. Can be one of: %s", argv[i], CTypeToString::PathFlagsToString(FPathAll).c_str() );
			return ECommandError;
		}
		FLAG_SET(iAddFlag, iFlags);
	}

	if ( iFlags == FPathNone )
	{
		CUtil::Message( pClient->GetEdict(), "Error, at least one path flag is needed. Can be one of: %s", CTypeToString::PathFlagsToString(FPathAll).c_str() );
		return ECommandError;
	}

	CWaypointPath* pPath = CWaypoints::GetPath(iPathFrom, pClient->iDestinationWaypoint);
	if ( pPath )
	{
		FLAG_SET(iFlags, pPath->iFlags);
		CUtil::Message(pClient->GetEdict(), "Added path types %s (path from %d to %d).", CTypeToString::PathFlagsToString(iFlags).c_str(), iPathFrom, iPathTo);
		return ECommandPerformed;
	}
	else
	{
		CUtil::Message(pClient->GetEdict(), "Error, no path from %d to %d.", iPathFrom, iPathTo);
		return ECommandError;
	}
}

TCommandResult CPathRemoveTypeCommand::Execute( CClient* pClient, int argc, const char** argv )
{
	if ( pClient == NULL )
		return ECommandError;

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
		CUtil::Message(pClient->GetEdict(), "Error, invalid parameters, current or 'destination' waypoints.");
		return ECommandError;
	}

	CWaypointPath* pPath = CWaypoints::GetPath(iPathFrom, iPathTo);
	if ( pPath )
	{
		pPath->iFlags = FPathNone;
		CUtil::Message(pClient->GetEdict(), "Removed all types for path from %d to %d.", iPathFrom, pClient->iDestinationWaypoint);
		return ECommandPerformed;
	}
	else
	{
		CUtil::Message(pClient->GetEdict(), "Error, no path from %d to %d.", iPathFrom, pClient->iDestinationWaypoint);
		return ECommandError;
	}
}

TCommandResult CPathArgumentCommand::Execute( CClient* pClient, int argc, const char** argv )
{
	if ( pClient == NULL )
		return ECommandError;

	if ( !CWaypoints::IsValid(pClient->iCurrentWaypoint) || !CWaypoints::IsValid(pClient->iDestinationWaypoint) )
	{
		CUtil::Message(pClient->GetEdict(), "Error, invalid current or 'destination' waypoints.");
		return ECommandError;
	}

	CWaypointPath* pPath = CWaypoints::GetPath(pClient->iCurrentWaypoint, pClient->iDestinationWaypoint);
	if ( pPath == NULL )
	{
		CUtil::Message(pClient->GetEdict(), "Error, no path from %d to %d.", pClient->iCurrentWaypoint, pClient->iDestinationWaypoint);
		return ECommandError;
	}

	if (argc == 0)
	{
		CUtil::Message( pClient->GetEdict(), "Path (from %d to %d) action time %d, action duration %d. Time in deciseconds.", 
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
			CUtil::Message(pClient->GetEdict(), "Error, invalid parameters, must be from 0 to 256.");
			return ECommandError;
		}

		pPath->iArgument = iFirst | (iSecond << 8);
		CUtil::Message( pClient->GetEdict(), "Set path (from %d to %d) action time %d, action duration %d. Time in deciseconds.", 
		                pClient->iCurrentWaypoint, pClient->iDestinationWaypoint, GET_1ST_BYTE(pPath->iArgument), GET_2ND_BYTE(pPath->iArgument) );
		return ECommandPerformed;
	}
}

TCommandResult CPathInfoCommand::Execute( CClient* pClient, int argc, const char** argv )
{
	if ( pClient == NULL )
		return ECommandError;

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
		CUtil::Message(pClient->GetEdict(), "Error, invalid parameters, current or 'destination' waypoints.");
		return ECommandError;
	}

	CWaypointPath* pPath = CWaypoints::GetPath(iPathFrom, iPathTo);
	if ( pPath )
	{
		if ( pPath->HasDemo() )
			CUtil::Message(pClient->GetEdict(), "Path (from %d to %d) has demo: id %d.", iPathFrom, iPathTo, pPath->DemoNumber());
		else
		{
			const good::string& sFlags = CTypeToString::PathFlagsToString(pPath->iFlags);
			CUtil::Message( pClient->GetEdict(), "Path (from %d to %d) has flags: %s.", iPathFrom, iPathTo, 
			                (sFlags.size() > 0) ? sFlags.c_str() : sNone.c_str() );
			CUtil::Message( pClient->GetEdict(), "Path action time %d, action duration %d. Time in deciseconds.", GET_1ST_BYTE(pPath->iArgument), GET_2ND_BYTE(pPath->iArgument) );
		}
		return ECommandPerformed;
	}
	else
	{
		CUtil::Message(pClient->GetEdict(), "Error, no path from %d to %d.", iPathFrom, pClient->iDestinationWaypoint);
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
			const CWeapon* pWeapon = CWeapons::Get(i);
			CUtil::Message(pEdict, "%s is %s.", pWeapon->pWeaponClass->sClassName.c_str(), pWeapon->bForbidden ? "forbidden" : "allowed" );
		}
	}
	else if ( (argc == 1) && (sAll == argv[0]) ) // Apply to all weapons.
	{
		for ( TWeaponId i=0; i < CWeapons::Size(); ++i )
		{
			const CWeapon* pWeapon = CWeapons::Get(i);
			((CWeapon*)pWeapon)->bForbidden = bForbid;
			CUtil::Message(pEdict, "%s is %s.", pWeapon->pWeaponClass->sClassName.c_str(), pWeapon->bForbidden ? "forbidden" : "allowed" );
		}
	}
	else
	{
		for ( int i=0; i < argc; ++i )
		{
			TWeaponId iWeaponId = CWeapons::GetIdFromWeaponClass( argv[i] );
			if ( CWeapon::IsValid(iWeaponId) )
			{
				const CWeapon* pWeapon = CWeapons::Get(iWeaponId);
				((CWeapon*)pWeapon)->bForbidden = bForbid;
				CUtil::Message(pEdict, "%s is %s.", argv[i], bForbid ? "forbidden" : "allowed" );
			}
			else
				CUtil::Message(pEdict, "Warning, no such weapon: %s, skipping.", argv[i]);
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


TCommandResult CBotAddCommand::Execute( CClient* pClient, int argc, const char** argv )
{	
	if ( pClient == NULL )
		return ECommandError;

	// TODO: bot's intelligence, name.
	TBotIntelligence iIntelligence = EBotIntelligenceTotal-1;//rand() % EBotIntelligenceTotal;
	CPlayer* pPlayer = CPlayers::AddBot(iIntelligence);
	if ( pPlayer )
	{
		CUtil::Message(pClient->GetEdict(), "Bot added: %s (%d).", pPlayer->GetName(), iIntelligence);
		return ECommandPerformed;
	}
	else
	{
		CUtil::Message(pClient->GetEdict(), "Error, couldn't create bot (check maxplayers).");
		return ECommandError;
	}
}

TCommandResult CBotKickCommand::Execute( CClient* pClient, int argc, const char** argv )
{
	if ( pClient == NULL )
		return ECommandError;

	if ( argc == 0 || argv[0] == NULL )
	{
		CPlayers::KickRandomBot();
	}
	else
	{
		int team = atoi(argv[0]);
		CPlayers::KickRandomBotOnTeam(team);
	}
	return ECommandPerformed;
}

TCommandResult CBotDebugCommand::Execute( CClient* pClient, int argc, const char** argv )
{
	if ( pClient == NULL )
		return ECommandError;

	if ( argc == 0 )
	{
		CUtil::Message( pClient->GetEdict(), "Error, you need to provide bot's name (or at least how it starts) or 'all'/'none'."); // TODO: all none.
		return ECommandError;
	}
	if ( argc > 2 )
	{
		CUtil::Message( pClient->GetEdict(), "Error, invalid arguments count.");
		return ECommandError;
	}

	good::string sName = argv[0];

	CBot* pBot = NULL;
	for ( int i=0; i < CPlayers::GetMaxPlayers(); ++i )
	{
		CPlayer* pPlayer = CPlayers::Get(i);
		if ( pPlayer && pPlayer->IsBot() )
		{
			good::string sBotName = pPlayer->GetName();
			if ( sBotName.starts_with(sName) )
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
				CUtil::Message( pClient->GetEdict(), "Error, unknown parameter %s, should be 'on' or 'off'.", argv[1] );
				return ECommandError;
			}
		}
		pBot->SetDebugging(bDebug != 0);
		return ECommandPerformed;
	}
	else
	{
		CUtil::Message( pClient->GetEdict(), "Error, no such bot: %s.", argv[0] );
		return ECommandError;
	}
}

TCommandResult CBotDrawPathCommand::Execute( CClient* pClient, int argc, const char** argv )
{
	if ( pClient == NULL )
		return ECommandError;

	if ( argc == 0 )
	{
		const good::string& sTypes = CTypeToString::PathDrawFlagsToString(CWaypointNavigator::iPathDrawFlags);
		CUtil::Message( pClient->GetEdict(), "Bot's path draw flags: %s.", (sTypes.size() > 0) ? sTypes.c_str() : sNone.c_str() );
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
				CUtil::Message( pClient->GetEdict(), "Error, invalid draw type(s). Can be 'none' / 'all' / 'next' or mix of: %s", CTypeToString::PathDrawFlagsToString(FPathDrawAll).c_str() );
				return ECommandError;
			}
			FLAG_SET(iAddFlag, iFlags);
		}
	}

	CWaypointNavigator::iPathDrawFlags = iFlags;
	CUtil::Message(pClient->GetEdict(), "Bot's path drawing is %s.", iFlags ? "on" : "off");
	return ECommandPerformed;
}

TCommandResult CBotTestPathCommand::Execute( CClient* pClient, int argc, const char** argv )
{
	if ( pClient == NULL )
		return ECommandError;

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
		CUtil::Message(pClient->GetEdict(), "Error, invalid parameters, current or 'destination' waypoints.");
		return ECommandError;
	}

	CPlayer* pPlayer = CPlayers::AddBot();
	if ( pPlayer )
	{
		DebugAssert( pPlayer->IsBot() );
		((CBot*)pPlayer)->TestWaypoints(iPathFrom, iPathTo);
		CUtil::Message(pClient->GetEdict(), "Bot added: %s. Testing path from %d to %d.", pPlayer->GetName(), iPathFrom, iPathTo);
		return ECommandPerformed;
	}
	else
	{
		CUtil::Message(pClient->GetEdict(), "Error, couldn't create bot (check maxplayers).");
		return ECommandError;
	}
}

//----------------------------------------------------------------------------------------------------------------
// Item commands.
//----------------------------------------------------------------------------------------------------------------
TCommandResult CItemDrawCommand::Execute( CClient* pClient, int argc, const char** argv )
{
	if ( pClient == NULL )
		return ECommandError;

	if ( argc == 0 )
	{
		const good::string& sFlags = CTypeToString::EntityTypeFlagsToString(pClient->iItemTypeFlags);
		CUtil::Message(pClient->GetEdict(), "Item types to draw: %s.", sFlags.size() ? sFlags.c_str(): "none");
		return ECommandPerformed;
	}

	// Retrieve flags from string arguments.
	bool bFinished = false;
	TEntityTypeFlags iFlags = 0;

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
				CUtil::Message( pClient->GetEdict(), "Error, invalid item type(s). Can be 'none' / 'all' / 'next' or mix of: %s", CTypeToString::EntityTypeFlagsToString(EItemTypeAll).c_str() );
				return ECommandError;
			}
			FLAG_SET(iAddFlag, iFlags);
		}
	}

	pClient->iItemTypeFlags = iFlags;
	const good::string& sFlags = CTypeToString::EntityTypeFlagsToString(pClient->iItemTypeFlags);
	CUtil::Message(pClient->GetEdict(), "Item types to draw: %s.", sFlags.size() ? sFlags.c_str(): "none");
	return ECommandPerformed;
}

TCommandResult CItemDrawTypeCommand::Execute( CClient* pClient, int argc, const char** argv )
{
	if ( pClient == NULL )
		return ECommandError;

	if ( argc == 0 )
	{
		const good::string& sFlags = CTypeToString::ItemDrawFlagsToString(pClient->iItemDrawFlags);
		CUtil::Message(pClient->GetEdict(), "Draw item flags: %s.", sFlags.size() ? sFlags.c_str(): "none");
		return ECommandPerformed;
	}

	// Retrieve flags from string arguments.
	bool bFinished = false;
	TItemDrawFlags iFlags = EItemDontDraw;

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
			iFlags = EItemDrawAll;
			bFinished = true;
		}
		else if ( sNext == argv[0] )
		{
			int iNew = (pClient->iItemDrawFlags)  ?  pClient->iItemDrawFlags << 1  :  1; 
			iFlags = (iNew > EItemDrawAll) ? 0 : iNew;
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
				CUtil::Message( pClient->GetEdict(), "Error, invalid draw type(s). Can be 'none' / 'all' / 'next' or mix of: %s", CTypeToString::ItemDrawFlagsToString(EItemDrawAll).c_str() );
				return ECommandError;
			}
			FLAG_SET(iAddFlag, iFlags);
		}
	}

	pClient->iItemDrawFlags = iFlags;
	CUtil::Message(pClient->GetEdict(), "Items drawing is %s.", iFlags ? "on" : "off");
	return ECommandPerformed;
}

//----------------------------------------------------------------------------------------------------------------
// Config commands.
//----------------------------------------------------------------------------------------------------------------
TCommandResult CConfigEventsCommand::Execute( CClient* pClient, int argc, const char** argv )
{
	if ( pClient == NULL )
		return ECommandError;

	if ( argc == 0 )
	{
		CUtil::Message(pClient->GetEdict(), "Display game events: %s.", pClient->bDebuggingEvents ?  "on" : "off");
		return ECommandPerformed;
	}

	int iValue = -1;
	if ( argc == 1 )
		iValue = CTypeToString::BoolFromString(argv[0]);

	if ( iValue == -1 )
	{
		CUtil::Message(pClient->GetEdict(), "Error, invalid argument (must be 'on' or 'off').");
		return ECommandError;
	}
	
	pClient->bDebuggingEvents = iValue != 0;
	CPlayers::CheckForDebugging();
	CUtil::Message(pClient->GetEdict(), "Display game events: %s.", pClient->bDebuggingEvents ?  "on" : "off");
	return ECommandPerformed;
}

TCommandResult CConfigAdminsShowCommand::Execute( CClient* pClient, int argc, const char** argv )
{
	if ( pClient == NULL )
		return ECommandError;

	for (int i = 0; i < CPlayers::GetMaxPlayers(); ++i)
	{
		CPlayer* pPlayer = CPlayers::Get(i);
		if ( pPlayer && !pPlayer->IsBot() )
		{
			CClient* pClient = (CClient*)pPlayer;
			if ( pClient->iCommandAccessFlags )
				CUtil::Message( pClient->GetEdict(), "Name: %s, access: %s, steam ID: %s.", pClient->GetName(), 
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
		CUtil::Message(pClient ? pClient->GetEdict() : NULL, "Error, you don't have access to this command.");
	else if (result == ECommandNotFound)
		CUtil::Message(pClient ? pClient->GetEdict() : NULL, "Error, command not found.");
	else if (result == ECommandError)
		CUtil::Message(pClient ? pClient->GetEdict() : NULL, "Command error.");	
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
	Add(new CWaypointCommand);
	Add(new CPathCommand);
	Add(new CItemCommand);
	Add(new CBotChat);
	Add(new CConfigCommand);

#ifdef SOURCE_ENGINE_2006
	CBotrixPlugin::pCvar->RegisterConCommandBase( &botrix );
#else
	CBotrixPlugin::pCvar->RegisterConCommand( &botrix );
#endif
}

CMainCommand::~CMainCommand()
{
#ifndef SOURCE_ENGINE_2006
	CBotrixPlugin::pCvar->UnregisterConCommand( &botrix );
#endif
}
