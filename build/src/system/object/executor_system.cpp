#include "pch.h"

using namespace CGDK;

namespace CGDK
{
	extern bool g_is_initialize;
}

system_executor::system_executor() noexcept :
	object::Nnameable("system executor")
{
}

system_executor::~system_executor() noexcept
{
	this->system_executor::destroy();
}

void system_executor::initialize_instance(size_t _count_thread)
{
	scoped_lock(m_s_lockable_create)
	{
		// 1) set thread_count
		m_s_instance.m_init_param = _count_thread;

		// 2) initialize
		m_s_instance.initialize();
	}
}

//! @brief '기본 실행자'의 싱글턴 객체를 초기화한다. @param _count_thread 생성할 쓰레드 갯수 CGEXECUTOR_AUTO(-1)이면 코어 갯수에 따라 자동 계산되 생성된다.
void system_executor::initialize_instance(context& _context)
{
	scoped_lock(m_s_lockable_create)
	{
		// 1) initialize
		m_s_instance.initialize(_context);
	}
}

void system_executor::destroy_instance() noexcept
{
	scoped_lock(m_s_lockable_create)
	{
		m_s_instance.destroy();
	}
}

void system_executor::detach_instance() noexcept
{
	m_s_instance.process_detach();
}

size_t system_executor::thread_count() noexcept
{
	// 1) executor를 구한다.
	auto psystem_executor = system_executor::executor();

	// 2) ThreadFactory을 얻는다.
	auto pthread_factory = dynamic_pointer_cast<api::threads>(psystem_executor);

	// 3) Thread의 갯수를 얻는다.
	auto ithread_count = (pthread_factory.exist()) ? pthread_factory->get_existing_thread_count() : 0;

	// return) 
	return ithread_count;
}

void f_terminate_handler()
{
}

result_code system_executor::process_initialize(context& _context)
{
	// definitions)
	using system_executor_t = executor::io_context_threads;

	// 1) pre initialize
	size_t count_thread = this->m_init_param;
	bool enable_thread_observer = true;

	auto context_system_executor = _context.find("system_executor"sv);
	if (context_system_executor.is_valid())
	{
		// 2) "thread_count"
		if (std::string pvalue_init = context_system_executor.find("thread_count"sv, "thread count"sv, "threadcount"sv); pvalue_init.empty() == false)
		{
			// - "default"면...
			if (__stricmp(pvalue_init, "default"sv) == 0)
			{
				count_thread = CGEXECUTOR_AUTO;

				// trace)
				LOG_INFO << "  + parameter['thread count']: 'DEFAULT'"sv;
			}
			// - "auto"면... 
			else if (__stricmp(pvalue_init, "auto"sv) == 0)
			{
				count_thread = CGEXECUTOR_AUTO;

				// trace)
				LOG_INFO << "  + parameter['thread count']: 'AUTO'"sv;
			}
			// - 
			else if (__stricmp(pvalue_init, "no thread"sv) == 0)
			{
				count_thread = __from_string<decltype(count_thread)>(pvalue_init);

				// trace)
				LOG_INFO << "  + parameter['thread count']: 'NO THREAD'"sv;
			}
			else
			{
				// - 문자열을 Address로 변경한다.
				count_thread = __from_string<decltype(count_thread)>(pvalue_init);

				// trace) 
				LOG_INFO << "  + parameter['thread count']: {"sv << count_thread << "}"sv;
			}
		}

		// 2) "enable_thread_observer"
		if (std::string str_find = context_system_executor.find("enable_thread_observer"sv, "enable thread observer"sv); str_find.empty() == false)
		{
			// - true면 재접속을 활성화한다.
			if (__stricmp(str_find, "true"sv) == 0
				|| __stricmp(str_find, "enable"sv) == 0)
			{
				// -  enable_thread_observer을 enable한다.
				enable_thread_observer = true;

				// trace)
				LOG_INFO << "  + parameter['enable_thread_observer']: 'enable'"sv;
			}
			// - false면 재접속을 비활성화한다.
			else if (__stricmp(str_find, "false"sv) == 0
				|| __stricmp(str_find, "disable"sv) == 0)
			{
				// -  enable_thread_observer을 enable한다.
				enable_thread_observer = false;

				// trace)
				LOG_INFO << "  + parameter['enable_thread_observer']: 'disable'"sv;
			}
			// - default면 기본 값을 사용한다. (기본값은 false이다.)
			else if (__stricmp(str_find, "default"sv) == 0)
			{
				// -  enable_thread_observer을 default로.
				enable_thread_observer = true;

				// trace)
				LOG_INFO << "  + parameter['enable_thread_observer']: 'default'[enable]"sv;
			}
		}
	}

	// 2) initialize
	scoped_unique_lock(this->m_lockable_lock)
	{
		// check)
		RETURN_IF(this->m_psystem_executor.exist(), true);

		if (g_is_initialize == true)
		{
			// declare)
			own_ptr<system_executor_t> psystem_executor;
			own_ptr<schedulable_manager_t> pschedulable_manager;
			own_ptr<executable_scheduler_t> pexecutable_scheduler;

			try
			{
				// 1) create 'system_executor'
				psystem_executor = _make_own_raw<system_executor_t>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA "system executor"sv);

				// 2) set executor type
				psystem_executor->m_statistics.type = eEXECUTOR_TYPE::SYSTEM;

				// 3) start 'system_executor'
				psystem_executor->start({ count_thread });

				// trace) 
				LOG_INFO << "  - [system executor] start"sv;

				// 4) create 'scheduler executor'
				pschedulable_manager = _make_own_raw<schedulable_manager_t>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA "system scheduler"sv);

				// 5) start 'scheduler executor'
				pschedulable_manager->start();

				// trace) 
				LOG_INFO << "  - [system scheduler] start"sv;

				// 6) create 'single execution executor'
				pexecutable_scheduler = _make_own_raw<executable_scheduler_t>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA "timed single execution"sv);

				// 7) set executor
				pexecutable_scheduler->executor(psystem_executor.get());

				// 8) start 'single execution executor'
				pexecutable_scheduler->start();

				// trace) 
				LOG_INFO << "  - [timed single execution] start (interval: 1ms)"sv;

				// 9) set terminate function
			#if !defined(_DEBUG) || defined(NDEBUG)
				std::set_terminate(f_terminate_handler);
			#endif

				this->m_psystem_executor = std::move(psystem_executor);
				this->m_pschedulable_manager = std::move(pschedulable_manager);
				this->m_pexecutable_scheduler = std::move(pexecutable_scheduler);

				// 10) set state to 'eOBJECT_STATE::STOPPED'
				this->set_object_state(eOBJECT_STATE::STOPPED);
			}
			catch (...)
			{
				// trace) 
				LOG_FATAL << log_level_t::important() << "(excp) fail to create defualt exexutor. ("sv << __FUNCTION__ << ")"sv;

				// reraise) 
				throw;
			}

			try
			{
				// 11) start 'factory sustainer'
				this->start_factory_garbage_collection();

				// 12) start 'exception observer'
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

			// 3) post initialize
			if (context_system_executor.is_valid())
			{
				// - dump manager
				if (auto context_dump = context_system_executor.find("dump"sv, "dump info"sv, "dump_info"sv, "dumpinfo"sv); context_dump.is_valid())
				{
					// - get dump manager
					auto pdump_manager = dump_manager::instance();

					// trace)
					LOG_INFO << "  @ dump manager"sv;

					// - enable/disable
					{
						auto context_find = context_dump.find("enable"sv);

						if (context_find.is_valid())
						{
							auto str_dumptype = CGDK::__to_lower<char>(static_cast<std::string>(context_find));

							if (str_dumptype == "false"sv
								&& str_dumptype == "0"sv)
							{
								// trace)
								LOG_INFO << "  + parameter['enable']: false"sv;
							}
							else if (str_dumptype == "true"sv
								&& str_dumptype == "1"sv)
							{
								// trace)
								LOG_INFO << "  + parameter['enable']: true"sv;
							}
						}
					}

					// - path
					{
						auto context_find = context_dump.find("path"sv);

						if (context_find.is_valid())
						{
							// get path
							auto str_path = static_cast<std::string>(context_find);

							//- set
							pdump_manager->set_dump_path(str_path);

							// trace)
							LOG_INFO << "  + parameter['path']: "sv << str_path;
						}
					}

					// - filename
					{
						auto context_find = context_dump.find("filename"sv);

						if (context_find.is_valid())
						{
							auto str_filename = static_cast<std::string>(context_find);

							// trace)
							LOG_INFO << "  + parameter['filename']: "sv << str_filename;
						}
					}

					// - count limit
					{
						auto context_find = context_dump.find("count_limit"sv, "count limit"sv, "countlimit"sv);

						if (context_find.is_valid())
						{
							// - get count
							int dump_count_limit = context_find;

							// - set
							pdump_manager->set_dump_count_limit(dump_count_limit);

							// trace)
							LOG_INFO << "  + parameter['count_limit']: "sv << dump_count_limit;
						}
					}

					// - dump type
					{
						auto context_find = context_dump.find("dump_type"sv, "dump type"sv, "dumptype"sv);

						if (context_find.is_valid())
						{
							auto str_dumptype = CGDK::__to_lower<char>(static_cast<std::string>(context_find));

							if (str_dumptype == "mini"sv)
							{
								// - set
								pdump_manager->set_dump_type(eDUMP_CONTEXT::DETAIL);

								// trace)
								LOG_INFO << "  + parameter['dump_type']: 'mini'"sv;
							}
							else if (str_dumptype == "defail"sv)
							{
								// - set
								pdump_manager->set_dump_type(eDUMP_CONTEXT::DETAIL);

								// trace)
								LOG_INFO << "  + parameter['dump_type']: 'defail'"sv;
							}
							else if (str_dumptype == "full"sv)
							{
								// - set
								pdump_manager->set_dump_type(eDUMP_CONTEXT::FULL);

								// trace)
								LOG_INFO << "  + parameter['dump_type']: 'full'"sv;
							}
							else if (str_dumptype == "default"sv)
							{
								// - set
								pdump_manager->set_dump_type(eDUMP_CONTEXT::DEFAULT);

								// trace)
								LOG_INFO << "  + parameter['dump_type']: 'default'"sv;
							}
						}
					}
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
	// declare) 
	own_ptr<executable_scheduler_t> pexecutable_scheduler;
	own_ptr<schedulable_manager_t> pschedulable_manager;
	own_ptr<executor::Iio_context> psystem_executor;

	// 1) Factory Sustaine를 닫는다.
	scoped_unique_lock(this->m_lockable_lock)
	{
		// check) 
		RETURN_IF(this->m_psystem_executor.empty() && g_is_initialize == true, true);

		// - stop factory sustaine
		this->stop_factory_garbage_collection();

		// - stop observing
		this->stop_thread_observer();

		// - moving to 
		pexecutable_scheduler = std::move(this->m_pexecutable_scheduler);
		pschedulable_manager = std::move(this->m_pschedulable_manager);
		psystem_executor = std::move(this->m_psystem_executor);
	}

	// check) scheduler에 걸려있는 executor이 하나도 없어야 한다.
	EXECUTE_IF(pschedulable_manager.exist() && pschedulable_manager->exist(), LOG(log_type_t::info) << "(warn) all scheduleres are not unregistered ("sv << __FUNCTION__ << ")"sv);

	// 2) singly schedulable
	pexecutable_scheduler.destroy();

	// 3) destroy singly schedulable executor
	pschedulable_manager.destroy();

	// 4) destroy executor
	psystem_executor.destroy();

	if (g_is_initialize == true)
	{
		// trace) 
		LOG_INFO << "  - [system executor] instance destoryed"sv;
	}
	else
	{
		// trace) 
		LOG_INFO << "  - [system executor] dll destoryed"sv;
	}

	// 5) set state to 'eOBJECT_STATE::NONE'
	this->set_object_state(eOBJECT_STATE::NONE);

	// return)
	return result_code(eRESULT::SUCCESS);
}

void system_executor::process_detach() noexcept
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

	scoped_unique_lock(psystem_executor->m_lockable_lock)
	{
		// check) 이미 Factory이 존재하면 끝낸다.
		RETURN_IF(psystem_executor->m_pschedulable_factory_garbage_collection.exist(), false);

		// 2) Factory Sustainer를 생성한다.
		auto pschedulable_factory_garbage_collection = make_object<factory_sustain_t>();

		// 3) scheduler를 얻는다.
		auto pschedulable_manager = psystem_executor->schedulable_manager();

		// check)
		RETURN_IF(pschedulable_manager.empty(), false);

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
	RETURN_IF(psystem_executor == nullptr, false);

	scoped_unique_lock(psystem_executor->m_lockable_lock)
	{
		// check) 
		RETURN_IF(psystem_executor->m_pschedulable_factory_garbage_collection.empty(), false);

		// 2) scheduler를 얻는다.
		auto pschedulable_manager = psystem_executor->schedulable_manager();

		// check)
		RETURN_IF(pschedulable_manager.empty(), false);

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

	scoped_unique_lock(psystem_executor->m_lockable_lock)
	{
		// check) 이미 Factory이 존재하면 끝낸다.
		RETURN_IF(psystem_executor->m_pschedulable_thread_observer.exist(), false);

		// 1) create thread observer
		auto pthread_observer = make_object<thread_observer_t>();

		// 2) get scheduled excutable manager
		auto pschedulable_manager = psystem_executor->schedulable_manager();

		// check)
		RETURN_IF(pschedulable_manager.empty(), false);

		// 3) start exception oberver
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
	RETURN_IF(psystem_executor == nullptr, false);

	scoped_unique_lock(psystem_executor->m_lockable_lock)
	{
		// check) 
		RETURN_IF(psystem_executor->m_pschedulable_thread_observer.empty(), false);

		// 1) scheduler를 얻는다.
		auto pschedulable_manager = psystem_executor->schedulable_manager();

		// check)
		RETURN_IF(pschedulable_manager.empty(), false);

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
	// 1) Instace가 empty면 initialize한다.
	if (m_s_instance.m_psystem_executor.empty())
	{
		initialize_instance();
	}

	// 2) executaor Default를 얻는다.
	object_ptr<executor::Iio_context> psystem_executor = m_s_instance.m_psystem_executor;
	
	// 3) executor를 돌려 IOCP의 I/O를 처리한다.
	psystem_executor->execute_all();
}

bool system_executor::register_schedulable(object_ptr<Ischedulable>&& _pschedulable)
{
	// check) executor가 nullptr인지 확인한다.
	RETURN_IF(_pschedulable == nullptr, false);

	// 1) scheduler를 얻는다.
	auto pschedulable_manager = system_executor::schedulable_manager();

	// check) scheduler가 nullptr이면 안됀다.
	CGASSERT_ERROR(pschedulable_manager.exist());

	// check) scheduler가 없으면 Exception을 던진다.
	THROW_IF(pschedulable_manager.empty(), throwable::failure(eRESULT::NO_INITIALIZED));

	// return) scheduler에 붙인다.
	return pschedulable_manager->register_schedulable(std::move(_pschedulable));
}

bool system_executor::unregister_schedulable(Ischedulable* _pschedulable) noexcept
{
	// check) executor가 nullptr인지 확인한다.
	RETURN_IF(_pschedulable == nullptr, false);

	// 1) Schedular를 얻는다.
	auto pschedulable_manager = _pschedulable->schedulable_manager();
		
	// check) scheduler가 없으면 여기서 끝낸다.
	RETURN_IF(pschedulable_manager.empty(), false);

	// 2) Schedulable Manager에서 Schedulable을 제거한다.
	return pschedulable_manager->unregister_schedulable(_pschedulable);
}
