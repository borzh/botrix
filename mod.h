#ifndef __BOTRIX_MOD_H__
#define __BOTRIX_MOD_H__


#include <stdlib.h> // rand()
#include <good/memory.h> // unique_ptr

#include "event.h"
#include "item.h"

//****************************************************************************************************************
/// Mod interface.
/**
 * Used to get strings, colors from types or flags. TODO:
 */
//****************************************************************************************************************
abstract class IMod
{
public: // Methods.

    ///
    virtual void LoadConfig(const good::string& sModName) = 0;
    // ProcessUnknownSection(const good::inifile::inisection...);


    /// Called when map is loaded, after waypoints and items has been loaded.
    virtual void MapLoaded() = 0;


    /// Get waypoint type count.
    virtual int GetWaypointTypeCount() = 0;

    /// Get waypoint type names.
    virtual const good::string* GetWaypointTypeNames() = 0;

    /// Get waypoint type colors.
    virtual const int* GetWaypointTypeColors() = 0;


    /// Get waypoint path count.
    virtual int GetWaypointPathCount() = 0;

    /// Get waypoints path names.
    virtual const good::string* GetWaypointPathNames() = 0;

    /// Get waypoints path colors.
    virtual const int* GetWaypointPathColors() = 0;


    /// Get bot's objective count.
    virtual int GetObjectivesCount() = 0;

    /// Get bot's objective names.
    virtual const good::string* GetObjectiveNames() = 0;


    /// Get chat count.
    virtual int GetChatCount() = 0;

    /// Get chat names.
    virtual const good::string* GetChatNames() = 0;

    /// Mod think function.
    virtual void Think() = 0;


};


//****************************************************************************************************************
/// Class for Half Life 2 mod types. By default loads all needed stuff for Half-life 2 deathmatch mod.
//****************************************************************************************************************
class CMod
{

public: // Methods.

    static IMod* pCurrentMod;

    /// Get mod id for this game mod.
    static TModId GetModId() { return m_iModId; }

    /// Load all needed staff for mod.
    static bool Load( TModId iModId );

    // Add event and start listening to it.
    static bool AddEvent( CEvent* pEvent );

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
        for ( size_t i=0; i < aTeamsNames.size(); ++i )
            if ( sTeam == aTeamsNames[i] )
                return i;
        return -1;
    }

    /// Get random bot name from [General] section, key bot_names.
    static const good::string* GetRandomModel( int iTeam )
    {
        // TODO: check if works with CSS.
        BASSERT( iTeam != iSpectatorTeam, return NULL );
        if ( (iTeam == iUnassignedTeam) && (m_aModels[iTeam].size() == 0) )
        {
            do {
                iTeam =  rand() % m_aModels.size();
            } while ( (iTeam == iUnassignedTeam) || (iTeam == iSpectatorTeam) );
        }
        return m_aModels[iTeam].size() ? &m_aModels[iTeam][rand() % m_aModels[iTeam].size()] : NULL;
    }

    /// Get random bot name from [General] section, key bot_names.
    static const good::string& GetRandomBotName();

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

    // Returns true there is a player/bot with name cName.
    static bool IsNameTaken(const good::string& cName);

    // Set bot names.
    static void SetBotNames( const StringVector& aBotNames ) { m_aBotNames = aBotNames; }

    // Set bot models for team index.
    static void SetBotModels( const StringVector& aModels, int iTeam  )
    {
        if ( (StringVector::size_type)iTeam >= m_aModels.size() )
            m_aModels.resize(iTeam+1);
        m_aModels[iTeam] = aModels;
    }


protected: // Members.
    static TModId m_iModId;                                  // Mod id.
    static StringVector m_aBotNames;                         // Available bot names.
    static good::vector<StringVector> m_aModels;              // Available models for teams.
    static good::vector<CEventPtr> m_aEvents;                 // Events this mod handles.
    static bool m_bMapHas[EEntityTypeTotal-1];               // To check if map has items or waypoints of types: health, armor, weapon, ammo.
};


#endif // __BOTRIX_MOD_H__
