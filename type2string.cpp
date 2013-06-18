#include "server_plugin.h"
#include "type2string.h"

#include "good/string_buffer.h"

good::string sUnknown("unknown");
good::string sEmpty("");

extern char* szMainBuffer;
extern int iMainBufferSize;


//================================================================================================================
const good::string& EnumToString( int iEnum, int iEnumCount, const good::string aStrings[], const good::string& sDefault )
{
	return (0 <= iEnum && iEnum < iEnumCount) ? aStrings[iEnum] : sDefault;
}

int EnumFromString( const good::string& s, int iEnumCount, const good::string aStrings[] )
{
	for ( int i=0; i < iEnumCount; ++i )
		if ( s == aStrings[i] )
			return i;
	return -1;
}


//================================================================================================================
const good::string& FlagsToString( int iFlags, int iFlagsCount, const good::string aStrings[] )
{
	if ( iFlags == 0 )
		return sEmpty;

	static good::string_buffer sbBuffer(szMainBuffer, iMainBufferSize, false);
	sbBuffer.erase();

	for ( int i=0; i < iFlagsCount; ++i )
		if ( FLAG_ALL_SET(1<<i, iFlags) )
		{
			sbBuffer.append( aStrings[i] );
			sbBuffer.append(' ');
		}

	sbBuffer.erase(sbBuffer.length()-1, 1); // Erase last space.
	return sbBuffer;
}

int FlagsFromString( const good::string& s, int iFlagsCount, const good::string aStrings[] )
{
	int iResult = 0;
	StringVector aFlags = s.split<good::vector>(' ', true);

	for ( int i=0; i < (int)aFlags.size(); ++i )
	{
		int j;
		for ( j=0; j < iFlagsCount; ++j )
		{
			if ( aFlags[i] == aStrings[j] )
			{
				FLAG_SET( 1<<j, iResult );
				break;
			}
		}
		if ( j == iFlagsCount ) // Couldn't find flag in array of strings, return an error.
			return -1;
	}
	
	return iResult;
}





//****************************************************************************************************************
const good::string& CTypeToString::StringVectorToString( const StringVector& aStrings )
{
	static good::string_buffer sbBuffer(szMainBuffer, iMainBufferSize, false);
	sbBuffer.erase();

	for ( StringVector::const_iterator it = aStrings.begin(); it != aStrings.end(); ++it )
	{
		sbBuffer.append(*it);
		sbBuffer.append(' ');
	}

	return sbBuffer;
}

	
//----------------------------------------------------------------------------------------------------------------
good::string aBools[2] =
{
	"off",
	"on"
};

int CTypeToString::BoolFromString( const good::string& sBool )
{
	return EnumFromString( sBool, 2, aBools );
}

const good::string& CTypeToString::BoolToString( bool b )
{
	return aBools[b];
}


//----------------------------------------------------------------------------------------------------------------
// Ordered by TModId.
//----------------------------------------------------------------------------------------------------------------
good::string aMods[EModId_Total] =
{
	"hl2dm",
	"css",
	"borzh"
};

int CTypeToString::ModFromString( const good::string& sMod )
{
	return EnumFromString( sMod, EModId_Total, aMods );
}

const good::string& CTypeToString::ModToString( TModId iMod )
{
	return EnumToString( iMod, EModId_Total, aMods, sUnknown );
}


//----------------------------------------------------------------------------------------------------------------
// Ordered by TCommandAccessFlags.
//----------------------------------------------------------------------------------------------------------------
good::string aAccessFlags[FCommandAccessTotal] =
{
	"waypoint",
	"bot",
	"config",
};

int CTypeToString::AccessFlagsFromString( const good::string& sFlags )
{
	return FlagsFromString( sFlags, FCommandAccessTotal, aAccessFlags );
}

const good::string& CTypeToString::AccessFlagsToString( TCommandAccessFlags iFlags )
{
	return FlagsToString( iFlags, FCommandAccessTotal, aAccessFlags );
}


//----------------------------------------------------------------------------------------------------------------
// Ordered by TWaypointFlag.
//----------------------------------------------------------------------------------------------------------------
good::string aWaypointFlags[FWaypointTotal] =
{
	"stop",
	"camper",
	"sniper",
	"weapon",
	"ammo",
	"health",
	"armor",
	"health_machine",
	"armor_machine",
	"button",
};

int CTypeToString::WaypointFlagsFromString( const good::string& sFlags )
{
	return FlagsFromString( sFlags, FWaypointTotal, aWaypointFlags ); 
}

const good::string& CTypeToString::WaypointFlagsToString(TWaypointFlags iFlags)
{
	return FlagsToString( iFlags, FWaypointTotal, aWaypointFlags );
}


//----------------------------------------------------------------------------------------------------------------
// Ordered by TPathFlag.
//----------------------------------------------------------------------------------------------------------------
good::string aPathFlags[FPathTotal] =
{
	"crouch",
	"jump",
	"break",
	"sprint",
	"ladder",
	"stop",
	"damage",
	"flashlight",
	"totem",
};


int CTypeToString::PathFlagsFromString( const good::string& sFlags )
{
	return FlagsFromString( sFlags, FPathTotal, aPathFlags );
}

const good::string& CTypeToString::PathFlagsToString( TPathFlags iFlags )
{
	return FlagsToString( iFlags, FPathTotal, aPathFlags );
}


//----------------------------------------------------------------------------------------------------------------
// Ordered by TWaypointDrawFlags.
//----------------------------------------------------------------------------------------------------------------
good::string aDrawTypeFlags[FWaypointDrawTotal] =
{
	"line",
	"beam",
	"box",
	"text",
};

int CTypeToString::WaypointDrawFlagsFromString( const good::string& sFlags )
{
	return FlagsFromString( sFlags, FWaypointDrawTotal, aDrawTypeFlags ); 
}

const good::string& CTypeToString::WaypointDrawFlagsToString( TWaypointDrawFlags iFlags )
{
	return FlagsToString( iFlags, FWaypointDrawTotal, aDrawTypeFlags );
}


int CTypeToString::PathDrawFlagsFromString( const good::string& sFlags )
{
	return FlagsFromString( sFlags, FPathDrawTotal, aDrawTypeFlags ); 
}

const good::string& CTypeToString::PathDrawFlagsToString( TPathDrawFlags iFlags )
{
	return FlagsToString( iFlags, FPathDrawTotal, aDrawTypeFlags );
}

//----------------------------------------------------------------------------------------------------------------
// Ordered by TEntityType.
//----------------------------------------------------------------------------------------------------------------
good::string aItemTypes[EEntityTypeTotal+1] =
{
	"health",
	"armor",
	"weapon",
	"ammo",
	"button",
	"door",
	"object",
	"other",
};


int CTypeToString::EntityTypeFromString( const good::string& sType )
{
	return EnumFromString( sType, EEntityTypeTotal+1, aItemTypes ); 
}

const good::string& CTypeToString::EntityTypeToString( TEntityType iType )
{
	return EnumToString( iType, EEntityTypeTotal+1, aItemTypes, sUnknown );
}

int CTypeToString::EntityTypeFlagsFromString( const good::string& sFlags )
{
	return FlagsFromString( sFlags, EEntityTypeTotal+1, aItemTypes ); 
}

const good::string& CTypeToString::EntityTypeFlagsToString( TEntityTypeFlags iItemTypeFlags )
{
	return FlagsToString( iItemTypeFlags, EEntityTypeTotal+1, aItemTypes );
}


//----------------------------------------------------------------------------------------------------------------
// Ordered by TEntityFlags.
//----------------------------------------------------------------------------------------------------------------
good::string aEntityClassFlags[FEntityTotal] =
{
	"use",
	"respawnable",
	"explosive",
	"heavy",
	"box",
};
const good::string sNone("none");

int CTypeToString::EntityClassFlagsFromString( const good::string& sFlags )
{
	if ( sFlags == sNone )
		return 0;
	else
		return FlagsFromString( sFlags, FEntityTotal, aEntityClassFlags ); 
}

const good::string& CTypeToString::EntityClassFlagsToString( TEntityFlags iItemFlags )
{
	if ( iItemFlags == 0 )
		return sNone;
	else
		return FlagsToString( iItemFlags, FEntityTotal, aEntityClassFlags );
}


//----------------------------------------------------------------------------------------------------------------
// Ordered by TItemDrawFlags.
//----------------------------------------------------------------------------------------------------------------
good::string aItemDrawFlags[EItemDrawTotal] =
{
	"name",
	"box",
	"waypoint",
};

int CTypeToString::ItemDrawFlagsFromString( const good::string& sFlags )
{
	return FlagsFromString( sFlags, EItemDrawTotal, aItemDrawFlags ); 
}

const good::string& CTypeToString::ItemDrawFlagsToString( TItemDrawFlags iFlags )
{
	return FlagsToString( iFlags, EItemDrawTotal, aItemDrawFlags );
}


//----------------------------------------------------------------------------------------------------------------
// Ordered by TWeaponType.
//----------------------------------------------------------------------------------------------------------------
good::string aWeaponTypes[EWeaponTotal] =
{
	"physics",
	"manual",
	"grenade",
	"flash",
	"smoke",
	"remote",
	"pistol",
	"shotgun",
	"rifle",
	"sniper",
	"rocket",
};

int CTypeToString::WeaponTypeFromString( const good::string& sType )
{
	return EnumFromString( sType, EWeaponTotal, aWeaponTypes );
}

const good::string& CTypeToString::WeaponTypeToString( TWeaponType iType )
{
	return EnumToString( iType, EWeaponTotal, aWeaponTypes, sUnknown );
}


//----------------------------------------------------------------------------------------------------------------
// Ordered by TPreference.
//----------------------------------------------------------------------------------------------------------------
good::string aPreferences[EBotIntelligenceTotal] =
{
	"lowest",
	"lowest",
	"normal",
	"high",
	"highest",
};

int CTypeToString::PreferenceFromString( const good::string& sPreference )
{
	return EnumFromString( sPreference, EBotIntelligenceTotal, aPreferences );
}

const good::string& CTypeToString::PreferenceToString( TBotIntelligence iPreference )
{
	return EnumToString( iPreference, EBotIntelligenceTotal, aPreferences, sUnknown );
}

//----------------------------------------------------------------------------------------------------------------
// Ordered by TPreference.
//----------------------------------------------------------------------------------------------------------------
good::string aIntelligences[EBotIntelligenceTotal] =
{
	"fool",
	"stupied",
	"normal",
	"smart",
	"pro",
};

const good::string& CTypeToString::IntelligenceToString( TBotIntelligence iIntelligence )
{
	return EnumToString( iIntelligence, EBotIntelligenceTotal, aIntelligences, sUnknown );
}


//----------------------------------------------------------------------------------------------------------------
// Ordered by TBotTasks.
//----------------------------------------------------------------------------------------------------------------
good::string aBotTasks[EBotTasksTotal] =
{
	"find health",
	"find armor",
	"find weapon",
	"find ammo",
	"chase enemy",
	"find enemy",
};

const good::string& CTypeToString::BotTaskToString( TBotTaskHL2DM iBotTask )
{
	return EnumToString( iBotTask, EBotTasksTotal, aBotTasks, sUnknown );
}


//----------------------------------------------------------------------------------------------------------------
// Ordered by TBotChat.
//----------------------------------------------------------------------------------------------------------------
good::string aBotCommands[EBotChatTotal] =
{
	"error",
	"greeting",
	"bye",
	"busy",
	"affirmative",
	"negative",
	"affirm",
	"negate",
	"call",
	"call response",
	"help",
	"stop",
	"come",
	"follow",
	"attack",
	"no kill",
	"sit down",
	"stand up",
	"jump",
	"leave",
};

int CTypeToString::BotCommandFromString( const good::string& sCommand )
{
	return EnumFromString( sCommand, EBotChatTotal, aBotCommands );
}


const good::string& CTypeToString::BotCommandToString( TBotChat iCommand )
{
	return EnumToString( iCommand, EBotChatTotal, aBotCommands, sUnknown );
}
