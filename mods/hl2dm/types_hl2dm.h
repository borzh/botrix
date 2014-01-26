#ifdef BOTRIX_HL2DM_MOD

#ifndef __BOTRIX_TYPES_HL2DM_H__
#define __BOTRIX_TYPES_HL2DM_H__


#include "types.h"


//****************************************************************************************************************
/// Bot task for hl2dm.	Note that objective has more priority than task.
//****************************************************************************************************************
enum TBotTasksHL2DM
{
	EBotTaskInvalid = -1,                        ///< Bot has no tasks.
	EBotTaskFindHealth = 0,                      ///< Find health.
	EBotTaskFindArmor,                           ///< Find armor.
	EBotTaskFindWeapon,                          ///< Find weapon.
	EBotTaskFindAmmo,                            ///< Find ammo for weapon.
	EBotTaskChaseEnemy,                          ///< Run after fleeing enemy (if health is bigger than enemy health).
	EBotTaskFindEnemy,                           ///< Run randomly around and check if can see an enemy.
	EBotTasksTotal,                              ///< This should be first task number of enums of mod tasks.
};
typedef int TBotTaskHL2DM;                       ///< Bot task.


#endif // __BOTRIX_TYPES_HL2DM_H__

#endif // BOTRIX_HL2DM_MOD
