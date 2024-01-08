#include "pch.h"

// declare) 
own_ptr<service>			g_pservice;
own_ptr<logger_hub_archive>	g_plogger;


service::service() noexcept :
	server::Nservice()
{
	// 1) Configure file
	m_filename_configure = CONFIGURE_FILE;

	// 2) Default 설정
	m_info_server_setting.rank.role	 /*= static_cast<int32_t>(eSERVER_TYPE::ADMIN_SERVER)*/;
	m_info_server_setting.id = server_admin::sID(1, 1);
	m_info_server_setting.name = "control server_entity";

	// 3) set name
	set_name(m_info_server_setting.name);

	// 4) logger_file
	m_plogger_file = make_own<logger_file>("logger_file");
	m_plogger_file->initialize(LOG_FILE);
	get_logger()->register_log_targetable(m_plogger_file);

	// 5) server instance manager
	m_pmanager_server_instance = make_own<server_entity::manager_instance>("manager_server_instance");
	Nattachable<Iobject_state>::process_attach(m_pmanager_server_instance);

	// 6) querier
	m_pquerier_server = make_own<querier_server>("querier_server");
	//Nattachable<Iobject_state>::process_attach(m_pquerier_server);
	
	// 7) server_entity - agent
	m_pgroup_server_entity_agent = make_own<server_entity::host::group>("group_server_agent");
	Nattachable<Iobject_state>::process_attach(m_pgroup_server_entity_agent);

	m_pgroup_server_entity_agent_immigration = make_own<server_entity::host::group_immigration>("group_server_agent_immigration");
	Nattachable<Iobject_state>::process_attach(m_pgroup_server_entity_agent_immigration);
	
	// 8) server_entity - server_entity
	m_pgroup_server_entity = make_own<server_entity::group>("group_server_server");
	Nattachable<Iobject_state>::process_attach(m_pgroup_server_entity);

	m_pgroup_server_entity_immigration = make_own<server_entity::group_immigration>("group_server_server_immigration");
	Nattachable<Iobject_state>::process_attach(m_pgroup_server_entity_immigration);

	// 9) admin client - app
	m_pgroup_client = make_own<client::app::group>("group_admin_client");
	Nattachable<Iobject_state>::process_attach(m_pgroup_client);

	m_pgroup_client_immigration = make_own<client::app::group_immigration>("group_admin_client_immigration");
	Nattachable<Iobject_state>::process_attach(m_pgroup_client_immigration);

	// 10) admin client- web
	m_pgroup_client_web_immigration = make_own<client::web::group_immigration>("group_websocket_immigration");
	Nattachable<Iobject_state>::process_attach(m_pgroup_client_web_immigration);

	m_pgroup_client_web = make_own<client::web::group>("group_websocket");
	Nattachable<Iobject_state>::process_attach(m_pgroup_client_web);

	m_pacceptor_client_web = make_own<net::acceptor<client::web::socket>>("acceptor_web");
	Nattachable<Iobject_state>::process_attach(m_pacceptor_client_web);

	// 11) server_entity
	m_pgroup_server_entity_immigration = make_own<server_entity::group_immigration>("group_server_immigration");
	Nattachable<Iobject_state>::process_attach(m_pgroup_server_entity_immigration);
	
	m_pacceptor_server_entity_agent = make_own<net::acceptor<server_entity::host::socket>>("acceptor_host");
	Nattachable<Iobject_state>::process_attach(m_pacceptor_server_entity_agent);

	m_pacceptor_server_entity = make_own<net::acceptor<server_entity::socket>>("acceptor_server");
	Nattachable<Iobject_state>::process_attach(m_pacceptor_server_entity);

	m_pacceptor_client = make_own<net::acceptor<client::app::socket>>("acceptor_admin_client");
	Nattachable<Iobject_state>::process_attach(m_pacceptor_client);
	
	// 12) time event
	m_pmanager_time_event = make_own<time_event_server::manager>("manager_time_event");
	Nattachable<Iobject_state>::process_attach(m_pmanager_time_event);
}

service::~service() noexcept
{
	// 1) get logger hub
	auto plogger = get_logger();

	// 2) detach file logger
	if (plogger.exist())
	{
		plogger->unregister_log_targetable(m_plogger_file);
	}

	// 2) reset file logger
	m_plogger_file.reset();
}

void service::add_event_log(server_event_log* _pserver_event_log)
{
	scoped_lock(m_list_server_event_log)
	{
		// 1) 최대 보관 갯수를 초과했으면 하나를 뺀다.
		if(m_list_server_event_log.size() >= 2000)
		{
			m_list_server_event_log.pop_front();
		}

		// 2) 신규추가~
		m_list_server_event_log.push_back(_pserver_event_log);

		// notify) 
		transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, CGNOTIFY_SERVER_TIME_EVENT_LOG_ADD, reinterpret_cast<uintptr_t>(_pserver_event_log)));
		m_pgroup_client->send(MESSAGE_ADMIN_SERVER_TIME_EVENT_LOG_ADD::MAKE_BUFFER(*_pserver_event_log));
	}
}

//shared_buffer service::message_SERVER_ADMIN_SERVER_TIME_EVENT_LOG_LIST()
//{
//	// declare) 
//	vector<sSERVER_EVENT_LOG>	vector_admin_server_event_log;
//
//	// 1) 메시지 내용을 작성한다.
//	scoped_lock(m_list_server_event_log)
//	{
//		// - 먼저 Reserve한다.
//		vector_admin_server_event_log.reserve(m_list_server_event_log.size());
//
//		// - 모두 추가~~
//		for(auto& iter: m_list_server_event_log)
//		{
//			vector_admin_server_event_log.push_back(*iter);
//		}
//	}
//
//	// 1) 버퍼를 할당한다.
//	shared_buffer buf_temp = alloc_shared_buffer(sizeof(MSG_HEADER) + get_size_of(vector_admin_server_event_log));
//
//	// 2) 메시지 헤더를 써넣는다.
//	buf_temp.append<uint32_t>();
//	buf_temp.append<eMESSAGE_TYPE>(eMESSAGE_TYPE::ADMIN_SERVER_TIME_EVENT_LOG_LIST);
//
//	// 3) 메시지 내용을 써넣는다.
//	buf_temp.append(vector_admin_server_event_log);
//
//	// 4) 메시지 길이를 써넣는다.
//	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();
//
//	// return)
//	return	buf_temp;
//}

void service::on_service_setting(context& _context)
{
	// 1) setting을 Load한다.
	_context.read_from_json_file(CONFIGURE_FILE);

	// 2) 
	Nservice::on_service_setting(_context);
}

void service::on_initializing(context& _context)
{
	// trace) 
	LOG_SYSTEM_IMPORTANT	<< "-----------------------------------------------------------------";
	LOG_SYSTEM				<< " ";
	LOG_INFO_IMPORTANT		<< "                        CGCII admin server";
	LOG_SYSTEM				<< " ";
	LOG_SYSTEM				<< "@ SVN_URL: " << m_info_server_setting.svn_url;
	LOG_SYSTEM				<< "@ SVN_UPDATE_REVISION: " << SVN_COMMIT_REVISION_STRING;
	LOG_SYSTEM				<< "@ SVN_DATE: " << SVN_TIME_NOW_STRING;
	LOG_SYSTEM				<< "@ CGDK: " << CGDK_CONFIGURATION;
	LOG_SYSTEM				<< "@ compile: " << __DATE__ << " " << __TIME__;
	LOG_SYSTEM				<< " ";
	LOG_SYSTEM_IMPORTANT	<< "-----------------------------------------------------------------";

	Nservice::on_initializing(_context);
}

void service::on_initialize(context& _context)
{
	Nservice::on_initialize(_context);
}

void service::on_destroying()
{
	Nservice::on_destroying();
}

void service::on_destroy()
{
	Nservice::on_destroy();
}

void service::on_starting(context& _context)
{
	Nservice::on_starting(_context);
}

void service::on_start(context& _context)
{
	Nservice::on_start(_context);
}

void service::on_stopping()
{
	Nservice::on_stopping();
}

void service::on_stop()
{
	Nservice::on_stop();
}

void service::on_execute()
{
}
