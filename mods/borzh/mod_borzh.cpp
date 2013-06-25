#include "chat.h"
#include "event.h"
#include "mods/borzh/mod_borzh.h"
#include "source_engine.h"
#include "waypoint.h"


//----------------------------------------------------------------------------------------------------------------
const good::string Mod_Borzh::m_aChats[CHATS_COUNT] =
{
	"ok",
	"no moves",
	"think",
	"explore",

	"error door",
	"error door_status",
	"error button",
	"error weapon",

	"weapon found",
	"door found",
	"door change",

	"door try",
	"door stay",
	"door go",

	"button see",
	"button can push",
	"button cant push",
	"button push",

	"button weapon",
	"button no weapon",
	"button shoot",

	"button try",
	"button stay",
};

TChatVariable Mod_Borzh::iVarDoor;
TChatVariable Mod_Borzh::iVarDoorStatus;
TChatVariable Mod_Borzh::iVarButton;
TChatVariable Mod_Borzh::iVarWeapon;
TChatVariable Mod_Borzh::iVarArea;

TChatVariableValue Mod_Borzh::iVarValueDoorStatusOpened;
TChatVariableValue Mod_Borzh::iVarValueDoorStatusClosed;

TChatVariableValue Mod_Borzh::iVarValueWeaponPhyscannon;
TChatVariableValue Mod_Borzh::iVarValueWeaponCrossbow;

//----------------------------------------------------------------------------------------------------------------
Mod_Borzh::Mod_Borzh()
{
	CMod::AddEvent(new CPlayerActivateEvent());
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
}


//----------------------------------------------------------------------------------------------------------------
void Mod_Borzh::MapLoaded()
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
		sprintf(szInt, "%d", i);
		CChat::AddVariableValue( iVarDoor, good::string(szInt).duplicate() );
	}

	// Add possible buttons values.
	for ( TEntityIndex i=0; i < CItems::GetItems(EEntityTypeButton).size(); ++i )
	{
		sprintf(szInt, "%d", i);
		CChat::AddVariableValue( iVarButton, good::string(szInt).duplicate() );
	}

	// Add possible areas values.
	const StringVector& cAreas = CWaypoints::GetAreas();
	for ( TAreaId i=0; i < cAreas.size(); ++i )
		CChat::AddVariableValue( iVarButton, cAreas[i].duplicate() );

	// Generate PDDL problem.
	GeneratePddl("D:\\Games\\Botrix2007\\src\\!BotrixPlugin\\ff\\problem-generated.pddl");
}

//----------------------------------------------------------------------------------------------------------------
void Mod_Borzh::GeneratePddl( const good::string& sProblemPath )
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
