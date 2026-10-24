#include "pch.h"

using namespace CGDK;


void CGDK::time_event::Ievent::set_next_time(chrono::time::utc::time_point _time_next) noexcept
{
	// check)
	RETURN_IF(_time_next == this->m_event_status.time_next);

	// 1) change
	this->m_event_status.time_next = _time_next;

	// 2)
	auto pentity = this->m_pentity;

	// check)
	RETURN_IF(pentity.empty());

	// 2) notify to manager
	if (this->m_event_status.state == eSTATE::RUN)
	{
		pentity->process_notify_next_time_changed(this);
	}
}

void CGDK::time_event::Ievent::process_reset()
{
}

result_code CGDK::time_event::Ievent::process_event(chrono::time::utc::time_point /*_time_now*/)
{
	return result_code(0);
}

void CGDK::time_event::Ievent::set_next_event_after_now()
{
	//SCOPED_LOCK(m_queue_event)
	//{
	//	// check) 
	//	RETURN_IF(m_queue_event.empty());

	//	// 1) 현재시간을 구한다. 
	//	auto time_now = chrono::time::utc::now();

	//	while(m_queue_event.empty()==false)
	//	{
	//		// 2) Event의 제일 앞을 얻는다.
	//		auto pevent_next = m_queue_event.front();;

	//		// check) 아직 Event의 실행 시간이 지나지 않았으면 끝낸다.
	//		BREAK_IF(time_now < pevent_next->get_next_time());

	//		// 3) Priority Queue에서 하나를 빼낸다.
	//		_pop_heap();

	//		// 4) Times를 줄인다.
	//		if(pevent_next->times() > 0)
	//		{
	//			--pevent_next->count_times;
	//		}

	//		// 5) 반복횟수(Times)가 0이 아닐 경우에 다시 추가한다.
	//		if(pevent_next->times() != 0)
	//		{
	//			// - 실행한 Event에 대해 Interval만큼 증가시킨다.
	//			pevent_next->time_event_next += pevent_next->interval();

	//			// - 추가한다.
	//			_push_heap(pevent_next);
	//		}
	//	}

	//	// 6) Next 시간을 써넣는다.
	//	if (m_queue_event.empty() == false)
	//	{
	//		_set_next_event_time(m_queue_event.front()->get_next_time());
	//	}
	//	else
	//	{
	//		m_entity_status.reset_next_time();
	//	}
	//}
}
