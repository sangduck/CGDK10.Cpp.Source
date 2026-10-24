#include "pch.h"


result_code user::group::on_member_entering(socket* _pmember, sMESSAGE& /*_msg*/)
{
	// 1) Register Messageable한다.
	_pmember->register_messageable(this);

	// return) 
	return	eRESULT::SUCCESS;
}

uintptr_t user::group::on_member_leaving(socket* _pmember, uintptr_t _param)
{
	// 1) unregister Mediator한다.
	_pmember->unregister_messageable(this);

	// return) 
	return _param;
}

result_code user::group::on_message(sMESSAGE_NETWORK& _msg)
{
	// 1) Message Map
	BEGIN_NESTED_CGMESSAGE_MAP
		CGMESSAGE_TYPE_SUB(eMESSAGE::SYSTEM::NETWORK, _msg.buf_message.front<uint32_t>(4))
			//ON_CGMESSAGE(SaPacket::packet_ALIVE, OnMessage_ALIVE, sMESSAGE_NETWORK);
			//ON_CGMESSAGE(SaPacket::packet_STATE, OnMessage_STATE, sMESSAGE_NETWORK);
		END_SUB_CGMESSAGE_MAP
	END_NESTED_CGMESSAGE_MAP

	// return)
	return eRESULT::BYPASS;
}

//result_code user::group::OnMessage_ALIVE(CGNETMSG& _Msg)
//{
//	//// Check) Message의 길이가 잘못되었으면 Exception을 던진다.
//	//ERROR_THROW_IF(_Msg.Buffer.len != sizeof(PACKET_ALIVE), CGException::CNetThrow_InvalidMessageLength(), CGLOG_EXCEPTION(g_pLog, _CGTEXT("Invalid Message Size [Received:%d - Require %d] (") _CGTEXT(__FUNCTION__) _CGTEXT(")"), _Msg.Buffer.len, sizeof(PACKET_ALIVE)));
//
//	//// 1) Socket을 얻는다.
//	//user::socket*	psocket = _Msg.GetSource<user::socket>();
//
//	//// Check) Socket이 nullptr이면 Exception을 던진다.
//	//ERROR_THROW_IF(psocket == nullptr, CGException::CNetThrow_InvalidSocketHandle(), CGLOG_EXCEPTION(g_pLog, _CGTEXT("Invalid Source socket (") _CGTEXT(__FUNCTION__) _CGTEXT(")")));
//
//	//// 2) Message를 읽어낸다.
//	//const PACKET_ALIVE&		message = _Msg.Buffer.front<PACKET_ALIVE>();
//
//	//// 3) Alive Tick
//	//psocket->m_AliveTick	 = 	message.AliveTick;
//
//	//// View)
//	//transmit_message_to(g_pservice, sMESSAGE_NOTIFY(CGMESSAGE::WINDOWS::NOTIFY_UPDATE, CGNOTIFY_GAMESERVER_UPDATE, (DWORD_PTR)psocket));
//
//	// return) 
//	return	1;
//}
//
//result_code user::group::OnMessage_STATE(CGNETMSG& _Msg)
//{
//	//// Check) Message의 길이가 잘못되었으면 Exception을 던진다.
//	//ERROR_THROW_IF(_Msg.Buffer.len != sizeof(PACKET_STATE), CGException::CNetThrow_InvalidMessageLength(), CGLOG_EXCEPTION(g_pLog, _CGTEXT("Invalid Message Size [Received:%d - Require %d] (") _CGTEXT(__FUNCTION__) _CGTEXT(")"), _Msg.Buffer.len, sizeof(PACKET_STATE)));
//
//	//// 1) Socket을 얻는다.
//	//user::socket*	psocket	 = _Msg.GetSource<user::socket>();
//
//	//// Check) Socket이 nullptr이면 Exception을 던진다.
//	//ERROR_THROW_IF(psocket==nullptr, CGException::CNetThrow_InvalidSocketHandle(), CGLOG_EXCEPTION(g_pLog, _CGTEXT("Invalid Source socket (") _CGTEXT(__FUNCTION__) _CGTEXT(")") ));
//
//	//// 2) Message를 읽어낸다.
//	//const PACKET_STATE&	message	 = _Msg.Buffer.front<PACKET_STATE>();
//
//	//// 3) Game Server info를 써넣는다.
//	//SCOPED_LOCK(psocket->m_infoGameServer)
//	//{
//	//	psocket->m_infoGameServer	 = message.m_infoGameServer;
//	//}
//
//	//// 4) Server Info flag를 Signal!
//	//m_bServerInfoUpdated.store(true);
//
//	//// View)
//	//transmit_message_to(g_pservice, sMESSAGE_NOTIFY(CGMESSAGE::WINDOWS::NOTIFY_UPDATE, CGNOTIFY_GAMESERVER_UPDATE, (DWORD_PTR)psocket));
//
//	// return) 
//	return	1;
//}

//shared_buffer user::group::Message_SERVER_LIST()
//{
//	// Declare) 
//	shared_buffer	buf_temp;
//
//	SCOPED_LOCK(GetGroupLock())
//	{
//		// 1) 버퍼를 할당받는다.
//		buf_temp = alloc_shared_buffer(sizeof(PACKET_SERVER_LIST) + GetMemberCount()*sizeof(SServerListInfo));
//
//		// 2) Message Headm를 써넣는다.
//		auto&	messageHead = buf_temp.append<PACKET_SERVER_LIST>();
//
//		// 3) 개수를 초기화
//		messageHead.Page	 = 0;
//
//		// 2) 만들기..
//		for(auto& iter:*this)
//		{
//			// Check)
//			CONTINUE_IF(iter->m_infoGameServer.State==-1);
//
//			// - 데이터를 추가한다.
//			buf_temp.append<SServerListInfo>(iter->m_infoGameServer);
//
//			// - 개수 증가
//			++messageHead.Page;
//		}
//	}
//
//	// return)
//	return	buf_temp;
//}
