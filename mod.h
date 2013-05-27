#ifndef __BOTRIX_MOD_H__
#define __BOTRIX_MOD_H__


#include <stdlib.h> // rand().

#include "event.h"
#include "item.h"


//****************************************************************************************************************
/// Class for Half Life 2 mod types. By default loads all needed stuff for Half-life 2 deathmatch mod.
//****************************************************************************************************************
class CMod
{

public: // Methods.

	/// Get mod id for this game mod.
	static TModId GetModId() { return m_iModId; }

	/// Load all needed staff for mod.
	static void Load( TModId iModId );

	/// Unload mod.
	static void UnLoad()
	{
		m_iModId = EModId_Invalid;
		sModName = "";
		aTeamsNames.clear();
		m_aBotNames.clear();
		m_aModels.clear();
		m_aEvents.clear();
	}

	/// Called when map finished loading items and waypoints.
	static void MapLoaded();

	/// Return true if map has items or waypoint's of given type.
	static bool HasMapItems( TEntityType iEntityType ) { return m_bMapHas[iEntityType]; }

	/// Get team index from team name.
	static int GetTeamIndex( const good::string& sTeam )
	{
		for ( int i=0; i < aTeamsNames.size(); ++i )
			if ( sTeam == aTeamsNames[i] )
				return i;
		return -1;
	}

	/// Get random bot name from [General] section, key bot_names.
	static const good::string* GetRandomModel( int iTeam )
	{
		DebugAssert( iTeam != iSpectatorTeam );
		if ( (iTeam == iUnassignedTeam) && (m_aModels[iTeam].size() == 0) )
		{
			do {
				iTeam =  rand() % m_aModels.size();
			} while ( (iTeam == iUnassignedTeam) || (iTeam == iSpectatorTeam) );
		}
		return m_aModels[iTeam].size() ? &m_aModels[iTeam][rand() % m_aModels[iTeam].size()] : NULL;
	}

	/// Get random bot name from [General] section, key bot_names.
	static const good::string& GetRandomBotName() { return m_aBotNames[rand() % m_aBotNames.size()]; }

	/// Execute event.
	static void ExecuteEvent( void* pEvent, TEventType iType );


public: // Static members.
	static good::string sModName;            ///< Mod name.
	static StringVector aTeamsNames;         ///< Name of teams.
	static int iUnassignedTeam;              ///< Index of unassigned (deathmatch) team.
	static int iSpectatorTeam;               ///< Index of spectator team.
	static bool bIntelligenceInBotName;      ///< Use bot's intelligence as part of his name.


protected: // Methods.
	friend class CConfiguration; // Give access to next protected methods.

	// Set bot names.
	static void SetBotNames( const StringVector& aBotNames ) { m_aBotNames = aBotNames; }
	
	// Set bot models for team index.
	static void SetBotModels( const StringVector& aModels, int iTeam  )
	{
		if ( iTeam >= m_aModels.size() )
			m_aModels.resize(iTeam+1);
		m_aModels[iTeam] = aModels;
	}

	// Add event and start listening to it.
	static void AddEvent( CEvent* pEvent );


protected: // Members.
	static TModId m_iModId;                                  // Mod id.
	static StringVector m_aBotNames;                         // Available bot names.
	static good::vector<StringVector> m_aModels;             // Available models for teams.
	static good::vector< good::auto_ptr<CEvent> > m_aEvents; // Events this mod handles.
	static bool m_bMapHas[EEntityTypeTotal-1];               // To check if map has items or waypoints of types: health, armor, weapon, ammo.
};


#endif // __BOTRIX_MOD_H__
