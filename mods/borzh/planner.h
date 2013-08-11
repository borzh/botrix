#ifndef __BOTRIX_PLANNER_H__
#define __BOTRIX_PLANNER_H__


#include "good/string.h"
#include "good/vector.h"

#include "mods/borzh/types_borzh.h"


/// Planner atomic action.
class CAction
{
public:
	CAction(TBotAction iAction, TPlayerIndex iExecutioner, int iArgument):
		iAction(iAction), iExecutioner(iExecutioner), iArgument(iArgument) {}

	TBotAction iAction;                  ///< Action: move, shoot, push button, climb bot one on another etc.
	TPlayerIndex iExecutioner;           ///< Bot who must perform action.
	TPlayerIndex iHelper;                ///< Bot who helps.
	int iArgument;                       ///< Task argument.
};


class CBot_BorzhMod; // Forward declaration.


/// Class that handles plan creation using FF planning system.
class CPlanner
{
public:
	typedef good::vector<CAction> CPlan; ///< Plan is just secuence of actions.

	///< Return true if planner is currently running.
	static bool IsRunning();

	/// Generate PDDL from bot's beliefs and execute planner with generated pddl.
	static void ExecutePlanner( const CBot_BorzhMod& cBot, const good::vector<TAreaId>& cDesiredPlayersPositions );
	
	/// Return NULL in case of failure. Empty plan if no action is needed.
	static const CPlan* GetPlan();
};


#endif // __BOTRIX_PLANNER_H__
