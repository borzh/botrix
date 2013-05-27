#ifndef __BOTRIX_PLAYER_H__
#define __BOTRIX_PLAYER_H__


#include "edict.h"
#include "iplayerinfo.h"

#include "types.h"

#include "util.h"


#if defined(DEBUG) || defined(_DEBUG)
#	define DRAW_PLAYER_HULL 0
#endif


class CClient; // Forward declaration.

typedef int TPlayerIndex; ///< Index of player in array of players.


//****************************************************************************************************************
/// Abstract class that defines a player.
//****************************************************************************************************************
class CPlayer
{
public:
	/// Constructor.
	CPlayer(edict_t* pEdict, TPlayerIndex iIndex, bool bIsBot): m_pEdict(pEdict), m_iIndex(iIndex), m_bBot(bIsBot), 
		m_pPlayerInfo(NULL), iCurrentWaypoint(-1), iNextWaypoint(-1), iPrevWaypoint(-1), iChatMate(-1), m_bAlive(false) {}


	/// Return true if player is a bot.
	bool IsBot() const { return m_bBot; }

	/// Return true if player is alive.
	bool IsAlive() const { return m_bAlive; }


	/// Get edict of player.
	TPlayerIndex GetIndex() const { return m_iIndex; }

	/// Get edict of player.
	edict_t* GetEdict() const { return m_pEdict; }

	/// Return bot's team.
	int GetTeam() { return m_pPlayerInfo->GetTeamIndex(); }

	/// Get player's info.
	IPlayerInfo* GetPlayerInfo() const { return m_pPlayerInfo; }

	/// Get name of this player.
	const char* GetName() const { return m_pPlayerInfo ? m_pPlayerInfo->GetName() : NULL; }

	/// Get lowercase name of this player.
	const good::string& GetLowerName() const { return m_sName; }

	/// Get head position of player.
	Vector const& GetHead() const { return m_vHead; }

	/// Get previous head position of player.
	Vector const& GetPreviousHead() const { return m_vPrevHead; }

	/// Get center position of player.
	void GetCenter( Vector& v ) const { CUtil::EntityCenter(m_pEdict, v); }

	/// Get foot position of player.
	void GetFoot( Vector& v ) const { v = m_pPlayerInfo->GetAbsOrigin(); }

	/// Get player's view angles.
	void GetEyeAngles( QAngle& a ) const
	{
		CBotCmd cCmd= m_pPlayerInfo->GetLastUserCommand();
		a = cCmd.viewangles;
	}


	//------------------------------------------------------------------------------------------------------------
	// Virtual functions, client or bot should override these.
	//------------------------------------------------------------------------------------------------------------
	/// Called when client finished connecting with server (becomes active).
	virtual const char* GetStatus() { return GetName(); }

	/// Called when client finished connecting with server (becomes active).
	virtual void Activated();

	/// Player is respawned. Set up current waypoint and head position. 
	virtual void Respawned();

	/// Called when player becomes dead.
	virtual void Dead() { m_bAlive = false; }

	/// Called each frame. Update current waypoint and head position.
	virtual void Think();


public:
	TWaypointId iCurrentWaypoint;    ///< Nearest waypoint to player's position.
	TWaypointId iNextWaypoint;       ///< Next waypoint in path, used by bot to check if touching next waypoint (to perform actions).
	TWaypointId iPrevWaypoint;       ///< Previous waypoint in path, used by bot to rewind action if action fails.

	TPlayerIndex iChatMate;          ///< Current chat mate.


protected:
	edict_t* m_pEdict;               // Player's edict.
	TPlayerIndex m_iIndex;           // Player's index.
	IPlayerInfo* m_pPlayerInfo;      // Player's info.
#if DRAW_PLAYER_HULL
	float m_fNextDrawHullTime;       // Next time to draw player's hull.
#endif

	good::string m_sName;            // Lowercased name of player.
	Vector m_vHead;                  // Head position of player.
	Vector m_vPrevHead;              // Previous position of player.

	bool m_bBot:1;                   // This member will be set to true by bot.
	bool m_bAlive:1;                 // IPlayerInfo::IsDead() returns true only when player is dead, not when becomes respawnable.

};




//****************************************************************************************************************
/// Class that holds both clients and bots.
//****************************************************************************************************************
class CPlayers
{
public:
	/// Get count of players on this server.
	static int GetMaxPlayers() { return (int)m_aPlayers.size(); };

	/// Get bots count.
	static int GetBotsCount() { return m_iBotsCount; }

	/// Get clients count.
	static int GetClientsCount() { return m_iClientsCount; }

	/// Get players count.
	static int GetPlayersCount() { return m_iClientsCount + m_iBotsCount; }

	/// Get player from index.
	static CPlayer* Get( TPlayerIndex iIndex ) { return m_aPlayers[iIndex].get(); }

	/// Get player index from edict.
	static int Get( edict_t* pPlayer )
	{
		return CBotrixPlugin::pEngineServer->IndexOfEdict(pPlayer)-1;
	}

	/// Get client that created listen server (not dedicated server).
	static CClient* GetListenServerClient() { return m_pListenServerClient; }


	/// Start new map.
	static void Init( int iMaxPlayers );

	/// End map.
	static void Clear();


	//------------------------------------------------------------------------------------------------------------
	// Bot handling.
	//------------------------------------------------------------------------------------------------------------
	/// Add bot on random team.
	static CPlayer* AddBot( TBotIntelligence iIntelligence = EBotPro );

	/// Add bot on given team.
	static CPlayer* AddBotOnTeam( int team, TBotIntelligence iIntelligence = EBotPro );

	/// Kick given bot.
	static void KickBot( CPlayer* pPlayer );

	/// Kick random bot on random team.
	static bool KickRandomBot();

	/// Kick random bot on given team.
	static bool KickRandomBotOnTeam( int team );

	/// Deliver chat / voice command to bots.
	static void DeliverChat( edict_t* pFrom, bool bTeamOnly, const char* szText );

	/// Deliver message to all clients.
	static void Message( const char* szFormat, ... );


	/// Called when player connects this server.
	static void PlayerConnected( edict_t* pEdict );

	/// Called when player disconnects from this server.
	static void PlayerDisconnected( edict_t* pEdict );

	/// Called when player starts to play (joins some team).
	static void PlayerRespawned( edict_t* pEdict )
	{
		int idx = CBotrixPlugin::pEngineServer->IndexOfEdict(pEdict)-1;
		if ( (idx >= 0)  &&  m_aPlayers[idx].get() )
			m_aPlayers[idx]->Respawned();
	}


	/// Called each frame. Will make players and bots 'think'.
	static void Think();


	//------------------------------------------------------------------------------------------------------------
	// Debugging events.
	//------------------------------------------------------------------------------------------------------------
	/// Returns true if some client is debugging events.
	static bool IsDebuggingEvents() { return m_bClientDebuggingEvents; }

	/// Check if some client is debugging events. If so, DebugEvent() with event info will be called.
	static void CheckForDebugging();

	/// Display info message on client that is debugging events.
	static void DebugEvent( const char *szFormat, ... );


protected:
	typedef good::vector< good::auto_ptr<CPlayer> > PlayersArray;

	static PlayersArray m_aPlayers;
	static CClient* m_pListenServerClient;                  // Client that created listen server (not dedicated server).
	static bool m_bClientDebuggingEvents;                   // True if some client is debugging event messages.

	static int m_iClientsCount;                             // Total amount of clients on this server.
	static int m_iBotsCount;                                // Total amount of bots on this server.

	static good::vector< good::vector<bool> > m_iChatPairs; // 2D array representing who chats with whom. 
};

#endif // __BOTRIX_PLAYER_H__
