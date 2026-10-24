#include "stdafx.h"
#include "Service.h"

CGPTR<CCGLogHub>	g_pLog;

CService::CService()
{
	// 1) Service객체 이름
	SetName(_CGTEXT("CGCII Sample Login Server"));

	// 2) Log file
	m_pLogFile.make_object(_CGTEXT("logfile"));	
	NCGAttachable<ICGObjectState>::ProcessAttach(m_pLogFile);
	g_pLog->RegisterMessageable(m_pLogFile);

	// 3) Account Manager
	m_pmanagerAccount.make_object(_CGTEXT("AccountManager"));
	NCGAttachable<ICGObjectState>::ProcessAttach(m_pmanagerAccount);

	// 4) Server Login
	m_pgroupServerLobby.make_object(_CGTEXT("groupServerLobby"));
	NCGAttachable<ICGObjectState>::ProcessAttach(m_pgroupServerLobby);

	m_pgroupServerLobbyImmigration.make_object(_CGTEXT("groupImmigrationServerLobby"));
	NCGAttachable<ICGObjectState>::ProcessAttach(m_pgroupServerLobbyImmigration);

	m_pacceptorServerLobby.make_object(_CGTEXT("acceptorServerLobby"));
	NCGAttachable<ICGObjectState>::ProcessAttach(m_pacceptorServerLobby);

	// 5) User
	m_pgroupUserImmigration.make_object(_CGTEXT("groupImmigrationUser"));
	NCGAttachable<ICGObjectState>::ProcessAttach(m_pgroupUserImmigration);

	m_pacceptorUser.make_object("acceptorUser");
	NCGAttachable<ICGObjectState>::ProcessAttach(m_pacceptorUser);
}

CService::~CService()
{
	// 1) Unregister
	g_pLog->UnregisterMessageable(m_pLogFile);
}

CGPTR<ServerLobby::CSocket>	CService::GetSuitableLobbyServer()
{
	return	m_pgroupServerLobby->GetSuitableLobbyServer();
}


void CService::OnServiceSetting(CGMSG_CONTEXT& _msg)
{
	// 1) Config file을 읽어 들인다.
	//_msg.Setting.read_from_JSON_file("LoginServer_Setting.json");

	// 1) Setting
	_msg.Setting["acceptorServerLobby"]["Port"]	 = 20000;
	_msg.Setting["acceptorUser"]["Port"]		 = 21000;
}


void CService::OnInitializing(CGMSG_CONTEXT& /*_msg*/)
{
	// Trace) 
	LOG_SYSTEM(nullptr, _CGTEXT("-----------------------------------------------------------------"));
	LOG_SYSTEM(nullptr, _CGTEXT(""));
	LOG_SYSTEM(nullptr, _CGTEXT(" {}"), GetName());
	LOG_SYSTEM(nullptr, _CGTEXT(""));
	LOG_SYSTEM(nullptr, _CGTEXT("@CGDK: {}"), _CGTEXT(CGDK_CONFIGURATION));
	LOG_SYSTEM(nullptr, _CGTEXT("@SVN_UPDATE_REVISION: {}"), _TO_CGSTRING(SVN_UPDATE_REVISION));
	LOG_SYSTEM(nullptr, _CGTEXT("@SVN_COMMIT_REVISION: {}"), _TO_CGSTRING(SVN_COMMIT_REVISION));
	LOG_SYSTEM(nullptr, _CGTEXT("@Compile: {} {}"), _CGTEXT(__DATE__), _CGTEXT(__TIME__));
	LOG_SYSTEM(nullptr, _CGTEXT("-----------------------------------------------------------------"));
}

void CService::OnInitialize(CGMSG_CONTEXT& _msg)
{
}

void CService::OnStarting(CGMSG_CONTEXT& /*_msg*/)
{
}

void CService::OnStart(CGMSG_CONTEXT& _msg)
{
}

void CService::OnStopping()
{
}

void CService::OnStop()
{
}

void CService::OnDestroying()
{
}

void CService::OnDestroy()
{
}

void CService::OnExecute()
{
}

int CService::OnMessage(CGMSG& /*_msg*/)
{
	// Return) 
	return	0;
}
