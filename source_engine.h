#ifndef __BOTRIX_UTIL_H__
#define __BOTRIX_UTIL_H__


#include "public/edict.h"
#include "public/eiface.h"
#include "public/engine/IEngineTrace.h"

#include "types.h"

#include "server_plugin.h"


/// Useful enum to know if one map position can be reached from another one.
enum TReachs
{
	EReachNotReachable = 0,                 ///< Something blocks our way.
	EReachReachable,                        ///< Path free.
	EReachNeedJump,                         ///< Need to jump to get there.
	//EReachNeedCrouchJump,                   ///< Not used, will always jump crouched.
	//EReachNeedCrouch,                       ///< Not used, will check player's height instead.
	EReachFallDamage,                       ///< Need to take fall damage to reach destination.
};
typedef int TReach;


/// Enum for flags used in CUtil::IsVisible() function.
enum TVisibilityFlag
{
	FVisibilityWorld    = 1<<0,             ///< Visibility includes world.
	FVisibilityProps    = 1<<1,             ///< Visibility includes props.
	FVisibilitySolid    = (1<<2)-1,         ///< Visibility includes world and props.

	FVisibilityEntity   = 1<<2,             ///< Visibility includes entities.
	FVisibilityAll      = (1<<3)-1,         ///< Visibility includes world, props and entities.
};
typedef int TVisibilityFlags;               ///< Flags for CUtil::IsVisible() function.


//****************************************************************************************************************
/// Usefull class to ease engine interraction.
//****************************************************************************************************************
class CUtil
{

public:
	/// First make folders in file path, then open that file.
	static FILE* OpenFile( const good::string& szFile, char *szMode );

	/// Util function to get path inside Botrix folder.
	static const good::string& BuildFileName( const good::string& sFolder, const good::string& sFile, const good::string& sExtension );

	/// Print a message to given client. If message tag is used then message will start with "[Botrix] ".
	static void Message( edict_t* pEntity, const char* fmt, ... );	

	/// If message tag is used then all client messages using Message() will start with "[Botrix] ".
	static void SetMessageUseTag( bool bUseTag ) { m_bMessageUseTag = bUseTag; }

	/// Return true if given entity is server networkable.
	static bool IsNetworkable( edict_t* pEntity );
	
	/// Get entity head.
	static void EntityHead( edict_t* pEntity, Vector& v ) { CBotrixPlugin::pServerGameClients->ClientEarPosition(pEntity, &v); }
	/// Get entity center.
	static void EntityCenter( edict_t* pEntity, Vector& v );
	
	/// Get player's entity by user ID.
	static edict_t* GetEntityByUserId( int iUserId );

	/// Set internal PVS (potentially visible set of clusters) for given vector vFrom.
	static void SetPVSForVector( const Vector& vFrom );
	/// Check if point v is in potentially visible set.
	static bool IsVisiblePVS( const Vector& v );

	/// Return true if vDest is visible from vSrc.
	static bool IsVisible( Vector const& vSrc, Vector const& vDest, TVisibilityFlags iFlags = FVisibilityWorld );
	/// Return true if entity is visible from vSrc.
	static bool IsVisible( Vector const& vSrc, edict_t* pDest );
	/// Return true if can get from vSrc to vDest walking or jumping.
	static TReach GetReachableInfoFromTo( Vector const& vSrc, Vector const& vDest, float fDistance = 0.0f );

	/// Trace line to know if hit any world object.
	static void TraceLine( Vector const& vSrc, Vector const& vDest, int mask, ITraceFilter *pFilter );
	/// Return result of TraceLine().
	static trace_t const& TraceResult() { return m_TraceResult; }
	/// Return true if TraceLine() hit something.
	static bool IsTraceHitSomething() { return m_TraceResult.fraction < 1.0f; }

	/// Util function to set angle to be [0..+360).
	static void NormalizeAngle( float& fAngle )
	{
		if (fAngle < 0.0f)
			fAngle += 360.0f;
		else if (fAngle >= 360.0f)
			fAngle -= 360.0f;
	}

	/// Util function to set angle to be [-180..+180).
	static void DeNormalizeAngle( float& fAngle )
	{
		if (fAngle < -180.0f)
			fAngle += 360.0f;
		else if (fAngle >= 180.0f)
			fAngle -= 360.0f;
	}

	/// Util function to get pitch/yaw angle difference forcing it to be [-180..+180).
	static void GetAngleDifference(QAngle const& angOrigin, QAngle const& angDestination, QAngle& angDiff)
	{
		angDiff = angDestination - angOrigin;

		DeNormalizeAngle(angDiff.x);
		DeNormalizeAngle(angDiff.y);
	}
	
	static bool IsTouchBoundingBox2d( const Vector2D &a1, const Vector2D &a2, const Vector2D &bmins, const Vector2D &bmaxs );
	static bool IsOnOppositeSides2d( const Vector2D &amins, const Vector2D &amaxs, const Vector2D &bmins, const Vector2D &bmaxs );
	static bool IsLineTouch2d( const Vector2D &amins, const Vector2D &amaxs, const Vector2D &bmins, const Vector2D &bmaxs );
	static bool IsTouchBoundingBox3d( Vector const& a1, Vector const& a2, Vector const& bmins, Vector const& bmaxs );
	static bool IsOnOppositeSides3d( Vector const& amins, Vector const& amaxs, Vector const& bmins, Vector const& bmaxs );
	static bool IsLineTouch3d( Vector const& amins, Vector const& amaxs, Vector const& bmins, Vector const& bmaxs );
	
	/// Useful math round function.
	static int Round( float fNum )
	{
		int iNum = (int)fNum;
		return (fNum - iNum >= 0.5f) ? iNum + 1 : iNum;
	}

	/// Util function that defines when one point 'touches' another one.
	static bool IsPointTouch3d( Vector const& v1, Vector const& v2, int iSqrDiffZ = iPointTouchSquaredZ, int iSqrDiffXY = iPointTouchSquaredXY )
	{
		//return v2.DistToSqr(v1) < 40*40;
		float zDiff = v1.z - v2.z;
		if ( SQR(zDiff) >= iSqrDiffZ ) // First check if Z difference is too big.
			return false;

		Vector2D vDiff(v1.x - v2.x, v1.y - v2.y);
		return ( vDiff.LengthSqr() <= iSqrDiffXY );
	}
	

public: // Draw functions.

	/// Draw beam.
	static void DrawBeam( const Vector& v1, const Vector& v2, unsigned char iWidth, float fDrawTime,
	                      unsigned char r, unsigned char g, unsigned char b );

	/// Draw line.
	static void DrawLine( const Vector& v1, const Vector& v2, float fDrawTime,
	                      unsigned char r, unsigned char g, unsigned char b );

	/// Draw box.
	static void DrawBox( const Vector& vOrigin, const Vector& vMins, const Vector& vMaxs, float fDrawTime,
	                     unsigned char r, unsigned char g, unsigned char b, const QAngle& ang = angZero );

	/// Draw text with white color (r, g, b arguments are not used).
	static void DrawText( const Vector& vOrigin, int iLine, float fDrawTime,
	                      unsigned char r, unsigned char g, unsigned char b, const char* szText );


public: // Members.
	static const int MAX_MAP_SIZE = 32768;   ///< This is max map size for HL2 (-16384..16383).
	static const int HALF_MAX_MAP_SIZE = MAX_MAP_SIZE/2;

	// Mod dependant variables that should be set at plugin load.
	// https://developer.valvesoftware.com/wiki/Dimensions
	static int iPlayerHeight;                ///< Player's height. 72 by default (HL2 defines).
	static int iPlayerHeightCrouched;        ///< Player's height while crouching. 36 by default.
	static int iPlayerWidth;                 ///< Player's width & length. 32 by default.
	static Vector vPlayerCollisionHull;      ///< Maxs of player collision box with origin in (0, 0, 0).

	static int iPlayerEyeLevel;              ///< Player's eye position. 64 by default.
	static int iPlayerEyeLevelCrouched;      ///< Player's eye position crouched. 28 by default.

	static int iPlayerMaxObstacleHeight;     ///< Max obstacle height that a player can walk over (18).
	static int iPlayerNormalJumpHeight;      ///< Z distance that a player can jump without crouching (20).
	static int iPlayerJumpCrouchHeight;      ///< Z distance that a player can jump with crouching (56).

	static int iPlayerMaxHeightNoFallDamage; ///< Max height to not take any damage if fall (185).
	
	static int iPlayerRadius;                ///< Player's radius (used to check if bot is stucked).
	static int iNearItemMaxDistanceSqr;      ///< Max distance to consider item to be near to player.
	static int iItemPickUpDistance;          ///< Additional distance from player to item to consider it taken.
	                                         // Item is picked, if distance-to-player < player's-radius + item's-radius + this-distance.

	/// This is the maximum slope angle, in degrees from the horizontal, that the player can scale (45 degrees).
	static int iPlayerMaxSlopeGradient;

	static int iPlayerMaxArmor;              ///< Maximum amount of armor, that player can have (100 by default).
	static int iPlayerMaxHealth;             ///< Maximum amount of health, that player can have (100 by default).

	static float fMinNonStuckSpeed;          ///< Minimum velocity to consider that bot is moving and non stucked.
	static float fMaxCrouchVelocity;         ///< Maximum velocity while crouching.
	static float fMaxWalkVelocity;           ///< Maximum velocity while walking.
	static float fMaxRunVelocity;            ///< Maximum velocity while running.
	static float fMaxSprintVelocity;         ///< Maximum velocity while sprinting.

	static int iPointTouchSquaredXY;         ///< Squared distance to consider that we are touching waypoint.
	static int iPointTouchSquaredZ;          ///< Z distance to consider that we are touching waypoint. Should be no more than player can jump.
	static int iPointTouchLadderSquaredZ;    ///< Z distance to consider that we are touching waypoint while on ladder.

	static const Vector vZero;               ///< Zero vector.
	static const QAngle angZero;             ///< Zero angle.

protected:
	static bool m_bMessageUseTag;
	static trace_t m_TraceResult;
};

#endif // __BOTRIX_UTIL_H__
