#include "good/string_buffer.h"
#include "good/process.h"
#include "good/thread.h"

#include "mods/borzh/bot_borzh.h"
#include "mods/borzh/mod_borzh.h"
#include "mods/borzh/planner.h"
#include "players.h"
#include "type2string.h"
#include "waypoint.h"


#define PRINT_MESSAGE(...) CUtil::PutMessageInQueue(__VA_ARGS__)
//#define PRINT_MESSAGE(...)

//----------------------------------------------------------------------------------------------------------------
const int g_iPlannerBufferSize = 64*1024;
char g_szPlannerBuffer[g_iPlannerBufferSize];                // Here we will read planner output.

bool bIsPlannerRunning = false;                              // Indicates if planner is running.

const good::string sProblemPath( "D:\\Games\\Botrix2007\\src\\!BotrixPlugin\\ff\\problem-generated.pddl" );
const good::string sExe( "D:\\Games\\Botrix2007\\src\\!BotrixPlugin\\ff\\ff.exe"  );
const good::string sCommand( "D:\\Games\\Botrix2007\\src\\!BotrixPlugin\\ff\\ff.exe -o domain.pddl -f problem-generated.pddl"  );
good::process g_cPlannerProcess(sExe, sCommand, true, true); // Spawned process of ff.exe

void PlannerThreadFunc( void* pArgument );                   // Forward declaration.
good::thread g_cThread(PlannerThreadFunc);                   // Thread that reads and transforms ff.exe output.

TAreaId g_iBoxArea = EAreaIdInvalid;                         // Where need to leave a box.

CPlanner::CPlan g_cPlan(64);                                 // Plan: global array of actions.
CPlanner::CPlan* g_pPlan = NULL;                             // Result of ff.exe, can be NULL or &g_cPlan.

const CBotBorzh* g_pBot = NULL;                              // Bot for which make a plan.

//----------------------------------------------------------------------------------------------------------------
bool CPlanner::m_bLocked = false;
const CBotBorzh* CPlanner::m_pBot = NULL;

//----------------------------------------------------------------------------------------------------------------
bool CPlanner::IsRunning()
{
	if ( !bIsPlannerRunning )
		g_cThread.dispose();
	return bIsPlannerRunning;
}

//----------------------------------------------------------------------------------------------------------------
void CPlanner::Start( const CBotBorzh* pBot )
{
	DebugAssert( !bIsPlannerRunning && m_bLocked && (pBot == m_pBot) );

	bIsPlannerRunning = true;
	g_pBot = pBot;
	g_cThread.launch(NULL, true);
}

//----------------------------------------------------------------------------------------------------------------
void CPlanner::Stop()
{
	g_cPlannerProcess.dispose();
	g_cThread.dispose(); // This will not kill the thread.
	//bIsPlannerRunning = false;
	g_pPlan = NULL;
}

//----------------------------------------------------------------------------------------------------------------
const CPlanner::CPlan* CPlanner::GetPlan()
{
	return g_pPlan;
}

//----------------------------------------------------------------------------------------------------------------
void GeneratePddl( bool bFromBotBelief )
{
	FILE* f = fopen( sProblemPath.c_str(), "w" );

	// Print problem domain.
	fprintf(f, "(define (problem bots)\n\n");
	fprintf(f, "	(:domain\n");
	fprintf(f, "		bots-domain\n");
	fprintf(f, "	)\n\n");

	// Print problem objects: bots, areas, etc.
	fprintf(f, "	(:objects\n		");
	for ( TPlayerIndex iPlayer = 0; iPlayer < CPlayers::Size(); ++iPlayer )
	{
		CPlayer* pPlayer = CPlayers::Get(iPlayer);
		if ( pPlayer && (!bFromBotBelief || g_pBot->m_cCollaborativePlayers.test(iPlayer)) )
			fprintf(f, "bot%d ", iPlayer);
	}
	fprintf(f, "- bot\n\n");

	fprintf(f, "		");
	const StringVector& aAreas = CWaypoints::GetAreas();
	for ( TAreaId i=0; i < aAreas.size(); ++i )
		fprintf(f, "area%d ", i);
	fprintf( f, "- waypoint\n" );

	fprintf(f, "		");
	const good::vector<CEntity>& cDoors = CItems::GetItems(EEntityTypeDoor);
	for ( TEntityIndex i=0; i < cDoors.size(); ++i )
		if ( !bFromBotBelief || g_pBot->m_cSeenDoors.test(i) )
			fprintf( f, "door%d ", i );
	fprintf(f, "- door\n\n");

	fprintf(f, "		");
	const good::vector<CEntity>& cButtons = CItems::GetItems(EEntityTypeButton);
	for ( TEntityIndex i=0; i < cButtons.size(); ++i )
		if ( !bFromBotBelief || g_pBot->m_cSeenButtons.test(i) )
			fprintf( f, "button%d ", i );
	fprintf(f, "- button\n");

	fprintf(f, "		");
	
	const good::vector<CEntity>& cWeapons = CItems::GetItems(EEntityTypeWeapon);
	if ( bFromBotBelief )
		fprintf(f, "gravity-gun crossbow - weapon\n\n"); // Add void weapons names for players that have that weapon.
	else
	{
		for ( TEntityIndex i=0; i < cWeapons.size(); ++i )
		{
			const CEntity& cWeapon = cWeapons[i];
			if ( cWeapon.IsFree() || !cWeapon.IsOnMap() )
				continue;

			const good::string& sWeapon = cWeapons[i].pItemClass->sClassName;
			if ( (sWeapon == "weapon_physcannon") || (sWeapon == "weapon_crossbow") )
				fprintf( f, "weapon%d ", i );
		}
		fprintf(f, "- weapon\n");
	}

	fprintf(f, "		");
	if ( bFromBotBelief )
	{
		if ( g_pBot->m_aBoxes.size() > 0 )
		{
			for ( TEntityIndex i=0; i < g_pBot->m_aBoxes.size(); ++i )
				fprintf( f, "box%d ", g_pBot->m_aBoxes[i].iBox );
			fprintf(f, "- box\n");
		}
	}
	else
	{
		if ( CModBorzh::GetBoxes().size() > 0 )
		{
			for ( TEntityIndex i=0; i < CModBorzh::GetBoxes().size(); ++i )
				fprintf( f, "box%d ", CModBorzh::GetBoxes()[i].iBox );
			fprintf(f, "- box\n");
		}
	}

	fprintf(f, "	)\n\n");


	// Print initial state.
	fprintf(f, "	(:init\n");

	// Default weapons.
	if ( bFromBotBelief )
	{
		fprintf(f, "		(physcannon gravity-gun)\n");
		fprintf(f, "		(sniper-weapon crossbow)\n");
	}
	else
	{
		for ( TEntityIndex i=0; i < cWeapons.size(); ++i )
		{
			const CEntity& cWeapon = cWeapons[i];
			if ( cWeapon.IsFree() || !cWeapon.IsOnMap() )
				continue;

			const good::string& sWeapon = cWeapon.pItemClass->sClassName;
			if ( sWeapon == "weapon_physcannon" )
				fprintf( f, "		(physcannon weapon%d)\n", i );
			else if ( sWeapon == "weapon_crossbow" )
				fprintf( f, "		(sniper-weapon weapon%d)\n", i );
			else
			{
				PRINT_MESSAGE("Warning, not using %s %d, it is not crossbow nor physcannon.", sWeapon.c_str(), i);
				continue;
			}

			if ( CWaypoints::IsValid(cWeapon.iWaypoint) )
				fprintf( f, "		(weapon-at weapon%d area%d)\n\n", i, CWaypoints::Get(cWeapon.iWaypoint).iAreaId );
			else
				PRINT_MESSAGE("Error, %s %d doesn't have waypoint close.", sWeapon.c_str(), i);
		}
	}

	// Bot's position and weapons.
	for ( TPlayerIndex iPlayer=0; iPlayer < CPlayers::Size(); ++iPlayer )
	{
		CPlayer* pPlayer = CPlayers::Get(iPlayer);
		if ( pPlayer && (!bFromBotBelief || g_pBot->m_cCollaborativePlayers.test(iPlayer)) )
		{
			if ( bFromBotBelief )
			{
				fprintf(f, "		(at bot%d area%d) (empty bot%d)", iPlayer, g_pBot->m_aPlayersAreas[iPlayer], iPlayer);
				if ( g_pBot->m_cPlayersWithPhyscannon.test(iPlayer) )
					fprintf(f, " (has bot%d gravity-gun)", iPlayer);
				if ( g_pBot->m_cPlayersWithCrossbow.test(iPlayer) )
					fprintf(f, " (has bot%d crossbow)", iPlayer);
			}
			else
				fprintf(f, "		(at bot%d area%d) (empty bot%d)", iPlayer, CWaypoints::Get(pPlayer->iCurrentWaypoint).iAreaId, iPlayer);
			fprintf(f, "\n\n");
		}
	}

	// Box-at.
	if ( bFromBotBelief )
	{
		for ( int i=0; i < g_pBot->m_aBoxes.size(); ++i )
			fprintf( f, "		(box-at box%d area%d)\n", g_pBot->m_aBoxes[i].iBox, g_pBot->m_aBoxes[i].iArea );
	}
	else
	{
		for ( int i=0; i < CModBorzh::GetBoxes().size(); ++i )
			fprintf( f, "		(box-at box%d area%d)\n", CModBorzh::GetBoxes()[i].iBox, CModBorzh::GetBoxes()[i].iArea );
	}
	fprintf(f, "\n");

	// Buttons positions.
	for ( TEntityIndex iButton=0; iButton < cButtons.size(); ++iButton )
	{
		if ( !bFromBotBelief || g_pBot->m_cSeenButtons.test(iButton) )
		{
			const CEntity& cButton = cButtons[iButton];
			int iWaypoint = cButton.iWaypoint;
			if ( CWaypoints::IsValid(iWaypoint) )
				fprintf(f, "		(button-at button%d area%d)\n", iButton, CWaypoints::Get(iWaypoint).iAreaId);
			else
			{
				iWaypoint = CModBorzh::GetWaypointToShootButton(iButton);
				fprintf(f, "		(can-shoot button%d area%d)\n", iButton, CWaypoints::Get(iWaypoint).iAreaId);
			}
		}
	}
	fprintf(f, "\n");

	// Between.
	for ( TEntityIndex iDoor=0; iDoor < cDoors.size(); ++iDoor )
	{
		if ( !bFromBotBelief || g_pBot->m_cSeenDoors.test(iDoor) )
		{
			const CEntity& cDoor = cDoors[iDoor];
			int iWaypoint1 = cDoor.iWaypoint;
			int iWaypoint2 = (TWaypointId)cDoor.pArguments;
			if ( CWaypoints::IsValid(iWaypoint1) && CWaypoints::IsValid(iWaypoint2) )
			{
				int iArea1 = CWaypoints::Get(iWaypoint1).iAreaId;
				int iArea2 = CWaypoints::Get(iWaypoint2).iAreaId;
				if ( !bFromBotBelief || ( g_pBot->m_cVisitedAreas.test(iArea1) && g_pBot->m_cVisitedAreas.test(iArea2) ) )
				{
					fprintf(f, "		(between door%d area%d area%d)\n", iDoor, iArea1, iArea2);
					bool bOpened = bFromBotBelief ? g_pBot->m_cOpenedDoors.test(iDoor) : CItems::IsDoorOpened(iDoor);
					if ( bOpened )
					{
						fprintf(f, "		(can-move area%d area%d)\n", iArea1, iArea2);
						fprintf(f, "		(can-move area%d area%d)\n", iArea2, iArea1);
					}
				}
			}
			else
			{
				DebugAssert(false);
				PRINT_MESSAGE("Error, door%d doesn't have 2 waypoints close.", iDoor+1);
			}
		}
	}
	fprintf(f, "\n");

	// Walls.
	for ( TAreaId iArea=0; iArea < aAreas.size(); ++iArea )
	{
		const good::vector<CWall>& aWalls = CModBorzh::GetWallsForArea(iArea);
		if ( (aWalls.size() > 0) && ( !bFromBotBelief || g_pBot->m_cSeenWalls.test(iArea) ) )
		{
			for ( int iWall=0; iWall < aWalls.size(); ++iWall )
			{
				DebugAssert( iArea == CWaypoints::Get(aWalls[iWall].iLowerWaypoint).iAreaId );
				fprintf(f, "		(wall area%d area%d)\n", iArea, CWaypoints::Get(aWalls[iWall].iHigherWaypoint).iAreaId);
			}
		}
	}

	// Buttons configuration.
	if ( bFromBotBelief )
	{
		for ( TEntityIndex iButton=0; iButton < cButtons.size(); ++iButton )
		{
			const good::bitset& cButtonTogglesDoor = g_pBot->m_cButtonTogglesDoor[iButton];
			if ( cButtonTogglesDoor.any() )
			{
				int iDoorsCount = 0;
				int iDoors[2] = { -1, -1};
				for ( TEntityIndex iDoor = 0; iDoor < cDoors.size(); ++iDoor )
					if ( cButtonTogglesDoor.test(iDoor) )
						iDoors[iDoorsCount++] = iDoor;

				DebugAssert( iDoorsCount <= 2 );
				if ( iDoors[1] == -1 )
					iDoors[1] = iDoors[0];
				fprintf(f, "		(toggle button%d door%d door%d)\n", iButton, iDoors[0], iDoors[1]);
			}
		}
	}
	else
		fprintf(f, "\n		; Auto-generated buttons configuration.\n");

	// End init.
	fprintf(f, "	)\n\n");

	// Goal.
	fprintf(f, "	(:goal\n");

	if ( !bFromBotBelief || g_pBot->m_cCurrentBigTask.iTask == EBorzhTaskGoToGoal )
	{
		fprintf(f, "		(and\n");
		TAreaId iGoalArea = CWaypoints::GetAreas().size() - 1;

		// Print goal positions for all bots.
		for ( int iPlayer = 0; iPlayer < CPlayers::Size(); ++iPlayer )
		{
			CPlayer* pPlayer = CPlayers::Get(iPlayer);
			if ( pPlayer && ( !bFromBotBelief || g_pBot->m_cCollaborativePlayers.test(iPlayer) ) )
				fprintf(f, "			(at bot%d area%d)\n", iPlayer, iGoalArea);
		}
		fprintf(f, "		)\n"); // and
	}
	else if ( g_pBot->m_cCurrentBigTask.iTask == EBorzhTaskBringBox )
	{
		TAreaId iArea = GET_2ND_BYTE(g_pBot->m_cCurrentBigTask.iArgument);
		fprintf(f, "			( exists (?box - box) (box-at ?box area%d) )\n", iArea);
	}
	else
	{
		DebugAssert( g_pBot->m_cCurrentBigTask.iTask == EBorzhTaskButtonTryDoor );

		TEntityIndex iButton = GET_2ND_BYTE(g_pBot->m_cCurrentBigTask.iArgument);
		TEntityIndex iDoor = GET_3RD_BYTE(g_pBot->m_cCurrentBigTask.iArgument);

		const CEntity& cButton = CItems::GetItems(EEntityTypeButton)[iButton];
		
		TWaypointId iButtonWaypoint = cButton.iWaypoint;
		bool bShoot = (iButtonWaypoint == EWaypointIdInvalid);
		if ( bShoot )
			iButtonWaypoint = CModBorzh::GetWaypointToShootButton(iButton);
		DebugAssert( iButtonWaypoint != EWaypointIdInvalid );
		
		fprintf(f, "		(and\n");
		if ( bShoot )
			fprintf(f, "			( exists (?bot - bot) (and (at ?bot area%d) (has ?bot crossbow)) )\n", CWaypoints::Get(iButtonWaypoint).iAreaId);
		else
			fprintf(f, "			( exists (?bot - bot) (at ?bot area%d) )\n", CWaypoints::Get(iButtonWaypoint).iAreaId);
		
		if ( iDoor != 0xFF )
		{
			const CEntity& cDoor = CItems::GetItems(EEntityTypeDoor)[iDoor];
			TWaypointId iDoorWaypoint1 = cDoor.iWaypoint;
			TWaypointId iDoorWaypoint2 = (TWaypointId)cDoor.pArguments;
			DebugAssert( (iDoorWaypoint1 != EWaypointIdInvalid) && (iDoorWaypoint2 != EWaypointIdInvalid) );

			TAreaId iDoorArea1 = CWaypoints::Get(iDoorWaypoint1).iAreaId;
			TAreaId iDoorArea2 = CWaypoints::Get(iDoorWaypoint2).iAreaId;
			fprintf(f, "			(or\n");
			if ( g_pBot->m_cVisitedAreas.test(iDoorArea1) )
				fprintf(f, "				( exists (?bot - bot) (at ?bot area%d) )\n", iDoorArea1);
			if ( g_pBot->m_cVisitedAreas.test(iDoorArea2) )
				fprintf(f, "				( exists (?bot - bot) (at ?bot area%d) )\n", iDoorArea2);
			fprintf(f, "			)\n");
		}
		fprintf(f, "		)\n"); // and
	}

	fprintf(f, "	)\n");
	fprintf(f, ")\n"); // (define (problem bots)

	fflush(f);
	fclose(f);
}

//----------------------------------------------------------------------------------------------------------------
// Transform ff.exe output to a plan.
//----------------------------------------------------------------------------------------------------------------
const good::string GetNextWord( good::string& sStr, int& iFrom )
{
	char c = sStr[iFrom];
	// Skip space.
	while ( ((c == ' ') || (c == '\n') || (c == '\r')) && (iFrom < sStr.size()) )
		c = sStr[++iFrom];

	// Skip non-space.
	int iTo = iFrom;
	while ( (c != ' ') && (c != '\n') && (c != '\r') && (iTo < sStr.size()) )
		c = sStr[++iTo];

	DebugAssert( iTo-iFrom > 0 );
	good::string result = sStr.substr(iFrom, iTo-iFrom, false);
	iFrom = iTo;
	return result;
}

bool TransformPlannerOutput()
{
	static good::string sPlanStart("ff: found legal plan as follows");
	static good::string sNoPlan("ff: goal can be simplified to FALSE");
	static good::string sEmptyPlan("ff: goal can be simplified to TRUE");

	g_cPlan.clear();

	good::string_buffer sbBuffer(g_szPlannerBuffer, g_iPlannerBufferSize, false, true);

	if ( sbBuffer.find(sNoPlan) != good::string::npos ) // No plan.
		return false;

	if ( sbBuffer.find(sEmptyPlan) != good::string::npos ) // Empty plan.
	{
		g_pPlan = &g_cPlan;
		return true;
	}

	int iPos = sbBuffer.find(sPlanStart);
	if ( iPos == good::string::npos ) // No plan.
		return false;

	// Skip end of line twice.
	iPos += sPlanStart.size();
	iPos = sbBuffer.find('\n', iPos+1);
	DebugAssert( iPos != good::string::npos );
	iPos = sbBuffer.find('\n', iPos+1);
	DebugAssert( iPos != good::string::npos );
	iPos++;

	// Find ':' until empty string is found.
	while ( iPos < sbBuffer.size() )
	{
		int iEnd = sbBuffer.find('\n', iPos);
		if ( iEnd == good::string::npos )
			break;
		iEnd++;

		iPos = sbBuffer.find(':', iPos);
		if ( iPos == good::string::npos )
			break;

		if ( iPos >= iEnd )
			break;

		// Normally action requieres 2 arguments: bot that performs that action and argument.
		// Get action.
		good::string sAction = GetNextWord(sbBuffer, ++iPos);
		DebugAssert( iPos < iEnd );
        if ( sAction == "REACH-GOAL" ) // FF interrnal action :S
            break;

		TBotAction iAction = CTypeToString::BotActionFromString(sAction);
		DebugAssert( iAction != EBotActionInvalid );

		// Get bot.
		static good::string sBot("BOT");
		good::string sPerformer = GetNextWord(sbBuffer, ++iPos);
		DebugAssert( iPos < iEnd );
		DebugAssert( sPerformer.size() > sBot.size() && sPerformer.starts_with(sBot) );
		const char* szBotNumber = sPerformer.c_str() + sBot.size();
		int iBot = atoi(szBotNumber);
		DebugAssert( 0 <= iBot && iBot <= CPlayers::Size() );

		// Get argument.
		good::string sArgument = GetNextWord(sbBuffer, ++iPos);
		DebugAssert( iPos < iEnd );
		const char* szArgumentNumber = sArgument.c_str();
		while ( *szArgumentNumber < '0' || *szArgumentNumber > '9' )
			szArgumentNumber++;
		int iArgument = atoi(szArgumentNumber);

		CAction cAction(iAction, iBot, iArgument);
		g_cPlan.push_back(cAction);
		iPos = iEnd;
	}

	return true;
}

//----------------------------------------------------------------------------------------------------------------
// Planner thread function: reads output of FF and then transforms it to a plan.
//----------------------------------------------------------------------------------------------------------------
void PlannerThreadFunc( void* pParameter )
{
	const int iThreadSleepTime = 500;
	const int iMaxTimeToRunProcess = 5000;
	int iTotalRead = 0;

	GeneratePddl(true);
	if ( !g_cPlannerProcess.launch(false, false) )
	{
		PRINT_MESSAGE( "Error while executing ff.exe:");
		PRINT_MESSAGE( "    %s", g_cPlannerProcess.get_last_error() );
		g_pPlan = NULL;
		goto planner_thread_end;
	}

	// Repeat reading process output while there is space in buffer and either process is running or process has more output.
	for ( int iTime = 0; (iTime < iMaxTimeToRunProcess) && (g_cPlannerProcess.is_running() || g_cPlannerProcess.has_data_stdout()); iTime += iThreadSleepTime )
	{
		int iRead;
		if ( g_cPlannerProcess.has_data_stdout() )
		{
			if ( g_cPlannerProcess.read_stdout(&g_szPlannerBuffer[iTotalRead], g_iPlannerBufferSize - iTotalRead, iRead) )
			{
				iTotalRead += iRead;
				DebugAssert( iTotalRead <= g_iPlannerBufferSize );
				if ( iTotalRead == g_iPlannerBufferSize ) // Not enough memory, force failure.
					break;
			}
			else
			{
				PRINT_MESSAGE( "Error while executing ff.exe:" );
				PRINT_MESSAGE( "    %s", g_cPlannerProcess.get_last_error() );
				break;
			}
		}
		good::thread::sleep(iThreadSleepTime);
	}
	DebugAssert( !g_cPlannerProcess.has_data_stdout() );

	if ( g_cPlannerProcess.is_finished() && (iTotalRead < g_iPlannerBufferSize) )
	{
		// Finished correctly.
		g_szPlannerBuffer[iTotalRead] = 0;
		g_cPlan.clear();
		PRINT_MESSAGE( "%s", g_szPlannerBuffer);
		if ( TransformPlannerOutput() )
			g_pPlan = &g_cPlan;
		else
			g_pPlan = NULL;
	}
	else
	{
		// Terminate either because process is taking too long, or just because 64k is not enough to read output.
		g_cPlannerProcess.terminate();
		g_pPlan = NULL;
	}

planner_thread_end:
	g_cPlannerProcess.dispose();
	bIsPlannerRunning = false;
}
