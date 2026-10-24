#include "pch.h"


object_ptr<server::socket> server::group_channel::get_suitable_channel_server()
{
	// Declare) 
	object_ptr<socket> psocket;
	
	SCOPED_LOCK(this->m_lockable_group)
	{
		// check) 서버가 하나도 없으면 nullptr을 리턴한다.
		RETURN_IF(is_member_empty()==true, object_ptr<socket>());

		// 설명) 적절한 Server를 선택하는 알고리즘을 선택한다.
		//       다양한 선택 방법 알고리즘이 있을 수 있다.
		//       여기서는 예시만을 위해 가장 간단한 방법의 서버 선택방법으로 작성하였다.
		//
		#define	SELECT_TYPE	1		// 그냥 제일 앞의 서버
		//#define	SELECT_TYPE	2	// Player Count가 제일 적은 서버

		// 1) Server 선택
		#if SELECT_TYPE==1
			psocket	 = begin()->member();
		#elif SELECT_TYPE == 2
			// 1) 전체를 Loop돌며 찾아도 된다.
			int	countPlayerMin	 = INT_MAX;
			for(auto iter=begin(); iter!=end(); ++iter)
			{
				// Declare) 
				int	countPlayerNow;

				// - 현재 플레이어수를 저장한다.
				SCOPED_LOCK(iter->member())
				{
					countPlayerNow = iter->member()->m_server_info.nServer_Player_Count;
				}

				if(countPlayerMin>countPlayerNow)
				{
					countPlayerMin = countPlayerNow;
					psocket = iter->member();
				}
			}
		#endif
	}

	// return)
	return	psocket;
}

void server::group_channel::on_start(context& /*_context*/)
{
	// 1) 10초마다 한번씩 호출된다.
	this->execute_interval(10s);

	// 2) 실행 등록한다.
	system_executor::register_schedulable(this);

	// 3) 입장을 받기 시작~
	this->enable_member_enter(true);
}

void server::group_channel::on_stopping()
{
	// 1) 입장을 금지
	this->disable_member_enter();

	// 2) 실행 등록 해제한다.
	system_executor::unregister_schedulable(this);

	// 3) 모든 Group에 접속한 모든 Member를 모두 접속 종료시킨다.
	for_each_member([=](socket* _pmember)->bool
	{
		_pmember->closesocket(); 
		return true;
	});
}
							
void server::group_channel::on_member_entered(socket* _pmember, sMESSAGE& /*_param*/)
{
	_pmember->register_messageable(this);
}

void server::group_channel::on_member_leaved(socket* _pmember, uintptr_t /*_result*/)
{
	_pmember->unregister_messageable(this);
}

result_code server::group_channel::on_message(sMESSAGE_NETWORK& _msg)
{
	// 1) message map
	BEGIN_NESTED_CGMESSAGE_MAP
		CGMESSAGE_TYPE_SUB(eMESSAGE::SYSTEM::NETWORK, ((sMESSAGE_BUFFER&)_msg).buf_message.front<uint32_t>(4))
			ON_CGMESSAGE(eMESSAGE::SEND_LOBBYSERVER_STATUS,		on_message_SEND_LOBBYSERVER_STATUS,		sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::REQUEST_ACCOUNT_CHECKIN,		on_message_REQUEST_ACCOUNT_CHECKIN,		sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::REQUEST_ACCOUNT_CHECKOUT,	on_message_REQUEST_ACCOUNT_CHECKOUT,	sMESSAGE_NETWORK);
			END_SUB_CGMESSAGE_MAP
	END_NESTED_CGMESSAGE_MAP

	// return)
	return 0;
}

result_code server::group_channel::on_message_SEND_LOBBYSERVER_STATUS(sMESSAGE_NETWORK& _msg)
{
	// 1) Message Casting
	auto psocket = _msg.get_source<socket>();

	// 2) SERVER_INFO를 저장한다.
	SCOPED_LOCK(psocket->m_lock_server_info)
	{
		psocket->m_server_info = _msg.buf_message.front<SERVER_INFO>(8);
	}

	// return)
	return 1;
}

result_code server::group_channel::on_message_REQUEST_ACCOUNT_CHECKIN(sMESSAGE_NETWORK& _msg)
{
	// 1) Message Casting
	auto psocket = _msg.get_source<user::socket>();

	// check) psocket이 nullptr이면 안 된다.
	CGASSERT_ERROR(psocket != nullptr);

	// 2) AccountID를 얻는다.
	auto transaction_id = _msg.buf_message.front<uint32_t>(8);
	auto& account_id = _msg.buf_message.front<ACCOUNT_ID>(12);

	// check) transaction_id를 확인한다.
	RETURN_IF(transaction_id==0, 1);

	// check) account_id를 확인한다.
	RETURN_IF(account_id.valid()==false, 1);

	try
	{
		// 3) 제대로된 login인지 AccountID를 확인한다.
		auto result = g_pservice->pmanager_account->request_account_checkin(account_id);

		// check) checkin에 실패했으면 Exception을 던진다.
		THROW_IF(result!=0, throwable::failure(result, 0, "error"sv));

		// 4) 성공 메시지를 전송한다.
		psocket->send(message_RESPONSE_ACCOUNT_CHECKIN(0, transaction_id, account_id));
	}
	catch(...)
	{
		// - 실패 메시지를 전송한다.
		psocket->send(message_RESPONSE_ACCOUNT_CHECKIN(1, transaction_id, account_id));

		// - 접속을 종료한다.
		psocket->disconnect();
	}

	// return)
	return 1;
}

result_code server::group_channel::on_message_REQUEST_ACCOUNT_CHECKOUT(sMESSAGE_NETWORK& _msg)
{
	// 1) AccountID를 얻는다.
	auto account_id = _msg.buf_message.front<ACCOUNT_ID>(8);

	// 2) checkout 처리를 한다.
	g_pservice->pmanager_account->request_account_checkout(account_id);

	// return)
	return 1;
}

void server::group_channel::on_execute()
{
}

shared_buffer server::group_channel::message_RESPONSE_ACCOUNT_CHECKIN(int _result, uint32_t _transaction_id, const ACCOUNT_ID& _account_id)
{
	// 1) 버퍼를 할당받는다.
	auto buf_temp = alloc_shared_buffer(64);

	// 2) 메시지 작성
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::RESPONSE_ACCOUNT_CHECKIN);
	buf_temp.append<uint32_t>(_transaction_id);
	buf_temp.append<int>(_result);
	buf_temp.append<ACCOUNT_ID>(_account_id);

	// 3) 메시지의 제일 앞에 메시지의 길이를 써넣는다.
	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

	// return) 
	return buf_temp;
}
