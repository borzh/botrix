#ifndef __BOTRIX_CONFIG_H__
#define __BOTRIX_CONFIG_H__


#include "types.h"

#include <good/ini_file.h>


//****************************************************************************************************************
/// Class for storing configuration in ini file format.
//****************************************************************************************************************
class CConfiguration
{

public: // Methods.

    /// Load configuration file. You need also provide game and mod folders in order to detect mod to use.
    static TModId Load( const good::string& sFileName, const good::string& sGameDir, const good::string& sModDir );

    /// Save configuration file (must be loaded first).
    static void Save()
    {
        if ( m_bModified )
        {
            m_iniFile.save();
            m_bModified = false;
        }
    }

    /// Unload configuration file, free all memory.
    static void Unload() { m_iniFile.clear(); }

    /// Get value for sKey of [General] section. Could be NULL.
    static const good::string* GetGeneralSectionValue( const good::string& sKey );

    /// Set value for sKey of [General] section.
    static void SetGeneralSectionValue( const good::string& sKey, const good::string& sValue );

    /// Get client access bits to execute commands on server (@see ). Section [User Access] is used.
    static TCommandAccessFlags ClientAccessLevel( const good::string& sSteamId );

    /// Set client access bits to execute commands on server (@see ).
    static void SetClientAccessLevel( const good::string& sSteamId, TCommandAccessFlags iAccess );


protected: // Members.
    static good::ini_file m_iniFile; // Ini file.
    static bool m_bModified;         // True if something was modified (to save later).

};


#endif // __BOTRIX_CONFIG_H__
