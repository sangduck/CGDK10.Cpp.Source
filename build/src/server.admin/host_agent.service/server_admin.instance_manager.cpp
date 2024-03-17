#include "pch.h"


#if defined(__linux__) && !defined(__ANDROID__)
#include <sys/sysinfo.h>
#endif

std::atomic<uint64_t> server_admin::instance_manager::m_id_alloc = 0;

#define	LOG_FILE	"host_agent.log"sv

own_ptr<server_admin::instance_host>	server_admin::instance_manager::m_instance_host_self;
object_ptr<api::performance_counter>	server_admin::instance_manager::m_perfornamce_processor;
std::string								server_admin::instance_manager::m_address_admin_server;

uint64_t server_admin::instance_manager::_alloc_object_id()
{
	return	m_id_alloc++;
}

object_ptr<server_admin::Iinstance> server_admin::instance_manager::register_instance(own_ptr<Iinstance>&& _pinstance)
{
	// check)
	CGASSERT_ERROR(_pinstance.exist());

	// check)
	CGASSERT_ERROR(_pinstance->get_iid() != 0);

	// check)
	THROW_IF(_pinstance->get_iid() == 0, throwable::failure(eRESULT::INVALID_REQUEST));

	// declare)
	object_ptr<server_admin::Iinstance> pinstance;

	// 1)
	switch (_pinstance->get_instance_type())
	{
	case	eINSTANCE_TYPE::HOST:
			pinstance = this->process_register_instance_host(std::move(_pinstance));
			break;

	case	eINSTANCE_TYPE::SERVICE:
			pinstance = this->process_register_instance_service(std::move(_pinstance));
			break;

	default:
			throw throwable::failure(eRESULT::INVALID_REQUEST);
	}

	// return) 
	return pinstance;
}

bool server_admin::instance_manager::unregister_instance(server_admin::iid_t _iid, socket_peer* _psocket) noexcept
{
	// declare) 
	object_ptr<Iinstance> pinstance;

	// declare)
	bool is_not_removed = false;

	scoped_lock(this->m_cs_instance)
	{
		// 1) 먼저 이미 등록된 것이 있는지 찾는다.
		auto iter = this->m_map_instance.find(_iid);

		// check) 찾지 못했으면 false를 리턴하고 끝낸다.
		RETURN_IF(iter == this->m_map_instance.end(), false);

		// - move-out instance (for delayed destroy)
		pinstance = iter->second;

		// check)
		CGASSERT_ERROR(pinstance.exist());

		// check)
		RETURN_IF(pinstance.empty(), false);

		// check)
		RETURN_IF(pinstance->get_iid() != _iid, false);

		// 3) local setting이면 지우지 않는다.
		is_not_removed = pinstance->process_sservice_disconnected(_psocket);

		if (is_not_removed == false)
		{
			// - erase ins
			this->m_map_instance.erase(iter);
		}
	}

	// 6) Server가 추가되었음을 notify한다.
		// 3) 제거한다.
	if (is_not_removed == true)
	{
		// - notify
		//service::pgroup_client->send(server_admin::message_SERVER_ADMIN_INSTANCE_UPDATE(_iid, pinstance));
		service::pgroup_websocket->notify_to_web_controls_INSTANCE_STATUS_UPDATE(pinstance);

		// log) 
		LOG_WARNING << "  - INSTANCE('"sv << pinstance->get_name() << "') (!) instance disconnected['instance id':"sv << _iid << "]"sv;
	}
	else
	{
		// - notify
		service::pgroup_client->send(server_admin::message_SERVER_ADMIN_INSTANCE_UNREGISTER(_iid));
		service::pgroup_websocket->notify_to_web_controls_INSTANCE_LEAVE(pinstance);

		// log) 
		LOG_INFO << "  - INSTANCE('"sv << pinstance->get_name() << "') < unregister ['instance id':"sv << _iid << "]"sv;
	}

	// return
	return true;
}

void server_admin::instance_manager::unregister_instance_all() noexcept
{
	// declare) 
	std::vector<server_admin::iid_t> vector_instance_iid;

	// 1) get..
	scoped_lock(this->m_cs_instance)
	{
		for (auto& iter : this->m_map_instance)
			vector_instance_iid.push_back(iter.first);
	}

	// 2) 모두 unregister한다.
	for (auto& iter : vector_instance_iid)
		this->unregister_instance(iter, nullptr);
}

//object_ptr<server_admin::instance_Iservice> server_admin::instance_manager::update_server_setting(const server_admin::sSERVICE_INSTANCE_INFO& _server_register_info)
//{
//	// declare) 
//	object_ptr<server_admin::instance_Iservice> pfind_server_setting;
//
//	scoped_lock(this->m_cs_instance)
//	{
//		// 1) 해당 객체를 찾는다.
//		auto iter_find = std::find_if(this->m_list_server_instance.begin(), this->m_list_server_instance.end(), [=](object_ptr<server_admin::instance_Iservice>& iter)->bool { return iter->m_service_instance.server_info.registered.iid == _server_register_info.iid; });
//d
//		// check) 찾지 못했다면 false를 리턴하며 끝낸다.
//		RETURN_IF(iter_find == this->m_list_server_instance.end(), pfind_server_setting);
//
//		// 2) 찿은 객체 저장
//		pfind_server_setting = *iter_find;
//
//		// 3) 값을 설정한다.
//		pfind_server_setting->set_server_register_info(_server_register_info);
//	}
//
//	// 4) Update된 설정을 저장한다.
//	this->save_server_list();
//
//	// log) 
//	LOG_INFO << " - SERVER_REGISTER info updated '"sv << _server_register_info.setting.nickname << "'[server_id:"sv << _server_register_info.iid << "]"sv;
//
//	// notify) Server가 Update되었음을 notify한다.
//	service::pgroup_client->send(server_admin::message_SERVER_ADMIN_SERVER_INSTANCE_SETTING_UPDATE(m_host_instance_info.host_iid, _server_register_info.iid, _server_register_info));
//
//	// return) 
//	return pfind_server_setting;
//}

object_ptr<server_admin::Iinstance> server_admin::instance_manager::find_instance(server_admin::iid_t _iid) const noexcept
{
	// declare) 
	object_ptr<server_admin::Iinstance> pinstance;

	scoped_lock(this->m_cs_instance)
	{
		// 1) find host
		auto iter_find = this->m_map_instance.find(_iid);

		// check)
		RETURN_IF(iter_find == this->m_map_instance.end(), pinstance);

		// 2) 
		pinstance = iter_find->second;
	}

	// return)
	return pinstance;
}

object_ptr<server_admin::Iinstance> server_admin::instance_manager::find_instance_by_pid(uint64_t _pid) const noexcept
{
	// declare) 
	object_ptr<server_admin::instance_Iservice> pfind;

	scoped_lock(this->m_cs_instance)
	{
		for (auto& iter : this->m_map_instance)
		{
			// chekc) 
			CONTINUE_IF(iter.second->get_instance_type() != eINSTANCE_TYPE::HOST);

			// - 
			auto pinstance_service = dynamic_pointer_cast<instance_Iservice>(iter.second);

			// check)
			BREAK_IF(pinstance_service.empty());

			if (pinstance_service->m_instance.status.pid == _pid)
			{
				pfind = pinstance_service;
				break;
			}
		}
	}

	// return)
	return pfind;
}

object_ptr<server_admin::Iinstance> server_admin::instance_manager::find_instance_by_pvoid(void* _id_select) const noexcept
{
	// declare)
	object_ptr<server_admin::Iinstance> pinstance;

	scoped_lock(this->m_cs_instance)
	{
		for (const auto& iter : this->m_map_instance)
		{
			// - tr node name
			if (iter.second.get() == _id_select)
			{
				pinstance = iter.second;
				break;
			}
		}
	}

	// return) 
	return pinstance;
}



void server_admin::instance_manager::on_initialize(context& _context)
{
	// log) 
	LOG_INFO << " - create SERVICE_ENTITY manager"sv;

	//-----------------------------------------------------------------
	// 2. read host info
	//-----------------------------------------------------------------
	// definition) 
#if defined(_WIN32)
	#define	SETTING_FILE_PATH	"./"sv
	#define	SETTING_FILE_NAME	"host_agent_setting.json"sv
#elif defined(__linux__)
	#define	SETTING_FILE_PATH	"/home/sangduck/projects/CGDK10.C++/src/server.admin/host_agent.service/"sv
	#define	SETTING_FILE_NAME	"host_agent_setting.json"sv
#endif

	// 1) set default value
	_context["name"sv] = "CGCII host_agent"sv;
	_context["log_file"sv]["path"sv] = SETTING_FILE_PATH;
	_context["log_file"sv]["filename"sv] = LOG_FILE;

	// 2) read setting json file
	_context.read_from_json_file(api::file::make_full_path(SETTING_FILE_PATH, SETTING_FILE_NAME));


	//-----------------------------------------------------------------
	// 2. ready instance manager
	//-----------------------------------------------------------------
	// 1) Register Mediator한다.
	this->register_messageable(this);

	// 2) processor...
	this->m_perfornamce_processor = make_object<api::performance_counter>("\\Processor(_Total)\\% Processor Time"sv);

	//-----------------------------------------------------------------
	// 3. read host setting info
	//-----------------------------------------------------------------
	// 1) load host list
	this->process_load_host_server_info();

	// trace) 
	LOG_SYSTEM << "-----------------------------------------------------------------"sv;
	LOG_SYSTEM << ""sv;
	LOG_SYSTEM << log_level_t::important() << "                    CGCII Network HOSTAGENT"sv;
	LOG_SYSTEM << ""sv;
	LOG_SYSTEM << "  @CGDK: "sv << CGDK_CONFIGURATION;
	LOG_SYSTEM << "  @compile: "sv << __DATE__ << " "sv << __TIME__;
	LOG_SYSTEM << ""sv;
	LOG_SYSTEM << "-----------------------------------------------------------------"sv;
}

void server_admin::instance_manager::on_destroy()
{
	// log) 
	LOG_INFO << " - destroy SERVICE_ENTITY manager"sv;

	// 1) write host server info to json file
	this->process_save_host_server_info();

	// 2) processor...
	this->m_perfornamce_processor.reset();

	// 3) Messageable을 unregister한다.
	this->unregister_messageable(this);
}

void server_admin::instance_manager::on_start(context& /*_context*/)
{
	// declare)
	auto& info = server_admin::instance_manager::self()->m_instance.info;
	auto& setting = server_admin::instance_manager::self()->m_instance.setting;

	// log) 
	LOG_INFO << " - HOST.name: "sv << server_admin::instance_manager::self()->m_instance.info.host_name;
	LOG_INFO << " - HOST.id: "sv << info.mac.sll_addr[0] << ":"sv << info.mac.sll_addr[1] << ":"sv << info.mac.sll_addr[2] << ":"sv << info.mac.sll_addr[3] << ":"sv << info.mac.sll_addr[4] << ":"sv << info.mac.sll_addr[5] << "."sv << setting.iid;
	LOG_INFO << " - HOST.infoSys: core count="sv << info.system_info.processor.number_of_processor;
	LOG_INFO << " - HOST.info_os: "sv << to_string<char>(info.os_info.type) << " "sv << info.os_info.version.major << "."sv << info.os_info.version.minor << "."sv << info.os_info.version.build;
	LOG_INFO << " - HOST.uidHost: "sv << info.uid_host;
	LOG_INFO << " - HOST.uidOS: "sv << info.uid_os;

	// 1) register message medaitor
	time_event::entity_manager::instance()->register_messageable(this);

	// 2) registered Server Info占쏙옙 占싻억옙쨈占?
	this->process_load_server_list();

	//// test) add event for test
	//{
	//	// 1) time_event 객체를 생성한다.
	//	auto pentity_a = make_object<foo_time_event_iteration>();

	//	// 2) setting
	//	pentity_a->set_name("test_entity");
	//	pentity_a->set_event_time(chrono::time::utc::now() + 1s);
	//	pentity_a->set_event_interval(10s);
	//	pentity_a->set_event_times(1000);

	//	// 3) start
	//	pentity_a->start();



	//	// 1) time_event 객체를 생성한다.
	//	auto pentity_b = make_object<foo_time_event_start_end>();

	//	// 2) setting
	//	pentity_b->set_name("test_start_end");
	//	pentity_b->set_event_start_time(chrono::time::utc::now() + 5s);
	//	pentity_b->set_event_end_time(chrono::time::utc::now() + 10s);

	//	// 3) start
	//	pentity_b->start();
	//}

	// 1) set executing interval
	this->execute_interval(chrono::seconds(5));

	// 2) start executing 
	system_executor::register_schedulable(this);




	//{
	//	auto p = m_map_instance.begin()->second.get_object_ptr();

	//	auto p_s = dynamic_cast<instance_Iservice*>(p.get());

	//	p_s->request_service_initialize();
	//}


}

void server_admin::instance_manager::on_stop()
{
	// 1) unregister message medaitor
	time_event::entity_manager::instance()->unregister_messageable(this);

	// 1) stop excution (unregister from schedulable manager)
	system_executor::unregister_schedulable(this);

	// 2) save server list
	this->process_save_server_list();

	// 3) unregister all server info
	this->unregister_instance_all();
}

result_code server_admin::instance_manager::on_message(sMESSAGE_NETWORK& _msg)
{
	BEGIN_NESTED_CGMESSAGE_MAP
		ON_CGMESSAGE(eMESSAGE::WINDOWS::NOTIFY_UPDATE, on_message_WINDOWS_NOTIFY_UPDATE, sMESSAGE_NOTIFY)

		CGMESSAGE_TYPE_SUB_BUFFER_NETWORK
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_INIT_SERVICE, on_message_SERVER_ADMIN_REQUEST_INIT_SERVICE, sMESSAGE_NETWORK)
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_DESTROY_SERVICE, on_message_SERVER_ADMIN_REQUEST_DESTROY_SERVICE, sMESSAGE_NETWORK)
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_START_SERVICE, on_message_SERVER_ADMIN_REQUEST_START_SERVICE, sMESSAGE_NETWORK)
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_STOP_SERVICE, on_message_SERVER_ADMIN_REQUEST_STOP_SERVICE, sMESSAGE_NETWORK)
		END_SUB_CGMESSAGE_MAP

		ON_CGMESSAGE(eMESSAGE::SERVER::TIME_EVENT::EVENT_REGISTER, on_message_SERVER_TIME_EVENT_EVENT_REGISTER, sMESSAGE)
		ON_CGMESSAGE(eMESSAGE::SERVER::TIME_EVENT::EVENT_UNREGISTER, on_message_SERVER_TIME_EVENT_EVENT_UNREGISTER, sMESSAGE)
		ON_CGMESSAGE(eMESSAGE::SERVER::TIME_EVENT::EVENT_UPDATE, on_message_SERVER_TIME_EVENT_EVENT_UPDATE, sMESSAGE)
		ON_CGMESSAGE(eMESSAGE::SERVER::TIME_EVENT::EVENT_EXECUTE, on_message_SERVER_TIME_EVENT_EVENT_EXECUTE, sMESSAGE)

		ON_CGMESSAGE(eMESSAGE::SYSTEM::LOG_, on_message_SYSTEM_LOG, sMESSAGE_LOG);
	END_NESTED_CGMESSAGE_MAP

	// return) 
	return	eRESULT::BYPASS;
}

//bool server_admin::instance_manager::request_version_update(server_admin::sRANK _i64SystemID)
//{
//	// log) 
//	LOG_INFO << " - VERSION update requested [server_id:"sv << _i64SystemID.key << "]"sv;
//
//	// Decalre) 
//	object_ptr<server_admin::instance_Iservice> pregistered;
//
//	scoped_lock(this->m_cs_instance)
//	{
//		// 1) 먼저 이미 등록된 것이 있는지 찾는다.
//		auto iter = std::find_if(this->m_map_instance_service.begin(), this->m_map_instance_service.end(), [=](auto& iter)->bool { return iter.second->m_service_instance.server_info.registered.iid == _i64SystemID.key; });
//
//		// check) 찾지 못했으면 false를 리턴하고 끝낸다.
//		RETURN_IF(iter == this->m_map_instance_service.end(), false);
//
//		// 2) Update를 요청한다.
//		pregistered = (*iter);
//	}
//
//	// check) 해당 등록 정보를 찾지 못했으면 그냥 끝낸다.
//	if (pregistered.empty())
//	{
//		// log) 
//		LOG_INFO << " - VERSION update rejected(not found)'[server_id:"sv << _i64SystemID.key << "]"sv;
//
//		// return) 
//		return false;
//	}
//
//	// check) 접속중이면 Update를 할수 없다.
//	if (pregistered->psocket.exist())
//	{
//		// log) 
//		LOG_INFO << " - VERSION update rejected(in running)'[server_id:"sv << _i64SystemID.key << "]"sv;
//
//		// return) 
//		return false;
//	}
//
//	// 3) Update Requested (Update를 요청한다. - 만약 CheckOut이 된 상태가 아니면 자동으로 Check Out을 먼저하며 버전이 다르면 Switch를 수행한다.)
//	pregistered->request_update();
//
//	// notify) Server가 Update되었음을 notify한다.
//	service::pgroup_client->send(server_admin::message_SERVER_ADMIN_SERVER_INSTANCE_SETTING_UPDATE(m_host_instance_info.host_iid, pregistered->m_service_instance.server_info.registered.iid, pregistered->m_service_instance.server_info.registered));
//
//	// return) 
//	return true;
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
	_value = (std::string)_context;
}

void GET_CONTEXT_VALUE(server_admin::sRANK& _value, context& _context)
{
	//  check) 해당 Context가 없으면 그냥 끝낸다.
	RETURN_IF(_context.is_valid() == false);

	// 1) get value
	GET_CONTEXT_VALUE(_value.group, _context["group"sv]);
	GET_CONTEXT_VALUE(_value.field, _context["field"sv]);
	GET_CONTEXT_VALUE(_value.role,  _context["role"sv]);
	GET_CONTEXT_VALUE(_value.index, _context["index"sv]);
}

void GET_CONTEXT_VALUE(shared_buffer& _buffer, context& _context)
{
	//  check) 해당 Context가 없으면 그냥 끝낸다.
	RETURN_IF(_context.is_valid() == false);

	// 1) 버퍼를 할당받는다.
	shared_buffer buf_temp = alloc_shared_buffer(65536);

	// 2) Buffer에 Append한다.
	buf_temp.append_text<char>((std::string)_context);
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
	buf_temp.append_text<char>((std::string)_context);
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
	buf_temp.append_text<char>((std::string)_context);
	_buffer.append<char>(0);

	// 3) Environment Block의 끝을 표시하기 위해 NULL을 한번더 추가한다.
	_buffer.append<char>(0);

	_buffer = buf_temp;
}

void server_admin::instance_manager::process_load_server_list()
{
	// log) 
	LOG_INFO << " - load Registered Server Info"sv;

	// declare) 
	std::vector<own_ptr<server_admin::instance_Iservice>> vector_server_setting_info;

	// Definition) 
	#define	SETTING_SERVER_LIST	"host_agent_server_list.json"

	// declare) 
	context	context_server_list;

	// 1) SERVICE_INSTANCE_LIST를 읽어들인다.
	context_server_list.read_from_json_file(SETTING_SERVER_LIST);

	for (auto& iter : context_server_list.map_node)
	{
		// check) ...
		CONTINUE_IF(iter.second.is_valid() == false);

		// - setting 정보를 작성한다.
		server_admin::sSERVICE_SETTING setting;
		server_admin::sSERVICE_INSTANCE_INFO info;

		setting.nickname = iter.first;
		GET_CONTEXT_VALUE(setting.iid, iter.second["iid"sv]);
		GET_CONTEXT_VALUE(setting.comment, iter.second["comment"sv]);
		info.host_iid = this->m_instance_host_self->m_instance.setting.iid;
		GET_CONTEXT_VALUE(setting.rank, iter.second["rank"sv]);
		//GET_CONTEXT_VALUE(setting.buf_configure, iter.second["configure"sv]);
		GET_CONTEXT_VALUE(setting.file_execute, iter.second["execute_file"sv]);
		GET_CONTEXT_VALUE(setting.str_path, iter.second["path"sv]);
		GET_CONTEXT_VALUE(setting.str_path_working, iter.second["working_path"sv]);
		GET_CONTEXT_COMMANDLINE(setting.buf_command_line, iter.second["command_line"sv]);
		GET_CONTEXT_ENVIRONMENT(setting.buf_environment, iter.second["environment"sv]);
		GET_CONTEXT_VALUE(setting.revision.url, iter.second["revision_url"sv]);
		GET_CONTEXT_VALUE(setting.revision.revision, iter.second["revision_revision"sv]);
		info.time_register = chrono::time::utc::now();
		info.time_update = info.time_register;

		// log) 
		LOG_INFO << "    + add SERVER setting '"sv << setting.nickname << "' id["sv << setting.iid << "]"sv;

		// - s`erver Info
		server_admin::sSERVICE_INSTANCE_STATUS proces_status;
		proces_status.iid = setting.iid;
		proces_status.state = eOBJECT_STATE::NONE;

		// - 추가할 ServerRegisterSettingInfo를 설정한다.
		auto pservice_entity_info = make_own<server_admin::instance_service_local>();
		pservice_entity_info->m_instance.setting = std::move(setting);
		pservice_entity_info->m_instance.info = std::move(info);
		pservice_entity_info->m_instance.status = std::move(proces_status);

		// check)
		CGASSERT_ERROR(setting.iid != 0);

		// - 추가
		vector_server_setting_info.push_back(std::move(pservice_entity_info));
	}

	// 2) 값을 설정한다.
	for (auto& iter : vector_server_setting_info)
	{
		this->register_instance(std::move(iter));
	}
}

void server_admin::instance_manager::process_save_server_list()
{
	//// declare) 
	//TCHAR strINIFileName[256];

	//// 1) Ini FileName을 설정한다.
	//wsprintf(strINIFileName, TEXT("%s\\%s"), TEXT("."), TEXT(CONFIGURE_JSON_FILENAME));

	//scoped_lock(this->m_cs_instance)
	//{
	//	// declare) 
	//	int i = 0;

	//	// 2) 모든 디렉토리들을 저장한다.
	//	for(auto& iter:this->m_map_instance)
	//	{
	//		// declare) 
	//		TCHAR strApp[MAX_STRING_LENGTH];
	//		TCHAR str_value[MAX_STRING_LENGTH];

	//		auto pinstance = iter.second.get_object_ptr();

	//		// - Name을 만든다.
	//		wsprintf(strApp, TEXT("DIRECTORY%03d"), i);

	//		// - 데이터들을 써넣는다.
	//		wsprintf(str_value, L"%lld", pinstance->info_server_setting.id.id); WritePrivateProfileString(strApp, TEXT("ID_ID"), str_value, strINIFileName);
	//		wsprintf(str_value, L"%d", pinstance->info_server_setting.id.eType); WritePrivateProfileString(strApp, TEXT("ID_TYPE"), str_value, strINIFileName);
	//		WritePrivateProfileString(strApp, TEXT("NAME"), pinstance->info_server_setting.name, strINIFileName);
	//		WritePrivateProfileString(strApp, TEXT("COMMENT"), pinstance->info_server_setting.strComment, strINIFileName);

	//		WritePrivateProfileString(strApp, TEXT("EXECUTE_FILE"), pinstance->info_server_setting.file_execute, strINIFileName);
	//		WritePrivateProfileString(strApp, TEXT("PATH"), pinstance->info_server_setting.str_path, strINIFileName);
	//		WritePrivateProfileString(strApp, TEXT("PATH_WORKING"), pinstance->info_server_setting.str_path_working, strINIFileName);
	//		WritePrivateProfileString(strApp, TEXT("DIRECTORY"), pinstance->info_server_setting.str_path_working, strINIFileName);
	//		WritePrivateProfileString(strApp, TEXT("URL"), pinstance->info_server_setting.strURL, strINIFileName);
	//		wsprintf(str_value, L"%d", pinstance->info_server_setting.iRevision); WritePrivateProfileString(strApp, TEXT("REVISION"), str_value, strINIFileName);

	//		wsprintf(str_value, L"%d", pinstance->bufferParameters.len); WritePrivateProfileString(strApp, TEXT("PARAMETERS_LENGTH"), str_value, strINIFileName);
	//		WritePrivateProfileStruct(strApp, TEXT("PARAMETERS"), pinstance->bufferParameters.buf, pinstance->bufferParameters.len, strINIFileName);

	//		// - Index 증가.
	//		++i;
	//	}

	//	// 3) 나머지 데이터는 지운다.
	//	for(; ; ++i)
	//	{
	//		// declare) 
	//		TCHAR strKey[MAX_STRING_LENGTH];

	//		// - Name을 만든다.
	//		wsprintf(strKey, TEXT("DIRECTORY%03d"), i);

	//		// - 해당 Data를 읽는다.
	//		DWORD dwResult = WritePrivateProfileString(TEXT("DIRECTORY"), strKey, nullptr, strINIFileName);

	//		// check) 
	//		BREAK_IF(dwResult != 0);
	//	}
	//}
}

void server_admin::instance_manager::process_load_host_server_info()
{
	//-----------------------------------------------------------------
	// 1. Host Setting Info
	//-----------------------------------------------------------------
	// declare)
	context context_config;

	// 1) JSON파일을 읽어들인다.
	context_config.read_from_json_file("CONFIGURE_JSON_FILENAME"sv);

	// 3) AdminServer
	{
		// - get value
		auto& context = context_config["adminserver"sv];

		// - ...
		if (context.is_valid() == true)
		{
			// - Address
			{
				// - host_id를 얻는다.
				auto& context_nest = context_config["address"sv];

				// - ...
				if (context_nest.is_valid() == true)
				{
					this->m_address_admin_server = (const std::string&)context_nest;
				}
			}
		}
	}


	//-----------------------------------------------------------------
	// 2. Host Info
	//-----------------------------------------------------------------
	// declare) 
	server_admin::sINSTANCE_HOST temp_instance_info;

	// 1) host instance - setting
	temp_instance_info.setting.iid = 0;
	//temp_instance_info.setting.name;  // std::string
	//temp_instance_info.setting.comment;	// std::string

	// 2) host instance info
	temp_instance_info.info.system_info = api::get_system_info();
	temp_instance_info.info.os_info = api::get_os_info();
	temp_instance_info.info.mac = api::get_mac_address().front();
	temp_instance_info.info.addr_host = api::socket::instance().get_local_address_public();
	temp_instance_info.info.uid_host = api::get_host_id();
	temp_instance_info.info.uid_os = api::get_os_unique_id();
	temp_instance_info.info.host_name = api::get_host_name();
	temp_instance_info.info.time_start = chrono::time::utc::now();

	// 3) host status
	temp_instance_info.status.iid = temp_instance_info.setting.iid;
	temp_instance_info.status.system_info = api::get_system_info();
	temp_instance_info.status.vector_status.push_back(api::get_system_status());

	// 4) make self host instance and set!s
	auto phost_instance_self = make_own<server_admin::instance_host>();
	phost_instance_self->m_instance = std::move(temp_instance_info);

	// log) 
	LOG_INFO << " - host name: '"sv << phost_instance_self->m_instance.info.host_name << "'[id: "sv << phost_instance_self->m_instance.info.mac.sll_mac << "]"sv;

	// 5) set
	this->m_instance_host_self = std::move(phost_instance_self);

	// 6) Host Info가 변경되었음으로 통보한다.
	//this->transmit_message(sMESSAGE_NOTIFY(eMESSAGE_WINDOWS_NOTIFY_UPDATE, eMESSAGE_UPDATE_HOST_INFO, reinterpret_cast<uintptr_t>(this)));
}

void server_admin::instance_manager::process_save_host_server_info()
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
}

result_code server_admin::instance_manager::on_message_WINDOWS_NOTIFY_UPDATE(sMESSAGE_NOTIFY& _msg)
{
	this->transmit_message(_msg);

	// return) 
	return eRESULT::DONE;
}

result_code server_admin::instance_manager::on_message_SERVER_ADMIN_REQUEST_INIT_SERVICE(sMESSAGE_NETWORK& /*_msg*/)
{
	// 1) Service를 close한다.
	this->initialize();

	// return) 
	return eRESULT::DONE;
}

result_code server_admin::instance_manager::on_message_SERVER_ADMIN_REQUEST_DESTROY_SERVICE(sMESSAGE_NETWORK& /*_msg*/)
{
	// 1) Service를 close한다.
	this->destroy();

	// return) 
	return eRESULT::DONE;
}

result_code server_admin::instance_manager::on_message_SERVER_ADMIN_REQUEST_START_SERVICE(sMESSAGE_NETWORK& /*_msg*/)
{
	// 1) Service를 open한다.
	this->start();

	// return) 
	return eRESULT::DONE;
}

result_code server_admin::instance_manager::on_message_SERVER_ADMIN_REQUEST_STOP_SERVICE(sMESSAGE_NETWORK& /*_msg*/)
{
	// 1) Service를 close한다.
	this->stop();

	// return) 
	return eRESULT::DONE;
}

//void server_admin::instance_manager::send_message_SYSTEM_STATUS_INFO()
//{
//	// 1) Buffer를 할당받는다.
//	auto buf_send = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);
//
//	// 2) Message를 쓴다...
//	buf_send.append<uint32_t>();
//	//	buf_send.append<uint32_t>(eMESSAGE::SERVER::ADMIN::SYSTEM_STATUS_INFO);
//	//
//	//#if defined(_WIN32)
//	//	auto qwMemoryTotal = m_host_instance_status.status_system_resource.bytes_memory_physical_total;
//	//	auto qwMemoryUsage = m_host_instance_status.status_system_resource.bytes_memory_physical_usage;
//	//
//	//	buf_send.append<server_admin::sRANK>(m_host_instance_info.host_iid);
//	//	buf_send.append<uint32_t>(m_host_instance_status.usage_processor);
//	//	buf_send.append<int64_t>(qwMemoryUsage);
//	//	buf_send.append<int64_t>(qwMemoryTotal);
//	//
//	//	// 3) Server List를 붙인다.
//	//	scoped_lock(this->m_cs_instance)
//	//	{
//	//		// - Server의 갯수를 붙인다.
//	//		int	 i = 0;
//	//		int& count = buf_send.append<int>();
//	//
//	//		// - 모든 Server의 정보를 붙인다.
//	//		for (auto& iter : this->m_map_instance_service)
//	//		{
//	//			if (iter->psocket.exist())
//	//			{
//	//				buf_send.append<int64_t>(iter->m_service_instance.server_info.registered.iid);
//	//				buf_send.append<uint64_t>(iter->m_service_instance.server_info.status.usage_process);
//	//				buf_send.append<uint64_t>(iter->m_service_instance.server_info.status.usage_memory);
//	//				buf_send.append<uint64_t>(iter->m_service_instance.server_info.status.usage_network);
//	//				++i;
//	//			}
//	//		}
//	//
//	//		count = i;
//	//	}
//	//#elif defined(__linux__)
//	//#else
//	//#error "CGDK Not supported platform"
//	//#endif
//
//		// 4) 최종적으로 길이를 써넣는다.
//	buf_send.front<uint32_t>() = buf_send.size<uint32_t>();
//
//	// 5) 전송.
//	service::pgroup_client->send(buf_send);
//	//m_psocket_to_admin_server->send(buf_send);
//}

result_code server_admin::instance_manager::on_message_SERVER_TIME_EVENT_EVENT_REGISTER(sMESSAGE& _msg)
{
	// 1) notify to group websocket
	service::pgroup_websocket->m_pobservable_websocket_host_info->add_time_event(_msg.get_source<time_event::Ientity>());

	// return) 
	return eRESULT::DONE;
}

result_code server_admin::instance_manager::on_message_SERVER_TIME_EVENT_EVENT_UNREGISTER(sMESSAGE& _msg)
{
	// 1) notify to group websocket
	service::pgroup_websocket->m_pobservable_websocket_host_info->rempve_time_event(_msg.get_source<time_event::Ientity>());

	// return) 
	return eRESULT::DONE;
}

result_code server_admin::instance_manager::on_message_SERVER_TIME_EVENT_EVENT_EXECUTE(sMESSAGE& _msg)
{
	// 1) notify to group websocket
	service::pgroup_websocket->m_pobservable_websocket_host_info->execute_time_event(_msg.get_source<time_event::Ientity>());

	// return) 
	return eRESULT::DONE;
}

result_code server_admin::instance_manager::on_message_SERVER_TIME_EVENT_EVENT_UPDATE(sMESSAGE& _msg)
{
	// 1) notify to group websocket
	service::pgroup_websocket->m_pobservable_websocket_host_info->update_time_event(_msg.get_source<time_event::Ientity>());

	// return) 
	return eRESULT::DONE;
}

result_code server_admin::instance_manager::on_message_SYSTEM_LOG(sMESSAGE_LOG& _msg)
{
	// check) plog_record가 nullptr이면 여기서 끝낸다.
	RETURN_IF(_msg.plog_record.empty(), 1);

	// check)
	RETURN_IF(service::pgroup_client.empty(), 1);

	//// 2) 전송한다.
	//service::pgroup_client->send(server_admin::message_SYSTEM_LOG(_msg.plog_record));

	// return) 
	return eRESULT::DONE;
}

void server_admin::instance_manager::on_execute()
{
	// check)
	RETURN_IF(this->get_object_state() != eOBJECT_STATE::RUNNING);

	// 1) System Status Info를 얻는다.
	this->process_update_system_status_info();

	// 2) System 정보를 Update한다.
	//service::pgroup_server->procssupdate_server_status_info();
}

void server_admin::instance_manager::process_update_system_status_info()
{
	auto phost_instance_self = server_admin::instance_manager::self().get_object_ptr();

	// 1) HostID와 HostCode...
	phost_instance_self->m_instance.status.iid = phost_instance_self->m_instance.setting.iid;

	// 2) Update한다...
	api::manager_perfornamce_query::instance()->update_query();

#if defined(_WIN32)
	PERFORMANCE_INFORMATION performance_info;

	// 3) Memory 상태를 얻는다.
	GetPerformanceInfo(&performance_info, sizeof(PERFORMANCE_INFORMATION));

	// 4) get total memory
	phost_instance_self->m_instance.status.system_info.memory.bytes_physical_total = ((uint64_t)performance_info.PhysicalTotal) * ((uint64_t)performance_info.PageSize);

	// 5) ...
	SYSTEM::sSTATUS status_now;
	status_now.bytes_memory_usage = phost_instance_self->m_instance.status.system_info.memory.bytes_physical_total - ((uint64_t)performance_info.PhysicalAvailable) * ((uint64_t)performance_info.PageSize);
	status_now.handle_count = performance_info.HandleCount;
	status_now.process_count = performance_info.ProcessCount;
	status_now.thread_count = performance_info.ThreadCount;

	// 4) CPU 사용률 구하기
	if (this->m_perfornamce_processor.exist())
	{
		status_now.processor_usage = this->m_perfornamce_processor->get_value_long();
	}
	// 5) Network 사용률구하기
	status_now.bytes_traffic = 0;

#elif defined(__linux__)
	// declare) 
	struct sysinfo temp_system_info;

	// 1) 물리 메모리량을 얻는다.
	auto result = sysinfo(&temp_system_info);

	// 1) 물리 메모리량을 얻는다.
	if (result == 0)
	{
		phost_instance_self->m_instance.status.system_info.memory.bytes_physical_total = temp_system_info.totalram;
	}

	// declaer) 
	SYSTEM::sSTATUS status_now;

	// 2) 사용중인 물리 메모리
	status_now.bytes_memory_usage = 0;

	// 3) handle/process/thread
	status_now.handle_count = 0;
	status_now.process_count = 0;
	status_now.thread_count = 0;

	// 4) CPU 사용률 구하기
	status_now.processor_usage = 0;

	// 5) Network 사용률구하기
	status_now.bytes_traffic = 0;

#else
#error "CGDK Not supported platform"
#endif
	// 6) 
	phost_instance_self->m_instance.status.vector_status.push_back(status_now);

	// 7) Update한 정보를 전송한다.
	service::pgroup_client->send(service::pmanager_instance->message_SERVER_ADMIN_HOST_STATUS_UPDATE());
	service::pgroup_websocket->notify_to_web_controls_INSTANCE_STATUS_UPDATE();
}

object_ptr<server_admin::Iinstance> server_admin::instance_manager::process_register_instance_host(own_ptr<Iinstance>&& _pinstance)
{
	// 1) get instance
	auto pinstance_host = dynamic_pointer_cast<instance_host>(_pinstance);

	// declare)
	object_ptr<Iinstance> pinstance;
	own_ptr<Iinstance> pinstance_old;

	// check) 
	CGASSERT_ERROR(pinstance_host.exist());

	scoped_lock(this->m_cs_instance)
	{
		// 2) get iid
		auto iid = pinstance_host->get_iid();

		// 3) find host id instance
		auto iter_find = this->m_map_instance.find(iid);

		// 4)
		if (iter_find != this->m_map_instance.end())
		{
			// - 기존 instance를 옮겨 놓는다.
			pinstance_old = std::move(iter_find->second);

			// - 새로운 instance를 추가한다.
			iter_find->second = std::move(_pinstance);

			// - get instance
			pinstance = iter_find->second.get_object_ptr();
		}
		else
		{
			// - get instance
			pinstance = _pinstance.get_object_ptr();

			// - 신규 추가!
			this->m_map_instance.insert({ iid, std::move(_pinstance) });
		}

		// log) 
		LOG_INFO << "  - INSTANCE('"sv << pinstance_host->get_name() << "') > registered ['instance id':"sv << iid << "]"sv;
	}

	// 5) Server가 추가되었음을 notify한다.
	service::pgroup_client->send(server_admin::message_SERVER_ADMIN_INSTANCE_REGISTER(pinstance_host));
	service::pgroup_websocket->notify_to_web_controls_INSTANCE_ENTER(pinstance_host);

	// 6) 접속 종료 처리
	if (pinstance_old.exist())
	{
		//pinstance_old->destroy();
	}

	// return) 
	return pinstance;
}

object_ptr<server_admin::instance_service_local> server_admin::instance_manager::_process_find_instance_by_pid(uint64_t _pid)
{
	// declare) 
	object_ptr<instance_service_local> pinstance_find;

	// check) 
	RETURN_IF(_pid == 0, pinstance_find);

	for (auto& iter : this->m_map_instance)
	{
		// check)
		CONTINUE_IF(iter.second->get_instance_type() != eINSTANCE_TYPE::SERVICE);

		// 1) get...
		auto pinstance_service_local = dynamic_pointer_cast<instance_service_local>(iter.second);

		// check)
		CONTINUE_IF(pinstance_service_local.empty());

		// 2) same?
		if (pinstance_service_local->m_instance.info.pid == _pid)
		{
			// - find!
			pinstance_find = pinstance_service_local;

			// break)
			break;
		}
	}

	// return)
	return pinstance_find;
}

object_ptr<server_admin::instance_service_local> server_admin::instance_manager::_process_find_instance_by_rank(server_admin::sRANK _rank)
{
	// declare) 
	object_ptr<instance_service_local> pinstance_find;

	// check) 
	RETURN_IF(_rank.empty(), pinstance_find);

	for (auto& iter : this->m_map_instance)
	{
		// check) 
		CONTINUE_IF(iter.second->get_instance_type() != eINSTANCE_TYPE::SERVICE);

		// 1) find instance
		auto pinstance_service_local = dynamic_pointer_cast<instance_service_local>(iter.second);

		// check)
		CONTINUE_IF(pinstance_service_local.empty());

		// 2) 확인한다.
		if (pinstance_service_local->m_instance.setting.rank == _rank)
		{
			pinstance_find = std::move(pinstance_service_local);
			break;
		}
	}

	// return)
	return pinstance_find;
}

object_ptr<server_admin::instance_service_local> server_admin::instance_manager::process_find_instance_by(uint64_t _pid, server_admin::sRANK _rank)
{
	// declare)
	object_ptr<instance_service_local> pinstance_find;

	scoped_lock(this->m_cs_instance)
	{
		// 1) pid로 찾기
		//    - host agent에서 instialize시킨 경우 이것으로 확인 가능하다.
		//    - host agent를 종료한 후 다시 띄워서 접속될 경우 이것으로 확인이 불가능하다.
		pinstance_find = this->_process_find_instance_by_pid(_pid);

		// 2) find instance by rank
		//    - rank로 찾는다.
		if (pinstance_find.empty())
		{
			pinstance_find = this->_process_find_instance_by_rank(_rank);
		}
	}

	// return)
	return pinstance_find;
}


object_ptr<server_admin::Iinstance> server_admin::instance_manager::process_register_instance_service(own_ptr<Iinstance>&& _pinstance)
{
	// 1) get instance
	auto pinstance_service = dynamic_pointer_cast<instance_Iservice>(_pinstance);

	// check) 
	CGASSERT_ERROR(pinstance_service.exist());

	scoped_lock(this->m_cs_instance)
	{
		// declare)
		object_ptr<instance_service_local> pinstance_find;
		
		// 2) pid로 찾기
		//    - host agent에서 instialize시킨 경우 이것으로 확인 가능하다.
		//    - host agent를 종료한 후 다시 띄워서 접속될 경우 이것으로 확인이 불가능하다.
		pinstance_find = this->_process_find_instance_by_pid(pinstance_service->m_instance.info.pid);

		// 3) rank로 찾기
		//    - host agent로 initialize 시킨 이후에 host agent를 종료시켰다가 다시 섭속되었을 때 확인하는 용도다.
		//    - host agent로 initialize 시킨 경우에만 rank정보를 전달하므로 이 경우에 해당할 것이다.
		if (pinstance_find.exist())
		{
			LOG_INFO << "  - INSTANCE('"sv << pinstance_service->get_name() << "') > certified by pid (pid:"sv << pinstance_service->m_instance.info.pid << ")"sv;
		}
		else
		{
			pinstance_find = this->_process_find_instance_by_rank(pinstance_service->m_instance.setting.rank);

			// log) 
			if (pinstance_find.exist())
			{
				LOG_INFO << "  - INSTANCE('"sv << pinstance_service->get_name() << "') > certified by RANK (rank:"sv << pinstance_service->m_instance.setting.rank.to_string() << ")"sv;
			}
		}

		// 4) 기존에 없을 경우 신규 추가한다.
		if (pinstance_find.empty())
		{
			// - get iid
			auto iid = _pinstance->get_iid();

			// - 먼저 이미 등록된 것이 있는지 찾는다.
			auto iter = this->m_map_instance.find(iid);

			// check) 찾지 못했으면 false를 리턴하고 끝낸다.
			THROW_IF(iter != this->m_map_instance.end(), throwable::failure(eRESULT::ALEADY_EXIST, 0, ""sv));

			// - insert
			this->m_map_instance.insert({ iid, std::move(_pinstance) });

			// log) 
			LOG_INFO << "  - INSTANCE('"sv << pinstance_service->get_name() << "') > registered [LOCAL,'instance id':"sv << iid << "]"sv;

			// - instance가 추가되었음을 notify한다.
			service::pgroup_client->send(server_admin::message_SERVER_ADMIN_INSTANCE_REGISTER(pinstance_service));
			service::pgroup_websocket->notify_to_web_controls_INSTANCE_ENTER(pinstance_service);
		}
		else
		{
			// - set socket and get old socket
			auto psocket_old = pinstance_find->set_socket(pinstance_service->move_out_socket());

			// - unflag service
			if (psocket_old.exist())
			{
				// - erase instance
				scoped_lock(psocket_old->m_cs_instance)
				{
					auto iter_find = std::find(psocket_old->m_vector_instance.begin(), psocket_old->m_vector_instance.end(), pinstance_find);

					if (iter_find != psocket_old->m_vector_instance.end())
					{
						psocket_old->m_vector_instance.erase(iter_find);
					}
				}

				// - request destory
				psocket_old->send(server_admin::message_SERVER_ADMIN_REQUEST_DESTROY_SERVICE(pinstance_find->m_instance.setting.iid));

				// - disconnect
				psocket_old->disconnect();
			}

			// - get iid
			pinstance_find->set_instance_info(std::move(pinstance_service->m_instance));

			// - set
			pinstance_service = pinstance_find;

			// log) 
			LOG_INFO << "  - INSTANCE('"sv << pinstance_service->get_name() << "') > connected [LOCAL,'instance id':"sv << pinstance_service->get_iid() << "]"sv;

			// - instance update 되었음을 notify한다.
			//service::pgroup_client->send(server_admin::message_SERVER_ADMIN_INSTANCE_REGISTER(pinstance_service));
			service::pgroup_websocket->notify_to_web_controls_INSTANCE_STATUS_UPDATE(pinstance_service);
		}
	}

	// return) 
	return pinstance_service;
}

shared_buffer server_admin::instance_manager::message_SERVER_ADMIN_HOST_STATUS_UPDATE()
{
	scoped_lock(this->m_cs_instance)
	{
		//return server_admin::message_SERVER_ADMIN_HOST_STATUS_UPDATE(server_admin::instance_manager::self()->m_instance.status);
	}

	return shared_buffer();
}

