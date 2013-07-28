#ifndef __BOTRIX_CLIENTS_H__
#define __BOTRIX_CLIENTS_H__


#include "vector.h"

#include "players.h"
#include "waypoint.h"


//****************************************************************************************************************
/// Class that represents a client, connected to this server.
//****************************************************************************************************************
class CClient: public CPlayer
{

public: // Methods.

	/// Constructor.
	CClient( edict_t* pPlayer, TPlayerIndex iIndex ): CPlayer(pPlayer, iIndex, false) {}


	/// Get Steam id of this player.
	const good::string& GetSteamID() { return m_sSteamId; }


	/// Called when client finished connecting with server (becomes active).
	virtual void Activated();

	/// Called each frame. Will draw waypoints for this player.
	virtual void PreThink();


public: // Members and constants.
	TCommandAccessFlags iCommandAccessFlags;   ///< Access for console commands.

	TWaypointId iDestinationWaypoint;          ///< Path destination (path origin is iCurrentWaypoint).

	TWaypointDrawFlags iWaypointDrawFlags;     ///< Draw waypoint flags for this player.
	TPathDrawFlags iPathDrawFlags;             ///< Draw path flags for this player.

	TEntityTypeFlags iItemTypeFlags;           ///< Items to draw for this player.
	TItemDrawFlags iItemDrawFlags;             ///< Draw item flags for this player.

	bool bAutoCreateWaypoints;                 ///< Generate automatically new waypoints, if distance is too far.
	bool bAutoCreatePaths;                     ///< Generate automatically paths for new waypoint.

	bool bDebuggingEvents;                     ///< Print info when game event is fired.


protected:
	good::string m_sSteamId;                   // Steam id.

};


#endif // __BOTRIX_CLIENTS_H__
