#ifndef __BOTRIX_MOD_BORZH_H__
#define __BOTRIX_MOD_BORZH_H__


#include "chat.h"
#include "mod.h"


class Mod_Borzh: public IMod
{
public: // Methods.

	Mod_Borzh();

	///
	virtual void LoadConfig(const good::string& sModName) {}


	/// Called when map is loaded, after waypoints and items has been loaded.
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
	virtual const good::string* GetWaypointPathColors() { return NULL; }


	/// Get bot's objective count.
	virtual int GetObjectivesCount() { return 0; }

	/// Get bot's objective names.
	virtual const good::string* GetObjectiveNames() { return NULL; }


	/// Get chat count.
	virtual int GetChatCount() { return 0; }

	/// Get chat names.
	virtual const good::string* GetChatNames() { return NULL; }

public: // Members.

	static TChatVariable iVarDoor;
	static TChatVariable iVarDoorStatus;
	static TChatVariable iVarButton;
	static TChatVariable iVarWeapon;
	static TChatVariable iVarArea;

	static TChatVariableValue iVarValueDoorStatusOpened;
	static TChatVariableValue iVarValueDoorStatusClosed;

	static TChatVariableValue iVarValueWeaponPhyscannon;
	static TChatVariableValue iVarValueWeaponCrossbow;

protected:
	static void GeneratePddl( const good::string& sProblemPath );

	static const int CHATS_COUNT = 23;
	static const good::string m_aChats[CHATS_COUNT];
};

#endif // __BOTRIX_MOD_BORZH_H__
