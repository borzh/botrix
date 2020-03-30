#include "clients.h"
#include "item.h"
#include "server_plugin.h"
#include "type2string.h"
#include "waypoint.h"

#include <good/string_buffer.h>

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

static const char* WAYPOINT_FILE_HEADER_ID     = "BtxW";   // Botrix's Waypoints.

static const int WAYPOINT_VERSION              = 1;        // Waypoints file version.
static const int WAYPOINT_FILE_FLAG_VISIBILITY = 1<<0;     // Flag for waypoint visibility table.
static const int WAYPOINT_FILE_FLAG_AREAS      = 1<<1;     // Flag for area names.


//----------------------------------------------------------------------------------------------------------------
// CWaypoint static members.
//----------------------------------------------------------------------------------------------------------------
int CWaypoint::iWaypointTexture = -1;
int CWaypoint::iDefaultDistance = 128;

const TWaypointFlags CWaypoint::m_aFlagsForEntityType[EItemTypeNotObject] =
{
    FWaypointHealth | FWaypointHealthMachine,
    FWaypointArmor | FWaypointArmorMachine,
    FWaypointWeapon,
    FWaypointAmmo
};


StringVector CWaypoints::m_cAreas;
CWaypoints::WaypointGraph CWaypoints::m_cGraph;
float CWaypoints::fNextDrawWaypointsTime = 0.0f;
CWaypoints::Bucket CWaypoints::m_cBuckets[CWaypoints::BUCKETS_SIZE_X][CWaypoints::BUCKETS_SIZE_Y][CWaypoints::BUCKETS_SIZE_Z];

good::vector< good::bitset > CWaypoints::m_aVisTable;
bool CWaypoints::bValidVisibilityTable = false;

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

    Vector vEnd = Vector(vOrigin.x, vOrigin.y, vOrigin.z - CMod::iPlayerEyeLevel);

    if ( FLAG_ALL_SET_OR_0(FWaypointDrawBeam, iDrawType) )
        CUtil::DrawBeam(vOrigin, vEnd, WIDTH, fDrawTime, r, g, b);

    if ( FLAG_ALL_SET_OR_0(FWaypointDrawLine, iDrawType) )
        CUtil::DrawLine(vOrigin, vEnd, fDrawTime, r, g, b);

    if ( FLAG_ALL_SET_OR_0(FWaypointDrawBox, iDrawType) )
    {
        Vector vBoxOrigin(vOrigin.x - CMod::iPlayerWidth/2, vOrigin.y - CMod::iPlayerWidth/2, vOrigin.z - CMod::iPlayerEyeLevel);
        CUtil::DrawBox(vBoxOrigin, CUtil::vZero, CMod::vPlayerCollisionHull, fDrawTime, r, g, b);
    }

    if ( FLAG_ALL_SET_OR_0(FWaypointDrawText, iDrawType) )
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
    //if ( Size() == 0 )
    //{
    //    BLOG_E( "No waypoints to save." );
    //    return false;
    //}

    const good::string& sFileName = CUtil::BuildFileName("waypoints", CBotrixPlugin::instance->sMapName, "way");

    FILE *f = CUtil::OpenFile(sFileName.c_str(), "wb");
    if (f == NULL)
        return false;

    waypoint_header header;
    header.iFlags = 0;
    FLAG_SET(WAYPOINT_FILE_FLAG_AREAS, header.iFlags);
    FLAG_SET(WAYPOINT_FILE_FLAG_VISIBILITY, header.iFlags);
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

    // Save area names.
    int iAreaNamesSize = m_cAreas.size();
    BASSERT( iAreaNamesSize >= 0, iAreaNamesSize=0 );

    fwrite(&iAreaNamesSize, sizeof(int), 1, f); // Save area names size.

    for ( int i=1; i < iAreaNamesSize; i++ ) // First area name is always empty, for new waypoints.
    {
        int iSize = m_cAreas[i].size();
        fwrite(&iSize, 1, sizeof(int), f); // Save string size.
        fwrite(m_cAreas[i].c_str(), 1, iSize+1, f); // Write string & trailing 0.
    }

    // Save waypoint visibility table.
    BLOG_W( "Saving waypoint visibility table... this may take a while." );
    m_aVisTable.resize( Size() );
    for ( TWaypointId i = 0; i < Size(); ++i )
    {
        good::bitset& cVisibles = m_aVisTable[i];
        cVisibles.resize( Size() );

        Vector vFrom = Get(i).vOrigin;
        for ( TWaypointId j = 0; j < Size(); ++j )
        {
            if ( i < j )
                cVisibles.set( j, CUtil::IsVisible(vFrom, Get(j).vOrigin) );
            else
                cVisibles.set( j, (i == j) || m_aVisTable[j].test(i) );
        }
        fwrite( cVisibles.data(), 1, cVisibles.byte_size(), f );
    }

    fclose(f);

    bValidVisibilityTable = true;
    return true;
}


//----------------------------------------------------------------------------------------------------------------
bool CWaypoints::Load()
{
    Clear();

    const good::string& sFileName = CUtil::BuildFileName("waypoints", CBotrixPlugin::instance->sMapName, "way");

    FILE *f = CUtil::OpenFile(sFileName, "rb");
    if ( f == NULL )
    {
        BLOG_W( "No waypoints for map %s:", CBotrixPlugin::instance->sMapName.c_str() );
        BLOG_W( "  File '%s' doesn't exists.", sFileName.c_str() );
        return false;
    }

    struct waypoint_header header;
    int iRead = fread(&header, 1, sizeof(struct waypoint_header), f);
    BASSERT(iRead == sizeof(struct waypoint_header), Clear();fclose(f);return false);

    if (*((int*)&WAYPOINT_FILE_HEADER_ID[0]) != header.szFileType)
    {
        BLOG_E("Error loading waypoints: invalid file header.");
        fclose(f);
        return false;
    }
    if ( (header.iVersion <= 0) || (header.iVersion > WAYPOINT_VERSION) )
    {
        BLOG_E( "Error loading waypoints, version mismatch:" );
        BLOG_E( "  File version %d, current waypoint version %d.", header.iVersion, WAYPOINT_VERSION );
        fclose(f);
        return false;
    }
    if ( CBotrixPlugin::instance->sMapName != header.szMapName )
    {
        BLOG_E( "Error loading waypoints, map name mismatch:" );
        BLOG_E( "  File map %s, current map %s.", header.szMapName, CBotrixPlugin::instance->sMapName.c_str() );
        fclose(f);
        return false;
    }

    Vector vOrigin;
    TWaypointFlags iFlags;
    int iNumPaths, iArgument = 0;
    TAreaId iAreaId = 0;

    // Read waypoints information.
    for ( TWaypointId i = 0; i < header.iNumWaypoints; ++i )
    {
        iRead = fread(&vOrigin, 1, sizeof(Vector), f);
        BASSERT(iRead == sizeof(Vector), Clear();fclose(f);return false);

        iRead = fread(&iFlags, 1, sizeof(TWaypointFlags), f);
        BASSERT(iRead == sizeof(TWaypointFlags), Clear();fclose(f);return false);

        iRead = fread(&iAreaId, 1, sizeof(TAreaId), f);
        BASSERT(iRead == sizeof(TAreaId), Clear();fclose(f);return false);
        if ( FLAG_CLEARED(WAYPOINT_FILE_FLAG_AREAS, header.iFlags) )
            iAreaId = 0;

        iRead = fread(&iArgument, 1, sizeof(int), f);
        BASSERT(iRead == sizeof(int), Clear();fclose(f);return false);

        Add(vOrigin, iFlags, iArgument, iAreaId);
    }

    // Read waypoints paths.
    TWaypointId iPathTo;
    TPathFlags iPathFlags;
    unsigned short iPathArgument;

    for ( TWaypointId i = 0; i < header.iNumWaypoints; ++i )
    {
        WaypointGraph::node_it from = m_cGraph.begin() + i;

        iRead = fread(&iNumPaths, 1, sizeof(int), f);
        BASSERT( (iRead == sizeof(int)) && (0 <= iNumPaths) && (iNumPaths < header.iNumWaypoints), Clear();fclose(f);return false );

        m_cGraph[i].neighbours.reserve(iNumPaths);

        for ( int n = 0; n < iNumPaths; n ++ )
        {
            iRead = fread(&iPathTo, 1, sizeof(int), f);
            BASSERT( (iRead == sizeof(int)) && (0 <= iPathTo) && (iPathTo < header.iNumWaypoints), Clear();fclose(f);return false );

            iRead = fread(&iPathFlags, 1, sizeof(TPathFlags), f);
            BASSERT( iRead == sizeof(TPathFlags), Clear();fclose(f);return false );

            iRead = fread(&iPathArgument, 1, sizeof(unsigned short), f);
            BASSERT( iRead == sizeof(unsigned short), Clear();fclose(f);return false );

            WaypointGraph::node_it to = m_cGraph.begin() + iPathTo;
            m_cGraph.add_arc( from, to, CWaypointPath(from->vertex.vOrigin.DistTo(to->vertex.vOrigin), iPathFlags, iPathArgument) );
        }
    }

    int iAreaNamesSize = 0;
    if ( FLAG_SOME_SET(WAYPOINT_FILE_FLAG_AREAS, header.iFlags) )
    {
        // Read area names.
        iRead = fread(&iAreaNamesSize, 1, sizeof(int), f);
        BASSERT( iRead == sizeof(int), Clear();fclose(f);return false);
        //BASSERT( (0 <= iAreaNamesSize) && (iAreaNamesSize <= header.iNumWaypoints), Clear();fclose(f);return false );

        m_cAreas.reserve(iAreaNamesSize);
        m_cAreas.push_back("default"); // New waypoints without area id will be put under this empty area id.

        for ( int i=1; i < iAreaNamesSize; i++ )
        {
            int iStrSize;
            iRead = fread(&iStrSize, 1, sizeof(int), f);
            BASSERT(iRead == sizeof(int), Clear();fclose(f);return false);

            BASSERT(0 < iStrSize && iStrSize < iMainBufferSize, Clear(); return false);
            if ( iStrSize > 0 )
            {
                iRead = fread(szMainBuffer, 1, iStrSize+1, f); // Read also trailing 0.
                BASSERT(iRead == iStrSize+1, Clear();fclose(f);return false);

                good::string sArea(szMainBuffer, true, true, iStrSize);
                m_cAreas.push_back(sArea);
            }
        }
    }
    else
        m_cAreas.push_back("default"); // New waypoints without area id will be put under this empty area id.

    // Check for areas names.
    iAreaNamesSize = m_cAreas.size();
    for ( TWaypointId i = 0; i < header.iNumWaypoints; ++i )
    {
        if ( m_cGraph[i].vertex.iAreaId >= iAreaNamesSize )
        {
            BreakDebugger();
            m_cGraph[i].vertex.iAreaId = 0;
        }
    }

    bValidVisibilityTable = header.iFlags & WAYPOINT_FILE_FLAG_VISIBILITY;
    if ( bValidVisibilityTable )
    {
        m_aVisTable.resize( header.iNumWaypoints );

        for ( TWaypointId i = 0; i < Size(); ++i )
        {
            m_aVisTable[i].resize( header.iNumWaypoints );
            int iByteSize = m_aVisTable[i].byte_size();
            iRead = fread( m_aVisTable[i].data(), 1, iByteSize, f );
            if ( iRead != iByteSize )
            {
                BLOG_E( "Invalid waypoints visibility table." );
                Clear();
                fclose(f);
                return false;
            }
        }
        BLOG_I( "Waypoints visibility table loaded." );
    }
    else
        BLOG_W( "No waypoints visibility table in file." );

    fclose(f);

    return true;
}


//----------------------------------------------------------------------------------------------------------------
TWaypointId CWaypoints::GetRandomNeighbour( TWaypointId iWaypoint, TWaypointId iTo, bool bVisible )
{
    const WaypointNode::arcs_t& aNeighbours = GetNode(iWaypoint).neighbours;
    if ( !aNeighbours.size() )
        return EWaypointIdInvalid;

    TWaypointId iResult = rand() % aNeighbours.size();
    if ( bValidVisibilityTable && CWaypoint::IsValid(iTo) )
    {
        for ( int i = 0; i < aNeighbours.size(); ++i )
        {
            TWaypointId iNeighbour = aNeighbours[iResult].target;
            if ( m_aVisTable[iNeighbour].test(iTo) == bVisible )
                return iNeighbour;
            if ( ++iResult == aNeighbours.size() )
                iResult = 0;
        }
    }
    return aNeighbours[iResult].target;
}


//----------------------------------------------------------------------------------------------------------------
TWaypointId CWaypoints::GetNearestNeighbour( TWaypointId iWaypoint, TWaypointId iTo, bool bVisible )
{
    GoodAssert( bValidVisibilityTable );
    const WaypointNode::arcs_t& aNeighbours = GetNode(iWaypoint).neighbours;
    if ( !aNeighbours.size() )
        return EWaypointIdInvalid;

    Vector vTo = Get(iTo).vOrigin;

    TWaypointId iResult = aNeighbours[0].target;
    float fMinDist = Get(iResult).vOrigin.DistToSqr(vTo);
    bool bResultVisibleOk = (m_aVisTable[iResult].test(iTo) == bVisible);

    for ( int i = 1; i < aNeighbours.size(); ++i )
    {
        TWaypointId iNeighbour = aNeighbours[i].target;
        bool bVisibleNeighbourOk = (m_aVisTable[iNeighbour].test(iTo) == bVisible);
        if ( bResultVisibleOk && !bVisibleNeighbourOk )
            continue;

        Vector& vOrigin = Get(iNeighbour).vOrigin;
        float fDist = vOrigin.DistToSqr(vTo);
        if ( fDist < fMinDist )
        {
            bVisible = bVisibleNeighbourOk;
            fMinDist = fDist;
            iResult = iNeighbour;
        }
    }
    return iResult;
}

//----------------------------------------------------------------------------------------------------------------
TWaypointId CWaypoints::GetFarestNeighbour( TWaypointId iWaypoint, TWaypointId iTo, bool bVisible )
{
    GoodAssert( bValidVisibilityTable );
    const WaypointNode::arcs_t& aNeighbours = GetNode(iWaypoint).neighbours;
    if ( !aNeighbours.size() )
        return EWaypointIdInvalid;

    Vector vTo = Get(iTo).vOrigin;

    TWaypointId iResult = aNeighbours[0].target;
    float fMaxDist = Get(iResult).vOrigin.DistToSqr(vTo);
    bool bResultVisibleOk = (m_aVisTable[iResult].test(iTo) == bVisible);

    for ( int i = 1; i < aNeighbours.size(); ++i )
    {
        TWaypointId iNeighbour = aNeighbours[i].target;
        bool bVisibleNeighbourOk = (m_aVisTable[iNeighbour].test(iTo) == bVisible);
        if ( bResultVisibleOk && !bVisibleNeighbourOk )
            continue;

        Vector& vOrigin = Get(iNeighbour).vOrigin;
        float fDist = vOrigin.DistToSqr(vTo);
        if ( fDist > fMaxDist )
        {
            bVisible = bVisibleNeighbourOk;
            fMaxDist = fDist;
            iResult = iNeighbour;
        }
    }
    return iResult;
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
TWaypointId CWaypoints::Add( const Vector& vOrigin, TWaypointFlags iFlags, int iArgument, int iAreaId )
{
    CWaypoint w(vOrigin, iFlags, iArgument, iAreaId);

    // lol, this is not working because m_cGraph.begin() is called first. wtf?
    // TWaypointId id = m_cGraph.add_node(w) - m_cGraph.begin();
    CWaypoints::WaypointNodeIt it( m_cGraph.add_node(w) );
    TWaypointId id = it - m_cGraph.begin();

    AddLocation(id, vOrigin);
    bValidVisibilityTable = false;
    return id;
}


//----------------------------------------------------------------------------------------------------------------
void CWaypoints::Remove( TWaypointId id )
{
    //CItems::WaypointDeleted(id);
    DecrementLocationIds(id);
    m_cGraph.delete_node( m_cGraph.begin() + id );
    bValidVisibilityTable = false;
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
    BASSERT( CWaypoints::IsValid(iWaypoint1) && CWaypoints::IsValid(iWaypoint2), return );

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
void CWaypoints::DecrementLocationIds( TWaypointId id )
{
    BASSERT( CWaypoint::IsValid(id), return );

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
    bucket.erase(find(bucket, id));
}


//----------------------------------------------------------------------------------------------------------------
TWaypointId CWaypoints::GetNearestWaypoint(const Vector& vOrigin, const good::bitset* aOmit,
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
                    if ( aOmit && aOmit->test(iWaypoint) )
                        continue;

                    WaypointNode& node = m_cGraph[iWaypoint];
                    if ( FLAG_SOME_SET_OR_0(iFlags, node.vertex.iFlags) )
                    {
                        float distTo = vOrigin.DistToSqr(node.vertex.vOrigin);
                        if ( (distTo <= sqDist) && (distTo < sqMinDistance) )
                        {
                            if ( !bNeedVisible || CUtil::IsVisible(vOrigin, node.vertex.vOrigin, FVisibilityWorld) )
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
TWaypointId CWaypoints::GetAnyWaypoint(TWaypointFlags iFlags)
{
    if ( CWaypoints::Size() == 0 )
        return EWaypointIdInvalid;

    TWaypointId id = rand() % CWaypoints::Size();
    for ( TWaypointId i = id; i >= 0; --i )
        if ( FLAG_SOME_SET_OR_0(iFlags, CWaypoints::Get(i).iFlags) )
            return i;
    for ( TWaypointId i = id+1; i < CWaypoints::Size(); ++i )
        if ( FLAG_SOME_SET_OR_0(iFlags, CWaypoints::Get(i).iFlags) )
            return i;
    return EWaypointIdInvalid;
}


//----------------------------------------------------------------------------------------------------------------
TWaypointId CWaypoints::GetAimedWaypoint( const Vector& vOrigin, const QAngle& ang )
{
    int x = GetBucketX(vOrigin.x);
    int y = GetBucketY(vOrigin.y);
    int z = GetBucketZ(vOrigin.z);

    // Draw only waypoints from nearest buckets.
    int minX, minY, minZ, maxX, maxY, maxZ;
    GetBuckets(x, y, z, minX, minY, minZ, maxX, maxY, maxZ);

    // Get visible clusters from player's position.
    CUtil::SetPVSForVector(vOrigin);

    TWaypointId iResult = EWaypointIdInvalid;
    float fLowestAngDiff = 180 + 90; // Set to max angle difference.

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
                    {
                        Vector vRelative(node.vertex.vOrigin);
                        vRelative.z -= CMod::iPlayerEyeLevel>>1; // Consider to look at center of waypoint.
                        vRelative -= vOrigin;

                        QAngle angDiff;
                        VectorAngles( vRelative, angDiff );
                        CUtil::DeNormalizeAngle(angDiff.y);
                        CUtil::GetAngleDifference(ang, angDiff, angDiff);
                        float fAngDiff = fabs(angDiff.x) + fabs(angDiff.y);
                        if ( fAngDiff < fLowestAngDiff )
                        {
                            fLowestAngDiff = fAngDiff;
                            iResult = *it;
                        }
                    }
                }
            }

    return iResult;
}


//----------------------------------------------------------------------------------------------------------------
void CWaypoints::Draw( CClient* pClient )
{
    if ( CBotrixPlugin::fTime < fNextDrawWaypointsTime )
        return;

    float fDrawTime = CWaypoint::DRAW_INTERVAL + (2.0f / CBotrixPlugin::iFPS); // Add two frames to not flick.
    fNextDrawWaypointsTime = CBotrixPlugin::fTime + CWaypoint::DRAW_INTERVAL;

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

    if ( bValidVisibilityTable && (pClient->iVisiblesDrawFlags != FPathDrawNone) &&
         CWaypoint::IsValid(pClient->iCurrentWaypoint) )
        DrawVisiblePaths( pClient->iCurrentWaypoint, pClient->iVisiblesDrawFlags );
}


//----------------------------------------------------------------------------------------------------------------
void CWaypoints::GetPathColor( TPathFlags iFlags, unsigned char& r, unsigned char& g, unsigned char& b )
{
    if ( FLAG_ALL_SET_OR_0(FPathDemo, iFlags) )
    {
        r = 0xFF; g = 0x00; b = 0xFF; // Magenta effect, demo.
    }
    else if ( FLAG_ALL_SET_OR_0(FPathBreak, iFlags) )
    {
        r = 0xFF; g = 0x00; b = 0x00; // Red effect, break.
    }
    else if ( FLAG_ALL_SET_OR_0(FPathSprint, iFlags) )
    {
        r = 0xFF; g = 0xFF; b = 0x00; // Yellow effect, sprint.
    }
    else if ( FLAG_ALL_SET_OR_0(FPathStop, iFlags) )
    {
        r = 0x66; g = 0x66; b = 0x00; // Dark yellow effect, stop.
    }
    else if ( FLAG_ALL_SET_OR_0(FPathLadder, iFlags) )
    {
        r = 0xFF; g = 0x33; b = 0x00; // Orange effect, ladder.
    }
    else if ( FLAG_ALL_SET_OR_0(FPathJump | FPathCrouch, iFlags) )
    {
        r = 0x00; g = 0x00; b = 0x66; // Dark blue effect, jump + crouch.
    }
    else if ( FLAG_ALL_SET_OR_0(FPathJump, iFlags) )
    {
        r = 0x00; g = 0x00; b = 0xFF; // Light blue effect, jump.
    }
    else if ( FLAG_ALL_SET_OR_0(FPathCrouch, iFlags) )
    {
        r = 0x00; g = 0xFF; b = 0x00; // Green effect, crouch.
    }
    else if ( FLAG_ALL_SET_OR_0(FPathDoor, iFlags) )
    {
        r = 0x8A; g = 0x2B; b = 0xE2;  // Violet effect, door.
    }
    else if ( FLAG_ALL_SET_OR_0(FPathTotem, iFlags) )
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
    BASSERT( iPathDrawFlags != FPathDrawNone, return );

    WaypointNode& w = m_cGraph[id];

    unsigned char r, g, b;
    Vector diff(0, 0, -CMod::iPlayerEyeLevel/4);
    float fDrawTime = CWaypoint::DRAW_INTERVAL + (2.0f / CBotrixPlugin::iFPS); // Add two frames to not flick.

    for (WaypointArcIt it = w.neighbours.begin(); it != w.neighbours.end(); ++it)
    {
        WaypointNode& n = m_cGraph[it->target];
        GetPathColor(it->edge.iFlags, r, g, b);

        if ( FLAG_SOME_SET(FPathDrawBeam, iPathDrawFlags) )
            CUtil::DrawBeam(w.vertex.vOrigin + diff, n.vertex.vOrigin + diff, CWaypoint::PATH_WIDTH, fDrawTime, r, g, b);
        if ( FLAG_SOME_SET(FPathDrawLine, iPathDrawFlags) )
            CUtil::DrawLine(w.vertex.vOrigin + diff, n.vertex.vOrigin + diff, fDrawTime, r, g, b);
    }
}


//----------------------------------------------------------------------------------------------------------------
void CWaypoints::DrawVisiblePaths( TWaypointId id, TPathDrawFlags iPathDrawFlags )
{
    GoodAssert( bValidVisibilityTable && (iPathDrawFlags != FPathDrawNone) );

    Vector vOrigin( Get(id).vOrigin );
    Vector diff(0, 0, -CMod::iPlayerEyeLevel/2);

    const unsigned char r = 0xFF, g = 0xFF, b = 0xFF;
    float fDrawTime = CWaypoint::DRAW_INTERVAL + (2.0f / CBotrixPlugin::iFPS); // Add two frames to not flick.

    for ( int i = 0; i < Size(); ++i )
        if ( (i != id) && m_aVisTable[id].test(i) )
        {
            const CWaypoint& cWaypoint = Get(i);
            if ( FLAG_SOME_SET(FPathDrawBeam, iPathDrawFlags) )
                CUtil::DrawBeam(vOrigin + diff, cWaypoint.vOrigin + diff, CWaypoint::PATH_WIDTH, fDrawTime, r, g, b);
            if ( FLAG_SOME_SET(FPathDrawLine, iPathDrawFlags) )
                CUtil::DrawLine(vOrigin + diff, cWaypoint.vOrigin + diff, fDrawTime, r, g, b);
        }
}
