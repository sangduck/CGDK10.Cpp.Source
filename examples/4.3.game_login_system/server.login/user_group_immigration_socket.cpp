#include "StdAfx.h"


User::CGroupImmigrationSocket::CGroupImmigrationSocket(const _CHAR* _strName) :
	CGNameable::NString(_strName)
{
}

User::CGroupImmigrationSocket::~CGroupImmigrationSocket()
{
}

void User::CGroupImmigrationSocket::OnInitializing(CGMSG_CONTEXT& /*_msg*/)
{
}

void User::CGroupImmigrationSocket::OnInitialize(CGMSG_CONTEXT& /*_msg*/)
{
}

void User::CGroupImmigrationSocket::OnDestroying()
{
}

void User::CGroupImmigrationSocket::OnDestroy()
{
}

void User::CGroupImmigrationSocket::OnStarting(CGMSG_CONTEXT& /*_msg*/)
{
}

void User::CGroupImmigrationSocket::OnStart(CGMSG_CONTEXT& _msg)
{
	// 1) 1분마다 한번씩 호출되도록 Schedulable의 간격을 설정한다.
	SetExecuteInterval(tick::minutes(10));

	// 2) Scheduler에  등록한다.
	REGISTER_SCHEDULABLE(this);

	// 3) 입장을 받기 시작~
	EnableEnter(true);
}

void User::CGroupImmigrationSocket::OnStopping()
{
	// 1) 입장을 금지
	DisableEnter();

	// 2) 실행 등록 해제한다.
	UNREGISTER_SCHEDULABLE(this);

	// 3) 모든 Group에 접속한 모든 Member를 모두 접속 종료시킨다.
	ForEachMember([=](User::CSocket* _pMember)->bool
	{
		_pMember->CloseSocket(); 
		return true;
	});
}

void User::CGroupImmigrationSocket::OnStop()
{
}

// 3) Member Enter/Leave
eOBJECT_RESULT User::CGroupImmigrationSocket::OnMemberEntering(User::CSocket* /*_pMember*/, CGMSG& /*_msg*/)
{
	// Return) 
	return	eOBJECT_RESULT::SUCCESS;
}

void User::CGroupImmigrationSocket::OnMemberEntered(User::CSocket* _pMember, CGMSG& /*_msg*/)
{
	// 1) Register Mediator한다.
	_pMember->RegisterMessageable(this);
}

uintptr_t User::CGroupImmigrationSocket::OnMemberLeaving(User::CSocket* /*_pMember*/)
{
	return	0;
}

void User::CGroupImmigrationSocket::OnMemberLeaved(User::CSocket* _pMember, _In_ uintptr_t /*_Result*/)
{
	// 1) Unregister Mediator한다.
	_pMember->UnregisterMessageable(this);
}

void User::CGroupImmigrationSocket::OnExecute()
{
}

int User::CGroupImmigrationSocket::OnMessage(CGMSG& _msg)
{
	// 1) Message Map
	EXECUTE_CGMESSAGE_MAP(_msg);
	
	// Return) Message 처리를 하지 못했다!!!
	return	0;
}

BEGIN_CGMESSAGE_MAP(User::CGroupImmigrationSocket)
	// 1) Message from Socket Chatting Server
	CGMESSAGE_TYPE_SUB(CGMESSAGE::SYSTEM::MESSAGE, ((CGMSG_BUFFER&)_msg).Buffer.front<uint32_t>(4))
		ON_CGMESSAGE(CGMESSAGE::REQUEST_ACCOUNT_LOGIN, OnMessage_REQUEST_ACCOUNT_LOGIN, CGNETMSG);
	END_SUB_CGMESSAGE_MAP
END_CGMESSAGE_MAP

int User::CGroupImmigrationSocket::OnMessage_REQUEST_ACCOUNT_LOGIN(CGNETMSG& _msg)
{
	// 1) Message의 크기를 검사한다.
	//CGASSERT_ERROR(_msg.Buffer.len == sizeof(PACKET_REQUEST_CERTIFICATION));

	// 2) Message의 크기를 검사한다.
	//THROW_IF(_msg.Buffer.len != sizeof(PACKET_REQUEST_CERTIFICATION), CGException::CNetThrow_InvalidMessageLength());

	// 3) 
	auto&	msgRecv = _msg.Buffer + 8;
	auto	pSocket = _msg.GetSource<User::CSocket>();

	try
	{
		// 4) Login Type (안씀)
		auto	iLoginType	 = msgRecv.extract<int>();

		// 4) ID와 Password를 얻는다.
		auto	strID		 = msgRecv.extract<const wchar_t*>();
		auto	strPassword	 = msgRecv.extract<const wchar_t*>();

		// 5) ID와 Password를 Query를 요청한다.
		//    성공한다면 적당한 Lobby Server 정보를 얻어 전송한다.
		g_pService->m_pmanagerAccount->RequestAccountLogin(strID, strPassword, pSocket);
	}
	catch (...)
	{
		// - 거절 메시지를 전송한다.
		pSocket->Send(Message_RESULT_ACCOUNT_LOGIN_REJECT(0));

		// - 접속을 끊는다.
		pSocket->Disconnect();
	}

	// Return)
	return	1;
}

CCGBuffer User::CGroupImmigrationSocket::Message_RESULT_ACCOUNT_LOGIN_ACCEPT(const ACCOUNT_ID& _idAccount, const SERVER_INFO& _rServerInfo)
{
	// 1) 버퍼를 할당받는다.
	CCGBuffer	bufTemp = MEM_POOL_ALLOC(256);

	// 2) 메시지 작성
	bufTemp.append<uint32_t>();
	bufTemp.append<uint32_t>(CGMESSAGE::RESPONSE_ACCOUNT_LOGIN);
	bufTemp.append<int>(0);
	bufTemp.append<ACCOUNT_ID>(_idAccount);
	bufTemp.append<SOCKADDR_IN>(_rServerInfo.nServer_Address);

	// 3) 메시지의 제일 앞에 메시지의 길이를 써넣는다.
	bufTemp.front<uint32_t>()	 = bufTemp.len;

	// Return) 
	return	bufTemp;
}

CCGBuffer User::CGroupImmigrationSocket::Message_RESULT_ACCOUNT_LOGIN_REJECT(int _Error)
{
	// 1) 버퍼를 할당받는다.
	CCGBuffer	bufTemp = MEM_POOL_ALLOC(64);

	// 2) 메시지 작성
	bufTemp.append<uint32_t>();
	bufTemp.append<uint32_t>(CGMESSAGE::RESPONSE_ACCOUNT_LOGIN);
	bufTemp.append<int>(_Error);

	// 3) 메시지의 제일 앞에 메시지의 길이를 써넣는다.
	bufTemp.front<uint32_t>()	 = bufTemp.len;

	// Return) 
	return	bufTemp;
}
