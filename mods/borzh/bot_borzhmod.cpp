
#include "bot_borzhmod.h"
#include "client.h"
#include "type2string.h"


//----------------------------------------------------------------------------------------------------------------
CBot_BorzhMod::CBot_BorzhMod( edict_t* pEdict, TPlayerIndex iIndex, TBotIntelligence iIntelligence ):
	CBot(pEdict, iIndex, iIntelligence)
{
	CBotrixPlugin::pEngineServer->SetFakeClientConVarValue(pEdict, "cl_autowepswitch", "0");	
	CBotrixPlugin::pEngineServer->SetFakeClientConVarValue(pEdict, "cl_defaultweapon", "weapon_crowbar");	
	m_bShootAtHead = false;
}
		
//----------------------------------------------------------------------------------------------------------------
void CBot_BorzhMod::Activated()
{
	CBot::Activated();

	// Initialize doors and buttons.
	m_cSeenDoors.resize( CItems::GetItems(EEntityTypeDoor).size() );
	m_cSeenButtons.resize( CItems::GetItems(EEntityTypeButton).size() );

	m_cOpenedDoors.resize( m_cSeenDoors.size() );
	m_cDoorToggle.resize(m_cSeenButtons.size()  );
	m_cDoorNoAffect.resize( m_cSeenButtons.size() );
	for ( int i=0; i < m_cDoorToggle.size(); ++i )
	{
		m_cDoorToggle[i].resize( m_cSeenDoors.size() );
		m_cDoorNoAffect[i].resize( m_cSeenDoors.size() );
	}

	m_aUnknownWaypoints.resize( CWaypoints::Size() );
	//m_aUnknownAreas.resize();

	
}

//----------------------------------------------------------------------------------------------------------------
void CBot_BorzhMod::Respawned()
{
	CBot::Respawned();

	m_bDontAttack = true;

	m_iCurrentTask = EBorzhTaskInvalid;
	m_bNeedTaskCheck = true;

	// Say hello to some other player.
}

//----------------------------------------------------------------------------------------------------------------
void CBot_BorzhMod::HurtBy( int iPlayerIndex, CPlayer* pAttacker, int iHealthNow )
{
	// TODO: say hey stop hurting me!
}

//----------------------------------------------------------------------------------------------------------------
void CBot_BorzhMod::ReceiveChatRequest( const CBotChat& cRequest )
{
	/*switch (cRequest.iBotRequest)
	{
	case EBorzhBotChatFoundDoor:
	case EBorzhBotChatFoundButton:
	case EBorzhBotChatFoundNewArea:
		break;
	}*/
}

//----------------------------------------------------------------------------------------------------------------
void CBot_BorzhMod::Think()
{
	if ( !m_bAlive )
	{
		m_cCmd.buttons = rand() & IN_ATTACK; // Force bot to respawn by hitting randomly attack button.
		return;
	}

	TaskUpdate();

	if ( m_bTaskFinished )
	{
		m_bTaskFinished = false;

		// Get new task from stack.
		if ( m_cTaskStack.size() > 0 )
		{
			task_t task = m_cTaskStack.back();
			m_cTaskStack.erase( m_cTaskStack.size() - 1 );

			m_iCurrentTask = task.first;
			m_iTaskArgument = task.second;

			m_bNeedTaskCheck = true;
		}
	}

	if ( m_bNeedTaskCheck )
	{
		m_bNeedTaskCheck = false;
		TaskCheck();
	}

	switch (m_iCurrentTask)
	{
	case EBorzhTaskWait:
		if ( CBotrixPlugin::fTime >= fWaitTime )
			m_bNeedTaskCheck = true;
		break;

	case EBorzhTaskExplore:
		break;
	}
}


//----------------------------------------------------------------------------------------------------------------
void CBot_BorzhMod::CurrentWaypointJustChanged()
{
	CBot::CurrentWaypointJustChanged();

	// Check new waypoint neighbours.
	CWaypoints::WaypointNode& cNode = CWaypoints::GetNode(iCurrentWaypoint);
	CWaypoints::WaypointNode::arcs_t& cNeighbours = cNode.neighbours;
	
	for ( int i=0; i < cNeighbours.size(); ++i)
	{
		CWaypoints::WaypointArc& cArc = cNeighbours[i];
		TWaypointId iWaypoint = cArc.target;
		if ( iWaypoint != iPrevWaypoint ) // Bot is not coming from there.
		{
			CWaypointPath& cPath = cArc.edge;
			if ( FLAG_SOME_SET(FPathDoor, cPath.iFlags) ) // Waypoint path is passing through a door.
			{
				TEntityIndex iDoor = cPath.iArgument;
				bool bOpened = CItems::IsDoorOpened(iDoor);
				if ( !m_cSeenDoors.test(iDoor) )
				{
					m_cSeenDoors.set(iDoor);
					m_cOpenedDoors.set(iDoor, bOpened);
					//Speak(EBorzhChatDoorFound, iDoor, bOpened);
				}
				else if ( m_cOpenedDoors.test(iDoor) != bOpened ) // Bot belief of opened/closed door is different.
				{
					m_cOpenedDoors.set(iDoor, bOpened);
					//Speak(EBorzhChatDoorChange, iDoor, bOpened);
				}
			}
		}
	}
}

//----------------------------------------------------------------------------------------------------------------
bool CBot_BorzhMod::DoWaypointAction()
{
	return CBot::DoWaypointAction();
}

//----------------------------------------------------------------------------------------------------------------
void CBot_BorzhMod::ApplyPathFlags()
{
	CBot::ApplyPathFlags();
}

//----------------------------------------------------------------------------------------------------------------
void CBot_BorzhMod::DoPathAction()
{
	CBot::DoPathAction();
}

//----------------------------------------------------------------------------------------------------------------
void CBot_BorzhMod::PickItem( const CEntity& cItem, TEntityType iEntityType, TEntityIndex iIndex )
{
	CBot::PickItem( cItem, iEntityType, iIndex );
	ConsoleCommand("say Hey, I just found %s %s.", CTypeToString::EntityTypeToString(iEntityType), cItem.pItemClass->sClassName.c_str());
}

//----------------------------------------------------------------------------------------------------------------
/*void CBot_BorzhMod::Speak( int iDirectedTo, TBotChat iChat, TEntityIndex iEntity, TChatVariableValue iValue )
{
	CBotChat cChat(iChat, m_iIndex, iDirectedTo, -1);
	if ( cResponse.iBotRequest != -1 )
	{
		const good::string& sText = CChat::ChatToText(cResponse);
		ConsoleCommand( "say %s", sText.c_str() );
	}
}

//----------------------------------------------------------------------------------------------------------------
void CBot_BorzhMod::ReceiveSpeak( TBotChat iChat, TEntityIndex iEntity, TBorzhChatArgument iArgument )
{
}
*/

//----------------------------------------------------------------------------------------------------------------
void CBot_BorzhMod::TaskUpdate()
{
	switch (m_iCurrentTask)
	{
	case EBorzhTaskWait:
		m_bTaskFinished = (CBotrixPlugin::fTime >= fWaitTime);
		break;

	case EBorzhTaskWaitBot: // When other bot will talk, then will end this task.
		break;
	}

}

//----------------------------------------------------------------------------------------------------------------
void CBot_BorzhMod::TaskChange( TBorzhTask iTask, int iTaskArgument )
{
	if ( m_iCurrentTask != EBorzhTaskInvalid )
		m_cTaskStack.push_back( task_t(m_iCurrentTask, m_iTaskArgument) );
	m_iCurrentTask = iTask;
	m_iTaskArgument = iTaskArgument;
	switch (iTask)
	{
	case EBorzhTaskWait:
		fWaitTime = CBotrixPlugin::fTime + iTaskArgument;
		break;
	}
}

//----------------------------------------------------------------------------------------------------------------
void CBot_BorzhMod::TaskCheck()
{
}
