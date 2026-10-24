#include "pch.h"
#include <charconv>
#include "platform.h"

namespace CGDK
{
	extern bool g_is_initialize;
}

using namespace CGDK;


system_executor::system_executor() noexcept
{
	this->name("system executor"sv);
}

system_executor::~system_executor() noexcept
{
	this->system_executor::destroy();
}

void system_executor::initialize_instance(int _count_thread)
{
	// lock)
	std::lock_guard<std::mutex> lock(m_s_lockable_create);

	// 1) set thread_count
	m_s_instance.m_init_param = _count_thread;

	// 2) initialize
	m_s_instance.initialize();
}

//! @brief '기본 실행자'의 싱글턴 객체를 초기화한다. @param _count_thread 생성할 스레드 개수 CGEXECUTOR_AUTO(-1)이면 코어 개수에 따라 자동 계산되 생성된다.
void system_executor::initialize_instance(context& _context)
{
	// lock)
	std::lock_guard<std::mutex> lock(m_s_lockable_create);

	// 1) initialize
	m_s_instance.initialize(_context);
}

void system_executor::destroy_instance() noexcept
{
	// lock)
	std::lock_guard<std::mutex> lock(m_s_lockable_create);

	m_s_instance.destroy();
}

void system_executor::detach_instance() noexcept
{
	m_s_instance.process_detach_instance();
}

size_t system_executor::thread_count() noexcept
{
	// 1) executor를 구한다.
	auto psystem_executor = system_executor::executor();

	// 2) ThreadFactory을 얻는다.
	auto pthread_factory = dynamic_pointer_cast<api::threads>(psystem_executor);

	// 3) Thread의 개수를 얻는다.
	auto ithread_count = (pthread_factory.exist()) ? pthread_factory->get_existing_thread_count() : 0;

	// return) 
	return ithread_count;
}

void f_handler_terminate()
{
}

result_code system_executor::process_initialize(context& _context)
{
	// definitions)
	using system_executor_t = executor::io_context_threads;

	// 1) pre initialize
	int count_thread = this->m_init_param;
	bool enable_thread_observer = true;

	const auto& context_system_executor = _context.find("system_executor"sv);
	if (context_system_executor.is_valid())
	{
		// 2) "thread_count"
		if (const auto& context_init = context_system_executor.find("thread_count"sv, "thread count"sv, "threadcount"sv); context_init.is_valid())
		{
			// - get string value
			std::string value_init = context_init;

			// - "default"면...
			if (CGDK::__stricmp(value_init, "default") == 0)
			{
				count_thread = static_cast<int>(CGEXECUTOR_AUTO);

				// trace)
				LOG_INFO << "  + parameter['thread count']: 'DEFAULT'"sv;
			}
			// - "auto"면... 
			else if (CGDK::__stricmp(value_init, "auto") == 0)
			{
				count_thread = static_cast<int>(CGEXECUTOR_AUTO);

				// trace)
				LOG_INFO << "  + parameter['thread count']: 'AUTO'"sv;
			}
			// - 
			else if (CGDK::__stricmp(value_init, "no thread") == 0)
			{
				count_thread = 0;

				// trace)
				LOG_INFO << "  + parameter['thread count']: 'NO THREAD'"sv;
			}
			else
			{
				// - 문자열을 숫자로 변환한다.
				count_thread = 0;
				std::from_chars(value_init.data(), value_init.data() + value_init.size(), count_thread);

				// trace) 
				LOG_INFO << "  + parameter['thread count']: {"sv << count_thread << "}"sv;
			}
		}

		// 2) "enable_thread_observer"
		if (const auto& context_find = context_system_executor.find("enable_thread_observer"sv, "enable thread observer"sv); context_find.is_valid())
		{
			// - get string value
			std::string str_find = context_find;

			// - true면 재접속을 활성화한다.
			if (CGDK::__stricmp(str_find, "true") == 0
				|| CGDK::__stricmp(str_find, "enable") == 0)
			{
				// -  enable_thread_observer을 enable한다.
				enable_thread_observer = true;

				// trace)
				LOG_INFO << "  + parameter['enable_thread_observer']: 'enable'"sv;
			}
			// - false면 재접속을 비활성화한다.
			else if (CGDK::__stricmp(str_find, "false") == 0
				|| CGDK::__stricmp(str_find, "disable") == 0)
			{
				// -  enable_thread_observer을 enable한다.
				enable_thread_observer = false;

				// trace)
				LOG_INFO << "  + parameter['enable_thread_observer']: 'disable'"sv;
			}
			// - default면 기본 값을 사용한다. (기본값은 false이다.)
			else if (CGDK::__stricmp(str_find, "default") == 0)
			{
				// -  enable_thread_observer을 default로.
				enable_thread_observer = true;

				// trace)
				LOG_INFO << "  + parameter['enable_thread_observer']: 'default'[enable]"sv;
			}
		}
	}

	// 2) initialize
	SCOPED_UNIQUE_LOCK(this->m_lockable_lock)
	{
		// check)
		if(this->m_psystem_executor.exist())
			return true;

		if (g_is_initialize == true)
		{
			// declare)
			own_ptr<system_executor_t> psystem_executor;
			own_ptr<schedulable_manager_t> pschedulable_manager;
			own_ptr<executable_scheduler_t> pexecutable_scheduler;

			try
			{
				// 1) create 'system_executor'
				psystem_executor = _make_own_raw<system_executor_t>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL);

				// 2) set name
				psystem_executor->name("system executor"sv);

				// 3) set executor type
				psystem_executor->m_statistics.type = eEXECUTOR_TYPE::SYSTEM;

				// 4) start 'system_executor'
				psystem_executor->start({ count_thread });

				// trace) 
				LOG_INFO << "  - [system executor] start"sv;

				// 5) create 'scheduler executor'
				pschedulable_manager = _make_own_raw<schedulable_manager_t>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL);

				// 6) set name
				pschedulable_manager->name("system scheduler"sv);

				// 7) start 'scheduler executor'
				pschedulable_manager->start();

				// trace) 
				LOG_INFO << "  - [system scheduler] start"sv;

				// 8) create 'single execution executor'
				pexecutable_scheduler = _make_own_raw<executable_scheduler_t>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL);

				// 9) set name
				pexecutable_scheduler->name("timed single execution"sv);

				// 10) set executor
				pexecutable_scheduler->executor(psystem_executor.get());

				// 11) start 'single execution executor'
				pexecutable_scheduler->start();

				// 11-1) schedulable manager posts registered schedulables to this scheduler
				pschedulable_manager->scheduler(object_ptr<executable_scheduler_t>(pexecutable_scheduler.get()));

				// trace) 
				LOG_INFO << "  - [timed single execution] start"sv;

				// 12) set terminate function
			#if !defined(_DEBUG) || defined(NDEBUG)
				std::set_terminate(f_handler_terminate);
			#endif

			#if defined(__linux__)
				dump_manager::set_signal_handler();
			#endif

				this->m_psystem_executor.exchange_release(std::move(psystem_executor));
				this->m_pschedulable_manager.exchange_release(std::move(pschedulable_manager));
				this->m_pexecutable_scheduler.exchange_release(std::move(pexecutable_scheduler));

				// 13) set state to 'eOBJECT_STATE::STOPPED'
				this->set_object_state(eOBJECT_STATE::STOPPED);
			}
			catch (...)
			{
				// trace) 
				LOG_FATAL << log_level_t::important() << "(excp) fail to create default executor. ("sv << __FUNCTION__ << ")"sv;

				// reraise) 
				throw;
			}

			try
			{
				// 14) start 'factory sustainer'
				this->start_factory_garbage_collection();

				// 15) start 'exception observer'
				if (enable_thread_observer)
				{
					this->start_thread_observer();
				}
			}
			catch (...)
			{
				// - stop
				this->stop_thread_observer();
				this->stop_factory_garbage_collection();

				// call 'process_edestroy'
				this->process_destroy();

				// reraise)
				throw;
			}

			// 16) post initialize
			if (context_system_executor.is_valid())
			{
				// - dump manager
				if (auto context_dump = context_system_executor.find("dump"sv, "dump info"sv, "dump_info"sv, "dumpinfo"sv); context_dump.is_valid())
				{
					dump_manager::initialize_instance(context_dump);
				}
			}

			// trace) 
			LOG_INFO << "  - [system executor] instance initialized"sv;
		}
		else
		{
			// 1) set state immediately
			this->set_object_state(eOBJECT_STATE::STOPPED);

			// trace) 
			LOG_INFO << "  - [system executor] dll initialized"sv;
		}
	}


	// return)
	return result_code(eRESULT::SUCCESS);
}

result_code system_executor::process_destroy() noexcept
{
	// 0) call the destroy handlers (normal termination only : ex. the emergency acceptor and its executors are stopped before the system executor)
	this->process_call_destroy_handlers();

	// declare) 
	own_ptr<executable_scheduler_t> pexecutable_scheduler;
	own_ptr<schedulable_manager_t> pschedulable_manager;
	own_ptr<executor::Iio_context> psystem_executor;

	// 1) Factory Sustain를 닫는다.
	SCOPED_UNIQUE_LOCK(this->m_lockable_lock)
	{
		// check) 
		if(this->m_psystem_executor.empty() && g_is_initialize == true)
			return true;

		// - stop factory sustain
		this->stop_factory_garbage_collection();

		// - stop observing
		this->stop_thread_observer();

		// - moving to 
		//   (lock 없이 get_acquire()로 읽는 쪽이 있으므로 atomic하게 바꾼다)
		pexecutable_scheduler = this->m_pexecutable_scheduler.exchange_release(own_ptr<executable_scheduler_t>());
		pschedulable_manager = this->m_pschedulable_manager.exchange_release(own_ptr<schedulable_manager_t>());
		psystem_executor = this->m_psystem_executor.exchange_release(own_ptr<executor::Iio_context>());
	}

	// check) scheduler에 걸려있는 executor이 하나도 없어야 한다.
	EXECUTE_IF(pschedulable_manager.exist() && pschedulable_manager->exist(), LOG(log_type_t::info) << "(warn) all scheduleres are not unregistered ("sv << __FUNCTION__ << ")"sv);

	// 2) destroy schedulable manager (cancels the schedulables pending in the scheduler)
	pschedulable_manager.destroy();

	// 3) destroy singly schedulable executor
	pexecutable_scheduler.destroy();

	// 4) destroy executor
	psystem_executor.destroy();

	// 5) 객체는 해제하지 않고 보관한다. (executor_ptr()로 얻은 raw pointer로 post 중인 thread가 있을 수 있다. system_executor가 소멸될 때 해제된다)
	if (psystem_executor.exist())
	{
		std::lock_guard lock(this->m_lockable_lock);
		this->m_vector_executor_retired.push_back(std::move(psystem_executor));
	}

	if (g_is_initialize == true)
	{
		// trace) 
		LOG_INFO << "  - [system executor] instance destroyed"sv;
	}
	else
	{
		// trace) 
		LOG_INFO << "  - [system executor] dll destroyed"sv;
	}

	// 6) set state to 'eOBJECT_STATE::NONE'
	this->set_object_state(eOBJECT_STATE::NONE);

	// return)
	return result_code(eRESULT::SUCCESS);
}

bool system_executor::register_destroy_handler(std::string_view _name, destroy_handler_t&& _handler)
{
	// check)
	RETURN_IF(!_handler, false);

	// 1) the system_executor must be initialized (the handler is called on its destroy)
	system_executor* psystem_executor = instance();

	// lock)
	std::lock_guard<std::recursive_mutex> lock(psystem_executor->m_lockable_lock);

	// 2) replace the handler of the same name
	for (auto& iter : psystem_executor->m_vector_destroy_handler)
	{
		if (iter.name == _name)
		{
			iter.handler = std::move(_handler);
			return true;
		}
	}

	// 3) add
	psystem_executor->m_vector_destroy_handler.push_back(DESTROY_HANDLER{ std::string(_name), std::move(_handler) });

	// return)
	return true;
}

bool system_executor::unregister_destroy_handler(std::string_view _name) noexcept
{
	// lock)
	std::lock_guard<std::recursive_mutex> lock(m_s_instance.m_lockable_lock);

	// 1) find & erase
	auto& vector_handler = m_s_instance.m_vector_destroy_handler;

	for (auto iter = vector_handler.begin(); iter != vector_handler.end(); ++iter)
	{
		if (iter->name == _name)
		{
			vector_handler.erase(iter);
			return true;
		}
	}

	// return) not found
	return false;
}

void system_executor::process_call_destroy_handlers() noexcept
{
	// 1) take the handlers out (one-shot : an unregister inside a handler is a no-op)
	std::vector<DESTROY_HANDLER> vector_handler;
	{
		std::lock_guard<std::recursive_mutex> lock(this->m_lockable_lock);

		vector_handler = std::move(this->m_vector_destroy_handler);
		this->m_vector_destroy_handler.clear();
	}

	// check)
	RETURN_IF(vector_handler.empty());

	// 2) call (in the reverse order of the registration)
	for (auto iter = vector_handler.rbegin(); iter != vector_handler.rend(); ++iter)
	{
		try
		{
			// trace)
			LOG_INFO << "  - [system executor] call destroy handler ['"sv << iter->name << "']"sv;

			// - call
			iter->handler();
		}
		catch (...)
		{
			// log)
			LOG_ERROR << "(excp) exception on destroy handler ['"sv << iter->name << "'] ("sv << __FUNCTION__ << ")"sv;
		}
	}
}

void system_executor::process_detach_instance() noexcept
{
	this->m_pexecutable_scheduler.detach();
	this->m_pschedulable_manager.detach();
	this->m_psystem_executor.detach();

	this->set_object_state(eOBJECT_STATE::NONE);
}


bool system_executor::start_factory_garbage_collection()
{
	// 1) Factory Sustainer를 얻는다.
	system_executor* psystem_executor = instance();

	SCOPED_UNIQUE_LOCK(psystem_executor->m_lockable_lock)
	{
		// check) 이미 Factory이 존재하면 끝낸다.
		if(psystem_executor->m_pschedulable_factory_garbage_collection.exist())
			return false;

		// 2) Factory Sustainer를 생성한다.
		auto pschedulable_factory_garbage_collection = make_object<factory_sustain_t>();

		// 3) scheduler를 얻는다.
		auto pschedulable_manager = psystem_executor->schedulable_manager();

		// check)
		if(pschedulable_manager.empty())
			return false;

		// 4) Factory Sustainer를 시작한다.
		pschedulable_manager->register_schedulable(pschedulable_factory_garbage_collection);

		// 5) 생성한 객체를 저장한다.
		psystem_executor->m_pschedulable_factory_garbage_collection = std::move(pschedulable_factory_garbage_collection);
	}

	// trace) 
	LOG_INFO << "  - start [factory garbage collector]"sv;

	// return) 
	return true;
}

bool system_executor::stop_factory_garbage_collection() noexcept
{
	// 1) 
	system_executor* psystem_executor = peek_instance();

	// check)
	if(psystem_executor == nullptr)
		return false;

	SCOPED_UNIQUE_LOCK(psystem_executor->m_lockable_lock)
	{
		// check) 
		if(psystem_executor->m_pschedulable_factory_garbage_collection.empty())
			return false;

		// 2) scheduler를 얻는다.
		auto pschedulable_manager = psystem_executor->schedulable_manager();

		// check)
		if(pschedulable_manager.empty())
			return false;

		// 3) Factory Sustainer를 시작한다.
		pschedulable_manager->unregister_schedulable(psystem_executor->m_pschedulable_factory_garbage_collection);

		// 4) reset한다.
		psystem_executor->m_pschedulable_factory_garbage_collection.reset();
	}

	// trace) 
	LOG_INFO << "  - stop [factory garbage collector]"sv;

	// return) 
	return true;
}

bool system_executor::start_thread_observer()
{
	// 1) Factory Sustainer를 얻는다.
	system_executor* psystem_executor = instance();

	SCOPED_UNIQUE_LOCK(psystem_executor->m_lockable_lock)
	{
		// check) 이미 Factory이 존재하면 끝낸다.
		if(psystem_executor->m_pschedulable_thread_observer.exist())
			return false;

		// 1) create thread observer
		auto pthread_observer = make_object<thread_observer_t>();

		// 2) get scheduled executable manager
		auto pschedulable_manager = psystem_executor->schedulable_manager();

		// check)
		if(pschedulable_manager.empty())
			return false;

		// 3) start exception observer
		pschedulable_manager->register_schedulable(pthread_observer);
				
		// 4) store object
		psystem_executor->m_pschedulable_thread_observer = std::move(pthread_observer);
	}

	// trace) 
	LOG_INFO << "  - start [thread observer]"sv;

	// return) 
	return true;
}

bool system_executor::stop_thread_observer() noexcept
{
	// 1) 
	system_executor* psystem_executor = peek_instance();

	// check)
	if(psystem_executor == nullptr)
		return false;

	SCOPED_UNIQUE_LOCK(psystem_executor->m_lockable_lock)
	{
		// check) 
		if(psystem_executor->m_pschedulable_thread_observer.empty())
			return false;

		// 1) scheduler를 얻는다.
		auto pschedulable_manager = psystem_executor->schedulable_manager();

		// check)
		if(pschedulable_manager.empty())
			return false;

		// 2) Factory Sustainer를 시작한다.
		pschedulable_manager->unregister_schedulable(psystem_executor->m_pschedulable_thread_observer);

		// 3) reset한다.
		psystem_executor->m_pschedulable_thread_observer.reset();
	}

	// trace) 
	LOG_INFO << "  - stop [thread  observer]"sv;

	// return) 
	return true;
}

void system_executor::run_executor()
{
	// 1) Instance가 empty면 initialize한다.
	if (m_s_instance.m_psystem_executor.empty())
	{
		initialize_instance();
	}

	// 2) executor Default를 얻는다.
	object_ptr<executor::Iio_context> psystem_executor = m_s_instance.m_psystem_executor;
	
	// 3) executor를 돌려 IOCP의 I/O를 처리한다.
	psystem_executor->execute_all();
}

bool system_executor::register_schedulable(object_ptr<Ischedulable>&& _pschedulable)
{
	// check) executor가 nullptr인지 확인한다.
	if(_pschedulable == nullptr)
		return false;

	// 1) scheduler를 얻는다.
	auto pschedulable_manager = system_executor::schedulable_manager();

	// check) scheduler가 nullptr이면 안 된다.
	CGASSERT_ERROR(pschedulable_manager.exist());

	// check) scheduler가 없으면 Exception을 던진다.
	THROW_IF(pschedulable_manager.empty(), throwable::failure(eRESULT::NO_INITIALIZED));

	// return) scheduler에 붙인다.
	return pschedulable_manager->register_schedulable(std::move(_pschedulable));
}

bool system_executor::unregister_schedulable(Ischedulable* _pschedulable) noexcept
{
	// check) executor가 nullptr인지 확인한다.
	if(_pschedulable == nullptr)
		return false;

	// 1) Schedular를 얻는다.
	auto pschedulable_manager = _pschedulable->schedulable_manager();
		
	// check) scheduler가 없으면 여기서 끝낸다.
	if(pschedulable_manager.empty())
		return false;

	// 2) Schedulable Manager에서 Schedulable을 제거한다.
	return pschedulable_manager->unregister_schedulable(_pschedulable);
}
