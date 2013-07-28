#ifndef __BOTRIX_PLANNER_H__
#define __BOTRIX_PLANNER_H__

#include "good/string.h"

/// Class that handles plan creation for 
class CPlanner
{
public:
	static bool bIsStarted;
	static bool bIsFinished;

protected:
	static const good::string sPath;
//	static void GeneratePddl( const good::string& sProblemPath );
};


#endif // __BOTRIX_PLANNER_H__
