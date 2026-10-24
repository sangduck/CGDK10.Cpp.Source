#include "pch.h"


void user::group_immigration::on_start(context& /*_context*/)
{
	// 1) 호출 간격을 10초로 설정
	this->execute_interval(10s);

	// 2) schedulable을 등록한다.
	system_executor::register_schedulable(this);

	// 3) 입장을 받기 시작~
	this->enable_member_enter(true);
}

void user::group_immigration::on_stopping()
{
	// 1) 입장을 금지
	this->disable_member_enter();

	// 2) schedulable에서 등록해제한다.
	system_executor::unregister_schedulable(this);

	// 3) group의 모든 socket 접속을 종료한다.
	for_each_member([=](user::socket* _pmember)->bool
		{
			// - leave group
			_pmember->leave_group();

			// - close socket
			_pmember->closesocket();

			// return) 
			return true;
		});
}

void user::group_immigration::on_member_entered(user::socket* _pmember, sMESSAGE& /*_param*/)
{
	// 1) register messageable
	_pmember->register_messageable(this);
}

void user::group_immigration::on_member_leaved(user::socket* _pmember, uintptr_t /*_result*/)
{
	// 1) unregister messageable
	_pmember->unregister_messageable(this);

	// 2) cancel command

}

void user::group_immigration::on_execute()
{
	// 1) 현재 시간을 얻는다.
	auto tick_now = chrono::tick::now();

	// 2) 접속한지 10초가 지났는데도 여전히 Immigration에 있다면 접속을 강제로 끊는다.
	for_each_member([=](user::socket* _pmember)
		{
			if (_pmember->statistics_get_tick_connect() + 10s < tick_now)
			{
				//_pmember->closesocket();
			}
			return true;
		});
}

result_code user::group_immigration::on_message(sMESSAGE_NETWORK& _msg)
{
	// 1) execute message
	BEGIN_NESTED_CGMESSAGE_MAP
		CGMESSAGE_TYPE_SUB(eMESSAGE::SYSTEM::NETWORK, ((sMESSAGE_BUFFER&)_msg).buf_message.front<uint32_t>(4))
			ON_CGMESSAGE(eMESSAGE::REQUEST_ACCOUNT_CHECKIN, on_message_REQUEST_ACCOUNT_CHECKIN, sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::RESPONSE_ACCOUNT_CHECKIN, on_message_RESPONSE_ACCOUNT_CHECKIN, sMESSAGE_NETWORK);
		END_SUB_CGMESSAGE_MAP
	END_NESTED_CGMESSAGE_MAP


	// return) 
	return eRESULT::BYPASS;
}

result_code user::group_immigration::on_message_REQUEST_ACCOUNT_CHECKIN(sMESSAGE_NETWORK& _msg)
{
	// 1) 
	auto msg_recv = _msg.buf_message + offset(8);
	auto psocket = _msg.get_source<user::socket>();

	// 2) ACCOUNT_ID를 읽는다.
	auto& account_id = msg_recv.front<ACCOUNT_ID>();

	try
	{
		// 3) Checkin을 요청한다.
		g_pservice->psocket_server_channel_manager->request_account_checkin(account_id, psocket);
	}
	catch(...)
	{
		// - Checkin 실패를 전송한다.
		psocket->send(message_RESPONSE_ACCOUNT_CHECKIN(1, 0, account_id));

		// - 접속을 끊는다.
		psocket->disconnect();
	}

	// return)
	return	1;
}

result_code user::group_immigration::on_message_RESPONSE_ACCOUNT_CHECKIN(sMESSAGE_NETWORK& /*_msg*/)
{
	// 1) psocket에 Result를 전송!


	//g_pservice->psocket_server_channel_manager->request_account_checkin(_Msg.Buffer, );


	// return)
	return	1;
}

//shared_buffer user::group_immigration::Message_ACCEPT_CERTICIATION()
//{
//	// 1) 버퍼 할당
//	shared_buffer	buf_temp = alloc_shared_buffer(sizeof(PACKET_ACCEPT_CERTIFICATION));
//
//	// 2) Message 작성
//	buf_temp.append<PACKET_ACCEPT_CERTIFICATION>();
//
//	// return) 
//	return	buf_temp;
//}

shared_buffer user::group_immigration::message_RESPONSE_ACCOUNT_CHECKIN(int _result, uint32_t _transaction_id, const ACCOUNT_ID& _account_id)
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