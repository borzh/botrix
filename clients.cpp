#include "clients.h"
#include "config.h"
#include "item.h"
#include "type2string.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//----------------------------------------------------------------------------------------------------------------
void CClient::Activated()
{
    CPlayer::Activated();

    BASSERT(m_pPlayerInfo, exit(1));
    m_sSteamId = m_pPlayerInfo->GetNetworkIDString();

    if ( m_sSteamId.size() )
    {
        TCommandAccessFlags iAccess = CConfiguration::ClientAccessLevel(m_sSteamId);
        if ( iAccess ) // Founded.
            iCommandAccessFlags = iAccess;
    }
    else
        iCommandAccessFlags = 0;

    BLOG_I( "User connected %s (steam id %s), access: %s.", GetName(), m_sSteamId.c_str(),
            CTypeToString::AccessFlagsToString(iCommandAccessFlags).c_str() );

    iWaypointDrawFlags = FWaypointDrawNone;
    iPathDrawFlags = FPathDrawNone;

    bAutoCreatePaths = FLAG_ALL_SET(FCommandAccessWaypoint, iCommandAccessFlags);
    bAutoCreateWaypoints = false;

    iItemDrawFlags = EItemDrawAll;
    iItemTypeFlags = 0;

    iDestinationWaypoint = EWaypointIdInvalid;

#if defined(DEBUG) || defined(_DEBUG)
    bDebuggingEvents = FLAG_ALL_SET(FCommandAccessConfig, iCommandAccessFlags);
#else
    bDebuggingEvents = false;
#endif
}

//----------------------------------------------------------------------------------------------------------------
void CClient::PreThink()
{
    int iLastWaypoint = iCurrentWaypoint;
    CPlayer::PreThink();

    // Check if lost waypoint, in that case add new one.
    if ( bAutoCreateWaypoints && m_bAlive && !CWaypoint::IsValid(iCurrentWaypoint) )
    {
        Vector vOrigin( GetHead() );

        // Add new waypoint, but distance from previous one must not be bigger than MAX_RANGE.
        if ( CWaypoint::IsValid(iLastWaypoint) )
        {
            CWaypoint& wLast = CWaypoints::Get(iLastWaypoint);
            vOrigin -= wLast.vOrigin;
            vOrigin.NormalizeInPlace();
            vOrigin *= CWaypoint::MAX_RANGE;
            vOrigin += wLast.vOrigin;
        }

        // Add new waypoint.
        iCurrentWaypoint = CWaypoints::Add(vOrigin);

        // Add paths from previous to current.
        if ( CWaypoint::IsValid(iLastWaypoint) )
        {
            float fHeight = GetPlayerInfo()->GetPlayerMaxs().z - GetPlayerInfo()->GetPlayerMins().z + 1;
            bool bIsCrouched = (fHeight < CMod::iPlayerHeight);

            CWaypoints::CreatePathsWithAutoFlags(iLastWaypoint, iCurrentWaypoint, bIsCrouched);
            iDestinationWaypoint = iLastWaypoint;
        }
    }

    // Draw waypoints.
    CWaypoints::Draw(this);

    // Draw entities.
    CItems::Draw(this);
}
