#ifndef __BOTRIX_TYPES_H__
#define __BOTRIX_TYPES_H__


#if defined(DEBUG) || defined(_DEBUG)
#	define DebugMessage(...) CUtil::Message(NULL, __VA_ARGS__)
//#	define DebugPrint(...) CUtil::Message(NULL, __VA_ARGS__)
#else
#	define DebugMessage(...)
#endif

#include "good/string.h"
#include "good/vector.h"
#include "good/utility.h"


//****************************************************************************************************************
/// Supported Source Mods.
//****************************************************************************************************************
enum TModIds
{
	EModId_Invalid = -1,                         ///< Unknown mods will be using HL2DM mod.
	EModId_HL2DM = 0,                            ///< Half Life 2 Deathmatch. Unknown mods will be using this one.
	EModId_CSS,                                  ///< Counter Strike Source.
	EModId_Borzh,                                ///< BorzhMod.

	EModId_Total                                 ///< Total amount of supported mods.
};
typedef int TModId;                              ///< Supported Source Mods.


//****************************************************************************************************************
/// Bot intelligence.
//****************************************************************************************************************
enum TBotIntelligences
{
	EBotFool = 0,                                ///< Inaccurate fire, no hearing, slow reaction to damage.
	EBotStupied,                                 ///< Better fire, inaccurate hearing, reaction to damage.
	EBotNormal,                                  ///< Normail fire, accurate hearing, normal reaction to damage.
	EBotSmart,                                   ///< Precise fire, fast reaction to damage.
	EBotPro,                                     ///< Invincible.

	EBotIntelligenceTotal                       ///< Amount of available intelligences.
};
typedef int TBotIntelligence;                    ///< Bot intelligence.



//****************************************************************************************************************
/// Bot's objectives.
//****************************************************************************************************************
enum TObjectives
{
	EObjectiveUnknown = -1,                      ///< No current objective.
	EObjectiveStop = 0,                          ///< Stop for a certain time.
	EObjectiveFollow,                            ///< Follow team mate.
	EObjectiveKill,                              ///< Kill enemy.
	EObjectiveSitDown,                           ///< Sit down.
	EObjectiveStandUp,                           ///< Stand up.
	EObjectiveJump,                              ///< Leave.
};
typedef int TObjective;                          ///< Bot's objectives.


//****************************************************************************************************************
/// Bot sentencies that bot can say.
//****************************************************************************************************************
enum TBotChats
{
	EBotChatUnknown = -1,                        ///< Unknown talk.

	EBotChatError = 0,                           ///< Can't understand what player said.

	EBotChatGreeting,                            ///< Initiation of conversation.
	EBotChatBye,                                 ///< End conversation.
	EBotChatBusy,                                ///< Bot is busy, cancel conversation.

	EBotChatAffirmative,                         ///< Affirmative/yes.
	EBotChatNegative,                            ///< Negative/no.
	EBotChatAffirm,                              ///< Affirm action request.
	EBotChatNegate,                              ///< Negate action request.

	EBotChatCall,                                ///< Addressing someone (hey %player).
	EBotChatCallResponse,                        ///< Callee answers positively (what do you need?).

	EBotChatHelp,                                ///< Asking for help (can you help me?).

	EBotChatStop,                                ///< Stop moving.
	EBotChatCome,                                ///< Come and stay at current player position.
	EBotChatFollow,                              ///< Follow player.
	EBotChatAttack,                              ///< Attack until new command.
	EBotChatNoKill,                              ///< Stop attacking until new command.
	EBotChatSitDown,                             ///< Crouch.
	EBotChatStandUp,                             ///< Stop crouching.
	EBotChatJump,                                ///< Jump.
	EBotChatLeave,                               ///< Continue playing, stop helping (you can leave now).
	EBotChatDontHurt,                            ///< Stop hurting me!

	EBorzhChatOk,                                ///< Ok.
	EBorzhChatDone,                              ///< Done, finished task.
	EBorzhChatNoMoves,                           ///< No more moves.
	EBorzhChatThink,                             ///< Bot will think (use FF).
	EBorzhChatExplore,                           ///< Explore new area.
	EBorzhChatWeaponFound,                       ///< Bot founds a weapon.

	EBorzhChatDoorFound,                         ///< Bot founds a new door.
	EBorzhChatDoorChange,                        ///< Bot sees a change in door status.
	EBorzhChatDoorNoChange,                      ///< Bot sees no change in door status.

	EBorzhChatSeeButton,                         ///< Bot sees a button.
	EBorzhChatButtonCanPush,                     ///< Bot can press a button.
	EBorzhChatButtonCantPush,                    ///< Bot can't reach this button.

	EBorzhChatButtonWeapon,                      ///< Bot has weapon to shoot button.
	EBorzhChatButtonNoWeapon,                    ///< Bot doesn't have weapon to shoot button.
	
	EBorzhChatDoorTry,                           ///< Bot says to find button that opens given door.
	EBorzhChatButtonTry,                         ///< Bot says to find doors that opens given button.

	EBorzhChatButtonIPush,                       ///< Bot will try to push button.
	EBorzhChatButtonYouPush,                     ///< Bot orders to push button to other bot.
	EBorzhChatButtonIShoot,                      ///< Bot will try to shoot button.
	EBorzhChatButtonYouShoot,                    ///< Bot orders to shoot button to other bot.

	EBorzhChatAreaGo,                            ///< Bot orders to go to some area to other bot.
	EBorzhChatDoorGo,                            ///< Bot orders to go to some door to other bot.
	EBorzhChatButtonGo,                          ///< Bot orders to go to some button to other bot.
	EBorzhChatCantGo,                            ///< Bot can't reach given area.

	EBotChatTotal                                ///< Amount of bot sentences.
};
typedef int TBotChat;                            ///< Bot sentence.



//****************************************************************************************************************
/// Result of executing console command.
//****************************************************************************************************************
enum TCommandResults
{
	ECommandPerformed,                            ///< All okay.
	ECommandNotFound,                             ///< Command not found.
	ECommandRequireAccess,                        ///< User don't have access to command.
	ECommandError,		                          ///< Error occurred.
};
typedef int TCommandResult;


//****************************************************************************************************************
/// Flags to access console commands.
//****************************************************************************************************************
enum TCommandAccessFlag
{
	FCommandAccessInvalid      = -1,             ///< Invalid access flag.
	FCommandAccessNone         = 0,              ///< No access to commands.

	FCommandAccessWaypoint     = 1<<0,           ///< Access to waypoint commands.
	FCommandAccessBot          = 1<<1,           ///< Access to bot's commands.
	FCommandAccessConfig       = 1<<2,           ///< Access to configuration's commands.

	FCommandAccessTotal        = 3,              ///< Amount of access flags.
	FCommandAccessAll          = (1<<3) - 1      ///< Access to all commands.
};
typedef int TCommandAccessFlags;


//****************************************************************************************************************
/// Waypoint flag.
//****************************************************************************************************************
enum TWaypointFlag
{
	FWaypointNone              = 0,              ///< Just reachable position.

	// First byte.
	FWaypointStop              = 1<<0,           ///< You need to stop totally to use waypoint
	FWaypointCamper            = 1<<1,           ///< Camp at spot and wait for enemy. Argument are 2 angles to aim (low and high words).
	FWaypointSniper            = 1<<2,           ///< Sniper spot. Argument are 2 angles to aim (low and high words).
	FWaypointWeapon            = 1<<3,           ///< Weapon is there. Arguments are weapon index/subindex (2nd byte).
	FWaypointAmmo              = 1<<4,           ///< Ammo is there. Arguments are ammo count (1st byte) and weapon (2nd byte).
	FWaypointHealth            = 1<<5,           ///< Health is there. Argument is health amount (3rd byte).
	FWaypointArmor             = 1<<6,           ///< Armor is there. Argument is armor amount (4th byte).
	FWaypointHealthMachine     = 1<<7,           ///< Health machine is there. Arguments are 1 angle (low word) and health amount (3rd byte).

	// Second byte.
	FWaypointArmorMachine      = 1<<8,           ///< Armor machine is there. Arguments are 1 angle (low word) and health amount (3rd byte).
	FWaypointButton            = 1<<9,           ///< A button is there. Arguments are angle (low word)  button index+1 (3rd byte), door index+1 (4th byte).
	FWaypointSeeButton         = 1<<10,          ///< Button is visible. Arguments are angle (low word), button index+1 (3rd byte), door index+1 (4th byte).

	FWaypointTotal             = 11,             ///< Amount of waypoint flags.
	FWaypointAll               = (1<<11) - 1     ///< All flags set.
};
typedef unsigned short TWaypointFlags;           ///< Set of waypoint flags.


//****************************************************************************************************************
/// Waypoint flag.
//****************************************************************************************************************
enum TPathFlag
{
	FPathNone                  = 0,              ///< Just run to reach adjacent waypoint.

	FPathCrouch                = 1<<0,           ///< Crouch to reach adjacent waypoint.
	FPathJump                  = 1<<1,           ///< Jump to reach adjacent waypoint. If crouch flag is set will use jump with duck.
	FPathBreak                 = 1<<2,           ///< Use weapon to break something to reach adjacent waypoint.
	FPathSprint                = 1<<3,           ///< Need to sprint to get to adjacent waypoint. Usefull for long jump.
	FPathLadder                = 1<<4,           ///< Need to use ladder move (IN_FORWARD & IN_BACK) to get to adjacent waypoint.
	FPathStop                  = 1<<5,           ///< Stop at current waypoint to go in straight line to next one. Usefull when risk of falling.
	FPathDamage                = 1<<6,           ///< Need to take damage to get to adjacent waypoint. Argument is damage count.
	FPathFlashlight            = 1<<7,           ///< Need to turn on flashlight.

	FPathDoor                  = 1<<8,           ///< There is a door on the way. Argument is door number (start from 1, 0 - invalid).
	FPathTotem                 = 1<<9,           ///< Need to make ladder of living corpses. Argument is count of players needed (1..).

	FPathTotal                 = 10,             ///< Amount of path flags. Note that FPathDemo not counts.
	FPathAll                   = (1<<10)-1,      ///< All path flags.

	FPathDemo                  = 0x8000,         ///< Flag for use demo to reach adjacent waypoints. Demo number is at lower bits.
};
typedef short TPathFlags;                        ///< Set of waypoint path flags.

enum TInvalidWaypoint
{
	EInvalidWaypointId         = -1              ///< Constant to indicate that waypoint is invalid.
};
typedef int TWaypointId;                         ///< Waypoint ID is index of waypoint in array of waypoints.

enum TInvalidArea
{
	EInvalidAreaId             = 255             ///< Constant to indicate that waypoint's area is invalid.
};
typedef unsigned char TAreaId;                   ///< Waypoints are grouped in areas. This is a type for area id.

//****************************************************************************************************************
/// Flags for draw type of waypoints.
//****************************************************************************************************************
enum TWaypointDrawFlag
{
	FWaypointDrawNone          = 0,              ///< Don't draw waypoints.
	FWaypointDrawLine          = 1<<0,           ///< Draw line.
	FWaypointDrawBeam          = 1<<1,           ///< Draw beam.
	FWaypointDrawBox           = 1<<2,           ///< Draw box.
	FWaypointDrawText          = 1<<3,           ///< Draw text (id, area, etc.).

	FWaypointDrawTotal         = 4,              ///< Amount of draw type flags.
	FWaypointDrawAll           = (1<<4)-1,       ///< Draw all.
};
typedef int TWaypointDrawFlags;                  ///< Set of waypoint draw types.


//****************************************************************************************************************
/// Flags for draw type of waypoints paths.
//****************************************************************************************************************
enum TPathDrawFlag
{
	FPathDrawNone              = 0,              ///< Don't draw paths.
	FPathDrawLine              = 1<<0,           ///< Draw line.
	FPathDrawBeam              = 1<<1,           ///< Draw beam.

	FPathDrawTotal             = 2,              ///< Amount of draw type flags.

	FPathDrawAll               = (1<<2)-1,       ///< Draw beams and lines.
};
typedef int TPathDrawFlags;                      ///< Set of draw types for paths.


//****************************************************************************************************************
/// Event types.
//****************************************************************************************************************
enum TEventType
{
	EEventTypeKeyValues,                         ///< Event is of type KeyValues for receiver IGameEventListener.
	EEventTypeIGameEvent                         ///< Event is of type IGameEvent for receiver IGameEventListener2.
};
typedef enum TEventType TEventType;


//****************************************************************************************************************
/// Items types / object / other entities.
//****************************************************************************************************************
enum TEntityTypes
{
	EEntityTypeHealth = 0,                       ///< Item that restores players health. Can be health machine also.
	EEntityTypeArmor,                            ///< Item that restores players armor. Can be armor machine also.
	EEntityTypeWeapon,                           ///< Weapon.
	EEntityTypeAmmo,                             ///< Ammo for weapon.
	EEntityTypeButton,                           ///< Button.
	EEntityTypeDoor,                             ///< Door.

	EEntityTypeObject,                           ///< Object that can stuck player (or optionally be moved).
	EEntityTypeTotal,                            ///< Amount of item types. Object and other doens't count as bot can't pick them up.

	EOtherEntityType = EEntityTypeTotal,         ///< All other type of entities.
};
typedef int TEntityType;                         ///<  Items types / object / other entities.

enum TEntityTypeFlag
{
	EItemTypeAll = (1<<(EOtherEntityType+1))-1   ///< Flag to draw all items.
};
typedef int TEntityTypeFlags;                    ///< Item type flags (used to define which items to draw).

typedef int TEntityClassIndex;                   ///< Index of entity class in CItems::GetClass().
typedef int TEntityIndex;                        ///< Index of entity in CItems::GetItems().


//****************************************************************************************************************
/// Item types.
//****************************************************************************************************************
enum TItemDrawFlag
{
	EItemDontDraw              = 0,              ///< Don't draw item.
	EItemDrawStats             = 1<<0,           ///< Draw item class name, stats, model.
	EItemDrawBoundBox          = 1<<1,           ///< Draw bound box around item.
	EItemDrawWaypoint          = 1<<2,           ///< Draw line to nearest waypoint.

	EItemDrawTotal             = 3,              ///< Amount of draw type flags.
	EItemDrawAll               = (1<<3)-1,       ///< Draw all.
};
typedef int TItemDrawFlags;                      ///< Item draw flags.


//****************************************************************************************************************
/// Item flags.
//****************************************************************************************************************
enum TEntityFlag
{
	FEntityNone                = 0,              ///< Entity has no flags.
	FItemUse                   = 1<<0,           ///< Press button USE to use entity (like health/armor machines).
	FEntityRespawnable         = 1<<1,           ///< Entity is respawnable.
	FObjectExplosive           = 1<<2,           ///< Entity is explosive.
	FObjectHeavy               = 1<<3,           ///< Can't use physcannon on this object.
	FObjectBox                 = 1<<4,           ///< Can use this entity to jump on.

	FEntityTotal               = 5,              ///< Amount of entity flags.
	FEntityAll                 = (1<<5)-1,       ///< All entity flags (that are configurable at config.ini).

	FTaken                     = 1<<4,           ///< This flag is set for all weapons that belong to some player.
};
typedef int TEntityFlags;                        ///< Entity flags.


//****************************************************************************************************************
/// Weapon types.
//****************************************************************************************************************
enum TWeaponTypes
{
	EWeaponPhysics = 0,                          ///< Physcannon.
	EWeaponManual,                               ///< Knife, stunstick, crowbar.

	// Grenades.
	EWeaponGrenade,                              ///< Explosive grenade.
	EWeaponFlash,                                ///< Flash grenade.
	EWeaponSmoke,                                ///< Smoke grenade.
	EWeaponRemoteDetonation,                     ///< Remote detonation.

	// Ranged.
	EWeaponPistol,                               ///< Desert eagle, usp, etc.
	EWeaponShotgun,                              ///< Shotgun, reload one by one.
	EWeaponRifle,                                ///< Automatic, just press once.
	EWeaponSniper,                               ///< Has zoom. Can also be rifle, automatic.
	EWeaponRpg,                                  ///< Rpg, need to aim while rocket is being propulsed.

	EWeaponTotal                                 ///< Amount of weapon flags.

};
typedef int TWeaponType;                         ///< Weapon type.
typedef int TWeaponId;                           ///< Weapon id.


//****************************************************************************************************************
/// Bot atomic actions.
//****************************************************************************************************************
enum TBotAtomicActions
{
	EActionAim = 0,                              ///< Aim to given vector.
	EActionAimEnemy,                             ///< Maintain aim at given enemy.
	EActionLockAim,                              ///< Don't change the aim.
	EActionMove,                                 ///< Move to given vector.
	EActionMoveWaypoint,                         ///< Move to given waypoint straight, not using waypoint navigator.
	EActionNavigate,                             ///< Navigate to given waypoint.

	EActionStop,                                 ///< Stop.
	EActionDuck,                                 ///< Duck.
	EActionWalk,                                 ///< Walk.
	EActionRun,                                  ///< Run.
	EActionSprint,                               ///< Sprint.
	EActionJump,                                 ///< Jump once.
	EActionJumpWithDuck,                         ///< Jump once with duck.

	EActionSetWeapon,                            ///< Set new given weapon.
	EActionSetBestWeapon,                        ///< Set best weapon.

	EActionAttack,                               ///< Press attack button for given time.
	EActionAttack2,                              ///< Press secondary attack button for given time.
	
	EActionUse,                                  ///< Press USE button (to use buttons, machines).
	EActionFlashlightOn,                         ///< Turn on flashlight.
	EActionFlashlightOff,                        ///< Turn off flashlight.

	EActionWait,                                 ///< Wait for certain condition.
};
typedef int TBotAtomicAction;                    ///< Bot atomic actions.


//****************************************************************************************************************
/// Wait conditions for bot commands. Wait means to delay execution of next command.
//****************************************************************************************************************
enum TWaitConditions
{
	EWaitNone = 0,                               ///< Don't wait.
	EWaitTime,                                   ///< Wait for given amount of time.

	EWaitAim,                                    ///< Wait until aiming is finished.
	EWaitMove,                                   ///< Wait until moving to destination is finished.
	EWaitCloseWaypoint,                          ///< Wait until next waypoint becomes closer than the current one.
	EWaitNextWaypoint,                           ///< Wait until next waypoint is reached.
	EWaitNavigate,                               ///< Wait until destination waypoint is reached.

	EWaitActionStart,                            ///< Wait until action start time.
	EWaitActionEnd,                              ///< Wait until action end time.

	EWaitItemTouch,                              ///< Wait until bot is touching an item.
	EWaitObjectGone,                             ///< Wait until certain object becomes broken or goes far away (not disturbing anymore).
	EWaitEnemyGone,                              ///< Wait until enemy is dead or not visible.
	EWaitMachine,                                ///< Wait until ends using machine.
};
typedef int TWaitCondition;                      ///< Wait conditions for bot commands.


typedef good::vector<good::string> StringVector; ///< Useful typedef.


#endif // __BOTRIX_TYPES_H__
