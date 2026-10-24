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
// system_executor::post() / Iexecutor::post()  (with a coroutine)
//
//  1) starts a coroutine::task from a normal function (or a coroutine) without
//     waiting for it. (task::start() with an executor, an owner and a completion)
//
//     system_executor::post() posts the task to the system_executor.
//
//        system_executor::post(foo());                                    // system_executor
//        system_executor::post(this, process_login());                    // + owner (kept alive until it finishes)
//        system_executor::post(foo(), [](std::exception_ptr _e) { ... }); // + completion
//        system_executor::post(this, process_login(), [](std::exception_ptr _e) { ... });
//
//     the member function of an executor (Iexecutor) posts the task to the executor itself.
//
//        pstrand->post(foo());                                            // bound to the strand
//        pstrand->post(this, process_login());                            // + owner
//        pstrand->post(foo(), [](std::exception_ptr _e) { ... });         // + completion
//        pstrand->post(this, process_login(), [](std::exception_ptr _e) { ... });
//
//     (declared in executor_system.h and Iexecutor.h. implemented by
//      coroutine::detail::post_impl_auto / execute_impl_auto / post_impl)
//
//     system_executor::execute() takes the same arguments but runs the task
//     right here on the calling thread until it suspends for the first time.
//     from then on it continues on the system_executor. a task that never
//     suspends is finished when execute() returns. (task::execute())
//     inside a coroutine it is posted like post(). (no nesting)
//
//  2) the argument is a coroutine::task<T> (an rvalue: it is moved) or a
//     callable without parameters.
//     - a task                         : posted as it is.
//     - a callable returning a task    : called here and the task is posted.
//                                        post([this] { return process_login(); });
//     - a callable returning T or void : wrapped in a coroutine which calls it
//                                        on the executor. (the result is discarded.
//                                        an exception goes to the completion or is logged)
//                                        post([this] { save_cache(); });
//       the callable is copied (or moved) into the coroutine frame, so a lambda
//       with captures is safe. (it is a parameter of the wrapper coroutine)
//     an executable, a duration or a time point is not a callable: the other
//     overloads of post() / execute() take them as before.
//  3) the task is bound to the executor.
//     on the system_executor, the coroutine may be resumed on any thread of it.
//     (protect the shared data with a lock or an atomic)
//     it starts on the executor (posted) and every resume after 'co_await' is
//     on the executor. bound to an executor::strand, the coroutines of the
//     strand never run at the same time.
//     if the executor is stopped, post() throws eRESULT::NOT_READY and nothing
//     of the task runs. a task waiting to be resumed on a stopped executor is
//     cancelled (its frames are destroyed), never run inline on another thread.
//  4) the owner (Ireferenceable) is kept alive until the task finishes.
//     (kept in the promise of the task: no wrapper coroutine frame is made)
//  5) the completion 'void(std::exception_ptr)' is called on the executor when
//     the task finishes. (nullptr: finished without an exception)
//     without the completion, an exception escaping from the task is logged.
//     (kept in the promise of the task and called in its final_suspend: no
//     wrapper coroutine frame is made)
//  6) the frame is released when it finishes. coroutine::wait_detached() waits
//     until all of them are finished. (call it before exiting the program)
//  7) a lambda coroutine must not be posted as a temporary.
//     (the lambda object with its captures is destroyed at the end of the
//     expression while the coroutine still runs)
//
//-----------------------------------------------------------------------------
namespace CGDK::coroutine::detail
{
	// - implementation of system_executor::post() / execute() with a task
	//   the owner and the completion are kept in the promise of the task itself.
	//   (no wrapper coroutine frame: one frame allocation and one resume less for every post)
	template <class TRESULT>
	struct post_impl
	{
		// - without a completion
		static void post(const object_ptr<Iexecutor>& _pexecutor, Ireferenceable* _powner, task<TRESULT>&& _task)
		{
			if (_powner != nullptr)
				_task._set_owner(_powner);

			_task.start(_pexecutor);
		}

		// - with a completion 'void(std::exception_ptr)'
		template <class FCOMPLETE>
		static void post(const object_ptr<Iexecutor>& _pexecutor, Ireferenceable* _powner, task<TRESULT>&& _task, FCOMPLETE&& _on_complete)
		{
			if (_powner != nullptr)
				_task._set_owner(_powner);

			_task.start(_pexecutor, std::forward<FCOMPLETE>(_on_complete));
		}

		// - execute (system_executor::execute()): runs the task right here until it suspends for the first time (task::execute())
		static void execute(const object_ptr<Iexecutor>& _pexecutor, Ireferenceable* _powner, task<TRESULT>&& _task)
		{
			if (_powner != nullptr)
				_task._set_owner(_powner);

			_task.execute(_pexecutor);
		}

		// - execute with a completion 'void(std::exception_ptr)'
		template <class FCOMPLETE>
		static void execute(const object_ptr<Iexecutor>& _pexecutor, Ireferenceable* _powner, task<TRESULT>&& _task, FCOMPLETE&& _on_complete)
		{
			if (_powner != nullptr)
				_task._set_owner(_powner);

			_task.execute(_pexecutor, std::forward<FCOMPLETE>(_on_complete));
		}
	};

	// - a normal function as a coroutine (post(f) / execute(f) with a callable which does not return a task)
	//   the callable is a parameter: it is copied (or moved) into the coroutine frame and outlives the call of post().
	//   (a lambda coroutine capturing it would refer to the closure object, which is destroyed when post() returns)
	//   'co_return _function()' calls return_void() of task<void> when the function returns void.
	template <class TRESULT, class TFUNCTION>
	task<TRESULT> wrap_function(TFUNCTION _function)
	{
		co_return _function();
	}

	// - the argument of post() / execute() (is_post_callable_v in Iexecutor.h)
	//   1) a coroutine::task<T> (an rvalue)   : posted as it is
	//   2) a callable returning task<T>       : called here and the task is posted
	//   3) a callable returning T or void     : wrapped in a coroutine (wrap_function) and posted
	//   '_on_complete' is none or one completion 'void(std::exception_ptr)'.
	template <class TCALLABLE>
	struct post_impl_auto
	{
		using callable_type = std::remove_cvref_t<TCALLABLE>;

		template <class... TCOMPLETE>
		static void post(const object_ptr<Iexecutor>& _pexecutor, Ireferenceable* _powner, TCALLABLE&& _callable, TCOMPLETE&&... _on_complete)
		{
			// 1) a task
			if constexpr (is_task_v<callable_type>)
			{
				static_assert(!std::is_lvalue_reference_v<TCALLABLE>, "post(task): the task must be an rvalue. it is moved into the executor. (use std::move)");
				post_impl<typename callable_type::result_type>::post(_pexecutor, _powner, std::move(_callable), std::forward<TCOMPLETE>(_on_complete)...);
			}
			else
			{
				using invoke_result = std::remove_cvref_t<std::invoke_result_t<callable_type&>>;

				// 2) a callable returning a task
				if constexpr (is_task_v<invoke_result>)
					post_impl<typename invoke_result::result_type>::post(_pexecutor, _powner, _callable(), std::forward<TCOMPLETE>(_on_complete)...);
				// 3) a normal function
				else
					post_impl<invoke_result>::post(_pexecutor, _powner, wrap_function<invoke_result>(std::forward<TCALLABLE>(_callable)), std::forward<TCOMPLETE>(_on_complete)...);
			}
		}
	};

	template <class TCALLABLE>
	struct execute_impl_auto
	{
		using callable_type = std::remove_cvref_t<TCALLABLE>;

		template <class... TCOMPLETE>
		static void execute(const object_ptr<Iexecutor>& _pexecutor, Ireferenceable* _powner, TCALLABLE&& _callable, TCOMPLETE&&... _on_complete)
		{
			// 1) a task
			if constexpr (is_task_v<callable_type>)
			{
				static_assert(!std::is_lvalue_reference_v<TCALLABLE>, "execute(task): the task must be an rvalue. it is moved into the executor. (use std::move)");
				post_impl<typename callable_type::result_type>::execute(_pexecutor, _powner, std::move(_callable), std::forward<TCOMPLETE>(_on_complete)...);
			}
			else
			{
				using invoke_result = std::remove_cvref_t<std::invoke_result_t<callable_type&>>;

				// 2) a callable returning a task
				if constexpr (is_task_v<invoke_result>)
					post_impl<typename invoke_result::result_type>::execute(_pexecutor, _powner, _callable(), std::forward<TCOMPLETE>(_on_complete)...);
				// 3) a normal function (runs right here like a task that never suspends. finished when execute() returns)
				else
					post_impl<invoke_result>::execute(_pexecutor, _powner, wrap_function<invoke_result>(std::forward<TCALLABLE>(_callable)), std::forward<TCOMPLETE>(_on_complete)...);
			}
		}
	};
}
