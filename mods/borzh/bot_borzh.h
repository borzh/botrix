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


//****************************************************************************************************************
/// Class representing a bot for BorzhMod.
//****************************************************************************************************************
class CBot_BorzhMod: public CBot
{

public:
	/// Constructor.
	CBot_BorzhMod( edict_t* pEdict, TPlayerIndex iIndex, TBotIntelligence iIntelligence );

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

	// When coming near door, check it's status.
	void CheckDoorStatus( TEntityIndex iDoor, bool bOpened, bool bNeedToPassThrough );

	// Door has same status as bot thinks.
	void SameDoorStatus( TEntityIndex iDoor, bool bOpened, bool bCheckingDoors );

	// Bot thinks that door was opened when it is closed, and viceversa.
	void DifferentDoorStatus( TEntityIndex iDoor, bool bOpened, bool bCheckingDoors );

	// Bot needs to pass through the door, but it is closed.
	void ClosedDoorOnTheWay( TEntityIndex iDoor, bool bCheckingDoors );

	// Task stack is empty, check if the big task has more steps.
	void CheckBigTask();

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
	void PushSpeakTask( TBotChat iChat, TEntityType iType, TEntityIndex iIndex, int iArguments = 0 );

	// Switch to speak task.
	void SwitchToSpeakTask( TBotChat iChat, TEntityType iType, TEntityIndex iIndex, int iArguments = 0 )
	{
		SaveCurrentTask();
		PushSpeakTask(iChat, iType, iIndex, iArguments);
		m_bTaskFinished = true;
	}

	// Perform speak task, say a phrase.
	void DoSpeakTask( int iArgument );

	// Get new task from stack.
	void TaskPop()
	{
		if ( m_cTaskStack.size() > 0 )
		{
			m_cCurrentTask = m_cTaskStack.back();
			m_cTaskStack.pop_back();
			m_bNewTask = true;
		}
		else
			m_cCurrentTask.iTask = EBorzhTaskInvalid;
		m_bTaskFinished = false;
	}

	// Cancel last task.
	void CancelTask()
	{
		m_cTaskStack.clear();
		m_cCurrentTask.iTask = EBorzhTaskInvalid;
		m_bNeedMove = m_bNeedAim = false;
	}

	// Go to new button and push it.
	void PushCheckButtonTask( TEntityIndex iButton, bool bShoot = false );

	//
	void CheckRemainingPlayersForBigTask();

	//
	void OfferCurrentBigTask();

	// 
	void ReceiveTaskOffer( TBorzhTask iProposedTask, int iArgument, TPlayerIndex iSpeaker );

	// Called when domain has been changed, like found opened/closed door when expecting otherwise, or button has been pressed.
	void DomainChanged() {}

	// Get nearest door that is reachable from current position.
	TEntityIndex GetNearestDoor() {}

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

	good::bitset m_aVisitedAreas;                           // Areas that were explored (1 = explored, 0 = unknown).
	good::bitset m_cReachableAreas;                         // Areas that can be reached from current one.
	good::bitset m_cVisitedAreasAfterPushButton;            // Areas that wasn't visited after pushing buttons.

	good::bitset m_cSeenDoors;                              // Seen doors. Other bot could see it also.
	good::bitset m_cOpenedDoors;                            // Opened doors. Closed door means seen & !opened.
	good::bitset m_cFalseOpenedDoors;                       // Used when checking a button.
	good::bitset m_cCheckedDoors;                           // Useful bitset when checking doors.

	good::bitset m_cSeenButtons;                            // Seen buttons. Other bot could see it also.
	good::bitset m_cPushedButtons;                          // Buttons pushed at least once.

	good::vector<good::bitset> m_cButtonTogglesDoor;        // Bot's belief of which set of doors button DO toggle (button is index in array).
	good::vector<good::bitset> m_cButtonNoAffectDoor;       // Bot's belief of which set of doors button DOESN'T toggle (button is index in array).

	good::bitset m_cDontTestButtons;                        // Buttons that has been tested. When domain change is produced, they will be cleared.
	good::bitset m_cDontTestDoors;                          // Door that has been tested. When domain change is produced, they will be cleared.

	good::bitset m_cCollaborativePlayers;                   // Players that are collaborating with this bot.
	good::bitset m_cAcceptedPlayers;                        // Players that accepted this bot task. All players need to accept task in order to perform it.
	good::bitset m_cBusyPlayers;                            // Players that are busy right now. All players need to accept task in order to perform it.
	good::bitset m_cWaitingPlayers;                         // Players that are waiting for this bot.
	good::vector<TAreaId> m_aPlayersAreas;                  // Bot's belief of where another player is.

	good::vector<TAreaId> m_cDesiredPlayersPositions;       // Desired positions of players, this will be passed to planner.

	float m_fEndWaitTime;                                   // Time when can stop waiting.


protected: // Flags.

	bool m_bUsingPlan;                                      // Using plan obtained from FF planner.

	bool m_bStarted:1;                                      // Started (some admin must say 'begin' or 'start').

	bool m_bNothingToDo:1;                                  // Bot has no more task to do, wait for domain change.
	bool m_bDomainChanged:1;                                // Domain has changed (like button has been pressed).

	bool m_bNewTask:1;                                      // New task need to be initiated.
	bool m_bTaskFinished:1;                                 // Need to pop new task from stack.
	bool m_bHasCrossbow:1;                                  // True if bot has crossbow to shoot buttons.
	bool m_bHasPhyscannon:1;                                // True if bot has physcannon to carry boxes.

	bool m_bAllPlayersIdle:1;                               // Some player has done it's tasks. This bot will OfferCurrentBigTask() when waiting for players answers if this is true.

};


#endif // __BOTRIX_BOT_BORZHMOD__
