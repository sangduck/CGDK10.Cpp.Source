#include "pch.h"


query_event::time_event_done::time_event_done() noexcept
{
}

query_event::time_event_done::~time_event_done() noexcept
{
}

void query_event::time_event_done::process_execute_query()
{
	//// check)
	//CGASSERT_ERROR(m_pserver_operation.exist());

	//// declare) 
	//shared_buffer		buf_query	 = alloc_shared_buffer(16384);

	//// check) check duplcate uid (only recover)
	//if (m_check_duplicate)
	//{
	//	// - shared_buffer copy
	//	shared_buffer	buf_temp	 = buf_query;

	//	// - 쿼리문의 기본문을 추가
	//	buf_query.append_text<char>("SELECT id_event FROM TimeEventServer WHERE id_event = %I64u", (uint64_t)m_id_event);

	//	// - query를 요청한다.
	//	execute_query(static_cast<std::string_view>(buf_query));

	//	// - exist?
	//	auto	result = move_first();

	//	// - close
	//	close_query();

	//	// - reset option
	//	m_check_duplicate = false;

	//	// check)
	//	RETURN_IF(result == eRESULT::SUCCESS, );
	//}



	//uint64_t	id_event	 = m_id_event;
	//uint64_t	id_source	 = m_pserver_operation->get_time_event_id();
	//uint64_t	id_group	 = id_source;
	//int			status		 = 1;
	//uint64_t	hid			 = 0;
	//uint64_t	id_server	 = 0;
	//int			times		 = 0;

	//// 2) 쿼리문의 기본문을 추가
	//buf_query.append_text<char>("INSERT INTO TimeEventServer (id_event,id_source,id_group,Name,eEventType,event_setter,event_level,time_next,status,eOperation,hid,idServer,setting,times) VALUES (%I64d,%I64d,%I64d,null,0,0,0,null,%d,0,%I64d,%I64d,null,%d);",
	//	(int64_t)id_event,
	//	(int64_t)id_source,
	//	(int64_t)id_group,
	//	status,
	//	hid,
	//	id_server,
	//	times
	//);

	//// 3) query를 요청한다.
	//execute_query(static_cast<std::string_view>(buf_query));
}
