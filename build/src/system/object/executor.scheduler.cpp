#include "pch.h"
#if defined(__GNUC__)
	#include <cxxabi.h>
#endif

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
	{
		// lock)
		std::lock_guard<std::mutex> lock(this->m_lockable_priorityqueue);

		this->m_priorityqueue_executable.reserve(16384);
	}

#if defined(_WIN32)
	// 4) 고해상도 타이머와 깨우기 이벤트를 생성한다.
	//    - Windows 10 1803 이상에서 지원된다. 생성하지 못하면 기존의 condition_variable로 대기한다.
	//    - condition_variable의 wait_for는 시스템 타이머 해상도(기본 15.6ms) 단위로 깨어나므로 정확도가 떨어진다.
	this->m_handle_timer = ::CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);

	if (this->m_handle_timer != nullptr)
	{
		this->m_handle_wake = ::CreateEventW(nullptr, FALSE, FALSE, nullptr);

		if (this->m_handle_wake == nullptr)
		{
			::CloseHandle(this->m_handle_timer);
			this->m_handle_timer = nullptr;
		}
	}
#endif

	// 5) register to factory manager
	this->process_register_executor();
}

executor::scheduler::~scheduler() noexcept
{
	// 1) cancel all executable
	this->cancel();

	// 2) unregister from executor manager
	this->process_unregister_executor();

#if defined(_WIN32)
	// 3) 스레드를 종료한 후 타이머와 이벤트를 닫는다.
	this->executor::scheduler::stop();

	if (this->m_handle_timer != nullptr)
	{
		::CloseHandle(this->m_handle_timer);
		this->m_handle_timer = nullptr;
	}

	if (this->m_handle_wake != nullptr)
	{
		::CloseHandle(this->m_handle_wake);
		this->m_handle_wake = nullptr;
	}
#endif
}

result_code executor::scheduler::start(context& /*_context*/)
{
	// 1) set m_flag_run 'true' and get old state
	const int state_old = this->m_flag_run.exchange(1);

	// check) return if already running
	if(state_old != 0)
		return result_code(eRESULT::FAIL);

	// 2) executor
	if (this->has_executor() == false)
		this->executor(system_executor::executor());

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
	if(state_old == 0)
		return result_code(eRESULT::DONE);

	// 2) cancel
	this->cancel();

	// 3) wait
	{
	#ifdef NDEBUG
		// declare) 최대 executor 완료 대기시간 (10초)
		const int MAX_LOOP = 1000;

		for (auto count_loop = MAX_LOOP; this->get_existing_thread_count(); --count_loop)
		{
			// check) 
			if(count_loop <= 0)
				return result_code(eRESULT::FAIL);

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
		this->api::threads::stop();
	}

	// return) 
	return result_code(eRESULT::SUCCESS);
}

size_t executor::scheduler::pended_count() const noexcept
{
	// lock)
	std::lock_guard<std::mutex> lock(this->m_lockable_priorityqueue);

	return this->m_priorityqueue_executable.size();
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
	// lock)
	std::lock_guard<std::mutex> lock(this->m_lockable_priorityqueue);

	// 1) get first executable object
	auto iter_end = this->m_priorityqueue_executable.end();
	auto iter = std::find_if(this->m_priorityqueue_executable.begin(), iter_end, [=](const EXECUTION_AT& iter) { return iter.pexecutable == _pexecutable;});

	// check) return if not exists
	if(iter == iter_end)
		return false;

	// 2) get last item
	auto ptarget = std::move(this->m_priorityqueue_executable.back());
	auto tick_compare = ptarget.tick_at;

	// 3) get pos_now of item
	auto pos_now = static_cast<size_t>(iter - this->m_priorityqueue_executable.begin());

	// check) 
	if (iter == this->m_priorityqueue_executable.begin())
		this->_notify_in_time(); // notify one 

	// 4) remove tail of heap
	this->m_priorityqueue_executable.pop_back();

	// check) 
	if(pos_now == this->m_priorityqueue_executable.size())
		return true;

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
			if(iter_now->tick_at <= tick_compare)
				break;

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
				++pos_now;
			}

			// check) 
			if(tick_compare <= iter_now->tick_at)
				break;

			// - move to parent node
			*iter_parent = std::move(*iter_now);
			iter_parent = iter_now;

			// - set new pos_now ( pos_now x 2 + 1 )
			pos_now = (pos_now << 1) + 1;
		}

		*iter_parent = std::move(ptarget);
	}

	// return) 
	return true;
}

void executor::scheduler::cancel() noexcept
{
	// lock)
	std::lock_guard<std::mutex> lock(this->m_lockable_priorityqueue);

	if (this->m_priorityqueue_executable.empty())
	{
		this->m_cv_priorityqueue_exist.notify_one();
	}
	else
	{
		this->m_priorityqueue_executable.clear();
		this->_notify_in_time();
	}
}

// type name of executable (demangled on gcc/clang)
static std::string _get_type_name(const std::type_info& _type_info)
{
#if defined(__GNUC__)
	int status = 0;
	char* name_demangled = abi::__cxa_demangle(_type_info.name(), nullptr, nullptr, &status);
	std::string result = (status == 0 && name_demangled != nullptr) ? std::string(name_demangled) : std::string(_type_info.name());
	free(name_demangled);
	return result;
#else
	return std::string(_type_info.name());
#endif
}

void executor::scheduler::print_statistics() noexcept
{
	try
	{
		// definitions)
		constexpr size_t MAX_PRINT_PENDED = 20;

		// declare)
		shared_buffer buf_output;
		auto tick_now = chrono::tick::now();

		// 1) executor to which the executable is posted
		auto pexecutor = this->executor();
		std::string str_executor = pexecutor.exist() ? std::string(pexecutor->name()) : std::string("-");

		// 2) pended executables (sorted by the execution time)
		std::vector<std::pair<chrono::tick::time_point, std::string>> vector_pended;
		size_t count_pended = 0;
		{
			// lock)
			std::unique_lock<std::mutex> lock(this->m_lockable_priorityqueue);

			count_pended = this->m_priorityqueue_executable.size();
			vector_pended.reserve(count_pended);

			for (auto& iter : this->m_priorityqueue_executable)
				vector_pended.emplace_back(iter.tick_at, iter.pexecutable.exist() ? _get_type_name(typeid(*iter.pexecutable)) : std::string("-"));
		}
		std::sort(vector_pended.begin(), vector_pended.end(), [](const auto& _lhs, const auto& _rhs) { return _lhs.first < _rhs.first; });

		// 3) alloc shared_buffer for log text
		buf_output = alloc_shared_buffer((std::min(count_pended, MAX_PRINT_PENDED) + 10) * 256);

		// 4) summary
		const auto& statistics = this->statistics();

		buf_output.append_text("\x1b[37m" "---------------------------------------------------------------------------------------------------------------------------------\n" "\x1b[0m"sv);
		buf_output.append_text(std::format("\x1b[37m" "{:30s} {:30s}{:>9s}{:>13s}{:>13s}{:>11s}\n" "\x1b[0m",
			"name",
			"executor",
			"thread",
			"executed",
			"pended",
			"next(ms)")
		);
		buf_output.append_text("\x1b[90m" "---------------------------------------------------------------------------------------------------------------------------------\n" "\x1b[0m"sv);
		buf_output.append_text(std::format("\x1b[92m" "{:30.30s} {:30.30s}{:>9}{:>13}{:>13}{:>11s}" "\x1b[0m\n",
			statistics.name,
			str_executor,
			this->get_existing_thread_count(),
			statistics.now.count_execute.load(),
			count_pended,
			vector_pended.empty() ? std::string("-") : std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(vector_pended.front().first - tick_now).count()))
		);

		// 5) pended executables (up to MAX_PRINT_PENDED)
		if (vector_pended.empty() == false)
		{
			buf_output.append_text("\x1b[90m" "---------------------------------------------------------------------------------------------------------------------------------\n" "\x1b[0m"sv);
			buf_output.append_text(std::format("\x1b[37m" "{:>11s}  {}\n" "\x1b[0m", "in(ms)", "executable"));

			for (size_t i = 0; i < vector_pended.size() && i < MAX_PRINT_PENDED; ++i)
			{
				buf_output.append_text(std::format("{:>11}  {:.110s}\n",
					std::chrono::duration_cast<std::chrono::milliseconds>(vector_pended[i].first - tick_now).count(),
					vector_pended[i].second)
				);
			}

			if (vector_pended.size() > MAX_PRINT_PENDED)
				buf_output.append_text(std::format("\x1b[90m" "{:>11s}  ... ({} more)\n" "\x1b[0m", "", vector_pended.size() - MAX_PRINT_PENDED));
		}
		buf_output.append_text("\x1b[37m" "---------------------------------------------------------------------------------------------------------------------------------\n" "\x1b[0m"sv);

		// 6) output
	#if defined(_WIN32)
		DWORD number_written = 0;
		WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), buf_output.data(), buf_output.size<DWORD>(), &number_written, nullptr);
	#else
		fwrite(buf_output.data(), 1, buf_output.size<int>(), stdout);
	#endif
	}
	catch (...)
	{
	}
}


bool executor::scheduler::process_execute(chrono::tick::duration /*_tick_diff*/, intptr_t /*_option*/)
{
	// 1) get current thread
	auto pthread = api::get_current_thread();

	// declare) 실행할 시각이 된 executable들 (lock 안에서 꺼내고 lock 밖에서 post한다)
	std::vector<object_ptr<Iexecutable>> vector_due;

	while (this->m_flag_run)
	{
		try
		{
			for(;;)
			{
				// check) 
				if(this->has_executor() == false)
					break;

				// declare)
				bool is_stop = false;

				CONDITIONAL_LOCK(this->m_lockable_priorityqueue, lock_wait)
				{
					// 1) wait until 
					if (this->m_priorityqueue_executable.empty())
						this->m_cv_priorityqueue_exist.wait(lock_wait);

					// 2) get now tick
					auto tick_now = chrono::tick::now();

					// statistics) 
					pthread->statistics_executing_begin(tick_now);

					// 3) execute 
					while (this->m_priorityqueue_executable.empty() == false)
					{
						// - get first executable object
						EXECUTION_AT& pexecutable = this->m_priorityqueue_executable.at(0);

						// check) wait if not yet 
						if(pexecutable.tick_at > tick_now)
						{
							// check) 이미 꺼낸 것이 있으면 기다리기 전에 먼저 post한다.
							if (vector_due.empty() == false)
								break;

							// statistics) 
							pthread->statistics_executing_end();

							// - wait time
							auto time_wait = pexecutable.tick_at - tick_now;

							// - sleep
#if defined(_WIN32)
							if (this->m_handle_timer != nullptr)
							{
								// - 고해상도 타이머를 설정한다. (상대 시간, 100ns 단위)
								LARGE_INTEGER due_time;
								due_time.QuadPart = -(std::max)(static_cast<int64_t>(1), static_cast<int64_t>(std::chrono::duration_cast<std::chrono::duration<int64_t, std::ratio<1, 10000000>>>(time_wait).count()));
								::SetWaitableTimer(this->m_handle_timer, &due_time, 0, nullptr, nullptr, FALSE);

								// - lock을 풀고 타이머나 깨우기 이벤트(새로운 더 이른 실행, 취소)를 기다린다.
								HANDLE handles[2] = { this->m_handle_timer, this->m_handle_wake };

								lock_wait.unlock();
								::WaitForMultipleObjects(2, handles, FALSE, INFINITE);
								lock_wait.lock();
							}
							else
#endif
							{
								this->m_cv_priorityqueue_in_time.wait_for(lock_wait, time_wait);
							}

							// - get now tick
							tick_now = chrono::tick::now();

							// statistics) 
							pthread->statistics_executing_begin(tick_now);

							// continue)
							continue;
						}

						// - 꺼낸다. (post는 lock 밖에서 한다)
						vector_due.push_back(std::move(pexecutable.pexecutable));

						// statistics)
						++this->m_statistics.now.count_execute;

						// - remove 'executable'
						this->process_pop_executable();
					}

					// check)
					if(this->m_flag_run == 0)
						is_stop = true;
				}

				// 4) 꺼낸 executable을 lock 밖에서 post한다.
				//    - post는 syscall(PostQueuedCompletionStatus / eventfd)이라 lock 안에서 하면 그동안 다른 thread의 post_at()/cancel()이 기다렸다.
				//    - 꺼낸 순서(실행 시각 순서)대로 post한다. 하나가 exception을 던져도 나머지를 post한 뒤 첫 exception을 다시 던진다.
				if (vector_due.empty() == false)
				{
					// declare)
					auto pexecutor = this->executor();
					std::exception_ptr pexception;

					for (auto& iter : vector_due)
					{
						try
						{
							if (pexecutor.exist())
								pexecutor->post(std::move(iter));
						}
						catch (...)
						{
							if (!pexception)
								pexception = std::current_exception();
						}
					}

					// - clear
					vector_due.clear();

					// reraise)
					if (pexception)
					{
						pthread->statistics_executing_end();
						std::rethrow_exception(pexception);
					}
				}

				// statistics)
				pthread->statistics_executing_end();

				// check)
				if (is_stop)
					break;

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
	// lock)
	std::lock_guard<std::mutex> lock(this->m_lockable_priorityqueue);

	// check) 
	if(this->m_flag_run == 0)
		return;

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
		if(iter_now->tick_at <= _executable.tick_at)
			break;

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
		this->m_cv_priorityqueue_exist.notify_one();
	else if (iter_child->pexecutable == this->m_priorityqueue_executable[0].pexecutable)
		this->_notify_in_time();
}

void executor::scheduler::_notify_in_time() noexcept
{
	// 설명) 가장 이른 실행 시각이 바뀌었을 때(추가, 취소) 대기 중인 스레드를 깨운다.
	this->m_cv_priorityqueue_in_time.notify_one();

#if defined(_WIN32)
	if (this->m_handle_wake != nullptr)
		::SetEvent(this->m_handle_wake);
#endif
}

void executor::scheduler::process_pop_executable() noexcept
{
	// 1) 
	auto ptarget = std::move(this->m_priorityqueue_executable.back());
	auto tick_compare = ptarget.tick_at;

	// 2) target객체를 제거하고 제일 마지막 객체를 그 위치로 가져 온다.
	this->m_priorityqueue_executable.pop_back();

	// check) 
	if(this->m_priorityqueue_executable.empty())
		return;

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
		if (tick_compare <= iter_now->tick_at)
			break;

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
