#include "StdAfx.h"


ServerLogin::CSocket::CSocket(const _CHAR* _Name) :
	CGNet::IO::Connector::NTCPReconnect(_Name),
	m_idxList(-1),
	m_request(make_object<manager_register<User::CSocket>>())
{
}

ServerLogin::CSocket::~CSocket()
{
}

void ServerLogin::CSocket::RequestAccountCheckin(const ACCOUNT_ID& _idAccount, User::CSocket* _pSource)
{
	// 1) Request를 먼저 등록한다.
	auto	idTransaction	 = m_request->RegisterRequest(_pSource);

	// Declare)
	bool	bResult;

	// 2) Account ...
	bResult	 = Send(Message_REQUEST_ACCOUNT_CHECKIN(_idAccount, idTransaction));

	// check)
	THROW_IF(bResult==false, CGException::CThrow_Error());
}

void ServerLogin::CSocket::OnConnect(CGNet::IO::IConnective* /*_pConnective*/)
{
	// Trace) 
	CGLOG(g_pLog, eLOG_TYPE::INFO_, eLOG_LEVEL::HIGHER, _CGTEXT("> Login Server Connected [{}]"), CGNetAPI::AddressToString<_CHAR>(GetPeerAddress()).c_str());

	// View)
	g_pservice->NotifyMessage(CGMSG_NOTIFY(CGMESSAGE::WINDOWS::NOTIFY_UPDATE, CGNOTIFY_LOGINSERVER_CONNECT, (DWORD_PTR)this));

	// 1) Certification을 요청하는 메시지를 전송한다.(성공한다면 ACCEPT_CERTIFICATION 메시지를 보내 올 것이다.)
	Send(g_pservice->Message_REQUEST_CERTIFICATION());
}

void ServerLogin::CSocket::OnDisconnect(uint32_t /*_Reason*/)
{
	// View)
	g_pservice->NotifyMessage(CGMSG_NOTIFY(CGMESSAGE::WINDOWS::NOTIFY_UPDATE, CGNOTIFY_LOGINSERVER_DISCONNECT, (DWORD_PTR)this));

	// Trace)
	CGLOG(g_pLog, eLOG_TYPE::INFO_, eLOG_LEVEL::HIGHER, _CGTEXT("< Login Server Disconnected [{}]"), CGNetAPI::AddressToString<_CHAR>(GetPeerAddress()).c_str());

	// 1) Schedulable에서 Unregister한다.
	UNREGISTER_SCHEDULABLE(this);
}

void ServerLogin::CSocket::OnExecute()
{
	// 1) Player Count를 UPdate한다.
	g_pservice->update_player_count();

	// 2) Send Message
	Send(g_pservice->Message_SEND_LOBBYSERVER_STATUS());
}

// Message Map)
BEGIN_CGMESSAGE_MAP(ServerLogin::CSocket)
	CGMESSAGE_TYPE_SUB(CGMESSAGE::SYSTEM::MESSAGE, ((CGMSG_BUFFER&)_msg).Buffer.front<uint32_t>(4))
		ON_CGMESSAGE(CGMESSAGE::ACCEPT_CERTIFICATION,		OnMessage_ACCEPT_CERTIFICATION,		CGNETMSG);
		ON_CGMESSAGE(CGMESSAGE::RESPONSE_ACCOUNT_CHECKIN,	OnMessage_RESPONSE_ACCOUNT_CHECKIN,	CGNETMSG);
	END_SUB_CGMESSAGE_MAP
END_CGMESSAGE_MAP

int ServerLogin::CSocket::OnMessage(CGMSG& _msg)
{
	// 1) Dispatch Message
	EXECUTE_CGMESSAGE_MAP(_msg);

	// Return) Message is not processed
	return	0;
}

int ServerLogin::CSocket::OnMessage_ACCEPT_CERTIFICATION(CGNETMSG& _msg)
{
	// 1) 10초에 한번씩 OnExecute를 호출하게 한다.(이때 서버 상태 정보를 전송한다.)
	SetExecuteInterval(tick::seconds(10));

	// 2) Schedulable을 시작한다.
	REGISTER_SCHEDULABLE(this);

	// Return) 
	return 1;
}

int ServerLogin::CSocket::OnMessage_RESPONSE_ACCOUNT_CHECKIN(CGNETMSG& _msg)
{
	// 1) 
	auto idTransaction = _msg.Buffer.front<uint32_t>(8);
	auto idAccount = _msg.Buffer.front<ACCOUNT_ID>(12);

	try
	{
		// 2) 등록된 Request에서 psocket값을 얻는다.
		auto psocket = m_request->UnregisterRequest(idTransaction);

		// check) 찾지 못했으면 Exception을 던진다.
		THROW_IF(psocket.empty(), CGException::CThrow_NotExist())

		// 3) 결과를 그대로 전송한다.
		psocket->Send(_msg.Buffer);
	}
	catch(...)
	{
		// - 바로 Check-out을 요청한다.
		Send(Message_REQUEST_ACCOUNT_CHECKOUT(idAccount));
	}

	// Return) 
	return	1;
}

CCGBuffer ServerLogin::CSocket::Message_REQUEST_ACCOUNT_CHECKIN(const ACCOUNT_ID& _idAccount, uint32_t _idRequest)
{
	// 1) 버퍼를 할당받는다. (작성할 메시지 크기만큼...)
	CCGBuffer	bufTemp	 = MEM_POOL_ALLOC(256);

	// 2) 메시지를 작성한다.
	bufTemp.append<uint32_t>();
	bufTemp.append<uint32_t>(CGMESSAGE::REQUEST_ACCOUNT_CHECKIN);
	bufTemp.append<uint32_t>(_idRequest);
	bufTemp.append<ACCOUNT_ID>(_idAccount);

	// 3) 메시지의 제일 앞에 메시지의 길이를 써넣는다.
	bufTemp.front<uint32_t>()	 = bufTemp.len;

	// Return) 
	return	bufTemp;
}

CCGBuffer ServerLogin::CSocket::Message_REQUEST_ACCOUNT_CHECKOUT(const ACCOUNT_ID& _idAccount)
{
	// 1) 버퍼를 할당받는다. (작성할 메시지 크기만큼...)
	CCGBuffer	bufTemp	 = MEM_POOL_ALLOC(256);

	// 2) 메시지를 작성한다.
	bufTemp.append<uint32_t>();
	bufTemp.append<uint32_t>(CGMESSAGE::REQUEST_ACCOUNT_CHECKOUT);
	bufTemp.append<ACCOUNT_ID>(_idAccount);

	// 3) 메시지의 제일 앞에 메시지의 길이를 써넣는다.
	bufTemp.front<uint32_t>()	 = bufTemp.len;

	// Return) 
	return	bufTemp;
}
