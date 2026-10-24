#include "pch.h"


shared_buffer message_USER_LOGIN_RESPONSE(REQUEST_ID _request_id, eRESULT _result, const sACCOUNT_INFO& _account_info)
{
	// 1) alloc buffer
	auto buf_temp = alloc_shared_buffer(8 + get_size_of(_request_id) + get_size_of(_result) + get_size_of(_account_info));

	// 2) writee message
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::USER_LOGIN_RESPONSE);

	// 3) object_event를 append한다.
	buf_temp.append<REQUEST_ID>(_request_id);
	buf_temp.append<eRESULT>(_result);
	buf_temp.append<sACCOUNT_INFO>(_account_info);

	// 4) write messagee length
	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

	// return)
	return buf_temp;
}

shared_buffer message_USER_LOGIN_RESPONSE(REQUEST_ID _request_id, eRESULT _result)
{
	// 1) alloc buffer
	auto buf_temp = alloc_shared_buffer(8 + get_size_of(_request_id) + get_size_of(_result));

	// 2) writee message
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::USER_LOGIN_RESPONSE);

	// 3) object_event를 append한다.
	buf_temp.append<REQUEST_ID>(_request_id);
	buf_temp.append<eRESULT>(_result);

	// 4) write messagee length
	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

	// return)
	return buf_temp;
}

void group_user_immigration::on_starting(context& /*_context*/)
{
	// 1) enable member entering
	this->enable_member_enter();

	// 2) set executing interval
	this->execute_interval(10s);

	// 3) reigister to system scheduler
	system_executor::register_schedulable(this);

	// notify)
	transmit_message_to(g_pservice, sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, CGNOTIFY_AREA_GROUP_ADD, (uint64_t)reinterpret_cast<Igroup_user*>(this)));
}

void group_user_immigration::on_stopping()
{
	// 1) disable member entering
	this->disable_member_enter();

	// 2) unreigister from system scheduler
	system_executor::unregister_schedulable(this);

	// notify)
	transmit_message_to(g_pservice, sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, CGNOTIFY_AREA_GROUP_REMOVE, (uint64_t)reinterpret_cast<Igroup_user*>(this)));
}

void group_user_immigration::on_execute()
{
	// 1) 일정시간(1분)이상 immigration에 머물러 있는 socket들을 접속을 끊어버린다.
	this->process_disconnect_timeout_sockets();
}

result_code group_user_immigration::on_member_entering(socket_user* _pmember, sMESSAGE& /*_msg*/)
{
	// 1) Register Messageable한다.
	_pmember->register_messageable(this);

	// return) 
	return	eRESULT::SUCCESS;
}

uintptr_t group_user_immigration::on_member_leaving(socket_user* _pmember, uintptr_t _param)
{
	// 1) unregister Mediator한다.
	_pmember->unregister_messageable(this);

	// return) 
	return _param;
}

result_code group_user_immigration::on_message(sMESSAGE_NETWORK& _msg)
{
	// 1) execute message
	BEGIN_NESTED_CGMESSAGE_MAP
		CGMESSAGE_TYPE_SUB(eMESSAGE::SYSTEM::NETWORK, ((sMESSAGE_BUFFER&)_msg).buf_message.front<uint32_t>(4))
			ON_CGMESSAGE(eMESSAGE::USER_LOGIN_REQUEST,		on_message_USER_LOGIN_REQUEST,		sMESSAGE_NETWORK);
		END_SUB_CGMESSAGE_MAP
	END_NESTED_CGMESSAGE_MAP

	// return) 
	return eRESULT::DONE;
}

result_code group_user_immigration::on_message_USER_LOGIN_REQUEST(sMESSAGE_NETWORK& _msg)
{
	// 1) get socket server
	auto psocket_user = _msg.get_source<socket_user>();

	// 2) channel info를 얻는다.
	auto buf = _msg.buf_message + offset(8);

	// 3) get area id
	auto request_id = buf.extract<REQUEST_ID>();

	// 4) get user id
	auto account_index = buf.extract<ACCOUNT_INDEX>();

	try
	{
		// 5) a..
		auto paccount = g_pservice->m_paccount_manager->login(psocket_user, account_index);

		// check)
		THROW_IF(paccount.empty(), throwable::failure(eRESULT::ACCESS_DENIED, 0, "fail to login"sv))

		// declare)
		shared_buffer buf_send;

		// 6) make login response message
		SCOPED_LOCK(paccount->cs_account_info)
		{
			buf_send = message_USER_LOGIN_RESPONSE(request_id, eRESULT::SUCCESS, paccount->account_info);
		}

		// 7) set account
		psocket_user->set_account(std::move(paccount));

		// 8) send result message
		psocket_user->send(buf_send);
	}
	catch (throwable::failure& _e)
	{
		psocket_user->send(message_USER_LOGIN_RESPONSE(request_id, _e.reason()));

		// return) 
		return eRESULT::DONE;
	}
	catch (...)
	{
		// - disconenct
		psocket_user->closesocket();

		// return) 
		return eRESULT::DONE;
	}

	try
	{
		// 8) account를 얻는다.
		auto paccount = psocket_user->get_account();

		// check) 
		THROW_IF(paccount.empty(), throwable::failure(eRESULT::NOT_ALLOWED, 0, "account is empty"sv));

		// check)
		CGASSERT_ERROR(paccount->parea_entity.exist());

		// 9) get area_index now
		auto area_index_start = paccount->area_index_now;

		// 10) if first.set suitable area_index_now == 0
		if (area_index_start == 0)
		{
			area_index_start = 1;
		}
		
		// 11) get area - 입장할 area를 얻는다.
		auto pgroup_area = g_pservice->m_parea_mamanger->find_area(area_index_start);

		// check)
		THROW_IF(pgroup_area.empty(), throwable::failure(eRESULT::NOT_EXIST, 0, ""sv));

		// 12) 입장~~!!
		auto result = pgroup_area->enter(psocket_user);

		// check)
		THROW_IF(result != eRESULT::SUCCESS, throwable::failure(result, 0, ""sv));
	}
	catch (...)
	{
		psocket_user->reset_account();
		psocket_user->closesocket();
	}

	// return) 
	return eRESULT::DONE;
}

void group_user_immigration::process_disconnect_timeout_sockets()
{
	// 1) 현재 시간을 얻는다.
	auto time_now = chrono::time::utc::now();

	// declare) 
	std::vector<object_ptr< socket_user>> vector_socket_time_out;

	// 2) 접속한지 1분 이상 지난 socket들을 골래난다.
	SCOPED_LOCK(this->get_group_lockable())
	{
		for (auto& iter : *this)
		{
			if (time_now - iter->statistics_get_connect_time() > 1min)
			{
				vector_socket_time_out.push_back(*iter);
			}
		}
	}

	// 3) 모두 강제 접속 종료
	for (auto& iter : vector_socket_time_out)
		iter->closesocket();
}
