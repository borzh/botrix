#ifndef __BOTRIX_TYPES_BORZH_H__
#define __BOTRIX_TYPES_BORZH_H__


#include "types.h"


//****************************************************************************************************************
/// Bot task for BorzhMod.
//****************************************************************************************************************
enum TBotTaskBorzh
{
	EBotTaskInvalid = -1,                        ///< Bot has no tasks.
	EBotTaskWait,                                ///< Wait for timer to expire. Used during/after talk.
	EBotTaskExplore,                             ///< Exploring area.
	EBotTaskFindDomainDifference,                ///< Find difference in domain.
	EBotTaskOpenDoor,                            ///< Find button that opens door.
};
typedef int TBotTaskHL2DM;                       ///< Bot task.


#endif // __BOTRIX_TYPES_BORZH_H__
