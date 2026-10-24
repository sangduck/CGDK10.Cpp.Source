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

namespace CGDK::coroutine
{
	//-----------------------------------------------------------------------------
	// coroutine::async_result<TRESULT>
	//  - an awaitable which bridges a callback api into a coroutine.
	//
	//  1) copyable. the copies share the result. capture it by value in the callback.
	//  2) set_result() may be called before or after 'co_await'. (thread-safe)
	//  3) the awaiting coroutine is resumed on its bound executor. (promise_base)
//  4) async_result<void> has no value. set_result() only wakes the awaiting coroutine.
//  5) set_exception(ep) delivers an exception instead of a value: 'co_await' rethrows it
//     in the awaiting coroutine. (set exactly one of set_result / set_exception)
	//
	//     coroutine::async_result<eRESULT> result_query;
	//     pstatements->f_complete = [result_query](eRESULT _result) { result_query.set_result(_result); };
	//     pquerier->post_query(std::move(pstatements));
	//
	//     auto result = co_await result_query;
	//
	//-----------------------------------------------------------------------------
	template <class TRESULT>
	class [[nodiscard]] async_result
	{
	private:
		struct state_t
		{
			lock_object			m_lock;
			std::optional<TRESULT> m_result;
			std::exception_ptr	m_exception;
			promise_base*		m_ppromise = nullptr;
		};

	public:
			async_result() : m_pstate(std::make_shared<state_t>()) {}

			void				set_result(TRESULT _result) const
			{
				// declare)
				promise_base* ppromise = nullptr;

				SCOPED_LOCK(this->m_pstate->m_lock)
				{
					// check) the result must be set only once
					CGASSERT_ERROR(this->m_pstate->m_result.has_value() == false);

					// 1) set the result
					this->m_pstate->m_result.emplace(std::move(_result));

					// 2) take the awaiting coroutine
					ppromise = std::exchange(this->m_pstate->m_ppromise, nullptr);
				}

				// 3) resume the awaiting coroutine (on its bound executor)
				if (ppromise != nullptr)
					ppromise->_resume_here_or_on_executor();
			}

			void				set_exception(std::exception_ptr _exception) const
			{
				// declare)
				promise_base* ppromise = nullptr;

				SCOPED_LOCK(this->m_pstate->m_lock)
				{
					// check) the result must be set only once
					CGASSERT_ERROR(this->m_pstate->m_result.has_value() == false && this->m_pstate->m_exception == nullptr);

					// 1) set the exception (rethrown by await_resume)
					this->m_pstate->m_exception = std::move(_exception);

					// 2) take the awaiting coroutine
					ppromise = std::exchange(this->m_pstate->m_ppromise, nullptr);
				}

				// 3) resume the awaiting coroutine (on its bound executor)
				if (ppromise != nullptr)
					ppromise->_resume_here_or_on_executor();
			}

		[[nodiscard]] bool		await_ready() const noexcept { SCOPED_LOCK(this->m_pstate->m_lock) { return this->m_pstate->m_result.has_value() || this->m_pstate->m_exception != nullptr; } }
			template <class TPROMISE>
		[[nodiscard]] bool		await_suspend(std::coroutine_handle<TPROMISE> _handle) noexcept
			{
				static_assert(is_cgdk_promise_v<TPROMISE>, "coroutine::async_result can be awaited only in CGDK coroutines");

				SCOPED_LOCK(this->m_pstate->m_lock)
				{
					// check) the result is already set: continue without suspending
					if (this->m_pstate->m_result.has_value() || this->m_pstate->m_exception != nullptr)
						return false;

					// 1) keep the awaiting coroutine (resumed by set_result())
					this->m_pstate->m_ppromise = &_handle.promise();

					// return) suspend
					return true;
				}
			}
			TRESULT				await_resume()
			{
				SCOPED_LOCK(this->m_pstate->m_lock)
				{
					// check) an exception was delivered: rethrow it in the awaiting coroutine
					if (this->m_pstate->m_exception != nullptr)
						std::rethrow_exception(this->m_pstate->m_exception);

					return std::move(*this->m_pstate->m_result);
				}
			}

	private:
			std::shared_ptr<state_t> m_pstate;
	};

	//-----------------------------------------------------------------------------
	// coroutine::async_result<void>
	//  - the same without a value. set_result() only wakes the awaiting coroutine.
	//
	//     coroutine::async_result<void> result_done;
	//     request.on_complete = [result_done]() { result_done.set_result(); };
	//
	//     co_await result_done;
	//
	//-----------------------------------------------------------------------------
	template <>
	class [[nodiscard]] async_result<void>
	{
	private:
		struct state_t
		{
			lock_object			m_lock;
			bool				m_is_set = false;
			std::exception_ptr	m_exception;
			promise_base*		m_ppromise = nullptr;
		};

	public:
			async_result() : m_pstate(std::make_shared<state_t>()) {}

			void				set_result() const
			{
				// declare)
				promise_base* ppromise = nullptr;

				SCOPED_LOCK(this->m_pstate->m_lock)
				{
					// check) the result must be set only once
					CGASSERT_ERROR(this->m_pstate->m_is_set == false);

					// 1) set
					this->m_pstate->m_is_set = true;

					// 2) take the awaiting coroutine
					ppromise = std::exchange(this->m_pstate->m_ppromise, nullptr);
				}

				// 3) resume the awaiting coroutine (on its bound executor)
				if (ppromise != nullptr)
					ppromise->_resume_here_or_on_executor();
			}

			void				set_exception(std::exception_ptr _exception) const
			{
				// declare)
				promise_base* ppromise = nullptr;

				SCOPED_LOCK(this->m_pstate->m_lock)
				{
					// check) the result must be set only once
					CGASSERT_ERROR(this->m_pstate->m_is_set == false && this->m_pstate->m_exception == nullptr);

					// 1) set the exception (rethrown by await_resume)
					this->m_pstate->m_exception = std::move(_exception);

					// 2) take the awaiting coroutine
					ppromise = std::exchange(this->m_pstate->m_ppromise, nullptr);
				}

				// 3) resume the awaiting coroutine (on its bound executor)
				if (ppromise != nullptr)
					ppromise->_resume_here_or_on_executor();
			}

		[[nodiscard]] bool		await_ready() const noexcept { SCOPED_LOCK(this->m_pstate->m_lock) { return this->m_pstate->m_is_set || this->m_pstate->m_exception != nullptr; } }
			template <class TPROMISE>
		[[nodiscard]] bool		await_suspend(std::coroutine_handle<TPROMISE> _handle) noexcept
			{
				static_assert(is_cgdk_promise_v<TPROMISE>, "coroutine::async_result can be awaited only in CGDK coroutines");

				SCOPED_LOCK(this->m_pstate->m_lock)
				{
					// check) already set: continue without suspending
					if (this->m_pstate->m_is_set || this->m_pstate->m_exception != nullptr)
						return false;

					// 1) keep the awaiting coroutine (resumed by set_result())
					this->m_pstate->m_ppromise = &_handle.promise();

					// return) suspend
					return true;
				}
			}
			void				await_resume() const
			{
				SCOPED_LOCK(this->m_pstate->m_lock)
				{
					// check) an exception was delivered: rethrow it in the awaiting coroutine
					if (this->m_pstate->m_exception != nullptr)
						std::rethrow_exception(this->m_pstate->m_exception);
				}
			}

	private:
			std::shared_ptr<state_t> m_pstate;
	};
}
