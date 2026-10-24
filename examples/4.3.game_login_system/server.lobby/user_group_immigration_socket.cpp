#include "StdAfx.h"


User::CGroupImmigrationSocket::CGroupImmigrationSocket(const _CHAR* _strName) :
	CGNameable::NString(_strName)
{
}

User::CGroupImmigrationSocket::~CGroupImmigrationSocket()
{
}

// 1) Initialize/Destroy
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

// 2) Start/Stop
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
		ON_CGMESSAGE(CGMESSAGE::REQUEST_ACCOUNT_CHECKIN, OnMessage_REQUEST_ACCOUNT_CHECKIN, CGNETMSG);
		ON_CGMESSAGE(CGMESSAGE::RESPONSE_ACCOUNT_CHECKIN, OnMessage_RESPONSE_ACCOUNT_CHECKIN, CGNETMSG);
	END_SUB_CGMESSAGE_MAP
END_CGMESSAGE_MAP

int User::CGroupImmigrationSocket::OnMessage_REQUEST_ACCOUNT_CHECKIN(CGNETMSG& _msg)
{
	// check) Message의 크기를 검사한다.
	//CGASSERT_ERROR(_msg.Buffer.len == sizeof(PACKET_REQUEST_CERTIFICATION));

	// check) Message의 크기를 검사한다.
	//THROW_IF(_msg.Buffer.len != sizeof(PACKET_REQUEST_CERTIFICATION), CGException::CNetThrow_InvalidMessageLength());

	// 1) Message Casting
	auto	pSocket = _msg.GetSource<User::CSocket>();

	// check) pSocket이 nullptr이면 안 된다.
	CGASSERT_ERROR(pSocket != nullptr);

	// 2) ACCOUNT_ID를 읽는다.
	auto&	idAccount	 = _msg.Buffer.front<ACCOUNT_ID>(8);

	// 3) Checkin을 요청한다.
	g_pservice->m_psocketServerLogin->RequestAccountCheckin(idAccount, pSocket);

	// Return)
	return	1;
}

int User::CGroupImmigrationSocket::OnMessage_RESPONSE_ACCOUNT_CHECKIN(CGNETMSG& _msg)
{
	// 1) pSocket에 Result를 전송!


	//g_pservice->m_psocketServerLogin->RequestAccountCheckin(_msg.Buffer);


	// Return)
	return	1;
}


//CCGBuffer User::CGroupImmigrationSocket::Message_ACCEPT_CERTICIATION()
//{
//	// 1) 버퍼 할당
//	CCGBuffer	bufTemp = MEM_POOL_ALLOC(sizeof(PACKET_ACCEPT_CERTIFICATION));
//
//	// 2) Message 작성
//	bufTemp.append<PACKET_ACCEPT_CERTIFICATION>();
//
//	// Return) 
//	return	bufTemp;
//}
