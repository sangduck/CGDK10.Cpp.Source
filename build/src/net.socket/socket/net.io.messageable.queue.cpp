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

	scoped_lock(this->m_lockable_queue_message)
	{
		// 1) 현재 실행중일 경우 queuing 시킨 후 리턴한다.
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

	scoped_lock(this->m_lockable_queue_message)
	{
		// 1) 현재 실행중일 경우 queuing 시킨 후 리턴한다.
		if (this->m_flag_processing == true)
		{
			this->m_queue_message.push_back(_msg);

			// return)
			return eRESULT::PENDED;
		}

		// 2) set flag true
		this->m_flag_processing = true;
	}

	// 3) get executor
	auto pexecutor = system_executor::executor();

	// check)
	RETURN_IF(pexecutor.empty(), eRESULT::NOT_READY);

	// 3) make executable_object
	auto pexecutable_post_messageable = make_object<executable_post_messageable>();

	// 4) set value
	pexecutable_post_messageable->m_message_network = _msg;
	pexecutable_post_messageable->m_pmessageable = this;

	// 5) post execute
	auto result = pexecutor->post(pexecutable_post_messageable);

	// check)
	RETURN_IF(result == false, eRESULT::FAIL);

	// return) 
	return eRESULT::PENDED;
}

result_code net::io::messageable::Nqueue::continue_process_message()
{
	// declare) 
	sMESSAGE_NETWORK* pmsg;

	// 1) get front message
	scoped_lock(this->m_lockable_queue_message)
	{
		pmsg = (this->m_queue_message.empty() == false)  ? &this->m_queue_message.front() : nullptr;
	}

	if (pmsg != nullptr)
	{
		for (;;)
		{
			// check)
			CGASSERT_ERROR(this->m_flag_processing == true);

			// - process_message
			auto result = this->process_message(*pmsg);

			// 3) pop message
			scoped_lock(this->m_lockable_queue_message)
			{
				// - pop_front
				this->m_queue_message.pop_front();

				// check) 
				RETURN_IF(result == eRESULT::PENDED, eRESULT::PENDED);

				// check) 
				BREAK_IF(this->m_queue_message.empty());

				// - get front
				pmsg = &this->m_queue_message.front();
			}
		}
	}

	// 4) done...
	scoped_lock(this->m_lockable_queue_message)
	{
		// check)
		CGASSERT_ERROR(this->m_flag_processing == true);

		// - set flag false
		this->m_flag_processing = false;
	}

	// return) 
	return eRESULT::DONE;
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
