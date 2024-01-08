#include "pch.h"


query_event::time_event_process::time_event_process() noexcept
{
}

query_event::time_event_process::~time_event_process() noexcept
{
}

void query_event::time_event_process::process_execute_query()
{
	//// declare) 
	//auto buf_query = alloc_shared_buffer(16384);

	//// check) check duplcate uid (only recover)
	//if (m_check_duplicate)
	//{
	//	// - shared_buffer copy
	//	shared_buffer buf_temp = buf_query;

	//	// - 쿼리문의 기본문을 추가
	//	buf_query.append_text<char>("SELECT id_event FROM TimeEventServer WHERE id_event = %I64u", (uint64_t)m_id_event);

	//	// - query를 요청한다.
	//	execute_query(static_cast<std::string_view>(buf_query));

	//	// - exist?
	//	auto result = move_first();

	//	// - close
	//	close_query();

	//	// - reset option
	//	m_check_duplicate = false;

	//	// check)
	//	RETURN_IF(result == eRESULT::SUCCESS, );
	//}

	//// 1) ...
	//auto id_event = m_id_event;
	//auto id_source = m_pserver_operation->m_operation.id_event;
	//auto id_group = m_pserver_operation->m_operation.id_group;
	//auto time_next = m_pserver_operation->get_next_time();
	//int	status = 1;
	//int	times = m_pserver_operation->count_times;

	//// 2) ...
	//buf_query.append_text<char>("INSERT INTO TimeEventServer (id_event,id_source,id_group,Name,eEventType,event_setter,event_level,time_next,status,eOperation,hid,idServer,setting,times) VALUES (%I64d,%I64d,%I64d,null,0,0,0,null,%d,0,0,0,null,%d);",
	//	(int64_t)id_event,
	//	(int64_t)id_source,
	//	(int64_t)id_group,
	//	status,
	//	times
	//);

	//// 3) query를 요청한다.
	//execute_query(static_cast<std::string_view>(buf_query));
}
