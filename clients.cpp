#include "clients.h"
#include "config.h"
#include "item.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//----------------------------------------------------------------------------------------------------------------
void CClient::Activated()
{
	CPlayer::Activated();
	
	DebugAssert(m_pPlayerInfo);
	m_sSteamId = m_pPlayerInfo->GetNetworkIDString();

	if ( m_sSteamId.size() )
	{
		// TODO: check if works on dedicated server.
		TCommandAccessFlags iAccess = CConfiguration::ClientAccessLevel(m_sSteamId);
		if (iAccess) // Founded.
			iCommandAccessFlags = iAccess;
	}
	else 
		iCommandAccessFlags = 0;

	iWaypointDrawFlags = FWaypointDrawNone;
	iPathDrawFlags = FPathDrawNone;

	bAutoCreatePaths = FLAG_ALL_SET(FCommandAccessWaypoint, iCommandAccessFlags);
	bAutoCreateWaypoints = false;
	
	iItemDrawFlags = EItemDrawAll;
	iItemTypeFlags = 0;

	iDestinationWaypoint = EInvalidWaypointId;

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
		DebugAssert(CWaypoint::IsValid(iCurrentWaypoint));

		// Add paths from previous to current.
		if ( CWaypoint::IsValid(iLastWaypoint) )
		{
			float zDist = GetPlayerInfo()->GetPlayerMaxs().z - GetPlayerInfo()->GetPlayerMins().z;
			bool bIsCrouched = (zDist < CUtil::iPlayerHeight);

			CWaypoints::CreatePathsWithAutoFlags(iLastWaypoint, iCurrentWaypoint, bIsCrouched);
			iDestinationWaypoint = iLastWaypoint;
		}
	}

	// Draw waypoints.
	CWaypoints::Draw(this);

	// Draw entities.
	CItems::Draw(this);
}
