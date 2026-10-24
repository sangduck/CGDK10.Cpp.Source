#include "pch.h"

using namespace CGDK;


//-----------------------------------------------------------------------------
//
// schedulable::manager
//
//  설명)
//  등록된 schedulable 객체를 관리(등록/등록해제/정보)만 한다.
//  실행은 별도의 스레드 없이 executor::scheduler(timed single execution)에
//  'next_tick' 시각으로 바로 걸어서 처리한다.
//
//   [등록] ──> scheduler.post_at(next_tick)
//                 └─> (시각 도달) ──> [schedulable의 executor] ──> process_schedule()
//                                                                  └─> 다음 next_tick으로 다시 post_at
//
//  - schedulable마다 실행 객체(executable_schedulable)는 하나이며 등록되어 있는 동안 재사용된다.
//  - 등록 해제하면 scheduler에 걸려 있는 실행을 취소한다.
//  - 실행 간격이 바뀌면(Nschedulable::execute_interval) reschedule()로 바로 다시 건다.
//
//-----------------------------------------------------------------------------
namespace
{
	constexpr uint8_t SCHEDULE_STATE_IDLE		 = 0;	// scheduler에 걸려 있지 않음
	constexpr uint8_t SCHEDULE_STATE_PENDING	 = 1;	// scheduler에 걸려 있음 (취소 가능)
	constexpr uint8_t SCHEDULE_STATE_RUNNING	 = 2;	// 실행 중 (실행 후 다시 걸린다)
}

class schedulable::manager::executable_schedulable :
	virtual public CGDK::Iexecutable,
	public factory::_traits_system
{
public:
			object_ptr<Ischedulable> pschedulable;
			object_ptr<schedulable::manager> pmanager;
			bool				is_dispatched = false;

protected:
	virtual	intptr_t			process_execute(intptr_t _return, size_t _param) override;
};

intptr_t schedulable::manager::executable_schedulable::process_execute(intptr_t /*_return*/, size_t /*_param*/)
{
	// declare)
	auto pmanager_hold = this->pmanager;

	// check)
	if (pmanager_hold.empty())
		return 0;

	// 1) execute
	pmanager_hold->process_execute_schedulable(this);

	// return)
	return 0;
}

schedulable::manager::manager() noexcept :
	m_flag_run(0)
{
	// 1) set object type
	this->set_object_type(eOBJECT_TYPE::EXECUTOR_SCHEDULABLE);

}

schedulable::manager::~manager() noexcept
{
	// 1) stop manager
	this->schedulable::manager::stop();
}

result_code schedulable::manager::start(context& /*_context*/)
{
	// 1) set m_flag_run 'true' and get old state
	const int state_old = this->m_flag_run.exchange(1);

	// check) return if already running
	if(state_old != 0)
		return result_code(eRESULT::ALREADY_DONE);

	// return)
	return result_code(eRESULT::SUCCESS);
}

result_code schedulable::manager::stop() noexcept
{
	// 1) set m_flag_run 'false' and get old state
	const int state_old = this->m_flag_run.exchange(0);

	// check) return if not running
	if(state_old == 0)
		return result_code(eRESULT::ALREADY_DONE);

	// 2) unregister all schedulable (cancel pending executions)
	this->process_cancel_schedulable_all();

	// return)
	return result_code(eRESULT::SUCCESS);
}

void schedulable::manager::process_print_statistics() noexcept
{
	try
	{
		// declare)
		shared_buffer buf_output;
		auto tick_now = chrono::tick::now();
		uint64_t count_execute_total = 0;

		SCOPED_SHARED_LOCK(this->m_lockable_set_schedulable)
		{
			// 1) alloc shared_buffer for log text
			buf_output = alloc_shared_buffer((static_cast<size_t>(this->m_set_schedulable.size()) + 8) * 256);

			// 2) title
			buf_output.append_text("\x1b[37m" "---------------------------------------------------------------------------------------------------------------------------------\n" "\x1b[0m"sv);
			buf_output.append_text(std::format("\x1b[37m" "{:30s} {:8s}{:>10s}{:>10s}{:>11s}{:>9s}{:>13s}\n" "\x1b[0m",
				"name",
				"type",
				"interval",
				"overrun",
				"next(ms)",
				"state",
				"executed")
			);
			buf_output.append_text("\x1b[90m" "---------------------------------------------------------------------------------------------------------------------------------\n" "\x1b[0m"sv);

			// 3) registered schedulables
			for (auto& iter : this->m_set_schedulable)
			{
				// check)
				CGASSERT_ERROR(iter.exist());

				// check)
				CONTINUE_IF(iter.empty());

				// - get statistics
				const auto& statistics = iter->statistics();

				// - "1. name" (id if not nameable)
				std::string str_name;
				if (auto* pnameable = dynamic_cast<object::Inameable*>(iter.get()); pnameable != nullptr)
					str_name = std::string(pnameable->name());
				if (str_name.empty())
					str_name = std::format("{:#x}", statistics.id);
				if (str_name.size() > 30)
					str_name.resize(30);

				// - "3. interval" "4. overrun" (Nschedulable only)
				std::string str_interval = "-";
				std::string str_overrun = "-";
				if (auto* pnschedulable = dynamic_cast<Nschedulable*>(iter.get()); pnschedulable != nullptr)
				{
					str_interval = std::format("{}ms", std::chrono::duration_cast<std::chrono::milliseconds>(pnschedulable->execute_interval()).count());
					str_overrun = std::string(to_string(pnschedulable->overrun_policy()));
				}

				// - "5. next(ms)"
				auto tick_next = iter->next_tick();
				std::string str_next = (tick_next == (chrono::tick::max)()) ? std::string("-") : std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(tick_next - tick_now).count());

				// - "6. state"
				std::string_view str_state = (iter->m_schedule_state == SCHEDULE_STATE_PENDING) ? "pending"sv : (iter->m_schedule_state == SCHEDULE_STATE_RUNNING) ? "running"sv : "idle"sv;

				// - "7. executed"
				auto count_execute = statistics.count_execute.load(std::memory_order_relaxed);
				count_execute_total += count_execute;

				// - append
				buf_output.append_text(std::format("{}" "{:30s} {:8s}{:>10s}{:>10s}{:>11s}{:>9s}{:>13}" "\x1b[0m\n",
					(count_execute != 0) ? "\x1b[92m" : "\x1b[32m",
					str_name,
					to_string(statistics.type),
					str_interval,
					str_overrun,
					str_next,
					str_state,
					count_execute)
				);
			}

			// 4) total
			buf_output.append_text("\x1b[90m" "---------------------------------------------------------------------------------------------------------------------------------\n" "\x1b[0m"sv);
			buf_output.append_text(std::format("\x1b[37m" "registered: {}  executed: {} (total by manager: {})\n" "\x1b[0m", this->m_set_schedulable.size(), count_execute_total, this->m_count_execute.load(std::memory_order_relaxed)));
			buf_output.append_text("\x1b[37m" "---------------------------------------------------------------------------------------------------------------------------------\n" "\x1b[0m"sv);
		}

		// 5) output
	#if defined(_WIN32)
		DWORD number_written = 0;
		WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), buf_output.data(), buf_output.size<DWORD>(), &number_written, nullptr);
	#else
		fwrite(buf_output.data(), 1, buf_output.size<int>(), stdout);
	#endif
	}
	catch (...)
	{
	}
}

void schedulable::manager::print_statistics() noexcept
{
	system_executor::schedulable_manager()->process_print_statistics();
}

void schedulable::manager::process_cancel_schedulable_all() noexcept
{
	// declare)
	std::set<object_ptr<Ischedulable>> set_schedulable;
	std::vector<object_ptr<CGDK::Iexecutable>> vector_executable;
	std::vector<object_ptr<schedulable::manager>> vector_hold;

	// 1) detach all
	{
		// lock)
		std::lock_guard<std::mutex> lock(this->m_lockable_schedule);

		{
			// lock)
			std::lock_guard<std::shared_mutex> lock_schedulable(this->m_lockable_set_schedulable);

			// - swap
			set_schedulable.swap(this->m_set_schedulable);
		}

		// - cancel pending executions and detach
		vector_executable.reserve(set_schedulable.size());
		vector_hold.reserve(set_schedulable.size());

		for (auto& iter : set_schedulable)
		{
			this->_cancel_schedulable(iter.get());

			vector_executable.emplace_back(std::move(iter->m_pexecutable_schedule));
			vector_hold.emplace_back(std::move(iter->m_pschedulable_manager));
		}
	}

	// 2) call 'process_on_unregister' all
	for(auto& iter: set_schedulable)
	{
		iter->process_on_unregister();
	}
}

object_ptr<executor::scheduler> schedulable::manager::_get_scheduler()
{
	// 설명) m_lockable_schedule이 lock된 상태에서 호출되어야 한다.

	// 1) default scheduler
	if (this->m_pscheduler.empty())
	{
		this->m_pscheduler = system_executor::scheduler();
	}

	// return)
	return this->m_pscheduler;
}

bool schedulable::manager::_post_schedulable(Ischedulable* _pschedulable)
{
	// 설명) m_lockable_schedule이 lock된 상태에서 호출되어야 한다.

	// check)
	if (_pschedulable->m_pexecutable_schedule.empty())
		return false;

	// 1) get next tick
	auto tick_next = _pschedulable->next_tick();

	// check) not scheduled
	if (tick_next == (chrono::tick::max)())
	{
		_pschedulable->m_schedule_state = SCHEDULE_STATE_IDLE;
		return false;
	}

	// 2) get scheduler
	auto pscheduler = this->_get_scheduler();

	// check)
	if (pscheduler.empty())
	{
		_pschedulable->m_schedule_state = SCHEDULE_STATE_IDLE;
		return false;
	}

	// 3) post at 'next tick'
	_pschedulable->m_schedule_state = SCHEDULE_STATE_PENDING;
	pscheduler->post_at(_pschedulable->m_pexecutable_schedule, tick_next);

	// return)
	return true;
}

void schedulable::manager::_cancel_schedulable(Ischedulable* _pschedulable) noexcept
{
	// 설명) m_lockable_schedule이 lock된 상태에서 호출되어야 한다.

	// 1) cancel the execution pending in scheduler
	if (_pschedulable->m_schedule_state == SCHEDULE_STATE_PENDING && this->m_pscheduler.exist() && _pschedulable->m_pexecutable_schedule.exist())
	{
		this->m_pscheduler->cancel(_pschedulable->m_pexecutable_schedule.get());
	}

	// 2) set state
	_pschedulable->m_schedule_state = SCHEDULE_STATE_IDLE;
}

bool schedulable::manager::process_attach(object_ptr<Ischedulable>&& _pschedulable)
{
	// check)
	CGASSERT_ERROR(_pschedulable != nullptr);

	// check)
	if(_pschedulable == nullptr)
		return false;

	// lock)
	std::lock_guard<std::mutex> lock(this->m_lockable_schedule);

	// check) already registered
	if(_pschedulable->m_pschedulable_manager.exist())
		return false;

	// 1) attach
	_pschedulable->m_pschedulable_manager = this;

	try
	{
		// 2) on register (set 'next tick' and executor)
		auto result = _pschedulable->process_on_register();

		// check)
		if(result == false)
		{
			_pschedulable->m_pschedulable_manager.reset();
			return false;
		}

		// 3) add to set
		{
			// lock)
			std::lock_guard<std::shared_mutex> lock_schedulable(this->m_lockable_set_schedulable);

			// check)
			CGASSERT_ERROR(this->m_set_schedulable.find(_pschedulable) == this->m_set_schedulable.end());

			// - add
			this->m_set_schedulable.emplace(_pschedulable);
		}

		// 4) create execution object (reused while registered)
		auto pexecutable = make_object<executable_schedulable>();
		pexecutable->pschedulable = _pschedulable;
		pexecutable->pmanager = this;

		_pschedulable->m_pexecutable_schedule = std::move(pexecutable);
		_pschedulable->m_schedule_state = SCHEDULE_STATE_IDLE;

		// 5) post to scheduler at 'next tick'
		this->_post_schedulable(_pschedulable.get());
	}
	catch (...)
	{
		// - cancel
		this->_cancel_schedulable(_pschedulable.get());

		// - detach from set'schedulable'
		{
			// lock)
			std::lock_guard<std::shared_mutex> lock_schedulable(this->m_lockable_set_schedulable);

			this->m_set_schedulable.erase(_pschedulable);
		}

		// - reset
		_pschedulable->m_pexecutable_schedule.reset();
		_pschedulable->m_pschedulable_manager.reset();

		// reraise)
		throw;
	}

	// return)
	return true;
}

bool schedulable::manager::process_detach(Ischedulable* _pschedulable) noexcept
{
	// check)
	if(_pschedulable == nullptr)
		return false;

	// declare)
	object_ptr<schedulable::manager> phold_this;
	object_ptr<CGDK::Iexecutable> pexecutable_hold;

	{
		// lock)
		std::lock_guard<std::mutex> lock(this->m_lockable_schedule);

		// check)
		if(_pschedulable->m_pschedulable_manager != this)
			return false;

		{
			// lock)
			std::lock_guard<std::shared_mutex> lock_schedulable(this->m_lockable_set_schedulable);

			// 1) find
			auto iter_find = this->m_set_schedulable.find(_pschedulable);

			// check)
			if(iter_find == this->m_set_schedulable.end())
				return false;

			// 2) erase from set
			this->m_set_schedulable.erase(iter_find);
		}

		// 3) cancel the pending execution
		//    - running execution will not be posted again (the manager is reset below)
		this->_cancel_schedulable(_pschedulable);

		// 4) reset
		pexecutable_hold = std::move(_pschedulable->m_pexecutable_schedule);
		phold_this = std::move(_pschedulable->m_pschedulable_manager);
	}

	// 5) Hook
	_pschedulable->process_on_unregister();

	// 6) reset hold
	pexecutable_hold.reset();
	phold_this.reset();

	// return)
	return true;
}

bool schedulable::manager::reschedule(Ischedulable* _pschedulable)
{
	// check)
	if (_pschedulable == nullptr)
		return false;

	// lock)
	std::lock_guard<std::mutex> lock(this->m_lockable_schedule);

	// check) not registered
	if (_pschedulable->m_pschedulable_manager != this || _pschedulable->m_pexecutable_schedule.empty())
		return false;

	// check) running - posted again with new 'next tick' after execution
	if (_pschedulable->m_schedule_state == SCHEDULE_STATE_RUNNING)
		return true;

	// 1) cancel the pending execution
	this->_cancel_schedulable(_pschedulable);

	// 2) post again at new 'next tick'
	return this->_post_schedulable(_pschedulable);
}

void schedulable::manager::process_execute_schedulable(executable_schedulable* _pexecutable)
{
	// declare)
	object_ptr<Ischedulable> pschedulable;

	// 1) check state
	{
		// lock)
		std::lock_guard<std::mutex> lock(this->m_lockable_schedule);

		// - get schedulable
		pschedulable = _pexecutable->pschedulable;

		// check) unregistered or not this execution object
		if (pschedulable.empty() || pschedulable->m_pschedulable_manager != this || pschedulable->m_pexecutable_schedule.get() != static_cast<CGDK::Iexecutable*>(_pexecutable))
			return;

		// - running
		pschedulable->m_schedule_state = SCHEDULE_STATE_RUNNING;
	}

	// 2) dispatch to the executor of schedulable (if it is not the executor of scheduler)
	if (_pexecutable->is_dispatched == false)
	{
		auto pexecutor = pschedulable->executor();
		auto pscheduler = this->scheduler();

		if (pexecutor.exist() && pscheduler.exist() && pexecutor.get() != pscheduler->executor().get())
		{
			_pexecutable->is_dispatched = true;

			// - post to executor of schedulable (executed again on that executor)
			if (pexecutor->post(object_ptr<CGDK::Iexecutable>(_pexecutable)))
				return;
		}
	}

	// - reset
	_pexecutable->is_dispatched = false;

	// statistics)
	pschedulable->statistics_on_execute_elapsed_time(chrono::tick::duration::zero());
	pschedulable->m_statistics_schedulable.count_execute.fetch_add(1, std::memory_order_relaxed);
	this->m_count_execute.fetch_add(1, std::memory_order_relaxed);

	// 3) execute ('next tick' is updated in process_schedule())
	try
	{
		pschedulable->process_schedule();
	}
	catch (std::exception& _exception)
	{
		throwable::execute_exception_handler(this, _exception, __FUNCTION__);
	}
	catch (...)
	{
	}

	// 4) post again at 'next tick'
	{
		// lock)
		std::lock_guard<std::mutex> lock(this->m_lockable_schedule);

		// check) unregistered while executing
		if (pschedulable->m_pschedulable_manager != this || pschedulable->m_pexecutable_schedule.get() != static_cast<CGDK::Iexecutable*>(_pexecutable))
			return;

		// - post
		this->_post_schedulable(pschedulable.get());
	}
}
