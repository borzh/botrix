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
	virtual void ReceiveChat( int iPlayerIndex, CPlayer* pPlayer , bool bTeamOnly, const char* szText ) {}

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
	void ButtonTogglesDoor( TEntityIndex iButton, TEntityIndex iDoor, bool bToggle )
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

	// Speak about door/button/box/weapon,
	//void Speak( TBotChat iChat, TEntityIndex iEntity, TBorzhChatArgument iArgument );

	// Receive chat from other bot/player about domain change.
	//void ReceiveSpeak( TBotChat iChat, TEntityIndex iEntity, TBorzhChatArgument iArgument );

	// Check if there is something to do.
	void TaskUpdate();

	// Save current task in task stack (will resume it later) and set new task.
	void TaskChange( TBorzhTask iTask, int iTaskArgument );

	// Check if there is something to do.
	void TaskCheck();


protected: // Members.

	//

	typedef good::pair<TBorzhTask, int> task_t;
	typedef good::vector< task_t > task_stack_t;
	task_stack_t m_cTaskStack;                           // Stack of tasks.

	TBorzhTask m_iCurrentTask;                           // Current bot's task.
	int m_iTaskArgument;                                 // Task argument.

	good::bitset m_cSeenDoors;                           // Seen doors.
	good::bitset m_cOpenedDoors;                         // Opened doors. Closed implies (seen & !opened).

	good::bitset m_cSeenButtons;                         // Seen buttons.
	good::bitset m_cPushedButtons;                       // Buttons pushed at least once.

	good::vector<good::bitset> m_cDoorToggle;            // Bot's belief of which set of doors button DO toggle (button is index in array).
	good::vector<good::bitset> m_cDoorNoAffect;          // Bot's belief of which set of doors button DOESN'T toggle (button is index in array).

	good::bitset m_aUnknownWaypoints;                    // Not visited waypoints (1 = unknown, 0 = visited).
	good::bitset m_aUnknownAreas;                        // Visited areas (1 = unknown, 0 = visited).
	//good::bitset m_aCheckedAreas;                        // Checked areas for current task (1 = checked, 0 = unchecked).

	float fWaitTime;                                     // Time when can stop waiting.

	TWaypointId m_iDestination;                          // Destination waypoint.

	TEntityIndex iCheckVisibleDoor;                      // Door to check visibility in this frame.
	TEntityIndex iCheckButton;                           // Door to check visibility in this frame.

protected: // Flags.

	bool m_bNeedTaskCheck:1;                             // True if there is need to check for new tasks.
	bool m_bTaskFinished:1;                              // True if there current task is finished.

};


#endif // __BOTRIX_BOT_BORZHMOD__
