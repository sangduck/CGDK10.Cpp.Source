#include "pch.h"


query_event::time_event_update::time_event_update() noexcept
{
}

query_event::time_event_update::~time_event_update() noexcept
{
}

void query_event::time_event_update::process_execute_query()
{
	// check)
	CGASSERT_ERROR(m_pserver_operation.exist());

	CGASSERT_ERROR(false);

	//try
	//{
	//	// Declar) 
	//	uint64_t	id_group;

	//	{
	//		// declare) 
	//		shared_buffer		buf_query = alloc_shared_buffer(16384);

	//		uint64_t					id_event			 = m_pserver_operation->get_time_event_id();
	//		uint64_t					id_source		 = m_ptime_event->m_idSource;
	//									id_group			 = m_pserver_operation->m_idSource;
	//		const char*					Name			 = m_pserver_operation->get_name();
	//		time_event::eTYPE			event_type		 = m_pserver_operation->get_time_event_type();
	//		time_event::eSETTER			event_setter	 = m_pserver_operation->get_time_event_setter();
	//		int							event_level		 = m_pserver_operation->get_event_level();
	//		chrono::time::utc::time_point	time_next		 = m_pserver_operation->get_next_time();
	//		int							status			 = 1;
	//		eSERVER_OPERATION			operation		 = eSERVER_OPERATION::NONE;
	//		uint64_t					hid				 = 0;
	//		uint64_t					id_server		 = 0;
	//		const char*					setting			 = "null";
	//		int							times			 = 0;

	//		// 2) 쿼리문의 기본문을 추가
	//		buf_query.append_text<char>("INSERT INTO TimeEventServer (id_event,id_source,id_group,Name,eEventType,event_setter,event_level,time_next,status,eOperation,hid,idServer,setting,times) VALUES (%I64d,%I64d,%I64d,'%s',%d,%d,%u,null,%d,%d,%I64d,%I64d,%s,%d);",
	//			(int64_t)id_event,
	//			(int64_t)id_source,
	//			(int64_t)id_group,
	//			Name,
	//			(int32_t)event_type,
	//			event_setter,
	//			event_level,
	//			status,
	//			(int32_t)operation,
	//			hid,
	//			id_server,
	//			setting,
	//			times
	//		);

	//		// 3) query를 요청한다.
	//		execute_query(std::string_view(buf_query.buf));

	//		// 4) query를 Close한다.
	//		close_query();
	//	}

	//	// 3) Schedule들을 써넣는다.
	//	for(auto& iter: m_pserver_operation->m_queue_event)
	//	{
	//		// declare) 
	//		shared_buffer		buf_query = alloc_shared_buffer(16384);

	//		auto	ptime_event_operation	 = dynamic_pointer_cast<time_event_server::server_operation>(iter);

	//		// check)
	//		CGASSERT_ERROR(ptime_event_operation.exist());

	//		uint64_t					id_event			 = ptime_event_operation->m_operation.id_event;
	//		uint64_t					id_source		 = ptime_event_operation->m_operation.id_source;
	//		//const char*				Name			 = "null";
	//		time_event::eTYPE			event_type		 = time_event::eTYPE::NONE;
	//		time_event::eSETTER			event_setter	 = time_event::eSETTER::NONE;
	//		int							event_level		 = 0;
	//		chrono::time::utc::time_point	time_next		 = ptime_event_operation->get_next_time();
	//		int							status			 = 1;
	//		eSERVER_OPERATION			operation		 = ptime_event_operation->m_operation.operation;
	//		uint64_t					hid				 = ptime_event_operation->m_operation.hid.mac.sll_mac;
	//		uint64_t					id_server		 = ptime_event_operation->m_operation.id_server.key;
	//		const char*					setting			 = ptime_event_operation->m_operation.setting.c_str();
	//		int							times			 = ptime_event_operation->count_times;

	//		buf_query.append_text<char>("INSERT INTO TimeEventServer (id_event,id_source,id_group,Name,event_type,event_setter,event_level,time_next,status,operation,hid,id_server,setting,times) VALUES (%I64d,%I64d,%I64d,null,%d,%d,%u,'%d-%d-%d %d:%d:%d.%d',%d,%d,%I64d,%I64d,'%s',%d);",
	//			(int64_t)id_event,
	//			(int64_t)id_source,
	//			(int64_t)id_group,
	//			(int32_t)event_type,
	//			event_setter,
	//			event_level,
	//			time_next.get_year(), time_next.get_month(), time_next.get_day(), time_next.get_hour(), time_next.get_minute(), time_next.get_second(), time_next.get_millisecond(),
	//			status,
	//			(int32_t)operation,			
	//			hid,
	//			id_server,
	//			setting,
	//			times
	//		);

	//		// 3) query를 요청한다.
	//		execute_query(std::string_view(buf_query.buf));

	//		// 4) query를 Close한다.
	//		close_query();
	//	}
	//}
	//catch (throwable::failure& /*_e*/)
	//{
	//	// check)
	//	CGASSERT_ERROR(false);

	//	// log) 
	//	LOG_ERROR(nullptr, "Fail to query 'Insert_TimeEventUpdate'");
	//}
	//catch (...)
	//{
	//	// check)
	//	CGASSERT_ERROR(false);

	//	// log) 
	//	LOG_ERROR(nullptr, "Fail to query 'Insert_TimeEventUpdate'");
	//}

	// 3) reset
	m_check_duplicate = false;
}
