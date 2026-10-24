#include "pch.h"
#include <charconv>

using namespace CGDK;


executor::queue_list_threads::queue_list_threads() :
	executor::queue_list(),
	api::threads(),
	m_is_active(false),
	m_enable_auto_expand(false),
	m_count_threads_blocked(0),
	m_count_threads_extra_threshold{ 0 },
	m_count_threads_extra_active{ 0 },
	m_count_threads_extra_exist{ 0},
	m_count_threads_extra_max{ 0 },
	m_lockable_threads_extra(),
	m_thread_index(0)
{
}

executor::queue_list_threads::~queue_list_threads() noexcept
{
	this->executor::queue_list_threads::stop();
}

result_code executor::queue_list_threads::start(context& _context)
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
	bool enable_auto_expand = false;

	if (_context.type() == context::value_type_t::buffer_)
	{
		// - get parameter
		auto& parameter = _context.v.value_buffer.front<START_PARAMETER>();

		// - get suitable provider;
		thread_count = parameter.thread_count;
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
	}

	// 5) Thread 개수가 0개 이하면 Thread 개수를 계산해 낸다.
	if (thread_count == static_cast<int>(CGEXECUTOR_AUTO))
	{
		thread_count = static_cast<int>(api::get_core_count()) * DEFAULT_THREAD_COUNT_PER_PROCESSOR + DEFAULT_BASE_THREAD_COUNT;
	}

	// check) 최대 Thread 개수 제한 (최대 Thread수는 1024개로 제한한다.)
	SATURATE_MAX(thread_count, 1024);

	// 6) I/O CompletionPort를 생성한다.
	executor::queue_list::start(_context);

	// 7) Done을 true로...
	this->m_is_active = true;
	this->m_enable_auto_expand = enable_auto_expand;
	this->m_count_threads_blocked = 0;
	this->m_count_threads_extra_threshold = static_cast<int>(api::get_core_count());
	this->m_count_threads_extra_active = 0;
	this->m_count_threads_extra_exist = 0;
	this->m_count_threads_extra_max = thread_count;
	this->m_thread_index = 0;

	try
	{
		// 8) 원하는 개수만큼 Thread를 만든다.
		this->begin_threads(thread_count, [this]()
			{
				return this->process_thread_run();
			});

		// trace) 
		LOG_INFO << "  - start "sv << thread_count << " threads on executor["sv << name() << "]"sv;
	}
	catch (...)
	{
		// - 먼저 executor을 닫는다.
		this->executor::queue_list::stop();

		// - m_is_active을 false로 환원
		this->m_is_active = false;

		// - 이미 만들어진 thread가 기다리지 않고 끝나게 한다.
		this->cancel_wait(true);

		// reraise)
		throw;
	}

	// return) 성공...
	return result_code(eRESULT::SUCCESS);
}

result_code executor::queue_list_threads::stop() noexcept
{
	// 1) 일단 모든 thread를 suspend 시킨다.

	// 2) 일단 queueing된 모든 executable을 실행해 실행 queue를 비운다.
	this->executor::queue_list::stop();

	// 3) Thread를 종료설정를 위해 m_is_active을 false로 설정한 후
	this->m_is_active = false;
	this->m_count_threads_blocked = 0;

	// - 기다리고 있는 thread를 모두 깨운다.
	//   (깨우지 않으면 각 thread가 wait의 timeout(1초)이 지나야 m_is_active를 다시 보므로 stop()이 최대 1초 늦어졌다)
	this->cancel_wait(true);

	// 4) stop extra threads
	this->_process_stop_extra_thread();

	// 5) 모든 Thread가 종료되길 대기한후 리턴한다. 
	auto result = this->wait();

	// check)
	if(result == false)
		return result_code(eRESULT::FAIL);

	// return)
	return result_code(eRESULT::SUCCESS);
}

uint32_t executor::queue_list_threads::process_thread_run()
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

	while (this->m_is_active)
	{
		// 2) executing thread 수 증가
		++this->m_statistics.now.thread_count;

		// 3) i/o thread의 무한 loop를 돌린다.
		try
		{
			// - 현재 가용 스레드 수가 필요한 스레드 수보다 적으면 계속 돈다~
			while (this->m_is_active)
			{
				// - execute를 수행한다.
				this->execute(1s, thread_index);
			}
		}
		catch (...)
		{
		}

		// 4) executing thread 수 감소
		--this->m_statistics.now.thread_count;

		// 5) 너무 많으면 wait에 들어간다.
		if (this->m_is_active)
		{
			// - wait한다.
		}
	}

	// return)
	return 0;
}

uint32_t executor::queue_list_threads::process_thread_run_extra()
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

	// 2) executing thread 수 증가
	++this->m_statistics.now.thread_count;
	SCOPED_LOCK_NAMED(this->m_lockable_threads_extra, cs_extra)
	{
		++this->m_count_threads_extra_active;
	}

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
	--this->m_statistics.now.thread_count;
	{
		// lock)
		std::lock_guard<std::mutex> lock(this->m_lockable_threads_extra);

		--this->m_count_threads_extra_active;
		--this->m_count_threads_extra_exist;
	}

	// return)
	return 0;
}

bool executor::queue_list_threads::process_control_thread_blocked() noexcept
{
	// lock)
	std::lock_guard<std::mutex> lock(this->m_lockable_threads_extra);

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

	// return)
	return true;
}

bool executor::queue_list_threads::process_control_thread_unblocked() noexcept
{
	// lock)
	std::lock_guard<std::mutex> lock(this->m_lockable_threads_extra);

	// 1) decrease
	--this->m_count_threads_blocked;

	// return)
	return true;
}

void executor::queue_list_threads::process_begin_thread_object(api::thread* _pthread) noexcept
{
	{
		std::lock_guard cs(_pthread->m_lockable_executor);

		_pthread->m_pexecutor = this;
	}

	api::threads::process_begin_thread_object(_pthread);
}

void executor::queue_list_threads::process_end_thread_object(api::thread* _pthread) noexcept
{
	api::threads::process_end_thread_object(_pthread);

	{
		std::lock_guard cs(_pthread->m_lockable_executor);

		_pthread->m_pexecutor = nullptr;
	}
}

void executor::queue_list_threads::_is_extra_thread_run() noexcept
{
	SCOPED_LOCK_NAMED(this->m_lockable_threads_extra, cs_extra)
	{
		// check)
		if(this->m_is_active == false)
			return;

		// check)
		if(this->m_count_threads_blocked >= this->m_count_threads_extra_active)
			return;

		// - executing thread 수 감소
		--this->m_count_threads_extra_active;

		// - wait
		this->m_cv_threads_extra.wait(cs_extra);

		// - executing thread 수 증가
		++this->m_count_threads_extra_active;
	}
}

void executor::queue_list_threads::_process_stop_extra_thread() noexcept
{
	// 1) notify all
	SCOPED_LOCK_NAMED(this->m_lockable_threads_extra, cs_extra)
	{
		this->m_cv_threads_extra.notify_all();
	}
}