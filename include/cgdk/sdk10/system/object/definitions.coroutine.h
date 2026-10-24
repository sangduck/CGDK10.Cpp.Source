//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                            coroutine Classes                              *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
//*  Best for Game Developement and Optimized for Game Developement.          *
//*                                                                           *
//*                (c) 2003. Cho Sanghyun. All right reserved.                *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************

#pragma once

namespace CGDK
{
//-----------------------------------------------------------------------------
//
// coroutine - definitions
//
//  1) a CGDK coroutine is a function returning coroutine::task<T>.
//     it is called with 'co_await foo()' and gives its result to the caller.
//  2) 'co_await f' with a callable without parameters runs f() on the
//     system_executor and resumes with its result. (coroutine.function.h)
//  3) the top coroutine is started from a normal function like main() by
//     foo().get() (waits for the result) or foo().start() (does not wait).
//     (coroutine.task.h)
//     system_executor::post(owner, foo(), on_complete) / executor->post(...) starts it on an executor.
//     (coroutine.post.h)
//  4) a coroutine is bound to an executor (the system_executor) and every
//     resume after 'co_await' happens on that executor.
//  5) a child task awaited by 'co_await' inherits the executor of its parent.
//  6) 'co_await co_sleep_for(100ms)' and 'co_await co_sleep_until(time)' wait
//     without blocking a thread. (coroutine.sleep.h)
//  7) resuming needs no allocation and the coroutine frame is allocated from
//     the memory_block pool. (promise_base)
//
//-----------------------------------------------------------------------------
namespace coroutine
{
	//-----------------------------------------------------------------------------
	// forward declarations
	//-----------------------------------------------------------------------------
	template <class TRESULT>
	class task;

	template <class TFUNCTION>
	class function_awaiter;

	//-----------------------------------------------------------------------------
	// traits
	//-----------------------------------------------------------------------------
	// 1) is_task_v<T>
	template <class>
	inline constexpr bool is_task_v = false;
	template <class TRESULT>
	inline constexpr bool is_task_v<task<TRESULT>> = true;

	// 2) is_awaitable_v<T> : has await_ready() or operator co_await
	template <class TTYPE>
	inline constexpr bool is_awaitable_v =
		   requires(TTYPE& _value) { _value.await_ready(); }
		|| requires(TTYPE& _value) { _value.operator co_await(); }
		|| requires(TTYPE& _value) { operator co_await(_value); };

	// 3) is_function_awaitable_v<T> : a callable without parameters which is not awaitable itself
	//    and does not return coroutine::task<T>. ('co_await f' runs f() on the system_executor)
	namespace detail
	{
		template <class TFUNCTION, bool = std::is_invocable_v<TFUNCTION&>>
		inline constexpr bool is_function_awaitable = false;
		template <class TFUNCTION>
		inline constexpr bool is_function_awaitable<TFUNCTION, true> = !is_awaitable_v<TFUNCTION> && !is_task_v<std::remove_cvref_t<std::invoke_result_t<TFUNCTION&>>>;
	}

	template <class TTYPE>
	inline constexpr bool is_function_awaitable_v = detail::is_function_awaitable<std::decay_t<TTYPE>>;

	//-----------------------------------------------------------------------------
	// promise_base
	//  - base class of the promise types of CGDK coroutines.
	//
	//  1) keeps the executor the coroutine is bound to.
	//  2) has the executable which resumes the coroutine, so resuming needs no
	//     allocation. a suspended coroutine waits for only one thing, so one
	//     executable is enough.
	//  3) the reference count of the executable keeps the coroutine frame alive.
	//     - the owner (task or the detached coroutine) holds 1 reference.
	//     - an executor holds 1 reference while the executable is posted or running.
	//     - the frame is destroyed when the count becomes 0. so it is never
	//       destroyed while an executor still touches the executable after running it.
	//  4) the release of a top coroutine (get(), start()) is deferred until the
	//     running resume finishes, because it is still running in final_suspend.
	//  5) the coroutine frame is allocated from the memory_block pool.
	//-----------------------------------------------------------------------------
	class promise_base
	{
	public:
			promise_base() noexcept { this->m_executable.add_ref(); }
			promise_base(const promise_base&) = delete;
			promise_base&		operator = (const promise_base&) = delete;

	public:
		[[nodiscard]] auto			executor() const noexcept -> const object_ptr<Iexecutor>& { return this->m_pexecutor; }
				void				executor(const object_ptr<Iexecutor>& _pexecutor) noexcept { this->m_pexecutor = _pexecutor; }

		// await_transform) 'co_await f' with a callable: runs f() on the system_executor (coroutine.function.h)
			template <class TFUNCTION> requires is_function_awaitable_v<TFUNCTION>
		[[nodiscard]] auto			await_transform(TFUNCTION&& _function) { return function_awaiter<std::decay_t<TFUNCTION>>(std::forward<TFUNCTION>(_function)); }

		// await_transform) the others are awaited as they are
			template <class TAWAITABLE> requires (!is_function_awaitable_v<TAWAITABLE>)
		[[nodiscard]] TAWAITABLE&&	await_transform(TAWAITABLE&& _awaitable) noexcept { return std::forward<TAWAITABLE>(_awaitable); }

		// frame) the coroutine frame is allocated from the memory_block pool
		[[nodiscard]] static void*	operator new(std::size_t _size);
				static void			operator delete(void* _pointer) noexcept;

	public:
		// internal) set by get_return_object() of the promise type
				void				_set_handle(std::coroutine_handle<> _handle) noexcept { this->m_executable.m_handle = _handle; }
		// internal) the owner releases the frame. (destroyed when no executor holds the executable)
				void				_release_frame() noexcept { this->m_executable.release(); }
		// internal) holds the frame one more
				void				_add_ref_frame() noexcept { this->m_executable.add_ref(); }
		// internal) releases the frame after the running resume finishes. (used in final_suspend
		//           of the top coroutine. its frame must not be destroyed while it is still running)
				void				_release_frame_deferred() noexcept;
		// internal) marks the coroutine started by task::start() (counted until its frame is destroyed. wait_detached())
				void				_mark_detached() noexcept;
		// internal) resume the coroutine on '_pexecutor' (posted. nullptr means the default executor)
		//           - false: the executor refused the post (stopped). the coroutine is never resumed inline.
		//             it is cancelled by _on_resume_refused() and must not be touched any more.
		[[nodiscard]] bool			_resume_on(Iexecutor* _pexecutor) noexcept;
		// internal) called when the coroutine can not be resumed (the executor refused it). the promise type cancels the
		//           coroutine: releases the top of its chain (the frames are destroyed with their locals) or wakes up the waiting thread.
		virtual	void				_on_resume_refused() noexcept {}
		// internal) resume the coroutine on its executor at '_tick_resume'
				void				_resume_at(chrono::tick::time_point _tick_resume) noexcept;
		// internal) resume the coroutine here when it is bound to the system_executor, or on its executor
				void				_resume_here_or_on_executor() noexcept;
		// internal) resume the coroutine right here on this thread, inside a resume scope. (task::execute())
		//           (nothing of the frame is touched after it. the release of the frame in final_suspend is deferred until it returns)
				void				_resume_here() noexcept;
		// internal) run '_pfunction(_pcontext)' on the system_executor (false: refused)
		[[nodiscard]] bool			_post_to_system(void (*_pfunction)(void*), void* _pcontext) noexcept;
		// internal) true when the stack of this thread is deep (256KB) inside a resume. a compiler which does not tail-call the
		//           symmetric transfer (gcc -O0) nests the frames of a long chain of 'co_await task' : the continuation is posted then
		//           instead of transferred, which unwinds the stack. (false outside a resume : the base of the stack is unknown)
		[[nodiscard]] static bool	_is_stack_deep() noexcept;

	protected:
				object_ptr<Iexecutor> m_pexecutor;

	private:
		// executable) posted to resume the coroutine. a part of the coroutine frame (no allocation)
		class executable : public Iexecutable
		{
		public:
			virtual	intptr_t			process_execute(intptr_t _result, size_t _param) override;
			virtual	void				process_dispose() noexcept override;

		public:
				std::coroutine_handle<> m_handle;
				void				(*m_pfunction)(void*) = nullptr;
				void*				m_pcontext = nullptr;
				bool				m_is_detached = false;
		};

		static	void				_on_resume(void* _pcontext) noexcept;
		static	void				_on_time(void* _pcontext) noexcept;

				executable			m_executable;
	};

	template <class TPROMISE>
	inline constexpr bool is_cgdk_promise_v = std::is_base_of_v<promise_base, TPROMISE>;

	//-----------------------------------------------------------------------------
	// functions (implemented in coroutine.cpp of CGDK10.system.object library)
	//-----------------------------------------------------------------------------
	// 1) default executor (system_executor::executor())
	[[nodiscard]] object_ptr<Iexecutor> default_executor();

	// 2) called when an exception escapes from a coroutine started by task::start() (logged)
	void on_unhandled_exception(std::exception_ptr _exception) noexcept;

	// 3) true while a CGDK coroutine runs on this thread
	//    (used by functions which run deferred to be awaited in a coroutine. ex. execute_query() of query::rdb::odbc)
	[[nodiscard]] bool is_in_coroutine() noexcept;

	// 4) the number of the coroutines started by task::start() and not destroyed yet
	[[nodiscard]] std::size_t count_detached() noexcept;

	// 5) waits until all the coroutines started by task::start() are finished and destroyed
	//    - call it before stopping the queriers or exiting the program. (blocks the thread)
	void wait_detached();
}
}
