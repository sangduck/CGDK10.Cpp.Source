#include "StdAfx.h"


User::CGroupSocket::CGroupSocket(const _CHAR* _strName) :
	CGNameable::NString(_strName)
{
}

User::CGroupSocket::~CGroupSocket()
{
}

void User::CGroupSocket::OnInitializing(CGMSG_CONTEXT& /*_msg*/)
{
}

void User::CGroupSocket::OnInitialize(CGMSG_CONTEXT& /*_msg*/)
{
}

void User::CGroupSocket::OnStarting(CGMSG_CONTEXT& /*_msg*/)
{
}

void User::CGroupSocket::OnStart(CGMSG_CONTEXT& _msg)
{
	// 1) 1분마다 한번씩 호출되도록 Schedulable의 간격을 설정한다.
	SetExecuteInterval(tick::seconds(10));

	// 2) Scheduler에  등록한다.
	REGISTER_SCHEDULABLE(this);

	// 3) 입장을 받기 시작~
	EnableEnter(true);
}


void User::CGroupSocket::OnStopping()
{
	// 1) 
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

void User::CGroupSocket::OnStop()
{
}

void User::CGroupSocket::OnDestroying()
{
}

void User::CGroupSocket::OnDestroy()
{
}

eOBJECT_RESULT User::CGroupSocket::OnMemberEntering(User::CSocket* /*_pMember*/, CGMSG& /*_msg*/)
{
	// Return) 
	return	eOBJECT_RESULT::SUCCESS;
}

void User::CGroupSocket::OnMemberEntered(User::CSocket* _pMember, CGMSG& /*_msg*/)
{
	// 1) Register Mediator한다.
	_pMember->RegisterMessageable(this);
}

uintptr_t User::CGroupSocket::OnMemberLeaving(User::CSocket* /*_pMember*/)
{
	// Return) 
	return	0;
}

void User::CGroupSocket::OnMemberLeaved(User::CSocket* _pMember, _In_ uintptr_t /*_Result*/)
{
	// 1) Unregister Mediator한다.
	_pMember->UnregisterMessageable(this);
}

void User::CGroupSocket::OnExecute()
{
}

int User::CGroupSocket::OnMessage(CGMSG& _msg)
{
	// 1) Message Map
	EXECUTE_CGMESSAGE_MAP(_msg);
	
	// Return) Message 처리를 하지 못했다!!!
	return	0;
}

BEGIN_CGMESSAGE_MAP(User::CGroupSocket)
	// 1) Message from Socket Chatting Server
	CGMESSAGE_TYPE_SUB(CGMESSAGE::SYSTEM::MESSAGE, ((CGMSG_BUFFER&)_msg).Buffer.front<uint32_t>(4))
		//ON_CGMESSAGE(SaPacket::packet_ALIVE,				OnMessage_ALIVE,				CGNETMSG);
		//ON_CGMESSAGE(SaPacket::packet_STATE,				OnMessage_STATE,				CGNETMSG);
	END_SUB_CGMESSAGE_MAP
END_CGMESSAGE_MAP

//int User::CGroupSocket::OnMessage_ALIVE(CGNETMSG& _msg)
//{
//	//// check) Message의 길이가 잘못되었으면 Exception을 던진다.
//	//THROW_IF(_msg.Buffer.len != sizeof(PACKET_ALIVE), CGException::CNetThrow_InvalidMessageLength(), LOG_EXCEPTION(g_pLog, _CGTEXT("Invalid Message Size [Received:%d - Require %d] (") _CGTEXT(__FUNCTION__) _CGTEXT(")"), _msg.Buffer.len, sizeof(PACKET_ALIVE)));
//
//	//// 1) Socket을 얻는다.
//	//User::CSocket*	pSocket = _msg.GetSource<User::CSocket>();
//
//	//// check) Socket이 nullptr이면 Exception을 던진다.
//	//THROW_IF(pSocket == nullptr, CGException::CNetThrow_InvalidSocketHandle(), LOG_EXCEPTION(g_pLog, _CGTEXT("Invalid Source socket (") _CGTEXT(__FUNCTION__) _CGTEXT(")")));
//
//	//// 2) Message를 읽어낸다.
//	//const PACKET_ALIVE&		message = _msg.Buffer.front<PACKET_ALIVE>();
//
//	//// 3) Alive Tick
//	//pSocket->m_AliveTick	 = 	message.AliveTick;
//
//	//// View)
//	//g_pservice->NotifyMessage(CGMSG_NOTIFY(CGMESSAGE::WINDOWS::NOTIFY_UPDATE, CGNOTIFY_GAMESERVER_UPDATE, (DWORD_PTR)pSocket));
//
//	// Return) 
//	return	1;
//}
//
//int User::CGroupSocket::OnMessage_STATE(CGNETMSG& _msg)
//{
//	//// check) Message의 길이가 잘못되었으면 Exception을 던진다.
//	//THROW_IF(_msg.Buffer.len != sizeof(PACKET_STATE), CGException::CNetThrow_InvalidMessageLength(), LOG_EXCEPTION(g_pLog, _CGTEXT("Invalid Message Size [Received:%d - Require %d] (") _CGTEXT(__FUNCTION__) _CGTEXT(")"), _msg.Buffer.len, sizeof(PACKET_STATE)));
//
//	//// 1) Socket을 얻는다.
//	//User::CSocket*	pSocket	 = _msg.GetSource<User::CSocket>();
//
//	//// check) Socket이 nullptr이면 Exception을 던진다.
//	//THROW_IF(pSocket==nullptr, CGException::CNetThrow_InvalidSocketHandle(), LOG_EXCEPTION(g_pLog, _CGTEXT("Invalid Source socket (") _CGTEXT(__FUNCTION__) _CGTEXT(")") ));
//
//	//// 2) Message를 읽어낸다.
//	//const PACKET_STATE&	message	 = _msg.Buffer.front<PACKET_STATE>();
//
//	//// 3) Game Server info를 써넣는다.
//	//SCOPED_LOCK(pSocket->m_infoGameServer)
//	//{
//	//	pSocket->m_infoGameServer	 = message.m_infoGameServer;
//	//}
//
//	//// 4) Server Info flag를 Signal!
//	//m_bServerInfoUpdated.store(true);
//
//	//// View)
//	//g_pservice->NotifyMessage(CGMSG_NOTIFY(CGMESSAGE::WINDOWS::NOTIFY_UPDATE, CGNOTIFY_GAMESERVER_UPDATE, (DWORD_PTR)pSocket));
//
//	// Return) 
//	return	1;
//}

//CCGBuffer User::CGroupSocket::Message_SERVER_LIST()
//{
//	// Declare) 
//	CCGBuffer	bufTemp;
//
//	SCOPED_LOCK(GetGroupLock())
//	{
//		// 1) 버퍼를 할당받는다.
//		bufTemp = MEM_POOL_ALLOC(sizeof(PACKET_SERVER_LIST) + GetMemberCount()*sizeof(SServerListInfo));
//
//		// 2) Message Headm를 써넣는다.
//		auto&	messageHead = bufTemp.append<PACKET_SERVER_LIST>();
//
//		// 3) 개수를 초기화
//		messageHead.Page	 = 0;
//
//		// 2) 만들기..
//		for(auto& iter:*this)
//		{
//			// check)
//			CONTINUE_IF(iter->m_infoGameServer.State==-1);
//
//			// - 데이터를 추가한다.
//			bufTemp.append<SServerListInfo>(iter->m_infoGameServer);
//
//			// - 개수 증가
//			++messageHead.Page;
//		}
//	}
//
//	// Return)
//	return	bufTemp;
//}
