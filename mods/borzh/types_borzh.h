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

	EBorzhTaskWaitAnswer,                        ///< Wait for other bot to answer.
	EBorzhTaskWaitBot,                           ///< Wait for other bot to do something. The bot will say "done" phrase when it finishes.
	EBorzhTaskUseWeapon,                         ///< Use weapon on box/button.
	EBorzhTaskPushButton,                        ///< Push a button.
	EBorzhTaskShootButton,                       ///< Shoot a button.

	// Next tasks are tasks that consist of atomic tasks.
	EBorzhTaskExplore,                           ///< Exploring current area.
	EBorzhTaskCheckButton,                       ///< Check which doors opens a button. Argument: index is button, type is bool (pushed or still not).

	EBorzhTaskHelping,                           ///< Helping another player. Argument is player index.

	EBorzhTaskCheckDoors,                        ///< Checking doors. Argument is last pushed button.

	EBorzhTaskCarryBox,                          ///< Start carrying box. Argument is box number.
	EBorzhTaskDropBox,                           ///< Drop box at needed position in an area. Arguments are box number and area number.
	EBorzhTaskClimbBox,                          ///< Climb to a box and then to another area. Arguments are box number and area number.

	EBorzhTaskDoor,                              ///< Try to figure out which button opens a door. Argument is door number.
	EBorzhTaskButton,                            ///< Try to figure out which door opens a button. Argument is button number.

	EBorzhTaskStayButton,                        ///< Check which doors opens this button. Argument is button.

	EBorzhTaskStayDoor,                          ///< Check which button opens this door. Argument is door.
};
typedef int TBorzhTask;                          ///< Bot task.


#endif // __BOTRIX_TYPES_BORZH_H__
