#ifndef __BOTRIX_TYPES_BORZH_H__
#define __BOTRIX_TYPES_BORZH_H__


#include "types.h"


//****************************************************************************************************************
/// Bot task for BorzhMod. Ordered by priority.
//****************************************************************************************************************
enum TBorzhTasks
{
	EBorzhTaskInvalid = -1,                      ///< Bot has no tasks.

	// Next tasks are atomic.
	EBorzhTaskWait,                              ///< Wait for timer to expire. Used during/after talk. Argument is time to wait in msecs.
	EBorzhTaskLook,                              ///< Look at door/button/box.
	EBorzhTaskMove,                              ///< Move to given waypoint.
	EBorzhTaskMoveAndCarry,                      ///< Move to given waypoint carrying a box.
	EBorzhTaskSpeak,                             ///< Speak about something. Argument represents door/button/box/weapon, etc.

	EBorzhTaskWaitAnswer,                        ///< Wait for other player to accept/reject task.
	//EBorzhTaskWaitPlayer,                        ///< Wait for other player to do something. The bot will say "done" phrase when it finishes.
	EBorzhTaskWaitButton,                        ///< Wait for other player to push a button. The other player must say "i will push button now".
	EBorzhTaskUseWeapon,                         ///< Use weapon on box/button.
	EBorzhTaskPushButton,                        ///< Push a button.
	EBorzhTaskShootButton,                       ///< Shoot a button.

	// Next tasks are tasks that consist of several atomic tasks. Ordered by priority: explore has higher priority than
	EBorzhTaskButtonDoorConfig,                  ///< Trying to check door-button configuration.
	EBorzhTaskButtonTry,                         ///< Check which doors opens a button. Argument: index is button, type is bool (already pushed or still not).
	EBorzhTaskCheckingDoors,                     ///< Checking if door status is changed.
	EBorzhTaskExplore,                           ///< Exploring new area.
	EBorzhTaskGoToGoal,                          ///< Performing go to goal task.

	EBorzhTaskCarryBox,                          ///< Start carrying box. Argument is box number.
	EBorzhTaskDropBox,                           ///< Drop box at needed position in an area. Arguments are box number and area number.
	EBorzhTaskClimbBox,                          ///< Climb to a box and then to another area. Arguments are box number and area number.
};
typedef int TBorzhTask;                          ///< Bot task.


//****************************************************************************************************************
/// Bot atomic actions for planner.
//****************************************************************************************************************
enum TBotActions
{
	EBotActionMove = 0,                          ///< Go to given area.
	EBotActionPushButton,                        ///< Push given button.
	EBotActionShootButton,                       ///< Shoot given button.
	EBotActionCarryBox,                          ///< Carry a box.
	EBotActionCarryBoxFar,                       ///< Carry a box from far distance.
	EBotActionDropBox,                           ///< Drop a box.
	EBotActionClimbBox,                          ///< Climb to a box.
	EBotActionClimbBot,                          ///< Climb to a bot.
	EBotActionClimbBoxBot,                       ///< Climb to a box and another bot climbs to the first one.

	EBotActionTotal                              ///< Amount of actions.
};
typedef int TBotAction;                          ///< Bot action.


#endif // __BOTRIX_TYPES_BORZH_H__
