#include "pch.h"

namespace CGDK
{

result_code object::Nstartable::start(context& _context)
{
	// 1) 먼저 initialize한다.
	result_code is_initialize = object::initialize(this, _context);

	// 2) Nameable 객체 포인터를 얻고 현재의 Context를 저장해 놓는다.
	auto* pnameable = dynamic_cast<object::Inameable*>(this);
	auto  pname = (pnameable != nullptr && pnameable->name().empty() == false) ? pnameable->name() : std::string();

	// 3) get now context
	auto* pcontext_now = &_context;

	// 4) context를 변경한다.
	if (pname.empty() == false)
	{
		auto& context_find = _context[pname];

		if (context_find.is_valid())
			pcontext_now = &context_find;
	}

	// 5) start parameters
	{
		// - ignore failure
		if (std::string str_find = pcontext_now->find("ignore_exception_on_start"sv, "ignore failure on start"sv); str_find.empty() == false)
		{
			// - true면 재접속을 활성화한다.
			if (CGDK::__stricmp(str_find, "true") == 0
				|| CGDK::__stricmp(str_find, "enable") == 0)
			{
				// -  block_util_completed을 enable한다.
				this->enable_ignore_exception_on_start();

				// trace)
				LOG_INFO << "  + parameter['ignore_exception_on_start']: 'enable'"sv;
			}
			// - false면 재접속을 비활성화한다.
			else if (CGDK::__stricmp(str_find, "false") == 0
				|| CGDK::__stricmp(str_find, "disable") == 0)
			{
				this->disable_ignore_exception_on_start();

				// trace)
				LOG_INFO << "  + parameter['ignore_exception_on_start']: 'disable'"sv;
			}
			// - default면 기본 값을 사용한다. (기본값은 false이다.)
			else if (CGDK::__stricmp(str_find, "default") == 0)
			{
				this->disable_ignore_exception_on_start();

				// trace)
				LOG_INFO << "  + parameter['ignore_exception_on_start']: 'default'[enable]"sv;
			}
			// - 그 외의 값이면 잘못된 값이다.
			else
			{
			}
		}

		// - ignore failure
		if (std::string str_find = pcontext_now->find("ignore_exception_in_child_on_start"sv, "ignore child failure on start"sv); str_find.empty() == false)
		{
			// - true면 재접속을 활성화한다.
			if (CGDK::__stricmp(str_find, "true") == 0
				|| CGDK::__stricmp(str_find, "enable") == 0)
			{
				// -  block_util_completed을 enable한다.
				this->enable_ignore_exception_in_child_on_start();

				// trace)
				LOG_INFO << "  + parameter['ignore_exception_in_child_on_start']: 'enable'"sv;
			}
			// - false면 재접속을 비활성화한다.
			else if (CGDK::__stricmp(str_find, "false") == 0
				|| CGDK::__stricmp(str_find, "disable") == 0)
			{
				this->disable_ignore_exception_in_child_on_start();

				// trace)
				LOG_INFO << "  + parameter['ignore_exception_in_child_on_start']: 'disable'"sv;
			}
			// - default면 기본 값을 사용한다. (기본값은 false이다.)
			else if (CGDK::__stricmp(str_find, "default") == 0)
			{
				this->disable_ignore_exception_in_child_on_start();

				// trace)
				LOG_INFO << "  + parameter['ignore_exception_in_child_on_start']: 'default'[enable]"sv;
			}
			// - 그 외의 값이면 잘못된 값이다.
			else
			{
			}
		}
	}

	// 6) call '_process_initialize_request'
	this->_process_start_request(*pcontext_now);

	// 7) 만약 이전 상태가 STOPPED상태면 START_PENDING상태로 변경한다.
	bool is_changed = this->set_object_state_if(eOBJECT_STATE::STOPPED, eOBJECT_STATE::START_PENDING);

	// check) 이전 상태가 STOPPED상태가 아니라면 false를 리턴한다.
	RETURN_IF(is_changed == false, eRESULT::INVALID_STATE);

	// declare) 
	result_code result;

	try
	{
		// 8) 자신을 먼저 start처리한다.
		result = this->process_start(*pcontext_now);
	}
	catch(Nthrowable& _exception)
	{
		// - stop상태로 되돌린다.
		this->set_object_state(eOBJECT_STATE::STOPPED);

		// - destroy한다.
		if (is_initialize == eRESULT::SUCCESS)
			object::destroy(this);

		// reraise)
		if (this->ignore_exception_on_start() == false)
			throw;

		// log) 
		LOG_WARNING << "(warn) exception IGNORED '_process_starting' ['"sv << pname << "']"sv << log_endl
					<< "       - descriotion: ["sv << to_string(_exception.reason<eRESULT>()) << "("sv << _exception.reason<uint64_t>() << ")] "sv << _exception.description();

		// - set result
		result = _exception.reason();
	}
	catch (std::exception& _exception)
	{
		// - stop상태로 되돌린다.
		this->set_object_state(eOBJECT_STATE::STOPPED);

		// - destroy한다.
		if (is_initialize == eRESULT::SUCCESS)
			object::destroy(this);

		// reraise)
		if (this->ignore_exception_on_start() == false)
			throw;

		// log) 
		LOG_WARNING << "(warn) exception IGNORED '_process_starting' ['"sv << pname << "']"sv << log_endl
					<< "       - descriotion: [std::excetion] "sv << std::string_view(_exception.what());

		// - set result
		result = eRESULT::EXCEPTION;
	}
	catch (...)
	{
		// - stop상태로 되돌린다.
		this->set_object_state(eOBJECT_STATE::STOPPED);

		// - destroy한다.
		if (is_initialize == eRESULT::SUCCESS)
			object::destroy(this);

		// reraise) 
		if (this->ignore_exception_on_start() == false)
			throw;

		// log) 
		LOG_WARNING << "(warn) exception IGNORED '_process_starting' ['"sv << pname << "']"sv;

		// - set result
		result = eRESULT::EXCEPTION;
	}

	// return) 
	return result;
}

result_code object::Nstartable::stop() noexcept
{
	// 1) call '_process_stop_request'
	this->_process_stop_request();

	// 2) eOBJECT_STATE::RUNNING 상태일 때만 eOBJECT_STATE::STOP_PENDING 상태로 변경한다.
	bool is_changed = this->set_object_state_if(eOBJECT_STATE::RUNNING, eOBJECT_STATE::STOP_PENDING);

	// check) 상태가 벼경되지 않았다면 그냥 끝낸다.
	RETURN_IF(is_changed == false, eRESULT::INVALID_STATE);

	// 3) process_stop함수를 호출한다.
	return this->process_stop();
}

result_code object::Nstartable::process_start(context& _context)
{
	// declare)
	result_code result;

	try
	{
		// 1) call _process_starting (내부에서 on_starting()함수가 호출된다.)
		result = this->_process_starting(_context);

		// check)
		RETURN_IF(result != eRESULT::SUCCESS, result);

		// 2) call '_process_initializing_self_complete'
		result = this->_process_starting_child(_context);
	}
	catch (Nthrowable& _exception)
	{
		// - call exception handler
		throwable::execute_exception_handler(this, _exception, __FUNCTION__);

		// - get name
		auto* pnameable = dynamic_cast<object::Inameable*>(this);
		auto pname = (pnameable != nullptr && pnameable->name().empty() == false) ? pnameable->name() : std::string();

		// log) 
		LOG_FATAL << "(excp) exception occurred on '_process_starting' ['"sv << pname << "']"sv << log_endl
				  << "       - descriotion: ["sv << to_string(_exception.reason<eRESULT>()) << "("sv << _exception.reason<uint64_t>() << ")] "sv << _exception.description();

		// reraise)
		throw;
	}
	catch (std::exception& _exception)
	{
		// - call exception handler
		throwable::execute_exception_handler(this, _exception, __FUNCTION__);

		// - get name
		auto* pnameable = dynamic_cast<object::Inameable*>(this);
		auto pname = (pnameable != nullptr && pnameable->name().empty() == false) ? pnameable->name() : std::string();

		// log) 
		LOG_FATAL << "(excp) exception occurred on '_process_starting' ['"sv << pname << "']"sv << log_endl
				  << "       - descriotion: [std::excetion] "sv << std::string_view(_exception.what());

		// reraise)
		throw;
	}
	catch (...)
	{
		// - get name
		auto* pnameable = dynamic_cast<object::Inameable*>(this);
		auto pname = (pnameable != nullptr && pnameable->name().empty() == false) ? pnameable->name() : std::string();

		// log) 
		LOG_FATAL << "(excp) exception occurred on '_process_starting' ['"sv << pname << "']"sv;

		// reraise)
		throw;
	}

	// return)
	return result;
}

result_code object::Nstartable::_process_starting_child(context& _context)
{
	// declare)
	bool pended = false;

	// declare) 
	std::vector<object_ptr<Istate>> vector_child;

	// 1) 자식 객체들을 stop한다.
	vector_child.reserve(this->m_container.size());

	// 2) copy pended info
	for (auto& iter : this->m_container)
		vector_child.push_back(iter);

	// 3) get iterator
	auto iter = vector_child.begin();
	auto iter_end = vector_child.end();

	try
	{
		// 4) 모든 자식 객체를 start한다.
		for (; iter != iter_end; ++iter)
		{
			// - push wait info
			this->m_queue_pended.emplace_back(object::Istate::WAIT_INFO{ *iter, eRESULT::PENDED });

			try
			{
				// - call child 
				result_code completed_nested = object::start(*iter, _context);

				// check)
				if (completed_nested != eRESULT::PENDED)
				{
					if (this->m_queue_pended.empty() == false && this->m_queue_pended.back().pobject == *iter)
						this->m_queue_pended.pop_back();
				}
				else
				{
					pended = true;
				}
			}
			catch (...)
			{
				// - remove wait info for the failed child
				if (this->m_queue_pended.empty() == false && this->m_queue_pended.back().pobject == *iter)
					this->m_queue_pended.pop_back();

				// - get name
				auto* pnameable = dynamic_cast<object::Inameable*>(this);
				auto pname = (pnameable != nullptr && pnameable->name().empty() == false) ? pnameable->name() : std::string();

				// log) 
				LOG_FATAL << "(excp) exception occurred on '_process_start' ['"sv << pname << "']"sv;

				// reraise)
				if (this->ignore_exception_in_child_on_start() == false)
					throw;
				
				// log) 
				LOG_WARNING << "(warn) exception in child IGNORED '_process_start' ['"sv << pname << "']"sv;
			}
		}
	}
	catch (...)
	{
		// - start했던 자식 객체를 Roll-back한다.
		for (; iter != vector_child.begin();)
			object::stop(*(--iter));

		// - clear pended queue
		this->m_queue_pended.clear();

		// - on_stop 함수를 호출한다.(on_starting()함수와 짝함수이므로 호출된다.)
		this->_process_stopping();

		// reraise) 
		if (this->ignore_exception_on_start() == false)
			throw;

		// - get name
		auto* pnameable = dynamic_cast<object::Inameable*>(this);
		auto pname = (pnameable != nullptr && pnameable->name().empty() == false) ? pnameable->name() : std::string();

		// log) 
		LOG_WARNING << "(warn) exception IGNORED '_process_starting' ['"sv << pname << "']"sv;
	}

	// check)
	RETURN_IF(pended, result_code(eRESULT::PENDED));

	// 2) complete child
	return this->_process_starting_child_complete(_context);
}

result_code object::Nstartable::_process_starting_child_complete(context& _context)
{
	// 1) _process_start 함수를 호출한다. (내부적으로 on_start()함수가 호출된다.)
	auto result = this->_process_start(_context);

	// check)
	RETURN_IF(result != eRESULT::SUCCESS, result);

	// 2) call
	this->_process_start_complete(_context);

	// 3) stet state to 'eOBJECT_STATE::RUNNING'
	//    - only from START_PENDING : on_start() may have set its own running state (e.g. doss repository : ESTABLISHED)
	this->set_object_state_if(eOBJECT_STATE::START_PENDING, eOBJECT_STATE::RUNNING);

	// return)
	return result;
}

void object::Nstartable::_process_start_complete(context& /*_context*/)
{
}

result_code object::Nstartable::process_stop() noexcept
{
	// 1) call '_process_stopping'
	auto result = this->_process_stopping();

	// check)
	RETURN_IF(result != eRESULT::SUCCESS, result);

	// 2) call '_process_destroying_self_complete'
	return this->_process_stopping_child();
}

result_code object::Nstartable::_process_stopping_child() noexcept
{
	// declare) 
	bool pended = false;

	// declare) 
	std::vector<object_ptr<Istate>> vector_pended;

	// 1) 자식 객체들을 stop한다.
	vector_pended.reserve(this->m_container.size());

	// 2) copy pended info
	for (auto riter = this->m_container.rbegin(); riter != this->m_container.rend(); ++riter)
		vector_pended.push_back(*riter);

	// 3) Childe를 stop한다.
	for (auto& pened: vector_pended)
	{
		// - push wait info
		this->m_queue_pended.emplace_back(object::Istate::WAIT_INFO{ pened, eRESULT::PENDED });

		// - call child 
		result_code completed_nested = object::stop(pened);

		// check)
		if (completed_nested != eRESULT::PENDED)
		{
			if (this->m_queue_pended.empty() == false && this->m_queue_pended.back().pobject == pened)
				this->m_queue_pended.pop_back();
		}
		else
		{
			pended = true;
		}
	}

	// check)
	RETURN_IF(pended, result_code(eRESULT::PENDED));

	// 2) 
	return this->_process_stopping_child_complete();
}

result_code object::Nstartable::_process_stopping_child_complete() noexcept
{
	// 1) call '_process_destroy()' (call 'on_destroy()')
	auto result = this->_process_stop();

	// check)
	RETURN_IF(result != eRESULT::SUCCESS, result)

	// 2) call '_process_destroy_complete()' (change state to 'eOBJECT_STATE::NONE')
	this->_process_stop_complete();

	// return)
	return result_code(eRESULT::SUCCESS);
}

void object::Nstartable::_process_stop_complete() noexcept
{
	// 1) 상태를 eOBJECT_STATE::STOPPED로 변경한다.
	this->set_object_state(eOBJECT_STATE::STOPPED);
}


}