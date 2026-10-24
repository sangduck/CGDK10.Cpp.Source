#include "StdAfx.h"


ServerLobby::CGroupSocket::CGroupSocket(const _CHAR* _strName) :
	CGNameable::NString(_strName)
{
}

ServerLobby::CGroupSocket::~CGroupSocket()
{
}

CGPTR<ServerLobby::CSocket> ServerLobby::CGroupSocket::GetSuitableLobbyServer()
{
	// Declare) 
	CGPTR<ServerLobby::CSocket>	pSocket;

	SCOPED_LOCK(m_csGroup)
	{
		// check) 서버가 하나도 없으면 nullptr을 리턴한다.
		RETURN_IF(IsMemberEmpty()==true, CGPTR<ServerLobby::CSocket>());

		// 1) 제일 앞에 Server를 무조건 선택
		pSocket	 = begin()->GetMember();
	}

	// Return)
	return	pSocket;
}

void ServerLobby::CGroupSocket::OnInitializing(CGMSG_CONTEXT& /*_msg*/)
{
}

void ServerLobby::CGroupSocket::OnInitialize(CGMSG_CONTEXT& /*_msg*/)
{
}

void ServerLobby::CGroupSocket::OnDestroying()
{
}

void ServerLobby::CGroupSocket::OnDestroy()
{
}

// 2) Start/Stop
void ServerLobby::CGroupSocket::OnStarting(CGMSG_CONTEXT& /*_msg*/)
{
}

void ServerLobby::CGroupSocket::OnStart(CGMSG_CONTEXT& _msg)
{
	// 1) 10초마다 한번씩 호출된다.
	SetExecuteInterval(tick::seconds(10));

	// 2) 실행 등록한다.
	REGISTER_SCHEDULABLE(this);

	// 3) 입장을 받기 시작~
	EnableEnter(true);
}

void ServerLobby::CGroupSocket::OnStopping()
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

void ServerLobby::CGroupSocket::OnStop()
{
}
							
// 3) Member Enter/Leave
eOBJECT_RESULT ServerLobby::CGroupSocket::OnMemberEntering(ServerLobby::CSocket* _pMember, CGMSG& _msg)
{
	// Return) 
	return	eOBJECT_RESULT::SUCCESS;
}

void ServerLobby::CGroupSocket::OnMemberEntered(ServerLobby::CSocket* _pMember, CGMSG& /*_msg*/)
{
	// 1) Register Mediator한다.
	_pMember->RegisterMessageable(this);
}

uintptr_t ServerLobby::CGroupSocket::OnMemberLeaving(ServerLobby::CSocket* /*_pMember*/)
{
	// Return) 
	return	0;
}

void ServerLobby::CGroupSocket::OnMemberLeaved(ServerLobby::CSocket* _pMember, _In_ uintptr_t /*_Result*/)
{
	// 1) Unregister Mediator한다.
	_pMember->UnregisterMessageable(this);
}

int ServerLobby::CGroupSocket::OnMessage(CGMSG& _msg)
{
	// 1) Message Map
	EXECUTE_CGMESSAGE_MAP(_msg);

	// Return) Message 처리를 하지 못했다!!!
	return	0;
}

// Message Map)
BEGIN_CGMESSAGE_MAP(ServerLobby::CGroupSocket)
	CGMESSAGE_TYPE_SUB(CGMESSAGE::SYSTEM::MESSAGE, ((CGMSG_BUFFER&)_msg).Buffer.front<uint32_t>(4))
		ON_CGMESSAGE(CGMESSAGE::SEND_LOBBYSERVER_STATUS,		OnMessage_SEND_LOBBYSERVER_STATUS,		CGNETMSG);
		ON_CGMESSAGE(CGMESSAGE::REQUEST_ACCOUNT_CHECKIN,		OnMessage_REQUEST_ACCOUNT_CHECKIN,		CGNETMSG);
		ON_CGMESSAGE(CGMESSAGE::REQUEST_ACCOUNT_CHECKOUT,		OnMessage_REQUEST_ACCOUNT_CHECKOUT,		CGNETMSG);
		END_SUB_CGMESSAGE_MAP
END_CGMESSAGE_MAP

int ServerLobby::CGroupSocket::OnMessage_SEND_LOBBYSERVER_STATUS(CGNETMSG& _msg)
{
	// 1) Message의 크기를 검사한다.
	CGASSERT_ERROR(_msg.Buffer.len == 8+sizeof(SERVER_INFO));

	// 2) Message의 크기를 검사한다.
	THROW_IF(_msg.Buffer.len != 8+sizeof(SERVER_INFO), CGException::CNetThrow_InvalidMessageLength());

	// 3) Message Casting
	auto	pSocket = _msg.GetSource<ServerLobby::CSocket>();

	// 4) SERVER_INFO를 저장한다.
	SCOPED_LOCK(pSocket->m_ServerInfo)
	{
		pSocket->m_ServerInfo	 = _msg.Buffer.front<SERVER_INFO>(8);
	}

	// Return)
	return	1;
}

int ServerLobby::CGroupSocket::OnMessage_REQUEST_ACCOUNT_CHECKIN(CGNETMSG& _msg)
{
	// 1) Message Casting
	auto	pSocket = _msg.GetSource<User::CSocket>();

	// check) pSocket이 nullptr이면 안 된다.
	CGASSERT_ERROR(pSocket != nullptr);

	// 2) AccountID를 얻는다.
	auto	idTransaction	= _msg.Buffer.front<uint32_t>(8);
	auto&	idAccount		= _msg.Buffer.front<ACCOUNT_ID>(12);

	try
	{
		// 3) 제대로된 login인지 AccountID를 확인한다.
		g_pservice->m_pmanagerAccount->RequestAccountCheckin(idAccount);

		// 4) 성공 메시지를 전송한다.
		pSocket->Send(Message_RESPONSE_ACCOUNT_CHECKIN(0, idTransaction, idAccount));
	}
	catch(...)
	{
		// - 실패 메시지를 전송한다.
		pSocket->Send(Message_RESPONSE_ACCOUNT_CHECKIN(1, idTransaction, idAccount));

		// - 접속을 종료한다.
		pSocket->Disconnect();
	}

	// Return)
	return	1;
}

int ServerLobby::CGroupSocket::OnMessage_REQUEST_ACCOUNT_CHECKOUT(CGNETMSG& _msg)
{
	// 1) AccountID를 얻는다.
	auto	idAccount	= _msg.Buffer.front<ACCOUNT_ID>(8);

	// 2) Checkout 처리를 한다.
	g_pservice->m_pmanagerAccount->RequestAccountCheckout(idAccount);

	// Return)
	return	1;
}

void ServerLobby::CGroupSocket::OnExecute()
{
}

CCGBuffer ServerLobby::CGroupSocket::Message_RESPONSE_ACCOUNT_CHECKIN(int _Result, uint32_t _idTransaction, const ACCOUNT_ID& _idAccount)
{
	// 1) 버퍼를 할당받는다.
	CCGBuffer	bufTemp = MEM_POOL_ALLOC(64);

	// 2) 메시지 작성
	bufTemp.append<uint32_t>();
	bufTemp.append<uint32_t>(CGMESSAGE::RESPONSE_ACCOUNT_CHECKIN);
	bufTemp.append<uint32_t>(_idTransaction);
	bufTemp.append<int>(_Result);
	bufTemp.append<ACCOUNT_ID>(_idAccount);

	// 3) 메시지의 제일 앞에 메시지의 길이를 써넣는다.
	bufTemp.front<uint32_t>()	 = bufTemp.len;

	// Return) 
	return	bufTemp;
}