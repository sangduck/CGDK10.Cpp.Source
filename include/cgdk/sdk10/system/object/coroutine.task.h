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

//-----------------------------------------------------------------------------
//
// coroutine::task<TRESULT>
//
//  1) return type of a CGDK coroutine.
//
//        coroutine::task<int> foo()
//        {
//            int value = co_await bar();
//            co_return value + 10;
//        }
//
//  2) lazy start: the body does not run until the task is
//     - awaited by another coroutine         : co_await foo();
//     - started and waited by a normal func  : foo().get();
//     - started without waiting              : foo().start();
//     - run right here until it suspends     : foo().execute();
//  3) the awaiting coroutine (or get()) gets the result or the exception of the task.
//  4) a task is move-only and owns the coroutine frame.
//
//  get())
//     starts the task on the system_executor and blocks the thread until it
//     finishes. returns the result or rethrows the exception. for main() and
//     normal functions. don't call it in a coroutine (use 'co_await').
//
//  start())
//     starts the task on the system_executor and returns at once. the frame is
//     released when it finishes. an exception escaping from it is logged.
//     the task object becomes empty.
//     coroutine::wait_detached() waits until all of them are finished.
//     (call it before stopping the queriers or exiting the program)
//     * a lambda coroutine must not be started with start() as a temporary.
//       (the lambda object with its captures is destroyed at the end of the
//       expression while the coroutine still runs)
//
//  execute())
//     runs the task right here on the calling thread until it suspends for the
//     first time. from then on it continues on the system_executor. a task that
//     never suspends is finished when execute() returns. otherwise like start().
//     it is posted (start()) instead when the executor is a strand or when a
//     coroutine is already running on this thread. (no nesting)
//
//  symmetric transfer)
//     a child task starts and finishes by symmetric transfer (no post, no stack
//     growth with a compiler which tail-calls it : msvc, clang, gcc -O1 or more).
//     gcc -O0 nests the frames instead. so when the stack gets deep (256KB) inside
//     a resume, the awaiting coroutine is resumed posted, which unwinds the stack.
//     (a long loop of 'co_await task' is safe in a debug build too)
//
//  stopped executor)
//     a coroutine is never run inline on a wrong thread. if the executor refuses
//     to start it, start() (and post()) throws eRESULT::NOT_READY and get()
//     rethrows it. a coroutine waiting to be resumed on a stopped executor is
//     cancelled: the rest of its body never runs, its frames are destroyed
//     (the locals are cleaned up) and a thread waiting in get() is woken up.
//
//-----------------------------------------------------------------------------
namespace CGDK::coroutine
{
	namespace detail
	{
		template <class TRESULT>
		class task_result
		{
		public:
			template <class TVALUE>
				void				return_value(TVALUE&& _value) noexcept(std::is_nothrow_constructible_v<TRESULT, TVALUE&&>) { this->m_value.emplace(std::forward<TVALUE>(_value)); }
				TRESULT				take_result() { return std::move(*this->m_value); }
		protected:
				std::optional<TRESULT> m_value;
		};

		template <>
		class task_result<void>
		{
		public:
				void				return_void() noexcept {}
				void				take_result() noexcept {}
		};
	}

	template <class TRESULT>
	class [[nodiscard]] task
	{
	public:
		class promise_type;
		using handle_type = std::coroutine_handle<promise_type>;
		using result_type = TRESULT;

		// final awaiter)
		class final_awaiter
		{
		public:
			[[nodiscard]] bool		await_ready() const noexcept { return false; }
			[[nodiscard]] std::coroutine_handle<> await_suspend(handle_type _handle) noexcept
			{
				auto& promise = _handle.promise();

				// 1) awaited by a coroutine
				if (promise.m_continuation)
				{
					// - same executor: continue the awaiting coroutine directly (symmetric transfer)
					if (promise.m_ppromise_continuation == nullptr || promise.m_pexecutor_continuation == promise.executor().get())
					{
						// - the stack is deep (a compiler which does not tail-call the symmetric transfer : gcc -O0 nests the frames of
						//   a long chain of 'co_await task') : resume the awaiting coroutine posted instead, which unwinds the stack
						if (promise.m_ppromise_continuation != nullptr && promise_base::_is_stack_deep())
						{
							(void)promise.m_ppromise_continuation->_resume_on(promise.m_pexecutor_continuation);
							return std::noop_coroutine();
						}

						return promise.m_continuation;
					}

					// - the task switched executor: resume the awaiting coroutine on its own executor
					//   (nothing of this frame is touched after it is posted. refused: the awaiting coroutine is cancelled, never run inline)
					(void)promise.m_ppromise_continuation->_resume_on(promise.m_pexecutor_continuation);

					return std::noop_coroutine();
				}

				// 2) started by start(): call the completion (or log the exception) and release the frame (the owner reference)
				if (promise.m_is_detached)
				{
					if (promise.m_fn_complete)
					{
						// - the completion is moved out and called before the frame is released.
						//   (an exception escaping from the completion is a bug. it must not throw)
						auto fn_complete = std::move(promise.m_fn_complete);
						try { fn_complete(promise.m_exception); } catch (...) { CGASSERT_ERROR(false); }
					}
					else if (promise.m_exception)
						on_unhandled_exception(promise.m_exception);

					promise._release_frame_deferred();

					return std::noop_coroutine();
				}

				// 3) waited by get(): wake up the waiting thread
				//    (the waiting thread may destroy the task as soon as it wakes up. so the frame is held until this resume finishes)
				promise._add_ref_frame();
				promise.m_is_done.store(true, std::memory_order_release);
				promise.m_is_done.notify_all();
				promise._release_frame_deferred();

				return std::noop_coroutine();
			}
				void				await_resume() const noexcept {}
		};

		class promise_type : public promise_base, public detail::task_result<TRESULT>
		{
		public:
			[[nodiscard]] task		get_return_object() noexcept { auto handle = handle_type::from_promise(*this); this->_set_handle(handle); return task(handle); }
			std::suspend_always initial_suspend() const noexcept { return {}; }
			final_awaiter		final_suspend() const noexcept { return {}; }
				void				unhandled_exception() noexcept { this->m_exception = std::current_exception(); }

			// - the executor refused to resume this coroutine (stopped): cancel it. (promise_base::_resume_on)
				void				_on_resume_refused() noexcept override
			{
				// 1) started by start(): release the frame (the owner reference). the locals and the awaited tasks are destroyed with it
				if (this->m_is_detached)
				{
					this->_release_frame_deferred();
					return;
				}

				// 2) awaited by a coroutine: that coroutine can not continue either. cancel it (this frame is destroyed with its task object)
				if (this->m_ppromise_continuation != nullptr)
				{
					this->m_ppromise_continuation->_on_resume_refused();
					return;
				}

				// 3) waited by get(): wake up the waiting thread with an exception (await_resume() rethrows it)
				//    (the waiting thread may destroy the task as soon as it wakes up. so the frame is held until this resume finishes)
				this->m_exception = std::make_exception_ptr(throwable::failure(eRESULT::NOT_READY, 0, "the executor refused to resume the coroutine (stopped)"sv));
				this->_add_ref_frame();
				this->m_is_done.store(true, std::memory_order_release);
				this->m_is_done.notify_all();
				this->_release_frame_deferred();
			}

		public:
				std::coroutine_handle<> m_continuation;
				promise_base*		m_ppromise_continuation = nullptr;
				Iexecutor*			m_pexecutor_continuation = nullptr;	// the executor of the awaiting coroutine. not held: its frame (which holds it) is alive while this task runs
				object_ptr<Ireferenceable> m_powner;					// kept alive until this frame is destroyed. (post(owner, task). no wrapper coroutine frame)
				std::exception_ptr	m_exception;
				std::function<void(std::exception_ptr)> m_fn_complete;	// called when a detached task finishes (start() with a completion)
				bool				m_is_detached = false;
				std::atomic<bool>	m_is_done { false };
		};

	public:
			task() noexcept = default;
			task(task&& _move) noexcept : m_handle(std::exchange(_move.m_handle, nullptr)) {}
			task(const task&) = delete;
			~task() noexcept { if (this->m_handle) this->m_handle.promise()._release_frame(); }

			task&				operator = (task&& _move) noexcept { if (this != &_move) { if (this->m_handle) this->m_handle.promise()._release_frame(); this->m_handle = std::exchange(_move.m_handle, nullptr); } return *this; }
			task&				operator = (const task&) = delete;

	public:
		[[nodiscard]] bool		valid() const noexcept { return static_cast<bool>(this->m_handle); }
		[[nodiscard]] bool		done() const noexcept { return this->m_handle && this->m_handle.done(); }

		// internal) keeps '_powner' alive until the task finishes. (system_executor::post(owner, task). kept in the promise: no wrapper coroutine frame)
			void				_set_owner(Ireferenceable* _powner) noexcept { CGASSERT_ERROR(this->m_handle); this->m_handle.promise().m_powner = object_ptr<Ireferenceable>(_powner); }

		// get) starts the task on the system_executor and waits for the result (blocks the thread)
			TRESULT				get()
		{
			// check)
			CGASSERT_ERROR(this->m_handle);

			auto& promise = this->m_handle.promise();

			// 1) start on the system_executor (refused: the coroutine is cancelled and await_resume() throws)
			promise.executor(default_executor());
			(void)promise._resume_on(promise.executor().get());

			// 2) wait until it finishes
			promise.m_is_done.wait(false, std::memory_order_acquire);

			// return) the result or the exception
			return this->await_resume();
		}

		// start) starts the task on the system_executor without waiting (the frame is released when it finishes)
			void				start() { this->start(object_ptr<Iexecutor>()); }

		// start) starts the task on the system_executor and calls '_on_complete(exception)' when it finishes.
		//        - no wrapper coroutine: the completion is stored in the promise and called in final_suspend.
		//          (the frame of this task is the only coroutine frame)
		//        - 'exception' is nullptr when the task finished without an exception.
		//        - the completion must not throw. captured objects are kept until the completion returns.
			template <class FCOMPLETE>
			std::enable_if_t<std::is_invocable_v<std::decay_t<FCOMPLETE>&, std::exception_ptr>, void>
								start(FCOMPLETE&& _on_complete) { this->start(object_ptr<Iexecutor>(), std::forward<FCOMPLETE>(_on_complete)); }

		// start) starts the task on '_pexecutor' and calls '_on_complete(exception)' when it finishes (empty: the system_executor)
			template <class FCOMPLETE>
			std::enable_if_t<std::is_invocable_v<std::decay_t<FCOMPLETE>&, std::exception_ptr>, void>
								start(const object_ptr<Iexecutor>& _pexecutor, FCOMPLETE&& _on_complete)
			{
				// check)
				CGASSERT_ERROR(this->m_handle);

				// 1) set the completion (called in final_suspend of the detached task)
				this->m_handle.promise().m_fn_complete = std::forward<FCOMPLETE>(_on_complete);

				// 2) start
				this->start(_pexecutor);
			}

		// start) starts the task on '_pexecutor' without waiting (empty: the system_executor. see also system_executor::post())
			void				start(const object_ptr<Iexecutor>& _pexecutor)
		{
			// check)
			CGASSERT_ERROR(this->m_handle);

			// 1) the coroutine owns its frame from now on
			auto handle = std::exchange(this->m_handle, nullptr);
			auto& promise = handle.promise();

			promise.m_is_detached = true;
			promise._mark_detached();

			// 2) start on the executor (nothing of the frame is touched after it is posted)
			//    - refused (the executor is stopped): the frame is already released (cancelled). nothing runs. throws
			auto pexecutor = _pexecutor.exist() ? _pexecutor : default_executor();
			promise.executor(pexecutor);

			if (promise._resume_on(pexecutor.get()) == false)
				throw throwable::failure(eRESULT::NOT_READY, 0, "the executor refused to start the coroutine (stopped)"sv);
		}

		// execute) runs the task right here on this thread until it suspends for the first time. then it continues on the system_executor
			void				execute() { this->execute(object_ptr<Iexecutor>()); }

		// execute) runs the task right here until it suspends for the first time. then it continues on '_pexecutor' (empty: the system_executor)
		//          - the frame is released when it finishes. the task object becomes empty. (like start())
		//          - a task that never suspends is finished when execute() returns.
		//          - it is posted instead (start()) when '_pexecutor' is not the system_executor (a strand must not run its coroutines
		//            on a foreign thread) or when a coroutine is already running on this thread. (no nesting: no recursion)
			void				execute(const object_ptr<Iexecutor>& _pexecutor)
		{
			// check)
			CGASSERT_ERROR(this->m_handle);

			// 1) not the system_executor, or inside a coroutine: post it
			auto pexecutor = _pexecutor.exist() ? _pexecutor : default_executor();

			if (pexecutor.get() != static_cast<Iexecutor*>(system_executor::executor_ptr_peek()) || is_in_coroutine())
			{
				this->start(pexecutor);
				return;
			}

			// 2) the coroutine owns its frame from now on
			auto handle = std::exchange(this->m_handle, nullptr);
			auto& promise = handle.promise();

			promise.m_is_detached = true;
			promise._mark_detached();
			promise.executor(pexecutor);

			// 3) run it here until it suspends for the first time (nothing of the frame is touched after it)
			promise._resume_here();
		}

		// execute) runs the task right here until it suspends for the first time (see above) and calls '_on_complete(exception)' when it finishes.
		//          - the completion is stored in the promise and called in final_suspend. (no wrapper coroutine frame)
		//            a task that never suspends calls it before execute() returns.
			template <class FCOMPLETE>
			std::enable_if_t<std::is_invocable_v<std::decay_t<FCOMPLETE>&, std::exception_ptr>, void>
								execute(const object_ptr<Iexecutor>& _pexecutor, FCOMPLETE&& _on_complete)
		{
			// check)
			CGASSERT_ERROR(this->m_handle);

			// 1) set the completion (called in final_suspend of the detached task)
			this->m_handle.promise().m_fn_complete = std::forward<FCOMPLETE>(_on_complete);

			// 2) execute
			this->execute(_pexecutor);
		}

		// awaiter) 'co_await task'
		[[nodiscard]] bool		await_ready() const noexcept { return !this->m_handle || this->m_handle.done(); }

			template <class TPROMISE>
		[[nodiscard]] std::coroutine_handle<> await_suspend(std::coroutine_handle<TPROMISE> _caller) noexcept
		{
			auto& promise = this->m_handle.promise();

			// 1) set continuation
			promise.m_continuation = _caller;

			// 2) inherit the executor of the awaiting coroutine
			if constexpr (is_cgdk_promise_v<TPROMISE>)
			{
				promise.m_ppromise_continuation = &_caller.promise();
				promise.m_pexecutor_continuation = _caller.promise().executor().get();

				if (promise.executor().empty())
					promise.executor(_caller.promise().executor());
			}
			else
			{
				// - awaited from a foreign coroutine: the caller continues where the task finishes
				promise.m_pexecutor_continuation = promise.executor().get();
			}

			// 3) start the task on this thread (it already runs on the right executor)
			return this->m_handle;
		}
			TRESULT				await_resume()
		{
			// check)
			CGASSERT_ERROR(this->m_handle);

			auto& promise = this->m_handle.promise();

			// check) rethrow the exception of the task
			if (promise.m_exception)
				std::rethrow_exception(promise.m_exception);

			// return)
			return promise.take_result();
		}

	private:
		explicit task(handle_type _handle) noexcept : m_handle(_handle) {}

			handle_type			m_handle;
	};
}
