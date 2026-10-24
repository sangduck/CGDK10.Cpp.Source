#include "pch.h"
#include "platform.h"

using namespace CGDK;


executor::io_context::io_context() noexcept
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

	// 2) unregister from factory manager
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
	// 1) queue을 생성한다.
	auto result = this->m_io_context.create();

	// check) IOCP의 생성에 실패했으면 Exception을 던진다.
	THROW_IF(result == false, throwable::failure(eRESULT::FAIL_TO_CREATE));

#if defined(__ANDROID__) || defined(__linux__)
	// - the backend (epoll / io_uring / io_uring_completion : executor::default_io_backend())
	LOG_INFO << "  - io_context['"sv << name() << "'] backend : "sv << ((this->m_io_context.backend() == executor::io_backend::io_uring_completion) ? "io_uring_completion"sv : (this->m_io_context.backend() == executor::io_backend::io_uring) ? "io_uring"sv : "epoll"sv);

	// - the default busy poll of epoll_wait() (executor::set_default_busy_poll()). an own value of the executor replaces it (io_context_threads)
	{
		uint32_t busy_poll_usecs = 0;
		uint16_t busy_poll_budget = 0;
		bool prefer_busy_poll = false;
		executor::get_default_busy_poll(busy_poll_usecs, busy_poll_budget, prefer_busy_poll);

		if (busy_poll_usecs != 0)
		{
			if (this->m_io_context.backend() != executor::io_backend::epoll)
				LOG_WARNING << "(warn) io_context['"sv << name() << "']: the busy poll of epoll_wait() is ignored on the io_uring backend"sv;
			else if (this->m_io_context.set_busy_poll(busy_poll_usecs, busy_poll_budget, prefer_busy_poll))
				LOG_INFO << "  - io_context['"sv << name() << "'] busy poll : "sv << busy_poll_usecs << "us (budget:"sv << busy_poll_budget << ", prefer:"sv << prefer_busy_poll << ")"sv;
		}
	}
#endif

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

void executor::io_context::process_bind_worker(bool _is_transient) noexcept
{
#if defined(__ANDROID__) || defined(__linux__)
	// - io_uring_completion : the ring of this thread (the others : nothing). a transient thread (an extra thread while the
	//   workers block : it exits later, and the kernel cancels the requests of an exiting thread) gets no ring
	this->m_io_context.bind_worker(_is_transient);
#else
	static_cast<void>(_is_transient);
#endif
}

void executor::io_context::process_unbind_worker() noexcept
{
#if defined(__ANDROID__) || defined(__linux__)
	this->m_io_context.unbind_worker();
#endif
}

bool executor::io_context::process_post(object_ptr<Iexecutable>&& _pexecutable)
{
	// check) _pexecutable가 NULL이면 Post를 던지지 않는다.
	if(_pexecutable.empty())
		return false;

	// 1) Post한다.
	return this->m_io_context.post_executable(0, 0, std::move(_pexecutable)) != 0;
}

#if defined(__ANDROID__) || defined(__linux__)
namespace
{
	// - io_uring completion : the sends of the handlers of a batch are deferred to its end (CGDK_IO_URING_BATCH_DEFER=0 : off)
	bool _is_batch_send_deferred() noexcept
	{
		static const bool s_value = []()
		{
			const char* penv = std::getenv("CGDK_IO_URING_BATCH_DEFER");
			return penv == nullptr || (penv[0] != '0' && penv[0] != 'f' && penv[0] != 'F' && penv[0] != 'n' && penv[0] != 'N');
		}();

		return s_value;
	}

	// - the sends deferred in a batch are sent before the next completion once the batch ran this long (a long handler)
	constexpr auto BATCH_SEND_DEFER_DURATION = std::chrono::microseconds(100);
}
#endif

bool executor::io_context::process_execute(chrono::tick::duration _ticks_wait, [[maybe_unused]] intptr_t _option)
{
	//-----------------------------------------------------------------
	struct AUTO_RELEASE_EXECUTABLE
	{
	public:
		Iexecutable*		m_pexecutable;
		api::Nthread*		m_pthread_current;
		chrono::tick::time_point m_start_time;

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
			CGASSERT_FATAL(this->m_pthread_current != nullptr);

		#if defined(_ENABLE_DEBUG_NEW)
			// check) ....
			CGASSERT_ERROR(sizeof(Iexecutable) > 32);

			// - 제일 앞  32Byte만 복사한다. virtual table 정도만 복사해 놓기 위해...
			memcpy(this->m_debugInfo_pexecutable, reinterpret_cast<void*>(_pexecutable), 32);
		#endif

			// - 실행 시작 시간을 기록한다.
			this->m_start_time = chrono::tick::now();

			// statistics)
			this->m_pthread_current->statistics_executing_begin();
		}

		~AUTO_RELEASE_EXECUTABLE() noexcept
		{
			this->reset();
		}

		void reset() noexcept
		{
			// check) m_pexecutable가 nullptr이면 그냥 리턴한다.
			CGASSERT_FATAL(this->m_pexecutable != nullptr);

			// statistics)
			this->m_pthread_current->statistics_executing_end();

			// statistics)
			this->m_pexecutable->statistics_on_execute_elapsed_time(chrono::tick::now() - this->m_start_time);

			// 2) m_pexecutable을 release한다.
			this->m_pexecutable->release();

			// 3) m_pexecutable을 nullptr로 설정한다.
			this->m_pexecutable = nullptr;
		}
	};
	//-----------------------------------------------------------------

	// check)
	if(this->handle() == INVALID_CGHANDLE_VALUE)
		return false;

#if defined(__ANDROID__) || defined(__linux__)
	// declare)
	api::io_context::POP_RESULT array_result[api::io_context::MAX_EVENT_BATCH];

	// - 한번에 받을 event 수 (thread별로 실행 시간에 맞춰 조절한다)
	//   epoll fd는 모든 thread가 함께 쓴다. 한 thread가 받은 event들은 순서대로 실행되므로 뒤쪽 event는 앞의 실행 시간만큼 기다린다.
	//   event 처리가 짧으면 많이 받아 epoll_wait 횟수와 경합을 줄이고, 길면 적게 받아 다른 thread가 나눠 실행하도록 한다.
	//   (batch 하나의 실행 시간이 EVENT_BATCH_DURATION 정도가 되도록 한다)
	static constexpr auto EVENT_BATCH_DURATION = std::chrono::microseconds(10);
	thread_local int t_count_event_batch = 1;

	// 1) get results from io_context (한번의 epoll_wait/io_uring_enter로 여러 event를 받는다. executable은 add_ref된 상태다)
	//    (epoll/io_uring : event와 executable. io_uring_completion : 이 thread의 ring의 CQE (op의 res/flags), post된 executable)
	int count_event = this->m_io_context.pop_results(array_result, t_count_event_batch, _ticks_wait);

	// - 실행 시작 시간
	const auto tick_start = (count_event != 0) ? chrono::tick::now() : chrono::tick::time_point{};

	// declare)
	std::size_t count_executed = 0;
	std::exception_ptr pexception;

	// 2) 'process_execute'
	//    - 받은 event는 모두 실행한다. 하나가 exception을 던져도 나머지를 실행한 뒤 첫 exception을 다시 던진다.
	//      (EPOLLONESHOT event는 다시 오지 않는다 : 실행하지 않고 버리면 그 socket은 더 이상 event를 받지 못한다)
	//    - io_uring_completion : the batch is visible to the backend while it runs (a handler that blocks hands the rest of it to the
	//      workers that run the ring of this thread meanwhile : count_event is cut. api::io_context_uring_cq::thread_blocked())
	api::io_context_uring_cq::BATCH batch{ array_result, &count_event, 0, this->m_io_context.completion_context(), api::io_context_uring_cq::current_batch() };

	//      the outermost batch is published on the ring too : a worker finding one executable of it running long (CPU) lends the ring and
	//      takes the rest (batch_next() : false, count_event cut. api::io_context_uring_cq::_lend_busy())
	if (batch.pcontext != nullptr)
	{
		api::io_context_uring_cq::current_batch() = &batch;
		api::io_context_uring_cq::batch_begin(batch);
	}

	// - io_uring completion : the sends of the batch are deferred and sent at its end (send_defer batch : like a group lock around the
	//   batch). a socket gets a completion per arrival (multishot RECV) : the replies to the completions of a socket in a batch go out in
	//   one send instead of one per completion (epoll reads them with one recv and sends once). a batch of one : the send cork of the socket
	const bool is_batch_deferred = (batch.pcontext != nullptr && count_event > 1 && _is_batch_send_deferred() && send_defer::begin_batch());
	auto tick_batch_flushed = tick_start;

	for (int i = 0; i < count_event; ++i)
	{
		// declare)
		const auto& result_event = array_result[i];

		// - the result executing now (io_uring completion : claimed. the rest taken by a worker that lent the ring : the end)
		if (i != 0 && batch.pring != nullptr && api::io_context_uring_cq::batch_next(batch, i) == false)
			break;

		batch.index = i;

		// check)
		CONTINUE_IF(result_event.pexecutable == nullptr);

		try
		{
			// declare)
			AUTO_RELEASE_EXECUTABLE	pexecutable(result_event.pexecutable);

			// - the batch : what was deferred in it is sent before this one once the batch ran for a while (the start time of this one)
			if (is_batch_deferred && pexecutable.m_start_time - tick_batch_flushed >= BATCH_SEND_DEFER_DURATION)
			{
				send_defer::flush();
				tick_batch_flushed = pexecutable.m_start_time;
			}

			// - send defer (auto) : the consecutive sends of the executable to one socket are sent at once (at the switch to another socket / the end)
			send_defer::auto_scope scope_send_defer;

			// - call 'process_execute'
			pexecutable.m_pexecutable->process_execute(result_event.result, result_event.param);
		}
		catch (...)
		{
			if (!pexception)
				pexception = std::current_exception();

			// continue)
			continue;
		}

		// - count
		++count_executed;
	}

	// - the deferred sends of the batch
	if (is_batch_deferred)
		send_defer::end_batch();

	// - the batch is not visible any more (the outer one of a handler pumping the executor again)
	if (batch.pcontext != nullptr)
	{
		api::io_context_uring_cq::batch_end(batch);
		api::io_context_uring_cq::current_batch() = batch.pouter;
	}

	// - io_uring : submits the re-arms this thread deferred while processing the batch (one io_uring_enter). epoll : nothing
	//   io_uring_completion : writes them (submitted with the enter of the next pop_results())
	this->m_io_context.flush_pending();

	// 3) 다음에 받을 event 수를 정한다. (event 하나의 평균 실행 시간으로 EVENT_BATCH_DURATION을 채울 수 있는 수)
	if (count_event != 0)
	{
		const auto tick_elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(chrono::tick::now() - tick_start).count();
		const auto count_fit = (tick_elapsed > 0) ? (std::chrono::nanoseconds(EVENT_BATCH_DURATION).count() * count_event / tick_elapsed) : api::io_context::MAX_EVENT_BATCH;

		// - 한번에 크게 바뀌지 않도록 이전 값과 평균한다.
		t_count_event_batch = std::clamp(static_cast<int>((t_count_event_batch + std::min<int64_t>(count_fit, api::io_context::MAX_EVENT_BATCH) + 1) / 2), 1, api::io_context::MAX_EVENT_BATCH);
	}

	// statistics)
	if (count_executed != 0)
		this->m_statistics.now.count_execute.fetch_add(count_executed, std::memory_order_relaxed);		// - statistics (relaxed)

	// reraise)
	if (pexception)
		std::rethrow_exception(pexception);

	// check)
	if (count_executed == 0)
	{
		// - process idle
		this->process_idle();

		// return)
		return false;
	}

	// return)
	return true;
#else
	// declare) 완료를 한 번에 여러 개 받는다. (GetQueuedCompletionStatusEx : syscall 한 번에 여러 개)
	//   - 받은 것은 이 thread가 모두 실행하므로 하나가 오래 걸리면 뒤의 것이 기다린다. 그래서 받는 수를 실행 시간에 맞춘다.
	//     (batch 하나의 실행 시간이 EVENT_BATCH_DURATION 정도가 되도록 한다 : linux 와 같은 규칙)
	api::io_context::RESULT array_result[api::io_context::MAX_EVENT_BATCH];
	static constexpr auto EVENT_BATCH_DURATION = std::chrono::microseconds(10);
	thread_local int t_count_event_batch = 1;

	// 1) get executables from io_context
	const int count_event = this->m_io_context.pop_results(array_result, t_count_event_batch, _ticks_wait);

	// - 실행 시작 시간
	const auto tick_start = (count_event != 0) ? chrono::tick::now() : chrono::tick::time_point{};

	// declare)
	std::size_t count_executed = 0;
	std::exception_ptr pexception;

	// 2) 'process_execute'
	//    - 받은 것은 모두 실행한다. 하나가 exception을 던져도 나머지를 실행한 뒤 첫 exception을 다시 던진다.
	//      (꺼낸 completion은 다시 오지 않는다 : 실행하지 않고 버리면 그 I/O의 executable이 완료되지 않는다)
	for (int i = 0; i < count_event; ++i)
	{
		// declare)
		const auto& result_event = array_result[i];

		// check)
		if (result_event.is_valid() == false)
			continue;

		try
		{
			// declare)
			AUTO_RELEASE_EXECUTABLE	pexecutable(result_event.executable());

			// - call 'process_execute'
			pexecutable.m_pexecutable->process_execute(result_event.result(), result_event.param());
		}
		catch (...)
		{
			if (!pexception)
				pexception = std::current_exception();

			// continue)
			continue;
		}

		// - count
		++count_executed;
	}

	// 3) 다음에 받을 수를 정한다. (하나의 평균 실행 시간으로 EVENT_BATCH_DURATION을 채울 수 있는 수)
	if (count_event != 0)
	{
		const auto tick_elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(chrono::tick::now() - tick_start).count();
		const auto count_fit = (tick_elapsed > 0) ? (std::chrono::nanoseconds(EVENT_BATCH_DURATION).count() * count_event / tick_elapsed) : api::io_context::MAX_EVENT_BATCH;

		// - 한번에 크게 바뀌지 않도록 이전 값과 평균한다.
		t_count_event_batch = std::clamp(static_cast<int>((t_count_event_batch + std::min<int64_t>(count_fit, api::io_context::MAX_EVENT_BATCH) + 1) / 2), 1, api::io_context::MAX_EVENT_BATCH);
	}

	// statistics)
	if (count_executed != 0)
		this->m_statistics.now.count_execute.fetch_add(count_executed, std::memory_order_relaxed);		// - statistics (relaxed)

	// reraise)
	if (pexception)
		std::rethrow_exception(pexception);

	// check)
	if (count_executed == 0)
	{
		// - process idle
		this->process_idle();

		// return)
		return false;
	}

	// return)
	return true;
#endif
}

void executor::io_context::execute_all() noexcept
{
	//-----------------------------------------------------------------
	// 설명) 이중 for문인 이유!!!
	//   만약 execute(0)함수의 실행 중 Exception이 발생했을 경우 
	//   Catch에서 잡은 후 다시 다음을 실행하기 위해 for문이 이중이다.
	//   만약에 더 이상 실행할 executor가 없을 경우 false가 리턴되면
	//   바로 리턴 된다.
	//
	//   이때 for문을 이중으로해서 TRY문을 뺀 이유는 성능의 향상을 위해서다.
	//   매번 for문이 돌때마다 try와 catch가 반복될 경우 성능을 하락시킬 수
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


//-----------------------------------------------------------------------------
// the default busy poll of epoll_wait() (executor::set_default_busy_poll : applied by _process_start() to every epoll io_context created afterwards)
//-----------------------------------------------------------------------------
namespace
{
	// - packed : usecs (32) | budget (16) | prefer (1). one atomic : read as a whole by _process_start()
	std::atomic<uint64_t> s_default_busy_poll{ 0 };
}

void executor::set_default_busy_poll(uint32_t _usecs, uint16_t _budget, bool _prefer) noexcept
{
	const uint64_t packed = (static_cast<uint64_t>(_usecs) << 32) | (static_cast<uint64_t>(_budget) << 16) | (_prefer ? 1u : 0u);
	s_default_busy_poll.store(packed, std::memory_order_release);
}

void executor::get_default_busy_poll(uint32_t& _usecs, uint16_t& _budget, bool& _prefer) noexcept
{
	const uint64_t packed = s_default_busy_poll.load(std::memory_order_acquire);
	_usecs = static_cast<uint32_t>(packed >> 32);
	_budget = static_cast<uint16_t>(packed >> 16);
	_prefer = (packed & 1u) != 0;
}

bool executor::is_busy_poll_available() noexcept
{
#if defined(__ANDROID__) || defined(__linux__)
	return api::io_context_Nepoll::is_busy_poll_available();
#else
	return false;
#endif
}
