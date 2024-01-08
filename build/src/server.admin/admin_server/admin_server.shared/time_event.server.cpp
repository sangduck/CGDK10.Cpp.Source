#include "pch.h"


time_event_server::operation::operation() noexcept :
	m_idx_list(-1)
{
}

time_event_server::operation::~operation() noexcept
{
}

//void time_event_server::operation::process_done_time_event() noexcept
//{
//	// 4) 실행 결과를 DB에 저장한다.
//	g_pservice->m_pmanager_time_event->done_time_event_result(this);
//}
