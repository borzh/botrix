#ifndef __BOTRIX_BOT_BORZHMOD__
#define __BOTRIX_BOT_BORZHMOD__


#include "bot.h"
#include "server_plugin.h"


typedef int TBotTaskBorzh;


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
	/// Called each time bot is respawned.
	virtual void Respawned();

	/// Called when this bot just killed an enemy.
	virtual void KilledEnemy( int iPlayerIndex, CPlayer* pVictim );

	/// Called when enemy just shot this bot.
	virtual void HurtBy( int iPlayerIndex, CPlayer* pAttacker, int iHealthNow );

	/// Set move and aim variables. You can also set shooting/crouching/jumping buttons in m_cCmd.buttons.
	virtual void Move();

	/// Called when chat arrives from other player.
	virtual void ReceiveChat( int iPlayerIndex, CPlayer* pPlayer , bool bTeamOnly, const char* szText ) {}

	/// Called when chat request arrives from other player.
	virtual void ReceiveChatRequest( const CBotChat& cRequest );


protected:

	// Inherited from CBot. Will check if arrived at m_iTaskDestination and invalidates current task.
	virtual bool DoWaypointAction();

	// Bot just picked up given item.
	virtual void PickItem( const CEntity& cItem, TEntityType iEntityType, TEntityIndex iIndex )
	{
		CBot::PickItem( cItem, iEntityType, iIndex );
		ConsoleCommand("say Hey, I just found %s.", cItem.pItemClass->sClassName.c_str());
	}

	// Check if there is something to do.
	void CheckNewTasks();

	good::bitset m_aUnknownWaypoints;                    // Not visited waypoints (1 = unknown, 0 = visited).
	good::bitset m_aUnknownAreas;                        // Not visited areas (1 = unknown, 0 = visited).

	TBotTaskBorzh m_iCurrentTask;                        // Current task.
	int m_iTaskArgument;                                 // Task argument.

	TWaypointId m_iDestination;                          // Destination waypoint.

protected: // Flags.

	bool m_bNeedTaskCheck:1;                             // True if there is need to check for new tasks.

};


#endif // __BOTRIX_BOT_BORZHMOD__
