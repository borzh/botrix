#ifdef BOTRIX_TF2

#ifndef __BOTRIX_TYPES_TF2_H__
#define __BOTRIX_TYPES_TF2_H__


#include "types.h"


typedef int TBotClass;                           ///< Bot class.


//****************************************************************************************************************
/// Bot task for tf2.
//****************************************************************************************************************
enum TBotTaskTF2Id
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
typedef int TBotTaskTF2;                         ///< Bot task.


#endif // __BOTRIX_TYPES_TF2_H__

#endif // BOTRIX_TF2
