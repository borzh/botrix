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


good::vector<TEntityIndex> CBot_BorzhMod::m_aLastPushedButtons;

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
	m_cPushedButtons.resize( CItems::GetItems(EEntityTypeButton).size() );

	m_cOpenedDoors.resize( m_cSeenDoors.size() );
	m_cButtonTogglesDoor.resize( m_cSeenButtons.size()  );
	m_cButtonNoAffectDoor.resize( m_cSeenButtons.size() );
	m_cCheckedDoors.resize( m_cSeenButtons.size() );
	for ( int i=0; i < m_cButtonTogglesDoor.size(); ++i )
	{
		m_cButtonTogglesDoor[i].resize( m_cSeenDoors.size() );
		m_cButtonNoAffectDoor[i].resize( m_cSeenDoors.size() );
	}

	m_aVisitedWaypoints.resize( CWaypoints::Size() );

	const StringVector& aAreas = CWaypoints::GetAreas();
	m_aVisitedAreas.resize( aAreas.size() );
	m_cReachableAreas.resize( aAreas.size() );

	m_cWaitingPlayers.resize( CPlayers::Size() );
}

//----------------------------------------------------------------------------------------------------------------
void CBot_BorzhMod::Respawned()
{
	CBot::Respawned();

	m_bDontAttack = m_bDomainChanged = true;
	m_cCurrentTask.iTask = EBorzhTaskInvalid;

	if ( iCurrentWaypoint != -1 )
		m_aVisitedWaypoints.set(iCurrentWaypoint);
	m_iCurrentArea = CWaypoints::Get(iCurrentWaypoint).iAreaId;

	SetReachableAreas(m_iCurrentArea, m_cSeenDoors, m_cOpenedDoors, m_cReachableAreas);

	if ( !m_aVisitedAreas.test(m_iCurrentArea) )
	{
		// Explore new area.
		m_cCurrentBigTask.iTask = EBorzhTaskExplore;
		m_cCurrentBigTask.iArgument = m_iCurrentArea;

		// Say: i will explore new area.
		PushSpeakTask(EBorzhChatExplore, EEntityTypeInvalid, m_iCurrentArea);

		// Say: i am in new area.
		PushSpeakTask(EBorzhChatNewArea, EEntityTypeInvalid, m_iCurrentArea);
	}
	else
		// Say: i have changed area.
		PushSpeakTask(EBorzhChatChangeArea, EEntityTypeInvalid, m_iCurrentArea);

	if ( m_bFirstRespawn )
	{
		// Say hello to some other player.
		for ( TPlayerIndex iPlayer = 0; iPlayer < CPlayers::Size(); ++iPlayer )
		{
			CPlayer* pPlayer = CPlayers::Get(iPlayer);
			if ( pPlayer && (m_iIndex != iPlayer) && (pPlayer->GetTeam() != 1) ) // Player is not an observer.
			{
				PushSpeakTask(EBotChatGreeting, EEntityTypeInvalid, iPlayer);
			}
		}
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

	if ( !m_bStarted || (m_bNothingToDo && !m_bDomainChanged) )
		return;

	// Check if there are some new tasks.
	if ( m_cCurrentTask.iTask == EBorzhTaskInvalid )
	{
		if ( m_cTaskStack.size() > 0 )
		{
			m_cCurrentTask = m_cTaskStack.back();
			m_cTaskStack.pop_back();
			m_bNewTask = true;
		}
		else
		{
			CheckBigTask();
			if ( m_cCurrentTask.iTask == EBorzhTaskInvalid )
				CheckForNewTasks();
		}
	}

	if ( m_bNewTask )
		InitNewTask();

	// Update current task.
	if ( !m_bTaskFinished )
	{
		switch (m_cCurrentTask.iTask)
		{
		case EBorzhTaskWait:
			if ( CBotrixPlugin::fTime >= m_fEndWaitTime ) // Bot finished waiting.
				m_bTaskFinished = true;
			break;

		case EBorzhTaskLook:
			if ( !m_bNeedAim ) // Bot finished aiming.
				m_bTaskFinished = true;
			break;

		case EBorzhTaskMove:
			if ( !m_bNeedMove ) // Bot finished moving.
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

	bool bDomainChanged = false;

	// Check neighbours of a new waypoint.
	const CWaypoints::WaypointNode& cNode = CWaypoints::GetNode(iCurrentWaypoint);
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
				TEntityIndex iDoor = cPath.iArgument;
				if ( iDoor > 0 )
				{
					--iDoor;

					bool bOpened = CItems::IsDoorOpened(iDoor);
					bool bNeedToPassThrough = (iWaypoint == m_iAfterNextWaypoint) && m_bNeedMove && m_bUseNavigatorToMove;
					CheckDoorStatus(iDoor, bOpened, bNeedToPassThrough);
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
			{
				m_cSeenButtons.set(iButton);
				SwitchToSpeakTask( EBorzhChatSeeButton, EEntityTypeButton, iButton );
			}
		}
		else
			CUtil::Message(NULL, "Error, waypoint %d has invalid button index.", iCurrentWaypoint);
	}

	// Check if bot enters new area.
	if ( cNode.vertex.iAreaId != m_iCurrentArea )
	{
		m_iCurrentArea = cNode.vertex.iAreaId;
		if ( m_aVisitedAreas.test(m_iCurrentArea) )
			SwitchToSpeakTask(EBorzhChatNewArea, EEntityTypeInvalid, m_iCurrentArea);
		else
			SwitchToSpeakTask(EBorzhChatChangeArea, EEntityTypeInvalid, m_iCurrentArea);
	}
}

//----------------------------------------------------------------------------------------------------------------
bool CBot_BorzhMod::DoWaypointAction()
{
	const CWaypoint& cWaypoint = CWaypoints::Get(iCurrentWaypoint);

	// Check if bot saw the button to shoot before.
	if ( FLAG_SOME_SET(FWaypointSeeButton, cWaypoint.iFlags) )
	{
		TEntityIndex iButton = CWaypoint::GetButton(cWaypoint.iArgument);
		if (iButton > 0)
		{
			--iButton;
			if ( !m_cSeenButtons.test(iButton) ) // Bot sees button for the first time.
			{
				if ( m_bHasCrossbow )
					SwitchToSpeakTask( EBorzhChatButtonWeapon, EEntityTypeButton, iButton );
				else
					SwitchToSpeakTask( EBorzhChatButtonNoWeapon, EEntityTypeButton, iButton );
				SwitchToSpeakTask( EBorzhChatSeeButton, EEntityTypeButton, iButton );
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
	switch( iEntityType )
	{
	case EEntityTypeWeapon:
		TEntityIndex iIdx;
		if ( cItem.pItemClass->sClassName == "weapon_crossbow" ) 
			iIdx = CMod_Borzh::iVarValueWeaponCrossbow;
		else if ( cItem.pItemClass->sClassName == "weapon_physcannon" ) 
			iIdx = CMod_Borzh::iVarValueWeaponPhyscannon;
		else
			break;
		SwitchToSpeakTask(EBorzhChatWeaponFound, EEntityTypeInvalid, iIdx);
		break;
	}
}

//----------------------------------------------------------------------------------------------------------------
void CBot_BorzhMod::SetReachableAreas( int iCurrentArea, const good::bitset& cSeenDoors, const good::bitset& cOpenedDoors, good::bitset& cReachableAreas )
{
	cReachableAreas.clear();

	good::vector<TAreaId> cToVisit( CWaypoints::GetAreas().size() );
	cToVisit.push_back( iCurrentArea );

	const good::vector<CEntity>& cDoorEntities = CItems::GetItems(EEntityTypeDoor);
	while ( !cToVisit.empty() )
	{
		int iArea = cToVisit.back();
		cToVisit.pop_back();

		cReachableAreas.set(iArea);

		const good::vector<TEntityIndex>& cDoors = CMod_Borzh::GetDoorsForArea(iArea);
		for ( TEntityIndex i = 0; i < cDoors.size(); ++i )
		{
			const CEntity& cDoor = cDoorEntities[ cDoors[i] ];
			if ( cSeenDoors.test( cDoors[i] ) && cOpenedDoors.test( cDoors[i] ) ) // Seen and opened door.
			{
				TWaypointId iWaypoint1 = cDoor.iWaypoint;
				TWaypointId iWaypoint2 = (TWaypointId)cDoor.pArguments;
				if ( CWaypoints::IsValid(iWaypoint1) && CWaypoints::IsValid(iWaypoint2) )
				{
					TAreaId iArea1 = CWaypoints::Get(iWaypoint1).iAreaId;
					TAreaId iArea2 = CWaypoints::Get(iWaypoint2).iAreaId;
					TAreaId iNewArea = ( iArea1 == iArea ) ? iArea2 : iArea1;
					if ( !cReachableAreas.test(iNewArea) )
						cToVisit.push_back(iArea2);
				}
			}
		}
	}
}

//----------------------------------------------------------------------------------------------------------------
TWaypointId CBot_BorzhMod::ButtonWaypoint( TEntityIndex iButton, const good::bitset& cReachableAreas )
{
	const good::vector<CEntity>& cButtonEntities = CItems::GetItems(EEntityTypeButton);
	const CEntity& cButton = cButtonEntities[ iButton ];

	if ( CWaypoints::IsValid(cButton.iWaypoint) )
		return cReachableAreas.test( CWaypoints::Get(cButton.iWaypoint).iAreaId ) ? cButton.iWaypoint : EInvalidWaypointId;
	return false;
}

//----------------------------------------------------------------------------------------------------------------
TWaypointId CBot_BorzhMod::DoorWaypoint( TEntityIndex iDoor, const good::bitset& cReachableAreas )
{
	const good::vector<CEntity>& cDoorEntities = CItems::GetItems(EEntityTypeDoor);
	const CEntity& cDoor = cDoorEntities[ iDoor ];

	TWaypointId iWaypoint1 = cDoor.iWaypoint;
	TWaypointId iWaypoint2 = (TWaypointId)cDoor.pArguments;
	if ( CWaypoints::IsValid(iWaypoint1) && CWaypoints::IsValid(iWaypoint2) )
	{
		TAreaId iArea1 = CWaypoints::Get(iWaypoint1).iAreaId;
		TAreaId iArea2 = CWaypoints::Get(iWaypoint2).iAreaId;
		return cReachableAreas.test(iArea1) ? iWaypoint1 : (cReachableAreas.test(iArea2) ? iWaypoint2 : EInvalidWaypointId);
	}
	return false;
}

//----------------------------------------------------------------------------------------------------------------
void CBot_BorzhMod::CheckDoorStatus( TEntityIndex iDoor, bool bOpened, bool bNeedToPassThrough )
{
	TChatVariableValue iDoorStatus = bOpened ? CMod_Borzh::iVarValueDoorStatusOpened : CMod_Borzh::iVarValueDoorStatusClosed;
	bool bCheckingDoor = (m_cCurrentBigTask.iTask == EBorzhTaskCheckButton) && (GET_TYPE(m_cCurrentBigTask.iArgument) != 0); // We already pushed button.

	if ( !m_cSeenDoors.test(iDoor) ) // Bot sees door for the first time.
	{
		DebugAssert( m_cCurrentBigTask.iTask == EBorzhTaskExplore ); // Should occur only when exploring new area.
		m_cSeenDoors.set(iDoor);
		m_cOpenedDoors.set(iDoor, bOpened);
		SwitchToSpeakTask(EBorzhChatDoorFound, EEntityTypeDoor, iDoor, iDoorStatus);
	}
	else if ( !bOpened && bNeedToPassThrough ) // Bot needs to pass through the door, but door is closed.
	{		
		ClosedDoorOnTheWay(iDoor);
	}
	else if ( m_cOpenedDoors.test(iDoor) != bOpened ) // Bot belief of opened/closed door is different.
	{
		m_cOpenedDoors.set(iDoor, bOpened);
		SwitchToSpeakTask(EBorzhChatDoorChange, EEntityTypeDoor, iDoor, iDoorStatus);
		if ( bCheckingDoor )
		{
			m_cCheckedDoors.set(iDoor);
			// Button that we are checking opens iDoor.
			SetButtonTogglesDoor(GET_INDEX(m_cCurrentBigTask.iArgument), iDoor, true);
		}
	}
	else if ( bCheckingDoor )
	{
		m_cCheckedDoors.set(iDoor);
		// Button that we are checking doesn't affect iDoor.
		SetButtonTogglesDoor(GET_INDEX(m_cCurrentBigTask.iArgument), iDoor, false);
		// Say: door $door has not changed.
		SwitchToSpeakTask(EBorzhChatDoorNoChange, EEntityTypeDoor, iDoor, iDoorStatus);
	}
}

//----------------------------------------------------------------------------------------------------------------
void CBot_BorzhMod::ClosedDoorOnTheWay( TEntityIndex iDoor )
{
	DebugAssert( m_cOpenedDoors.test(iDoor) ); // Bot should think that this door was opened.
	m_cOpenedDoors.clear(iDoor);

	if ( m_cCurrentBigTask.iTask == EBorzhTaskCheckButton )
	{
		CancelTask();

		// Recalculate reachable areas.
		SetReachableAreas(m_iCurrentArea, m_cSeenDoors, m_cOpenedDoors, m_cReachableAreas);
	}
	else
	{
		// Say: I can't reach area $area.
		TAreaId iArea = CWaypoints::Get( m_iDestinationWaypoint ).iAreaId;
		SwitchToSpeakTask(EBorzhChatAreaCantGo, EEntityTypeInvalid, iArea);
	}

	// Say: Door $door is closed now.
	SwitchToSpeakTask(EBorzhChatDoorChange, EEntityTypeDoor, iDoor, CMod_Borzh::iVarValueDoorStatusClosed);
}

//----------------------------------------------------------------------------------------------------------------
void CBot_BorzhMod::CheckBigTask()
{
	DebugAssert( m_cCurrentTask.iTask == EBorzhTaskInvalid );
	switch ( m_cCurrentBigTask.iTask )
	{
		case EBorzhTaskExplore:
		{
			TAreaId iArea = m_cCurrentBigTask.iArgument;

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
			if ( iWaypoint == EInvalidWaypointId )
			{
				m_aVisitedAreas.set(iArea);
				m_cCurrentBigTask.iTask = EBorzhTaskInvalid;
				DoSpeakTask(EBorzhChatFinishExplore);
			}
			else
			{
				// Start task to move to given waypoint.
				m_cCurrentTask.iTask = EBorzhTaskMove;
				m_cCurrentTask.iArgument = iWaypoint;
				m_bNewTask = true;
			}
			break;
		}

		case EBorzhTaskCheckButton:
		{
			const good::vector<CEntity>& aDoors = CItems::GetItems(EEntityTypeDoor);
			for ( int iDoor = 0; iDoor < aDoors.size(); ++iDoor )
				if ( !m_cCheckedDoors.test(iDoor) )
				{
					TWaypointId iWaypoint = DoorWaypoint(iDoor, m_cReachableAreas);
					if ( iWaypoint != EInvalidWaypointId )
					{
						m_cCurrentTask.iTask = EBorzhTaskMove;
						m_cCurrentTask.iArgument = iWaypoint;
						m_bNewTask = true;
						return;
					}
				}
			m_cCurrentBigTask.iTask = EBorzhTaskInvalid;
			break;
		}
	}
}

//----------------------------------------------------------------------------------------------------------------
void CBot_BorzhMod::CheckForNewTasks()
{
	DebugAssert( m_cCurrentTask.iTask == EBorzhTaskInvalid );

	// Check if there is some player waiting for this bot. TODO:
	/*if ( m_cWaitingPlayers.any() )
	{
		for ( TPlayerIndex iPlayer = 0; iPlayer < m_cWaitingPlayers.size(); ++iPlayer )
			if ( m_cWaitingPlayers.test(iPlayer) )
			{
				m_cCurrentBigTask.iTask = EBorzhTaskHelping;
				m_cCurrentBigTask.iArgument = iPlayer;

				// Wait for answer to the question.
				m_cCurrentTask.iTask = EBorzhTaskWaitAnswer;
				m_cCurrentTask.iArgument = 0;
				//SET_TYPE(, m_cCurrentTask.iArgument);
				//SET_INDEX(iPlayer, m_cCurrentTask.iArgument);
				return;
			}
	}*/

	// TODO: Check if all bots can pass to goal area.

	// Check if there is some area to investigate.
	const StringVector& aAreas = CWaypoints::GetAreas();
	for ( TAreaId iArea = 0; iArea < aAreas.size(); ++iArea )
	{
		if ( !m_aVisitedAreas.test(iArea) && m_cReachableAreas.test(iArea) )
		{
			m_cCurrentBigTask.iTask = m_cCurrentTask.iTask = EBorzhTaskExplore;
			m_cCurrentBigTask.iArgument = m_cCurrentTask.iArgument = iArea;
			m_bNewTask = true;
			return;
		}
	}

	// Check if there are new button to push.
	const good::vector<CEntity>& aButtons = CItems::GetItems(EEntityTypeButton);
	for ( TEntityIndex iButton = 0; iButton < aButtons.size(); ++iButton )
	{
		if ( m_cSeenButtons.test(iButton) && !m_cPushedButtons.test(iButton) && 
		     ButtonWaypoint(iButton, m_cReachableAreas) != EInvalidWaypointId ) // Can reach button.
		{
			PushCheckButtonTask(iButton);
			return;
		}
	}

	// Check if there are unknown button-door configuration to check (without pushing intermediate buttons).
	/*const good::vector<CEntity>& aDoors = CItems::GetItems(EEntityTypeDoor);
	good::bitset cReachableAreas( aAreas.size() );

	for ( TEntityIndex iButton = 0; iButton < aButtons.size(); ++iButton )
	{
		TWaypointId iButtonWaypoint = aButtons[iButton].iWaypoint;
		TAreaId iAreaButton = ( iButtonWaypoint == EInvalidWaypointId ) ? EInvalidAreaId : CWaypoints::Get(iButtonWaypoint).iAreaId;

		if ( iAreaButton == EInvalidAreaId )
		{
			// TODO: check shoot button.
		}
		else
		{
			for ( TPlayerIndex iPlayer = 0; iPlayer < m_cCollaborativePlayers.size(); ++iPlayer )
			{
				if ( (iPlayer != m_iIndex) && m_cCollaborativePlayers.test(iPlayer) && (m_aPlayersAreas[iPlayer] != EInvalidAreaId) )
				{
					SetReachableAreas(m_aPlayersAreas[iPlayer], m_cSeenDoors, m_cOpenedDoors, cReachableAreas);

					if ()
					for ( TEntityIndex iDoor = 0; iDoor < aDoors.size(); ++iDoor )
					{
						if (  )
					}
				}
			}
		}
	}*/

	// Bot has nothing to do, wait for domain change.
	DoSpeakTask(EBorzhChatNoMoves);
	m_bNothingToDo = true;
	m_bDomainChanged = false;
}

//----------------------------------------------------------------------------------------------------------------
void CBot_BorzhMod::InitNewTask()
{
	m_bNewTask = m_bTaskFinished = false;

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
		{
			m_bNeedMove = m_bUseNavigatorToMove = m_bDestinationChanged = false;
			m_bTaskFinished = true;
		}
		else
		{
			m_bNeedMove = m_bUseNavigatorToMove = m_bDestinationChanged = true;
			m_iDestinationWaypoint = m_cCurrentTask.iArgument;
		}
		break;
	case EBorzhTaskSpeak:
		m_bNeedMove = false;
		DoSpeakTask(m_cCurrentTask.iArgument);
		break;

	case EBorzhTaskPushButton:
		FLAG_SET(IN_USE, m_cCmd.buttons);
		m_cPushedButtons.set(m_cCurrentTask.iArgument);
		m_aLastPushedButtons.push_back(m_cCurrentTask.iArgument);
		SET_TYPE(true, m_cCurrentBigTask.iArgument); // Save that button was pushed.
		Wait(2000); // Wait 2 seconds after pushing button.
		break;

	case EBorzhTaskShootButton: // TODO: set current weapon & right-click & wait & left click.
		m_cPushedButtons.set(m_cCurrentTask.iArgument);
		m_aLastPushedButtons.push_back(m_cCurrentTask.iArgument);
		SET_TYPE(true, m_cCurrentBigTask.iArgument); // Save that button was pushed.
		Wait(2000); // Wait 2 seconds after shooting button.
		break;
	}
}

//----------------------------------------------------------------------------------------------------------------
void CBot_BorzhMod::PushSpeakTask( TBotChat iChat, TEntityType iType, TEntityIndex iIndex, int iArguments )
{
	// Speak.
	CBorzhTask task(EBorzhTaskSpeak);
	SET_TYPE(iChat, task.iArgument);
	SET_INDEX(iIndex, task.iArgument);
	SET_AUX1(iArguments, task.iArgument);
	m_cTaskStack.push_back( task );

	if ( iType != EEntityTypeInvalid ) // There is something to look at.
	{
		// Look at entity before speak (door, button, box, etc).
		task.iTask = EBorzhTaskLook;
		task.iArgument = 0;
		SET_TYPE(iType, task.iArgument);
		SET_INDEX(iIndex, task.iArgument);
		m_cTaskStack.push_back( task );
	}
}

//----------------------------------------------------------------------------------------------------------------
void CBot_BorzhMod::DoSpeakTask( int iArgument )
{
	m_cChat.iBotRequest = GET_TYPE(iArgument);
	m_cChat.cMap.clear();

	switch ( m_cChat.iBotRequest )
	{
	case EBotChatGreeting:
		m_cChat.iDirectedTo = GET_INDEX(iArgument);
		break;

	case EBorzhChatDoorFound:
	case EBorzhChatDoorChange:
	case EBorzhChatDoorNoChange:
		// Add door status.
		m_cChat.cMap.push_back( CChatVarValue(CMod_Borzh::iVarDoorStatus, 0, GET_AUX1(iArgument)) );
		// Don't break to add door index.

	case EBorzhChatDoorTry:
	case EBorzhChatDoorGo:
		m_cChat.cMap.push_back( CChatVarValue(CMod_Borzh::iVarDoor, 0, GET_INDEX(iArgument) ) );
		break;

	case EBorzhChatSeeButton:
	case EBorzhChatButtonIPush:
	case EBorzhChatButtonYouPush:
	case EBorzhChatButtonIShoot:
	case EBorzhChatButtonYouShoot:
	case EBorzhChatButtonTry:
	case EBorzhChatButtonGo:
		m_cChat.cMap.push_back( CChatVarValue(CMod_Borzh::iVarButton, 0, GET_INDEX(iArgument) ) );
		break;

	case EBorzhChatWeaponFound:
		m_cChat.cMap.push_back( CChatVarValue(CMod_Borzh::iVarWeapon, 0, GET_INDEX(iArgument)) );
		break;

	case EBorzhChatNewArea:
	case EBorzhChatChangeArea:
	case EBorzhChatAreaCantGo:
	case EBorzhChatAreaGo:
		m_cChat.cMap.push_back( CChatVarValue(CMod_Borzh::iVarArea, 0, GET_INDEX(iArgument)) );
		break;
	}

	Speak(false);
	Wait(3000);
}

//----------------------------------------------------------------------------------------------------------------
void CBot_BorzhMod::PushCheckButtonTask( TEntityIndex iButton, bool bShoot )
{
	DebugAssert( !bShoot || m_bHasCrossbow );

	// Mark all doors as unchecked.
	m_cCheckedDoors.clear();

	m_cCurrentBigTask.iTask = EBorzhTaskCheckButton;
	m_cCurrentBigTask.iArgument = 0;
	SET_INDEX(iButton, m_cCurrentBigTask.iArgument);

	// Push button.
	m_cCurrentTask.iTask = bShoot ? EBorzhTaskShootButton : EBorzhTaskPushButton;
	m_cCurrentTask.iArgument = iButton;
	m_cTaskStack.push_back(m_cCurrentTask);

	// Say: I will push/shoot button $button now.
	m_cCurrentTask.iTask = EBorzhTaskSpeak;
	m_cCurrentTask.iArgument = 0;
	SET_TYPE(bShoot ? EBorzhChatButtonIShoot : EBorzhChatButtonIPush, m_cCurrentTask.iArgument);
	SET_INDEX(iButton, m_cCurrentTask.iArgument);
	m_cTaskStack.push_back(m_cCurrentTask);

	// Look at button.
	m_cCurrentTask.iTask = EBorzhTaskLook;
	m_cCurrentTask.iArgument = 0;
	SET_TYPE(EEntityTypeButton, m_cCurrentTask.iArgument);
	SET_INDEX(iButton, m_cCurrentTask.iArgument);
	m_cTaskStack.push_back(m_cCurrentTask);

	// Go to button waypoint.
	m_cCurrentTask.iTask = EBorzhTaskMove;
	const CEntity& cButton = CItems::GetItems(EEntityTypeButton)[iButton];
	if ( bShoot )
		m_cCurrentTask.iArgument = CMod_Borzh::GetWaypointToShootButton(iButton);
	else
		m_cCurrentTask.iArgument = cButton.iWaypoint;
	m_cTaskStack.push_back(m_cCurrentTask);

	// Say: Let's try to find which doors opens button $button.
	m_cCurrentTask.iTask = EBorzhTaskSpeak;
	m_cCurrentTask.iArgument = 0;
	SET_TYPE(EBorzhChatButtonTry, m_cCurrentTask.iArgument);
	SET_INDEX(iButton, m_cCurrentTask.iArgument);
	m_bNewTask = true; // Switch to new task.
}
