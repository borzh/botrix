#ifndef __BOTRIX_ITEM_H__
#define __BOTRIX_ITEM_H__


#include <good/bitset.h>

#include "defines.h"
#include "types.h"

#include "public/edict.h"


class CClient;
class CPlayer;


//****************************************************************************************************************
/// Class that represents class of entities.
//****************************************************************************************************************
class CEntityClass
{
public:
    /// Constructor.
    CEntityClass(): szEngineName(NULL), iFlags(0), fPickupDistanceSqr(0.0f)  {}

    /// Set class argument.
    void SetArgument( int iArgument ) { SET_2ND_WORD(iArgument, iFlags); }

    /// Get class argument.
    int GetArgument() const { return GET_2ND_WORD(iFlags); }

    /// == operator.
    bool operator==( const CEntityClass& cOther ) const
    {
        return ( szEngineName && (szEngineName == cOther.szEngineName) ) ||
               ( sClassName == cOther.sClassName );
    }

    /// == operator with string.
    bool operator==( const good::string& sOther ) const
    {
        return (sClassName == sOther);
    }

    good::string sClassName;            ///< Entity class name (like "prop_physics_multiplayer" or "item_healthkit").
    const char* szEngineName;           ///< Can compare this string with edict_t::GetClassName() only by pointer. Faster.
    TEntityFlags iFlags;                ///< Entity flags and argument (how much health/armor restore, or bullets gives etc.).
    float fPickupDistanceSqr;           ///< Distance to entity to consider it can be picked up.
};



//****************************************************************************************************************
/// Class that represents entity on a map.
//****************************************************************************************************************
class CEntity
{
public:
    /// Default constructor for array templates.
    CEntity(): pEdict(NULL) {}

    /// Constructor with parameters.
    CEntity( edict_t* pEdict, TEntityFlags iFlags, float fPickupDistanceSqr, const CEntityClass* pItemClass,
             const Vector& vOrigin, TWaypointId iWaypoint ):
        pEdict(pEdict), iFlags(iFlags), fPickupDistanceSqr(fPickupDistanceSqr), iWaypoint(iWaypoint),
        vOrigin(vOrigin), pItemClass(pItemClass), pArguments(NULL) {}

    /// Return true if item was freed, (for example broken, but not respawnable).
    bool IsFree() const { return (pEdict == NULL) || pEdict->IsFree(); }

    /// Return true if item currently is on map (not broken or taken).
    bool IsOnMap() const;

    /// Return true if item is breakable (bot will break it instead of jumping on it or moving it).
    bool IsBreakable() const;

    /// Return true if item can explode (bot will never break it and can throw it, if near).
    bool IsExplosive() const { return FLAG_SOME_SET(FObjectExplosive, iFlags); }

    /// Return true if item can't be picked with physcannon.
    bool IsHeavy() const { return FLAG_SOME_SET(FObjectHeavy, iFlags); }

    /// Return current position of the item.
    const Vector& CurrentPosition() const { return pEdict->GetCollideable()->GetCollisionOrigin(); }

    /// == operator.
    bool operator== ( const CEntity& other ) const { return pEdict == other.pEdict; }

    /// Maximum distance from item to waypoint, to consider that to grab item you need to go to that waypoint.
    static const int iMaxDistToWaypoint = 100;

    edict_t* pEdict;                    ///< Entity's edict.
    TEntityFlags iFlags;                ///< Entity's flags.
    float fPickupDistanceSqr;           ///< Distance to entity to consider it can be picked up.
    TWaypointId iWaypoint;              ///< Entity's nearest waypoint.
    Vector vOrigin;                     ///< Entity's respawn position on map (bots will be looking there).
    const CEntityClass* pItemClass;     ///< Entity's class.
    void* pArguments;                   ///< Entity's arguments. For example for door we have 2 waypoints.
};


//****************************************************************************************************************
/// Class that represents picked item along with pick time.
//****************************************************************************************************************
class CPickedItem
{
public:
    /// Constructor with entity.
    CPickedItem( TEntityType iType, TEntityIndex iIndex, float fRemoveTime = 0.0f ):
        iType(iType), iIndex(iIndex), fRemoveTime(fRemoveTime) {}

    /// == operator.
    bool operator==( const CPickedItem& other ) const { return (other.iType == iType) && (other.iIndex == iIndex); }

    TEntityType iType;    ///< Item's type (health, armor, ammo or weapon).
    TEntityIndex iIndex;  ///< Entity's index in array CItems::Get(iType).
    float fRemoveTime;    ///< Time when item should be removed from array of picked items (0 if shouldn't).
};


//****************************************************************************************************************
/// Class that represents set of items on map (health, armor, boxes, chairs, etc.).
//****************************************************************************************************************
class CItems
{

public:
    /// Get random item clas for given entity type.
    static const CEntityClass* GetRandomItemClass( TEntityType iEntityType )
    {
        BASSERT( !m_aItemClasses[iEntityType].empty(), return NULL );
        int iSize = m_aItemClasses[iEntityType].size();
        return iSize ? &m_aItemClasses[iEntityType][ rand() % iSize ] : NULL;
    }

    /// Get array of items of needed type.
    static const good::vector<CEntity>& GetItems( TEntityType iEntityType ) { return m_aItems[iEntityType]; }

    /// Get items class for entity class name.
    static const CEntityClass* GetItemClass( TEntityType iEntityType, const good::string& sClassName )
    {
        good::vector<CEntityClass>::const_iterator it = good::find(m_aItemClasses[iEntityType], sClassName);
        return ( it == m_aItemClasses[iEntityType].end() ) ? NULL : &*it;
    }

    /// Get nearest item for a class (for example some item_battery or item_suitcharger for armor), skipping picked items in aSkip array.
    static TEntityIndex GetNearestItem( TEntityType iEntityType, const Vector& vOrigin, const good::vector<CPickedItem>& aSkip, const CEntityClass* pClass = NULL );

    /// Return true if at least one entity of this class exists on current map.
    static bool ExistsOnMap( const CEntityClass* pEntityClass ) { return pEntityClass->szEngineName != NULL; }

    /// Set size of entity classes for entity type.
    /** This is done for one time allocation of array of entity classes, because pointers to entity classes will be used
     *  (and we don't want array to be reallocated, as it invalidates pointers). */
    static void SetEntityClassesSizeForType( TEntityType iEntityType, int iSize )
    {
        m_aItemClasses[iEntityType].reserve(iSize);
    }

    /// Add item class (for example item_healthkit for health class).
    static const CEntityClass* AddItemClassFor( TEntityType iEntityType, CEntityClass& cItemClass )
    {
        m_aItemClasses[iEntityType].push_back(cItemClass);
        return &m_aItemClasses[iEntityType].back();
    }

    /// Set object flags for given model.
    static void SetObjectFlagForModel( TEntityFlags iItemFlag, const good::string& sModel )
    {
        m_aObjectFlagsForModels.push_back( good::pair<good::string, TEntityFlags>(sModel, iItemFlag) );
    }

#ifndef SOURCE_ENGINE_2006
    /// Called when entity is allocated. Return true if entity is health/armor/weapon/ammo/object.
    static void Allocated( edict_t* pEdict );

    /// Called when entity is freed.
    static void Freed( const edict_t* pEdict );
#endif

    /// Clear all item classes.
    static void Unload()
    {
        MapUnloaded();
        for ( int iType = 0; iType < EEntityTypeTotal; ++iType )
            m_aItemClasses[iType].clear();
        m_aObjectFlagsForModels.clear();
    }

    /// Clear all loaded entities.
    static void MapUnloaded();

    /// Load entities from current map.
    static void MapLoaded();

    /// Update items. Called when player is connected or picks up weapon (new one will be created to respawn later).
    static void Update();

    /// Check if given door is opened.
    static bool IsDoorOpened( TEntityIndex iDoor );

    /// Draw items for a given client.
    static void Draw( CClient* pClient );

protected:
    /// Get entity type and class given entity name.
    static TEntityType GetEntityType( const char* szClassName, CEntityClass* & pEntityClass,
                                      TEntityType iFrom, TEntityType iTo, bool bFastCmp = false );

    static void CheckNewEntity( edict_t* pEdict );
    static TEntityIndex InsertEntity( int iEntityType, const CEntity& cEntity );
    static void AutoWaypointPathFlagsForEntity( TEntityType iEntityType, TEntityIndex iIndex, CEntity& cEntity );
    static TEntityIndex AddItem( TEntityType iEntityType, edict_t* pEdict, CEntityClass* pItemClass, IServerEntity* pServerEntity );
    static void AddObject( edict_t* pEdict, const CEntityClass* pObjectClass, IServerEntity* pServerEntity );

    friend class CWaypoints; // Give access to WaypointDeleted().
    static void WaypointDeleted( TWaypointId id );

    static good::vector<CEntity> m_aItems[EEntityTypeTotal];            // Array of items.
    static good::vector<CEntityClass> m_aItemClasses[EEntityTypeTotal]; // Array of item classes.
    static TEntityIndex m_iFreeIndex[EEntityTypeTotal];                 // First free entity index.
    static int m_iFreeEntityCount[EEntityTypeTotal];                    // Free entities count. TODO:

    static good::vector<edict_t*> m_aOthers;                            // Array of other entities.

    static TEntityIndex m_iCurrentEntity;                               // Current entity index to check.
    static const int m_iCheckEntitiesPerFrame = 32;

    // This one is to have models specific flags (for example car model with 'heavy' flag, or barrel model with 'explosive' flag).
    static good::vector< good::pair<good::string, TEntityFlags> > m_aObjectFlagsForModels;

    static good::bitset m_aUsedItems; // To know which items are already in m_aItems.

    static bool m_bMapLoaded; // Will be set to true at MapLoaded() and to false at Clear().

#ifndef SOURCE_ENGINE_2006
    static good::vector<edict_t*> m_aNewEntities; // When Allocated() is called, new entity still has no IServerEntity, check at next frame.
#endif
};


#endif // __BOTRIX_ITEM_H__
