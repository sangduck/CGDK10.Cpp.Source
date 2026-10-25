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
	EXECUTION_AT(chrono::tick::time_point _tick_at, object_ptr<Iexecutable>&& _pexecutable) noexcept : tick_at(_tick_at), pexecutable(std::move(_pexecutable)) {}

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
	//    - the thread checks m_flag_run under the queue lock before it sleeps and cancel() wakes it : it ends soon (a thread posting
	//      the executables it took out ends after them). yields first, then sleeps (the 1000 loops without a sleep ended in tens of
	//      microseconds : FAIL without the join while the thread still ran)
	{
	#ifdef NDEBUG
		// declare) 최대 executor 완료 대기시간 (10초)
		const auto tick_limit = chrono::tick::now() + std::chrono::seconds(10);
	#endif

		for (int count_loop = 0; this->get_existing_thread_count(); ++count_loop)
		{
		#ifdef NDEBUG
			// check)
			if (chrono::tick::now() >= tick_limit)
				return result_code(eRESULT::FAIL);
		#endif

			// - wake the thread
			this->cancel();

			// - yield, then sleep (windows : sleep_for(1ms) is about 15.6ms)
			if (count_loop < 100)
				std::this_thread::yield();
			else
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

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

	// 1) push (false : not running. stopped or not started : the executable is not executed)
	return this->process_push_executable(EXECUTION_AT(_tick_execute, std::move(_pexecutable)));
}

bool executor::scheduler::cancel(Iexecutable* _pexecutable) noexcept
{
	// check)
	if (_pexecutable == nullptr)
		return false;

	// declare) the removed one is released after the lock (its destructor may post_at() / cancel() on this scheduler)
	object_ptr<Iexecutable> pexecutable_removed;

	// lock)
	std::lock_guard<std::mutex> lock(this->m_lockable_priorityqueue);

	// 1) 힙 위치를 executable에서 바로 얻는다. (O(1))
	auto pos_now = _pexecutable->m_scheduler_position.index.load(std::memory_order_relaxed);

	// check) 위치가 맞지 않으면
	//    - 같은 executable이 여러 번(또는 다른 scheduler에도) 걸린 적이 없으면 걸려 있지 않은 것이다.
	//    - 있으면 위치가 덮어써졌을 수 있으므로 처음부터 찾는다.
	if (pos_now >= this->m_priorityqueue_executable.size() || this->m_priorityqueue_executable[pos_now].pexecutable != _pexecutable)
	{
		// check)
		if (_pexecutable->m_scheduler_position.is_shared.load(std::memory_order_relaxed) == false)
			return false;

		// - find
		auto iter_end = this->m_priorityqueue_executable.end();
		auto iter = std::find_if(this->m_priorityqueue_executable.begin(), iter_end, [=](const EXECUTION_AT& iter) { return iter.pexecutable == _pexecutable;});

		// check) return if not exists
		if (iter == iter_end)
			return false;

		pos_now = static_cast<size_t>(iter - this->m_priorityqueue_executable.begin());
	}

	// check) 가장 이른 것이 취소되면 대기 중인 스레드를 깨운다.
	if (pos_now == 0)
		this->_notify_in_time(); // notify one

	// 2) remove (released after the lock)
	pexecutable_removed = this->process_pop_executable(pos_now);

	// return)
	return true;
}

void executor::scheduler::cancel() noexcept
{
	// declare) the removed ones are released after the lock (their destructors may post_at() / cancel() on this scheduler)
	std::vector<EXECUTION_AT> vector_removed;

	// lock)
	std::lock_guard<std::mutex> lock(this->m_lockable_priorityqueue);

	if (this->m_priorityqueue_executable.empty() == false)
	{
		// - 힙 위치를 지운다. (다른 곳에 다시 걸려 덮어써진 것은 그대로 둔다)
		for (size_t pos = 0; pos < this->m_priorityqueue_executable.size(); ++pos)
		{
			auto& position = this->m_priorityqueue_executable[pos].pexecutable->m_scheduler_position;

			if (position.index.load(std::memory_order_relaxed) == pos)
				position.index.store(SIZE_MAX, std::memory_order_relaxed);
		}

		// - taken out (released after the lock)
		vector_removed.swap(this->m_priorityqueue_executable);
	}

	// - wake the thread (sleeping for the earliest one or for the first one : stop() calls this to end it)
	this->m_cv_priorityqueue_exist.notify_one();
	this->_notify_in_time();
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
					//    (m_flag_run is checked under the lock : a stop() setting it before this thread took the lock does not wait for
					//     a wakeup that already went)
					if (this->m_priorityqueue_executable.empty() && this->m_flag_run != 0)
					{
						this->m_is_waiting = true;
						this->m_tick_wait_until = (chrono::tick::time_point::max)();
						this->m_cv_priorityqueue_exist.wait(lock_wait, [this]() { return this->m_priorityqueue_executable.empty() == false || this->m_flag_run == 0; });
						this->m_is_waiting = false;
					}

					// 2) get now tick
					auto tick_now = chrono::tick::now();

					// statistics) 
					pthread->statistics_executing_begin(tick_now);

					// 3) execute 
					while (this->m_priorityqueue_executable.empty() == false)
					{
						// check) stopped (stop() cancels the rest)
						if (this->m_flag_run == 0)
							break;

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

							// - 자는 동안 이보다 이른 것이 들어올 때만 깨운다. (process_push_executable)
							const auto tick_at = pexecutable.tick_at;
							this->m_is_waiting = true;
							this->m_tick_wait_until = tick_at;

							// - spin wait (spin_wait()) : the last part of the wait is spun outside the lock (the timer wakes this thread
							//   that much earlier). woken by an earlier post_at / cancel / stop (_notify_in_time)
							const auto time_spin = std::chrono::nanoseconds(this->m_spin_wait_ns.load(std::memory_order_relaxed));

							if (time_spin.count() > 0 && time_wait <= time_spin)
							{
								this->m_is_spin_woken.store(false, std::memory_order_relaxed);

								lock_wait.unlock();
								while (chrono::tick::now() < tick_at && this->m_is_spin_woken.load(std::memory_order_acquire) == false)
									std::this_thread::yield();
								lock_wait.lock();
							}
							else
							{
								// - the timer wakes it time_spin earlier (spun after)
								time_wait -= time_spin;

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
							}

							this->m_is_waiting = false;

							// - get now tick
							tick_now = chrono::tick::now();

							// statistics) 
							pthread->statistics_executing_begin(tick_now);

							// continue)
							continue;
						}

						// - 꺼낸다. (post는 lock 밖에서 한다)
						vector_due.push_back(this->process_pop_executable(0));

						// statistics)
						++this->m_statistics.now.count_execute;
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

bool executor::scheduler::process_push_executable(EXECUTION_AT&& _executable)
{
	// declare)
	const auto tick_at = _executable.tick_at;

	// lock)
	std::unique_lock<std::mutex> lock(this->m_lockable_priorityqueue);

	// check) not running (stopped, not started) : refused (the caller releases it after the lock)
	if(this->m_flag_run == 0)
		return false;

	// declare)
	auto pexecutable = _executable.pexecutable.get();

	// check) 이미 어딘가에 걸려 있으면 그 위치는 덮어써지므로 cancel()이 찾아서 지우도록 표시한다.
	//    (on the executable : the scheduler it is pending in (another one too) searches when the position does not match)
	if (pexecutable->m_scheduler_position.index.load(std::memory_order_relaxed) != SIZE_MAX)
		pexecutable->m_scheduler_position.is_shared.store(true, std::memory_order_relaxed);

	// 1) push back item and heap up
	this->m_priorityqueue_executable.emplace_back();
	this->_heap_up(this->m_priorityqueue_executable.size() - 1, std::move(_executable));

	// check) 자고 있는 스레드가 스스로 깨는 시각보다 이를 때만 깨운다.
	//    - 깨운 시각을 기록해 두어 그보다 늦은 것이 이어서 들어오면 다시 깨우지 않는다.
	//    - 자고 있지 않으면 잠들기 전에 가장 이른 것을 다시 보므로 깨우지 않는다.
	if (this->m_is_waiting == false || tick_at >= this->m_tick_wait_until)
		return true;

	this->m_tick_wait_until = tick_at;
	const bool is_first = (this->m_priorityqueue_executable.size() == 1);

	// 2) notify_one (lock 밖에서 깨워 깨어난 스레드가 바로 lock을 기다리지 않게 한다)
	lock.unlock();

	if (is_first)
		this->m_cv_priorityqueue_exist.notify_one();
	else
		this->_notify_in_time();

	// return)
	return true;
}

void executor::scheduler::spin_wait(chrono::tick::duration _duration) noexcept
{
	this->m_spin_wait_ns.store((std::max)(static_cast<int64_t>(0), static_cast<int64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(_duration).count())), std::memory_order_relaxed);
}

chrono::tick::duration executor::scheduler::spin_wait() const noexcept
{
	return std::chrono::duration_cast<chrono::tick::duration>(std::chrono::nanoseconds(this->m_spin_wait_ns.load(std::memory_order_relaxed)));
}

void executor::scheduler::_notify_in_time() noexcept
{
	// 설명) 가장 이른 실행 시각이 바뀌었을 때(추가, 취소) 대기 중인 스레드를 깨운다.
	this->m_is_spin_woken.store(true, std::memory_order_release);
	this->m_cv_priorityqueue_in_time.notify_one();

#if defined(_WIN32)
	if (this->m_handle_wake != nullptr)
		::SetEvent(this->m_handle_wake);
#endif
}

void executor::scheduler::_heap_place(size_t _pos, EXECUTION_AT&& _executable) noexcept
{
	// 설명) 힙의 _pos에 놓고 그 위치를 executable에 기록한다.
	_executable.pexecutable->m_scheduler_position.index.store(_pos, std::memory_order_relaxed);
	this->m_priorityqueue_executable[_pos] = std::move(_executable);
}

void executor::scheduler::_heap_up(size_t _pos, EXECUTION_AT&& _executable) noexcept
{
	while (_pos != 0)
	{
		// - 빼기 1 후 나누기 2한다.
		auto pos_parent = (_pos - 1) >> 1;

		// check)
		if (this->m_priorityqueue_executable[pos_parent].tick_at <= _executable.tick_at)
			break;

		// - move to down
		this->_heap_place(_pos, std::move(this->m_priorityqueue_executable[pos_parent]));
		_pos = pos_parent;
	}

	this->_heap_place(_pos, std::move(_executable));
}

void executor::scheduler::_heap_down(size_t _pos, EXECUTION_AT&& _executable) noexcept
{
	// 1) Size
	const auto size = this->m_priorityqueue_executable.size();

	// 2) down heap
	for (;;)
	{
		// - get child node (pos x 2 + 1)
		auto pos_child = (_pos << 1) + 1;

		// check)
		if (pos_child >= size)
			break;

		// - Left가 더 크면 Right 선택한다.
		if (pos_child + 1 < size && this->m_priorityqueue_executable[pos_child].tick_at > this->m_priorityqueue_executable[pos_child + 1].tick_at)
			++pos_child;

		// check) Child가 더 크면 끝냄.
		if (_executable.tick_at <= this->m_priorityqueue_executable[pos_child].tick_at)
			break;

		// - move to up
		this->_heap_place(_pos, std::move(this->m_priorityqueue_executable[pos_child]));
		_pos = pos_child;
	}

	this->_heap_place(_pos, std::move(_executable));
}

object_ptr<Iexecutable> executor::scheduler::process_pop_executable(size_t _pos) noexcept
{
	// 1) 꺼낸다.
	auto pexecutable = std::move(this->m_priorityqueue_executable[_pos].pexecutable);

	// - 힙 위치를 지운다. (다른 곳에 다시 걸려 덮어써진 것은 그대로 둔다)
	if (pexecutable->m_scheduler_position.index.load(std::memory_order_relaxed) == _pos)
		pexecutable->m_scheduler_position.index.store(SIZE_MAX, std::memory_order_relaxed);

	// 2) 제일 마지막 객체를 그 위치로 가져 온다.
	auto ptarget = std::move(this->m_priorityqueue_executable.back());
	this->m_priorityqueue_executable.pop_back();

	// check)
	if (this->m_priorityqueue_executable.empty())
		return pexecutable;

	// check) 마지막 것을 꺼냈으면 끝
	if (_pos == this->m_priorityqueue_executable.size())
		return pexecutable;

	// 3) heap up or down
	if (_pos != 0 && this->m_priorityqueue_executable[(_pos - 1) >> 1].tick_at > ptarget.tick_at)
		this->_heap_up(_pos, std::move(ptarget));
	else
		this->_heap_down(_pos, std::move(ptarget));

	// return)
	return pexecutable;
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
