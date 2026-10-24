
#include "pch.h"

#if defined(_MSC_VER)
	#include <intrin.h>
#endif

#if defined(__cpp_impl_coroutine)

namespace CGDK::coroutine
{
//-----------------------------------------------------------------------------
// coroutine frame (memory_block pool)
//
//  - a header is placed in front of the frame. it keeps the memory_block to
//    free. (nullptr: allocated by ::operator new when the pool is not ready)
//-----------------------------------------------------------------------------
namespace
{
	constexpr std::size_t FRAME_HEADER_SIZE = 16;
	static_assert(sizeof(Imemory*) <= FRAME_HEADER_SIZE);
}

void* promise_base::operator new(std::size_t _size)
{
	// 1) alloc from the memory_block pool
	Imemory* pmemory = nullptr;

	try
	{
		auto pblock = mem_alloc(_size + FRAME_HEADER_SIZE);

		// - take over the reference of the block (released in operator delete. no add_ref and release)
		pmemory = pblock.get();
		pblock.detach();
	}
	catch (...)
	{
		// - the memory_block pool is not ready (before initialized or after destroyed)
		pmemory = nullptr;
	}

	// 2) memory (the pool is not ready: ::operator new)
	auto pframe = (pmemory != nullptr) ? reinterpret_cast<char*>(pmemory->data()) : static_cast<char*>(::operator new(_size + FRAME_HEADER_SIZE));

	// 3) header
	*reinterpret_cast<Imemory**>(pframe) = pmemory;

	// return)
	return pframe + FRAME_HEADER_SIZE;
}

void promise_base::operator delete(void* _pointer) noexcept
{
	// check)
	if (_pointer == nullptr)
		return;

	// 1) header
	auto pframe = static_cast<char*>(_pointer) - FRAME_HEADER_SIZE;
	auto pmemory = *reinterpret_cast<Imemory**>(pframe);

	// 2) free
	if (pmemory != nullptr)
		pmemory->release();
	else
		::operator delete(pframe);
}


//-----------------------------------------------------------------------------
// deferred release
//
//  - the top coroutine (get(), start()) releases its frame in final_suspend
//    while it is still running. so the release is deferred until the running
//    resume finishes. (the outermost resume_scope of the thread)
//-----------------------------------------------------------------------------
namespace
{
	thread_local int t_resume_depth = 0;
	thread_local std::vector<promise_base*> t_release_deferred;
	thread_local int t_coroutine_depth = 0;
	thread_local const char* t_stack_base = nullptr;		// the stack position of the outermost resume of this thread (_is_stack_deep)
	constexpr std::ptrdiff_t STACK_DEEP_LIMIT = 256 * 1024;

	// - the position of the stack of this thread (an address in the current frame)
	inline const char* stack_position() noexcept
	{
	#if defined(_MSC_VER)
		return static_cast<const char*>(_AddressOfReturnAddress());
	#else
		return static_cast<const char*>(__builtin_frame_address(0));
	#endif
	}
	std::atomic<std::size_t> g_count_detached { 0 };

	// - marks that a CGDK coroutine runs on this thread (is_in_coroutine())
	class coroutine_scope
	{
	public:
		coroutine_scope() noexcept { ++t_coroutine_depth; }
		~coroutine_scope() noexcept { --t_coroutine_depth; }
	};

	class resume_scope
	{
	public:
		resume_scope() noexcept { if (t_resume_depth++ == 0) t_stack_base = stack_position(); }
		~resume_scope() noexcept
		{
			// check) not the outermost
			if (--t_resume_depth != 0)
				return;

			// - release the deferred frames
			while (!t_release_deferred.empty())
			{
				auto ppromise = t_release_deferred.back();
				t_release_deferred.pop_back();

				ppromise->_release_frame();
			}
		}
	};
}

bool promise_base::_is_stack_deep() noexcept
{
	// check) outside a resume : the base of the stack is unknown
	if (t_resume_depth == 0)
		return false;

	// - the distance from the outermost resume (the direction of the growth of the stack does not matter)
	auto distance = t_stack_base - stack_position();

	return distance > STACK_DEEP_LIMIT || distance < -STACK_DEEP_LIMIT;
}

void promise_base::_release_frame_deferred() noexcept
{
	// 1) in a resume: release after it finishes
	if (t_resume_depth > 0)
	{
		try
		{
			t_release_deferred.push_back(this);
			return;
		}
		catch (...)
		{
		}
	}

	// 2) not in a resume (or out of memory): release now
	this->_release_frame();
}


//-----------------------------------------------------------------------------
// promise_base::executable
//-----------------------------------------------------------------------------
intptr_t promise_base::executable::process_execute(intptr_t, size_t)
{
	// - take the action first. (this executable can be posted again while the action runs)
	auto pfunction = std::exchange(this->m_pfunction, nullptr);
	auto pcontext = this->m_pcontext;

	// - run (nothing of this executable is touched after it)
	if (pfunction != nullptr)
	{
		resume_scope scope;

		pfunction(pcontext);
	}

	// return)
	return intptr_t(0);
}

void promise_base::executable::process_dispose() noexcept
{
	// - no owner and no executor holds it any more: destroy the coroutine frame
	//   (this executable is a part of the frame. nothing is touched after it)
	auto handle = this->m_handle;
	auto is_detached = this->m_is_detached;

	if (handle)
		handle.destroy();

	// - started by task::start(): the frame is freed. (wait_detached())
	if (is_detached && g_count_detached.fetch_sub(1, std::memory_order_acq_rel) == 1)
		g_count_detached.notify_all();
}


//-----------------------------------------------------------------------------
// promise_base
//-----------------------------------------------------------------------------
void promise_base::_on_resume(void* _pcontext) noexcept
{
	// - resume (the frame may be destroyed while it runs. nothing is touched after it)
	auto handle = static_cast<promise_base*>(_pcontext)->m_executable.m_handle;

	coroutine_scope scope;
	handle.resume();
}

void promise_base::_on_time(void* _pcontext) noexcept
{
	// - the time has come (on the scheduler): resume on the executor of the coroutine
	auto ppromise = static_cast<promise_base*>(_pcontext);
	//   (this promise is alive: the scheduler holds its executable. its executor is not changed while the coroutine runs)
	(void)ppromise->_resume_on(ppromise->m_pexecutor.get());
}

bool promise_base::_resume_on(Iexecutor* _pexecutor) noexcept
{
	// 1) hold the executor
	//    - post한 뒤에는 이 promise를 건드리면 안 된다. (다른 thread에서 이미 재개되어 끝났을 수 있다)
	//      그래서 post하는 동안 executor가 사라지지 않도록 참조를 잡는다.
	//    - system executor는 사라지지 않으므로 (system_executor::executor_ptr()) 참조를 잡지 않는다. (재개할 때마다 참조 count를 올리지 않도록)
	object_ptr<Iexecutor> pexecutor_hold;

	if (_pexecutor == nullptr)
		_pexecutor = system_executor::executor_ptr();
	else if (_pexecutor != static_cast<Iexecutor*>(system_executor::executor_ptr_peek()))
		pexecutor_hold = _pexecutor;

	// 2) action
	this->m_executable.m_pfunction = &promise_base::_on_resume;
	this->m_executable.m_pcontext = this;

	// 3) post (this promise must not be touched after it is posted)
	bool is_posted = false;

	if (_pexecutor != nullptr)
	{
		try
		{
			is_posted = _pexecutor->post(object_ptr<Iexecutable>(&this->m_executable));
		}
		catch (...)
		{
			is_posted = false;
		}
	}

	// check) posted
	if (is_posted)
		return true;

	// 4) no executor, or the executor refused it (stopped): the coroutine is NOT resumed inline.
	//    - running it on this thread would break the executor binding. (strand, io thread, ...)
	//    - it will never be resumed, so it is cancelled: the top of its chain is released (the frames are destroyed
	//      with their locals) or the thread waiting in get() is woken up with an exception. (_on_resume_refused)
	LOG_ERROR << "(!) coroutine: the executor refused to resume the coroutine (stopped). the coroutine is cancelled"sv;

	this->m_executable.m_pfunction = nullptr;
	this->_on_resume_refused();

	// return)
	return false;
}

void promise_base::_resume_at(chrono::tick::time_point _tick_resume) noexcept
{
	// 1) action
	this->m_executable.m_pfunction = &promise_base::_on_time;
	this->m_executable.m_pcontext = this;

	// 2) post to the scheduler (this promise must not be touched after it is posted)
	bool is_posted = false;

	try
	{
		is_posted = system_executor::scheduler()->post_at(object_ptr<Iexecutable>(&this->m_executable), _tick_resume);
	}
	catch (...)
	{
		is_posted = false;
	}

	// check) posted
	if (is_posted)
		return;

	// 3) the scheduler refused it: resume now on the executor (refused there too: cancelled)
	this->m_executable.m_pfunction = nullptr;

	(void)this->_resume_on(this->m_pexecutor.get());
}

void promise_base::_resume_here_or_on_executor() noexcept
{
	// 1) bound to the system_executor: resume right here (already on a thread of it)
	if (this->m_pexecutor.empty() || this->m_pexecutor.get() == static_cast<Iexecutor*>(system_executor::executor_ptr_peek()))
	{
		auto handle = this->m_executable.m_handle;
		coroutine_scope scope;
		handle.resume();
		return;
	}

	// 2) bound to another executor: resume on it (refused: cancelled)
	(void)this->_resume_on(this->m_pexecutor.get());
}

void promise_base::_resume_here() noexcept
{
	// - resume right here (the frame may be destroyed while it runs. nothing is touched after it)
	//   the release of the frame in final_suspend is deferred until this resume finishes. (resume_scope)
	auto handle = this->m_executable.m_handle;

	resume_scope scope;
	coroutine_scope scope_coroutine;

	handle.resume();
}

void promise_base::_mark_detached() noexcept
{
	this->m_executable.m_is_detached = true;

	g_count_detached.fetch_add(1, std::memory_order_relaxed);
}

bool promise_base::_post_to_system(void (*_pfunction)(void*), void* _pcontext) noexcept
{
	// 1) action
	this->m_executable.m_pfunction = _pfunction;
	this->m_executable.m_pcontext = _pcontext;

	// 2) post (this promise must not be touched after it is posted)
	bool is_posted = false;

	try
	{
		is_posted = system_executor::post(object_ptr<Iexecutable>(&this->m_executable));
	}
	catch (...)
	{
		is_posted = false;
	}

	// check) refused
	if (!is_posted)
		this->m_executable.m_pfunction = nullptr;

	// return)
	return is_posted;
}


//-----------------------------------------------------------------------------
// functions
//-----------------------------------------------------------------------------
object_ptr<Iexecutor> default_executor()
{
	return object_ptr<Iexecutor>(system_executor::executor());
}

bool is_in_coroutine() noexcept
{
	return t_coroutine_depth > 0;
}

std::size_t count_detached() noexcept
{
	return g_count_detached.load(std::memory_order_acquire);
}

void wait_detached()
{
	for (auto count = g_count_detached.load(std::memory_order_acquire); count != 0; count = g_count_detached.load(std::memory_order_acquire))
		g_count_detached.wait(count);
}

void on_unhandled_exception(std::exception_ptr _exception) noexcept
{
	try
	{
		if (_exception)
			std::rethrow_exception(_exception);
	}
	catch (const throwable::failure& _e)
	{
		LOG_ERROR << "(err) coroutine: unhandled exception (throwable::failure) "sv << _e.description();
	}
	catch (const std::exception& _e)
	{
		LOG_ERROR << "(err) coroutine: unhandled exception (std::exception) "sv << _e.what();
	}
	catch (...)
	{
		LOG_ERROR << "(err) coroutine: unhandled exception (unknown)"sv;
	}
}

}

#endif
