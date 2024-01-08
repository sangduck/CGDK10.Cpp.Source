#include "pch.h"

namespace CGDK
{

result_code object::Ninitializable::initialize(context& _context)
{
	// 1) Nameable 객체 포인터를 얻고 현재의 Context를 저장해 놓는다.
	auto* pnameable = dynamic_cast<object::Inameable*>(this);
	auto pname = (pnameable != nullptr && pnameable->name().empty() == false) ? pnameable->name() : ""sv;

	// 2) get now context
	auto* pcontext_now = &_context;

	// 3) context를 변경한다.
	if (pname.empty() == false)
	{
		auto& context_find = _context[pname];

		if (context_find.is_valid())
		{
			pcontext_now = &context_find;
		}
	}

	// 4) reset applied initializion string
	_process_reset_applied_initialization_param();

	// 5) initialize parameters
	{
		// - ignore failure
		if (std::string str_find = pcontext_now->find("ignore_exception_on_initialize"sv, "ignore failure on initialize"sv); str_find.empty() == false)
		{
			// - true면 재접속을 활성화한다.
			if (__stricmp(str_find, "true"sv) == 0
				|| __stricmp(str_find, "enable"sv) == 0)
			{
				// -  block_util_completed을 enable한다.
				this->enable_ignore_exception_on_initialize();

				// trace)
				LOG_INFO << "  + parameter['ignore_exception_on_initialize']: 'enable'"sv;
			}
			// - false면 재접속을 비활성화한다.
			else if (__stricmp(str_find, "false"sv) == 0
				|| __stricmp(str_find, "disable"sv) == 0)
			{
				this->disable_ignore_exception_on_initialize();

				// trace)
				LOG_INFO << "  + parameter['ignore_exception_on_initialize']: 'disable'"sv;
			}
			// - default면 기본 값을 사용한다. (기본값은 false이다.)
			else if (__stricmp(str_find, "default"sv) == 0)
			{
				this->disable_ignore_exception_on_initialize();

				// trace)
				LOG_INFO << "  + parameter['ignore_exception_on_initialize']: 'default'[enable]"sv;
			}
			// - 그 외의 값이면 잘못된 값이다.
			else
			{
			}
		}

		// - ignore failure
		if (std::string str_find = pcontext_now->find("ignore_exception_in_child_on_initialize", "ignore child failure on initialize"); str_find.empty() == false)
		{
			// - true면 재접속을 활성화한다.
			if (__stricmp(str_find, "true"sv) == 0
				|| __stricmp(str_find, "enable"sv) == 0)
			{
				// -  block_util_completed을 enable한다.
				this->enable_ignore_exception_in_child_on_initialize();

				// trace)
				LOG_INFO << "  + parameter['ignore_exception_in_child_on_initialize']: 'enable'"sv;
			}
			// - false면 재접속을 비활성화한다.
			else if (__stricmp(str_find, "false"sv) == 0
				|| __stricmp(str_find, "disable"sv) == 0)
			{
				this->disable_ignore_exception_in_child_on_initialize();

				// trace)
				LOG_INFO << "  + parameter['ignore_exception_in_child_on_initialize']: 'disable'"sv;
			}
			// - default면 기본 값을 사용한다. (기본값은 false이다.)
			else if (__stricmp(str_find, "default"sv) == 0)
			{
				this->disable_ignore_exception_in_child_on_initialize();

				// trace)
				LOG_INFO << "  + parameter['ignore_exception_in_child_on_initialize']: 'default'[enable]"sv;
			}
			// - 그 외의 값이면 잘못된 값이다.
			else
			{
			}
		}
	}

	// 5) call _process_initialize_request
	this->_process_initialize_request(*pcontext_now);

	// 6) eOBJECT_STATE::NONE 상태일 때만 eOBJECT_STATE::INITIALIZING_PENDING 상태로 바꾼다.
	bool is_changed = this->set_object_state_if(eOBJECT_STATE::NONE, eOBJECT_STATE::INITIALIZING_PENDING);

	// check) 상태가 바뀌지 않았으면 false를 리턴하며 끝낸다.
	RETURN_IF(is_changed == false, result_code(eRESULT::INVALID_STATE));

	// declare) 
	result_code result;

	try
	{
		// 7) process_initialize()함수를 호출한다.
		result = this->process_initialize(*pcontext_now);
	}
	catch (Nthrowable& _exception)
	{
		// - 상태를 다시 eOBJECT_STATE::NONE으로 되돌린다.
		this->set_object_state(eOBJECT_STATE::NONE);

		// reraise)
		if (this->ignore_exception_on_initialize() == false)
			throw;

		// log) 
		LOG_WARNING << "(warn) exception IGNORED '_process_initializing' ['"sv << pname << "']"sv << log_endl
					<< "       - descriotion: ["sv << to_string<char>(_exception.reason<eRESULT>()) << "("sv << _exception.reason<uint64_t>() << ")] "sv << _exception.description();

		// - set result
		result = _exception.reason();
	}
	catch (std::exception& _exception)
	{
		// - 상태를 다시 eOBJECT_STATE::NONE으로 되돌린다.
		this->set_object_state(eOBJECT_STATE::NONE);

		// reraise)
		if (this->ignore_exception_on_initialize() == false)
			throw;

		// log) 
		LOG_WARNING << "(warn) exception IGNORED '_process_initializing' ['"sv << pname << "']"sv << log_endl
					<< "       - descriotion: [std::excetion] "sv << std::string_view(_exception.what());

		// - set result
		result = eRESULT::EXCEPTION;
	}
	catch(...)
	{
		// - 상태를 다시 eOBJECT_STATE::NONE으로 되돌린다.
		this->set_object_state(eOBJECT_STATE::NONE);

		// reraise)
		if (this->ignore_exception_on_initialize() == false)
			throw;

		// log) 
		LOG_WARNING << "(warn) exception IGNORED '_process_initializing' ['"sv << pname << "']"sv;

		// - set result
		result = eRESULT::EXCEPTION;
	}

	// return) 
	return result;
}

result_code object::Ninitializable::destroy() noexcept
{
	// 1) 일단 stop 먼저 한다.
	object::stop(this);

	// 2) call _process_initialize_request
	this->_process_destroy_request();

	// 3) eOBJECT_STATE::STOPPED 상태일 때만 eOBJECT_STATE::DESTROYING_PENDING로 상태를 바꾼다.
	bool is_changed = this->set_object_state_if(eOBJECT_STATE::STOPPED, eOBJECT_STATE::DESTROYING_PENDING);

	// check) 상태가 바뀌지 않았을 경우에는 false를 리턴하며 끝낸다.
	RETURN_IF(is_changed == false, eRESULT::INVALID_STATE);

	// 4) process_destroy 함수를 호출한다.
	return this->process_destroy();
}

result_code object::Ninitializable::process_initialize(context& _context)
{
	// declare)
	result_code result;

	try
	{
		// 1) call '_process_initializing' (내부에서 on_initializing()함수가 호출된다.)
		result = this->_process_initializing(_context);

		// check)
		RETURN_IF(result != eRESULT::SUCCESS, result);

		// 2)  call '_process_initializing_self_complete'
		result = this->_process_initializing_child(_context);
	}
	catch (Nthrowable& _exception)
	{
		// - call exception handler
		throwable::execute_exception_handler(this, _exception, __FUNCTION__);

		// - get name
		auto* pnameable = dynamic_cast<object::Inameable*>(this);
		auto pname = (pnameable != nullptr && pnameable->name().empty() == false) ? pnameable->name() : std::string_view();

		// log) 
		LOG_FATAL << "(excp) exception occure on '_process_initializing' ['"sv << pname << "']"sv << log_endl
		  		  << "       - descriotion: ["sv << to_string<char>(_exception.reason<eRESULT>()) << "("sv << _exception.reason<uint64_t>() << ")] "sv << _exception.description();

		// reraise)
		throw;
	}
	catch (std::exception& _exception)
	{
		// - call exception handler
		throwable::execute_exception_handler(this, _exception, __FUNCTION__);

		// - get name
		auto* pnameable = dynamic_cast<object::Inameable*>(this);
		auto pname = (pnameable != nullptr && pnameable->name().empty() == false) ? pnameable->name() : std::string_view();

		// log) 
		LOG_FATAL << "(excp) exception occure on '_process_initializing' ['"sv << pname << "']"sv << log_endl
				  << "       - descriotion: [std::excetion] "sv << std::string_view(_exception.what());

		// reraise)
		throw;
	}
	catch (...)
	{
		// - get name
		auto* pnameable = dynamic_cast<object::Inameable*>(this);
		auto pname = (pnameable != nullptr && pnameable->name().empty() == false) ? pnameable->name() : std::string_view();

		// log) 
		LOG_FATAL << "(excp) exception occure on '_process_initializing' ['"sv << pname << "']"sv;

		// reraise)
		throw;
	}

	// return)
	return result;
}

result_code object::Ninitializable::_process_initializing_child(context& _context)
{
	// declare)
	bool pended = false;

	// declare) 
	auto iter = this->m_container.begin();
	auto iter_end = this->m_container.end();

	try
	{
		// 4) 모든 자식 객체를 initialize한다.
		for (; iter != iter_end; ++iter)
		{
			// - push wait info
			this->m_queue_pended.emplace_back(object::Istate::WAIT_INFO{ *iter, eRESULT::PENDED });

			try
			{
				// - call child 
				result_code completed_nested = object::initialize(*iter, _context);

				// check)
				if (completed_nested == eRESULT::PENDED)
				{
					if (this->m_queue_pended.empty() == false && this->m_queue_pended.back().pobject == *iter)
					{
						this->m_queue_pended.pop_back();
					}

					pended = true;
				}
			}
			catch (...)
			{
				// - get name
				auto* pnameable = dynamic_cast<object::Inameable*>(this);
				auto pname = (pnameable != nullptr && pnameable->name().empty() == false) ? pnameable->name() : std::string_view();

				// log) 
				LOG_FATAL << "(excp) exception occure on '_process_initialize' ['"sv << pname << "']"sv;

				// reraise)
				if (this->ignore_exception_in_child_on_initialize() == false)
					throw;
				
				// log) 
				LOG_WARNING << "(warn) exception in child IGNORED '_process_initialize' ['"sv << pname << "']"sv;
			}
		}
	}
	catch (...)
	{
		// - initialize했던 자식 객체를 Roll-back한다.
		for (auto iter_rollback = this->m_container.begin(); iter_rollback != iter; ++iter_rollback)
		{
			object::destroy(*iter_rollback);
		}

		// - on_destroy 함수를 호출한다.(on_initialize()함수와 짝함수이므로 호출된다.)
		this->_process_destroying();

		// reraise) 
		if (this->ignore_exception_on_initialize() == false)
			throw;

		// - get name
		auto* pnameable = dynamic_cast<object::Inameable*>(this);
		auto pname = (pnameable != nullptr && pnameable->name().empty() == false) ? pnameable->name() : std::string_view();

		// log) 
		LOG_WARNING << "(warn) exception IGNORED '_process_initialize' ['"sv << pname << "']"sv;
	}

	// check)
	RETURN_IF(pended, result_code(eRESULT::PENDED));

	// 2) complete child
	return this->_process_initializing_child_complete(_context);
}

result_code object::Ninitializable::_process_initializing_child_complete(context& _context)
{
	// 1) eOBJECT_STATE::STOPPED 상태로 변경한다.
	this->set_object_state(eOBJECT_STATE::STOPPED);

	// 2) _process_initialize 함수를 호출한다. (내부적으로 on_initialize()함수가 호출된다.)
	auto result = this->_process_initialize(_context);

	// check)
	RETURN_IF(result != eRESULT::SUCCESS, result);

	// 3) call 
	this->_process_initialize_complete(_context);

	// return)
	return result;
}

void object::Ninitializable::_process_initialize_complete(context& /*_context*/)
{
	// 1) notify to parent

}

result_code object::Ninitializable::process_destroy() noexcept
{
	// 1) call '_process_destroying'
	result_code result = this->_process_destroying();

	// check)
	RETURN_IF(result != eRESULT::SUCCESS, result);

	// 2) call '_process_destroying_self_complete'
	return this->_process_destroying_child();
}

result_code object::Ninitializable::_process_destroying_child() noexcept
{
	// declare) 
	bool pended = false;

	// 1) destroy child objects
	for (auto riter = this->m_container.rbegin(); riter != this->m_container.rend(); ++riter)
	{
		// - push wait info
		this->m_queue_pended.emplace_back(object::Istate::WAIT_INFO{ *riter, eRESULT::PENDED });

		// - call child 
		result_code completed_nested = object::destroy(*riter);

		// check)
		if (completed_nested == eRESULT::PENDED)
		{
			if (this->m_queue_pended.empty() == false && this->m_queue_pended.back().pobject == *riter)
			{
				this->m_queue_pended.pop_back();
			}

			pended = true;
		}
	}

	// check)
	RETURN_IF(pended , result_code(eRESULT::PENDED));

	// 2) 
	return this->_process_destroying_child_complete();
}

result_code object::Ninitializable::_process_destroying_child_complete() noexcept
{
	// 1) call '_process_destroy()' (call 'on_destroy()')
	result_code result = this->_process_destroy();

	// check)
	RETURN_IF(result != result_code(eRESULT::SUCCESS), result)

	// 2) call '_process_destroy_complete()' (change state to 'eOBJECT_STATE::NONE')
	this->_process_destroy_complete();

	// return)
	return result_code(eRESULT::SUCCESS);
}

void object::Ninitializable::_process_destroy_complete() noexcept
{
	// 1) 상태를 eOBJECT_STATE::NONE로 변경한다.
	this->set_object_state(eOBJECT_STATE::NONE);
}

void object::Ninitializable::_process_add_applied_initialization_param(const std::string& _key, const std::string_view _value)
{
	scoped_lock(m_cs_map_applied_initialization_param)
	{
		auto iter_find = m_map_applied_initialization_param.find(_key);

		if (iter_find != m_map_applied_initialization_param.end())
		{
			iter_find->second = _value;
		}
		else
		{
			m_map_applied_initialization_param.emplace(_key, std::string(_value));
		}
	}
}

void object::Ninitializable::_process_reset_applied_initialization_param() noexcept
{
	scoped_lock(m_cs_map_applied_initialization_param)
	{
		m_map_applied_initialization_param.clear();
	}
}



}