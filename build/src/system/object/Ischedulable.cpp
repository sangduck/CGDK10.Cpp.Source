#include "pch.h"

namespace CGDK
{

Ischedulable::Ischedulable() noexcept
{
	this->m_statistics_schedulable.id = reinterpret_cast<uint64_t>(this);
}

void Nschedulable::execute_interval(chrono::tick::duration _tick_interval)
{
	// 1) set interval and next tick
	this->m_tick_diff_execute = _tick_interval;
	this->m_tick_next = chrono::tick::now() + this->m_tick_diff_execute;

	// 2) reschedule if registered (apply the new interval immediately)
	auto pschedulable_manager = this->m_pschedulable_manager;

	if (pschedulable_manager.exist())
		pschedulable_manager->reschedule(this);
}

void Nschedulable::process_advance_next_tick() noexcept
{
	// check) 실행하지 않는 상태 (unregister되면 max) : 더하면 overflow(정의되지 않은 동작)가 된다. 그대로 둔다.
	//        (process_schedule의 시각 확인 후 다른 thread가 unregister할 수 있다.)
	if (this->m_tick_next == (chrono::tick::max)() || (this->m_tick_diff_execute > chrono::tick::duration::zero() && this->m_tick_next > (chrono::tick::max)() - this->m_tick_diff_execute))
	{
		this->m_tick_next = (chrono::tick::max)();
		return;
	}

	// 1) next tick (fixed interval from the previous 'next tick')
	auto tick_next = this->m_tick_next + this->m_tick_diff_execute;

	// 2) SKIP : skip the missed executions (keep the interval grid)
	//    - CATCH_UP : the 'next tick' in the past is executed immediately (execution count is guaranteed)
	if (this->m_overrun_policy == eSCHEDULE_OVERRUN::SKIP && this->m_tick_diff_execute > chrono::tick::duration::zero())
	{
		auto tick_now = chrono::tick::now();

		if (tick_next <= tick_now)
		{
			auto count_missed = (tick_now - tick_next) / this->m_tick_diff_execute + 1;

			tick_next += this->m_tick_diff_execute * count_missed;
		}
	}

	// 3) set
	this->m_tick_next = tick_next;
}

}