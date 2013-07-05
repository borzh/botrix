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

	// Called when bot figures out if a button toggles or doesn't affect a door.
	void SetButtonTogglesDoor( TEntityIndex iButton, TEntityIndex iDoor, bool bToggle )
	{
		if ( bToggle )
			m_cDoorToggle[iButton].set(iDoor);
		else
			m_cDoorNoAffect[iButton].set(iDoor);
	}

	// Check if given button toggles given door.
	TYesNoAnswer IsButtonTogglesDoor( TEntityIndex iButton, TEntityIndex iDoor )
	{
		if ( m_cDoorNoAffect[iButton].test(iDoor) )
			return ENo;
		else if ( m_cDoorToggle[iButton].test(iDoor) )
			return EYes;
		else
			return EUnknown;
	}

	// Init current task.
	void InitNewTask();

	// Speak about door/button/box/weapon.
	void PushSpeakTask( TBotChat iChat, TEntityType iType, TEntityIndex iIndex, int iArguments = 0 );

	// Perform speak task, say a phrase.
	void DoSpeakTask();

	// Get new task from stack.
	void TaskPop()
	{
		if ( m_cTaskStack.size() > 0 )
		{
			m_cCurrentTask = m_cTaskStack.back();
			m_cTaskStack.erase( m_cTaskStack.size() - 1 );
		}
		else
			m_cCurrentTask.iTask = EBorzhTaskInvalid;
		m_bNewTask = true;
		m_bTaskFinished = false;
	}

	// Check if there is something to do.
	void TaskUpdate();


protected: // Members.

	class CBorzhTask
	{
	public:
		TBorzhTask iTask;                                // Task number.
		int iArgument;                                   // Task argument. May be number of door, button, etc.
	};

	typedef good::vector< CBorzhTask > task_stack_t;     // Typedef for stack of tasks.

	task_stack_t m_cTaskStack;                           // Stack of tasks.
	CBorzhTask m_cCurrentTask;                           // Current bot's task.

	TAreaId m_iCurrentArea;                              // Current area.

	good::bitset m_cSeenDoors;                           // Seen doors.
	good::bitset m_cKnownDoors;                          // Known doors, other bot has spoken of. TODO: implement.
	good::bitset m_cOpenedDoors;                         // Opened doors. Closed door means seen & !opened.

	good::bitset m_cSeenButtons;                         // Seen buttons.
	good::bitset m_cKnownButtons;                        // Known buttons, other bot has spoken of. TODO: implement.
	good::bitset m_cPushedButtons;                       // Buttons pushed at least once.

	good::vector<good::bitset> m_cDoorToggle;            // Bot's belief of which set of doors button DO toggle (button is index in array).
	good::vector<good::bitset> m_cDoorNoAffect;          // Bot's belief of which set of doors button DOESN'T toggle (button is index in array).

	good::bitset m_aVisitedWaypoints;                    // Visited waypoints (1 = visited, 0 = unknown).
	good::bitset m_aVisitedAreas;                        // Visited areas (1 = visited, 0 = unknown).
	good::bitset m_aCheckedAreas;                        // Checked areas for current task (1 = checked, 0 = unchecked).

	float m_fEndWaitTime;                                // Time when can stop waiting.

	//TWaypointId m_iDestination;                          // Destination waypoint.


protected: // Flags.

	static bool m_bUsingPlanner;                         // Set if some of the bots is using planner.

	bool m_bStarted:1;                                   // Started by admin.

	bool m_bNewTask:1;                                   // New task need to be initiated.
	bool m_bTaskFinished:1;                              // Need to pop new task from stack.
	bool m_bHasCrossbow:1;                               // True if bot has crossbow to shoot buttons.
	bool m_bHasPhyscannon:1;                             // True if bot has physcannon to carry boxes.

};


#endif // __BOTRIX_BOT_BORZHMOD__
