#include "pch.h"

using namespace CGDK;


executor::io_context::io_context() noexcept
{
	// 1) set object type
	this->set_object_type(eOBJECT_TYPE::EXECUTOR_COMPLETION_PORT);

	// 2) register to factory manager
	this->process_register_executor();
}

executor::io_context::io_context(std::string_view _name) :
	Nexecutor(_name)
{
	// 1) set object type
	this->set_object_type(eOBJECT_TYPE::EXECUTOR_COMPLETION_PORT);

	// 2) register to factory manager
	this->process_register_executor();
}

executor::io_context::~io_context() noexcept
{
	// 1) stop executor
	executor::io_context::stop();

	// 2) unrregister from factory manager
	this->process_unregister_executor();

	// 3) set state to eOBJECT_STATE::NONE
	this->set_object_state(eOBJECT_STATE::NONE);
}

result_code executor::io_context::_process_initializing(context& _context)
{
	// 1) call 'on_initializing'
	this->on_initializing(_context);

	// return)
	return result_code(eRESULT::SUCCESS);
}

result_code executor::io_context::_process_initialize(context& _context)
{
	// 1) call 'on_initializing'
	this->on_initialize(_context);

	// return)
	return result_code(eRESULT::SUCCESS);
}

result_code executor::io_context::_process_destroying() noexcept
{
	try
	{
		// 1) call 'on_destroying'
		this->on_destroying();
	}
	catch (...)
	{
		CGASSERT_ERROR(false);
	}

	// return)
	return result_code(eRESULT::SUCCESS);
}

result_code executor::io_context::_process_destroy() noexcept
{
	try
	{
		// 1) call 'on_destroy'
		this->on_destroy();
	}
	catch (...)
	{
		CGASSERT_ERROR(false);
	}

	// return)
	return result_code(eRESULT::SUCCESS);
}

result_code executor::io_context::_process_starting(context& _context)
{
	// 1) call 'on_starting'
	this->on_starting(_context);

	// return)
	return result_code(eRESULT::SUCCESS);
}

result_code executor::io_context::_process_start(context& _context)
{
	// 1) queeue을 생성한다.
	auto result = this->m_io_context.create();

	// check) IOCP의 생성에 실패했으면 Exception을 던진다.
	THROW_IF(result == false, throwable::failure(eRESULT::FAIL_TO_CREATE));

	// 2) 
	this->on_start(_context);

	// return)
	return result_code(eRESULT::SUCCESS);
}

result_code executor::io_context::_process_stopping() noexcept
{
	try
	{
		// 1) call 'on_stopping'
		this->on_stopping();
	}
	catch (...)
	{
		CGASSERT_ERROR(false);
	}

	// return)
	return result_code(eRESULT::SUCCESS);
}

result_code executor::io_context::_process_stop() noexcept
{
	// 1) stop io_context
	this->m_io_context.dispose();

	// 2) queued된 것을 일단 모두 실행한다.
	this->execute_all();

	// 3) completionPort를 제거한다.
	this->m_io_context.destroy();

	try
	{
		// 3) call 'on_stop'
		this->on_stop();
	}
	catch (...)
	{
		CGASSERT_ERROR(false);
	}

	// return)
	return result_code(eRESULT::SUCCESS);
}

bool executor::io_context::process_post(object_ptr<Iexecutable>&& _pexecutable)
{
	// check) _pexecutable가 NULL이면 Post를 던지지 않는다.
	RETURN_IF(_pexecutable.empty(), false);

	// 1) Post한다.
	return this->m_io_context.post_executable(0, 0, std::move(_pexecutable)) != 0;
}

bool executor::io_context::process_execute(chrono::tick::duration _ticks_wait, [[maybe_unused]] intptr_t _option)
{
	//-----------------------------------------------------------------
	struct AUTO_RELEASE_EXECUTABLE
	{
	public:
		Iexecutable*		m_pexecutable;
		api::Nthread*		m_pthread_current;

	#if defined(_ENABLE_DEBUG_NEW)
		char				m_debugInfo_pexecutable[sizeof(Iexecutable)];
		Iexecutable*		m_debugInfo_pexecutable_Debug;
		const char*			m_debug_info_filename;
		int					m_debug_info_line;
	#endif

	public:
		explicit AUTO_RELEASE_EXECUTABLE(Iexecutable* _pexecutable) noexcept : 
			m_pexecutable(_pexecutable),
			m_pthread_current(api::get_current_thread())
		#if defined(_ENABLE_DEBUG_NEW)
			,m_debugInfo_pexecutable{}
			,m_debugInfo_pexecutable_Debug(reinterpret_cast<Iexecutable*>(m_debugInfo_pexecutable))
			,m_debug_info_filename(_pexecutable->m_debug_info_filename)
			,m_debug_info_line(_pexecutable->m_debug_info_line)
		#endif
		{
			// check) m_pthread_current가 null이면 안된다.
			CGASSERT_ERROR(this->m_pthread_current != nullptr);

		#if defined(_ENABLE_DEBUG_NEW)
			// check) ....
			CGASSERT_ERROR(sizeof(Iexecutable) > 32);

			// - 제일 앞  32Byte만 복사한다. virtual table 정도만 복사해 놓기 위해...
			memcpy(this->m_debugInfo_pexecutable, reinterpret_cast<void*>(_pexecutable), 32);
		#endif

			// statistics)
			this->m_pthread_current->statistics_executing_begin();
		}

		~AUTO_RELEASE_EXECUTABLE() noexcept
		{
			// check) m_pexecutable가 nullptr이면 그냥 리턴한다.
			CGASSERT_ERROR(this->m_pexecutable != nullptr);

			// statistics)
			this->m_pthread_current->statistics_executing_end();

			// statistics)
			this->m_pexecutable->statistics_on_extra();

			// 2) m_pexecutable을 release한다.
			this->m_pexecutable->release();

			// 3) m_pexecutable을 nullptr로 설정한다.
			this->m_pexecutable = nullptr;
		}
	};
	//-----------------------------------------------------------------
	// check)
	RETURN_IF(this->handle() == INVALID_CGHANDLE_VALUE, false);

	// declare) 
	api::io_context::RESULT result_event;

	// 1) get executable from io_context 
	this->m_io_context.pop_executable(result_event, _ticks_wait);

	// check) 
	if (result_event.is_valid() == false)
	{
		// - process idle
		this->process_idle();

		// return)
		return false;
	}

	// 2) 'process_execute'
	{
		// declare)
		AUTO_RELEASE_EXECUTABLE	pexecutable(result_event.executable());

		// - call 'process_execute'
		pexecutable.m_pexecutable->process_execute(result_event.result(), result_event.param());
	}

	// statistics)
	++this->m_statistics.now.count_execute;

	// return) 
	return true;
}

void executor::io_context::execute_all() noexcept
{
	//-----------------------------------------------------------------
	// 설명) 이중 for문인 이유!!!
	//   만약 execute(0)함수의 실행중 Exception이 발생했을 경우 
	//   Catch에서 잡은 후 다시 다음을 실행하기 위해 for문이 이중이다.
	//   만약에 더이상 실행할 executor가 없을 경우 false가 리턴되면
	//   바로 리턴 된다.
	//
	//   이때 for문을 이중으로해서 TRY문을 뺀 이유는 성능의 향상을 위해서다.
	//   매번 for문이 돌때마다 try와 catch가 반복될 경우 성능을 하락시킬수
	//   있기 때문이다.
	//-----------------------------------------------------------------
	for(;;)
	{
		try
		{
			// 1) execute를 수행한다.
			for (;;)
			{
				if (this->execute(chrono::tick::duration::zero()) == false)
				{
					return;
				}
			}
		}
		catch(...)
		{
		}
	}
}
