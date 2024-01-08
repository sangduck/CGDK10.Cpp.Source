#include "pch.h"

namespace CGDK
{

#define	SUSTAIN_INTERVAL	2s
#define	THRESHOLD_DEADLOCK	2s

extern TLS	api::Nthread*	g_pthread_now;


api::manager_thread::manager_thread() noexcept :
	object::Nnameable("thread_manager"),
	m_tick_observing_last(chrono::tick::now()),
	m_tick_differ_observing(SUSTAIN_INTERVAL),
	m_tick_diff_deadlock_threshold(THRESHOLD_DEADLOCK),
	m_enable_observing_deadlock(0)
{
	// 1) Thread를 생성한다.
	auto pthread_main = make_object<api::thread>();

	// 2) Thread 이름을 설정한다.
	pthread_main->set_name("main");

	// 3) Thread를 등록한다.
	register_thread(pthread_main);

	// 4) 설정한다.
	m_pthread_default = std::move(pthread_main);
	g_pthread_now = m_pthread_default;
}

api::manager_thread::~manager_thread() noexcept
{
	// 1) 먼저 destroy
	destroy();

	// 2) Thread Now를 nullptr로 설정한다.
	g_pthread_now = nullptr;

	// 3) Thread를 등록해제한다.
	unregister_thread(m_pthread_default);

	// 4) Thread를 reset한다.
	m_pthread_default.reset();

	// check) Manager가 파괴될땐 분명 Thread가 하나도 없어야 한다.
	CGASSERT_ERROR(m_list_thread.empty());
}

result_code api::manager_thread::destroy() noexcept
{
	// 1) Pool Sustainer를 먼저 떼낸다.
	stop_deadlock_observing();

	// return)
	return result_code(eRESULT::SUCCESS);
}

bool api::manager_thread::register_thread(api::Nthread* _pthread)
{
	scoped_lock(m_lockable_list_thread)
	{
		// 1) 이미 등록되어 이는지 확인한다.
		auto iter_thread = std::find(m_list_thread.begin(), m_list_thread.end(), _pthread);

		// check) 이미 등록되어 있으면 끝낸다.
		RETURN_IF(iter_thread != m_list_thread.end(), false);

		// 2) Thread를 List에 추가된다.
		m_list_thread.push_back(_pthread);
	}

	// 3) View에 통보한다.
	transmit_message(sMESSAGE_NOTIFY(eMESSAGE::THREAD_REGISTER, 0, reinterpret_cast<uintptr_t>(_pthread)));

	// return) 
	return true;
}

bool api::manager_thread::unregister_thread(api::Nthread* _pthread)
{
	scoped_lock(m_lockable_list_thread)
	{
		// 1) Thread를 List에서 찾는다.
		auto iter_thread = std::find(m_list_thread.begin(), m_list_thread.end(), _pthread);

		// check) 
		CGASSERT_ERROR(iter_thread != m_list_thread.end());

		// check) 찾지 못했으면 그냥 끝낸다.
		RETURN_IF(iter_thread == m_list_thread.end(), false);

		// 2) Thread를 지운다.
		m_list_thread.erase(iter_thread);
	}

	// 3) View에 통보한다.
	transmit_message(sMESSAGE_NOTIFY(eMESSAGE::THREAD_UNREGISTER, 0, reinterpret_cast<uintptr_t>(_pthread)));

	// return) 
	return true;
}

void api::manager_thread::start_deadlock_observing(chrono::tick::duration _tick_deadlock_threshold)
{
	// 1) ObservingDeatlock 상태를 1로 만든다.
	int bPre = m_enable_observing_deadlock.exchange(1);

	// check) 이미 시작했다면 그냥 끝낸다.
	RETURN_IF(bPre == 1);

	// 1) DeadlockThreashold를 설정한다.
	if(_tick_deadlock_threshold != 0ms)
	{
		observing_interval(_tick_deadlock_threshold);
	}

	// 2) Schedulare에 등록한다.
	if(system_executor::register_schedulable(this) == false)
	{
		return;
	}

	// 3) 현재 Tick Count를 저장해서 초기화한다.
	m_tick_observing_last = chrono::tick::now();
	
	// trace) 
	LOG_INFO << "(info) start deadlock observer"sv;
}

void api::manager_thread::stop_deadlock_observing() noexcept
{
	// 1) ObservingDeatlock 상태를 0로 만든다.
	int bPre = m_enable_observing_deadlock.exchange(false);

	// check) 이미 끝냈다면 그냥 끝낸다.
	RETURN_IF(bPre == 0);

	// 1) Schedulare에 등록해제한다.
	if(system_executor::unregister_schedulable(this)==false)
	{
		return;
	}

	// trace) 
	LOG_INFO << "(info) stop deadlock observer"sv;
}

bool api::manager_thread::process_schedule()
{
	// 1) 현재 Tick을 구한다.
	auto tick_differ_execute = chrono::tick::now() - m_tick_observing_last;

	// check) m_tick_diffSustain마다 실행한다.
	RETURN_IF(tick_differ_execute < m_tick_differ_observing, false);

	// 2) Object Pool을 sustain한다.
	check_deadlock();

	// 3) 다음 Tick을 구한다.
	m_tick_observing_last += m_tick_differ_observing;

	// return)
	return true;
}

void api::manager_thread::check_deadlock()
{
	// 1) 현재 Tick를 얻는다.
	auto tick_now = chrono::tick::now();

	scoped_lock(m_lockable_list_thread)
	{
		for(auto& iter:m_list_thread)
		{
			if(iter->is_time_out(m_tick_diff_deadlock_threshold, tick_now))
			{
				iter->raise_exception();
			}
		}
	}
}

void api::manager_thread::initialize_instance()
{
	scoped_lock(m_s_lockable_create)
	{
		// check) 
		RETURN_IF(m_s_pinstance.exist());

		// 1_ create instance and set
		m_s_pinstance = _make_own_raw<manager_thread>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL);
	}
}

void api::manager_thread::set_instance(own_ptr<manager_thread>&& _pinstance) noexcept
{
	// declare)
	own_ptr<manager_thread> pinstance_pre;

	scoped_lock(m_s_lockable_create)
	{
		// 1) move old instance
		pinstance_pre = std::move(m_s_pinstance);

		// 2) set new instance
		m_s_pinstance = std::move(_pinstance);
	}

	// 3) reset old instance
	pinstance_pre.reset();
}

void api::manager_thread::reset_instance() noexcept
{
	// declare)
	own_ptr<manager_thread> pinstance;

	scoped_lock(m_s_lockable_create)
	{
		// 1) move instance
		pinstance = std::move(m_s_pinstance);
	}

	// 2) reset old instance
	pinstance.reset();
}


}