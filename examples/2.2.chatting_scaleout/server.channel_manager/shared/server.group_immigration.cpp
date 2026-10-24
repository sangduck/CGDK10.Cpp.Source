#include "pch.h"


void server::group_immigration::on_start(context& /*_context*/)
{
	// 1) 호출 간격을 10초로 설정
	this->execute_interval(10s);

	// 2) schedulable을 등록한다.
	system_executor::register_schedulable(this);

	// 3) 입장을 받기 시작~
	this->enable_member_enter(true);
}

void server::group_immigration::on_stopping()
{
	// 1) 입장을 금지
	this->disable_member_enter();

	// 2) schedulable에서 등록해제한다.
	system_executor::unregister_schedulable(this);

	// 3) group의 모든 socket 접속을 종료한다.
	for_each_member([=](socket* _pmember)->bool
		{
			// - leave group
			_pmember->leave_group();

			// - close socket
			_pmember->closesocket();

			// return) 
			return true;
		});
}
							
void server::group_immigration::on_member_entered(socket* _pmember, sMESSAGE& /*_param*/)
{
	// 1) register messageable
	_pmember->register_messageable(this);
}

void server::group_immigration::on_member_leaved(socket* _pmember, uintptr_t /*_result*/)
{
	// 1) unregister messageable
	_pmember->unregister_messageable(this);

	// 2) cancel command

}

void server::group_immigration::on_execute()
{
	// 1) 현재 시간을 얻는다.
	auto tick_now = chrono::tick::now();

	// 2) 접속한지 10초가 지났는데도 여전히 Immigration에 있다면 접속을 강제로 끊는다.
	for_each_member([=](socket* _pmember)
		{
			if (_pmember->statistics_get_tick_connect() + 10s < tick_now)
			{
				//_pmember->closesocket();
			}
			return true;
		});
}

result_code server::group_immigration::on_message(sMESSAGE_NETWORK& _msg)
{
	// 1) execute message
	BEGIN_NESTED_CGMESSAGE_MAP
		CGMESSAGE_TYPE_SUB(eMESSAGE::SYSTEM::NETWORK, ((sMESSAGE_BUFFER&)_msg).buf_message.front<uint32_t>(4))
			ON_CGMESSAGE(eMESSAGE::REQUEST_CERTIFICATION, on_message_REQUEST_CERTIFICATION, sMESSAGE_NETWORK);
		END_SUB_CGMESSAGE_MAP
	END_NESTED_CGMESSAGE_MAP
	
	// return) 
	return eRESULT::BYPASS;
}

result_code server::group_immigration::on_message_REQUEST_CERTIFICATION(sMESSAGE_NETWORK& _msg)
{
	// 1) Message Casting
	auto psocket = _msg.get_source<server::socket>();

	// check) psocket이 nullptr이면 안 된다.
	CGASSERT_ERROR(psocket != nullptr);

	// 2) Server 정보를 저장한다.
	SCOPED_LOCK(psocket->m_lock_server_info)
	{
		psocket->m_server_info = _msg.buf_message.front<SERVER_INFO>(8);
	}

	// 3) Name을 읽어들인다.
	psocket->m_strServerName = _msg.buf_message.front<std::string>(8+sizeof(SERVER_INFO));

	// 4) ACCEPT_CERTIFICATION Message를 전송한다.
	psocket->send(message_ACCEPT_CERTICIATION());

	// 5) group Login으로 옮긴다.
	g_pservice->pgroup_server_channel->enter(psocket);

	// View)
	transmit_message_to(g_pservice, sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, CGNOTIFY_LOBBYSERVER_UPDATE, (DWORD_PTR)psocket));

	// return)
	return eRESULT::DONE;
}

shared_buffer server::group_immigration::message_ACCEPT_CERTICIATION()
{
	// 1) 버퍼를 할당받는다.
	auto buf_temp  = alloc_shared_buffer(32);

	// 2) 메시지를 작성한다.
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::ACCEPT_CERTIFICATION);

	// 3) 메시지의 제일 앞에 메시지의 길이를 써넣는다.
	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

	// return) 
	return buf_temp;
}
