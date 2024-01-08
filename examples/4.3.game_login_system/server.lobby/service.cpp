#include "stdafx.h"
#include "Service.h"

CGPTR<CCGLogHub>			g_pLog;

CService::CService()
{
	// 1) Service객체 이름
	SetName(_CGTEXT("CGCII Sample Lobby Server"));

	// 2) Log file
	m_pLogFile.NEW(_CGTEXT("logfile"));
	NCGAttachable<ICGObjectState>::ProcessAttach(m_pLogFile);
	g_pLog->RegisterMessageable(m_pLogFile);

	// 4) User
	m_pgroupUser.NEW(_CGTEXT("groupUser"));
	NCGAttachable<ICGObjectState>::ProcessAttach(m_pgroupUser);

	m_pgroupUserImmigration.NEW(_CGTEXT("groupImmigrationUser"));
	NCGAttachable<ICGObjectState>::ProcessAttach(m_pgroupUserImmigration);

	m_pacceptorUser.NEW("acceptorUser");
	NCGAttachable<ICGObjectState>::ProcessAttach(m_pacceptorUser);

	// 5) Server Login
	m_psocketServerLogin.NEW(_CGTEXT("socketServerLobby"));
	NCGAttachable<ICGObjectState>::ProcessAttach(m_psocketServerLogin);
	m_psocketServerLogin->SetReconnectInterval(tick::seconds(5));
}

CService::~CService()
{
	// 1) Unregister
	g_pLog->UnregisterMessageable(m_pLogFile);
}

void CService::UpdatePlayerCount()
{
	// check)
	RETURN_IF(m_pacceptorUser.empty());

	scoped_lock(m_ServerInfo)
	{
		m_ServerInfo.nServer_Player_Count = m_pacceptorUser->GetCount();
	}
}


void CService::OnServiceSetting(CGMSG_CONTEXT& _msg)
{
	// 1) Config file을 읽어 들인다.
	//_msg.Setting.read_from_JSON_file("LobbyServer_Setting.json");

	// 1) Setting
	_msg.Setting["Server_No"]			 = 0;
	_msg.Setting["Server_SessionKey"]	 = 1;
	_msg.Setting["Server_Type"]			 = 0;

	_msg.Setting["socketServerLobby"]["Address"]		 = "127.0.0.1:20000";
	_msg.Setting["socketServerLobby"]["Reconnection"]	 = "enable";
	_msg.Setting["acceptorUser"]["Port"]				 = 20000;
}

void CService::OnInitializing(CGMSG_CONTEXT& _msg)
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

	// 1) 설정
	m_ServerInfo.nServer_No			 = _msg.Setting["Server_No"];
	m_ServerInfo.nServer_SessionKey	 = _msg.Setting["Server_SessionKey"];
	m_ServerInfo.nServer_Type		 = _msg.Setting["Server_Type"];
}

void CService::OnInitialize(CGMSG_CONTEXT& _msg)
{
}

void CService::OnStarting(CGMSG_CONTEXT& /*_msg*/)
{
}

void CService::OnStart(CGMSG_CONTEXT& _msg)
{
	// 1) Acceptor의 Address를 저장한다.
	m_ServerInfo.nServer_Address	 = m_pacceptorUser->GetSocketAddressInPublic();
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

CCGBuffer CService::Message_REQUEST_CERTIFICATION()
{
	// 1) 버퍼를 할당받는다. (작성할 메시지 크기만큼...)
	CCGBuffer	bufTemp	 = MEM_POOL_ALLOC(8+sizeof(SERVER_INFO));

	// 2) 메시지를 작성한다.
	bufTemp.append<uint32_t>();
	bufTemp.append<uint32_t>(CGMESSAGE::REQUEST_CERTIFICATION);
	scoped_lock(m_ServerInfo)
	{
		bufTemp.append<SERVER_INFO>(m_ServerInfo);
	}

	// 3) 메시지의 제일 앞에 메시지의 길이를 써넣는다.
	bufTemp.front<uint32_t>()	 = bufTemp.len;

	// Return) 
	return	bufTemp;
}

CCGBuffer CService::Message_SEND_LOBBYSERVER_STATUS()
{
	// 1) 버퍼를 할당받는다. (작성할 메시지 크기만큼...)
	CCGBuffer	bufTemp	 = MEM_POOL_ALLOC(8+sizeof(SERVER_INFO));

	// 2) 메시지를 작성한다.
	bufTemp.append<uint32_t>();
	bufTemp.append<uint32_t>(CGMESSAGE::SEND_LOBBYSERVER_STATUS);
	scoped_lock(m_ServerInfo)
	{
		bufTemp.append<SERVER_INFO>(m_ServerInfo);
	}

	// 3) 메시지의 제일 앞에 메시지의 길이를 써넣는다.
	bufTemp.front<uint32_t>()	 = bufTemp.len;

	// Return) 
	return	bufTemp;
}