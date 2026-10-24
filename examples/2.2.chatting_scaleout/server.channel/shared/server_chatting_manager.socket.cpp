#include "pch.h"


void server_channel_manager::socket::request_account_checkin(const ACCOUNT_ID& /*_account_id*/, user::socket* /*_psource*/)
{
	//// Declare) 
	//uint32_t	idTransaction	 = 0;

	//SCOPED_LOCK(m_request->m_map_request)
	//{
	//	// 1) Request를 먼저 등록한다.
	//	idTransaction	 = m_request->RegisterRequest(_psource);

	//	// 2) Account ID와 idTransaction를 저장한다.
	//	_psource->m_account_id.reset();
	//	_psource->m_transaction_id = idTransaction;
	//}

	//try
	//{
	//	// Declare)
	//	bool bResult;

	//	// 3) Login Server에 CHECKIN을 요청한다.
	//	bResult = send(message_REQUEST_ACCOUNT_CHECKIN(_account_id, idTransaction));

	//	// Check) send에  실패했을 경우 Cancel처리를 한다.
	//	ERROR_THROW_IF(bResult==false, CGException::CThrow_Error(), );
	//}
	//catch(...)
	//{
	//	SCOPED_LOCK(m_request->m_map_request)
	//	{
	//		// - Transaction을 제거
	//		m_request->UnregisterRequest(idTransaction);

	//		// - Reset
	//		_psource->m_transaction_id	 = 0;
	//	}

	//	// Reraise) 
	//	throw;
	//}
}

void server_channel_manager::socket::request_account_checkout(const ACCOUNT_ID& _account_id)
{
	// Check) 제대로된 Account ID가 아니라면 false를 리턴한다.
	RETURN_IF(_account_id.valid()==false, )

	// 1) Account ...
	this->send(message_REQUEST_ACCOUNT_CHECKOUT(_account_id));
}

void server_channel_manager::socket::cancel_account_checkin(uint32_t /*_transaction_id*/)
{
	//SCOPED_LOCK(m_request->m_map_request)
	//{
	//	// 1) 얻는다.
	//	auto	psocket	 = m_request->UnregisterRequest(_transaction_id);

	//	// Check) Socket을 찾지 못했으면 그냥 끝낸다.
	//	RETURN_IF(psocket.empty(), );

	//	// 2) Transaction id르 reset한다.
	//	psocket->m_transaction_id	 = 0;
	//}
}

void server_channel_manager::socket::on_connect()
{
	// trace)
	LOG_INFO << log_level_t::higher() << "< server connected ["sv << to_string(this->remote_endpoint()) << "]";

	// View)
	transmit_message_to(g_pservice, sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, CGNOTIFY_SERVER_SOCKET_ADD, (DWORD_PTR)this));

	// 2) Certification을 요청하는 메시지를 전송한다.(성공한다면 ACCEPT_CERTIFICATION 메시지를 보내 올 것이다.)
	this->send(g_pservice->message_REQUEST_CERTIFICATION());
}

void server_channel_manager::socket::on_disconnect(uint64_t /*_reason*/)
{
	// View)
	transmit_message_to(g_pservice, sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, CGNOTIFY_SERVER_SOCKET_REMOVE, (DWORD_PTR)this));

	// trace)
	LOG_INFO << log_level_t::higher() << "< server Disconnected ["sv << to_string(this->remote_endpoint())  << "]";

	// 1) schedulable에서 Unregister한다.
	this->unregister_messageable(this);
}

void server_channel_manager::socket::on_execute()
{
	// 1) Player Count를 UPdate한다.
	g_pservice->update_player_count();

	// 2) send Message
	this->send(g_pservice->message_SEND_LOBBYSERVER_STATUS());
}

result_code server_channel_manager::socket::on_message(sMESSAGE_NETWORK& _msg)
{
	// 1) set source
	_msg.set_source(this);

	try
	{
		// 2) execute message
		BEGIN_NESTED_CGMESSAGE_MAP
			CGMESSAGE_TYPE_SUB(eMESSAGE::SYSTEM::NETWORK, ((sMESSAGE_BUFFER&)_msg).buf_message.front<uint32_t>(4))
				ON_CGMESSAGE(eMESSAGE::ACCEPT_CERTIFICATION,		on_message_ACCEPT_CERTIFICATION,		sMESSAGE_NETWORK);
				ON_CGMESSAGE(eMESSAGE::RESPONSE_ACCOUNT_CHECKIN,	on_message_RESPONSE_ACCOUNT_CHECKIN,	sMESSAGE_NETWORK);
			END_SUB_CGMESSAGE_MAP
		END_NESTED_CGMESSAGE_MAP

	}
	catch (...)
	{
		// Error!)
		CGASSERT_ERROR(false);
	}

	// return) 
	return eRESULT::BYPASS;
}

result_code server_channel_manager::socket::on_message_ACCEPT_CERTIFICATION(sMESSAGE_NETWORK& /*_msg*/)
{
	// 1) 2초에 한번씩 OnExecute를 호출하게 한다.(이때 서버 상태 정보를 전송한다.)
	this->execute_interval(2s);

	// 2) Schedulable을 시작한다.
	this->register_messageable(this);

	// return) 
	return	1;
}

result_code server_channel_manager::socket::on_message_RESPONSE_ACCOUNT_CHECKIN(sMESSAGE_NETWORK& /*_msg*/)
{
	//auto idTransaction = _Msg.Buffer.front<uint32_t>(8);
	//auto result = _Msg.Buffer.front<int>(12);
	//auto account_id = _Msg.Buffer.front<ACCOUNT_ID>(16);

	//try
	//{
	//	// Declare) 
	//	object_ptr<user::socket> psocket;

	//	SCOPED_LOCK(m_request->m_map_request)
	//	{
	//		// 2) 등록된 Request에서 psocket값을 얻는다.
	//		psocket = m_request->UnregisterRequest(idTransaction);

	//		// Check) 찾지 못했으면 Exception을 던진다.
	//		ERROR_THROW_IF(psocket.empty(), CGException::CThrow_NotExist(), )

	//		// 3) Socket의 Transaction id를 리셋한다.
	//		psocket->m_transaction_id = 0;

	//		// 4) 성공했을 경우 AccountiD를 설정한다.
	//		if(result==0)
	//		{
	//			psocket->m_account_id = account_id;
	//		}
	//	}

	//	// 4) 결과를 그대로 전송한다.
	//	psocket->send(_Msg.Buffer);
	//}
	//catch(...)
	//{
	//	// - 바로 Check-out을 요청한다.
	//	send(message_REQUEST_ACCOUNT_CHECKOUT(account_id));
	//}

	// return) 
	return 1;
}

shared_buffer server_channel_manager::socket::message_REQUEST_ACCOUNT_CHECKIN(const ACCOUNT_ID& _account_id, uint32_t _transaction_id)
{
	auto buf_temp = alloc_shared_buffer(256);

	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::REQUEST_ACCOUNT_CHECKIN);
	buf_temp.append<uint32_t>(_transaction_id);
	buf_temp.append<ACCOUNT_ID>(_account_id);

	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

	return buf_temp;
}

shared_buffer server_channel_manager::socket::message_REQUEST_ACCOUNT_CHECKOUT(const ACCOUNT_ID& _account_id)
{
	auto buf_temp = alloc_shared_buffer(256);

	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::REQUEST_ACCOUNT_CHECKOUT);
	buf_temp.append<ACCOUNT_ID>(_account_id);

	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

	return buf_temp;
}
