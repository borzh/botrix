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
	m_cDoorToggle.resize( m_cSeenButtons.size()  );
	m_cDoorNoAffect.resize( m_cSeenButtons.size() );
	for ( int i=0; i < m_cDoorToggle.size(); ++i )
	{
		m_cDoorToggle[i].resize( m_cSeenDoors.size() );
		m_cDoorNoAffect[i].resize( m_cSeenDoors.size() );
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
		// Explore new area after speak about it.
		m_cCurrentTask.iTask = EBorzhTaskExplore;
		m_cCurrentTask.iArgument = m_iCurrentArea;
		PushSpeakTask(EBorzhChatExplore, EEntityTypeInvalid, m_iCurrentArea);

		// Speak about new area after saying hello.
		PushSpeakTask(EBorzhChatNewArea, EEntityTypeInvalid, m_iCurrentArea);
	}
	else
		PushSpeakTask(EBorzhChatChangeArea, EEntityTypeInvalid, m_iCurrentArea);

	if ( m_bFirstRespawn )
	{
		// Say hello to some other player.
		for ( TPlayerIndex iPlayer = 0; iPlayer < CPlayers::Size(); ++iPlayer )
		{
			CPlayer* pPlayer = CPlayers::Get(iPlayer);
			if ( pPlayer && (m_iIndex != iPlayer) )
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
		CheckForNewTasks();

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
					TChatVariableValue iDoorStatus = bOpened ? CMod_Borzh::iVarValueDoorStatusOpened : CMod_Borzh::iVarValueDoorStatusClosed;

					if ( m_cCurrentBigTask.iTask == EBorzhTaskCheckButton )
					{
						DebugAssert( m_cSeenDoors.test(iDoor) ); // Bot should at least see this door.
						m_cCheckedDoors.set(iDoor);
						PushSpeakTask(EBorzhChatDoorChange, EEntityTypeDoor, iDoor, iDoorStatus);
					}

					if ( !bOpened && (iWaypoint == m_iAfterNextWaypoint) && m_bNeedMove && m_bUseNavigatorToMove )
					{
						// Bot needs to pass through the door, but door is closed.
						DebugAssert( m_cCurrentTask.iTask == EBorzhTaskMove );
						TAreaId iArea = CWaypoints::Get( m_iDestinationWaypoint ).iAreaId;
						CancelTask(); // Cancel last task, like investigate area.
						PushSpeakTask(EBorzhChatDoorChange, EEntityTypeDoor, iDoor, iDoorStatus);
						PushSpeakTask(EBorzhChatAreaCantGo, EEntityTypeInvalid, iArea);
					}
					else if ( !m_cSeenDoors.test(iDoor) ) // Bot sees door for the first time.
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

	// Check if bot enters new area.
	if ( cNode.vertex.iAreaId != m_iCurrentArea )
	{
		m_iCurrentArea = cNode.vertex.iAreaId;
		if ( m_aVisitedAreas.test(m_iCurrentArea) )
			PushSpeakTask(EBorzhChatNewArea, EEntityTypeInvalid, m_iCurrentArea);
		else
			PushSpeakTask(EBorzhChatChangeArea, EEntityTypeInvalid, m_iCurrentArea);
	}

	if ( bDomainChanged )
		SetReachableAreas(m_iCurrentArea, m_cSeenDoors, m_cOpenedDoors, m_cReachableAreas);
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
		PushSpeakTask(EBorzhChatWeaponFound, EEntityTypeInvalid, iIdx);
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
bool CBot_BorzhMod::IsButtonReachable( TEntityIndex iButton, const good::bitset& cReachableAreas )
{
	const good::vector<CEntity>& cButtonEntities = CItems::GetItems(EEntityTypeButton);
	const CEntity& cButton = cButtonEntities[ iButton ];

	if ( CWaypoints::IsValid(cButton.iWaypoint) )
		return cReachableAreas.test( CWaypoints::Get(cButton.iWaypoint).iAreaId );
	return false;
}

//----------------------------------------------------------------------------------------------------------------
bool CBot_BorzhMod::IsDoorReachable( TEntityIndex iDoor, const good::bitset& cReachableAreas )
{
	const good::vector<CEntity>& cDoorEntities = CItems::GetItems(EEntityTypeDoor);
	const CEntity& cDoor = cDoorEntities[ iDoor ];

	TWaypointId iWaypoint1 = cDoor.iWaypoint;
	TWaypointId iWaypoint2 = (TWaypointId)cDoor.pArguments;
	if ( CWaypoints::IsValid(iWaypoint1) && CWaypoints::IsValid(iWaypoint2) )
	{
		TAreaId iArea1 = CWaypoints::Get(iWaypoint1).iAreaId;
		TAreaId iArea2 = CWaypoints::Get(iWaypoint2).iAreaId;
		return cReachableAreas.test(iArea1) || cReachableAreas.test(iArea2);
	}
	return false;
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
		if ( m_cSeenButtons.test(iButton) && !m_cPushedButtons.test(iButton) && IsButtonReachable(iButton, m_cReachableAreas) )
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
	PushSpeakTask(EBorzhChatNoMoves, EEntityTypeInvalid, EEntityIndexInvalid);
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
		DoSpeakTask();
		break;
	case EBorzhTaskExplore:
	{
		TAreaId iArea = m_cCurrentTask.iArgument;

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
			m_cCurrentTask.iTask = EBorzhTaskInvalid; // Don't save current task, it is finished.
			PushSpeakTask(EBorzhChatFinishExplore, EEntityTypeInvalid, EEntityIndexInvalid);
		}
		else
		{
			m_cTaskStack.push_back( m_cCurrentTask ); // Return to explore task later.
			
			// Start task to move to given waypoint.
			m_cCurrentTask.iTask = EBorzhTaskMove;
			m_cCurrentTask.iArgument = iWaypoint;
			m_cTaskStack.push_back( m_cCurrentTask ); // Return to explore task later.

			m_bTaskFinished = true;
		}
		break;
	}
	case EBorzhTaskPushButton:
		FLAG_SET(IN_USE, m_cCmd.buttons);
		m_cCurrentTask.iTask = EBorzhTaskWait;
		m_fEndWaitTime = CBotrixPlugin::fTime + 2.0f; // Wait 2 seconds after pushing button.
		break;
	}
}

//----------------------------------------------------------------------------------------------------------------
void CBot_BorzhMod::PushSpeakTask( TBotChat iChat, TEntityType iType, TEntityIndex iIndex, int iArguments, bool bWaitFirst )
{
	SaveCurrentTask();

	// Wait after speak.
	m_cCurrentTask.iTask = EBorzhTaskWait;
	m_cCurrentTask.iArgument = 3000;
	m_cTaskStack.push_back( m_cCurrentTask );

	// Speak.
	m_cCurrentTask.iTask = EBorzhTaskSpeak;
	m_cCurrentTask.iArgument = 0;
	SET_TYPE(iChat, m_cCurrentTask.iArgument);
	SET_INDEX(iIndex, m_cCurrentTask.iArgument);
	SET_AUX1(iArguments, m_cCurrentTask.iArgument);
	m_cTaskStack.push_back( m_cCurrentTask );

	if ( iType != EEntityTypeInvalid ) // There is something to look at.
	{
		// Look at entity before speak (door, button, box, etc).
		m_cCurrentTask.iTask = EBorzhTaskLook;
		m_cCurrentTask.iArgument = 0;
		SET_TYPE(iType, m_cCurrentTask.iArgument);
		SET_INDEX(iIndex, m_cCurrentTask.iArgument);
		m_cTaskStack.push_back( m_cCurrentTask );
	}

	if ( bWaitFirst )
	{
		m_cCurrentTask.iTask = EBorzhTaskWait;
		m_cCurrentTask.iArgument = 2000;
		m_cTaskStack.push_back( m_cCurrentTask );
	}

	m_bTaskFinished = true; // Switch to new task.
}

//----------------------------------------------------------------------------------------------------------------
void CBot_BorzhMod::DoSpeakTask()
{
	DebugAssert( m_cCurrentTask.iTask == EBorzhTaskSpeak );
	m_cChat.iBotRequest = GET_TYPE(m_cCurrentTask.iArgument);
	m_cChat.cMap.clear();

	switch ( m_cChat.iBotRequest )
	{
	case EBotChatGreeting:
		m_cChat.iDirectedTo = GET_INDEX(m_cCurrentTask.iArgument);
		break;

	case EBorzhChatDoorFound:
	case EBorzhChatDoorChange:
		// Add door status.
		m_cChat.cMap.push_back( CChatVarValue(CMod_Borzh::iVarDoorStatus, 0, GET_AUX1(m_cCurrentTask.iArgument)) );
		// Don't break to add door index.

	case EBorzhChatDoorNoChange:
	case EBorzhChatDoorTry:
	case EBorzhChatDoorGo:
		m_cChat.cMap.push_back( CChatVarValue(CMod_Borzh::iVarDoor, 0, GET_INDEX(m_cCurrentTask.iArgument) ) );
		break;

	case EBorzhChatSeeButton:
	case EBorzhChatButtonIPush:
	case EBorzhChatButtonYouPush:
	case EBorzhChatButtonIShoot:
	case EBorzhChatButtonYouShoot:
	case EBorzhChatButtonGo:
		m_cChat.cMap.push_back( CChatVarValue(CMod_Borzh::iVarButton, 0, GET_INDEX(m_cCurrentTask.iArgument) ) );
		break;

	case EBorzhChatWeaponFound:
		m_cChat.cMap.push_back( CChatVarValue(CMod_Borzh::iVarWeapon, 0, GET_INDEX(m_cCurrentTask.iArgument)) );
		break;

	case EBorzhChatNewArea:
	case EBorzhChatChangeArea:
	case EBorzhChatAreaCantGo:
	case EBorzhChatAreaGo:
		m_cChat.cMap.push_back( CChatVarValue(CMod_Borzh::iVarArea, 0, GET_INDEX(m_cCurrentTask.iArgument)) );
		break;
	}

	Speak(false);
}

//----------------------------------------------------------------------------------------------------------------
void CBot_BorzhMod::PushCheckButtonTask( TEntityIndex iButton, bool bShoot )
{
	DebugAssert( !bShoot || m_bHasCrossbow );

	SaveCurrentTask();

	m_cCurrentBigTask.iTask = EBorzhTaskCheckButton;
	m_cCurrentBigTask.iArgument = iButton;

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
	m_cTaskStack.push_back(m_cCurrentTask);

	m_bTaskFinished = true; // Switch to new task.
}
