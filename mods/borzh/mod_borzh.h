#ifndef __BOTRIX_MOD_BORZH_H__
#define __BOTRIX_MOD_BORZH_H__


#include "chat.h"
#include "mod.h"


class CMod_Borzh: public IMod
{
public: // Methods.

	/// Constructor. Initializes events and chat variables.
	CMod_Borzh();

	/// Load chat configuration file.
	virtual void LoadConfig(const good::string& sModName) {}

	/// Called when map is loaded, after waypoints and items has been loaded. TODO: map unload.
	virtual void MapLoaded();


	/// Get waypoint type count.
	virtual int GetWaypointTypeCount() { return 0; }

	/// Get waypoint type names.
	virtual const good::string* GetWaypointTypeNames() { return NULL; }

	/// Get waypoint type colors.
	virtual const int* GetWaypointTypeColors() { return NULL; }


	/// Get waypoint path count.
	virtual int GetWaypointPathCount() { return 0; }

	/// Get waypoints path names.
	virtual const good::string* GetWaypointPathNames() { return NULL; }

	/// Get waypoints path colors.
	virtual const int* GetWaypointPathColors() { return NULL; }


	/// Get bot's objective count.
	virtual int GetObjectivesCount() { return 0; }

	/// Get bot's objective names.
	virtual const good::string* GetObjectiveNames() { return NULL; }


	/// Get chat count.
	virtual int GetChatCount() { return CHATS_COUNT; }

	/// Get chat names.
	virtual const good::string* GetChatNames() { return NULL; }


public: // Static methods.

	/// Get waypoints that are in given area.
	static const good::vector<TWaypointId>& GetWaypointsForArea( TAreaId iArea ) { return m_aAreasWaypoints[iArea]; }

	/// Get doors that are in given area.
	static const good::vector<TEntityIndex>& GetDoorsForArea( TAreaId iArea ) { return m_aAreasDoors[iArea]; }

	/// Get buttons that are in given area.
	static const good::vector<TEntityIndex>& GetButtonsForArea( TAreaId iArea ) { return m_aAreasButtons[iArea]; }

	/// Get waypoint to shoot button.
	static TWaypointId GetWaypointToShootButton( TEntityIndex iButton )
	{
		const good::vector<TWaypointId>& aWaypoints = m_aShootButtonWaypoints[iButton];
		return aWaypoints.at( rand() % aWaypoints.size() );
	}


public: // Members.
	static TChatVariable iVarDoor;
	static TChatVariable iVarDoorStatus;
	static TChatVariable iVarButton;
	static TChatVariable iVarWeapon;
	static TChatVariable iVarArea;
	static TChatVariable iVarPlayer;

	static TChatVariableValue iVarValueDoorStatusOpened;
	static TChatVariableValue iVarValueDoorStatusClosed;

	static TChatVariableValue iVarValueWeaponPhyscannon;
	static TChatVariableValue iVarValueWeaponCrossbow;

protected:
	static const int CHATS_COUNT = 23;
	//static const good::string m_aChats[CHATS_COUNT];

	static good::vector< good::vector<TWaypointId> > m_aAreasWaypoints;       // Waypoints for areas.
	static good::vector< good::vector<TEntityIndex> > m_aAreasDoors;          // Doors for areas.
	static good::vector< good::vector<TEntityIndex> > m_aAreasButtons;        // Buttons for areas.
	static good::vector< good::vector<TWaypointId> > m_aShootButtonWaypoints; // Waypoints to shoot buttons.

};

#endif // __BOTRIX_MOD_BORZH_H__
