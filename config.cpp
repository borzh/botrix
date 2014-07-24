#include <stdlib.h> // atoi
#include <good/string_buffer.h>

#include <good/string_utils.h>

#include "chat.h"
#include "config.h"
#include "mod.h"
#include "source_engine.h"
#include "type2string.h"
#include "types.h"
#include "weapon.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#if defined(DEBUG) || defined(_DEBUG)
#	define ConfigMessage(...) CUtil::Message(NULL, __VA_ARGS__)
#else
#	define ConfigMessage(...)
#endif

#	define ConfigError(...) CUtil::Message(NULL, __VA_ARGS__)



extern char* szMainBuffer;
extern int iMainBufferSize;


//------------------------------------------------------------------------------------------------------------
good::ini_file CConfiguration::m_iniFile; // Ini file.
bool CConfiguration::m_bModified;         // True if something was modified (to save later).

//------------------------------------------------------------------------------------------------------------
TModId CConfiguration::Load( const good::string& sFileName, const good::string& sGameDir, const good::string& sModDir )
{
    TModId iModId = EModId_Invalid;

    StringVector aBotNames, aModels, aTeams;
    aBotNames.reserve(32);
    aModels.reserve(32);
    aTeams.reserve(4);

    good::string_buffer sbBuffer(szMainBuffer, iMainBufferSize, false);

    m_iniFile.name = sFileName;
    if ( m_iniFile.load() >= good::IniFileNotFound )
    {
        ConfigError("Error reading configuration file %s", m_iniFile.name.c_str());
        return EModId_Invalid;
    }

    m_bModified = false;

    // Process general section.
    good::ini_file::iterator it = m_iniFile.find("General");
    if ( it != m_iniFile.end() )
    {
        // Check if need to log to a file.
        good::ini_section::iterator kv = it->find("log_to_file");
        if ( kv != it->end() )
        {
            int iValue = CTypeToString::BoolFromString(kv->value);
            if ( iValue == -1 )
                CUtil::Message( NULL, "File \"%s\", section [%s]: invalid parameter %s for %s.", m_iniFile.name.c_str(),
                                it->name.c_str(), kv->value.c_str(), kv->key.c_str() );
            else
            {
                CUtil::bLogMessageToFile = (iValue != 0);
                ConfigMessage("Log to file: %s.", kv->value.c_str());
            }
        }

        // Check if need to show intelligence in it's name.
        kv = it->find("name_with_intelligence");
        if ( kv != it->end() )
        {
            int iValue = CTypeToString::BoolFromString(kv->value);
            if ( iValue == -1 )
                CUtil::Message( NULL, "File \"%s\", section [%s]: invalid parameter %s for %s.", m_iniFile.name.c_str(),
                                it->name.c_str(), kv->value.c_str(), kv->key.c_str() );
            else
            {
                CMod::bIntelligenceInBotName = (iValue != 0);
                ConfigMessage("Intelligence in bot name: %s.", kv->value.c_str());
            }
        }

        // Get bot names.
        kv = it->find("names");
        if ( kv == it->end() )
            ConfigError("File \"%s\", section [%s]: missing bot_names.", m_iniFile.name.c_str(), it->name.c_str());
        else
        {
            sbBuffer = kv->value;
            good::escape(sbBuffer);
            good::split((good::string)sbBuffer, aBotNames, ',', true);
        }

        ConfigMessage("Bot names:");
        for ( size_t i = 0; i < aBotNames.size(); ++i )
            ConfigMessage( "  %s", aBotNames[i].c_str() );

    }
    else
        ConfigError("File %s, missing [%s] section.", m_iniFile.name.c_str(), it->name.c_str());

    CMod::SetBotNames(aBotNames);

    // Set current mod from mods sections.
    for ( it = m_iniFile.begin(); it != m_iniFile.end(); ++it )
    {
        if ( good::ends_with(it->name, ".mod") )
        {
            good::ini_section::iterator games = it->find("games");
            good::ini_section::iterator mods = it->find("mods");
            if ( (games != it->end()) && (mods != it->end()) )
            {
                sbBuffer = games->value;
                good::lower_case(sbBuffer);
                StringVector aGameDirs;
                good::split((good::string)sbBuffer, aGameDirs, ',', true);
                for ( StringVector::const_iterator gameDirsIt = aGameDirs.begin(); gameDirsIt != aGameDirs.end(); ++gameDirsIt )
                {
                    if ( *gameDirsIt == sGameDir )
                    {
                        sbBuffer = mods->value;
                        good::escape(sbBuffer);
                        good::list<good::string> aModsDirs;
                        good::split( (good::string)sbBuffer, aModsDirs, ',', true );
                        for ( good::list<good::string>::const_iterator modDirsIt = aModsDirs.begin(); modDirsIt != aModsDirs.end(); ++modDirsIt )
                        {
                            if ( *modDirsIt == sModDir )
                            {
                                // Get bot used in this mod.
                                good::ini_section::iterator bot = it->find("bot");
                                if ( bot != it->end() )
                                {
                                    iModId = CTypeToString::ModFromString(bot->value);
                                    if ( iModId == EModId_Invalid )
                                    {
                                        ConfigError("File \"%s\", section [%s], invalid bot -> %s.", m_iniFile.name.c_str(), it->name.c_str(), bot->value.c_str());
                                        ConfigError("Using default bot: hl2dm.");
                                        iModId = EModId_HL2DM;
                                    }
                                }
                                else
                                {
                                    ConfigError("Not 'bot' key in section [%s]. Using default bot: hl2dm.", it->name.c_str());
                                    iModId = EModId_HL2DM;
                                }

                                // Get player teams.
                                good::ini_section::iterator teams = it->find("teams");
                                if ( teams != it->end() )
                                {
                                    sbBuffer = teams->value;
                                    good::escape(sbBuffer);
                                    good::split( (good::string)sbBuffer, aTeams, ',', true );

                                    ConfigMessage("Team names:");
                                    for ( size_t i = 0; i < aTeams.size(); ++i )
                                    {
                                        if ( aTeams[i] == "unassigned" )
                                            CMod::iUnassignedTeam = i;
                                        else if ( aTeams[i] == "spectators" )
                                            CMod::iSpectatorTeam = i;
                                        ConfigMessage( "  %s", aTeams[i].c_str() );
                                    }
                                }

                                // Get player models.
                                for ( size_t i = 0; i < aTeams.size(); ++i )
                                {
                                    if ( aTeams[i] == "spectators" )
                                        continue;

                                    sbBuffer = "models ";
                                    sbBuffer << aTeams[i];

                                    good::ini_section::iterator models = it->find(sbBuffer);
                                    if ( models != it->end() )
                                    {
                                        sbBuffer = models->value;
                                        good::escape(sbBuffer);
                                        good::split( (good::string)sbBuffer, aModels, ',', true );
#if defined(DEBUG) || defined(_DEBUG)
                                        ConfigMessage("Model names for team %s:", aTeams[i].c_str());
                                        for ( int j = 0; j < (int)aModels.size(); ++j )
                                            ConfigMessage( "  %s", aModels[j].c_str() );
#endif
                                        CMod::SetBotModels( aModels, i );
                                    }
                                }

                                CMod::aTeamsNames = aTeams;

                                // Get mod name in sbBuffer.
                                sbBuffer = it->name;
                                sbBuffer.erase( sbBuffer.length() - 4, 4 ); // Erase ".mod" from section name.

                                goto mod_found;
                            }
                        }
                    }
                }
            }
            else
                ConfigError("File \"%s\", section [%s], invalid mod section (missing games/mods keys).", m_iniFile.name.c_str(), it->name.c_str());
        }
    }
    mod_found:

    if ( iModId == EModId_Invalid )
    {
        ConfigError("File %s: there is no mod available that matches current game & mod folders.", m_iniFile.name.c_str());
        ConfigError("Using default mod 'HalfLife2Deathmatch' with bot 'hl2dm', no items available.");
        CMod::sModName = "HalfLife2Deathmatch";
        sbBuffer = CMod::sModName;
    }
    else
    {
        ConfigMessage("Current mod: '%s'.", sbBuffer.c_str());
        CMod::sModName.assign(sbBuffer, true);
    }

    // Load health /armor / object entity classes.
    for ( TEntityType iType = 0; iType < EEntityTypeTotal; ++iType )
    {
        // Get mod item section, i.e. [HalfLife2Deathmatch.item.health].
        sbBuffer.erase( CMod::sModName.size() );
        sbBuffer << ".items.";
        sbBuffer <<  CTypeToString::EntityTypeToString(iType);

        good::ini_file::iterator it = m_iniFile.find( sbBuffer );
        if ( it != m_iniFile.end() )
        {
            CItems::SetEntityClassesSizeForType( iType, it->size() ); // Reserve needed space, as we will use pointers to that space.

            // Iterate throught key values.
            for ( good::ini_section::const_iterator itemIt = it->begin(); itemIt != it->end(); ++itemIt )
            {
                CEntityClass cEntityClass;
                cEntityClass.sClassName.assign( itemIt->key.c_str(), itemIt->key.size() ); // String instance will live until plugin is unloaded.

                // Get item flags.
                StringVector aArguments;
                good::split(itemIt->value, aArguments, ',', true);
                for ( size_t i=0; i < aArguments.size(); ++i )
                {
                    StringVector aCurrent;
                    good::split<good::vector>(aArguments[i], aCurrent);

                    int iFlag = CTypeToString::EntityClassFlagsFromString(aCurrent[0]);

                    if ( iFlag == -1 )
                        ConfigError("File \"%s\", section [%s], invalid entity flag %s for %s.",  m_iniFile.name.c_str(), it->name.c_str(), aArguments[i].c_str(), itemIt->key.c_str());
                    else
                    {
                        FLAG_SET(iFlag, cEntityClass.iFlags);
                        /*if ( iFlag == FEntityRespawnable ) // Check respawn time.
                        {
                            if ( aCurrent.size() == 2 )
                            {
                                int iValue = -1;
                                sscanf(aCurrent[1].c_str(), "%d", &iValue);
                                if ( iValue > 0 )
                                    SET_2ND_WORD(iValue, cEntityClass.iFlags);
                                else
                                    ConfigError("File \"%s\", section [%s], invalid respawn time for: %s.",  m_iniFile.name.c_str(), it->name.c_str(), itemIt->key.c_str());
                            }
                            else if ( aCurrent.size() > 2 )
                                ConfigError("File \"%s\", section [%s], invalid arguments count for: %s.",  m_iniFile.name.c_str(), it->name.c_str(), itemIt->key.c_str());
                        }*/
                    }
                }

                CItems::AddItemClassFor( iType, cEntityClass );
            }
        }
    }

    // Load object models.
    sbBuffer = CMod::sModName;
    sbBuffer.erase( CMod::sModName.size() );
    sbBuffer << ".items.object.models";
    it = m_iniFile.find( sbBuffer );
    if ( it != m_iniFile.end() )
    {
        // Iterate throught key values.
        for ( good::ini_section::const_iterator itemIt = it->begin(); itemIt != it->end(); ++itemIt )
        {
            int iValue = CTypeToString::EntityClassFlagsFromString(itemIt->value);
            if ( iValue > 0 )
                CItems::SetObjectFlagForModel(iValue, itemIt->key);
            else
            {
                ConfigError("File \"%s\", section [%s], invalid object model flag: %s.",  m_iniFile.name.c_str(), it->name.c_str(), itemIt->value.c_str());
                ConfigError("Can be one of: %s", CTypeToString::EntityClassFlagsToString(FEntityAll).c_str());
            }
        }
    }

    // Load weapons.
    CWeapons::Clear();

    ConfigMessage("Weapons:");
    sbBuffer.erase( CMod::sModName.size() );
    sbBuffer << ".weapons";
    it = m_iniFile.find( sbBuffer );
    if ( it != m_iniFile.end() )
    {
        // Iterate throught key values.
        for ( good::ini_section::const_iterator itemIt = it->begin(); itemIt != it->end(); ++itemIt )
        {
            sbBuffer = itemIt->value;
            good::escape(sbBuffer);

            StringVector aParams;
            good::split((good::string)sbBuffer, aParams, ',', true);
            good::vector<CEntityClass> aAmmos[2];

            if ( itemIt->key == "default" ) // Default weapons.
            {
                for ( StringVector::iterator paramsIt = aParams.begin(); paramsIt != aParams.end(); ++paramsIt )
                {
                    StringVector aCurrent;
                    good::split(*paramsIt, aCurrent);
                    DebugAssert( aCurrent.size() > 0, exit(1) );

                    // Get weapon from name.
                    TWeaponId iWeaponId = CWeapons::GetIdFromWeaponName( aCurrent[0] );
                    if ( iWeaponId == -1 )
                    {
                        ConfigError("File \"%s\", section [%s], 'default' weapons: unknown weapon '%s', skipping.",
                                       m_iniFile.name.c_str(), it->name.c_str(), aCurrent[0].c_str());
                        continue;
                    }
                    const CWeapon* pWeapon = CWeapons::Get(iWeaponId);

                    size_t iNeedArgument = 1;

                    if ( pWeapon->iClipSize[0] > 0 )
                        iNeedArgument++;
                    if ( pWeapon->bHasSecondary && !pWeapon->bSecondaryUseSameBullets )
                        iNeedArgument++;

                    if ( aCurrent.size() != iNeedArgument )
                        ConfigError("File \"%s\", section [%s], 'default' weapons, weapon '%s': invalid parameters count.",
                                       m_iniFile.name.c_str(), it->name.c_str(), aCurrent[0].c_str());

                    // Get primary extra ammo for this weapon.
                    int iExtra[2] = { -1, -1 };
                    if ( (pWeapon->iClipSize[0] > 0) && (aCurrent.size() >= 2 )) // Weapon name, primary extra ammo.
                    {
                        sscanf( aCurrent[1].c_str(), "%d", &iExtra[0]);
                        if ( iExtra[0] < 0 )
                            ConfigError("File \"%s\", section [%s], 'default' weapons, weapon '%s': invalid parameter %s.",
                                           m_iniFile.name.c_str(), it->name.c_str(), aCurrent[0].c_str(), aCurrent[1].c_str());
                    }

                    // Get secondary extra ammo.
                    if ( pWeapon->bHasSecondary && !pWeapon->bSecondaryUseSameBullets && (aCurrent.size() >= 3) )
                    {
                        sscanf( aCurrent[2].c_str(), "%d", &iExtra[1]);
                        if ( iExtra[1] < 0 )
                            ConfigError("File \"%s\", section [%s], 'default' weapons, weapon '%s': invalid parameter %s.",
                                           m_iniFile.name.c_str(), it->name.c_str(), aCurrent[0].c_str(), aCurrent[3].c_str());
                    }

                    if ( pWeapon->iClipSize[0] && (iExtra[0] < 0) )
                    {
                        iExtra[0] = pWeapon->iDefaultAmmo[0];
                        ConfigError("File \"%s\", section [%s], 'default' weapons, weapon '%s': using default primary %u.",
                                       m_iniFile.name.c_str(), it->name.c_str(), aCurrent[0].c_str(), iExtra[0]);
                    }
                    if ( pWeapon->iClipSize[1] && (iExtra[1] < 0) )
                    {
                        iExtra[1] = pWeapon->iDefaultAmmo[1];
                        ConfigError("File \"%s\", section [%s], 'default' weapons, weapon '%s': using default secondary %u.",
                                       m_iniFile.name.c_str(), it->name.c_str(), aCurrent[0].c_str(), iExtra[1]);
                    }

                    CWeapons::SetDefault( iWeaponId, iExtra[0] >= 0 ? iExtra[0] : 0, iExtra[1] >= 0 ? iExtra[1] : 0);
                }
            }

            else // Weapon definition.
            {
                CWeapon* pWeapon = new CWeapon();

                bool bSecondary = false, bError = false;
                for ( StringVector::iterator paramsIt = aParams.begin(); paramsIt != aParams.end(); ++paramsIt )
                {
                    int iValue = -1;

                    StringVector aCurrent;
                    good::split(*paramsIt, aCurrent);
                    DebugAssert( aCurrent.size() > 0, exit(1) );

                    if ( aCurrent.size() == 1 )
                    {
                        if ( aCurrent[0] == "secondary" )
                            pWeapon->bHasSecondary = bSecondary = true;
                        else if ( aCurrent[0] == "same_ammo" )
                            pWeapon->bSecondaryUseSameBullets = true;
                        else if ( aCurrent[0] == "force_aim" )
                            pWeapon->bForceAim = true;
                        else if ( aCurrent[0] == "background_reload" )
                            pWeapon->bBackgroundReload = true;
                        else if ( aCurrent[0] == "add_clip" )
                            pWeapon->bAddClip = true;
                        else
                        {
                            ConfigError("File \"%s\", section [%s], weapon %s, unknown keyword: %s or invalid parameters count.",
                                m_iniFile.name.c_str(), it->name.c_str(), itemIt->key.c_str(), aCurrent[0].c_str());
                            bError = true;
                            break;
                        }
                    }
                    else if ( aCurrent.size() == 2 )
                    {
                        if ( aCurrent[0] == "type" )
                        {
                            int iType = CTypeToString::WeaponTypeFromString(aCurrent[1]);
                            if ( iType == -1 )
                            {
                                ConfigError("File \"%s\", section [%s], weapon %s, invalid weapon type: %s.",
                                               m_iniFile.name.c_str(), it->name.c_str(), itemIt->key.c_str(), aCurrent[1].c_str());
                                bError = true;
                                break;
                            }
                            pWeapon->iType = iType;

                            // Set weapon default parameters.
                            switch (iType)
                            {
                            case EWeaponManual:
                            case EWeaponPhysics:
                                pWeapon->iAttackBullets[0] = pWeapon->iAttackBullets[1] = 0;
                                break;
                            case EWeaponRpg:
                            case EWeaponGrenade:
                            case EWeaponFlash:
                            case EWeaponSmoke:
                            case EWeaponRemoteDetonation:
                                pWeapon->iClipSize[0] = 1;
                                break;
                            }
                        }
                        else if ( aCurrent[0] == "preference" )
                        {
                            iValue = CTypeToString::PreferenceFromString(aCurrent[1]);
                            if ( iValue == -1 )
                                ConfigError("File \"%s\", section [%s], weapon %s, invalid preference: %s, using 'lowest'.",
                                               m_iniFile.name.c_str(), it->name.c_str(), itemIt->key.c_str(), aCurrent[1].c_str());
                            else
                                pWeapon->iBotPreference = iValue;
                        }
                        else if ( aCurrent[0] == "team" )
                        {
                            iValue = CMod::GetTeamIndex(aCurrent[1]);
                            if ( iValue == -1 )
                            {
                                ConfigError("File \"%s\", section [%s], weapon %s, invalid team: %s.",
                                            m_iniFile.name.c_str(), it->name.c_str(),
                                            itemIt->key.c_str(), aCurrent[1].c_str());
                                bError = true;
                                break;
                            }
                            pWeapon->iTeamOnly = iValue;
                        }
                        else if ( aCurrent[0] == "range" )
                        {
                            // TODO:

                        }
                        else
                        {
                            sscanf(aCurrent[1].c_str(), "%d", &iValue);
                            if ( iValue < 0 )
                            {
                                ConfigError("File \"%s\", section [%s], weapon %s, invalid number: %s for parameter %s.",
                                               m_iniFile.name.c_str(), it->name.c_str(), itemIt->key.c_str(),
                                               aCurrent[1].c_str(), aCurrent[0].c_str());
                                bError = true;
                                break;
                            }

                            if ( aCurrent[0] == "clip" )
                                pWeapon->iClipSize[bSecondary] = iValue;

                            else if ( aCurrent[0] == "damage" )
                                pWeapon->iDamage[bSecondary] = iValue;

                            else if ( aCurrent[0] == "delay" )
                                pWeapon->fShotTime[bSecondary] = iValue / 1000.0f;

                            else if ( aCurrent[0] == "reload" )
                                pWeapon->fReloadTime[bSecondary] = iValue / 1000.0f;

                            else if ( aCurrent[0] == "holster" )
                                pWeapon->fHolsterTime = iValue / 1000.0f;

                            else if ( aCurrent[0] == "default_ammo" )
                                pWeapon->iDefaultAmmo[bSecondary] = iValue;

                            else if ( aCurrent[0] == "max_ammo" )
                                pWeapon->iMaxAmmo[bSecondary] = iValue;

                            else if ( aCurrent[0] == "bullets" )
                                pWeapon->iAttackBullets[bSecondary] = iValue;

                            else if ( aCurrent[0] == "default_ammo" )
                                pWeapon->iDefaultAmmo[bSecondary] = iValue;

                            else if ( aCurrent[0] == "parabolic" )
                                pWeapon->iParabolic[bSecondary] = iValue;

                            else if ( aCurrent[0] == "zoom_distance" )
                                pWeapon->fMinDistanceSqr[1] = SQR(iValue);

                            else if ( aCurrent[0] == "zoom_time" )
                                pWeapon->fShotTime[1] = iValue / 1000.0f;

                            else
                            {
                                ConfigError("File \"%s\", section [%s], weapon %s, unknown keyword: %s or invalid parameters count.",
                                               m_iniFile.name.c_str(), it->name.c_str(), itemIt->key.c_str(), aCurrent[0].c_str());
                                bError = true;
                                break;
                            }
                        }
                    }
                    else if ( aCurrent[0] == "ammo" ) // 3+ arguments.
                    {
                        if ( (aCurrent.size() & 1) == 0 ) // Number must be odd.
                        {
                            ConfigError("File \"%s\", section [%s], weapon %s: invalid parameters count for %s.",
                                           m_iniFile.name.c_str(), it->name.c_str(), itemIt->key.c_str(), aCurrent[0].c_str());
                            bError = true;
                            break;
                        }
                        aAmmos[bSecondary].reserve( (aCurrent.size() - 1) >> 1 ); // Reserve some space for ammos.
                        for ( size_t i=1; i < aCurrent.size(); i+=2 )
                        {
                            int iValue = -1;
                            sscanf(aCurrent[i+1].c_str(), "%d", &iValue);
                            if ( iValue <= 0 ) // Ammo count can't be 0.
                            {
                                ConfigError("File \"%s\", section [%s], weapon %s, invalid parameter for '%s' ammo's count: %s.",
                                               m_iniFile.name.c_str(), it->name.c_str(), itemIt->key.c_str(), aCurrent[i].c_str(), aCurrent[i+1].c_str());
                                bError = true;
                                break;
                            }
                            CEntityClass cAmmoClass;
                            cAmmoClass.sClassName.assign(aCurrent[i], true);
                            cAmmoClass.SetArgument(iValue);
                            aAmmos[bSecondary].push_back(cAmmoClass);
                        }
                        if ( bError )
                            break;
                    }
                    else
                    {
                        ConfigError("File \"%s\", section [%s], weapon %s: unknown parameter %s.",
                                       m_iniFile.name.c_str(), it->name.c_str(), itemIt->key.c_str(), aCurrent[0].c_str());
                        bError = true;
                        break;
                    }
                }

                if ( !bError )
                {
                    ConfigMessage( "  %s", itemIt->key.c_str() );

                    pWeapon->iId = CWeapons::Size();

                    // Add weapon class.
                    CEntityClass cEntityClass;
                    cEntityClass.sClassName.assign(itemIt->key, true);
                    pWeapon->pWeaponClass = CItems::AddItemClassFor( EEntityTypeWeapon, cEntityClass );

                    // Add ammo classes.
                    pWeapon->aAmmos[0].reserve(aAmmos[0].size());
                    pWeapon->aAmmos[1].reserve(aAmmos[1].size());
                    for ( int bSec=0; bSec < 2; ++bSec )
                        for ( size_t i=0; i < aAmmos[bSec].size(); ++i )
                        {
                            const CEntityClass* pAmmoClass = CItems::AddItemClassFor( EEntityTypeAmmo, aAmmos[bSec][i] );
                            pWeapon->aAmmos[bSec].push_back( pAmmoClass );
                            ConfigMessage( "    ammo %s (%u bullets)", pWeapon->aAmmos[bSec][i]->sClassName.c_str(),
                                           pWeapon->aAmmos[bSec][i]->GetArgument(), itemIt->key.c_str() );
                        }

                    CWeaponWithAmmo cWeapon(pWeapon);
                    CWeapons::Add(cWeapon);
                }
                else
                    delete pWeapon;
            }
        }
    }

#ifdef BOTRIX_CHAT
    // Load chat: synonims.
    ConfigMessage("Loading chat synonims:");
    it = m_iniFile.find( "Chat.replace" );
    if ( it != m_iniFile.end() )
    {
        // Iterate throught key values.
        for ( good::ini_section::const_iterator wordIt = it->begin(); wordIt != it->end(); ++wordIt )
            CChat::AddSynonims( wordIt->key, wordIt->value );
    }

    // Load chat: commands.
    ConfigMessage("Loading chat sentences:");
    it = m_iniFile.find( "Chat.sentences" );
    if ( it != m_iniFile.end() )
    {
        // Iterate throught key values.
        for ( good::ini_section::const_iterator wordIt = it->begin(); wordIt != it->end(); ++wordIt )
            CChat::AddChat( wordIt->key, wordIt->value );
    }
#endif // BOTRIX_CHAT

    return iModId;
}


//------------------------------------------------------------------------------------------------------------
const good::string* CConfiguration::GetGeneralSectionValue( const good::string& sKey )
{
    good::ini_section& sect = m_iniFile["General"];
    good::ini_section::iterator kv = sect.find(sKey);
    return (kv == sect.end()) ? NULL : &kv->value;
}

void CConfiguration::SetGeneralSectionValue( const good::string& sKey, const good::string& sValue )
{
    good::ini_section& sect = m_iniFile["General"];
    sect[sKey] = sValue;
    m_bModified = true;
}

//------------------------------------------------------------------------------------------------------------
TCommandAccessFlags CConfiguration::ClientAccessLevel( const good::string& sSteamId )
{
    good::ini_section& sect = m_iniFile["User access"];
    good::ini_section::iterator kv = sect.find(sSteamId);
    if ( kv == sect.end() )
        return 0;

    return CTypeToString::AccessFlagsFromString(kv->value);
}

void CConfiguration::SetClientAccessLevel( const good::string& sSteamId, TCommandAccessFlags iAccess )
{
    good::ini_section& sect = m_iniFile["User access"];
    char buffer[10];
    sprintf( buffer, "%d", iAccess );
    sect[sSteamId] = good::string(buffer, true, true);
    m_bModified = true;
}
