
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
void CBot_BorzhMod::Respawned()
{
	CBot::Respawned();
	
	m_bDontAttack = true;

	m_iFailWaypoint = EInvalidWaypointId;
	
	m_iCurrentTask = EBotTaskInvalid;
	m_bNeedTaskCheck = true;
	m_bCheckWeapon = CMod::HasMapItems(EEntityTypeWeapon);
}

//----------------------------------------------------------------------------------------------------------------
void CBot_BorzhMod::KilledEnemy( int iPlayerIndex, CPlayer* pVictim )
{
}

//----------------------------------------------------------------------------------------------------------------
void CBot_BorzhMod::HurtBy( int iPlayerIndex, CPlayer* pAttacker, int iHealthNow )
{
	if ( pAttacker && (pAttacker != this) )
		CheckEnemy(iPlayerIndex, pAttacker, false);
	if ( iHealthNow < (CUtil::iPlayerMaxHealth/2) )
		m_bNeedTaskCheck = true; // Check if need search for health.
}

//----------------------------------------------------------------------------------------------------------------
void CBot_BorzhMod::Move()
{
	if ( !m_bAlive )
	{
		m_cCmd.buttons = rand() & IN_ATTACK; // Force bot to respawn by hitting randomly attack button.
		return;
	}
}

//----------------------------------------------------------------------------------------------------------------
void CBot_BorzhMod::ReceiveChat( int iPlayerIndex, CPlayer* pPlayer, bool bTeamOnly, const char* szText )
{
}

//----------------------------------------------------------------------------------------------------------------
bool CBot_BorzhMod::DoWaypointAction()
{
	return CBot::DoWaypointAction();
}

//----------------------------------------------------------------------------------------------------------------
void CBot_BorzhMod::CheckNewTasks( bool bForceTaskChange )
{
}

void CBot_BorzhMod::TaskFinished()
{
}
