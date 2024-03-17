#include "pch.h"

#if defined(_WIN32)
	#include "process.h"
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
	RETURN_IF(pthread_now == nullptr, object_ptr<Iexecutor>());

	// return)
	return pthread_now->m_pexecutor;
}


api::Nthread::~Nthread() noexcept
{
	this->reset();
}

void api::Nthread::reset() noexcept
{
	// 1) Thread 객체를 reset한다.
	if (this->m_thread.joinable())
	{
		this->m_thread.detach();
	}

	// 2) Thread ID를 reset한다.
	this->m_statistics_thread.id_thread = 0;
}

void api::Nthread::register_thread()
{
	// declare) 
	auto pmanager = api::manager_thread::instance();

	// check) pmanager
	CGASSERT_ERROR(pmanager.exist());

	// 1) Pool Manager에 등록한다.
	auto result = pmanager->register_thread(this);
	
	// check) 
	RETURN_IF(result == false);

	// 2) Manager를 등록한다.
	scoped_lock(this->m_pmanager)
	{
		// check)
		CGASSERT_ERROR(this->m_pmanager.empty());

		// - 등록...
		this->m_pmanager = pmanager;
	}
}

void api::Nthread::unregister_thread() noexcept
{
	// declare) 
	object_ptr<api::manager_thread>	pmanager;

	scoped_lock(this->m_pmanager)
	{
		pmanager = this->m_pmanager;
		this->m_pmanager.reset();
	}

	// check) 
	RETURN_IF(pmanager.empty());

	// 1) UnRegister한다.
	if(pmanager->unregister_thread(this)==false)
	{
		// - 해당 Thread를 찾지 못했다.
		CGASSERT_ERROR(false);
	}
}

void api::Nthread::statistics_executing_begin() noexcept
{
	statistics_executing_begin(chrono::tick::now());
}

void api::Nthread::statistics_executing_begin(chrono::tick::time_point _tick ) noexcept
{
	this->m_statistics_thread.is_executing = true;

	this->m_statistics_thread.tick_last_execute = _tick;

	++this->m_statistics_thread.count_executed;
}

void api::Nthread::statistics_executing_end() noexcept
{
	this->m_statistics_thread.is_executing = false;
}

void api::Nthread::statistics_error() noexcept
{
	++this->m_statistics_thread.count_error;
}

bool api::Nthread::is_time_out(chrono::tick::duration _tick_execute) const noexcept
{
	// check) 
	RETURN_IF(this->m_statistics_thread.is_executing == false, false);

	// 1) 차이값을 계산한다.
	chrono::tick::duration tick_differ = chrono::tick::now() - this->m_statistics_thread.tick_last_execute;

	// return) 
	return (tick_differ >= _tick_execute);
}

bool api::Nthread::is_time_out(chrono::tick::duration _tick_execute, chrono::tick::time_point _tick_now) const noexcept
{
	// check) 
	RETURN_IF(this->m_statistics_thread.is_executing == false, false);

	// 1) 차이값을 계산한다.
	chrono::tick::duration tick_differ = _tick_now - this->m_statistics_thread.tick_last_execute;

	// return) 
	return (tick_differ >= _tick_execute);
}

#ifdef _WIN32
__declspec (noreturn) void raise_throw()
{
	throw throwable::force_raise();
}
#endif

bool api::Nthread::wait()
{
#if _MSC_VER>=_MSC_VER_2015
#define	USE_STD
#endif

#if !defined(_MSC_VER) || defined(USE_STD)
	// check)
	RETURN_IF(this->m_thread.joinable() == false, false);

	// 1) wait한다.
	this->m_thread.join();
#else
	// check) 
	RETURN_IF(this->m_thread.joinable() == false, false);

	// 1) Datach한다.
	if (this->m_thread.joinable())
	{
		this->m_thread.detach();
	}

	// declare)
	uint32_t dwResult;

	// 2) 종료를 기다림 (최대 60초)
	while((dwResult = WaitForSingleObject(this->m_thread.native_handle(), 60000)) == WAIT_TIMEOUT)
	{
	#ifndef NDEBUG
		// - 강제 종료시킨다.
		this->Terminate();

		// trace) 
		LOG_ERROR << "(err ) CGDK.system.object: thread is abortive closed!";

		// Break)
		break;
	#endif
	}
#endif

	// return) 
	return true;
}

#if defined(_WIN32)
bool api::Nthread::terminate(uint32_t _exit_code, chrono::tick::duration /*_wait*/)
{
#pragma warning( push )
#pragma warning( disable : 6258)	// TerminateThread는 항상 경고를 내므로 이렇게 처리해 놓는다.
	// check) 
	RETURN_IF(this->m_thread.get_id() == std::thread::id(), false);

	// warning) don't use this function!!!
	CGASSERT_ERROR(false);

	// 1) Terminate한다.
	::TerminateThread(m_thread.native_handle(), _exit_code);

	// 2) Handle을 닫는다.
	if (this->m_thread.joinable())
	{
		this->m_thread.detach();
	}

	// statistics)
	this->m_statistics_thread.id_thread = 0;
	this->m_statistics_thread.is_executing = false;

	// 성공!!
	return true;
#pragma warning( pop )
}
#else
bool api::Nthread::terminate(uint32_t /*_exit_code*/, chrono::tick::duration /*_wait*/)
{
	// 1) get thread handle
	auto thread_handle = this->thread_handle();

	// 2) kill thread
	::pthread_kill(thread_handle, SIGINT);

	// 3) wait until end
	::pthread_join(thread_handle, NULL);

	printf("thread terminated");

	// 성공!!
	return false;
}
#endif

bool api::Nthread::raise_exception()
{
#if defined (_MSC_VER)
	// 1) 일단 Thread를 suspend한다.
	uint32_t dwVal = suspend();

	// check) 
	RETURN_IF(dwVal == INFINITE, false);

#if defined(_WIN32)
#if defined(_M_X64)
	// declare) 
	WOW64_CONTEXT context_control;

	// 2) ContextFlag를 변경
	context_control.ContextFlags = CONTEXT_CONTROL;

	// 3) Thread Context를 얻는다.
	BOOL result = ::Wow64GetThreadContext(thread_handle(), &context_control);

	// check) 얻지 못했다면 false를 리턴한다.
	ERROR_RETURN_IF(result == FALSE, false, this->resume());

	// 4) Throw() 함수로 점프하도록 한다.
	context_control.Eip = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(raise_throw));

	// 5) Context를 설정한다.
	result = Wow64SetThreadContext(thread_handle(), &context_control);

	// check) 얻지 못했다면 false를 리턴한다.
	ERROR_RETURN_IF(result == FALSE, false, this->resume());
#elif defined(_M_IX86) 
	// declare) 
	CONTEXT context_control;

	// 2) ContextFlag를 변경
	context_control.ContextFlags = CONTEXT_CONTROL;

	// 3) Thread Context를 얻는다.
	auto result = GetThreadContext(thread_handle(), &context_control);

	// check) 얻지 못했다면 false를 리턴한다.
	ERROR_RETURN_IF(result == FALSE, false, this->resume());

	// 4) Throw() 함수로 점프하도록 한다.
	context_control.Eip = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(raise_throw));

	// 5) Context를 설정한다.
	result = SetThreadContext(thread_handle(), &context_control);

	// check) 얻지 못했다면 false를 리턴한다.
	ERROR_RETURN_IF(result == FALSE, false, this->resume());
#endif
#else
#endif

	// 6) resume한다.
	this->resume();
#else

#endif

	// return) 성공!
	return true;
}

uint32_t api::Nthread::suspend()
{
#if defined (_MSC_VER)/* && !defined(WINAPI_FAMILY)*/
#if defined(_M_X64)
	return ::Wow64SuspendThread(thread_handle());
#else
	return SuspendThread(thread_handle());
#endif
#else
	return 0;
#endif
}

uint32_t api::Nthread::resume()
{
#if defined (_MSC_VER)/* && !defined(WINAPI_FAMILY)*/
	return ::ResumeThread(thread_handle());
#else
	return 0;
#endif
}

void api::Nthread::process_begin_thread()
{
	// 1) Exception Translator를 설정한다.
	set_seh_to_thread();

	// 2) 강제 Exception Raise를 위해 Optimization을 방지해주는 Code...
#if defined(_WIN32)
	volatile int i = 0;
	if (i) raise_throw();
#endif

	// 3) 현재 Thread를 설정한다.
	g_pthread_now = this;

	// 4) Thread를 등록한다.
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
	scoped_lock(this->m_lockable_executor)
	{
		// check) ...
		RETURN_IF(this->m_is_blocked == true);

		// check) ...
		RETURN_IF(this->m_pexecutor == nullptr);

		// 1) notify blocked
		this->m_pexecutor->control(1, 0);

		// 2) blocked == true;
		this->m_is_blocked = true;
	}
}
void api::Nthread::notify_unblocked() noexcept
{
	scoped_lock(this->m_lockable_executor)
	{
		// check) ...
		RETURN_IF(this->m_is_blocked == false);

		// check) ...
		RETURN_IF(this->m_pexecutor == nullptr);

		// 1) notify unblocked
		this->m_pexecutor->control(2, 0);

		// 2) blocked == true;
		this->m_is_blocked = false;
	}
}


}