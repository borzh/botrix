#include "clients.h"
#include "item.h"
#include "server_plugin.h"
#include "type2string.h"
#include "waypoint.h"

#include "good/string_buffer.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//----------------------------------------------------------------------------------------------------------------
extern char* szMainBuffer;
extern int iMainBufferSize;


//----------------------------------------------------------------------------------------------------------------
// Waypoints file header.
//----------------------------------------------------------------------------------------------------------------
#pragma pack(push)
#pragma pack(1)
struct waypoint_header
{
	int          szFileType;
	char         szMapName[64];
	int          iVersion;
	int          iNumWaypoints;
	int          iFlags;
};
#pragma pack(pop)


static const char WAYPOINT_FILE_HEADER_ID[4] = {'B','t','x','W'}; // Botrix's Waypoints.
static const int WAYPOINT_VERSION = 1;                            // Waypoints file version.
static const int WAYPOINT_FILE_FLAG_VISIBILITY = 1<<0;            // Flag for waypoint visibility table.
static const int WAYPOINT_FILE_FLAG_AREAS = 2<<0;                 // Flag for area names.


//----------------------------------------------------------------------------------------------------------------
// CWaypoint static members.
//----------------------------------------------------------------------------------------------------------------
int CWaypoint::iWaypointTexture = -1;
const TWaypointFlags CWaypoint::m_aFlagsForEntityType[EEntityTypeTotal-1] =
{
	FWaypointHealth | FWaypointHealthMachine,
	FWaypointArmor | FWaypointArmorMachine,
	FWaypointWeapon,
	FWaypointAmmo
};
	

// CWaypoints static members.
StringVector CWaypoints::m_cAreas;
CWaypoints::WaypointGraph CWaypoints::m_cGraph;
float CWaypoints::m_fNextDrawWaypointsTime = 0.0f;
CWaypoints::Bucket CWaypoints::m_cBuckets[CWaypoints::BUCKETS_SIZE_X][CWaypoints::BUCKETS_SIZE_Y][CWaypoints::BUCKETS_SIZE_Z];


//----------------------------------------------------------------------------------------------------------------
void CWaypoint::GetColor(unsigned char& r, unsigned char& g, unsigned char& b) const
{
	if ( FLAG_SOME_SET(FWaypointStop, iFlags) )
	{
		r = 0x00; g = 0x00; b = 0xFF;  // Blue effect, stop.
	}
	else if ( FLAG_SOME_SET(FWaypointCamper, iFlags) )
	{
		r = 0x33; g = 0x00; b = 0x00;  // Red effect, camper.
	}
	else if ( FLAG_SOME_SET(FWaypointSniper, iFlags) )
	{
		r = 0xFF; g = 0x00; b = 0x00;  // Red effect, sniper.
	}
	else if ( FLAG_SOME_SET(FWaypointWeapon, iFlags) )
	{
		r = 0xFF; g = 0xFF; b = 0x00;  // Light yellow effect, weapon.
	}
	else if ( FLAG_SOME_SET(FWaypointAmmo, iFlags) )
	{
		r = 0x33; g = 0x33; b = 0x00;  // Dark yellow effect, ammo.
	}
	else if ( FLAG_SOME_SET(FWaypointHealth, iFlags) )
	{
		r = 0xFF; g = 0xFF; b = 0xFF;  // Light white effect, health.
	}
	else if ( FLAG_SOME_SET(FWaypointHealthMachine, iFlags) )
	{
		r = 0x66; g = 0x66; b = 0x66;  // Gray effect, health machine.
	}
	else if ( FLAG_SOME_SET(FWaypointArmor, iFlags) )
	{
		r = 0x00; g = 0xFF; b = 0x00;  // Light green effect, armor.
	}
	else if ( FLAG_SOME_SET(FWaypointArmorMachine, iFlags) )
	{
		r = 0x00; g = 0x33; b = 0x00;  // Dark green effect, armor machine.
	}
	else if ( FLAG_SOME_SET(FWaypointButton | FWaypointSeeButton, iFlags) )
	{
		r = 0x8A; g = 0x2B; b = 0xE2;  // Violet effect, button.
	}
	else
	{
		r = 0x00; g = 0xFF; b = 0xFF;  // Cyan effect, other flags.
	}
}


//----------------------------------------------------------------------------------------------------------------
void CWaypoint::Draw( TWaypointId iWaypointId, TWaypointDrawFlags iDrawType, float fDrawTime ) const
{
	unsigned char r, g, b; // Red, green, blue.
	GetColor(r, g, b);

	Vector vEnd = Vector(vOrigin.x, vOrigin.y, vOrigin.z - CUtil::iPlayerEyeLevel);

	if ( FLAG_ALL_SET(FWaypointDrawBeam, iDrawType) )
		CUtil::DrawBeam(vOrigin, vEnd, WIDTH, fDrawTime, r, g, b);
	
	if ( FLAG_ALL_SET(FWaypointDrawLine, iDrawType) )
		CUtil::DrawLine(vOrigin, vEnd, fDrawTime, r, g, b);

	if ( FLAG_ALL_SET(FWaypointDrawBox, iDrawType) )
	{
		Vector vBoxOrigin(vOrigin.x - CUtil::iPlayerWidth/2, vOrigin.y - CUtil::iPlayerWidth/2, vOrigin.z - CUtil::iPlayerEyeLevel);
		CUtil::DrawBox(vBoxOrigin, CUtil::vZero, CUtil::vPlayerCollisionHull, fDrawTime, r, g, b);
	}

	if ( FLAG_ALL_SET(FWaypointDrawText, iDrawType) )
	{
		static char szId[16];
		sprintf(szId, "%d", iWaypointId);
		Vector v = vOrigin;
		v.z -= 20.0f;
		int i = 0;
		CUtil::DrawText(v, i++, fDrawTime, 0xFF, 0xFF, 0xFF, szId);
		CUtil::DrawText(v, i++, fDrawTime, 0xFF, 0xFF, 0xFF, CWaypoints::GetAreas()[iAreaId].c_str());
		CUtil::DrawText(v, i++, fDrawTime, 0xFF, 0xFF, 0xFF, CTypeToString::WaypointFlagsToString(iFlags).c_str());
	}
}


//********************************************************************************************************************
bool CWaypoints::Save()
{
	const good::string& sFileName = CUtil::BuildFileName("waypoints", CBotrixPlugin::instance->sMapName, "way");

	FILE *f = CUtil::OpenFile(sFileName.c_str(), "wb");
	if (f == NULL)
		return false;

	waypoint_header header;
	header.iFlags = 0;
	if ( m_cAreas.size() > 1 )
		FLAG_SET(WAYPOINT_FILE_FLAG_AREAS, header.iFlags);
	//if ( bVisiblityMade )
	//	FLAG_SET(WAYPOINT_FILE_FLAG_VISIBILITY, header.iFlags);
	header.iNumWaypoints = m_cGraph.size();
	header.iVersion = WAYPOINT_VERSION;
	header.szFileType = *((int*)&WAYPOINT_FILE_HEADER_ID[0]);
	strncpy(header.szMapName, CBotrixPlugin::instance->sMapName.c_str(), sizeof(header.szMapName));

	// Write header.
	fwrite(&header, sizeof(waypoint_header), 1, f);

	// Write waypoints data.
	for (WaypointNodeIt it = m_cGraph.begin(); it != m_cGraph.end(); ++it)
	{
		fwrite(&it->vertex.vOrigin, sizeof(Vector), 1, f);
		fwrite(&it->vertex.iFlags, sizeof(TWaypointFlags), 1, f);
		fwrite(&it->vertex.iAreaId, sizeof(TAreaId), 1, f);
		fwrite(&it->vertex.iArgument, sizeof(int), 1, f);
	}

	// Write waypoints neighbours.
	for (WaypointNodeIt it = m_cGraph.begin(); it != m_cGraph.end(); ++it)
	{
		int iNumPaths = it->neighbours.size();
		fwrite(&iNumPaths, sizeof(int), 1, f);

		for ( WaypointArcIt arcIt = it->neighbours.begin(); arcIt != it->neighbours.end(); ++arcIt)
		{			
			fwrite(&arcIt->target, sizeof(TWaypointId), 1, f);             // Save waypoint id.
			fwrite(&arcIt->edge.iFlags, sizeof(TPathFlags), 1, f); // Save path flags.
			fwrite(&arcIt->edge.iArgument, sizeof(unsigned short), 1, f);  // Save path arguments.
		}
	}

	DebugAssert( m_cAreas.size() > 0 );
	int iAreaNamesSize = m_cAreas.size() - 1;
	fwrite(&iAreaNamesSize, sizeof(int), 1, f); // Save area names size.

	for ( int i=1; i < iAreaNamesSize+1; i++ ) // First area name is always empty, for new waypoints.
	{
		int iSize = m_cAreas[i].size();
		fwrite(&iSize, sizeof(int), 1, f); // Save string size.
		fwrite(m_cAreas[i].c_str(), sizeof(char), iSize+1, f); // Write string & trailing 0.
	}

	fclose(f);

	return true;
}


//----------------------------------------------------------------------------------------------------------------
bool CWaypoints::Load()
{
	Clear();

	const good::string& sFileName = CUtil::BuildFileName("waypoints", CBotrixPlugin::instance->sMapName, "way");

	FILE *f = CUtil::OpenFile(sFileName, "rb");
	if ( f == NULL )
		return false;

	struct waypoint_header header;
	fread(&header, sizeof(struct waypoint_header), 1, f);

	if (*((int*)&WAYPOINT_FILE_HEADER_ID[0]) != header.szFileType)
	{
		CUtil::Message(NULL, "Error loading waypoints: invalid file.");
		fclose(f);
		return false;
	}
	if (header.iVersion > WAYPOINT_VERSION)
	{
		CUtil::Message(NULL, "Error loading waypoints: version mismatch.");
		fclose(f);
		return false;
	}
	if (CBotrixPlugin::instance->sMapName != header.szMapName)
	{
		CUtil::Message(NULL, "Error loading waypoints: map name mismatch.s");
		fclose(f);
		return false;
	}

	int iSize = header.iNumWaypoints;

	Vector vOrigin;
	TWaypointFlags iFlags;
	int iNumPaths, iArgument = 0;
	TAreaId iAreaId = 0;

	// Read waypoints information.
	for (TWaypointId i = 0; i < iSize; ++i)
	{
		fread(&vOrigin, sizeof(Vector), 1, f);
		fread(&iFlags, sizeof(TWaypointFlags), 1, f);
		fread(&iAreaId, sizeof(TAreaId), 1, f);
		fread(&iArgument, sizeof(int), 1, f);

		Add(vOrigin, iFlags, iArgument, iAreaId);
	}

	// Read waypoints paths.
	TWaypointId iPathTo;
	TPathFlags iPathFlags;
	unsigned short iPathArgument;

	for (TWaypointId i = 0; i < iSize; ++i)
	{
		WaypointGraph::node_it from = m_cGraph.begin() + i;

		fread(&iNumPaths, sizeof(int), 1, f);
		m_cGraph[i].neighbours.reserve(iNumPaths);

		for ( int n = 0; n < iNumPaths; n ++ )
		{		
			fread(&iPathTo, sizeof(int), 1, f);
			fread(&iPathFlags, sizeof(TPathFlags), 1, f);
			fread(&iPathArgument, sizeof(unsigned short), 1, f);

			WaypointGraph::node_it to = m_cGraph.begin() + iPathTo;
			m_cGraph.add_arc( from, to, CWaypointPath(from->vertex.vOrigin.DistTo(to->vertex.vOrigin), iPathFlags, iPathArgument) );
		}
	}

	if (header.iFlags & WAYPOINT_FILE_FLAG_AREAS)
	{
		// Read area names.
		int iAreaNamesSize = 0;
		fread(&iAreaNamesSize, sizeof(int), 1, f);
		DebugAssert(iAreaNamesSize >= 0);

		m_cAreas.reserve(iAreaNamesSize + 1);
		m_cAreas.push_back("default"); // New waypoints without area id will be put under this empty area id.

		for ( int i=0; i < iAreaNamesSize; i++ )
		{
			int iSize;
			fread(&iSize, sizeof(int), 1, f);

			DebugAssert(0 < iSize && iSize < iMainBufferSize);
			if (iSize > 0)
			{
				fread(szMainBuffer, sizeof(char), iSize+1, f); // Read also trailing 0.
				good::string sArea(szMainBuffer, true, true, iSize);
				m_cAreas.push_back(sArea);
			}
		}
	}
/*
	bool bHasVisibility = header.iFlags & WAYPOINT_FILE_FLAG_VISIBILITY;
	if (bHasVisibility)
	{
		bHasVisibility = m_pVisibilityTable->ReadFromFile();
	}
	else
		CUtil::Message("No waypoint visibility in file");
*/
	fclose(f);

	return true;
}


//----------------------------------------------------------------------------------------------------------------
CWaypointPath* CWaypoints::GetPath(TWaypointId iFrom, TWaypointId iTo)
{
	WaypointGraph::node_t& from = m_cGraph[iFrom];
	for (WaypointGraph::arc_it it = from.neighbours.begin(); it != from.neighbours.end(); ++it)
		if (it->target == iTo)
			return &(it->edge);
	return NULL;
}


//----------------------------------------------------------------------------------------------------------------
TWaypointId CWaypoints::Add( Vector const& vOrigin, TWaypointFlags iFlags, int iArgument, int iAreaId )
{
	CWaypoint w(vOrigin, iFlags, iArgument, iAreaId);

	// lol, this is not working because m_cGraph.begin() is called first. wtf?
	// TWaypointId id = m_cGraph.add_node(w) - m_cGraph.begin();
	CWaypoints::WaypointNodeIt it = m_cGraph.add_node(w);
	TWaypointId id = it - m_cGraph.begin();

	AddLocation(id, vOrigin);
	//AddVisibility(id, vOrigin);
	return id;
}


//----------------------------------------------------------------------------------------------------------------
void CWaypoints::Remove( TWaypointId id )
{
	CItems::WaypointDeleted(id);
	RemoveLocation(id);
	//RemoveVisibility(id);
	m_cGraph.delete_node( m_cGraph.begin() + id );
}


//----------------------------------------------------------------------------------------------------------------
bool CWaypoints::AddPath( TWaypointId iFrom, TWaypointId iTo, float fDistance, TPathFlags iFlags )
{
	if ( !CWaypoint::IsValid(iFrom) || !CWaypoint::IsValid(iTo) || (iFrom == iTo) || HasPath(iFrom, iTo) )
		return false;

	WaypointGraph::node_it from = m_cGraph.begin() + iFrom;
	WaypointGraph::node_it to = m_cGraph.begin() + iTo;

	if (fDistance == 0.0f)
		fDistance = from->vertex.vOrigin.DistTo(to->vertex.vOrigin);

	m_cGraph.add_arc( from, to, CWaypointPath(fDistance, iFlags) );
	return true;
}


//----------------------------------------------------------------------------------------------------------------
bool CWaypoints::RemovePath( TWaypointId iFrom, TWaypointId iTo )
{
	if ( !CWaypoint::IsValid(iFrom) || !CWaypoint::IsValid(iTo) || (iFrom == iTo) || !HasPath(iFrom, iTo) )
		return false;

	m_cGraph.delete_arc( m_cGraph.begin() + iFrom, m_cGraph.begin() + iTo );
	return true;
}


//----------------------------------------------------------------------------------------------------------------
void CWaypoints::CreatePathsWithAutoFlags( TWaypointId iWaypoint1, TWaypointId iWaypoint2, bool bIsCrouched )
{
	DebugAssert( CWaypoints::IsValid(iWaypoint1) && CWaypoints::IsValid(iWaypoint2) );

	WaypointNode& w1 = m_cGraph[iWaypoint1];
	WaypointNode& w2 = m_cGraph[iWaypoint2];

	float fDist = w1.vertex.vOrigin.DistTo( w2.vertex.vOrigin );

	TReach iReach = CUtil::GetReachableInfoFromTo( w1.vertex.vOrigin, w2.vertex.vOrigin, fDist );
	if (iReach != EReachNotReachable)
	{
		TPathFlags iFlags = (iReach == EReachNeedJump) ? FPathJump :
									(iReach == EReachFallDamage) ? FPathDamage :
		                             FPathNone;
		if ( bIsCrouched )
			FLAG_SET(FPathCrouch, iFlags);

		AddPath(iWaypoint1, iWaypoint2, fDist, iFlags);
	}

	iReach = CUtil::GetReachableInfoFromTo( w2.vertex.vOrigin, w1.vertex.vOrigin, fDist );
	if (iReach != EReachNotReachable)
	{
		TPathFlags iFlags = (iReach == EReachNeedJump) ? FPathJump :
									(iReach == EReachFallDamage) ? FPathDamage :
		                             FPathNone;
		if ( bIsCrouched )
			FLAG_SET(FPathCrouch, iFlags);

		AddPath(iWaypoint2, iWaypoint1, fDist, iFlags);
	}

}


//----------------------------------------------------------------------------------------------------------------
void CWaypoints::CreateAutoPaths( TWaypointId id, bool bIsCrouched )
{
	WaypointNode& w = m_cGraph[id];
	Vector vOrigin = w.vertex.vOrigin;

	int minX, minY, minZ, maxX, maxY, maxZ;
	int x = GetBucketX(vOrigin.x);
	int y = GetBucketY(vOrigin.y);
	int z = GetBucketZ(vOrigin.z);
	GetBuckets(x, y, z, minX, minY, minZ, maxX, maxY, maxZ);

	for (x = minX; x <= maxX; ++x)
		for (y = minY; y <= maxY; ++y)
			for (z = minZ; z <= maxZ; ++z)
			{		
				Bucket& bucket = m_cBuckets[x][y][z];
				for (Bucket::iterator it=bucket.begin(); it != bucket.end(); ++it)
				{
					if (*it == id)
						continue;
					
					CreatePathsWithAutoFlags(id, *it, bIsCrouched);
				}
			}
}


//----------------------------------------------------------------------------------------------------------------
void CWaypoints::RemoveLocation( TWaypointId id )
{
	DebugAssert( CWaypoint::IsValid(id) );

	// Shift waypoints indexes, all waypoints with index > id.
	for (int x=0; x<BUCKETS_SIZE_X; ++x)
		for (int y=0; y<BUCKETS_SIZE_Y; ++y)
			for (int z=0; z<BUCKETS_SIZE_Z; ++z)
			{
				Bucket& bucket = m_cBuckets[x][y][z];
				for (Bucket::iterator it=bucket.begin(); it != bucket.end(); ++it)
					if (*it > id)
						--(*it);
			}

	// Remove waypoint id from bucket.
	Vector vOrigin = m_cGraph[id].vertex.vOrigin;
	Bucket& bucket = m_cBuckets[GetBucketX(vOrigin.x)][GetBucketY(vOrigin.y)][GetBucketZ(vOrigin.z)];
	bucket.erase(find(bucket.begin(), bucket.end(), id));
}


//----------------------------------------------------------------------------------------------------------------
TWaypointId CWaypoints::GetNearestWaypoint(Vector const& vOrigin, const good::bitset* aOmit, 
                                           bool bNeedVisible, float fMaxDistance, TWaypointFlags iFlags)
{
	TWaypointId result = -1;

	float sqDist = SQR(fMaxDistance);
	float sqMinDistance = sqDist;

	int minX, minY, minZ, maxX, maxY, maxZ;
	int x = GetBucketX(vOrigin.x);
	int y = GetBucketY(vOrigin.y);
	int z = GetBucketZ(vOrigin.z);
	GetBuckets(x, y, z, minX, minY, minZ, maxX, maxY, maxZ);

	for (x = minX; x <= maxX; ++x)
		for (y = minY; y <= maxY; ++y)
			for (z = minZ; z <= maxZ; ++z)
			{		
				Bucket& bucket = m_cBuckets[x][y][z];
				for (Bucket::iterator it=bucket.begin(); it != bucket.end(); ++it)
				{
					TWaypointId iWaypoint = *it;
					if (aOmit && aOmit->test(iWaypoint)) continue;

					WaypointNode& node = m_cGraph[iWaypoint];
					if ( FLAG_SOME_SET(iFlags, node.vertex.iFlags) )
					{
						float distTo = vOrigin.DistToSqr(node.vertex.vOrigin);
						if ( (distTo <= sqDist) && (distTo < sqMinDistance) )
						{
							if ( !bNeedVisible || CUtil::IsVisible(vOrigin, node.vertex.vOrigin) )
							{
								result = iWaypoint;
								sqMinDistance = distTo;
							}
						}
					}
				}
			}

	return result;
}


//----------------------------------------------------------------------------------------------------------------
TWaypointId CWaypoints::GetAnyWaypoint(Vector const& vOrigin, TWaypointFlags iFlags)
{
	if ( CWaypoints::Size() == 0 )
		return EInvalidWaypointId;

	TWaypointId id = rand() % CWaypoints::Size();
	for ( TWaypointId i = id; i >= 0; --i )
		if ( FLAG_SOME_SET(iFlags, CWaypoints::Get(i).iFlags) )
			return i;
	for ( TWaypointId i = id+1; i < CWaypoints::Size(); ++i )
		if ( FLAG_SOME_SET(iFlags, CWaypoints::Get(i).iFlags) )
			return i;
	return EInvalidWaypointId;
}


//----------------------------------------------------------------------------------------------------------------
void CWaypoints::Draw( CClient* pClient )
{
	if ( CBotrixPlugin::fTime < m_fNextDrawWaypointsTime )
		return;

	float fDrawTime = CWaypoint::DRAW_INTERVAL + (2.0f / CBotrixPlugin::iFPS); // Add two frames to not flick.
	m_fNextDrawWaypointsTime = CBotrixPlugin::fTime + CWaypoint::DRAW_INTERVAL;

	if ( pClient->iWaypointDrawFlags != FWaypointDrawNone )
	{
		Vector vOrigin;
		vOrigin = pClient->GetHead();
		int x = GetBucketX(vOrigin.x);
		int y = GetBucketY(vOrigin.y);
		int z = GetBucketZ(vOrigin.z);

		// Draw only waypoints from nearest buckets.
		int minX, minY, minZ, maxX, maxY, maxZ;
		GetBuckets(x, y, z, minX, minY, minZ, maxX, maxY, maxZ);

		// Get visible clusters from player's position.
		CUtil::SetPVSForVector(vOrigin);

		for (x = minX; x <= maxX; ++x)
			for (y = minY; y <= maxY; ++y)
				for (z = minZ; z <= maxZ; ++z)
				{		
					Bucket& bucket = m_cBuckets[x][y][z];
					for (Bucket::iterator it=bucket.begin(); it != bucket.end(); ++it)
					{
						WaypointNode& node = m_cGraph[*it];

						// Check if waypoint is in pvs from player's position.
						if ( CUtil::IsVisiblePVS(node.vertex.vOrigin) )
							node.vertex.Draw(*it, pClient->iWaypointDrawFlags, fDrawTime);
					}
				}
	}

	if ( pClient->iPathDrawFlags != FPathDrawNone )
	{
		// Draw nearest waypoint paths.
		if ( CWaypoint::IsValid( pClient->iCurrentWaypoint ) )
		{
			if ( pClient->iPathDrawFlags != FPathDrawNone )
				DrawWaypointPaths( pClient->iCurrentWaypoint, pClient->iPathDrawFlags );
			CWaypoint& w = CWaypoints::Get( pClient->iCurrentWaypoint );
			CUtil::DrawText(w.vOrigin, 0, fDrawTime, 0xFF, 0xFF, 0xFF, "Current");
		}

		if ( CWaypoint::IsValid(pClient->iDestinationWaypoint) )
		{
			CWaypoint& w = CWaypoints::Get( pClient->iDestinationWaypoint );
			Vector v(w.vOrigin);
			v.z -= 10.0f;
			CUtil::DrawText(v, 0, fDrawTime, 0xFF, 0xFF, 0xFF, "Destination");
		}
	}
}


//----------------------------------------------------------------------------------------------------------------
void CWaypoints::GetPathColor( TPathFlags iFlags, unsigned char& r, unsigned char& g, unsigned char& b )
{
	if ( FLAG_ALL_SET(FPathDemo, iFlags) )
	{
		r = 0xFF; g = 0x00; b = 0xFF; // Magenta effect, demo.
	}
	else if ( FLAG_ALL_SET(FPathBreak, iFlags) )
	{
		r = 0xFF; g = 0x00; b = 0x00; // Red effect, break.
	}
	else if ( FLAG_ALL_SET(FPathSprint, iFlags) )
	{
		r = 0xFF; g = 0xFF; b = 0x00; // Yellow effect, sprint.
	}
	else if ( FLAG_ALL_SET(FPathStop, iFlags) )
	{
		r = 0x66; g = 0x66; b = 0x00; // Dark yellow effect, stop.
	}
	else if ( FLAG_ALL_SET(FPathLadder, iFlags) )
	{
		r = 0xFF; g = 0x33; b = 0x00; // Orange effect, ladder.
	}
	else if ( FLAG_ALL_SET(FPathJump | FPathCrouch, iFlags) )
	{
		r = 0x00; g = 0x00; b = 0x66; // Dark blue effect, jump + crouch.
	}
	else if ( FLAG_ALL_SET(FPathJump, iFlags) )
	{
		r = 0x00; g = 0x00; b = 0xFF; // Light blue effect, jump.
	}
	else if ( FLAG_ALL_SET(FPathCrouch, iFlags) )
	{
		r = 0x00; g = 0xFF; b = 0x00; // Green effect, crouch.
	}
	else if ( FLAG_ALL_SET(FPathDoor, iFlags) )
	{
		r = 0x8A; g = 0x2B; b = 0xE2;  // Violet effect, door.
	}
	else if ( FLAG_ALL_SET(FPathTotem, iFlags) )
	{
		r = 0x96; g = 0x48; b = 0x00;  // Brown effect, totem.
	}
	else
	{
		r = 0x00; g = 0x7F; b = 0x7F; // Cyan effect, other flags.
	}
}


//----------------------------------------------------------------------------------------------------------------
void CWaypoints::DrawWaypointPaths( TWaypointId id, TPathDrawFlags iPathDrawFlags )
{
	DebugAssert( iPathDrawFlags != FPathDrawNone );

	WaypointNode& w = m_cGraph[id];
	
	unsigned char r, g, b;
	Vector diff(0, 0, -CUtil::iPlayerEyeLevel/4);
	float fDrawTime = CWaypoint::DRAW_INTERVAL + (2.0f / CBotrixPlugin::iFPS); // Add two frames to not flick.

	for (WaypointArcIt it = w.neighbours.begin(); it != w.neighbours.end(); ++it)
	{
		WaypointNode& n = m_cGraph[it->target];
		GetPathColor(it->edge.iFlags, r, g, b);

		if ( FLAG_ALL_SET(FPathDrawBeam, iPathDrawFlags) )
			CUtil::DrawBeam(w.vertex.vOrigin + diff, n.vertex.vOrigin + diff, CWaypoint::PATH_WIDTH, fDrawTime, r, g, b);
		if ( FLAG_SOME_SET(FPathDrawLine, iPathDrawFlags) )
			CUtil::DrawLine(w.vertex.vOrigin + diff, n.vertex.vOrigin + diff, fDrawTime, r, g, b);
	}
}
