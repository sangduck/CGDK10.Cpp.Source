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
// co_sleep_for() / co_sleep_until()
//
//  1) co_await co_sleep_for(100ms)       : resumes the coroutine after the duration
//     co_await co_sleep_until(time)      : resumes the coroutine at the time
//
//        coroutine::task<void> foo()
//        {
//            co_await co_sleep_for(100ms);                        // no thread is blocked
//            co_await co_sleep_until(chrono::tick::now() + 1s);
//        }
//
//  2) no thread is blocked while sleeping. the coroutine is suspended and
//     resumed on its executor by the scheduler of the system_executor.
//     (std::this_thread::sleep_for() blocks the thread)
//  3) a duration of zero or less, or a time already passed, does not suspend.
//  4) co_sleep_until() accepts chrono::tick::time_point and std::chrono::time_point
//     of any clock. a time point of another clock (ex. std::chrono::system_clock)
//     is converted with the time remaining from now, so it does not follow later
//     changes of that clock.
//  5) never wakes up earlier than the time. (durations are rounded up to ticks)
//  6) can be used only in CGDK coroutines (coroutine::task<T>)
//
//-----------------------------------------------------------------------------
namespace CGDK::coroutine
{
	class sleep_awaiter
	{
	public:
		explicit sleep_awaiter(chrono::tick::time_point _tick_resume) noexcept : m_tick_resume(_tick_resume) {}

		[[nodiscard]] bool		await_ready() const noexcept { return this->m_tick_resume <= chrono::tick::now(); }
			template <class TPROMISE>
			void				await_suspend(std::coroutine_handle<TPROMISE> _handle)
		{
			static_assert(is_cgdk_promise_v<TPROMISE>, "co_sleep_for() and co_sleep_until() can be used only in CGDK coroutines");

			// - resumed on the executor of the coroutine at the time (this awaiter may be destroyed after this call)
			_handle.promise()._resume_at(this->m_tick_resume);
		}
			void				await_resume() const noexcept {}

	private:
			chrono::tick::time_point m_tick_resume;
	};
}

namespace CGDK
{
	// 1) co_sleep_for(duration)
	template <class TREP, class TPERIOD>
	[[nodiscard]] coroutine::sleep_awaiter co_sleep_for(std::chrono::duration<TREP, TPERIOD> _duration) noexcept
	{
		return coroutine::sleep_awaiter(chrono::tick::now() + std::chrono::ceil<chrono::tick::duration>(_duration));
	}

	// 2) co_sleep_until(chrono::tick::time_point)
	[[nodiscard]] inline coroutine::sleep_awaiter co_sleep_until(chrono::tick::time_point _tick_wake) noexcept
	{
		return coroutine::sleep_awaiter(_tick_wake);
	}

	// 3) co_sleep_until(std::chrono::time_point) of any clock
	template <class TCLOCK, class TDURATION>
	[[nodiscard]] coroutine::sleep_awaiter co_sleep_until(const std::chrono::time_point<TCLOCK, TDURATION>& _time) noexcept
	{
		// - the clock of chrono::tick
		if constexpr (std::is_same_v<TCLOCK, typename chrono::tick::time_point::clock>)
			return coroutine::sleep_awaiter(chrono::tick::time_point(std::chrono::ceil<chrono::tick::duration>(_time)));
		// - another clock: the time remaining from now
		else
			return coroutine::sleep_awaiter(chrono::tick::now() + std::chrono::ceil<chrono::tick::duration>(_time - TCLOCK::now()));
	}
}
