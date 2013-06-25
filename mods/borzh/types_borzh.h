#ifndef __BOTRIX_TYPES_BORZH_H__
#define __BOTRIX_TYPES_BORZH_H__


#include "types.h"


//****************************************************************************************************************
/// Bot task for BorzhMod. Ordered by priority.
//****************************************************************************************************************
enum TBorzhTasks
{
	EBorzhTaskInvalid = -1,                      ///< Bot has no tasks.
	EBorzhTaskWait,                              ///< Wait for timer to expire. Used during/after talk. Argument is time to wait.
	EBorzhTaskExplore,                           ///< Exploring unknown area. Argument is area number.
	EBorzhTaskDoor,                              ///< Try to figure out which button opens a door. Argument is door number.
	EBorzhTaskButton,                            ///< Try to figure out which door opens a button. Argument is button number.

	EBorzhTaskPushUnknownButton,                 ///< Pushing button for first time.
	EBorzhTaskPushButton,                        ///< Pushing button.

	EBorzhTaskWaitBot,                           ///< Wait for other bot to investigate area.
	EBorzhTaskStayButton,                        ///< Check which doors opens this button. Argument is button.
	EBorzhTaskCheckDoors,                        ///< Check which doors opens this button. Argument is door.

	EBorzhTaskStayDoor,                          ///< Check which button opens this door. Argument is door.
	EBorzhTaskCheckButtons,                      ///< Check which button opens this door. Argument is door.
};
typedef int TBorzhTask;                          ///< Bot task.


#endif // __BOTRIX_TYPES_BORZH_H__
