#include "pch.h"

using namespace CGDK;

struct executor::scheduler::EXECUTION_AT
{
public:
	chrono::tick::time_point	tick_at;
	object_ptr<Iexecutable>		pexecutable;

public:
	EXECUTION_AT() noexcept {}
	EXECUTION_AT(const EXECUTION_AT& _copy) noexcept : tick_at(_copy.tick_at), pexecutable(_copy.pexecutable) {}
	EXECUTION_AT(EXECUTION_AT&& _move) noexcept : tick_at(_move.tick_at), pexecutable(std::move(_move.pexecutable)) {}
	EXECUTION_AT(chrono::tick::time_point _tick_at, Iexecutable* _pexecutable) noexcept : tick_at(_tick_at), pexecutable(_pexecutable) {}

public:
	void			swap(EXECUTION_AT& _rhs) noexcept { SWAP(tick_at, _rhs.tick_at); pexecutable.swap(_rhs.pexecutable); }

public:
	EXECUTION_AT& operator=(const EXECUTION_AT& _copy) noexcept { tick_at = _copy.tick_at; pexecutable = _copy.pexecutable; return *this; }
	EXECUTION_AT& operator=(EXECUTION_AT&& _move) noexcept { tick_at = _move.tick_at; pexecutable = std::move(_move.pexecutable); return *this; }

};

executor::scheduler::scheduler() noexcept :
	m_flag_run(0)
{
	// 1) set object type
	this->set_object_type(eOBJECT_TYPE::EXECUTOR_SINGLE_EXECUTE);

	// 2) set executor type
	this->m_statistics.type = eEXECUTOR_TYPE::SYSTEM;

	// 3) reserve
	scoped_lock(this->m_lockable_priorityqueue)
	{
		this->m_priorityqueue_executable.reserve(16384);
	}
}

executor::scheduler::scheduler(std::string_view _name) :
	CGDK::Nexecutor(_name),
	m_pexecutor(),
	m_flag_run(0)
{
	// 1) set object type
	this->set_object_type(eOBJECT_TYPE::EXECUTOR_SINGLE_EXECUTE);

	// 2) set executor type
	this->m_statistics.type = eEXECUTOR_TYPE::SYSTEM;

	// 3) reserve
	scoped_lock(this->m_lockable_priorityqueue)
	{
		this->m_priorityqueue_executable.reserve(16384);
	}

	// 4) register to factory manager
	this->process_register_executor();
}

executor::scheduler::~scheduler() noexcept
{
	// 1) cancel all executable
	this->cancel();

	// 2) unregister from executor manager
	this->process_unregister_executor();
}

result_code executor::scheduler::start(context& /*_context*/)
{
	// 1) set m_flag_run 'true' and get old state
	const int state_old = this->m_flag_run.exchange(1);

	// check) return if aleady running
	RETURN_IF(state_old != 0, false);

	// 2) executor
	if (this->has_executor() == false)
	{
		this->executor(system_executor::executor());
	}

	try
	{
		// 3) begin thread
		this->begin_thread(fn_thread, this);
	}
	catch (...)
	{
		// - stop
		this->stop();

		// reraise)
		throw;
	}

	return result_code(eRESULT::SUCCESS);
}

result_code executor::scheduler::stop() noexcept
{
	// 1) set m_flag_run 'false' and get old state
	const int state_old = this->m_flag_run.exchange(0);

	// check) return if not running
	RETURN_IF(state_old == 0, false);

	// 2) calcel
	this->cancel();

	// 3) wait
	{
	#ifdef NDEBUG
		// declare) 최대 executor 완료 대기시간 (10초)
		const int MAX_LOOP = 1000;

		for (auto count_loop = MAX_LOOP; this->get_existing_thread_count(); --count_loop)
		{
			// check) 
			RETURN_IF(count_loop <= 0, false);

			// - execute
			this->cancel();

			//// - 1mss만큼 sleep한다.
			//api::sleep_for(1ms);
		}
	#else
		while (this->get_existing_thread_count())
		{
			// - execute
			this->cancel();

			//// - 1ms만큼 sleep한다.
			//api::sleep_for(0ms);
		}
	#endif

		// - terminate
		this->terminate_all();
	}

	// return) 
	return result_code(eRESULT::SUCCESS);
}

size_t executor::scheduler::pended_count() const noexcept
{
	scoped_lock(m_lockable_priorityqueue)
	{
		return this->m_priorityqueue_executable.size();
	}
}

bool executor::scheduler::process_post(object_ptr<Iexecutable>&& /*_pexecutable*/)
{
	return true;
}

bool executor::scheduler::process_post_at(object_ptr<Iexecutable>&& _pexecutable, chrono::tick::time_point _tick_execute)
{
	// check) 
	CGASSERT_ERROR(_pexecutable.exist());

	// 1) ...
	this->process_push_executable(EXECUTION_AT(_tick_execute, std::move(_pexecutable)));

	// return) 
	return true;
}

bool executor::scheduler::cancel(Iexecutable* _pexecutable) noexcept
{
	scoped_lock(this->m_lockable_priorityqueue)
	{
		// 1) get first executable object
		auto iter_end = this->m_priorityqueue_executable.end();
		auto iter = std::find_if(this->m_priorityqueue_executable.begin(), iter_end, [=](const EXECUTION_AT& iter) { return iter.pexecutable == _pexecutable;});

		// check) return if not exists
		RETURN_IF(iter == iter_end, false);

		// 2) get last item
		auto ptarget = std::move(this->m_priorityqueue_executable.back());
		auto tick_compare = ptarget.tick_at;

		// 3) get pos_now of item
		auto pos_now = static_cast<size_t>(iter - this->m_priorityqueue_executable.begin());

		// check) 
		if (iter == this->m_priorityqueue_executable.begin())
		{
			// - notify one
			this->m_cv_priorityqueue_in_time.notify_one();
		}

		// 4) remove tail of heap
		this->m_priorityqueue_executable.pop_back();

		// check) 
		RETURN_IF(pos_now == this->m_priorityqueue_executable.size(), true)

		// check) 
		CGASSERT_ERROR(pos_now < this->m_priorityqueue_executable.size());

		// case A) up heap
		if(pos_now != 0 && this->m_priorityqueue_executable.at((pos_now - 1) >> 1).tick_at > tick_compare)
		{
			// - ...
			auto iter_child = iter;

			do
			{
				// - minus 1 then divied 2
				pos_now = (pos_now - 1) >> 1;

				// - get left node
				auto iter_now = this->m_priorityqueue_executable.begin() + pos_now;

				// check) 
				BREAK_IF(iter_now->tick_at <= tick_compare);

				// - move to child node
				*iter_child = std::move(*iter_now);
				iter_child = iter_now;
			} while (pos_now != 0);

			// - 
			*iter_child = std::move(ptarget);
		}
		// case B) down heap
		else
		{
			pos_now = (pos_now << 1) + 1;

			// - size & parent
			auto pos_max = this->m_priorityqueue_executable.size() - 1;
			auto iter_parent = iter;

			while(pos_now <= pos_max)
			{
				// - get left node
				auto iter_now = this->m_priorityqueue_executable.begin() + pos_now;

				// - select right node if right node is larger then left node
				if(pos_now < pos_max && iter_now->tick_at > (iter_now + 1)->tick_at)
				{
					++iter_now;
				}

				// check) 
				BREAK_IF(tick_compare <= iter_now->tick_at);

				// - move to parent node
				*iter_parent = std::move(*iter_now);
				iter_parent = iter_now;

				// - set new pos_now ( pos_now x 2 + 1 )
				pos_now = (pos_now << 1) + 1;
			}

			*iter_parent = std::move(ptarget);
		}
	}

	// return) 
	return true;
}

void executor::scheduler::cancel() noexcept
{
	scoped_lock(m_lockable_priorityqueue)
	{
		if (this->m_priorityqueue_executable.empty())
		{
			this->m_cv_priorityqueue_exist.notify_one();
		}
		else
		{
			this->m_priorityqueue_executable.clear();
			this->m_cv_priorityqueue_in_time.notify_one();
		}
	}
}

void executor::scheduler::print_statistics() noexcept
{
	//try
	//{
	//	// 1) get instance
	//	auto instance = instance();

	//	// 2) alloc shared_buffer for log text
	//	auto buf_output = alloc_shared_buffer((static_cast<size_t>(instance->size()) + 4) * 256);

	//	buf_output.append_text("\x1b[37m" "---------------------------------------------------------------------------------------------------------------------------------\n" "\x1b[0m");
	//	buf_output.append_text("\x1b[37m" "{:30s} {:8s}{:>5s}{:>9s}{:>13s}{:>13s}\n" "\x1b[0m",
	//		"name",
	//		"type",
	//		"id",
	//		"thread",
	//		"executed",
	//		"pended"
	//	);
	//	buf_output.append_text("\x1b[90m" "---------------------------------------------------------------------------------------------------------------------------------\n" "\x1b[0m");

	//	scoped_shared_lock(instance->m_lockable_list_executor)
	//	{
	//		for (auto& iter : instance->m_list_executor)
	//		{
	//			// check)
	//			CGASSERT_ERROR(iter != nullptr);

	//			// check)
	//			CONTINUE_IF(iter == nullptr);

	//			// - get statistics
	//			auto& statistics = iter->statistics();

	//			std::string_view executor_name = (statistics.name.size() < 30) ? std::string_view(statistics.name) : std::string_view(statistics.name.c_str(), 30);

	//			auto alloc_execute = statistics.now.count_execute.load();

	//			if (statistics.type == eEXECUTOR_TYPE::USER)
	//			{
	//				// "1. name" "2. type"
	//				if (alloc_execute != 0)
	//				{
	//					buf_output.append_text("\x1b[92m" "{:30s} {:8s}" "\x1b[0m", executor_name, to_string<char>(statistics.type));
	//				}
	//				else
	//				{
	//					if (_using_only == true)
	//						continue;

	//					buf_output.append_text("\x1b[32m" "{:30s} {:8s}" "\x1b[0m", executor_name, to_string<char>(statistics.type));
	//				}

	//				// - "3. id"
	//				{
	//					auto count = statistics.id;

	//					if (count != 0)
	//					{
	//						buf_output.append_text(" {:>4}", count);
	//					}
	//					else
	//					{
	//						buf_output.append_text("\x1b[90m" "    0" "\x1b[0m");
	//					}
	//				}

	//				// - "4. thread_count"
	//				{
	//					if (statistics.now.thread_count != 0)
	//					{
	//						buf_output.append_text(" {:>8}", statistics.now.thread_count);
	//					}
	//					else
	//					{
	//						buf_output.append_text("\x1b[90m" "        0" "\x1b[0m");
	//					}
	//				}

	//				// - "5. executed"
	//				{
	//					auto count = statistics.now.count_execute.load();

	//					if (count != 0)
	//					{
	//						buf_output.append_text(" {:>9}", count);
	//					}
	//					else
	//					{
	//						buf_output.append_text("\x1b[90m" "         0" "\x1b[0m");
	//					}
	//				}

	//				// - "6. pended"
	//				{
	//					auto* pqueue = dynamic_cast<executor::Iqueue*>(iter);

	//					if (pqueue != nullptr)
	//					{
	//						auto count = pqueue->pended_count();

	//						if (count != 0)
	//						{
	//							buf_output.append_text(" {:>12}", count);
	//						}
	//						else
	//						{
	//							buf_output.append_text("\x1b[90m" "            0" "\x1b[0m");
	//						}
	//					}
	//				}

	//				// - new line~
	//				buf_output.append_text("\n");
	//			}
	//			else
	//			{
	//				// "1. name" "2. type"
	//				if (alloc_execute != 0)
	//				{
	//					buf_output.append_text("\x1b[37m" "{:30s} {:8s}" "\x1b[0m", executor_name, to_string<char>(statistics.type));
	//				}
	//				else
	//				{
	//					if (_using_only == true)
	//						continue;

	//					buf_output.append_text("\x1b[90m" "{:30s} {:8s}" "\x1b[0m", executor_name, to_string<char>(statistics.type));
	//				}

	//				// - "3. id"
	//				{
	//					auto count = statistics.id;

	//					if (count != 0)
	//					{
	//						buf_output.append_text(" {:>4}", count);
	//					}
	//					else
	//					{
	//						buf_output.append_text("\x1b[90m" "    0" "\x1b[0m");
	//					}
	//				}

	//				// - "4. thread_count"
	//				{
	//					if (statistics.now.thread_count != 0)
	//					{
	//						buf_output.append_text(" {:>8}", statistics.now.thread_count);
	//					}
	//					else
	//					{
	//						buf_output.append_text("\x1b[90m" "        0" "\x1b[0m");
	//					}
	//				}

	//				// - "5. executed"
	//				{
	//					auto count = statistics.now.count_execute.load();

	//					if (count != 0)
	//					{
	//						buf_output.append_text(" {:>12}", count);
	//					}
	//					else
	//					{
	//						buf_output.append_text("\x1b[90m" "            0" "\x1b[0m");
	//					}
	//				}

	//				// - "6. pended"
	//				{
	//					auto* pqueue = dynamic_cast<executor::Iqueue*>(iter);

	//					if (pqueue != nullptr)
	//					{
	//						auto count = pqueue->pended_count();

	//						if (count != 0)
	//						{
	//							buf_output.append_text(" {:>12}", count);
	//						}
	//						else
	//						{
	//							buf_output.append_text("\x1b[90m" "            0" "\x1b[0m");
	//						}
	//					}
	//				}

	//				// - new line~
	//				buf_output.append_text("\n");
	//			}
	//		}
	//	}
	//	buf_output.append_text("\x1b[37m" "---------------------------------------------------------------------------------------------------------------------------------\n" "\x1b[0m");

	//	// 3) 최종 출력
	//#if defined(_WIN32)
	//	DWORD number_written = 0;
	//	WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), buf_output.data(), buf_output.size<DWORD>(), &number_written, nullptr);
	//#else
	//	fwrite(buf_output.data(), 1, buf_output.size<int>(), stdout);
	//#endif
	//}
	//catch (...)
	//{
	//}
}


bool executor::scheduler::process_execute(chrono::tick::duration /*_tick_diff*/, intptr_t /*_option*/)
{
	// 1) get current thread
	auto pthread = api::get_current_thread();

	while (this->m_flag_run)
	{
		try
		{
			for(;;)
			{
				// check) 
				BREAK_IF(this->m_pexecutor.empty());

				conditional_lock(this->m_lockable_priorityqueue, lock_wait)
				{
					// 1) wait until 
					if (this->m_priorityqueue_executable.empty())
					{
						this->m_cv_priorityqueue_exist.wait(lock_wait);
					}

					// 2) get now tick
					auto tick_now = chrono::tick::now();

					// statistics) 
					pthread->statistics_executing_begin(tick_now);

					// 3) execute 
					while (this->m_priorityqueue_executable.empty() == false)
					{
						// - get first eecutable object
						EXECUTION_AT& pexecutable = this->m_priorityqueue_executable.at(0);

						// check) wait if not yet 
						if(pexecutable.tick_at > tick_now)
						{
							// statistics) 
							pthread->statistics_executing_end();

							// - wait time
							auto time_wait = pexecutable.tick_at - tick_now;

							// - sleep
							this->m_cv_priorityqueue_in_time.wait_for(lock_wait, time_wait);

							// - get now tick
							tick_now = chrono::tick::now();

							// statistics) 
							pthread->statistics_executing_begin(tick_now);

							// continue)
							continue;
						}

						// - execute 'executable'
						this->m_pexecutor->post(object_ptr<Iexecutable>(pexecutable.pexecutable));

						// statistics)
						++this->m_statistics.now.count_execute;

						// - remove 'executable'
						this->process_pop_executable();
					}

					// statistics) 
					pthread->statistics_executing_end();

					// check)
					BREAK_IF(this->m_flag_run == 0)
				}

				// - process idle
				this->process_idle();
			}
		}
		// On Exception) 
		catch (std::exception& _exception)
		{
			// - Exception Handler를 실행한다.
			throwable::execute_exception_handler(this, _exception, __FUNCTION__);
		}
	}

	// return)
	return true;
}

void executor::scheduler::process_push_executable(EXECUTION_AT&& _executable)
{
	scoped_lock(this->m_lockable_priorityqueue)
	{
		// check) 
		RETURN_IF(this->m_flag_run == 0, );

		// 1) store pos_now
		auto pos_now = this->m_priorityqueue_executable.size();

		// 2) push back item
		this->m_priorityqueue_executable.emplace_back(EXECUTION_AT());

		// 3) heap up
		auto iter_child = --this->m_priorityqueue_executable.end();

		while(pos_now != 0)
		{
			// - 빼기 1 후 나누기 2한다.
			pos_now = ((pos_now - 1) >> 1);

			// - ...
			auto iter_now = this->m_priorityqueue_executable.begin() + pos_now;

			// check) 
			BREAK_IF(iter_now->tick_at <= _executable.tick_at);

			// - move to down
			*iter_child = std::move(*iter_now);
			iter_child = iter_now;
		}

		// 4) ...
		*iter_child = std::move(_executable);

		// check)
		CGASSERT_ERROR(this->m_priorityqueue_executable.empty() == false);

		// 5) notify_one
		if (this->m_priorityqueue_executable.size() == 1)
		{
			this->m_cv_priorityqueue_exist.notify_one();
		}
		else if (iter_child->pexecutable == this->m_priorityqueue_executable[0].pexecutable)
		{
			this->m_cv_priorityqueue_in_time.notify_one();
		}
	}
}

void executor::scheduler::process_pop_executable() noexcept
{
	// 1) 
	auto ptarget = std::move(this->m_priorityqueue_executable.back());
	auto tick_compare = ptarget.tick_at;

	// 2) target객체를 제거하고 제일 마지막 객체를 그 위치로 가져 온다.
	this->m_priorityqueue_executable.pop_back();

	// check) 
	RETURN_IF(this->m_priorityqueue_executable.empty())

	// 3) Size & Parent
	auto pos_max = this->m_priorityqueue_executable.size() - 1;
	auto pos_now = size_t(1);
	auto iter_parent = this->m_priorityqueue_executable.begin();

	// 4) down heap
	while (pos_now <= pos_max)
	{
		// - get now node
		auto iter_now = this->m_priorityqueue_executable.begin() + pos_now;

		// - Left가 더 크면 Right 선택한다.
		if (pos_now < pos_max && iter_now->tick_at > (iter_now + 1)->tick_at)
		{
			++iter_now;
			++pos_now;
		}

		// check) Child가 더 크면 끝냄.
		BREAK_IF(tick_compare <= iter_now->tick_at);

		// - move to up
		*iter_parent = std::move(*iter_now);
		iter_parent = iter_now;

		// - set child node as new pos_now (pos_now = pos_now x 2 + 1)
		pos_now = (pos_now << 1) + 1;
	}

	*iter_parent = std::move(ptarget);
}

uint32_t executor::scheduler::fn_thread(executor::scheduler* _this)
{
	// statistics)
	++_this->m_statistics.now.thread_count;

	// 1) call 'execute'
	auto result = _this->execute(chrono::tick::duration(), 0);

	// statistics)
	--_this->m_statistics.now.thread_count;

	// return) 
	return result;
}
