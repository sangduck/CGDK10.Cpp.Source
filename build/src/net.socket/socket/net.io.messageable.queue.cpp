#include "pch.h"

using namespace CGDK;

class executable_post_messageable : virtual public Iexecutable
{
private:
	virtual	intptr_t process_execute(intptr_t /*_result*/, size_t /*_param*/)
	{
		// check)
		CGASSERT_ERROR(m_pmessageable.exist());

		// 1) process_message
		auto result = m_pmessageable->process_message(m_message_network);

		// check) ...
		RETURN_IF(result == eRESULT::PENDED, 0);

		// 2) process_message를 실행한다.
		return m_pmessageable->continue_process_message();
	}
public:
	sMESSAGE_NETWORK m_message_network;
	object_ptr<net::io::messageable::Nqueue> m_pmessageable;
};

result_code net::io::messageable::Nqueue::request_process_message(sMESSAGE& _msg)
{
	// check) 
	RETURN_IF(is_messageable_disabled(), eRESULT::FAIL_DISABLED);

	// declare)
	sMESSAGE_NETWORK& msg = static_cast<sMESSAGE_NETWORK&>(_msg);

	SCOPED_LOCK(this->m_lockable_queue_message)
	{
		// 1) 현재 실행 중일 경우 queuing 시킨 후 리턴한다.
		if (this->m_flag_processing == true)
		{
			this->m_queue_message.push_back(msg);

			// return)
			return eRESULT::PENDED;
		}

		// 2) set flag true
		this->m_flag_processing = true;
	}

	// 3) process_message
	auto result = process_message(_msg);

	// check) ...
	RETURN_IF(result == eRESULT::PENDED, eRESULT::PENDED);

	// 4) process_message를 실행한다.
	return this->continue_process_message();
}

result_code net::io::messageable::Nqueue::request_post_process_message(sMESSAGE_NETWORK& _msg)
{
	// check) 
	RETURN_IF(is_messageable_disabled(), eRESULT::FAIL_DISABLED);

	SCOPED_LOCK(this->m_lockable_queue_message)
	{
		// 1) 현재 실행 중일 경우 queuing 시킨 후 리턴한다.
		if (this->m_flag_processing == true)
		{
			this->m_queue_message.push_back(_msg);

			// return)
			return eRESULT::PENDED;
		}

		// 2) set flag true
		this->m_flag_processing = true;
	}

	// 3) make executable_object
	auto pexecutable_post_messageable = make_object<executable_post_messageable>();

	// 4) set value
	pexecutable_post_messageable->m_message_network = _msg;
	pexecutable_post_messageable->m_pmessageable = this;

	// 5) post execute (system executor : 참조 count 없이 post한다)
	auto result = system_executor::post(std::move(pexecutable_post_messageable));

	// check) post 실패 (flag를 내린다)
	if (result == false)
	{
		this->_reset_processing_flag();
		return eRESULT::FAIL;
	}

	// return) 
	return eRESULT::PENDED;
}

void net::io::messageable::Nqueue::_reset_processing_flag() noexcept
{
	SCOPED_LOCK(this->m_lockable_queue_message)
	{
		this->m_flag_processing = false;
	}
}

result_code net::io::messageable::Nqueue::continue_process_message()
{
	// declare)
	sMESSAGE_NETWORK* pmsg;

	// 1) get front message
	//    - 비어 있으면 같은 lock 안에서 flag를 내린다.
	//      (lock을 풀었다가 다시 잡아서 내리면 그 사이 다른 thread가 queuing한 메시지(flag가 true라 queuing만 함)가 처리되지 않고 남는다)
	SCOPED_LOCK(this->m_lockable_queue_message)
	{
		// check)
		CGASSERT_ERROR(this->m_flag_processing == true);

		if (this->m_queue_message.empty())
		{
			this->m_flag_processing = false;
			return eRESULT::DONE;
		}

		pmsg = &this->m_queue_message.front();
	}

	for (;;)
	{
		// - process_message (lock 밖. 이 node는 pop_front 전까지 그대로다)
		auto result = this->process_message(*pmsg);

		// 2) pop message
		SCOPED_LOCK(this->m_lockable_queue_message)
		{
			// - pop_front
			this->m_queue_message.pop_front();

			// check)
			RETURN_IF(result == eRESULT::PENDED, eRESULT::PENDED);

			// check) 다 처리했으면 같은 lock 안에서 flag를 내리고 끝낸다.
			if (this->m_queue_message.empty())
			{
				this->m_flag_processing = false;
				return eRESULT::DONE;
			}

			// - get front
			pmsg = &this->m_queue_message.front();
		}
	}
}

result_code net::io::messageable::Nqueue::process_message(sMESSAGE& _msg)
{
	// declare) 
	result_code	result = eRESULT::BYPASS;

	// 1) call 'on_message' function
	result = this->on_message(static_cast<sMESSAGE_NETWORK&>(_msg));

	// return)
	return result;
}
