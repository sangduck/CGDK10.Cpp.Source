#include "pch.h"


query_event::host_certify::host_certify() noexcept
{
}

query_event::host_certify::~host_certify() noexcept
{
}

void query_event::host_certify::process_execute_query()
{
	//// declare) 
	//auto buf_query = alloc_shared_buffer(4096);

	//if (m_check_duplicate)
	//{
	//	// - shared_buffer copy
	//	auto buf_temp = buf_query;

	//	// - 쿼리문의 기본문을 추가
	//	buf_query.append_text<char>("SELECT uid FROM EventHost WHERE uid = %I64u", (uint64_t)m_uid);

	//	// - query를 요청한다.
	//	execute_query(static_cast<std::string_view>(buf_query));

	//	// check) 
	//	for (auto iter = begin(); iter != end(); ++iter)
	//	{
	//		//return;
	//	}

	//	// - close
	//	close_query();

	//	// - reset option
	//	m_check_duplicate = false;
	//}

	//// declare)
	//char		str_Source_command_uid[64];
	//const char*	pstr_source_command_uid;

	//// declare)
	//uint64_t	command	 = 0;
	//uint32_t	eReason	 = 0;

	//// 1) Command
	//if (command != 0)
	//{
	//	__to_string(str_Source_command_uid, command);
	//	pstr_source_command_uid = str_Source_command_uid;
	//}
	//else
	//{
	//	pstr_source_command_uid = "null";
	//}

	//// declare)
	//char		str_Source_system_uid[64];
	//const char*	pstr_source_system_uid;

	//// 2) host ID를 설정한다.
	//if(m_info_server_setting.hid.code != 0)
	//{
	//	__to_string(str_Source_system_uid, m_info_server_setting.hid.code);
	//	pstr_source_system_uid = str_Source_system_uid;
	//}
	//else
	//{
	//	pstr_source_system_uid = "null";
	//}
	//	
	//// 2) 쿼리문의 기본문을 추가
	//buf_query.append_text<char>("INSERT INTO EventHost (uid,id_server,eType,eReason,timeOccure,Source_command_uid,Source_server_uid,Source_system_uid,ip,MAC) VALUES (%I64u,%I64u,%u,%u,GETUTCDATE(),%s,%I64u,%s,'%s','%s');",
	//	(uint64_t)m_uid,
	//	(uint64_t)m_info_server_setting.id.key,
	//	(uint32_t)eEVENT_TYPE::SERVER_CONNECT,
	//	(uint32_t)eReason,
	//	pstr_source_command_uid,
	//	pstr_source_system_uid
	//);

	//// 3) query를 요청한다.
	//execute_query(static_cast<std::string_view>(buf_query));

	//// reset)
	//m_psocket.reset();
}
