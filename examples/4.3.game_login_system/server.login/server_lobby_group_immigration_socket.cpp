#include "StdAfx.h"


ServerLobby::CGroupImmigrationSocket::CGroupImmigrationSocket(const _CHAR* _strName) :
	CGNameable::NString(_strName)
{
}

ServerLobby::CGroupImmigrationSocket::~CGroupImmigrationSocket()
{
}

void ServerLobby::CGroupImmigrationSocket::OnInitializing(CGMSG_CONTEXT& /*_msg*/)
{
}

void ServerLobby::CGroupImmigrationSocket::OnInitialize(CGMSG_CONTEXT& /*_msg*/)
{
}

void ServerLobby::CGroupImmigrationSocket::OnDestroying()
{
}

void ServerLobby::CGroupImmigrationSocket::OnDestroy()
{
}

// 2) Start/Stop
void ServerLobby::CGroupImmigrationSocket::OnStarting(CGMSG_CONTEXT& /*_msg*/)
{
}

void ServerLobby::CGroupImmigrationSocket::OnStart(CGMSG_CONTEXT& _msg)
{
	// 1) 10초마다 한번씩 호출된다.
	SetExecuteInterval(tick::seconds(10));

	// 2) 실행 등록한다.
	REGISTER_SCHEDULABLE(this);

	// 3) 입장을 받기 시작~
	EnableEnter(true);
}

void ServerLobby::CGroupImmigrationSocket::OnStopping()
{
	// 1) 입장을 금지
	DisableEnter();

	// 2) 실행 등록 해제한다.
	UNREGISTER_SCHEDULABLE(this);

	// 3) 모든 Group에 접속한 모든 Member를 모두 접속 종료시킨다.
	ForEachMember([=](ServerLobby::CSocket* _pMember)->bool
	{
		_pMember->CloseSocket(); 
		return true;
	});
}

void ServerLobby::CGroupImmigrationSocket::OnStop()
{
}
							
// 3) Member Enter/Leave
eOBJECT_RESULT ServerLobby::CGroupImmigrationSocket::OnMemberEntering(ServerLobby::CSocket* _pMember, CGMSG& _msg)
{
	// Return) 
	return	eOBJECT_RESULT::SUCCESS;
}

void ServerLobby::CGroupImmigrationSocket::OnMemberEntered(ServerLobby::CSocket* _pMember, CGMSG& /*_msg*/)
{
	// 1) Register Mediator한다.
	_pMember->RegisterMessageable(this);
}

uintptr_t ServerLobby::CGroupImmigrationSocket::OnMemberLeaving(ServerLobby::CSocket* /*_pMember*/)
{
	// Return) 
	return	0;
}

void ServerLobby::CGroupImmigrationSocket::OnMemberLeaved(ServerLobby::CSocket* _pMember, _In_ uintptr_t /*_Result*/)
{
	// 1) Unregister Mediator한다.
	_pMember->UnregisterMessageable(this);
}

int ServerLobby::CGroupImmigrationSocket::OnMessage(CGMSG& _msg)
{
	// 1) Message Map
	EXECUTE_CGMESSAGE_MAP(_msg);

	// Return) Message 처리를 하지 못했다!!!
	return	0;
}

// Message Map)
BEGIN_CGMESSAGE_MAP(ServerLobby::CGroupImmigrationSocket)
	CGMESSAGE_TYPE_SUB(CGMESSAGE::SYSTEM::MESSAGE, ((CGMSG_BUFFER&)_msg).Buffer.front<uint32_t>(4))
		ON_CGMESSAGE(CGMESSAGE::REQUEST_CERTIFICATION,	OnMessage_REQUEST_CERTIFICATION,	CGNETMSG);
	END_SUB_CGMESSAGE_MAP
END_CGMESSAGE_MAP

int ServerLobby::CGroupImmigrationSocket::OnMessage_REQUEST_CERTIFICATION(CGNETMSG& _msg)
{
	// 1) Message의 크기를 검사한다.
	CGASSERT_ERROR(_msg.Buffer.len == 8+sizeof(SERVER_INFO));

	// 2) Message의 크기를 검사한다.
	THROW_IF(_msg.Buffer.len != 8+sizeof(SERVER_INFO), CGException::CNetThrow_InvalidMessageLength());

	// 3) Message Casting
	auto	pSocket = _msg.GetSource<ServerLobby::CSocket>();

	// check) pSocket이 nullptr이면 안됀다.
	CGASSERT_ERROR(pSocket != nullptr);

	// 4) Server 정보를 저장한다.
	scoped_lock(pSocket->m_ServerInfo)
	{
		pSocket->m_ServerInfo	 = _msg.Buffer.front<SERVER_INFO>(8);
	}

	// 5) ACCEPT_CERTIFICATION Message를 전송한다.
	pSocket->Send(Message_ACCEPT_CERTICIATION());

	// 6) group Login으로 옮긴다.
	g_pService->m_pgroupServerLobby->EnterMember(pSocket);

	// View)
	g_pService->NotifyMessage(CGMSG_NOTIFY(CGMESSAGE::WINDOWS::NOTIFY_UPDATE, CGNOTIFY_LOBBYSERVER_UPDATE, (DWORD_PTR)pSocket));

	// Return)
	return	1;
}

CCGBuffer ServerLobby::CGroupImmigrationSocket::Message_ACCEPT_CERTICIATION()
{
	// 1) 버퍼를 할당받는다.
	CCGBuffer	bufTemp	 = MEM_POOL_ALLOC(32);

	// 2) 메시지를 작성한다.
	bufTemp.append<uint32_t>();
	bufTemp.append<uint32_t>(CGMESSAGE::ACCEPT_CERTIFICATION);

	// 3) 메시지의 제일 앞에 메시지의 길이를 써넣는다.
	bufTemp.front<uint32_t>()	 = bufTemp.len;

	// Return) 
	return	bufTemp;
}

void ServerLobby::CGroupImmigrationSocket::OnExecute()
{
}
