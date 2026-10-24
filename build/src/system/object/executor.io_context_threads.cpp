#include "pch.h"
#include <charconv>
#include "platform.h"

using namespace CGDK;


executor::io_context_threads::io_context_threads() :
	executor::io_context(),
	api::threads(),
	m_is_active(false),
	m_enable_auto_expand{ false },
	m_count_threads_blocked{ 0 },
	m_count_threads_extra_threshold{ 0 },
	m_count_threads_extra_active{ 0 },
	m_count_threads_extra_exist{ 0},
	m_count_threads_extra_max{ 0 },
	m_cs_threads_extra(),
	m_thread_index(0)
{
}

executor::io_context_threads::~io_context_threads() noexcept
{
	this->executor::io_context_threads::stop();
}

result_code executor::io_context_threads::start(context& _context)
{
	// 1) Initializable
	auto* pInitializable = dynamic_cast<object::Iinitializable*>(this);

	// 2) Initializable이 있으면 initialize한다.
	if (pInitializable != nullptr)
	{
		pInitializable->initialize();
	}

	// declare)
	int thread_count = static_cast<int>(CGEXECUTOR_AUTO);
	int cpu_affinity = -1;
	bool enable_auto_expand = false;
	uint32_t busy_poll_usecs = 0;
	uint16_t busy_poll_budget = 0;
	bool prefer_busy_poll = false;
	eTHREAD_PRIORITY thread_priority = eTHREAD_PRIORITY::DEFAULT;

	if (_context.type() == context::value_type_t::buffer_)
	{
		// - get parameter
		auto& parameter = _context.v.value_buffer.front<START_PARAMETER>();

		// - get suitable provider;
		thread_count = parameter.thread_count;
		cpu_affinity = parameter.cpu_affinity;
		busy_poll_usecs = parameter.busy_poll_usecs;
		busy_poll_budget = parameter.busy_poll_budget;
		prefer_busy_poll = parameter.prefer_busy_poll;
		thread_priority = parameter.thread_priority;
	}
	else
	{
		// - Value를 얻는다.
		const auto& context_thread_count = _context.find("thread_count"sv, "thread count"sv, "threadcount"sv);

		// - Address를 얻는다.
		if (context_thread_count.is_valid())
		{
			std::string str_thread_count = context_thread_count;

			std::from_chars(str_thread_count.data(), str_thread_count.data() + str_thread_count.size(), thread_count);
		}

		// - "cpu_affinity" : the CPU the threads are pinned to
		if (const auto& context_cpu_affinity = _context.find("cpu_affinity"sv, "cpu affinity"sv); context_cpu_affinity.is_valid())
		{
			std::string str_cpu_affinity = context_cpu_affinity;

			std::from_chars(str_cpu_affinity.data(), str_cpu_affinity.data() + str_cpu_affinity.size(), cpu_affinity);
		}

		// - "busy_poll" (microseconds), "busy_poll_budget", "prefer_busy_poll" : the busy poll of epoll_wait() of this io_context
		if (const auto& context_busy_poll = _context.find("busy_poll"sv, "busy poll"sv, "busy_poll_usecs"sv); context_busy_poll.is_valid())
		{
			std::string str_busy_poll = context_busy_poll;

			std::from_chars(str_busy_poll.data(), str_busy_poll.data() + str_busy_poll.size(), busy_poll_usecs);
		}

		if (const auto& context_busy_poll_budget = _context.find("busy_poll_budget"sv, "busy poll budget"sv); context_busy_poll_budget.is_valid())
		{
			std::string str_busy_poll_budget = context_busy_poll_budget;

			std::from_chars(str_busy_poll_budget.data(), str_busy_poll_budget.data() + str_busy_poll_budget.size(), busy_poll_budget);
		}

		if (const auto& context_prefer_busy_poll = _context.find("prefer_busy_poll"sv, "prefer busy poll"sv); context_prefer_busy_poll.is_valid())
		{
			std::string str_prefer_busy_poll = context_prefer_busy_poll;

			prefer_busy_poll = (CGDK::__stricmp(str_prefer_busy_poll, "true") == 0 || CGDK::__stricmp(str_prefer_busy_poll, "on") == 0 || str_prefer_busy_poll == "1");
		}

		// - "thread_priority" : "lowest", "low", "normal", "high", "highest", "time_critical"
		if (const auto& context_thread_priority = _context.find("thread_priority"sv, "thread priority"sv); context_thread_priority.is_valid())
		{
			std::string str_thread_priority = context_thread_priority;

			thread_priority = to_thread_priority(str_thread_priority);

			if (thread_priority == eTHREAD_PRIORITY::DEFAULT && CGDK::__stricmp(str_thread_priority, "default") != 0)
				LOG_WARNING << "(warn) executor["sv << name() << "]: unknown 'thread_priority' ["sv << str_thread_priority << "] (ignored)"sv;
		}
	}

	// 5) Thread 개수가 0개 이하면 Thread 개수를 계산해 낸다.
	if (thread_count == static_cast<int>(CGEXECUTOR_AUTO))
	{
		thread_count = static_cast<int>(api::get_core_count()) * DEFAULT_THREAD_COUNT_PER_PROCESSOR + DEFAULT_BASE_THREAD_COUNT;
	}

	// check) 최대 Thread 개수 제한 (최대 Thread수는 1024개로 제한한다.)
	SATURATE_MAX(thread_count, 1024);

	// 6) I/O CompletionPort를 생성한다.
	executor::io_context::start(_context);

	// - the busy poll of epoll_wait() of this io_context (replaces the default applied by io_context::_process_start())
	if (busy_poll_usecs != 0)
	{
	#if defined(__ANDROID__) || defined(__linux__)
		if (this->backend() != executor::io_backend::epoll)
			LOG_WARNING << "(warn) executor["sv << name() << "]: the busy poll of epoll_wait() is ignored on the io_uring backend"sv;
		else if (this->set_busy_poll(busy_poll_usecs, busy_poll_budget, prefer_busy_poll))
			LOG_INFO << "  - executor["sv << name() << "] busy poll : "sv << busy_poll_usecs << "us (budget:"sv << busy_poll_budget << ", prefer:"sv << prefer_busy_poll << ")"sv;
	#else
		LOG_WARNING << "(warn) executor["sv << name() << "]: 'busy_poll' is ignored (linux epoll only)"sv;
	#endif
	}

	// 7) Done을 true로...
	this->m_is_active = true;
	this->m_enable_auto_expand = enable_auto_expand;
	this->m_cpu_affinity = cpu_affinity;
	this->m_thread_priority = thread_priority;
	this->m_is_thread_priority_warned = false;
	this->m_count_threads_blocked = 0;
	this->m_count_threads_extra_threshold = static_cast<int>(api::get_core_count());
	this->m_count_threads_extra_active = 0;
	this->m_count_threads_extra_exist = 0;
	this->m_count_threads_extra_max = thread_count;
	this->m_thread_index = 0;

	// - the io_context has workers (linux io_uring : only the threads below enter the ring from now. the SQEs of the other threads wait
	//   for them : a request is owned by the thread that submitted it)
#if defined(__ANDROID__) || defined(__linux__)
	if (thread_count > 0)
		this->m_io_context.expect_workers();
#endif

	try
	{
		// 8) 원하는 개수만큼 Thread를 만든다.
		this->begin_threads(thread_count, [this]()
			{
				return this->process_thread_run();
			});

		// trace)
		if (cpu_affinity >= 0)
			LOG_INFO << "  - start "sv << thread_count << " threads on executor["sv << name() << "] (pinned to cpu "sv << cpu_affinity << ")"sv;
		else
			LOG_INFO << "  - start "sv << thread_count << " threads on executor["sv << name() << "]"sv;

		// trace)
		if (thread_priority != eTHREAD_PRIORITY::DEFAULT)
			LOG_INFO << "  - executor["sv << name() << "] thread priority : "sv << to_string(thread_priority);
	}
	catch (...)
	{
		// - 먼저 executor을 닫는다.
		this->executor::io_context::stop();

		// - m_is_active을 false로 환원
		this->m_is_active = false;

		// reraise)
		throw;
	}

	// return) 성공...
	return result_code(eRESULT::SUCCESS);
}

result_code executor::io_context_threads::stop() noexcept
{
	// 1) stop without waiting (the io_context is disposed, the threads end by themselves)
	this->request_stop();

	// 2) wait until all the threads end
	auto result = this->wait();

	// check)
	if(result == false)
		return result_code(eRESULT::FAIL);

	// return)
	return result_code(eRESULT::SUCCESS);
}

void executor::io_context_threads::request_stop() noexcept
{
	// 1) dispose the io_context (the threads waiting on it wake up)
	this->executor::io_context::stop();

	// 2) the threads end by themselves (m_is_active is checked by their loop)
	this->m_is_active = false;
	this->m_count_threads_blocked = 0;

	// 3) wake up the extra threads
	this->_process_stop_extra_thread();
}

void executor::io_context_threads::_apply_thread_priority() noexcept
{
	// check)
	RETURN_IF(this->m_thread_priority == eTHREAD_PRIORITY::DEFAULT);

	// 1) set the priority of this thread
	if (api::set_current_thread_priority(this->m_thread_priority))
		return;

	// log) once per start
	if (this->m_is_thread_priority_warned.exchange(true) == false)
		LOG_WARNING << "(warn) executor["sv << name() << "]: fail to set the thread priority '"sv << to_string(this->m_thread_priority) << "' (no permission?)"sv;
}

object_ptr<executor::Iio_context_threads> executor::make_io_context_threads(std::string_view _name, int _thread_count)
{
	// 1) create
	auto pexecutor = make_object<executor::io_context_threads>();

	// 2) name
	pexecutor->name(_name);

	// 3) start (throws on a failure)
	executor::io_context_threads::START_PARAMETER parameter{ (_thread_count > 0) ? _thread_count : 1 };

	pexecutor->start(parameter);

	// return)
	return object_ptr<executor::Iio_context_threads>(pexecutor.get());
}

object_ptr<executor::Iio_context_threads> executor::make_io_context_threads(std::string_view _name, int _thread_count, int _cpu_affinity, uint32_t _busy_poll_usecs)
{
	// 1) create
	auto pexecutor = make_object<executor::io_context_threads>();

	// 2) name
	pexecutor->name(_name);

	// 3) start (throws on a failure). the threads are pinned to the CPU, the busy poll of epoll_wait() (0 : the default)
	executor::io_context_threads::START_PARAMETER parameter{ (_thread_count > 0) ? _thread_count : 1, _cpu_affinity, _busy_poll_usecs };

	pexecutor->start(parameter);

	// return)
	return object_ptr<executor::Iio_context_threads>(pexecutor.get());
}

object_ptr<executor::Iio_context_threads> executor::make_io_context_threads(std::string_view _name, context& _context)
{
	// 1) create
	auto pexecutor = make_object<executor::io_context_threads>();

	// 2) name
	pexecutor->name(_name);

	// 3) start by the context (throws on a failure) : "thread_count", "cpu_affinity", "busy_poll", "busy_poll_budget", "prefer_busy_poll", "thread_priority"
	pexecutor->start(_context);

	// return)
	return object_ptr<executor::Iio_context_threads>(pexecutor.get());
}

uint32_t executor::io_context_threads::process_thread_run()
{
	//-----------------------------------------------------------------
	// 1. 무한대로 돌린다~
	//
	//    - 여기서 이중 while을 두었는데 이는 Exception의 처리 때문이라고
	//      할 수 있다.
	//    - 만약 execute()중에 Exception이 발생할 경우 Catch에서 받아
	//      처리하고 만약 m_is_active이 아직 true일 경우 계속 처리하게 된다.
	//      만약 m_is_active이 false면 바로 빠져나온다.
	//
	//-----------------------------------------------------------------
	// 1) index
	int thread_index = this->m_thread_index++;

	// - pinned to a CPU (START_PARAMETER::cpu_affinity)
	if (this->m_cpu_affinity >= 0)
		api::set_current_thread_affinity(this->m_cpu_affinity);

	// - the priority of the threads (START_PARAMETER::thread_priority) : set before the first execution
	this->_apply_thread_priority();

	// 2) executing thread 수 증가
	++this->m_statistics.now.thread_count;

	// - a worker of the io_context (io_uring_completion : the ring of this thread)
	this->process_bind_worker();

	while (this->m_is_active)
	{
		// 3) i/o thread의 무한 loop를 돌린다.
		try
		{
			// - execute를 수행한다.
			this->execute(1s, thread_index);
		}
		catch (...)
		{
		}
	}

	// 4) executing thread 수 감소
	this->process_unbind_worker();
	--this->m_statistics.now.thread_count;

	// return)
	return 0;
}

uint32_t executor::io_context_threads::process_thread_run_extra()
{
	//-----------------------------------------------------------------
	// 1. extra용 thread
	//
	//    - 메인 thread가 blocking되어 있을 때 임시로 실행되는 thread
	//    - 메인 thread가 blocking에서 풀리면 이 스레드는 종료된다.
	//
	//-----------------------------------------------------------------
	// 1) index
	int thread_index = this->m_thread_index++;

	// - pinned to the CPU of the executor
	if (this->m_cpu_affinity >= 0)
		api::set_current_thread_affinity(this->m_cpu_affinity);

	// - the priority of the threads (START_PARAMETER::thread_priority) : set before the first execution
	this->_apply_thread_priority();

	// 2) executing thread 수 증가
	++this->m_statistics.now.thread_count;
	SCOPED_LOCK_NAMED(this->m_cs_threads_extra, cs_extra)
	{
		++this->m_count_threads_extra_active;
	}

	// - a transient worker of the io_context (io_uring_completion : no ring of its own. this thread exits when the blocked
	//   workers return, and the kernel cancels every io_uring request of an exiting thread)
	this->process_bind_worker(true);

	while (this->m_is_active)
	{
		// 4) i/o thread의 무한 loop를 돌린다.
		try
		{
			// - 현재 가용 스레드 수가 필요한 스레드 수보다 적으면 계속 돈다~
			while (this->m_is_active)
			{
				// - execute를 수행한다.
				this->execute(1s, thread_index);

				// - extra thread를 종료할 지 재검사한다.
				this->_is_extra_thread_run();
			}
		}
		catch (...)
		{
		}
	}

	// 5) executing thread 수 감소
	this->process_unbind_worker();
	--this->m_statistics.now.thread_count;
	{
		// lock)
		std::lock_guard<std::mutex> lock(this->m_cs_threads_extra);

		--this->m_count_threads_extra_active;
		--this->m_count_threads_extra_exist;
	}

	// return)
	return 0;
}

bool executor::io_context_threads::process_control_thread_blocked() noexcept
{
#if defined(__ANDROID__) || defined(__linux__)
	// - io_uring_completion : the ring of this thread (a worker) is executed by the other workers while it blocks
	this->m_io_context.thread_blocked();
#endif

	{
		// lock)
		std::lock_guard<std::mutex> lock(this->m_cs_threads_extra);

		// 1) increase m_count_threads_extra
		auto count_threads_blocked = ++this->m_count_threads_blocked;

		// 2) get extra count
		auto extra_count = count_threads_blocked - this->m_count_threads_extra_threshold;

		// 3) start extra thread if needed
		if (extra_count > this->m_count_threads_extra_active)
		{
			if (this->m_count_threads_extra_active < this->m_count_threads_extra_exist)
			{
				this->m_cv_threads_extra.notify_one();
			}
			else
			{
				// check)
				if(this->m_count_threads_extra_exist >= this->m_count_threads_extra_max)
					return false;

				++m_count_threads_extra_exist;

				// - start new extra thread
				this->begin_thread([this]()
					{
						return this->process_thread_run_extra();
					});
			}
		}
	}

	// return)
	return true;
}

bool executor::io_context_threads::process_control_thread_unblocked() noexcept
{
#if defined(__ANDROID__) || defined(__linux__)
	// - io_uring_completion : the ring of this thread is taken back
	this->m_io_context.thread_unblocked();
#endif

	// lock)
	std::lock_guard<std::mutex> lock(this->m_cs_threads_extra);

	--this->m_count_threads_blocked;

	// return)
	return true;
}

void executor::io_context_threads::process_begin_thread_object(api::thread* _pthread) noexcept
{
	{
		std::lock_guard cs(_pthread->m_lockable_executor);

		_pthread->m_pexecutor = this;
	}

	api::threads::process_begin_thread_object(_pthread);
}

void executor::io_context_threads::process_end_thread_object(api::thread* _pthread) noexcept
{
	api::threads::process_end_thread_object(_pthread);

	{
		std::lock_guard cs(_pthread->m_lockable_executor);

		_pthread->m_pexecutor = nullptr;
	}
}

void executor::io_context_threads::_is_extra_thread_run() noexcept
{
	// lock)
	std::unique_lock<std::mutex> cs_extra(this->m_cs_threads_extra);

	// check)
	if(this->m_is_active == false)
		return;

	// 2) get extra count
	auto extra_count = this->m_count_threads_blocked - this->m_count_threads_extra_threshold;

	// check)
	if(extra_count >= this->m_count_threads_extra_active)
		return;

	// - executing thread 수 감소
	--this->m_count_threads_extra_active;

	// - wait
	this->m_cv_threads_extra.wait(cs_extra);

	// - executing thread 수 증가
	++this->m_count_threads_extra_active;
}

void executor::io_context_threads::_process_stop_extra_thread() noexcept
{
	// lock)
	std::lock_guard<std::mutex> cs_extra(this->m_cs_threads_extra);

	// 1) notify all
	this->m_cv_threads_extra.notify_all();
}