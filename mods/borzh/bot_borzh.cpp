#include "chat.h"
#include "clients.h"
#include "type2string.h"

#include "mods/borzh/bot_borzh.h"
#include "mods/borzh/mod_borzh.h"


#define GET_TYPE(arg)                GET_1ST_BYTE(arg)
#define SET_TYPE(type, arg)          SET_1ST_BYTE(type, arg)

#define GET_INDEX(arg)               GET_2ND_BYTE(arg)
#define SET_INDEX(index, arg)        SET_2ND_BYTE(index, arg)

#define GET_AUX1(arg)                GET_3RD_BYTE(arg)
#define SET_AUX1(aux, arg)           SET_3RD_BYTE(aux, arg)

#define GET_AUX2(arg)                GET_4TH_BYTE(arg)
#define SET_AUX2(aux, arg)           SET_4TH_BYTE(aux, arg)


//----------------------------------------------------------------------------------------------------------------
bool CBot_BorzhMod::m_bUsingPlanner = false; // Set if some of the bots is using planner.


//----------------------------------------------------------------------------------------------------------------
CBot_BorzhMod::CBot_BorzhMod( edict_t* pEdict, TPlayerIndex iIndex, TBotIntelligence iIntelligence ):
	CBot(pEdict, iIndex, iIntelligence), m_bHasCrossbow(false), m_bHasPhyscannon(false), m_bStarted(false)
{
	CBotrixPlugin::pEngineServer->SetFakeClientConVarValue(pEdict, "cl_autowepswitch", "0");	
	CBotrixPlugin::pEngineServer->SetFakeClientConVarValue(pEdict, "cl_defaultweapon", "weapon_crowbar");	
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

	m_aVisitedWaypoints.resize( CWaypoints::Size() );
	m_aVisitedAreas.resize( CWaypoints::GetAreas().size() );
	m_aCheckedAreas.resize( CWaypoints::GetAreas().size() );
}

//----------------------------------------------------------------------------------------------------------------
void CBot_BorzhMod::Respawned()
{
	CBot::Respawned();

	m_bDontAttack = true;

	if ( iCurrentWaypoint != -1 )
		m_aVisitedWaypoints.set(iCurrentWaypoint);

	m_bNewTask = true;
	m_cCurrentTask.iTask = EBorzhTaskExplore;

	if ( m_bFirstRespawn )
	{
		// Say hello to some other player.
	}
}

//----------------------------------------------------------------------------------------------------------------
void CBot_BorzhMod::HurtBy( int iPlayerIndex, CPlayer* pAttacker, int iHealthNow )
{
	m_cChat.iBotRequest = EBotChatDontHurt;
	m_cChat.iDirectedTo = iPlayerIndex;
	m_cChat.cMap.clear();

	CBot::Speak(false);
}

//----------------------------------------------------------------------------------------------------------------
void CBot_BorzhMod::ReceiveChat( int iPlayerIndex, CPlayer* pPlayer, bool bTeamOnly, const char* szText )
{
	if ( m_bStarted )
		return;

	good::string sText(szText, true, true);
	sText.lower_case();

	if ( (sText == "start") || (sText == "begin") )
	{
		DebugAssert( !pPlayer->IsBot() );
		CClient* pClient = (CClient*)pPlayer;
		if ( FLAG_ALL_SET(FCommandAccessBot, pClient->iCommandAccessFlags) )
			m_bStarted = true;
	}
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

	if ( !m_bStarted )
		return;

	if ( m_bNewTask )
		InitNewTask();

	// Update current task.
	if ( !m_bTaskFinished )
	{
		switch (m_cCurrentTask.iTask)
		{
		case EBorzhTaskWait:
			if ( CBotrixPlugin::fTime >= m_fEndWaitTime )
				m_bTaskFinished = true;
			break;

		case EBorzhTaskLook:
			if ( !m_bNeedAim )
				m_bTaskFinished = true;
			break;

		case EBorzhTaskMove:
			if ( !m_bNeedMove )
				m_bTaskFinished = true;
			break;

		case EBorzhTaskSpeak:
			m_bTaskFinished = true;
			break;

		//case EBorzhTaskWaitBot:
		}
	}

	if ( m_bTaskFinished )
		TaskPop();
}


//----------------------------------------------------------------------------------------------------------------
void CBot_BorzhMod::CurrentWaypointJustChanged()
{
	CBot::CurrentWaypointJustChanged();
	
	m_aVisitedWaypoints.set(iCurrentWaypoint);
	bool bNearDoor = false;

	// Get waypoint node.
	const CWaypoints::WaypointNode& cNode = CWaypoints::GetNode(iCurrentWaypoint);
	m_iCurrentArea = cNode.vertex.iAreaId;

	// Check new waypoint neighbours.
	const CWaypoints::WaypointNode::arcs_t& cNeighbours = cNode.neighbours;
	for ( int i=0; i < cNeighbours.size(); ++i)
	{
		const CWaypoints::WaypointArc& cArc = cNeighbours[i];
		TWaypointId iWaypoint = cArc.target;
		if ( iWaypoint != iPrevWaypoint ) // Bot is not coming from there.
		{
			const CWaypointPath& cPath = cArc.edge;
			if ( FLAG_SOME_SET(FPathDoor, cPath.iFlags) ) // Waypoint path is passing through a door.
			{
				bNearDoor = true;

				TEntityIndex iDoor = cPath.iArgument;
				if ( iDoor > 0 )
				{
					--iDoor;
					bool bOpened = CItems::IsDoorOpened(iDoor);
					TChatVariableValue iDoorStatus = bOpened ? CMod_Borzh::iVarValueDoorStatusOpened : CMod_Borzh::iVarValueDoorStatusClosed;

					if ( !m_cSeenDoors.test(iDoor) ) // Bot sees door for the first time.
					{
						m_cSeenDoors.set(iDoor);
						m_cOpenedDoors.set(iDoor, bOpened);
						PushSpeakTask(EBorzhChatDoorFound, EEntityTypeDoor, iDoor, iDoorStatus);
					}
					else if ( m_cOpenedDoors.test(iDoor) != bOpened ) // Bot belief of opened/closed door is different.
					{
						m_cOpenedDoors.set(iDoor, bOpened);
						PushSpeakTask(EBorzhChatDoorChange, EEntityTypeDoor, iDoor, iDoorStatus);
					}
				}
				else
					CUtil::Message(NULL, "Error, waypoint path from %d to %d has invalid door index.", iCurrentWaypoint, iWaypoint);
			}
		}
	}

	// Check if bot saw the button before.
	if ( FLAG_SOME_SET(FWaypointButton, cNode.vertex.iFlags) )
	{
		TEntityIndex iButton = CWaypoint::GetButton(cNode.vertex.iArgument);
		if (iButton > 0)
		{
			--iButton;
			if ( !m_cSeenButtons.test(iButton) ) // Bot sees button for the first time.
				PushSpeakTask( EBorzhChatSeeButton, EEntityTypeButton, iButton );
		}
		else
			CUtil::Message(NULL, "Error, waypoint %d has invalid button index.", iCurrentWaypoint);
	}
}

//----------------------------------------------------------------------------------------------------------------
bool CBot_BorzhMod::DoWaypointAction()
{
	const CWaypoint& cWaypoint = CWaypoints::Get(iCurrentWaypoint);

	// Check if bot saw the button before.
	if ( FLAG_SOME_SET(FWaypointSeeButton, cWaypoint.iFlags) )
	{
		TEntityIndex iButton = CWaypoint::GetButton(cWaypoint.iArgument);
		if (iButton > 0)
		{
			--iButton;
			if ( !m_cSeenButtons.test(iButton) ) // Bot sees button for the first time.
			{
				if ( m_bHasCrossbow )
					PushSpeakTask( EBorzhChatButtonWeapon, EEntityTypeButton, iButton );
				else
					PushSpeakTask( EBorzhChatButtonNoWeapon, EEntityTypeButton, iButton );
				PushSpeakTask( EBorzhChatSeeButton, EEntityTypeButton, iButton );
			}
		}
		else
			CUtil::Message(NULL, "Error, waypoint %d has invalid button index.", iCurrentWaypoint);
	}

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
	//switch( iEntityType )
	ConsoleCommand("say Hey, I just found %s %s.", CTypeToString::EntityTypeToString(iEntityType).c_str(), cItem.pItemClass->sClassName.c_str());
}

//----------------------------------------------------------------------------------------------------------------
void CBot_BorzhMod::InitNewTask()
{
	m_bNewTask = false;
	switch ( m_cCurrentTask.iTask )
	{
	case EBorzhTaskWait:
		m_fEndWaitTime = CBotrixPlugin::fTime + m_cCurrentTask.iArgument/1000.0f;
		break;
	case EBorzhTaskLook:
	{
		TEntityType iType = GET_TYPE(m_cCurrentTask.iArgument);
		TEntityIndex iIndex = GET_INDEX(m_cCurrentTask.iArgument);
		const CEntity& cEntity = CItems::GetItems(iType)[iIndex];
		m_vLook = cEntity.vOrigin;
		m_bNeedMove = false;
		m_bNeedAim = true;
		break;
	}
	case EBorzhTaskMove:
		if ( iCurrentWaypoint == m_cCurrentTask.iArgument )
			m_bTaskFinished = true;
		else
		{
			m_bNeedMove = true;
			m_bUseNavigatorToMove = true;
			m_bDestinationChanged = true;
			m_iDestinationWaypoint = m_cCurrentTask.iArgument;
		}
		break;
	case EBorzhTaskSpeak:
		m_bNeedMove = false;
		DoSpeakTask();
		break;
	case EBorzhTaskExplore:
	{
		TAreaId iArea = CWaypoints::Get(iCurrentWaypoint).iAreaId;

		const good::vector<TWaypointId>& cWaypoints = CMod_Borzh::GetWaypointsForArea(iArea);
		DebugAssert( cWaypoints.size() > 0 );

		TWaypointId iWaypoint = EInvalidWaypointId;
		int iIndex = rand() % cWaypoints.size();

		// Search for some not visited waypoint in this area.
		for ( int i=iIndex; i >= 0; --i)
		{
			int iCurrent = cWaypoints[i];
			if ( (iCurrent != iCurrentWaypoint) && !m_aVisitedWaypoints.test(iCurrent) )
			{
				iWaypoint = iCurrent;
				break;
			}
		}
		if ( iWaypoint == EInvalidWaypointId )
		{
			for ( int i=iIndex+1; i < cWaypoints.size(); ++i)
			{
				int iCurrent = cWaypoints[i];
				if ( (iCurrent != iCurrentWaypoint) && !m_aVisitedWaypoints.test(iCurrent) )
				{
					iWaypoint = iCurrent;
					break;
				}
			}
		}

		DebugAssert( iWaypoint != iCurrentWaypoint );
		if ( iWaypoint != EInvalidWaypointId )
		{
			m_cTaskStack.push_back( m_cCurrentTask ); // Return to this task later.
			
			// Start task to move to given waypoint.
			m_cCurrentTask.iTask = EBorzhTaskMove;
			m_cCurrentTask.iArgument = iWaypoint;
			m_cTaskStack.push_back( m_cCurrentTask );
		}
		m_bTaskFinished = true;
		break;
	}
//	case EBorzhTaskExplore:
	}
}

//----------------------------------------------------------------------------------------------------------------
void CBot_BorzhMod::PushSpeakTask( TBotChat iChat, TEntityType iType, TEntityIndex iIndex, int iArguments )
{
	int iArgumentSpeak = 0;
	SET_TYPE(iChat, iArgumentSpeak);
	SET_INDEX(iIndex, iArgumentSpeak);
	SET_AUX1(iArguments, iArgumentSpeak);

	int iArgumentLook = 0;
	SET_TYPE(iType, iArgumentLook);
	SET_INDEX(iIndex, iArgumentLook);

	// Save current task.
	if ( m_cCurrentTask.iTask != EBorzhTaskInvalid )
		m_cTaskStack.push_back( m_cCurrentTask );

	// Wait after speak.
	m_cCurrentTask.iTask = EBorzhTaskWait;
	m_cCurrentTask.iArgument = 4000;
	m_cTaskStack.push_back( m_cCurrentTask );

	// Speak after look.
	m_cCurrentTask.iTask = EBorzhTaskSpeak;
	m_cCurrentTask.iArgument = iArgumentSpeak;
	m_cTaskStack.push_back( m_cCurrentTask );

	// Look at entity (door, button, box, etc).
	m_cCurrentTask.iTask = EBorzhTaskLook;
	m_cCurrentTask.iArgument = iArgumentLook;

	m_bNeedMove = false;
	m_bNewTask = true;
}

//----------------------------------------------------------------------------------------------------------------
void CBot_BorzhMod::DoSpeakTask()
{
	DebugAssert( m_cCurrentTask.iTask == EBorzhTaskSpeak );
	m_cChat.iBotRequest = GET_TYPE(m_cCurrentTask.iArgument);
	m_cChat.cMap.clear();

	switch ( m_cChat.iBotRequest )
	{
	case EBorzhChatDoorFound:
	case EBorzhChatDoorChange:
		// Add door status.
		m_cChat.cMap.push_back( CChatVarValue(CMod_Borzh::iVarDoorStatus, 0, GET_AUX1(m_cCurrentTask.iArgument)) );
		// Don't break to add door index.

	case EBorzhChatDoorNoChange:
	case EBorzhChatDoorTry:
		m_cChat.cMap.push_back( CChatVarValue(CMod_Borzh::iVarDoor, 0, GET_INDEX(m_cCurrentTask.iArgument) ) );
		break;

	case EBorzhChatSeeButton:
		m_cChat.cMap.push_back( CChatVarValue(CMod_Borzh::iVarButton, 0, GET_INDEX(m_cCurrentTask.iArgument) ) );
		break;

	case EBorzhChatWeaponFound:
		m_cChat.cMap.push_back( CChatVarValue(CMod_Borzh::iVarWeapon, 0, GET_INDEX(m_cCurrentTask.iArgument)) );
		break;
	}

	Speak(false);
}
