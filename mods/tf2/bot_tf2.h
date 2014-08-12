#ifdef BOTRIX_TF2

#ifndef __BOTRIX_BOT_TF2_H__
#define __BOTRIX_BOT_TF2_H__


#include "bot.h"
#include "server_plugin.h"


//****************************************************************************************************************
/// Class representing a bot for Half Life 2 Deathmatch.
// sv_hl2mp_weapon_respawn_time sv_hl2mp_item_respawn_time
//****************************************************************************************************************
class CBot_TF2: public CBot
{

public:
    /// Constructor.
    CBot_TF2( edict_t* pEdict, TBotIntelligence iIntelligence, int iTeam, int iClass );

    //------------------------------------------------------------------------------------------------------------
    // Next functions are mod dependent. You need to implement those in order to make bot moving around.
    //------------------------------------------------------------------------------------------------------------
    /// Called when player becomes active, before first respawn. Sets players model and team.
    virtual void Activated();

    /// Called each time bot is respawned.
    virtual void Respawned();

    /// Called when this bot just killed an enemy.
    virtual void KilledEnemy( int iPlayerIndex, CPlayer* pVictim );

    /// Called when enemy just shot this bot.
    virtual void HurtBy( int iPlayerIndex, CPlayer* pAttacker, int iHealthNow );

    /// Set move and aim variables. You can also set shooting/crouching/jumping buttons in m_cCmd.buttons.
    virtual void Think();

    /// Called when chat arrives from other player.
    virtual void ReceiveChat( int iPlayerIndex, CPlayer* pPlayer, bool bTeamOnly, const char* szText );


protected:

    // Inherited from CBot. Will check if arrived at m_iTaskDestination and invalidates current task.
    virtual bool DoWaypointAction();

    // Bot just picked up given item.
    virtual void PickItem( const CEntity& cItem, TEntityType iEntityType, TEntityIndex iIndex )
    {
        CBot::PickItem( cItem, iEntityType, iIndex );
        if ( m_bFlee && (iEntityType == EEntityTypeHealth) )
        {
            m_bFlee = ( m_pPlayerInfo->GetHealth() < (CMod::iPlayerMaxHealth/8) );
            m_bDontAttack = m_bFlee;
        }
    }

    // Check if new tasks are needed.
    void CheckNewTasks( bool bForceTaskChange );

    // Mark task as finished.
    void TaskFinished();

    good::bitset m_aWaypoints;                           // Waypoints, that bot can't use.

    int m_iCurrentTask;                                  // Current task.
    TWaypointId m_iTaskDestination;                      // Waypoint for task destination.
    CPickedItem m_cItemToSearch;                         // Item, we are searching right now. If iType is -1 then iIndex
                                                         // is waypoint (found no items, so heading to waypoint of that type).

    TWaypointId m_iFailWaypoint;                         // Waypoint id where bot stucks.
    int m_iFailsCount;                                   // Times bot stucks at m_iFailWaypoint (at 3 times will change destination).

    good::bitset m_cSkipWeapons;                         // Weapons to skip searching.

    int m_iDesiredTeam;                                  // Desired team to join when activated. TODO: use var in bot.h.

protected: // Flags.

    bool m_bNeedTaskCheck:1;                             // True if there is need to check for new tasks.

};

#endif // __BOTRIX_BOT_TF2_H__

#endif // BOTRIX_TF2
