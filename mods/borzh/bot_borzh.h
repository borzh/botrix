#ifndef __BOTRIX_BOT_BORZHMOD__
#define __BOTRIX_BOT_BORZHMOD__


#include "bot.h"
#include "server_plugin.h"
#include "types_borzh.h"


/// Enum to represent possible answer to yes/no question.
enum TYesNoAnswer
{
	EUnknown = 0,
	EYes,
	ENo,
};

class CAction; // Forward declaration.

//****************************************************************************************************************
/// Class representing a bot for BorzhMod.
//****************************************************************************************************************
class CBotBorzh: public CBot
{

public:
	/// Constructor.
	CBotBorzh( edict_t* pEdict, TPlayerIndex iIndex, TBotIntelligence iIntelligence );

	/// Destructor.
	virtual ~CBotBorzh();


	//------------------------------------------------------------------------------------------------------------
	// Next functions are mod dependent. You need to implement those in order to make bot moving around.
	//------------------------------------------------------------------------------------------------------------
	/// Called when player becomes active, before first respawn.
	virtual void Activated();

	/// Called each time bot is respawned.
	virtual void Respawned();

	/// Called when this bot just killed an enemy.
	virtual void KilledEnemy( int iPlayerIndex, CPlayer* pVictim ) {}

	/// Called when enemy just shot this bot.
	virtual void HurtBy( int iPlayerIndex, CPlayer* pAttacker, int iHealthNow );

	/// Called when chat arrives from other player.
	virtual void ReceiveChat( int iPlayerIndex, CPlayer* pPlayer , bool bTeamOnly, const char* szText );

	/// Called when chat request arrives from other player.
	virtual void ReceiveChatRequest( const CBotChat& cRequest );


protected: // Inherited methods.

	// Called each frame. Set move and look variables. You can also set shooting/crouching/jumping buttons in m_cCmd.buttons.
	virtual void Think();

	// This function get's called when next waypoint in path becomes closer than current one. By default sets new
	// look forward to next waypoint in path (after current one).
	virtual void CurrentWaypointJustChanged();

	// Check waypoint flags and perform waypoint action. Called when arrives to next waypoint. By default this 
	// will check if waypoint has health/armor machine flags and if bot needs to use it. If so, then this function 
	// returns true to not to call ApplyPathFlags() / DoPathAction(), while performing USE action (for machine),
	// and ApplyPathFlags() / DoPathAction() will be called after using machine.
	virtual bool DoWaypointAction();

	// Check waypoint path flags and set bot flags accordingly. Called when arrives to next waypoint.
	// Note that you need to set action variables in DoPathAction(). This function is just to
	// figure out if need to crouch/sprint/etc and set new aim/destination to next waypoint in path.
	virtual void ApplyPathFlags();

	// Called when started to touch waypoint. You can use iCurrentWaypoint and iNextWaypoint to get path flags.
	// Will set all needed action variables to jump / jump with duck / break.
	virtual void DoPathAction();

	// Return true if given player is enemy.
	virtual bool IsEnemy( CPlayer* pPlayer ) const { return false; }

	// Bot just picked up given item.
	virtual void PickItem( const CEntity& cItem, TEntityType iEntityType, TEntityIndex iIndex );


protected: // Methods.

	// Set areas that can be reached from iCurrentArea according to seen&opened doors.
	static void SetReachableAreas( int iCurrentArea, const good::bitset& cSeenDoors, const good::bitset& cOpenedDoors, good::bitset& cReachableAreas );

	// Check if button is reachable.
	static TWaypointId GetButtonWaypoint( TEntityIndex iButton, const good::bitset& cReachableAreas );

	// Check if door is reachable.
	static TWaypointId GetDoorWaypoint( TEntityIndex iDoor, const good::bitset& cReachableAreas );

	// Check if task is trying a button.
	bool IsTryingButton() { return (m_cCurrentBigTask.iTask == EBorzhTaskButtonTry) || (m_cCurrentBigTask.iTask == EBorzhTaskButtonTryHelp); }
	
	// Check if task is a planner task.
	bool IsPlannerTask() { return (m_cCurrentBigTask.iTask == EBorzhTaskButtonDoorConfig) || (m_cCurrentBigTask.iTask == EBorzhTaskGoToGoal); }
	
	// Check if task is a planner task.
	bool IsPlannerTaskHelp() { return (m_cCurrentBigTask.iTask == EBorzhTaskButtonDoorConfigHelp) || (m_cCurrentBigTask.iTask == EBorzhTaskGoToGoalHelp); }
	
	// Check if task is a collaborative task.
	bool IsCollaborativeTask() { return (m_cCurrentBigTask.iTask == EBorzhTaskButtonTry) || IsPlannerTask(); }
	
	bool IsPlayerInDoorArea( TPlayerIndex iPlayer, const CEntity& cDoor )
	{
		TAreaId iArea = m_aPlayersAreas[iPlayer];
		DebugAssert( iArea != EAreaIdInvalid );
		TWaypointId iWaypoint1 = cDoor.iWaypoint;
		TWaypointId iWaypoint2 = (TWaypointId)cDoor.pArguments;
		DebugAssert( iWaypoint1 != EWaypointIdInvalid && iWaypoint2 != EWaypointIdInvalid );
		TAreaId iArea1 = CWaypoints::Get(iWaypoint1).iAreaId;
		TAreaId iArea2 = CWaypoints::Get(iWaypoint2).iAreaId;
		return (iArea == iArea1) || (iArea == iArea2);
	}

	// Get last plan step performer.
	TPlayerIndex GetPlanStepPerformer();

	// Called when bot figures out if a button toggles or doesn't affect a door.
	void SetButtonTogglesDoor( TEntityIndex iButton, TEntityIndex iDoor, bool bToggle )
	{
		if ( bToggle )
			m_cButtonTogglesDoor[iButton].set(iDoor);
		else
			m_cButtonNoAffectDoor[iButton].set(iDoor);
	}

	// Check if given button toggles given door.
	TYesNoAnswer IsButtonTogglesDoor( TEntityIndex iButton, TEntityIndex iDoor )
	{
		if ( m_cButtonNoAffectDoor[iButton].test(iDoor) )
			return ENo;
		else if ( m_cButtonTogglesDoor[iButton].test(iDoor) )
			return EYes;
		else
			return EUnknown;
	}

	// When starting new task, check for doors at current waypoint.
	void DoorsCheckAtCurrentWaypoint();

	// When coming near door, check it's status.
	void DoorStatusCheck( TEntityIndex iDoor, bool bOpened, bool bNeedToPassThrough );

	// Door has same status as bot thinks.
	void DoorStatusSame( TEntityIndex iDoor, bool bOpened, bool bCheckingDoors );

	// Bot thinks that door was opened when it is closed, and viceversa.
	void DoorStatusDifferent( TEntityIndex iDoor, bool bOpened, bool bCheckingDoors );

	// Bot needs to pass through the door, but it is closed.
	void DoorClosedOnTheWay( TEntityIndex iDoor, bool bCheckingDoors );

	// Say hello to all players and start investigating areas.
	void Start();

	// Task stack is empty, check if the big task has more steps.
	bool CheckBigTask();

	// Task stack is empty, check if there is something to do: investigate new area, push some button, or use FF. 
	// If iProposedTask is valid, then check if there is some other task of more importance to do. Return true when accepting proposed task.
	bool CheckForNewTasks( TBorzhTask iProposedTask = EBorzhTaskInvalid );

	// Init current task.
	void InitNewTask();

	// Save current task in stack.
	void SaveCurrentTask()
	{
		if ( m_cCurrentTask.iTask == EBorzhTaskInvalid )
			return;
		if ( m_cCurrentTask.iTask == EBorzhTaskWait )
			m_cCurrentTask.iArgument = (m_fEndWaitTime - CBotrixPlugin::fTime) * 1000; // Save only time left to wait.
		m_cTaskStack.push_back(m_cCurrentTask);
		m_cCurrentTask.iTask = EBorzhTaskInvalid;
	}

	// Wait given amount of milliseconds.
	void Wait( int iMSecs, bool bSaveCurrentTask = false )
	{
		if ( bSaveCurrentTask && !m_bTaskFinished )
			SaveCurrentTask();

		m_cCurrentTask.iTask = EBorzhTaskWait;
		m_cCurrentTask.iArgument = iMSecs;
		m_fEndWaitTime = CBotrixPlugin::fTime + iMSecs / 1000.0f;
		m_bNeedMove = m_bNeedAim = m_bTaskFinished = false;
	}

	// Speak about door/button/box/weapon.
	void PushSpeakTask( TBotChat iChat, int iArgument = 0, TEntityType iType = EEntityTypeInvalid, TEntityIndex iIndex = EEntityIndexInvalid );

	// Switch to speak task.
	void SwitchToSpeakTask( TBotChat iChat, int iArgument = 0, TEntityType iType = EEntityTypeInvalid, TEntityIndex iIndex = EEntityIndexInvalid )
	{
		SaveCurrentTask();
		PushSpeakTask(iChat, iArgument, iType, iIndex);
		TaskFinish();
	}

	// Start planner trying to reach goal area for all collaborative players.
	void StartPlannerForGoal();

	// Check if there is some unknown button-door configuration and start planner trying to figuring it out.
	// Return false if there is none such configuration.
	bool CheckButtonDoorConfigurations();

	// Perform speak task, say a phrase.
	void DoSpeakTask( int iArgument );

	// Get new task from stack.
	inline void TaskFinish()
	{
		m_bTaskFinished = true;
		m_cCurrentTask.iTask = EBorzhTaskInvalid;
	}

	// End big task.
	void BigTaskFinish();

	// Get new task from stack.
	void TaskPop()
	{
		DebugAssert( m_cCurrentTask.iTask == EBorzhTaskInvalid );

		if ( m_cTaskStack.size() > 0 )
		{
			m_cCurrentTask = m_cTaskStack.back();
			m_cTaskStack.pop_back();
			m_bNewTask = true;
		}
		m_bTaskFinished = false;
	}

	// Cancel last task.
	void CancelTasksInStack()
	{
		m_cTaskStack.clear();
		m_cCurrentTask.iTask = EBorzhTaskInvalid;
		m_bNeedMove = m_bNeedAim = false;
	}


	// Go to new button and push it.
	void PushPressButtonTask( TEntityIndex iButton, bool bShoot );

	// Go to new button and push it.
	void PushCheckButtonTask( TEntityIndex iButton, bool bShoot = false );

	// Check if all collaborative players accepted big task.
	void CheckAcceptedPlayersForCollaborativeTask();

	// Offer current task to players.
	void OfferCollaborativeTask();

	// Receive task offer from another player.
	void ReceiveTaskOffer( TBorzhTask iProposedTask, int iArgument1, int iArgument2, TPlayerIndex iSpeaker );

	// Button was pushed, update reacheable areas according to which doors button closes.
	void ButtonPushed( TEntityIndex iButton );

	// Return true if task of planner is finished.
	bool IsPlannerTaskFinished();

	// Perform next step in plan in execution. Return false if there is nothing left to do.
	bool PlanStepNext();

	// Execute action of the plan.
	void PlanStepExecute( const CAction& cAction );

	// Execute last step of the plan.
	void PlanStepLast();

	// Cancel collaborative task (if planner task, stop planner).
	void CancelCollaborativeTask( TPlayerIndex iWaitForPlayer = EPlayerIndexInvalid );

	// Called when unexpected chat arrives when performing collaborative task.
	void UnexpectedChatForCollaborativeTask( TPlayerIndex iSpeaker, bool bWaitForThisPlayer );

protected: // Members.
	friend void GeneratePddl();

	class CBorzhTask
	{
	public:
		CBorzhTask(TBorzhTask iTask = EBorzhTaskInvalid, int iArgument = 0):
			iTask(iTask), iArgument(iArgument), pArguments(NULL) {}

		TBorzhTask iTask;                                   // Task number.
		int iArgument;                                      // Task argument. May be number of door, button, etc.
		void* pArguments;                                   // Other task arguments;
	};

	static const int m_iTimeAfterPushingButton = 2000;      // Wait 2 seconds after pushing button.
	static const int m_iTimeAfterSpeak = 3000;              // Wait 3 seconds after speaking (other bots will stop and wait too).
	static const int m_iTimeToWaitPlayer = 30000;           // Wait 60 seconds for another player.

	static good::vector<TEntityIndex> m_aLastPushedButtons; // We need to know the order of pushed buttons.
	static CBorzhTask m_cCurrentProposedTask;               // Last proposed task.

	typedef good::vector< CBorzhTask > task_stack_t;        // Typedef for stack of tasks.
	good::string m_sNameWithoutSpaces;                      // Bot's name without spaces to use it in planner. TODO:

	task_stack_t m_cTaskStack;                              // Stack of tasks for small tasks.
	CBorzhTask m_cCurrentTask;                              // Current bot's task.

	CBorzhTask m_cCurrentBigTask;                           // Current bot's task at high level.

	good::bitset m_aVisitedWaypoints;                       // Visited waypoints (1 = visited, 0 = unknown).

	good::bitset m_cVisitedAreas;                           // Areas that were explored (1 = explored, 0 = unknown).
	good::bitset m_cReachableAreas;                         // Areas that can be reached from current one.
	//good::bitset m_cAuxReachableAreas;                      // Aux reachable areas, tested when need to push button and test if area can be reached.
	good::bitset m_cVisitedAreasAfterPushButton;            // Areas that wasn't visited after pushing buttons. TODO

	good::bitset m_cSeenDoors;                              // Seen doors. Other bot could see it also.
	good::bitset m_cOpenedDoors;                            // Opened doors. Closed door means seen & !opened.
	good::bitset m_cFalseOpenedDoors;                       // Used when checking a button.
	good::bitset m_cCheckedDoors;                           // Useful bitset when checking doors.

	good::bitset m_cSeenButtons;                            // Seen buttons. Other bot could see it also.
	good::bitset m_cPushedButtons;                          // Buttons pushed at least once.

	good::vector<good::bitset> m_cButtonTogglesDoor;        // Bot's belief of which set of doors button DO toggle (button is index in array).
	good::vector<good::bitset> m_cButtonNoAffectDoor;       // Bot's belief of which set of doors button DOESN'T toggle (button is index in array).

	good::vector<good::bitset> m_cTestedToggles;            // Button-doors configurations that has been tested. When domain change is produced, they will be cleared.
	//good::bitset m_cDontTestButtons;                        // Buttons that has been tested. When domain change is produced, they will be cleared.
	//good::bitset m_cDontTestDoors;                          // Door that has been tested. When domain change is produced, they will be cleared.

	good::bitset m_cCollaborativePlayers;                   // Players that are collaborating with this bot.
	good::bitset m_cBusyPlayers;                            // Players that are busy. m_bNothingToDo will be true until all of them became idle.
	good::bitset m_cAcceptedPlayers;                        // Players that accepted this bot task. All players need to accept task in order to perform it.
	good::bitset m_cPlayersWithPhyscannon;                  // Players that have a gravity gun.
	good::bitset m_cPlayersWithCrossbow;                    // Players that have a crossbow.
	good::vector<TAreaId> m_aPlayersAreas;                  // Bot's belief of where another player is.
	good::vector<TBorzhTask> m_aPlayersTasks;               // Bot's belief of what another player is doing.

	TWeaponId m_iCrossbow;                                  // Index of crossbow. -1 if bot doesn't have it.
	TWeaponId m_iPhyscannon;                                // Index of gravity gun. -1 if bot doesn't have it.

	float m_fEndWaitTime;                                   // Time when can stop waiting.


protected: // Flags.
	bool m_bStarted:1;                                      // Started (someone must say 'start'). Say 'pause' to pause all bots.
	bool m_bWasMovingBeforePause:1;                         // True if bot was moving before pause.

	bool m_bSaidHello:1;                                    // To not say hello twice.
	bool m_bNothingToDo:1;                                  // Bot has no more task to do, wait for domain change.

	bool m_bNewTask:1;                                      // New task need to be initiated.
	bool m_bTaskFinished:1;                                 // Need to pop new task from stack.
	bool m_bHasCrossbow:1;                                  // True if bot has crossbow to shoot buttons.
	bool m_bHasPhyscannon:1;                                // True if bot has physcannon to carry boxes.

	bool m_bUsingPlanner:1;                                 // Trying to get plan for something currently.
	bool m_bUsedPlannerForGoal:1;                           // .
	bool m_bUsedPlannerForButton:1;                         // .

	bool m_bSpokenAboutAllBoxes:1;                          // .

};


#endif // __BOTRIX_BOT_BORZHMOD__
