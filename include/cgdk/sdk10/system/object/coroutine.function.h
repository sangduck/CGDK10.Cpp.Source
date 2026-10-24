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
// co_await function
//
//  1) 'co_await f' with a callable without parameters runs f() on the
//     system_executor (system_executor::post) and resumes the coroutine
//     when f() returns. the result of 'co_await f' is the return value of f().
//
//        coroutine::task<void> foo()
//        {
//            // - runs on a thread of the system_executor
//            int value = co_await []{ return heavy_calculation(); };
//
//            // - a function (pointer)
//            co_await load_table;
//
//            // - a function with parameters: bind them with a lambda
//            auto data = co_await [=]{ return read_file(path); };
//        }
//
//  2) after f() returns, the coroutine is resumed
//     - directly on that thread when it is bound to the system_executor.
//     - on its own executor (posted) when it is bound to another executor.
//       so the rule 'always resumed on the bound executor' is kept.
//  3) an exception thrown by f() is rethrown from 'co_await f'.
//  4) the callable is moved (or copied) into the awaiter and kept until f()
//     returns.
//  5) no allocation. the executable of the coroutine (promise_base) is posted.
//  6) not applied to
//     - awaitables (await_ready() or operator co_await) - awaited as they are
//     - callables returning coroutine::task<T> - use 'co_await f()'
//  7) if the system_executor refuses the post (stopped), f() runs inline and
//     the coroutine continues without suspending.
//
//  * implemented by promise_base::await_transform() (definitions.coroutine.h)
//
//-----------------------------------------------------------------------------
namespace CGDK::coroutine
{
	template <class TFUNCTION>
	class function_awaiter
	{
	public:
		using result_type = std::invoke_result_t<TFUNCTION&>;
		using value_type = std::conditional_t<std::is_void_v<result_type>, char, std::remove_cvref_t<result_type>>;

	public:
		explicit function_awaiter(TFUNCTION _function) noexcept(std::is_nothrow_move_constructible_v<TFUNCTION>) : m_function(std::move(_function)) {}

		[[nodiscard]] bool		await_ready() const noexcept { return false; }
			template <class TPROMISE>
		[[nodiscard]] bool		await_suspend(std::coroutine_handle<TPROMISE> _handle) noexcept
		{
			static_assert(is_cgdk_promise_v<TPROMISE>, "co_await function can be used only in CGDK coroutines");

			// 1) the coroutine to resume
			this->m_ppromise = &_handle.promise();

			// 2) run the function on the system_executor with the executable of the coroutine
			//    (posted: this awaiter must not be touched any more. it may be resumed already)
			if (this->m_ppromise->_post_to_system(&function_awaiter::_execute, this))
				return true;

			// 3) refused (the system_executor is stopped): run here and continue without suspending
			this->m_ppromise = nullptr;
			this->_invoke();

			// return)
			return false;
		}
			result_type			await_resume()
		{
			// check) rethrow the exception of the function
			if (this->m_exception)
				std::rethrow_exception(this->m_exception);

			// return)
			if constexpr (!std::is_void_v<result_type>)
				return std::move(*this->m_result);
		}

	private:
		// - runs on a thread of the system_executor
		static	void				_execute(void* _pcontext) noexcept
		{
			auto pawaiter = static_cast<function_awaiter*>(_pcontext);

			// 1) run the function
			pawaiter->_invoke();

			// 2) resume the coroutine (this awaiter may be destroyed by it. nothing is touched after it)
			pawaiter->m_ppromise->_resume_here_or_on_executor();
		}

			void				_invoke() noexcept
		{
			try
			{
				if constexpr (std::is_void_v<result_type>)
					this->m_function();
				else
					this->m_result.emplace(this->m_function());
			}
			catch (...)
			{
				this->m_exception = std::current_exception();
			}
		}

	private:
			TFUNCTION			m_function;
			std::optional<value_type> m_result;
			std::exception_ptr	m_exception;
			promise_base*		m_ppromise = nullptr;
	};
}
