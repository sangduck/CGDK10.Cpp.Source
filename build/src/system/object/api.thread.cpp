#include "pch.h"
#include "platform.h"

#if defined(_WIN32)
	#include "process.h"
#elif defined(__linux__)
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#endif

namespace CGDK
{

extern TLS	api::Nthread* g_pthread_now;


int api::get_current_pid() noexcept
{
#if defined(_WIN32)
	return _getpid();
#else
	return static_cast<int>(getpid());
#endif
}

object_ptr<api::Nthread> api::get_current_thread() noexcept
{
	// check) Thread 객체가 없으면 Default 객체를 설정한다.
	if(g_pthread_now == nullptr)
	{
		g_pthread_now = api::manager_thread::get_default_thread_object();
	}

	// return) 
	return g_pthread_now;
}

object_ptr<Iexecutor> get_current_executor() noexcept
{
	// 1) thread_now
	auto pthread_now = g_pthread_now;

	// check) 
	if(pthread_now == nullptr) 
		return object_ptr<Iexecutor>();

	// return)
	return pthread_now->m_pexecutor;
}


api::Nthread::~Nthread() noexcept
{
	this->reset();
}

void api::Nthread::reset() noexcept
{
	{
		// lock)
		std::lock_guard lock(this->m_lockable_thread);

		// 1) Thread 객체를 reset한다.
		if (this->m_thread.joinable())
		{
			this->m_thread.detach();
		}
	}

	// 2) close control handle
	this->_close_control_handle();

	// 3) Thread ID를 reset한다.
	this->m_statistics_thread.id_thread = THREAD_ID{};
}

void api::Nthread::_on_thread_started()
{
	// lock)
	std::lock_guard lock(this->m_lockable_control);

	// 1) control handle for suspend() / resume() (still valid after m_thread is joined)
#if defined(_WIN32)
	if (this->m_handle_control != nullptr)
	{
		::CloseHandle(this->m_handle_control);
		this->m_handle_control = nullptr;
	}
	::DuplicateHandle(::GetCurrentProcess(), this->m_thread.native_handle(), ::GetCurrentProcess(), &this->m_handle_control, 0, FALSE, DUPLICATE_SAME_ACCESS);
#endif

	// statistics)
	this->m_statistics_thread.id_thread = this->m_thread.native_handle();
}

void api::Nthread::_close_control_handle() noexcept
{
#if defined(_WIN32)
	// lock)
	std::lock_guard lock(this->m_lockable_control);

	// 1) close
	if (this->m_handle_control != nullptr)
	{
		::CloseHandle(this->m_handle_control);
		this->m_handle_control = nullptr;
	}
#endif
}

void api::Nthread::register_thread()
{
	// declare) 
	auto pmanager = api::manager_thread::instance();

	// check) pmanager
	CGASSERT_FATAL(pmanager.exist());

	// 1) Pool Manager에 등록한다.
	auto result = pmanager->register_thread(this);
	
	// check) 
	if (result == false)
		return;

	// 2) Manager를 등록한다.
	{
		// lock)
		std::lock_guard lock(this->m_lockable_pmanager);

		// check)
		CGASSERT_FATAL(this->m_pmanager.empty());

		// - 등록...
		this->m_pmanager = pmanager;
	}
}

void api::Nthread::unregister_thread() noexcept
{
	// declare) 
	object_ptr<api::manager_thread>	pmanager;

	{
		// lock)
		std::lock_guard lock(this->m_lockable_pmanager);

		pmanager = this->m_pmanager;
		this->m_pmanager.reset();
	}

	// check) 
	if(pmanager.empty())
		return;

	// 1) UnRegister한다.
	if(pmanager->unregister_thread(this)==false)
	{
		// - 해당 Thread를 찾지 못했다.
		CGASSERT_FATAL(false);
	}
}

void api::Nthread::statistics_executing_begin() noexcept
{
	statistics_executing_begin(chrono::tick::now());
}

//-----------------------------------------------------------------------------
// 설명) 실행 통계 (이 thread만 쓰고 monitor(is_time_out)가 읽는다.)
//  - 실행할 때마다 불리므로 lock 계열 명령(seq_cst store=xchg, ++=lock xadd)을 쓰지 않는다.
//    (x86/arm64에서 relaxed/release store는 일반 store, 횟수는 relaxed load + store다.)
//  - tick_last_execute를 먼저 쓰고 is_executing을 release로 쓴다.
//    (is_executing을 acquire로 읽어 true면 그 실행의 tick_last_execute를 본다.)
//-----------------------------------------------------------------------------
void api::Nthread::statistics_executing_begin(chrono::tick::time_point _tick ) noexcept
{
	auto& statistics = this->m_statistics_thread;

	statistics.tick_last_execute.store(_tick, std::memory_order_relaxed);
	statistics.is_executing.store(true, std::memory_order_release);
	statistics.count_executed.store(statistics.count_executed.load(std::memory_order_relaxed) + 1, std::memory_order_relaxed);
}

void api::Nthread::statistics_executing_end() noexcept
{
	this->m_statistics_thread.is_executing.store(false, std::memory_order_release);
}

void api::Nthread::statistics_error() noexcept
{
	auto& statistics = this->m_statistics_thread;
	statistics.count_error.store(statistics.count_error.load(std::memory_order_relaxed) + 1, std::memory_order_relaxed);
}

bool api::Nthread::is_time_out(chrono::tick::duration _tick_execute) const noexcept
{
	// check) 
	if(this->m_statistics_thread.is_executing.load(std::memory_order_acquire) == false)
		return false;

	// 1) 차이값을 계산한다.
	chrono::tick::duration tick_differ = chrono::tick::now() - this->m_statistics_thread.tick_last_execute.load(std::memory_order_relaxed);

	// return) 
	return (tick_differ >= _tick_execute);
}

bool api::Nthread::is_time_out(chrono::tick::duration _tick_execute, chrono::tick::time_point _tick_now) const noexcept
{
	// check) 
	if(this->m_statistics_thread.is_executing.load(std::memory_order_acquire) == false)
		return false;

	// 1) 차이값을 계산한다.
	chrono::tick::duration tick_differ = _tick_now - this->m_statistics_thread.tick_last_execute.load(std::memory_order_relaxed);

	// return) 
	return (tick_differ >= _tick_execute);
}

bool api::Nthread::request_stop() noexcept
{
	// 1) copy the stop_source (does not wait for a thread joining m_thread)
	std::stop_source stop_source;
	{
		// lock)
		std::lock_guard lock(this->m_lockable_control);

		stop_source = this->m_stop_source;
	}

	// 2) request stop (outside the lock : stop_callbacks run here)
	return stop_source.request_stop();
}

bool api::Nthread::wait() noexcept
{
	// check) called on this thread itself => cannot wait for itself
	//  - checked before the lock : another thread may be holding it while joining this thread
	if (g_pthread_now == this)
		return false;

	// lock) only one thread joins at a time
	std::lock_guard lock(this->m_lockable_thread);

	// check) no thread
	if (this->m_thread.joinable() == false)
		return false;

	// check) called on this thread itself => cannot wait for itself
	if (this->m_thread.get_id() == std::this_thread::get_id())
		return false;

	// 1) wait한다.
	try
	{
		this->m_thread.join();
	}
	catch (...)
	{
		return false;
	}

	// statistics)
	this->m_statistics_thread.id_thread = THREAD_ID{};
	this->m_statistics_thread.is_executing = false;

	// return)
	return true;
}

bool api::Nthread::stop() noexcept
{
	// 1) request stop (the return value is not used : the stop may have been requested already)
	this->request_stop();

	// 2) wait until the thread ends
	return this->wait();
}

bool api::Nthread::force_raise_exception(const std::string_view& _message) noexcept
{
#if defined(_WIN32)
	// Windows implementation: APC-based exception injection
	{
		// lock)
		std::lock_guard lock(this->m_lockable_control);

		// check) thread handle exists
		if (this->m_handle_control == nullptr)
			return false;

		// check) don't call on self
		if (::GetCurrentThreadId() == ::GetThreadId(this->m_handle_control))
			return false;

		// 1) allocate and store exception message
		static thread_local std::string exception_msg;
		exception_msg = std::string(_message);

		// 2) APC callback that throws exception
		auto apc_callback = [](ULONG_PTR /*_param*/) noexcept
		{
			try
			{
				// This will be called when the thread is in alertable state
				throw std::runtime_error(exception_msg);
			}
			catch (...)
			{
				// Let the thread's exception handling deal with it
			}
		};

		try
		{
			// 3) Queue APC to the thread
			//    (thread must be in alertable state to execute: WaitForSingleObject, Sleep, etc.)
			PAPCFUNC pfn_apc = apc_callback;

			if (!::QueueUserAPC(pfn_apc, this->m_handle_control, 0))
			{
				// If QueueUserAPC fails, fallback to SetThreadContext method
				// 1) suspend thread
				if (::SuspendThread(this->m_handle_control) == static_cast<DWORD>(-1))
					return false;

				// 2) get thread context
				CONTEXT ctx = {};
				ctx.ContextFlags = CONTEXT_FULL;
				if (!::GetThreadContext(this->m_handle_control, &ctx))
				{
					::ResumeThread(this->m_handle_control);
					return false;
				}

				// 3) set RIP to terminate (safer than trying to throw C++ exception)
#ifdef _WIN64
				ctx.Rip = reinterpret_cast<DWORD64>(&std::terminate);
#else
				ctx.Eip = reinterpret_cast<DWORD>(&std::terminate);
#endif

				// 4) set context and resume
				if (!::SetThreadContext(this->m_handle_control, &ctx))
				{
					::ResumeThread(this->m_handle_control);
					return false;
				}

				::ResumeThread(this->m_handle_control);
				return true;
			}

			return true;
		}
		catch (...)
		{
			return false;
		}
	}
#elif defined(__linux__)
	// Linux : not supported (returns false). it installed a throwing SIGUSR2 handler, sent SIGUSR2 with pthread_kill() and restored the
	// old handler right away : the signal is delivered asynchronously (often after the restore : the old handler ran instead), SIGUSR2
	// is also the signal of the emergency terminate (net.emergency.manager) and of the dump manager, and throwing from a signal handler
	// is undefined behavior (the thread is interrupted at any instruction, holding any lock). api::manager_thread::check_deadlock() logs
	// and dumps a deadlocked thread : this only does nothing now (as on Windows, where the APC catches its own exception).
	(void)_message;
	return false;
#else
	// Unsupported platform
	(void)_message;
	return false;
#endif
}

uint32_t api::Nthread::suspend()
{
#if defined(_WIN32)
	// lock)
	std::lock_guard lock(this->m_lockable_control);

	// check)
	if (this->m_handle_control == nullptr)
		return static_cast<uint32_t>(-1);

	// 1) suspend
	return ::SuspendThread(this->m_handle_control);
#else
	return 0;
#endif
}

uint32_t api::Nthread::resume()
{
#if defined(_WIN32)
	// lock)
	std::lock_guard lock(this->m_lockable_control);

	// check)
	if (this->m_handle_control == nullptr)
		return static_cast<uint32_t>(-1);

	// 1) resume
	return ::ResumeThread(this->m_handle_control);
#else
	return 0;
#endif
}

void api::Nthread::process_set_exception_handler_to_thread() noexcept
{
#if defined(_WIN32)
	// 1) SEH Exception Handler를 설정한다.
	dump_manager::instance()->set_seh_to_thread();

	// 2) Invalid parameter Handler를 설정한다.
	dump_manager::instance()->set_invalid_parameter_handle();
#elif defined(__linux__)
	// 1) crash signal handler (process-wide : installed once)
	dump_manager::set_signal_handler_crash();
#endif
}

void api::Nthread::process_begin_thread()
{
	// 1) 강제 Exception Raise를 위해 Optimization을 방지해주는 Code...
	process_set_exception_handler_to_thread();

	// 2) 현재 Thread를 설정한다.
	g_pthread_now = this;

	// 3) Thread를 등록한다.
	this->register_thread();
}

void api::Nthread::process_end_thread()
{
	// 1) 현재 Thread를 reset한다.
	g_pthread_now = nullptr;

	// 2) Thread를 등록해제한다.
	this->unregister_thread();

	// 3) 기타...
	this->notify_unblocked();

}

void api::Nthread::notify_blocked() noexcept
{
	// lock)
	std::lock_guard lock(this->m_lockable_executor);

	// check) ...
	if(this->m_is_blocked == true)
		return;

	// check) ...
	if(this->m_pexecutor == nullptr)
		return;

	// - runnext : the slot of this thread goes to the shared queue (an executable posted before blocking is run by the other workers)
	executor::io_context::runnext_suspend();

	// 1) notify blocked
	this->m_pexecutor->thread_blocked();

	// 2) blocked == true;
	this->m_is_blocked = true;
}
void api::Nthread::notify_unblocked() noexcept
{
	// lock)
	std::lock_guard lock(this->m_lockable_executor);

	// check) ...
	if(this->m_is_blocked == false)
		return;

	// check) ...
	if(this->m_pexecutor == nullptr)
		return;

	// 1) notify unblocked
	this->m_pexecutor->thread_unblocked();

	// - runnext
	executor::io_context::runnext_resume();

	// 2) blocked == true;
	this->m_is_blocked = false;
}


}