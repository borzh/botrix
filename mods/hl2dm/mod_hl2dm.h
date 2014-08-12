#ifdef BOTRIX_HL2DM

#ifndef __BOTRIX_MOD_HL2DM_H__
#define __BOTRIX_MOD_HL2DM_H__


#include "mod.h"


/// Class for Team Fortress 2 mod.
class CModHL2DM: public IMod
{

public: // Methods.
    /// Constructor. Initializes console commands.
    CModHL2DM();

    //------------------------------------------------------------------------------------------------------------
    // Implementation of IMod inteface.
    //------------------------------------------------------------------------------------------------------------
    /// Process configuration file.
    virtual bool ProcessConfig( good::ini_file cIni );


    /// Called when map is loaded, after waypoints and items has been loaded.
    virtual void MapLoaded() {}

    /// Called when map is unloaded.
    virtual void MapFinished() {}


    /// Add bot with given name, intelligence, class and other optional parameters.
    virtual CPlayer* AddBot( const char* szName, TBotIntelligence iIntelligence, TTeam iTeam,
                                 TClass iClass, int iParamsCount, const char **aParams );


    /// Get waypoint type count.
    virtual int GetWaypointTypeCount() { return 0; }

    /// Get waypoint type names.
    virtual const good::string* GetWaypointTypeNames() { return NULL; }

    /// Get waypoint type colors.
    virtual const int* GetWaypointTypeColors() { return NULL; }


    /// Get waypoint path count.
    virtual int GetWaypointPathCount() { return 0; }

    /// Get waypoints path names.
    virtual const good::string* GetWaypointPathNames() { return NULL; }

    /// Get waypoints path colors.
    virtual const int* GetWaypointPathColors() { return NULL; }


    /// Get chat count.
    virtual int GetChatCount() { return 0; }

    /// Get chat names.
    virtual const good::string* GetChatNames() { return NULL; }

    /// Mod think function.
    virtual void Think() {}


    //------------------------------------------------------------------------------------------------------------
    /// Get random bot model.
    const good::string* GetRandomModel( int iTeam )
    {
        BASSERT( iTeam != CMod::iSpectatorTeam, return NULL );

        // Get random team if iTeam is unassigned (deathmatch) team.
        if ( (iTeam == CMod::iUnassignedTeam) && (m_aModels[iTeam].size() == 0) )
        {
            do {
                iTeam = rand() % m_aModels.size();
            } while ( (iTeam == CMod::iUnassignedTeam) || (iTeam == CMod::iSpectatorTeam) );
        }
        return m_aModels[iTeam].size() ? &m_aModels[iTeam][rand() % m_aModels[iTeam].size()] : NULL;
    }


protected:
    good::vector<StringVector> m_aModels; ///< Available models for teams.

};

#endif // __BOTRIX_MOD_HL2DM_H__

#endif // BOTRIX_HL2DM
