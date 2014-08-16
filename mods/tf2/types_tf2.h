#ifdef BOTRIX_TF2

#ifndef __BOTRIX_TYPES_TF2_H__
#define __BOTRIX_TYPES_TF2_H__


#include "types.h"



//****************************************************************************************************************
/// Bot task for tf2.
//****************************************************************************************************************
enum TBotTaskTf2Id
{
    EBotTaskTf2Invalid = -1,                     ///< Bot has no tasks.
    EBotTaskTf2FindHealth = 0,                   ///< Find health.
    EBotTaskTf2FindArmor,                        ///< Find armor.
    EBotTaskTf2FindWeapon,                       ///< Find weapon.
    EBotTaskTf2FindAmmo,                         ///< Find ammo for weapon.
    EBotTaskTf2EngageEnemy,                       ///< Run after fleeing enemy (if health is bigger than enemy health).
    EBotTaskTf2FindEnemy,                        ///< Run randomly around and check if can see an enemy.
    EBotTaskTf2sTotal,                           ///< This should be first task number of enums of mod tasks.
};
typedef int TBotTaskTf2;                         ///< Bot task.


#endif // __BOTRIX_TYPES_TF2_H__

#endif // BOTRIX_TF2
