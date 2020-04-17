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
int CWaypoint::iDefaultDistance = 144;

int CWaypoint::iAnalizeDistance = 96;
int CWaypoint::iWaypointsMaxCountToAnalizeMap = 64;
float CWaypoint::fAnalizeWaypointsPerFrame = 1;

const TWaypointFlags CWaypoint::m_aFlagsForEntityType[EItemTypeTotalNotObject] =
{
    FWaypointHealth | FWaypointHealthMachine,
    FWaypointArmor | FWaypointArmorMachine,
    FWaypointWeapon,
    FWaypointAmmo,
    FWaypointButton,
    FWaypointNone, // FWaypointDoor,
    FWaypointNone, // FWaypointLadder,
};


StringVector CWaypoints::m_cAreas;
CWaypoints::WaypointGraph CWaypoints::m_cGraph;
float CWaypoints::fNextDrawWaypointsTime = 0.0f;
CWaypoints::Bucket CWaypoints::m_cBuckets[CWaypoints::BUCKETS_SIZE_X][CWaypoints::BUCKETS_SIZE_Y][CWaypoints::BUCKETS_SIZE_Z];

good::vector< good::bitset > CWaypoints::m_aVisTable;
bool CWaypoints::bValidVisibilityTable = false;

// Sometimes the analize doesn't add waypoints in small passages, so we try to add waypoints at inter-position between waypoint 
// analized neighbours. Waypoint analized neighbours are x's. W = analized waypoint.
//
// x - x - x
// |   |   |
// x - W - x
// |   |   |
// x - x - x
//
// '-' and '|' on the picture will be evaluated in 'inters' step, when the adjacent waypoints are not set for some reason (like hit wall).
good::vector<TWaypointId> CWaypoints::m_aWaypointsToAnalize;
good::vector<CWaypoints::CNeighbour> CWaypoints::m_aWaypointsNeighbours;
good::vector<Vector> CWaypoints::m_aWaypointsToOmitAnalize;

CWaypoints::TAnalizeStep CWaypoints::m_iAnalizeStep = EAnalizeStepTotal;
float CWaypoints::m_fAnalizeWaypointsForNextFrame = 0;
edict_t* CWaypoints::m_pAnalizer = NULL;
bool CWaypoints::m_bIsAnalizeStepAddedWaypoints = false;

static const float ANALIZE_HELP_IF_LESS_WAYPOINTS_PER_FRAME = 0.0011;

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

    Vector vEnd = Vector(vOrigin.x, vOrigin.y, vOrigin.z - CMod::GetVar( EModVarPlayerEye ));

    if ( FLAG_ALL_SET_OR_0(FWaypointDrawBeam, iDrawType) )
        CUtil::DrawBeam(vOrigin, vEnd, WIDTH, fDrawTime, r, g, b);

    if ( FLAG_ALL_SET_OR_0(FWaypointDrawLine, iDrawType) )
        CUtil::DrawLine(vOrigin, vEnd, fDrawTime, r, g, b);

    if ( FLAG_ALL_SET_OR_0(FWaypointDrawBox, iDrawType) )
    {
        Vector vBoxOrigin(vOrigin.x - CMod::GetVar( EModVarPlayerWidth )/2, vOrigin.y - CMod::GetVar( EModVarPlayerWidth )/2, vOrigin.z - CMod::GetVar( EModVarPlayerEye ));
        CUtil::DrawBox(vBoxOrigin, CUtil::vZero, CMod::vPlayerCollisionHull, fDrawTime, r, g, b);
    }

    if ( FLAG_ALL_SET_OR_0(FWaypointDrawText, iDrawType) )
    {
        sprintf(szMainBuffer, "%d", iWaypointId);
        Vector v = vOrigin;
        v.z -= 20.0f;
        int i = 0;
		CUtil::DrawText( v, i++, fDrawTime, 0xFF, 0xFF, 0xFF, szMainBuffer );
        CUtil::DrawText(v, i++, fDrawTime, 0xFF, 0xFF, 0xFF, CWaypoints::GetAreas()[iAreaId].c_str());
		if ( FLAG_SOME_SET( FWaypointButton | FWaypointSeeButton, iFlags ) )
		{
			sprintf( szMainBuffer, FLAG_SOME_SET( FWaypointSeeButton, iFlags ) ? "see button %d" : "button %d", 
			         CWaypoint::GetButton( iArgument ) );
			CUtil::DrawText( v, i++, fDrawTime, 0xFF, 0xFF, 0xFF, szMainBuffer );
			sprintf( szMainBuffer, FLAG_SOME_SET( FWaypointElevator, iFlags ) ? "for elevator %d" : "for door %d",
			         CWaypoint::GetDoor( iArgument ) );
			CUtil::DrawText( v, i++, fDrawTime, 0xFF, 0xFF, 0xFF, szMainBuffer );
		}
		else
			CUtil::DrawText( v, i++, fDrawTime, 0xFF, 0xFF, 0xFF, CTypeToString::WaypointFlagsToString( iFlags ).c_str() );
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
            fwrite(&arcIt->edge.iFlags, sizeof(TPathFlags), 1, f);         // Save path flags.
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
                cVisibles.set( j, CUtil::IsVisible( vFrom, Get( j ).vOrigin, EVisibilityWorld ) );
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
        BLOG_W( "Warning loading waypoints, map name mismatch:" );
        BLOG_W( "  File map %s, current map %s.", header.szMapName, CBotrixPlugin::instance->sMapName.c_str() );
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
    BASSERT( CWaypoint::IsValid(iFrom) && CWaypoint::IsValid(iTo) && (iFrom != iTo) && !HasPath(iFrom, iTo), return false );

    WaypointGraph::node_it from = m_cGraph.begin() + iFrom;
    WaypointGraph::node_it to = m_cGraph.begin() + iTo;

    if ( fDistance <= 0.0f)
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
void CWaypoints::CreatePathsWithAutoFlags( TWaypointId iWaypoint1, TWaypointId iWaypoint2, bool bIsCrouched, int iMaxDistance, bool bShowHelp )
{
    BASSERT( CWaypoints::IsValid(iWaypoint1) && CWaypoints::IsValid(iWaypoint2), return );

    WaypointNode& w1 = m_cGraph[iWaypoint1];
    WaypointNode& w2 = m_cGraph[iWaypoint2];

    float fDist = w1.vertex.vOrigin.DistTo( w2.vertex.vOrigin );

    Vector v1 = w1.vertex.vOrigin, v2 = w2.vertex.vOrigin;
    bool bCrouch = bIsCrouched;
    TReach iReach = CUtil::GetReachableInfoFromTo( v1, v2, bCrouch, 0, Sqr( iMaxDistance ), bShowHelp );
    if (iReach != EReachNotReachable)
    {
        TPathFlags iFlags = (iReach == EReachNeedJump) ? FPathJump :
            (iReach == EReachFallDamage) ? FPathDamage : FPathNone;
        if ( bIsCrouched )
            FLAG_SET(FPathCrouch, iFlags);

        AddPath(iWaypoint1, iWaypoint2, fDist, iFlags);
    }

    iReach = CUtil::GetReachableInfoFromTo( v2, v1, bCrouch, 0, Sqr( iMaxDistance ), bShowHelp );
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
void CWaypoints::CreateAutoPaths( TWaypointId id, bool bIsCrouched, float fMaxDistance, bool bShowHelp )
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
                for ( Bucket::iterator it = bucket.begin(); it != bucket.end(); ++it )
                {
                    if ( *it == id || HasPath( *it, id ) || HasPath( id, *it ) )
                        continue;

                    CreatePathsWithAutoFlags( id, *it, bIsCrouched, fMaxDistance, bShowHelp );
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
	TWaypointId result = EWaypointIdInvalid;

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
                            if ( !bNeedVisible || CUtil::IsVisible( vOrigin, node.vertex.vOrigin, EVisibilityWorld ) )
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
void CWaypoints::GetNearestWaypoints( good::vector<TWaypointId>& aResult, const Vector& vOrigin, bool bNeedVisible, float fMaxDistance )
{
    float fDistSqr = SQR( fMaxDistance );

    int minX, minY, minZ, maxX, maxY, maxZ;
    int x = GetBucketX( vOrigin.x );
    int y = GetBucketY( vOrigin.y );
    int z = GetBucketZ( vOrigin.z );

    GetBuckets( x, y, z, minX, minY, minZ, maxX, maxY, maxZ );

    for ( x = minX; x <= maxX; ++x )
        for ( y = minY; y <= maxY; ++y )
            for ( z = minZ; z <= maxZ; ++z )
            {
                Bucket& bucket = m_cBuckets[ x ][ y ][ z ];
                for ( Bucket::iterator it = bucket.begin(); it != bucket.end(); ++it )
                {
                    TWaypointId iWaypoint = *it;
                    const WaypointNode& node = m_cGraph[ iWaypoint ];

                    //if ( fabs( vOrigin.z - node.vertex.vOrigin.z ) <= fMaxDistance )
                    {
                        float distTo = vOrigin.AsVector2D().DistToSqr( node.vertex.vOrigin.AsVector2D() );
                        if ( distTo <= fDistSqr && ( !bNeedVisible || distTo == 0 || CUtil::IsVisible( vOrigin, node.vertex.vOrigin, EVisibilityWorld ) ) )
                            aResult.push_back( iWaypoint );
                    }
                }
            }
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
                        vRelative.z -= CMod::GetVar( EModVarPlayerEye ) / 2; // Consider to look at center of waypoint.
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
void CWaypoints::Analize( edict_t* pClient)
{
    m_pAnalizer = pClient;
    const good::vector<CItem>& aSpawnSpots = CItems::GetItems( EItemTypePlayerSpawn );
    float fPlayerEye = CMod::GetVar( EModVarPlayerEye );
    for ( TItemIndex i = 0; i < aSpawnSpots.size(); ++i )
    {
        Vector vPos = aSpawnSpots[ i ].CurrentPosition();
        vPos.z += fPlayerEye;
        vPos = CUtil::GetGroundVec( vPos );
        vPos.z += fPlayerEye;

        TWaypointId iWaypoint = CWaypoints::GetNearestWaypoint( vPos, NULL, true, CWaypoint::iDefaultDistance );
        if ( iWaypoint == EWaypointIdInvalid )
        {
            iWaypoint = Add( vPos );
            BULOG_T( m_pAnalizer, "Added waypoint %d at (%.0f, %.0f, %.0f)", iWaypoint, vPos.x, vPos.y, vPos.z );
        }
    }

     if ( Size() == 0 )
    {
        StopAnalizing();
        BULOG_W( pClient, "No waypoints to analize (no player spawn entities on the map?)." );
        BULOG_W( pClient, "Please add some waypoints manually and run the command again." );
        BULOG_W( pClient, "Stopped analyzing waypoints." );
    }

     m_aWaypointsToAnalize = good::vector<TWaypointId>( 1024 );
     m_aWaypointsNeighbours.resize( Size() );
     for ( TWaypointId iWaypoint = 0; iWaypoint < Size(); ++iWaypoint )
         m_aWaypointsToAnalize.push_back( iWaypoint );

     m_iAnalizeStep = EAnalizeStepNeighbours;
}

void CWaypoints::StopAnalizing()
{
    m_aWaypointsToAnalize = good::vector<TWaypointId>();
    m_aWaypointsNeighbours = good::vector<CNeighbour>();
    m_fAnalizeWaypointsForNextFrame = 0;
    m_iAnalizeStep = EAnalizeStepTotal;
    m_pAnalizer = NULL;
}

void CWaypoints::AnalizeStep()
{
    GoodAssert( m_iAnalizeStep < EAnalizeStepTotal );
    
    // Search for analizer in players, else remove him. Can't rely on CPlayers::GetIndex() because the server is reusing edicts.
    if ( m_pAnalizer )
    {
        bool bFound = false;
        for ( TPlayerIndex iPlayer = 0; iPlayer < CPlayers::Size(); ++iPlayer )
        {
            if ( CPlayers::Get( iPlayer )->GetEdict() == m_pAnalizer )
            {
                bFound = true;
                break;
            }
        }

        if ( !bFound )
            m_pAnalizer = NULL;
    }

    if ( m_iAnalizeStep < EAnalizeStepDeleteOrphans )
    {
        float fPlayerEye = CMod::GetVar( EModVarPlayerEye );
        float fHalfPlayerWidthSqr = Sqr( CMod::GetVar( EModVarPlayerWidth ) / 2 );

        float fAnalizeDistance = CWaypoint::iAnalizeDistance;
        float fAnalizeDistanceExtra = fAnalizeDistance * 1.9f; // To include diagonal, almost but not 2 (Pythagoras).
        float fAnalizeDistanceExtraSqr = Sqr( fAnalizeDistanceExtra * 1.9f );

        // Check more waypoints in inters step, as less traces are required.
        float fToAnalize = m_fAnalizeWaypointsForNextFrame + CWaypoint::fAnalizeWaypointsPerFrame * ( 1 + m_iAnalizeStep * 4 );
        int iToAnalize = (int)fToAnalize;
        m_fAnalizeWaypointsForNextFrame = fToAnalize - iToAnalize;

        for ( int i = 0; i < iToAnalize && m_aWaypointsToAnalize.size() > 0; ++i )
        {
            TWaypointId iWaypoint = m_aWaypointsToAnalize.back();
            m_aWaypointsToAnalize.pop_back();

            Vector vPos = Get( iWaypoint ).vOrigin;
            if ( good::find( m_aWaypointsToOmitAnalize, vPos ) != m_aWaypointsToOmitAnalize.end() )
                continue;

            CNeighbour neighbours = m_aWaypointsNeighbours[ iWaypoint ];
            for ( int x = -1; x <= 1; ++x )
            {
                for ( int y = -1; y <= 1; ++y )
                {
                    if ( ( x == 0 && y == 0 ) || neighbours.a[ x + 1 ][ y + 1 ] )
                        continue;

                    Vector vNew = vPos;

                    if ( m_iAnalizeStep == EAnalizeStepNeighbours )
                    {
                        // First check if there is a waypoint near final position.
                        vNew.x += fAnalizeDistance * x;
                        vNew.y += fAnalizeDistance * y;

                        if ( AnalizeWaypoint( iWaypoint, vPos, vNew, fPlayerEye, fHalfPlayerWidthSqr,
                                              fAnalizeDistance, fAnalizeDistanceExtra, fAnalizeDistanceExtraSqr ) )
                            neighbours.a[ x + 1 ][ y + 1 ] = true; // Position is already occupied or new waypoint is added.
                    }
                    else // m_iAnalizeStep == EAnalizeStepInters
                    {
                        if ( x == 1 && y == 1 ) // Omit (1, 1), as there are no adjacent points up / right to it.
                            continue;

                        if ( m_aWaypointsNeighbours[ iWaypoint ].a[ x + 1 ][ y + 1 ] ) // Convert [-1..1] to [0..2].
                            continue; // Don't use neighbours here, as it can be updated.

                        int incX = x + 1; // Adjacent point on X.
                        if ( incX <= 1 && !( incX == 0 && y == 0 ) && // Omit (0, 0) and (2, y).
                             !m_aWaypointsNeighbours[ iWaypoint ].a[ incX + 1 ][ y + 1 ] )
                        {
                            vNew.x += fAnalizeDistance * ( x + incX / 2.0f ); // Will be -1/2 or 1/2.
                            vNew.y += fAnalizeDistance * y;

                            if ( AnalizeWaypoint( iWaypoint, vPos, vNew, fPlayerEye, fHalfPlayerWidthSqr,
                                                  fAnalizeDistance, fAnalizeDistanceExtra, fAnalizeDistanceExtraSqr ) )
                            {
                                neighbours.a[ x + 1 ][ y + 1 ] = true;
                            }
                        }

                        int incY = y + 1; // Adjacent point on Y.
                        if ( incY <= 1 && !( x == 0 && incY == 0 ) && // Omit (0, 0) and (x, 2).
                             !m_aWaypointsNeighbours[ iWaypoint ].a[ x + 1 ][ incY + 1 ] )
                        {
                            vNew = vPos;
                            vNew.x += fAnalizeDistance * x;
                            vNew.y += fAnalizeDistance * ( y + incY / 2.0f ); // Will be -1/2 or 1/2.

                            if ( AnalizeWaypoint( iWaypoint, vPos, vNew, fPlayerEye, fHalfPlayerWidthSqr,
                                                  fAnalizeDistance, fAnalizeDistanceExtra, fAnalizeDistanceExtraSqr ) )
                            {
                                neighbours.a[ x + 1 ][ y + 1 ] = true;
                            }
                        }
                    }
                }
            } // for x.. y..
            m_aWaypointsNeighbours[ iWaypoint ] = neighbours;
        }
    }
    else
    {
        // Remove waypoints without paths.
        int iOldSize = Size();
        for ( int i = Size() - 1; i >= 0; --i )
            if ( m_cGraph[ i ].neighbours.size() == 0 || 
                 good::find( m_aWaypointsToOmitAnalize, m_cGraph[i].vertex.vOrigin ) != m_aWaypointsToOmitAnalize.end() )
                Remove( i++ );
        BULOG_I( m_pAnalizer, "Removed %d orphan / omitted waypoints.", iOldSize - Size() );

    }

    if ( m_aWaypointsToAnalize.size() == 0 )
    {
        switch ( m_iAnalizeStep )
        {
            case EAnalizeStepNeighbours:
                BULOG_I( m_pAnalizer, "Checking for missing spots." );
                for ( TWaypointId iWaypoint = 0; iWaypoint < Size(); ++iWaypoint )
                    m_aWaypointsToAnalize.push_back( iWaypoint );
                m_bIsAnalizeStepAddedWaypoints = false;
                ++m_iAnalizeStep;
                break;

            case EAnalizeStepInters:
                if ( m_bIsAnalizeStepAddedWaypoints )
                {
                    --m_iAnalizeStep;
                    BULOG_I( m_pAnalizer, "Try to add new waypoints from the added ones." );
                }
                else
                {
                    ++m_iAnalizeStep;
                    BULOG_I( m_pAnalizer, "Extra step: erasing orphans." );
                }
                break;

            case EAnalizeStepDeleteOrphans:
                StopAnalizing(); // Stop analizing, no more waypoints.
                BULOG_W( m_pAnalizer, "Stopped analyzing waypoints." );
                break;

            default:
                GoodAssert( false );
        }
    }
}

bool CWaypoints::AnalizeWaypoint( TWaypointId iWaypoint, Vector& vPos, Vector& vNew, float fPlayerEye, float fHalfPlayerWidthSqr,
                                  float fAnalizeDistance, float fAnalizeDistanceExtra, float fAnalizeDistanceExtraSqr )
{
    static good::vector<TWaypointId> aNearWaypoints( 16 );

    if ( ( CBotrixPlugin::pEngineTrace->GetPointContents( vNew ) & MASK_SOLID_BRUSHONLY ) != 0 )
        return false; // Ignore, if inside some solid brush.

    Vector vGround = CUtil::GetGroundVec( vNew );
    vGround.z += fPlayerEye;

    aNearWaypoints.clear();
    CWaypoints::GetNearestWaypoints( aNearWaypoints, vGround, true, fAnalizeDistance / 1.41f );

    bool bSkip = false, showHelp = CWaypoint::fAnalizeWaypointsPerFrame < ANALIZE_HELP_IF_LESS_WAYPOINTS_PER_FRAME;
    for ( int w = 0; !bSkip && w < aNearWaypoints.size(); ++w )
    {
        int iNear = aNearWaypoints[ w ];
        if ( iNear != iWaypoint && !CWaypoints::HasPath( iWaypoint, iNear ) && !CWaypoints::HasPath( iNear, iWaypoint ) )
            CreatePathsWithAutoFlags( iWaypoint, iNear, false, fAnalizeDistanceExtra, showHelp  );
        
        // If has path, set bSkip to true.
        bSkip |= CWaypoints::HasPath( iWaypoint, iNear ) != NULL;

        // If path is not adding somehow, but the waypoint is really close (half player's width or closer).
        bSkip |= CWaypoints::Get( iNear ).vOrigin.AsVector2D().DistToSqr( vGround.AsVector2D() ) <= fHalfPlayerWidthSqr;
    }

    if ( bSkip )
        return true;

    bool bCrouch = false;
    showHelp = iWaypoint == 34;
    TReach reach = CUtil::GetReachableInfoFromTo( vPos, vGround, bCrouch, 0.0f, fAnalizeDistanceExtraSqr, showHelp );
    if ( reach != EReachFallDamage && reach != EReachNotReachable )
    {
        TWaypointId iNew = CWaypoints::Add( vGround );
        BULOG_T( m_pAnalizer, "Added waypoint %d at (%.0f, %.0f, %.0f)", iNew, vGround.x, vGround.y, vGround.z );

        m_aWaypointsToAnalize.push_back( iNew );
        m_bIsAnalizeStepAddedWaypoints = true;

        TPathFlags iFlags = bCrouch ? FPathCrouch : FPathNone;
        CWaypoints::AddPath( iWaypoint, iNew, 0, iFlags | ( reach == EReachNeedJump ? FPathJump : FPathNone ) );

        bool bDestCrouch = false;
        reach = CUtil::GetReachableInfoFromTo( vGround, vPos, bDestCrouch, 0, fAnalizeDistanceExtraSqr, showHelp );
        if ( reach != EReachFallDamage && reach != EReachNotReachable )
        {
            iFlags = bDestCrouch ? FPathCrouch : FPathNone;
            CWaypoints::AddPath( iNew, iWaypoint, 0, iFlags | ( reach == EReachNeedJump ? FPathJump : FPathNone ) );
        }

        CreateAutoPaths( iNew, bCrouch, fAnalizeDistanceExtra, showHelp );

        m_aWaypointsNeighbours.resize( CWaypoints::Size() );
        return true;
    }

    return false; // No waypoint is added.
}

//----------------------------------------------------------------------------------------------------------------
void CWaypoints::GetPathColor( TPathFlags iFlags, unsigned char& r, unsigned char& g, unsigned char& b )
{
    if (FLAG_SOME_SET(FPathDemo, iFlags) )
    {
        r = 0xFF; g = 0x00; b = 0xFF; // Magenta effect, demo.
    }
    else if (FLAG_SOME_SET(FPathBreak, iFlags) )
    {
        r = 0xFF; g = 0x00; b = 0x00; // Red effect, break.
    }
    else if (FLAG_SOME_SET(FPathSprint, iFlags) )
    {
        r = 0xFF; g = 0xFF; b = 0x00; // Yellow effect, sprint.
    }
    else if (FLAG_SOME_SET(FPathStop, iFlags) )
    {
        r = 0x66; g = 0x66; b = 0x00; // Dark yellow effect, stop.
    }
    else if ( FLAG_SOME_SET(FPathLadder, iFlags) )
    {
        r = 0xFF; g = 0x33; b = 0x00; // Orange effect, ladder.
    }
    else if ( FLAG_ALL_SET_OR_0(FPathJump | FPathCrouch, iFlags) )
    {
        r = 0x00; g = 0x00; b = 0x66; // Dark blue effect, jump + crouch.
    }
    else if (FLAG_SOME_SET(FPathJump, iFlags) )
    {
        r = 0x00; g = 0x00; b = 0xFF; // Light blue effect, jump.
    }
    else if (FLAG_SOME_SET(FPathCrouch, iFlags) )
    {
        r = 0x00; g = 0xFF; b = 0x00; // Green effect, crouch.
    }
    else if ( FLAG_SOME_SET(FPathDoor | FPathElevator, iFlags) )
    {
        r = 0x8A; g = 0x2B; b = 0xE2;  // Violet effect, door.
    }
    else if (FLAG_SOME_SET(FPathTotem, iFlags) )
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
    Vector diff(0, 0, -CMod::GetVar( EModVarPlayerEye )/4);
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
    Vector diff(0, 0, -CMod::GetVar( EModVarPlayerEye )/2);

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
