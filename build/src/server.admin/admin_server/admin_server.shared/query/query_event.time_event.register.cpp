#include "pch.h"


query_event::time_event_register::time_event_register() noexcept
{
}

query_event::time_event_register::~time_event_register() noexcept
{
}

void query_event::time_event_register::process_execute_query()
{
	//// check)
	//CGASSERT_ERROR(m_pserver_operation.exist());

	//{
	//	// declare) 
	//	auto buf_query = alloc_shared_buffer(16384);

	//	// check) check duplcate uid (only recover)
	//	if (m_check_duplicate)
	//	{
	//		// - shared_buffer copy
	//		shared_buffer buf_temp = buf_query;

	//		// - 쿼리문의 기본문을 추가
	//		buf_query.append_text<char>("SELECT id_event FROM TimeEventServer WHERE id_event = %I64u", (uint64_t)m_pserver_operation->get_time_event_id());

	//		// - query를 요청한다.
	//		execute_query(static_cast<std::string_view>(buf_query));

	//		// - exist?
	//		auto result = move_first();

	//		// - close
	//		close_query();

	//		// - reset option
	//		m_check_duplicate = false;

	//		// check)
	//		RETURN_IF(result == eRESULT::SUCCESS, );
	//	}

	//	uint64_t					id_event		 = m_pserver_operation->get_time_event_id();
	//	uint64_t					id_source		 = id_event;
	//	uint64_t					id_group		 = id_event;
	//	std::string_view			Name			 = m_pserver_operation->get_name();
	//	time_event::eTYPE			event_type		 = m_pserver_operation->get_time_event_type();
	//	time_event::eSETTER			event_setter	 = m_pserver_operation->get_time_event_setter();
	//	int							event_level		 = m_pserver_operation->get_event_level();
	//	chrono::time::utc::time_point	time_next		 = m_pserver_operation->get_next_time();
	//	int							status			 = 0;
	//	eSERVER_OPERATION			operation		 = eSERVER_OPERATION::NONE;
	//	uint64_t					hid				 = 0;
	//	uint64_t					id_server		 = 0;
	//	const char*					setting			 = "null";
	//	int							times			 = 0;

	//	// 2) 쿼리문의 기본문을 추가
	//	buf_query.append_text<char>("INSERT INTO TimeEventServer (id_event,id_source,id_group,Name,eEventType,event_setter,event_level,time_next,status,eOperation,hid,idServer,setting,times) VALUES (%I64d,%I64d,%I64d,'%s',%d,%d,%u,null,%d,%d,%I64d,%I64d,%s,%d);",
	//		(int64_t)id_event,
	//		(int64_t)id_source,
	//		(int64_t)id_group,
	//		Name.data(),
	//		(int32_t)event_type,
	//		event_setter,
	//		event_level,
	//		status,
	//		(int32_t)operation,
	//		hid,
	//		id_server,
	//		setting,
	//		times
	//	);

	//	// 3) query를 요청한다.
	//	execute_query(static_cast<std::string_view>(buf_query));

	//	// 4) query를 Close한다.
	//	close_query();
	//}

	//// 3) Schedule들을 써넣는다.
	//for(auto& iter: m_pserver_operation->m_queue_event)
	//{
	//	// declare) 
	//	auto buf_query = alloc_shared_buffer(16384);

	//	// - get time_event_operation
	//	auto	ptime_event_operation = dynamic_pointer_cast<time_event_server::server_operation>(iter);

	//	// check)
	//	CGASSERT_ERROR(ptime_event_operation.exist());

	//	// check) check duplcate uid (only recover)
	//	if (m_check_duplicate)
	//	{
	//		// - shared_buffer copy
	//		shared_buffer buf_temp = buf_query;

	//		// - 쿼리문의 기본문을 추가
	//		buf_query.append_text<char>("SELECT id_event FROM TimeEventServer WHERE id_event = %I64u", (uint64_t)ptime_event_operation->m_operation.id_event);

	//		// - query를 요청한다.
	//		execute_query(static_cast<std::string_view>(buf_query));

	//		// - exist?
	//		auto result = move_first();

	//		// - close
	//		close_query();

	//		// - reset option
	//		m_check_duplicate = false;

	//		// check)
	//		RETURN_IF(result == eRESULT::SUCCESS, );
	//	}

	//	uint64_t					id_event		 = ptime_event_operation->m_operation.id_event;
	//	uint64_t					id_source		 = id_event;
	//	uint64_t					id_group		 = m_pserver_operation->get_time_event_id();
	//	//const char*				Name			 = "null";
	//	time_event::eTYPE			event_type		 = time_event::eTYPE::NONE;
	//	time_event::eSETTER			event_setter	 = time_event::eSETTER::NONE;
	//	int							event_level		 = 0;
	//	chrono::time::utc::time_point	time_next		 = ptime_event_operation->get_next_time();
	//	int							status			 = 0;
	//	eSERVER_OPERATION			operation		 = ptime_event_operation->m_operation.operation;
	//	uint64_t					hid				 = ptime_event_operation->m_operation.hid.mac.sll_mac;
	//	uint64_t					id_server		 = ptime_event_operation->m_operation.id_server.key;
	//	const char*					setting			 = ptime_event_operation->m_operation.setting.c_str();
	//	int							times			 = ptime_event_operation->count_times;

	//	buf_query.append_text<char>("INSERT INTO TimeEventServer (id_event,id_source,id_group,Name,eEventType,event_setter,event_level,time_next,status,eOperation,hid,idServer,Setting,times) VALUES (%I64d,%I64d,%I64d,null,%d,%d,%u,'%d-%d-%d %d:%d:%d.%d',%d,%d,%I64d,%I64d,'%s',%d);",
	//		(int64_t)id_event,
	//		(int64_t)id_source,
	//		(int64_t)id_group,
	//		(int32_t)event_type,
	//		event_setter,
	//		event_level,
	//		time_next.get_year(), time_next.get_month(), time_next.get_day(), time_next.get_hour(), time_next.get_minute(), time_next.get_second(), time_next.get_millisecond(),
	//		status,
	//		(int32_t)operation,			
	//		hid,
	//		id_server,
	//		setting,
	//		times
	//	);

	//	// 3) query를 요청한다.
	//	execute_query(static_cast<std::string_view>(buf_query));

	//	// 4) query를 Close한다.
	//	close_query();
	//}

	//// - reset option
	//m_check_duplicate = false;
}
