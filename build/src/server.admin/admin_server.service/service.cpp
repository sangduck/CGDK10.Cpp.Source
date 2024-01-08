#include "pch.h"

std::atomic<uint64_t> service::m_id_alloc = 0;

#define	LOG_FILE	"admin_server.log"


uint64_t service::_alloc_object_id()
{
	return	m_id_alloc++;
}

service::service() :
	m_is_disconnected_unregistered(false)
{
	////-----------------------------------------------------------------
	//// 1. child object create and attach
	////-----------------------------------------------------------------
	//// 1) create child node
	//m_pgroup_server_immigration		 = make_own<group_server_immigration>("groupServerImmigration");		 Nattachable<Iobject_state>::process_attach(m_pgroup_server_immigration);
	//m_pgroup_server					 = make_own<group_server>("groupServer");								 Nattachable<Iobject_state>::process_attach(m_pgroup_server);
	//m_pgroup_host_agent_immigration	 = make_own<group_host_agent_immigration>("groupControlImmigration");		 Nattachable<Iobject_state>::process_attach(m_pgroup_host_agent_immigration);
	//m_pgroup_host_agent				 = make_own<group_host_agent>("groupControl");								 Nattachable<Iobject_state>::process_attach(m_pgroup_host_agent);
	//m_pacceptor_server				 = make_own<net::acceptor<socket_server>>("acceptor_server");			 Nattachable<Iobject_state>::process_attach(m_pacceptor_server);
	//m_pgroup_websocket_immigration	 = make_own<group_websocket_immigration>("group_websocket_immigration"); Nattachable<Iobject_state>::process_attach(m_pgroup_websocket_immigration);
	//m_pgroup_websocket				 = make_own<group_websocket>("group_websocket");						 Nattachable<Iobject_state>::process_attach(m_pgroup_websocket);
	//m_pacceptor_web					 = make_own<net::acceptor<socket_web>>("acceptor_web");					 Nattachable<Iobject_state>::process_attach(m_pacceptor_web);
	//m_pacceptor_websocket			 = make_own<net::acceptor<socket_websocket>>("acceptor_websocket");		 Nattachable<Iobject_state>::process_attach(m_pacceptor_websocket);
	//m_pacceptor_control				 = make_own<net::acceptor<socket_host_agent>>("acceptor_control");			 Nattachable<Iobject_state>::process_attach(m_pacceptor_control);
	//m_psocket_to_admin_server		 = make_own<socket_host_agent_admin_server>("socket_to_admin_server");		 Nattachable<Iobject_state>::process_attach(m_psocket_to_admin_server);


	////-----------------------------------------------------------------
	//// 2. Host Info
	////-----------------------------------------------------------------
	//// 1) host setting info
	//m_setting_host.hid.mac		 = api::get_mac_address().front();
	//m_setting_host.hid.code		 = 0;
	//m_setting_host.name			 = api::get_host_name();
	//m_setting_host.comment[0]	 = 0;

	//// 2) host info
	//m_info_host.host_name		 = api::get_host_name();
	//m_info_host.hid.mac			 = api::get_mac_address().front();
	//m_info_host.hid.code		 = 0;
	//m_info_host.info_system		 = api::get_system_info();
	//m_info_host.info_os			 = api::get_os_info();
	//m_info_host.time_start		 = chrono::time::utc::now();
	//m_info_host.addr_host		 = api::socket::get_instance().get_local_address_public();
	//m_info_host.uid_host		 = api::get_host_id();
	//m_info_host.uid_os			 = api::get_os_unique_id();


	////-----------------------------------------------------------------
	//// 3. host status
	////-----------------------------------------------------------------
	//// 1) set host status value
	//m_status_host.hid = m_info_host.hid;
	//m_status_host.usage_network = 0;
	//m_status_host.usage_processor = 0;
}

service::~service()
{
}

void service::on_service_setting(context& _context)
{
	// Definition) 
#if defined(_WIN32)
	#define	SETTING_FILE_PATH	"./"
	#define	SETTING_FILE_NAME	"host_agent_setting.json"
#elif defined(__linux__)
	#define	SETTING_FILE_PATH	"/home/sangduck/projects/CGDK10.C++/src/server.admin/host_agent.service/"
	#define	SETTING_FILE_NAME	"host_agent_setting.json"
#endif

	// 1) set default value
	_context["name"] = "CGCII host_agent"sv;
	_context["log_file"]["path"] = SETTING_FILE_PATH;
	_context["log_file"]["filename"] = LOG_FILE;

	// 2) read setting json file
	_context.read_from_json_file(api::file::make_full_path(SETTING_FILE_PATH, SETTING_FILE_NAME));
}

void service::on_initializing(context& _context)
{
	//-----------------------------------------------------------------
	// 1. log object
	//-----------------------------------------------------------------
	// declare) 
	own_ptr<logger_file> plog_file;

	try
	{
		// 1) create log_object (for file loging)
		plog_file = make_own<logger_file>("log_file");

		// 2) initialize
		plog_file->initialize(_context);

		// 3) register to logger_hub(g_plogger)
		get_logger()->register_log_targetable(plog_file);

		// 4) set..
		m_plogger_file = std::move(plog_file);
	}
	catch(...)
	{
		if(plog_file.exist())
		{
			get_logger()->unregister_log_targetable(plog_file);
			plog_file->destroy();
		}

		// reraise)
		throw;
	}


	//-----------------------------------------------------------------
	// 2. read host info
	//-----------------------------------------------------------------
	// 1) processor...
	m_perfornamce_processor = make_object<api::performance_counter>("\\Processor(_Total)\\% Processor Time");

	// 2) load host list
	load_host_server_info();

	// trace) 
	LOG_SYSTEM			 << "-----------------------------------------------------------------";
	LOG_SYSTEM			 << "";
	LOG_SYSTEM_IMPORTANT << "                    CGCII Network HOSTAGENT";
	LOG_SYSTEM			 << "";
	LOG_SYSTEM			 << "  @CGDK: " << CGDK_CONFIGURATION;
	LOG_SYSTEM			 << "  @compile: " << __DATE__ << " " << __TIME__;
	LOG_SYSTEM			 << "";
	LOG_SYSTEM			 << "-----------------------------------------------------------------";
}

void service::on_initialize(context& /*_context*/)
{
}

void service::on_destroying()
{

}

void service::on_destroy()
{
	// 1) write host server info to json file
	save_host_server_info();

	// 2) processor...
	m_perfornamce_processor.reset();

	// 3) detach log object 
	if(m_plogger_file.exist())
	{
		get_logger()->unregister_log_targetable(m_plogger_file);
		m_plogger_file->destroy();
		m_plogger_file.reset();
	}
}

void service::on_starting(context& /*_context*/)
{
	// log) 
	LOG_INFO << " - HOST.name: " << m_info_host.host_name;
	LOG_INFO << " - HOST.id: " << m_info_host.hid.mac.sll_addr[0] << ":" << m_info_host.hid.mac.sll_addr[1] << ":" << m_info_host.hid.mac.sll_addr[2] << ":" << m_info_host.hid.mac.sll_addr[3] << ":" << m_info_host.hid.mac.sll_addr[4] << ":" << m_info_host.hid.mac.sll_addr[5] << "." << m_info_host.hid.code;
	LOG_INFO << " - HOST.infoSys: core count=" << m_info_host.info_system.processor.number_of_processor;
	LOG_INFO << " - HOST.info_os: " << to_string<char>(m_info_host.info_os.type) << " " << m_info_host.info_os.version.major << "." << m_info_host.info_os.version.minor << "." << m_info_host.info_os.version.build;
	LOG_INFO << " - HOST.uidHost: " << m_info_host.uid_host;
	LOG_INFO << " - HOST.uidOS: " << m_info_host.uid_os;

	// 1) register message medaitor
	time_event::entity_manager::get_instance()->register_messageable(this);

	// 2) registered Server Info占쏙옙 占싻억옙쨈占?
	load_server_list();
}

void service::on_start(context& /*_context*/)
{
	// 1) set executing interval
	execute_interval(chrono::seconds(2));

	// 2) start executing 
	system_executor::register_schedulable(this);
}

void service::on_stopping()
{
	// 1) unregister message medaitor
	time_event::entity_manager::get_instance()->unregister_messageable(this);
}

void service::on_stop()
{
	// 1) stop excution (unregister from schedulable manager)
	system_executor::unregister_schedulable(this);

	// 2) save server list
	save_server_list();

	// 3) unregister all server info
	unregister_server_setting_all();
}

void service::on_pause()
{
	pause();
}

void service::on_resume()
{
	resume();
}

void service::set_host_info(const server_admin::sHOST_INFO& _host_info)
{
	// 1) set host info
	m_info_host = _host_info;

	// 2) get public ip of this host
	api::socket::get_instance().get_local_address_public(m_info_host.addr_host);
	m_info_host.addr_host.port(0);

	// 3) store host server info to json file
	save_host_server_info();

	//// notify) notify modification of host server info
	//transmit_message(sMESSAGE_NOTIFY(eMESSAGE_WINDOWS_NOTIFY_UPDATE, eMESSAGE_UPDATE_HOST_INFO, reinterpret_cast<uintptr_t>(this)));
}

void service::set_host_setting(const server_admin::sHOST_SETTING& _host_setting)
{
	// 1) set value
	m_setting_host = _host_setting;

	// 2) store to json file
	save_host_server_info();
}

object_ptr<server_entity> service::find_registered_server_setting(uint64_t _pid) const noexcept
{
	// declare) 
	object_ptr<server_entity> pfind;

	scoped_lock(m_list_server_entity)
	{
		for (auto& iter : m_list_server_entity)
		{
			if (iter->status_server.pid == _pid)
			{
				pfind = iter;
				break;
			}
		}
	}

	// return)
	return pfind;
}

object_ptr<server_entity> service::find_registered_server_setting(const server_admin::sID& _id) const noexcept
{
	// declare) 
	object_ptr<server_entity> pfind;

	scoped_lock(m_list_server_entity)
	{
		for (auto& iter : m_list_server_entity)
		{
			if (iter->info_server_register.server_setting.id.key == _id.key)
			{
				pfind = iter;
				break;
			}
		}
	}

	// return)
	return pfind;
}

object_ptr<server_entity> service::init_server_register_info(uint64_t _pid, const sVERSION& /*_Version*/)
{
#if defined(_WIN32)
	// declare) 
	char str_exe[260];

	// 1) convert process id to process handle
	auto process_handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_TERMINATE, FALSE, static_cast<DWORD>(_pid));

	// declare) 
	DWORD dwSize = sizeof(str_exe);
			
	// 2) get image name
	QueryFullProcessImageNameA(process_handle, 0, str_exe, &dwSize);
#elif defined(__linux__)
	PROCESS_ID process_handle = INVALID_PROCESS_ID;
#else
	#error "CGDK Not supported platform"
#endif

	// declare) 
	std::string	str_file_path;
	std::string	str_file_name;

	// 3) get file path and filename
	{
		// declare) 
		char name[260];
		char str_ext[260];
		char str_drive[32];
		char str_path[260];
		
	#if defined(_WIN32)
		// - extract filename
		_splitpath_s(str_exe, str_drive, 32, str_path, 260, name, 260, str_ext, 260);
	#elif defined(__linux__)
	#else
		#error "CGDK Not supported platform"
	#endif

		// - splite file path and filename and store
		str_file_path = fmt::format("{}{}", str_drive, str_path);
		str_file_name = fmt::format("{}{}", name, str_ext);
	}

	// declare) 
	object_ptr<server_entity> pserver_entity_info;

	scoped_lock(m_list_server_entity)
	{
		// 4) 먼저 process_handle로 찾는다
		pserver_entity_info = find_registered_server_setting(_pid);

		// Case) 찾지 못했거나 찾았어도 이미 접속된 Socket이 있을 경우) 새로 생성한다.
		if (pserver_entity_info.empty() || pserver_entity_info->psocket.exist())
		{
			// - 신규로 생성한다.
			pserver_entity_info = make_object<server_entity>();

			// - 값을 설정한다.
			server_admin::sSERVER_SETTING_INFO	info_server_setting;
			pserver_entity_info->info_server_register.server_setting.name;
			pserver_entity_info->info_server_register.server_setting.id		 = server_admin::sID{ 0, 0 };
			pserver_entity_info->info_server_register.server_setting.rank	 = server_admin::sSERVER_RANK{ 0, 0, 0, 0, 0 };
			pserver_entity_info->info_server_register.server_setting.hid	 = get_host_info().hid;
			pserver_entity_info->info_server_register.server_setting.comment;
			pserver_entity_info->info_server_register.file_execute;
			pserver_entity_info->info_server_register.str_path_working;
			pserver_entity_info->info_server_register.svn_url;
			pserver_entity_info->info_server_register.svn_revision			 = 0;
			pserver_entity_info->info_server_register.is_registered			 = 0;

			// - infoRegisterSetting
			pserver_entity_info->handle_process								 = INVALID_PROCESS_ID;
		}

		// Case) 이미 ProcessID가 설정되지 않는 경우.
		if (pserver_entity_info->handle_process == INVALID_PROCESS_ID)
		{
			// - Process ID로 Pross Handle을 얻어낸다.
			pserver_entity_info->handle_process								 = process_handle;
			pserver_entity_info->status_server.pid							 = _pid;

			// - Path/execute/Working을 설정한다.
			pserver_entity_info->info_server_register.str_path				 = str_file_path;
			pserver_entity_info->info_server_register.file_execute			 = str_file_name;
			pserver_entity_info->info_server_register.str_path_working		 = str_file_path;
		}

	#if defined(_WIN32)
		// declare) 
		TCHAR	str_query[256];
		swprintf_s(str_query, 256, TEXT("\\Processor(%hs)\\%%Processor Time"), str_file_name.c_str());

		// 5) Performance Query를 수행한다.
		pserver_entity_info->perfornamce_processor = make_object<api::performance_counter>(str_query);

		// 6) 등록한다.
		if (pserver_entity_info->info_server_register.is_registered == false)
		{
			register_server_setting(pserver_entity_info);
		}
	#elif defined(__linux__)
	#else
		#error "CGDK Not supported platform"
	#endif
	}

	// 7) control에 PID를 전송한다.
	m_pgroup_host_agent->send(server_admin::message_SERVER_ADMIN_SERVER_PID_UPDATE(pserver_entity_info->info_server_register.server_setting.hid, pserver_entity_info->info_server_register.server_setting.id, _pid));

	// return) 
	return pserver_entity_info;
}


void service::_notify_to_controls(const shared_buffer& _buffer)
{
	// 1) send to controls
	m_pgroup_host_agent->send(_buffer);
}

void service::_notify_to_controls(shared_buffer&& _buffer)
{
	_notify_to_controls(_buffer);
}

void service::notify_to_controls(const shared_buffer& _buffer)
{
	//1) get instance
	auto pinstance = peek_instance();

	// check)
	RETURN_IF(pinstance.empty());

	// 2) send
	pinstance->_notify_to_controls(_buffer);
}

bool service::request_init_service(const server_admin::sID& _id)
{
	// 1) find register info
	auto pregister_info = find_registered_server_setting(_id);

	// check) return false if failed
	RETURN_IF(pregister_info.empty(), false);

	// 2) request initialize service (execute server)
	auto result = pregister_info->request_init_service();

	// check)
	if (result == false)
	{
		// log) 
		LOG_ERROR << "@SERVER['" << pregister_info->info_server_register.server_setting.name << "':" << pregister_info->info_server_register.server_setting.id.key << "] fail to initialization";
	}

	// return)
	return true;
}

bool service::request_destroy_service(const server_admin::sID& _id)
{
	// 1) find register info
	auto pregister_info = m_pgroup_server->find_server(_id);

	// check) return false if failed
	RETURN_IF(pregister_info.empty(), false);

	// 2) request destroying service
	auto result = pregister_info->request_destroy_service();

	// check) 
	RETURN_IF(result == false, false);

	// return)
	return true;
}

bool service::request_start_service(const server_admin::sID& _id, const std::string& _setting)
{
	// 1) find register info
	auto pregister_info = find_registered_server_setting(_id);

	// check) return false if failed
	RETURN_IF(pregister_info.empty(), false);

	// 2) request starting service
	auto result = pregister_info->request_start_service(_setting);

	// check) 
	RETURN_IF(result == false, false);

	// return) 
	return true;
}

bool service::request_stop_service(const server_admin::sID& _id)
{
	// 1) find register info
	auto pregister_info = m_pgroup_server->find_server(_id);

	// check) return false if failed
	RETURN_IF(pregister_info.empty(), true);

	// 2) request stopping service
	auto result = pregister_info->request_stop_service();

	// check) 
	RETURN_IF(result == false, false);

	// return) 
	return true;
}

bool service::request_pause_service(const server_admin::sID& _id)
{
	// 1) find register info
	auto pregister_info = m_pgroup_server->find_server(_id);

	// check) return false if failed
	RETURN_IF(pregister_info.empty(), true);

	// 2) request pause service
	auto result = pregister_info->request_pause_service();

	// check) 
	RETURN_IF(result == false, false);

	// return) 
	return true;
}

bool service::request_continue_service(const server_admin::sID& _id)
{
	// 1) find register info
	auto pregister_info = m_pgroup_server->find_server(_id);

	// check) return false if failed
	RETURN_IF(pregister_info.empty(), true);

	// 2) request continue service
	auto result = pregister_info->request_continue_service();

	// check) 
	RETURN_IF(result == false, false);

	// return) 
	return true;
}

bool service::request_kill_service(const server_admin::sID& _id)
{
	// 1) find register info
	auto pregister_info = m_pgroup_server->find_server(_id);

	// check) return false if failed
	RETURN_IF(pregister_info.empty(), true);

	// 2) request kill service force
	auto result = pregister_info->request_kill_service();

	// check) 
	RETURN_IF(result == false, false);

	// return) 
	return true;
}

bool service::register_server_setting(server_entity* _pregister_info)
{
	// check) 
	CGASSERT_ERROR(_pregister_info != nullptr);

	// check) 
	THROW_IF(_pregister_info==nullptr, throwable::failure(eRESULT::INVALID_ARGUMENT));
	
	// declare) 
	object_ptr<server_entity> pfind_server_setting;

	scoped_lock(m_list_server_entity)
	{
		// 1) 같은 id로 등록된 것이 있는지 확인한다.
		auto iter = std::find_if(m_list_server_entity.begin(), m_list_server_entity.end(), [=](object_ptr<server_entity>& Iter)->bool { return Iter->info_server_register.server_setting.id.key == _pregister_info->info_server_register.server_setting.id.key;});

		// check) return if aleady exsiting
		RETURN_IF(iter != m_list_server_entity.end(), false);

		// 2) alloc new object id
		auto id_code = _alloc_object_id();

		// 3) set new object id to server setting info
		_pregister_info->info_server_register.server_setting.id.code = id_code;
	
		// 4) register server setting info
		m_list_server_entity.push_back(_pregister_info);
	}

	// 3) 등록에 성공했다면 등록된 설정을 저장한다.
	save_server_list();

	// log) 
	LOG_INFO << " -SERVER_INFO register '" << _pregister_info->info_server_register.server_setting.name << "'[server_id:" << _pregister_info->info_server_register.server_setting.id.key << "]";

	// notify)
	_notify_to_controls(server_admin::message_SERVER_ADMIN_SERVER_SETTING_REGISTER(_pregister_info->info_server_register));

	// return) 
	return true;
}

bool service::unregister_server_setting(server_entity* _pregister_info)
{
	scoped_lock(m_list_server_entity)
	{
		// 1) 먼저 이미 등록된 것이 있는지 찾는다.
		auto iter = std::find(m_list_server_entity.begin(), m_list_server_entity.end(), _pregister_info);

		// check) 찾지 못했으면 false를 리턴하고 끝낸다.
		RETURN_IF(iter == m_list_server_entity.end(), false);

		// 2) Socket을 Reset한다.
		(*iter)->psocket.reset();

		// 3) 제거한다.
		m_list_server_entity.erase(iter);
	}

	// 4) 변경된 설정을 저장한다.
	save_server_list();

	// log) 
	LOG_INFO << " -SERVER_INFO unregister '" << _pregister_info->info_server_register.server_setting.name << "'[server_id:" << _pregister_info->info_server_register.server_setting.id.key << "]";

	// 6) Server가 추가되었음을 notify한다.
	_notify_to_controls(server_admin::message_SERVER_ADMIN_SERVER_SETTING_UNREGISTER(m_info_host.hid, _pregister_info->info_server_register.server_setting.id));

	// return) 
	return true;
}

void service::unregister_server_setting_all()
{
	list<object_ptr<server_entity>>	list_server_register_info;

	scoped_lock(m_list_server_entity)
	{
		list_server_register_info.splice(list_server_register_info.begin(), m_list_server_entity);
	}

	// 1) 모두 unregister한다.
	for(auto& iter: list_server_register_info)
	{
		unregister_server_setting(iter);
	}
}

object_ptr<server_entity> service::update_server_setting(const server_admin::sSERVER_REGISTER& _server_register_info)
{
	// declare) 
	object_ptr<server_entity> pfind_server_setting;

	scoped_lock(m_list_server_entity)
	{
		// 1) 해당 객체를 찾는다.
		auto iterFind = std::find_if(m_list_server_entity.begin(), m_list_server_entity.end(), [=](object_ptr<server_entity>& Iter)->bool { return Iter->info_server_register.server_setting.id.key == _server_register_info.server_setting.id.key;});

		// check) 찾지 못했다면 false를 리턴하며 끝낸다.
		RETURN_IF(iterFind == m_list_server_entity.end(), pfind_server_setting);

		// 2) 찿은 객체 저장
		pfind_server_setting = *iterFind;

		// 3) 값을 설정한다.
		pfind_server_setting->set_server_register_info(_server_register_info);
	}

	// 4) Update된 설정을 저장한다.
	save_server_list();

	// log) 
	LOG_INFO << " -SERVER_INFO updated '" << _server_register_info.server_setting.name << "'[server_id:" << _server_register_info.server_setting.id.key << "]";

	// notify) Server가 Update되었음을 notify한다.
	_notify_to_controls(server_admin::message_SERVER_ADMIN_SERVER_INSTANCE_SETTING_UPDATE(m_info_host.hid, _server_register_info.server_setting.id, _server_register_info));

	// return) 
	return	pfind_server_setting;
}

object_ptr<server_entity> service::find_server_entity(uint64_t _id_select) const noexcept
{
	// declare)
	object_ptr<server_entity> pserver_entity;

	scoped_lock(m_list_server_entity)
	{
		for (const auto& iter : m_list_server_entity)
		{
			// - tr node name
			if (_id_select == reinterpret_cast<uint64_t>(iter.get()))
			{
				pserver_entity = iter;
				break;
			}
		}
	}

	// return) 
	return pserver_entity;
}

//bool service::request_version_update(const server_admin::sID& _i64SystemID)
//{
//	// log) 
//	LOG_INFO << " -VERSION update requested [server_id:" << _i64SystemID.key << "]";
//
//	// Decalre) 
//	object_ptr<server_entity> pregistered;
//
//	scoped_lock(m_list_server_entity)
//	{
//		// 1) 먼저 이미 등록된 것이 있는지 찾는다.
//		auto	iter = std::find_if(m_list_server_entity.begin(), m_list_server_entity.end(), [=](object_ptr<server_entity>& Iter)->bool { return Iter->info_server_register.server_setting.id.key == _i64SystemID.key;});
//
//		// check) 찾지 못했으면 false를 리턴하고 끝낸다.
//		RETURN_IF(iter == m_list_server_entity.end(), false);
//
//		// 2) Update를 요청한다.
//		pregistered = (*iter);
//	}
//
//	// check) 해당 등록 정보를 찾지 못했으면 그냥 끝낸다.
//	if (pregistered.empty())
//	{
//		// log) 
//		LOG_INFO << " -VERSION update rejected(not found)'[server_id:" << _i64SystemID.key << "]";
//
//		// return) 
//		return false;
//	}
//
//	// check) 접속중이면 Update를 할수 없다.
//	if (pregistered->psocket.exist())
//	{
//		// log) 
//		LOG_INFO << " -VERSION update rejected(in running)'[server_id:" << _i64SystemID.key << "]";
//
//		// return) 
//		return false;
//	}
//
//	// 3) Update Requested (Update를 요청한다. - 만약 CheckOut이 된 상태가 아니면 자동으로 Check Out을 먼저하며 버전이 다르면 Switch를 수행한다.)
//	pregistered->request_update();
//
//	// notify) Server가 Update되었음을 notify한다.
//	_notify_to_controls(server_admin::message_SERVER_ADMIN_SERVER_INSTANCE_SETTING_UPDATE(m_info_host.hid, pregistered->info_server_register.server_setting.id, pregistered->info_server_register));
//
//	// return) 
//	return true;
//}
//
//void service::set_admin_server_address(const std::string_view _address)
//{
//	// 1) 기존 Address와 동일한지를 확인하기 위한 작업을 수행한다.(다를 경우에만 재접속을 수행하기 위해...)
//	net::sockaddr addr_old(net::ip::make_address(m_address_admin_server));
//	net::sockaddr add_new(net::ip::make_address(_address));
//
//	// 임시) make_address에서 제대로 주소 안 만듬 - port가 반영되지 않음.
//	CGASSERT_ERROR(false);
//
//	// 2) Address값을 복사한다.
//	m_address_admin_server = _address;
//
//	// 3) save한다.
//	save_host_server_info();
//
//	// 4) 모두에게 전송한다.
//	m_pgroup_host_agent->send(server_admin::message_SERVER_ADMIN_SEND_ADMINSERVERADDRESS(_address));
//
//	// check) 새로 바꾼 주소가 같으면 재접속은 하지 않는다.
//	RETURN_IF(addr_old == add_new);
//
//	// check) 
//	RETURN_IF(m_psocket_to_admin_server.empty());
//
//	// 5) 기존 접속을 끊는다.
//	m_psocket_to_admin_server->disable_reconnect();
//	m_psocket_to_admin_server->closesocket();
//
//	// 6) 접속을 새로 한다.
//	m_psocket_to_admin_server->enable_reconnect();
//	m_psocket_to_admin_server->start(add_new);
//}

void GET_CONTEXT_VALUE(int32_t& _value, context& _context)
{
	//  check) 해당 Context가 없으면 그냥 끝낸다.
	RETURN_IF(_context.is_valid() == false);

	// 1) 값 저장.
	_value = (int32_t)_context;

}

void GET_CONTEXT_VALUE(uint32_t& _value, context& _context)
{
	//  check) 해당 Context가 없으면 그냥 끝낸다.
	RETURN_IF(_context.is_valid() == false);

	// 1) 값 저장.
	_value = (uint32_t)_context;

}

void GET_CONTEXT_VALUE(int64_t& _value, context& _context)
{
	//  check) 해당 Context가 없으면 그냥 끝낸다.
	RETURN_IF(_context.is_valid() == false);

	// 1) 값 저장.
	_value = (int64_t)_context;

}

void GET_CONTEXT_VALUE(uint64_t& _value, context& _context)
{
	//  check) 해당 Context가 없으면 그냥 끝낸다.
	RETURN_IF(_context.is_valid() == false);

	// 1) 값 저장.
	_value = (uint64_t)_context;

}

void GET_CONTEXT_VALUE(eSERVER_SYSTEM_TYPE& _value, context& _context)
{
	//  check) 해당 Context가 없으면 그냥 끝낸다.
	RETURN_IF(_context.is_valid() == false);

	// 1) 값 저장.
	_value = (eSERVER_SYSTEM_TYPE)(uint32_t)_context;
}

void GET_CONTEXT_VALUE(std::string& _value, context& _context)
{
	//  check) 해당 Context가 없으면 그냥 끝낸다.
	RETURN_IF(_context.is_valid() == false);

	// 1) 값 저장.
	_value = (std::string_view)_context;
}

void GET_CONTEXT_VALUE(shared_buffer& _buffer, context& _context)
{
	//  check) 해당 Context가 없으면 그냥 끝낸다.
	RETURN_IF(_context.is_valid() == false);

	// 1) 버퍼를 할당받는다.
	shared_buffer buf_temp = alloc_shared_buffer(65536);

	// 2) Buffer에 Append한다.
	buf_temp.append_text<char>((std::string_view)_context);
	_buffer.append<char>(0);

	// 3) Environment Block의 끝을 표시하기 위해 NULL을 한번더 추가한다.
	_buffer.append<char>(0);

	_buffer = buf_temp;
}

void GET_CONTEXT_COMMANDLINE(shared_buffer& _buffer, context& _context)
{
	//  check) 해당 Context가 없으면 그냥 끝낸다.
	RETURN_IF(_context.is_valid() == false);

	// 1) 버퍼를 할당받는다.
	auto buf_temp = alloc_shared_buffer(65536);

	// 2) Buffer에 Append한다.
	buf_temp.append_text<char>((std::string_view)_context);
	_buffer.append<char>(0);

	// 3) Environment Block의 끝을 표시하기 위해 NULL을 한번더 추가한다.
	_buffer.append<char>(0);

	_buffer = buf_temp;
}

void GET_CONTEXT_ENVIRONMENT(shared_buffer& _buffer, context& _context)
{
	//  check) 해당 Context가 없으면 그냥 끝낸다.
	RETURN_IF(_context.is_valid() == false);

	// 1) 버퍼를 할당받는다.
	auto buf_temp = alloc_shared_buffer(65536);

	// 2) Buffer에 Append한다.
	buf_temp.append_text<char>((std::string_view)_context);
	_buffer.append<char>(0);

	// 3) Environment Block의 끝을 표시하기 위해 NULL을 한번더 추가한다.
	_buffer.append<char>(0);

	_buffer = buf_temp;
}

void service::load_server_list()
{
	// log) 
	LOG_INFO << " - load Registered Server Info";

	// declare) 
	vector<object_ptr<server_entity>> vector_server_setting_info;

	// Definition) 
	#define	SETTING_SERVER_LIST	"host_agent_server_list.json"

	// declare) 
	context	context_server_list;
	
	// 1) SERVER_LIST를 읽어들인다.
	context_server_list.read_from_json_file(SETTING_SERVER_LIST);

	for(auto& iter : context_server_list.map_node)
	{
		// check) ...
		CONTINUE_IF(iter.second.is_valid() == false);

		// - server_setting 정보를 작성한다.
		server_admin::sSERVER_REGISTER info_server_register;

		info_server_register.server_setting.name = iter.first;
		GET_CONTEXT_VALUE				(info_server_register.server_setting.id.key,			iter.second["server_id"sv]);
		GET_CONTEXT_VALUE				(info_server_register.server_setting.id.code,			iter.second["server_code"sv]);
		GET_CONTEXT_VALUE				(info_server_register.server_setting.comment,			iter.second["comment"sv]);
		info_server_register.server_setting.hid	 = m_info_host.hid;
		GET_CONTEXT_VALUE				(info_server_register.server_setting.rank.group,		iter.second["RANK_Group"sv]);
		GET_CONTEXT_VALUE				(info_server_register.server_setting.rank.field,		iter.second["RANK_Field"sv]);
		GET_CONTEXT_VALUE				(info_server_register.server_setting.rank.role,			iter.second["RANK_Role"sv]);
		GET_CONTEXT_VALUE				(info_server_register.server_setting.rank.index,		iter.second["RANK_Index"sv]);
		GET_CONTEXT_VALUE				(info_server_register.server_setting.rank.attribute,	iter.second["RANK_Attribute"sv]);
		GET_CONTEXT_VALUE				(info_server_register.server_setting.buf_configure,		iter.second["configure"sv]);
		GET_CONTEXT_VALUE				(info_server_register.file_execute,						iter.second["execute_file"sv]);
		GET_CONTEXT_VALUE				(info_server_register.str_path,							iter.second["path"sv]);
		GET_CONTEXT_VALUE				(info_server_register.str_path_working,					iter.second["working_path"sv]);
		GET_CONTEXT_COMMANDLINE			(info_server_register.buf_command_line,					iter.second["command_line"sv]);
		GET_CONTEXT_ENVIRONMENT			(info_server_register.buf_environment,					iter.second["environment"sv]);
		GET_CONTEXT_VALUE				(info_server_register.svn_url,							iter.second["svn_url"sv]);
		GET_CONTEXT_VALUE				(info_server_register.svn_revision,						iter.second["SVN_revision"sv]);
		info_server_register.time_register	 = chrono::time::utc::now();
		info_server_register.time_update	 = info_server_register.time_register;
		info_server_register.is_registered	 = true;

		// log) 
		LOG_INFO << "    + add SERVER setting '"sv << info_server_register.server_setting.name << "' id["sv << info_server_register.server_setting.id.key << "]"sv;

		// - Server Info
		server_admin::sSERVER_STATUS	status_server;
		status_server.id							 = info_server_register.server_setting.id;
		status_server.pid					 = 0;
		status_server.usage_process			 = 0;
		status_server.usage_memory			 = 0;
		status_server.usage_network			 = 0;
		status_server.state					 = eOBJECT_STATE::NONE;

		// - 추가할 ServerRegisterSettingInfo를 설정한다.
		auto	pserver_entity_info			 = make_object<server_entity>();
		pserver_entity_info->info_server_register = std::move(info_server_register);
		pserver_entity_info->status_server	 = status_server;
		pserver_entity_info->handle_process	 = INVALID_PROCESS_ID;
		//pserver_entity_info->usage_system

		// - 추가
		vector_server_setting_info.push_back(pserver_entity_info);
	}

	// 2) 값을 설정한다.
	scoped_lock(m_list_server_entity)
	{
		for (auto& iter : vector_server_setting_info)
		{
			m_list_server_entity.push_back(iter);
		}
	}
}

void service::save_server_list()
{
	//// declare) 
	//TCHAR strINIFileName[256];

	//// 1) Ini FileName을 설정한다.
	//wsprintf(strINIFileName, TEXT("%s\\%s"), TEXT("."), TEXT(CONFIGURE_JSON_FILENAME));

	//scoped_lock(m_list_server_entity)
	//{
	//	// declare) 
	//	int		i = 0;

	//	// 2) 모든 디렉토리들을 저장한다.
	//	for(auto& iter:m_list_server_entity)
	//	{
	//		// declare) 
	//		TCHAR	strApp[MAX_STRING_LENGTH];
	//		TCHAR	str_value[MAX_STRING_LENGTH];

	//		// - Name을 만든다.
	//		wsprintf(strApp, TEXT("DIRECTORY%03d"), i);

	//		// - 데이터들을 써넣는다.
	//		wsprintf(str_value, L"%lld", iter->info_server_setting.id.id);		WritePrivateProfileString(strApp, TEXT("ID_ID"),	str_value, strINIFileName);
	//		wsprintf(str_value, L"%d", iter->info_server_setting.id.eType);		WritePrivateProfileString(strApp, TEXT("ID_TYPE"),	str_value, strINIFileName);
	//		WritePrivateProfileString(strApp, TEXT("NAME"),			iter->info_server_setting.name,			strINIFileName);
	//		WritePrivateProfileString(strApp, TEXT("COMMENT"),		iter->info_server_setting.strComment,			strINIFileName);

	//		WritePrivateProfileString(strApp, TEXT("EXECUTE_FILE"),	iter->info_server_setting.file_execute,		strINIFileName);
	//		WritePrivateProfileString(strApp, TEXT("PATH"),			iter->info_server_setting.str_path,			strINIFileName);
	//		WritePrivateProfileString(strApp, TEXT("PATH_WORKING"), iter->info_server_setting.str_path_working,		strINIFileName);
	//		WritePrivateProfileString(strApp, TEXT("DIRECTORY"),	iter->info_server_setting.str_path_working,		strINIFileName);
	//		WritePrivateProfileString(strApp, TEXT("URL"),			iter->info_server_setting.strURL,				strINIFileName);
	//		wsprintf(str_value, L"%d", iter->info_server_setting.iRevision);		WritePrivateProfileString(strApp, TEXT("REVISION"),	str_value, strINIFileName);

	//		wsprintf(str_value, L"%d", iter->buffer_parameters.len);				WritePrivateProfileString(strApp, TEXT("PARAMETERS_LENGTH"), str_value, strINIFileName);
	//		WritePrivateProfileStruct(strApp, TEXT("PARAMETERS"),	iter->buffer_parameters.buf, iter->buffer_parameters.len,	strINIFileName);

	//		// - Index 증가.
	//		++i;
	//	}

	//	// 3) 나머지 데이터는 지운다.
	//	for(; ; ++i)
	//	{
	//		// declare) 
	//		TCHAR	strKey[MAX_STRING_LENGTH];

	//		// - Name을 만든다.
	//		wsprintf(strKey, TEXT("DIRECTORY%03d"), i);

	//		// declare) 
	//		DWORD	dwResult;

	//		// - 해당 Data를 읽는다.
	//		dwResult	 = WritePrivateProfileString(TEXT("DIRECTORY"), strKey, nullptr, strINIFileName);

	//		// check) 
	//		BREAK_IF(dwResult!=0);
	//	}
	//}
}

void service::load_host_server_info()
{
	//-----------------------------------------------------------------
	// 1. Host Setting Info
	//-----------------------------------------------------------------
	// declare)
	context context_config;

	// 1) JSON파일을 읽어들인다.
	context_config.read_from_json_file("CONFIGURE_JSON_FILENAME"sv);

	// 2) Local IP를 얻어낸다.
	net::sockaddr addr_local;
	api::socket::get_instance().get_local_address_public(addr_local);

	// 3) AdminServer
	{
		// - get value
		auto context = context_config["adminserver"sv];

		// - ...
		if (context.is_valid() == true)
		{
			// - Address
			{
				// - host_id를 얻는다.
				auto context_nest = context_config["address"sv];

				// - ...
				if (context_nest.is_valid() == true)
				{
					m_address_admin_server = (const std::string&)context_nest;
				}
			}
		}
	}


	//-----------------------------------------------------------------
	// 2. Host Info
	//-----------------------------------------------------------------
	// 1) HOST id값을 설정한다.(HOST id는 sHOST_SETTING과 같아야 한다.)
	m_info_host.hid = m_setting_host.hid;

	// 2) Host Name을 읽어들인다.
	m_info_host.host_name = api::get_host_name();

	// 3) Host Address를 설정한다.
	m_info_host.addr_host = addr_local;

	// 4) Host Agent의 시작 시간
	m_info_host.time_start = chrono::time::utc::now();

	// 6) 등록되지 않은 서버는 바로 접속을 종료해 버릴 것인가 여부
	//m_is_disconnected_unregistered		 = GetPrivateProfileInt(TEXT("SETTING"), TEXT("DISCONNECT_UNREGISTERED"), FALSE, strINIFileName) != 0;

	// log) 
	LOG_INFO << " -host name: '" << m_setting_host.name << "'[id: " << m_setting_host.hid.mac.sll_mac << "]";

	// 7) Host Info가 변경되었음으로 통보한다.
	//transmit_message(sMESSAGE_NOTIFY(eMESSAGE_WINDOWS_NOTIFY_UPDATE, eMESSAGE_UPDATE_HOST_INFO, reinterpret_cast<uintptr_t>(this)));
}

void service::save_host_server_info()
{
	//// declare) 
	//TCHAR	strINIFileName[256];
	//TCHAR	str_value[256];

	//// 1) Ini FileName을 설정한다.
	//wsprintf(strINIFileName, TEXT("%s\\%s"), TEXT("."), TEXT(CONFIGURE_JSON_FILENAME));

	//// 2) System Info
	//__to_string(str_value, m_setting_host.id.key);							WritePrivateProfileString(TEXT("HOST"), TEXT("HOST_ID"), str_value, strINIFileName);
	//WritePrivateProfileString(TEXT("HOST"), TEXT("NAME"),					(TCHAR*)m_setting_host.name,			strINIFileName);
	//WritePrivateProfileString(TEXT("HOST"), TEXT("EXPLANATION"),			(TCHAR*)m_setting_host.strComment,		strINIFileName);
	//WritePrivateProfileString(TEXT("HOST"), TEXT("ADMINSERVER_ADDRESS"),	(TCHAR*)m_address_admin_server.c_str(),	strINIFileName);

	//// 3) Setting
	//__to_string(str_value, m_is_disconnected_unregistered);	WritePrivateProfileString(TEXT("SETTING"), TEXT("DISCONNECT_UNREGISTERED"), str_value, strINIFileName);
}

BEGIN_CGMESSAGE_MAP(service)
	ON_CGMESSAGE(eMESSAGE::WINDOWS::NOTIFY_UPDATE,						on_message_WINDOWS_NOTIFY_UPDATE,				sMESSAGE_NOTIFY)
	CGMESSAGE_TYPE_SUB_BUFFER_NETWORK
		ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_INIT_SERVICE,		on_message_SERVER_ADMIN_REQUEST_INIT_SERVICE,	sMESSAGE_NETWORK)
		ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_DESTROY_SERVICE,	on_message_SERVER_ADMIN_REQUEST_DESTROY_SERVICE,sMESSAGE_NETWORK)
		ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_START_SERVICE,	on_message_SERVER_ADMIN_REQUEST_START_SERVICE,	sMESSAGE_NETWORK)
		ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_STOP_SERVICE,		on_message_SERVER_ADMIN_REQUEST_STOP_SERVICE,	sMESSAGE_NETWORK)
	END_SUB_CGMESSAGE_MAP

	ON_CGMESSAGE(eMESSAGE::SERVER::TIME_EVENT::EVENT_REGISTER,			on_message_SERVER_TIME_EVENT_EVENT_REGISTER,	sMESSAGE)
	ON_CGMESSAGE(eMESSAGE::SERVER::TIME_EVENT::EVENT_UNREGISTER,		on_message_SERVER_TIME_EVENT_EVENT_UNREGISTER,	sMESSAGE)
	ON_CGMESSAGE(eMESSAGE::SERVER::TIME_EVENT::EVENT_UPDATE,			on_message_SERVER_TIME_EVENT_EVENT_UPDATE,		sMESSAGE)
	ON_CGMESSAGE(eMESSAGE::SERVER::TIME_EVENT::EVENT_EXECUTE,			on_message_SERVER_TIME_EVENT_EVENT_EXECUTE,		sMESSAGE)

	ON_CGMESSAGE(eMESSAGE::SYSTEM::LOG_,								on_message_LOG,									sMESSAGE_LOG);
END_CGMESSAGE_MAP

result_code service::process_message(sMESSAGE& _msg)
{
	// 1) Message Map을 처리한다.
	EXECUTE_CGMESSAGE_MAP(_msg);

	// return) 
	return eRESULT::BYPASS;
}

result_code service::on_message_WINDOWS_NOTIFY_UPDATE(sMESSAGE_NOTIFY& _msg)
{
	transmit_message(_msg);

	// return) 
	return eRESULT::DONE;
}

result_code service::on_message_SERVER_ADMIN_REQUEST_INIT_SERVICE(sMESSAGE_NETWORK& /*_msg*/)
{
	// 1) Service를 close한다.
	initialize();

	// return) 
	return eRESULT::DONE;
}

result_code service::on_message_SERVER_ADMIN_REQUEST_DESTROY_SERVICE(sMESSAGE_NETWORK& /*_msg*/)
{
	// 1) Service를 close한다.
	destroy();

	// return) 
	return eRESULT::DONW;
}

result_code service::on_message_SERVER_ADMIN_REQUEST_START_SERVICE(sMESSAGE_NETWORK& /*_msg*/)
{
	// 1) Service를 open한다.
	start();

	// return) 
	return eRESULT::DONW;
}

result_code service::on_message_SERVER_ADMIN_REQUEST_STOP_SERVICE(sMESSAGE_NETWORK& /*_msg*/)
{
	// 1) Service를 close한다.
	stop();

	// return) 
	return eRESULT::DONE;
}

void service::send_message_SYSTEM_STATUS_INFO()
{
	// 1) Buffer를 할당받는다.
	auto buf_send = alloc_shared_buffer(8192);

	// 2) Message를 쓴다...
	buf_send.append<uint32_t>();
	buf_send.append<uint32_t>(eMESSAGE::SERVER::ADMIN::SYSTEM_STATUS_INFO);

#if defined(_WIN32)
	auto qwMemoryTotal = m_status_host.status_system_resource.bytes_memory_physical_total;
	auto qwMemoryUsage = m_status_host.status_system_resource.bytes_memory_physical_usage;

	buf_send.append<server_admin::sHOST_ID> (m_info_host.hid);
	buf_send.append<uint32_t>(m_status_host.usage_processor);
	buf_send.append<int64_t>(qwMemoryUsage);
	buf_send.append<int64_t>(qwMemoryTotal);

	// 3) Server List를 붙인다.
	scoped_lock(m_list_server_entity)
	{
		// - Server의 갯수를 붙인다.
		int	 i = 0;
		int& count = buf_send.append<int>();

		// - 모든 Server의 정보를 붙인다.
		for(auto& iter: m_list_server_entity)
		{
			if(iter->psocket.exist())
			{
				buf_send.append<int64_t>(iter->info_server_register.server_setting.id.key);
				buf_send.append<uint64_t>(iter->status_server.usage_process);
				buf_send.append<uint64_t>(iter->status_server.usage_memory);
				buf_send.append<uint64_t>(iter->status_server.usage_network);
				++i;
			}
		}

		count = i;
	}
#elif defined(__linux__)
#else
	#error "CGDK Not supported platform"
#endif

	// 4) 최종적으로 길이를 써넣는다.
	buf_send.front<uint32_t>() = buf_send.size<uint32_t>();

	// 5) 전송.
	m_pgroup_host_agent->send(buf_send);
	//m_psocket_to_admin_server->send(buf_send);
}

result_code service::on_message_SERVER_TIME_EVENT_EVENT_REGISTER(sMESSAGE& /*_msg*/)
{
	//// 1) notify to group websocket
	//m_pgroup_websocket->m_pobservable_websocket_host_info->process_time_event_add(_msg.get_source<time_event::Ientity>());

	// return) 
	return eRESULT::DONE;
}

result_code service::on_message_SERVER_TIME_EVENT_EVENT_UNREGISTER(sMESSAGE& /*_msg*/)
{
	//// 1) notify to group websocket
	//m_pgroup_websocket->m_pobservable_websocket_host_info->process_time_event_remove(_msg.get_source<time_event::Ientity>());

	// return) 
	return eRESULT::DONE;
}

result_code service::on_message_SERVER_TIME_EVENT_EVENT_EXECUTE(sMESSAGE& /*_msg*/)
{
	//// 1) notify to group websocket
	//m_pgroup_websocket->m_pobservable_websocket_host_info->process_time_event_execute(_msg.get_source<time_event::Ientity>());

	// return) 
	return eRESULT::DONE;
}

result_code service::on_message_SERVER_TIME_EVENT_EVENT_UPDATE(sMESSAGE& /*_msg*/)
{
	//// 1) notify to group websocket
	//m_pgroup_websocket->m_pobservable_websocket_host_info->process_time_event_update(_msg.get_source<time_event::Ientity>());

	// return) 
	return eRESULT::DONE;
}

result_code service::on_message_LOG(sMESSAGE_LOG& _msg)
{
	// 1) Log Record를 얻는다.
	auto plog_record = _msg.plog_record;

	// check) plog_record가 nullptr이면 여기서 끝낸다.
	RETURN_IF(plog_record.empty(), eRESULT::DONE);

	// check)
	RETURN_IF(m_pgroup_host_agent.empty(), eRESULT::DONE);

	// 2) 전송한다.
	m_pgroup_host_agent->send(server_admin::message_SYSTEM_LOG(plog_record));

	// return) 
	return eRESULT::DONE;
}

void service::on_execute()
{
	//// check)
	//RETURN_IF(get_object_state() != eOBJECT_STATE::RUNNING);

	//// 1) System Status Info를 얻는다.
	//update_system_status_info();

	//// 2) System 정보를 Update한다.
	//m_pgroup_server->update_server_status_info();
}

//void service::update_system_status_info()
//{
//	// 1) HostID와 HostCode...
//	m_status_host.hid = m_info_host.hid;
//
//	// 2) Update한다...
//	api::manager_perfornamce_query::get_instance()->update_query();
//
//#if defined(_WIN32)
//	PERFORMANCE_INFORMATION performance_info;
//
//	// 3) Memory 상태를 얻는다.
//	GetPerformanceInfo(&performance_info, sizeof(PERFORMANCE_INFORMATION));
//
//	m_status_host.status_system_resource.bytes_memory_physical_total = ((uint64_t)performance_info.PhysicalTotal) * ((uint64_t)performance_info.PageSize);
//	m_status_host.status_system_resource.bytes_memory_physical_usage = m_status_host.status_system_resource.bytes_memory_physical_total - ((uint64_t)performance_info.PhysicalAvailable) * ((uint64_t)performance_info.PageSize);
//	m_status_host.status_system_resource.count_handle = performance_info.HandleCount;
//	m_status_host.status_system_resource.count_process = performance_info.ProcessCount;
//	m_status_host.status_system_resource.count_thread = performance_info.ThreadCount;
//
//	// 4) CPU 사용률 구하기
//	if(m_perfornamce_processor.exist())
//	{
//		m_status_host.usage_processor = m_perfornamce_processor->GetValueLONG();
//	}
//	// 5) Network 사용률구하기
//	m_status_host.usage_network = 0;
//
//#elif defined(__linux__)
//#else
//	#error "CGDK Not supported platform"
//#endif
//
//	// 6) Update한 정보를 전송한다.
//	_notify_to_controls(message_SERVER_ADMIN_HOST_STATUS_UPDATE());
//}

shared_buffer service::message_SERVER_ADMIN_SEND_ADMINSERVERADDRESS()
{
	scoped_lock(m_list_server_entity)
	{
		return server_admin::message_SERVER_ADMIN_SEND_ADMINSERVERADDRESS(m_address_admin_server);
	}
}

shared_buffer service::message_SERVER_ADMIN_SEND_HOSTINFO()
{
	scoped_lock(m_list_server_entity)
	{
		return server_admin::message_SERVER_ADMIN_SEND_HOSTINFO(get_host_info());
	}
}

shared_buffer service::message_SERVER_ADMIN_SEND_HOST_SETTING()
{
	scoped_lock(m_list_server_entity)
	{
		return server_admin::message_SERVER_ADMIN_SEND_HOST_SETTING(get_host_setting());
	}
}

shared_buffer service::message_SERVER_ADMIN_HOST_STATUS_UPDATE()
{
	scoped_lock(m_list_server_entity)
	{
		return server_admin::message_SERVER_ADMIN_HOST_STATUS_UPDATE(m_status_host);
	}
}

shared_buffer service::message_SERVER_ADMIN_SERVER_SETTING_LIST()
{
	// declare)
	shared_buffer buf_send;

	scoped_lock(m_list_server_entity)
	{
		// 1) 등록된 갯수
		auto count_registered = (int)m_list_server_entity.size();

		// 2) 메시지의 크기를 계산한다.
		auto size_message = 8 + sizeof(server_admin::sHOST_ID) + sizeof(int);

		// 3) List server_settingInfo의 크기를 계산한다.
		for(auto& iter: m_list_server_entity)
		{
			size_message += get_size_of(iter->info_server_register);
		}

		// 4) Buffer를 할당받는다.
		buf_send = alloc_shared_buffer(size_message);

		// 5) Message를 쓴다.
		buf_send.append<uint32_t>();
		buf_send.append<uint32_t>(eMESSAGE::SERVER::ADMIN::SERVER_SETTING_LIST);

		// 6) Host ID를 설정한다.
		buf_send.append<server_admin::sHOST_ID>(m_info_host.hid);

		// 7) 등록된 서버수를 써넣는다.
		buf_send.append<int>(count_registered);

		// 8) 등록된 서버 정보들을 써넣는다.
		for(auto& iter: m_list_server_entity)
		{
			// - ServerInf를 붙인다.
			buf_send.append<server_admin::sSERVER_REGISTER>(iter->info_server_register);
		}

		// 9) Message의 크기를 설정한다.
		buf_send.front<uint32_t>() = buf_send.size<uint32_t>();
	}

	// return)
	return buf_send;
}
