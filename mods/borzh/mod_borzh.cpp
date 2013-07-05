#include "chat.h"
#include "event.h"
#include "mods/borzh/mod_borzh.h"
#include "players.h"
#include "source_engine.h"
#include "waypoint.h"


//----------------------------------------------------------------------------------------------------------------
/*const good::string CMod_Borzh::m_aChats[CHATS_COUNT] =
{
};*/

TChatVariable CMod_Borzh::iVarDoor;
TChatVariable CMod_Borzh::iVarDoorStatus;
TChatVariable CMod_Borzh::iVarButton;
TChatVariable CMod_Borzh::iVarWeapon;
TChatVariable CMod_Borzh::iVarArea;
TChatVariable CMod_Borzh::iVarPlayer;

TChatVariableValue CMod_Borzh::iVarValueDoorStatusOpened;
TChatVariableValue CMod_Borzh::iVarValueDoorStatusClosed;

TChatVariableValue CMod_Borzh::iVarValueWeaponPhyscannon;
TChatVariableValue CMod_Borzh::iVarValueWeaponCrossbow;

good::vector< good::vector<TWaypointId> > CMod_Borzh::m_cAreasWaypoints; // Waypoints for areas.

//----------------------------------------------------------------------------------------------------------------
CMod_Borzh::CMod_Borzh()
{
	//CMod::AddEvent(new CPlayerActivateEvent()); // No need for this.
	CMod::AddEvent(new CPlayerTeamEvent());
	CMod::AddEvent(new CPlayerSpawnEvent());

	CMod::AddEvent(new CPlayerHurtEvent());
	CMod::AddEvent(new CPlayerDeathEvent());

	CMod::AddEvent(new CPlayerChatEvent());

	iVarDoor = CChat::AddVariable("$door");
	iVarDoorStatus = CChat::AddVariable("$door_status");
	iVarButton = CChat::AddVariable("$button");
	iVarWeapon = CChat::AddVariable("$weapon");
	iVarArea = CChat::AddVariable("$area");
	iVarPlayer = CChat::AddVariable("$player"); // TODO: move away.
}


//----------------------------------------------------------------------------------------------------------------
void CMod_Borzh::MapLoaded()
{
	// Add possible chat variable values for doors, buttons and weapons.
	iVarValueDoorStatusOpened = CChat::AddVariableValue(iVarDoorStatus, "opened");
	iVarValueDoorStatusClosed = CChat::AddVariableValue(iVarDoorStatus, "closed");

	iVarValueWeaponPhyscannon = CChat::AddVariableValue(iVarWeapon, "weapon_physcannon");
	iVarValueWeaponCrossbow = CChat::AddVariableValue(iVarWeapon, "weapon_crossbow");

	static char szInt[16];

	// Add possible doors values.
	for ( TEntityIndex i=0; i < CItems::GetItems(EEntityTypeDoor).size(); ++i )
	{
		sprintf(szInt, "%d", i+1);
		CChat::AddVariableValue( iVarDoor, good::string(szInt).duplicate() );
	}

	// Add possible buttons values.
	for ( TEntityIndex i=0; i < CItems::GetItems(EEntityTypeButton).size(); ++i )
	{
		sprintf(szInt, "%d", i+1);
		CChat::AddVariableValue( iVarButton, good::string(szInt).duplicate() );
	}

	// Add possible areas values.
	const StringVector& cAreas = CWaypoints::GetAreas();
	for ( TAreaId i=0; i < cAreas.size(); ++i )
		CChat::AddVariableValue( iVarButton, cAreas[i].duplicate() );

	// Add empty player names.
	for ( int i=0; i < CPlayers::Size(); ++i )
		CChat::AddVariableValue( iVarPlayer, "" );

	// Load waypoints for areas.
	m_cAreasWaypoints.clear();
	m_cAreasWaypoints.resize( cAreas.size() );
	for ( TWaypointId iWaypoint = 0; iWaypoint < CWaypoints::Size(); ++iWaypoint )
		m_cAreasWaypoints[ CWaypoints::Get(iWaypoint).iAreaId ].push_back( iWaypoint );

	// Generate PDDL problem.
	GeneratePddl("D:\\Games\\Botrix2007\\src\\!BotrixPlugin\\ff\\problem-generated.pddl");
}

//----------------------------------------------------------------------------------------------------------------
void CMod_Borzh::GeneratePddl( const good::string& sProblemPath )
{
	FILE* f = fopen( sProblemPath.c_str(), "w" );

	// Print problem domain.
	fprintf(f, "(define (problem bots)\n\n");
	fprintf(f, "	(:domain\n");
	fprintf(f, "		bots-domain\n");
	fprintf(f, "	)\n\n");

	// Print problem objects.
	fprintf(f, "	(:objects\n");
	fprintf(f, "		bot1 bot2 - bot\n\n");

	fprintf(f, "		");
	const StringVector& cAreas = CWaypoints::GetAreas();
	for ( TAreaId i=1; i < cAreas.size(); ++i )
		fprintf(f, "area-%s ", cAreas[i].c_str());
	fprintf( f, "- waypoint\n" );

	fprintf(f, "		");
	const good::vector<CEntity>& cDoors = CItems::GetItems(EEntityTypeDoor);
	for ( TEntityIndex i=0; i < cDoors.size(); ++i )
		fprintf( f, "door%d ", i+1 );
	fprintf(f, "- door\n\n");

	fprintf(f, "		");
	const good::vector<CEntity>& cButtons = CItems::GetItems(EEntityTypeButton);
	for ( TEntityIndex i=0; i < cButtons.size(); ++i )
		fprintf( f, "button%d ", i+1 );
	fprintf(f, "- button\n");

	fprintf(f, "		");
	const good::vector<CEntity>& cWeapons = CItems::GetItems(EEntityTypeWeapon);
	for ( TEntityIndex i=0; i < cWeapons.size(); ++i )
	{
		const good::string& sWeapon = cWeapons[i].pItemClass->sClassName;
		if ( (sWeapon == "weapon_physcannon") || (sWeapon == "weapon_crossbow") )
			fprintf( f, "%s%d ", sWeapon.c_str(), i+1 );
	}
	fprintf(f, "- weapon\n\n");

	fprintf(f, "		");
	const good::vector<CEntity>& cObjects = CItems::GetItems(EEntityTypeObject);
	for ( TEntityIndex i=0; i < cObjects.size(); ++i )
		if ( FLAG_SOME_SET(FObjectBox, cObjects[i].iFlags) )
			fprintf( f, "box%d ", i+1 );
	fprintf(f, "- box\n");

	fprintf(f, "	)\n\n");


	// Print initial state.
	fprintf(f, "	(:init\n");

	// Print weapons type and where weapons are.
	for ( TEntityIndex i=0; i < cWeapons.size(); ++i )
	{
		if ( CWaypoints::IsValid(cWeapons[i].iWaypoint) )
		{
			CWaypoint& cWaypoint = CWaypoints::Get(cWeapons[i].iWaypoint);
			TAreaId iArea = cWaypoint.iAreaId;

			const good::string& sWeapon = cWeapons[i].pItemClass->sClassName;
			if ( sWeapon == "weapon_physcannon" )
			{
				fprintf( f, "		(physcannon %s%d)\n", sWeapon.c_str(), i+1 );
				fprintf( f, "		(weapon-at %s%d area-%s)\n\n", sWeapon.c_str(), i+1, cAreas[iArea].c_str() );
			}
			else if ( sWeapon == "weapon_crossbow" )
			{
				fprintf( f, "		(sniper-weapon %s%d)\n", sWeapon.c_str(), i+1 );
				fprintf( f, "		(weapon-at %s%d area-%s)\n\n", sWeapon.c_str(), i+1, cAreas[iArea].c_str() );
			}
		}
	}

	// Bots initial position.
	fprintf(f, "		(at bot1 area-respawn1) (empty bot1)\n");
	fprintf(f, "		(at bot2 area-respawn2) (empty bot2)\n\n");

	// Buttons positions.
	for ( TEntityIndex i=0; i < cButtons.size(); ++i )
	{
		const CEntity& cButton = cButtons[i];
		int iWaypoint = cButton.iWaypoint;
		if ( CWaypoints::IsValid(iWaypoint) )
		{
			int iArea = CWaypoints::Get(iWaypoint).iAreaId;
			fprintf(f, "		(button-at button%d area-%s)\n", i+1, cAreas[iArea].c_str());
		}
	}

	fprintf(f, "\n");

	// Between.
	for ( TEntityIndex i=0; i < cDoors.size(); ++i )
	{
		const CEntity& cDoor = cDoors[i];
		int iWaypoint1 = cDoor.iWaypoint;
		int iWaypoint2 = (TWaypointId)cDoor.pArguments;
		if ( CWaypoints::IsValid(iWaypoint1) && CWaypoints::IsValid(iWaypoint2) )
		{
			int iArea1 = CWaypoints::Get(iWaypoint1).iAreaId;
			int iArea2 = CWaypoints::Get(iWaypoint2).iAreaId;
			fprintf(f, "		(between door%d area-%s area-%s)\n", i+1, cAreas[iArea1].c_str(), cAreas[iArea2].c_str());
			if ( CItems::IsDoorOpened(i) )
			{
				fprintf(f, "		(can-move area-%s area-%s)\n", cAreas[iArea1].c_str(), cAreas[iArea2].c_str());
				fprintf(f, "		(can-move area-%s area-%s)\n", cAreas[iArea2].c_str(), cAreas[iArea1].c_str());
			}

		}
		else
			CUtil::Message(NULL, "Error, door%d doesn't have 2 waypoints close.", i+1);
	}

	fprintf(f, "\n");

	// Box-at.
	for ( TEntityIndex i=0; i < cObjects.size(); ++i )
	{
		const CEntity& cObject = cObjects[i];
		if ( FLAG_SOME_SET(FObjectBox, cObject.iFlags) )
		{
			// Neares waypoints for objects are not set, calculate it.
			TWaypointId iWaypoint = CWaypoints::GetNearestWaypoint(cObject.vOrigin);
			if ( CWaypoints::IsValid(iWaypoint) )
				fprintf( f, "		(box-at box%d area-%s)\n", i+1, cAreas[ CWaypoints::Get(iWaypoint).iAreaId ].c_str() );
			else
				CUtil::Message(NULL, "Warning, box%d has no waypoint near.", i+1);
		}
	}

	fprintf(f, "\n");

	// can-climb-two, can-climb-three, can-shoot.
	for ( TWaypointId iWaypoint1=0; iWaypoint1 < CWaypoints::Size(); ++iWaypoint1 )
	{
		CWaypoints::WaypointNode& cNode = CWaypoints::GetNode(iWaypoint1);
		int iArea1 = cNode.vertex.iAreaId;

		if ( FLAG_SOME_SET(FWaypointSeeButton, cNode.vertex.iFlags) )
		{
			TEntityIndex iButton = CWaypoint::GetButton(cNode.vertex.iArgument);
			if ( iButton > 0 )
				fprintf(f, "		(can-shoot button%d area-%s)\n", iButton, cAreas[iArea1].c_str());
			else
				CUtil::Message(NULL, "Error, waypoint %d: see_button argument has invalid button.", iWaypoint1);
		}

		CWaypoints::WaypointNode::arcs_t& cNeighbours = cNode.neighbours;
		for ( int i=0; i < cNeighbours.size(); ++i)
		{
			CWaypoints::WaypointArc& cArc = cNeighbours[i];
			TWaypointId iWaypoint2 = cArc.target;
			CWaypointPath& cPath = cArc.edge;
			TAreaId iArea2 = CWaypoints::Get(iWaypoint2).iAreaId;

			if ( iArea1 != iArea2 )
			{
				if ( FLAG_SOME_SET(FPathTotem, cPath.iFlags) )
				{
					if ( cPath.iArgument == 2 )
						fprintf(f, "		(can-climb-two area-%s area-%s)\n", cAreas[iArea1].c_str(), cAreas[iArea2].c_str());
					else if ( cPath.iArgument == 3 )
						fprintf(f, "		(can-climb-three area-%s area-%s)\n", cAreas[iArea1].c_str(), cAreas[iArea2].c_str());
					else
						CUtil::Message(NULL, "Error, totem value must be 2 or 3 for path from %d to %d.", iWaypoint1, iWaypoint2);
				}
				else if ( !FLAG_SOME_SET(FPathDoor, cPath.iFlags) ) // Has no door between, and no need to make totem, just pass.
					fprintf(f, "		(can-move area-%s area-%s)\n", cAreas[iArea1].c_str(), cAreas[iArea2].c_str());
			}
		}
	}

	fprintf(f, "\n		; Auto-generated buttons configuration.\n\n	)\n\n");

	// Goal.
	fprintf(f, "	(:goal\n");
	fprintf(f, "		(and\n");
	fprintf(f, "			(at bot1 area-goal) (at bot2 area-goal)\n");
	fprintf(f, "		)\n");
	fprintf(f, "	)\n");
	fprintf(f, ")\n");

	fclose(f);
}
