#include "pch.h"


#if defined(__linux__) && !defined(__ANDROID__)
#include <sys/sysinfo.h>
#elif defined(_WIN32)
#include <iphlpapi.h>
#include <netioapi.h>
#pragma comment(lib, "iphlpapi.lib")
#endif

std::atomic<uint64_t> server_system::instance_manager::m_id_alloc = 0;

#define	LOG_FILE	"host_agent.log"sv

own_ptr<server_system::instance_host>	server_system::instance_manager::m_instance_host_self;
object_ptr<api::performance_counter>	server_system::instance_manager::m_performance_processor;
std::string								server_system::instance_manager::m_address_admin_server;

uint64_t server_system::instance_manager::_alloc_object_id()
{
	return	m_id_alloc++;
}

std::filesystem::file_time_type server_system::instance_manager::_get_file_time(std::string_view _filename) noexcept
{
	// - 파일이 없으면 기본값
	std::error_code ec;
	auto time_file = std::filesystem::last_write_time(std::filesystem::path(_filename), ec);
	return ec ? std::filesystem::file_time_type{} : time_file;
}

object_ptr<server_system::Iinstance> server_system::instance_manager::register_instance(own_ptr<Iinstance>&& _pinstance)
{
	// check)
	CGASSERT_ERROR(_pinstance.exist());

	// check)
	CGASSERT_ERROR(_pinstance->get_iid() != 0);

	// check)
	THROW_IF(_pinstance->get_iid() == 0, throwable::failure(eRESULT::INVALID_REQUEST));

	// declare)
	object_ptr<server_system::Iinstance> pinstance;

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

bool server_system::instance_manager::unregister_instance(server_system::iid_t _iid, socket_peer* _psocket) noexcept
{
	// declare) 
	object_ptr<Iinstance> pinstance;

	// declare)
	bool is_not_removed = false;

	SCOPED_LOCK(this->m_lockable_instance)
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
		//::service::pgroup_peer_client->send(server_system::make_message_SERVER_ADMIN_INSTANCE_UPDATE(_iid, pinstance));
		::service::pgroup_websocket->notify_to_web_controls_INSTANCE_STATUS_UPDATE(pinstance);

		// log) 
		LOG_WARNING << "  - INSTANCE('"sv << pinstance->get_name() << "') (!) instance disconnected['instance id':"sv << _iid << "]"sv;
	}
	else
	{
		// - notify
		::service::pgroup_peer_client->send(server_system::make_message_SERVER_ADMIN_INSTANCE_UNREGISTER(_iid));
		::service::pgroup_websocket->notify_to_web_controls_INSTANCE_LEAVE(pinstance);

		// log) 
		LOG_INFO << "  - INSTANCE('"sv << pinstance->get_name() << "') < unregister ['instance id':"sv << _iid << "]"sv;
	}

	// return
	return true;
}

void server_system::instance_manager::unregister_instance_all() noexcept
{
	// declare) 
	std::vector<server_system::iid_t> vector_instance_iid;

	// 1) get..
	SCOPED_LOCK(this->m_lockable_instance)
	{
		for (auto& iter : this->m_map_instance)
			vector_instance_iid.push_back(iter.first);
	}

	// 2) 모두 unregister한다.
	for (auto& iter : vector_instance_iid)
		this->unregister_instance(iter, nullptr);
}

//object_ptr<server_system::instance_service> server_system::instance_manager::update_server_setting(const server_system::sSERVICE::sINFO& _server_register_info)
//{
//	// declare) 
//	object_ptr<server_system::instance_service> pfind_server_setting;
//
//	SCOPED_LOCK(this->m_cs_instance)
//	{
//		// 1) 해당 객체를 찾는다.
//		auto iter_find = std::find_if(this->m_list_server_instance.begin(), this->m_list_server_instance.end(), [=](object_ptr<server_system::instance_service>& iter)->bool { return iter->m_service_instance.server_info.registered.iid == _server_register_info.iid; });
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
//	LOG_INFO << " - SERVER_REGISTER info updated '"sv << _server_register_info.setting.name << "'[server_id:"sv << _server_register_info.iid << "]"sv;
//
//	// notify) Server가 Update되었음을 notify한다.
//	::service::pgroup_peer_client->send(server_system::make_message_SERVER_ADMIN_INSTANCE_UPDATE_SETTING(m_host_instance_info.host_iid, _server_register_info.iid, _server_register_info));
//
//	// return) 
//	return pfind_server_setting;
//}

object_ptr<server_system::Iinstance> server_system::instance_manager::find_instance(server_system::iid_t _iid) const noexcept
{
	// declare) 
	object_ptr<server_system::Iinstance> pinstance;

	SCOPED_LOCK(this->m_lockable_instance)
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

object_ptr<server_system::Iinstance> server_system::instance_manager::find_instance_by_pid(uint64_t _pid) const noexcept
{
	// declare) 
	object_ptr<server_system::instance_service> pfind;

	SCOPED_LOCK(this->m_lockable_instance)
	{
		for (auto& iter : this->m_map_instance)
		{
			// check) 
			CONTINUE_IF(iter.second->get_instance_type() != eINSTANCE_TYPE::HOST);

			// - 
			auto pinstance_service = dynamic_pointer_cast<instance_service>(iter.second);

			// check)
			BREAK_IF(pinstance_service.empty());

			if (pinstance_service->m_instance.info.info.pid == _pid)
			{
				pfind = pinstance_service;
				break;
			}
		}
	}

	// return)
	return pfind;
}

object_ptr<server_system::Iinstance> server_system::instance_manager::find_instance_by_pvoid(void* _id_select) const noexcept
{
	// declare)
	object_ptr<server_system::Iinstance> pinstance;

	SCOPED_LOCK(this->m_lockable_instance)
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



void server_system::instance_manager::on_initialize(context& _context)
{
	// log) 
	LOG_INFO << " - create SERVICE_ENTITY manager"sv;

	//-----------------------------------------------------------------
	// 2. read host info
	//-----------------------------------------------------------------
	// definition) 
	//  - working directory 기준 (linux의 systemd는 WorkingDirectory=로 지정한다.)
	#define	SETTING_FILE_PATH	"./"sv
	#define	SETTING_FILE_NAME	SETTING_FILE_HOST_AGENT ""sv

	// 1) set default value
	_context["name"sv] = "CGCII host_agent"sv;
	_context["log_file"sv]["path"sv] = SETTING_FILE_PATH;
	_context["log_file"sv]["filename"sv] = LOG_FILE;

	// 2) read setting json file
	load_json_file(_context, api::file::make_full_path(SETTING_FILE_PATH, SETTING_FILE_NAME));


	//-----------------------------------------------------------------
	// 2. ready instance manager
	//-----------------------------------------------------------------
	// 1) Register Mediator한다.
	this->register_messageable(this);

	// 2) processor...
	this->m_performance_processor = make_object<api::performance_counter>("\\Processor(_Total)\\% Processor Time"sv);

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

void server_system::instance_manager::on_destroy()
{
	// log) 
	LOG_INFO << " - destroy SERVICE_ENTITY manager"sv;

	// 1) write host server info to json file
	this->process_save_host_server_info();

	// 2) processor...
	this->m_performance_processor.reset();

	// 3) Messageable을 unregister한다.
	this->unregister_messageable(this);
}

void server_system::instance_manager::on_start(context& /*_context*/)
{
	// declare)
	auto& info = server_system::instance_manager::self()->m_instance.info.info;
	auto& setting = server_system::instance_manager::self()->m_instance.info.setting;

	// log) 
	LOG_INFO << " - HOST.name: "sv << server_system::instance_manager::self()->m_instance.info.info.host_name;
	LOG_INFO << " - HOST.id: "sv << info.mac.sll_addr[0] << ":"sv << info.mac.sll_addr[1] << ":"sv << info.mac.sll_addr[2] << ":"sv << info.mac.sll_addr[3] << ":"sv << info.mac.sll_addr[4] << ":"sv << info.mac.sll_addr[5] << "."sv << setting.iid;
	LOG_INFO << " - HOST.infoSys: core count="sv << info.system_info.processor.number_of_processor;
	LOG_INFO << " - HOST.info_os: "sv << static_cast<int>(info.os_info.type) << " "sv << info.os_info.version.major << "."sv << info.os_info.version.minor << "."sv << info.os_info.version.build;
	LOG_INFO << " - HOST.uidHost: "sv << info.uid_host;
	LOG_INFO << " - HOST.uidOS: "sv << info.uid_os;

	// 1) register message mediator
	time_event::entity_manager::instance()->register_messageable(this);

	// 2) registered Server Info占쏙옙 占싻억옙쨈占?
	this->process_load_server_list();

	//// test) add event for test
	//{
	//	// 1) time_event 객체를 생성한다.
	//	auto pentity_a = make_object<foo_time_event_iteration>();

	//	// 2) setting
	//	pentity_a->name("test_entity");
	//	pentity_a->set_event_time(chrono::time::utc::now() + 1s);
	//	pentity_a->set_event_interval(10s);
	//	pentity_a->set_event_times(1000);

	//	// 3) start
	//	pentity_a->start();



	//	// 1) time_event 객체를 생성한다.
	//	auto pentity_b = make_object<foo_time_event_start_end>();

	//	// 2) setting
	//	pentity_b->name("test_start_end");
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

	//	auto p_s = dynamic_cast<instance_service*>(p.get());

	//	p_s->request_service_initialize();
	//}


}

void server_system::instance_manager::on_stop()
{
	// 1) unregister message mediator
	time_event::entity_manager::instance()->unregister_messageable(this);

	// 1) stop execution (unregister from schedulable manager)
	system_executor::unregister_schedulable(this);

	// 2) save server list
	this->process_save_server_list();

	// 3) unregister all server info
	this->unregister_instance_all();
}

result_code server_system::instance_manager::on_message(sMESSAGE_NETWORK& _msg)
{
	BEGIN_NESTED_CGMESSAGE_MAP
		ON_CGMESSAGE(eMESSAGE::WINDOWS::NOTIFY_UPDATE, on_message_WINDOWS_NOTIFY_UPDATE, sMESSAGE_NOTIFY)

		CGMESSAGE_TYPE_SUB_BUFFER_NETWORK
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_INIT_SERVICE, on_message_server_system_REQUEST_INIT_SERVICE, sMESSAGE_NETWORK)
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_DESTROY_SERVICE, on_message_server_system_REQUEST_DESTROY_SERVICE, sMESSAGE_NETWORK)
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_START_SERVICE, on_message_server_system_REQUEST_START_SERVICE, sMESSAGE_NETWORK)
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_STOP_SERVICE, on_message_server_system_REQUEST_STOP_SERVICE, sMESSAGE_NETWORK)
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

//bool server_system::instance_manager::request_version_update(server_system::sRANK _i64SystemID)
//{
//	// log) 
//	LOG_INFO << " - VERSION update requested [server_id:"sv << _i64SystemID.key << "]"sv;
//
//	// Declare) 
//	object_ptr<server_system::instance_service> pregistered;
//
//	SCOPED_LOCK(this->m_cs_instance)
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
//	// check) 접속 중이면 Update를 할 수 없다.
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
//	::service::pgroup_peer_client->send(server_system::make_message_SERVER_ADMIN_INSTANCE_UPDATE_SETTING(m_host_instance_info.host_iid, pregistered->m_service_instance.server_info.registered.iid, pregistered->m_service_instance.server_info.registered));
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
	_value = _context.operator std::string();
}

void GET_CONTEXT_VALUE(std::vector<std::string>& _value, context& _context)
{
	//  check) 해당 Context가 없으면 그냥 끝낸다.
	RETURN_IF(_context.is_valid() == false);

	// 1) 배열("0", "1", ...)이면 순서대로 읽는다. (map은 문자열 순서이므로 index로 찾는다.)
	_value.clear();
	if (_context.size() != 0)
	{
		for (int index = 0;; ++index)
		{
			auto& context_item = _context.find(std::to_string(index));
			BREAK_IF(context_item.is_valid() == false);

			_value.push_back(context_item.operator std::string());
		}
		return;
	}

	// 2) 문자열 하나
	_value.push_back(_context.operator std::string());
}

void GET_CONTEXT_VALUE(server_system::sRANK& _value, context& _context)
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
	buf_temp.append_text<char>(_context.operator std::string());
	_buffer.append<char>(0);

	// 3) Environment Block의 끝을 표시하기 위해 NULL을 한 번 더 추가한다.
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
	buf_temp.append_text<char>(_context.operator std::string());
	_buffer.append<char>(0);

	// 3) Environment Block의 끝을 표시하기 위해 NULL을 한 번 더 추가한다.
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
	buf_temp.append_text<char>(_context.operator std::string());
	_buffer.append<char>(0);

	// 3) Environment Block의 끝을 표시하기 위해 NULL을 한 번 더 추가한다.
	_buffer.append<char>(0);

	_buffer = buf_temp;
}

std::vector<own_ptr<server_system::instance_service_local>> server_system::instance_manager::process_read_server_list()
{
	// declare)
	std::vector<own_ptr<server_system::instance_service_local>> vector_server_setting_info;

	// declare)
	context	context_server_list;

	// 1) SERVICE_INSTANCE_LIST를 읽어들인다.
	load_json_file(context_server_list, SETTING_SERVER_LIST);

	for (auto& iter : context_server_list.map_nodes)
	{
		// check) ...
		CONTINUE_IF(iter.second.is_valid() == false);

		// - setting 정보를 작성한다.
		server_system::sSERVICE::sSETTING setting;
		server_system::sSERVICE::sINFO info;

		setting.name = iter.first;
		GET_CONTEXT_VALUE(setting.iid, iter.second["iid"sv]);
		GET_CONTEXT_VALUE(setting.comment, iter.second["comment"sv]);
		info.host_iid = this->m_instance_host_self->m_instance.info.setting.iid;
		GET_CONTEXT_VALUE(setting.rank, iter.second["rank"sv]);
		//GET_CONTEXT_VALUE(setting.buf_configure, iter.second["configure"sv]);
		GET_CONTEXT_VALUE(setting.filename, iter.second["execute_file"sv]);
		GET_CONTEXT_VALUE(setting.path, iter.second["path"sv]);
		GET_CONTEXT_VALUE(setting.working_path, iter.second["working_path"sv]);
		if (auto& context_service_type = iter.second.find("service_type"sv); context_service_type.is_valid())
			setting.service_type = server_system::to_service_type(context_service_type.operator std::string());	// - "execute"(기본) / "service" / "docker"
		if (auto& context_stop_grace_period = iter.second.find("stop_grace_period"sv); context_stop_grace_period.is_valid())
			setting.stop_grace_period = static_cast<uint32_t>(context_stop_grace_period);	// - 멈출 때 기다리는 시간(초. 없으면 설정하지 않는다.)
		GET_CONTEXT_VALUE(setting.arguments, iter.second["arguments"sv]);
		GET_CONTEXT_VALUE(setting.environment, iter.second["environment"sv]);
		GET_CONTEXT_VALUE(setting.revision.url, iter.second["revision_url"sv]);
		GET_CONTEXT_VALUE(setting.revision.revision, iter.second["revision_revision"sv]);
		info.time_register = chrono::time::utc::now();
		info.time_update = info.time_register;

		// log) 
		LOG_INFO << "    + add SERVER setting '"sv << setting.name << "' id["sv << setting.iid << "]"sv;

		// - s`erver Info
		server_system::sSERVICE::sSTATUS proces_status;
		proces_status.iid = setting.iid;
		proces_status.state = eOBJECT_STATE::NONE;

		// - 추가할 ServerRegisterSettingInfo를 설정한다.
		auto pservice_entity_info = make_own<server_system::instance_service_local>();
		pservice_entity_info->m_instance.info.setting = std::move(setting);
		pservice_entity_info->m_instance.info.info = std::move(info);
		pservice_entity_info->m_instance.info.status = std::move(proces_status);

		// check)
		CGASSERT_ERROR(setting.iid != 0);

		// - 추가
		vector_server_setting_info.push_back(std::move(pservice_entity_info));
	}

	// return)
	return vector_server_setting_info;
}

void server_system::instance_manager::process_load_server_list()
{
	// log)
	LOG_INFO << " - load Registered Server Info"sv;

	// 1) 파일의 수정 시각을 기록한다. (reload 확인용)
	this->m_time_server_list = _get_file_time(SETTING_SERVER_LIST);
	this->m_time_setting = _get_file_time(SETTING_FILE_HOST_AGENT);

	// 2) 읽어서 등록한다.
	for (auto& iter : this->process_read_server_list())
	{
		this->register_instance(std::move(iter));
	}
}

void server_system::instance_manager::process_reload_server_list()
{
	//-----------------------------------------------------------------
	// 실행 중에 'host_agent_server_list.json'이 바뀌면 다시 읽는다.
	//  1) 있는 iid   : 설정(실행 파일, 인자, 환경 변수 등)을 바꾼다. (다음 실행부터 적용)
	//  2) 새 iid     : 등록한다.
	//  3) 없어진 iid : 멈춰 있고(NONE) 접속되어 있지 않으면 지운다. (실행 중이면 남겨둔다.)
	//  - 등록/삭제가 있으면 admin server와 다시 인증해서 목록을 맞춘다.
	//-----------------------------------------------------------------
	// log)
	LOG_INFO << " - reload Registered Server Info ('"sv << SETTING_SERVER_LIST << "')"sv;

	// 1) 읽는다.
	auto vector_new = this->process_read_server_list();

	// declare)
	std::set<iid_t> set_iid;
	std::vector<object_ptr<instance_service>> vector_added;
	std::vector<object_ptr<instance_service>> vector_removed;

	SCOPED_LOCK(this->m_lockable_instance)
	{
		// 2) 바꾸거나 추가한다.
		for (auto& iter : vector_new)
		{
			auto iid = iter->get_iid();
			set_iid.insert(iid);
			this->m_set_pending_remove.erase(iid);

			auto iter_find = this->m_map_instance.find(iid);

			// - 새 iid
			if (iter_find == this->m_map_instance.end())
			{
				object_ptr<instance_service> pinstance = iter.get_object_ptr();
				this->m_map_instance.insert({ iid, std::move(iter) });
				vector_added.push_back(std::move(pinstance));
				continue;
			}

			// - 있는 iid (local service만 바꾼다.)
			auto pinstance_local = dynamic_pointer_cast<instance_service_local>(iter_find->second);
			CONTINUE_IF(pinstance_local.empty());

			auto& setting_new = iter->m_instance.info.setting;
			SCOPED_LOCK(pinstance_local->m_lockable_instance)
			{
				auto& setting = pinstance_local->m_instance.info.setting;
				setting.name = std::move(setting_new.name);
				setting.comment = std::move(setting_new.comment);
				setting.rank = setting_new.rank;
				setting.revision = std::move(setting_new.revision);
				setting.service_type = setting_new.service_type;
				setting.filename = std::move(setting_new.filename);
				setting.path = std::move(setting_new.path);
				setting.working_path = std::move(setting_new.working_path);
				setting.arguments = std::move(setting_new.arguments);
				setting.environment = std::move(setting_new.environment);
				setting.stop_grace_period = setting_new.stop_grace_period;
			}
		}

		// 3) 목록에서 없어진 local service를 지운다.
		for (auto iter = this->m_map_instance.begin(); iter != this->m_map_instance.end();)
		{
			auto pinstance_local = dynamic_pointer_cast<instance_service_local>(iter->second);

			// check) local service가 아니거나 목록에 있으면 넘어간다.
			if (pinstance_local.empty() || set_iid.find(iter->first) != set_iid.end())
			{
				++iter;
				continue;
			}

			// - 접속되어 있거나 실행 중이면 남겨둔다.
			bool is_idle = false;
			SCOPED_LOCK(pinstance_local->m_lockable_instance)
			{
				is_idle = pinstance_local->m_psocket.empty() && pinstance_local->m_instance.info.status.state == eOBJECT_STATE::NONE;
			}

			if (is_idle == false)
			{
				LOG_WARNING << "    ! SERVER setting '"sv << pinstance_local->get_name() << "' id["sv << iter->first << "] is removed from the list but still running (removed when it stops)"sv;
				this->m_set_pending_remove.insert(iter->first);
				++iter;
				continue;
			}

			vector_removed.push_back(pinstance_local);
			iter = this->m_map_instance.erase(iter);
		}
	}

	// 4) notify
	this->process_notify_list_changed(vector_added, vector_removed);
}

void server_system::instance_manager::process_notify_list_changed(const std::vector<object_ptr<instance_service>>& vector_added, const std::vector<object_ptr<instance_service>>& vector_removed)
{
	for (auto& iter : vector_added)
	{
		LOG_INFO << "    + add SERVER setting '"sv << iter->get_name() << "' id["sv << iter->get_iid() << "]"sv;
		::service::pgroup_peer_client->send(server_system::make_message_SERVER_ADMIN_INSTANCE_REGISTER(iter));
		::service::pgroup_websocket->notify_to_web_controls_INSTANCE_ENTER(iter);
	}
	for (auto& iter : vector_removed)
	{
		LOG_INFO << "    - remove SERVER setting '"sv << iter->get_name() << "' id["sv << iter->get_iid() << "]"sv;
		::service::pgroup_peer_client->send(server_system::make_message_SERVER_ADMIN_INSTANCE_UNREGISTER(iter->get_iid()));
		::service::pgroup_websocket->notify_to_web_controls_INSTANCE_LEAVE(iter);
	}

	// 5) 목록이 바뀌었으면 admin server와 다시 인증한다. (다시 접속하면 CERTIFY/INSTANCE_CERTIFY를 다시 보낸다.)
	if ((vector_added.empty() == false || vector_removed.empty() == false) && ::service::psocket_to_admin_server.exist())
	{
		LOG_INFO << "    (instance list is changed. re-certify to admin server)"sv;
		::service::psocket_to_admin_server->closesocket();
	}
}

void server_system::instance_manager::process_reload_setting()
{
	// log)
	LOG_INFO << " - reload setting ('"sv << SETTING_FILE_HOST_AGENT << "')"sv;

	// 1) 읽는다.
	context context_setting;
	if (load_json_file(context_setting, SETTING_FILE_HOST_AGENT) == false)
		return;

	// 2) 접속 정책을 다시 적용한다. (이미 접속된 것은 그대로 둔다.)
	::service::apply_access_policy(context_setting);

	// 3) admin server 접속 (remote_endpoint, certify_key가 바뀌었으면 다시 접속한다.)
	if (::service::psocket_to_admin_server.exist())
		::service::psocket_to_admin_server->apply_setting(context_setting["socket_to_admin_server"sv]);
}

void server_system::instance_manager::process_remove_pending()
{
	// check)
	std::vector<object_ptr<instance_service>> vector_removed;

	SCOPED_LOCK(this->m_lockable_instance)
	{
		RETURN_IF(this->m_set_pending_remove.empty());

		for (auto iter_iid = this->m_set_pending_remove.begin(); iter_iid != this->m_set_pending_remove.end();)
		{
			auto iter = this->m_map_instance.find(*iter_iid);
			auto pinstance_local = (iter != this->m_map_instance.end()) ? dynamic_pointer_cast<instance_service_local>(iter->second) : object_ptr<instance_service_local>();

			// - 이미 없다.
			if (pinstance_local.empty())
			{
				iter_iid = this->m_set_pending_remove.erase(iter_iid);
				continue;
			}

			// - 멈췄으면 지운다.
			bool is_idle = false;
			SCOPED_LOCK(pinstance_local->m_lockable_instance)
			{
				is_idle = pinstance_local->m_psocket.empty() && pinstance_local->m_instance.info.status.state == eOBJECT_STATE::NONE;
			}

			if (is_idle == false)
			{
				++iter_iid;
				continue;
			}

			vector_removed.push_back(pinstance_local);
			this->m_map_instance.erase(iter);
			iter_iid = this->m_set_pending_remove.erase(iter_iid);
		}
	}

	// - notify
	if (vector_removed.empty() == false)
		this->process_notify_list_changed({}, vector_removed);
}

void server_system::instance_manager::process_check_reload()
{
	// 0) 목록에서 빠졌지만 실행 중이던 service (멈추면 지운다.)
	this->process_remove_pending();

	// 1) server list
	if (auto time_file = _get_file_time(SETTING_SERVER_LIST); time_file != this->m_time_server_list)
	{
		this->m_time_server_list = time_file;
		if (time_file != std::filesystem::file_time_type{})
			this->process_reload_server_list();
	}

	// 2) setting
	if (auto time_file = _get_file_time(SETTING_FILE_HOST_AGENT); time_file != this->m_time_setting)
	{
		this->m_time_setting = time_file;
		if (time_file != std::filesystem::file_time_type{})
			this->process_reload_setting();
	}
}

void server_system::instance_manager::process_save_server_list()
{
	//// declare) 
	//TCHAR strINIFileName[256];

	//// 1) Ini FileName을 설정한다.
	//wsprintf(strINIFileName, TEXT("%s\\%s"), TEXT("."), TEXT(CONFIGURE_JSON_FILENAME));

	//SCOPED_LOCK(this->m_cs_instance)
	//{
	//	// declare) 
	//	int i = 0;

	//	// 2) 모든 디렉터리들을 저장한다.
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

void server_system::instance_manager::process_load_host_server_info()
{
	//-----------------------------------------------------------------
	// 1. Host Setting Info
	//-----------------------------------------------------------------
	// declare)
	context context_config;

	// 1) JSON파일을 읽어들인다.
	load_json_file(context_config, CONFIGURE_JSON_FILENAME);

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
	server_system::sHOST temp_instance_info;

	// 1) host instance - setting (아래 2)에서 설정한다.)
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

	// 2-1) host instance setting
	//  - host_agent_info.json의 "host": { "iid": ..., "name": "..." }
	//  - 없으면 host id로 만든 값을 쓰고 host_agent_info.json에 저장한다.
	bool is_host_info_generated = false;
	//  - setting 파일의 "host": { "iid": ..., "name": "..." }를 쓴다.
	//  - iid가 없으면 host id(uid_host)로 만든 고정 값을 쓴다. (admin server에 등록하려면 0이 아니어야 한다.)
	{
		uint64_t host_iid = 0;
		std::string host_name;

		auto& context_host = context_config["host"sv];
		if (context_host.is_valid() == true)
		{
			GET_CONTEXT_VALUE(host_iid, context_host["iid"sv]);
			GET_CONTEXT_VALUE(host_name, context_host["name"sv]);
		}

		if (host_iid == 0 || host_iid == static_cast<uint64_t>(server_system::IID_AUTO))
		{
			// - 새로 만든 iid는 아래에서 host_agent_info.json에 저장한다. (다음 실행에도 같은 iid를 쓴다.)
			is_host_info_generated = true;

			host_iid = std::hash<std::string>{}(temp_instance_info.info.uid_host) & 0x7fffffffffffffffull;
			if (host_iid == 0)
				host_iid = 1;
		}

		temp_instance_info.setting.iid = static_cast<server_system::iid_t>(host_iid);
		temp_instance_info.setting.name = host_name.empty() ? temp_instance_info.info.host_name : host_name;
	}

	// 3) host status
	temp_instance_info.status.iid = temp_instance_info.setting.iid;
	temp_instance_info.status.system_info = api::get_system_info();
	temp_instance_info.status.vector_status.push_back(api::get_system_status());

	// 4) make self host instance and set!s
	auto phost_instance_self = make_own<server_system::instance_host>();
	phost_instance_self->m_instance.info = std::move(temp_instance_info);

	// log) 
	LOG_INFO << " - host name: '"sv << phost_instance_self->m_instance.info.info.host_name << "'[id: "sv << phost_instance_self->m_instance.info.info.mac.sll_mac << "]"sv;
	LOG_INFO << " - host instance: '"sv << phost_instance_self->m_instance.info.setting.name << "' [iid: "sv << phost_instance_self->m_instance.info.setting.iid << "]"sv;

	// 5) set
	this->m_instance_host_self = std::move(phost_instance_self);

	// 6) 새로 만든 host 정보는 바로 저장한다.
	if (is_host_info_generated)
	{
		this->process_save_host_server_info();
	}

	// 6) Host Info가 변경되었음으로 통보한다.
	//this->transmit_message(sMESSAGE_NOTIFY(eMESSAGE_WINDOWS_NOTIFY_UPDATE, eMESSAGE_UPDATE_HOST_INFO, reinterpret_cast<uintptr_t>(this)));
}

void server_system::instance_manager::process_save_host_server_info()
{
	//-----------------------------------------------------------------
	// 설명) host 정보(iid/name)를 host_agent_info.json에 저장한다.
	//  - 파일의 다른 key는 그대로 둔다.
	//-----------------------------------------------------------------

	// check)
	if (this->m_instance_host_self.empty())
		return;

	try
	{
		// 1) read existing file (없으면 비어 있다.)
		context context_config;
		load_json_file(context_config, CONFIGURE_JSON_FILENAME);

		// 2) host
		server_system::iid_t host_iid;
		std::string host_name;
		SCOPED_LOCK(this->m_instance_host_self->m_lockable_instance)
		{
			host_iid = this->m_instance_host_self->m_instance.info.setting.iid;
			host_name = this->m_instance_host_self->m_instance.info.setting.name;
		}

		context_config["host"sv]["iid"sv] = static_cast<uint64_t>(host_iid);
		context_config["host"sv]["name"sv] = std::string_view(host_name);

		// 3) write
		api::file::write_file(context_config.to_json(), CONFIGURE_JSON_FILENAME);
	}
	catch (...)
	{
		LOG_WARNING << "(warn) fail to save host info to '"sv << CONFIGURE_JSON_FILENAME << "'"sv;
	}
}

result_code server_system::instance_manager::on_message_WINDOWS_NOTIFY_UPDATE(sMESSAGE_NOTIFY& _msg)
{
	this->transmit_message(_msg);

	// return) 
	return eRESULT::DONE;
}

result_code server_system::instance_manager::on_message_server_system_REQUEST_INIT_SERVICE(sMESSAGE_NETWORK& /*_msg*/)
{
	// 1) Service를 close한다.
	this->initialize();

	// return) 
	return eRESULT::DONE;
}

result_code server_system::instance_manager::on_message_server_system_REQUEST_DESTROY_SERVICE(sMESSAGE_NETWORK& /*_msg*/)
{
	// 1) Service를 close한다.
	this->destroy();

	// return) 
	return eRESULT::DONE;
}

result_code server_system::instance_manager::on_message_server_system_REQUEST_START_SERVICE(sMESSAGE_NETWORK& /*_msg*/)
{
	// 1) Service를 open한다.
	this->start();

	// return) 
	return eRESULT::DONE;
}

result_code server_system::instance_manager::on_message_server_system_REQUEST_STOP_SERVICE(sMESSAGE_NETWORK& /*_msg*/)
{
	// 1) Service를 close한다.
	this->stop();

	// return) 
	return eRESULT::DONE;
}

//void server_system::instance_manager::send_message_SYSTEM_STATUS_INFO()
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
//	//	buf_send.append<server_system::sRANK>(m_host_instance_info.host_iid);
//	//	buf_send.append<uint32_t>(m_host_instance_status.usage_processor);
//	//	buf_send.append<int64_t>(qwMemoryUsage);
//	//	buf_send.append<int64_t>(qwMemoryTotal);
//	//
//	//	// 3) Server List를 붙인다.
//	//	SCOPED_LOCK(this->m_cs_instance)
//	//	{
//	//		// - Server의 개수를 붙인다.
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
//	::service::pgroup_peer_client->send(buf_send);
//	//m_psocket_to_admin_server->send(buf_send);
//}

result_code server_system::instance_manager::on_message_SERVER_TIME_EVENT_EVENT_REGISTER(sMESSAGE& _msg)
{
	// 1) notify to group websocket
	::service::pgroup_websocket->m_pobservable_websocket_host_info->add_time_event(_msg.get_source<time_event::Ientity>());

	// return) 
	return eRESULT::DONE;
}

result_code server_system::instance_manager::on_message_SERVER_TIME_EVENT_EVENT_UNREGISTER(sMESSAGE& _msg)
{
	// 1) notify to group websocket
	::service::pgroup_websocket->m_pobservable_websocket_host_info->remove_time_event(_msg.get_source<time_event::Ientity>());

	// return) 
	return eRESULT::DONE;
}

result_code server_system::instance_manager::on_message_SERVER_TIME_EVENT_EVENT_EXECUTE(sMESSAGE& _msg)
{
	// 1) notify to group websocket
	::service::pgroup_websocket->m_pobservable_websocket_host_info->execute_time_event(_msg.get_source<time_event::Ientity>());

	// return) 
	return eRESULT::DONE;
}

result_code server_system::instance_manager::on_message_SERVER_TIME_EVENT_EVENT_UPDATE(sMESSAGE& _msg)
{
	// 1) notify to group websocket
	::service::pgroup_websocket->m_pobservable_websocket_host_info->update_time_event(_msg.get_source<time_event::Ientity>());

	// return) 
	return eRESULT::DONE;
}

result_code server_system::instance_manager::on_message_SYSTEM_LOG(sMESSAGE_LOG& _msg)
{
	// check) plog_record가 nullptr이면 여기서 끝낸다.
	RETURN_IF(_msg.plog_record.empty(), 1);

	// check)
	RETURN_IF(::service::pgroup_peer_client.empty(), 1);

	// 2) 전송한다.
	//::service::pgroup_peer_client->send(server_system::message_SYSTEM_LOG(_msg.plog_record));

	// return) 
	return eRESULT::DONE;
}

void server_system::instance_manager::on_execute()
{
	// check)
	RETURN_IF(this->get_object_state() != eOBJECT_STATE::RUNNING);

	// 1) System Status Info를 얻는다.
	this->process_update_system_status_info();

	// 2) 설정 파일이 바뀌었으면 다시 읽는다.
	try
	{
		this->process_check_reload();
	}
	catch (...)
	{
		LOG_ERROR << "(err) fail to reload setting"sv;
	}

	// 3) System 정보를 Update한다.
	//::service::pgroup_peer_server->procssupdate_server_status_info();
}

void server_system::instance_manager::process_update_system_status_info()
{
	auto phost_instance_self = server_system::instance_manager::self().get_object_ptr();

	// 1) HostID와 HostCode...
	phost_instance_self->m_instance.info.status.iid = phost_instance_self->m_instance.info.setting.iid;

	// 2) Update한다...
	api::manager_performance_query::instance()->update_query();

#if defined(_WIN32)
	PERFORMANCE_INFORMATION performance_info;

	// 3) Memory 상태를 얻는다.
	GetPerformanceInfo(&performance_info, sizeof(PERFORMANCE_INFORMATION));

	// 4) get total memory
	phost_instance_self->m_instance.info.status.system_info.memory.bytes_physical_total = ((uint64_t)performance_info.PhysicalTotal) * ((uint64_t)performance_info.PageSize);

	// 5) ...
	SYSTEM::sSTATUS status_now;
	status_now.bytes_memory_usage = phost_instance_self->m_instance.info.status.system_info.memory.bytes_physical_total - ((uint64_t)performance_info.PhysicalAvailable) * ((uint64_t)performance_info.PageSize);
	status_now.handle_count = performance_info.HandleCount;
	status_now.process_count = performance_info.ProcessCount;
	status_now.thread_count = performance_info.ThreadCount;

	// 4) CPU 사용률 구하기
	if (this->m_performance_processor.exist())
	{
		status_now.processor_usage = this->m_performance_processor->get_value_long();
	}
	// 5) Network 사용량 (누적 송수신 byte. 실제 장치만. loopback/가상 제외)
	status_now.bytes_traffic = 0;
	{
		MIB_IF_TABLE2* ptable = nullptr;
		if (GetIfTable2(&ptable) == NO_ERROR && ptable != nullptr)
		{
			for (ULONG i = 0; i < ptable->NumEntries; ++i)
			{
				const auto& row = ptable->Table[i];
				if (row.Type == IF_TYPE_SOFTWARE_LOOPBACK || row.InterfaceAndOperStatusFlags.HardwareInterface == FALSE || row.InterfaceAndOperStatusFlags.FilterInterface)
					continue;
				status_now.bytes_traffic += row.InOctets + row.OutOctets;
			}
			FreeMibTable(ptable);
		}
	}

#elif defined(__linux__)
	// declare) 
	struct sysinfo temp_system_info;

	// 1) 물리 메모리량을 얻는다.
	auto result = sysinfo(&temp_system_info);

	// 1) 물리 메모리량을 얻는다.
	if (result == 0)
	{
		phost_instance_self->m_instance.info.status.system_info.memory.bytes_physical_total = temp_system_info.totalram;
	}

	// 2) memory/handle/process/thread/cpu/network (/proc. api.system.linux.cpp)
	SYSTEM::sSTATUS status_now = api::get_system_status();

#else
#error "CGDK Not supported platform"
#endif
	// 6) 기록 (최근 것만 남긴다. 계속 쌓이지 않도록 한다.)
	{
		constexpr std::size_t MAX_STATUS_HISTORY = 120;
		auto& vector_status = phost_instance_self->m_instance.info.status.vector_status;
		vector_status.push_back(status_now);
		if (vector_status.size() > MAX_STATUS_HISTORY)
			vector_status.erase(vector_status.begin(), vector_status.begin() + static_cast<std::ptrdiff_t>(vector_status.size() - MAX_STATUS_HISTORY));
	}

	// - admin server에 host 상태를 보낸다. (HOST_UPDATE_STATUS. admin의 /api/hosts, /metrics)
	//   (web ui 상태 : 접속 수, 시간 제한으로 끊은 수. admin의 cgdk_host_web_*)
	if (::service::psocket_to_admin_server.exist())
	{
		auto f_connections = [](const auto& _pacceptor) -> uint64_t
		{
			if (_pacceptor.empty())
				return 0;
			auto count_allocated = _pacceptor->allocated_socket_count();
			auto count_prepared = _pacceptor->prepared_socket_count();
			return (count_allocated > count_prepared) ? count_allocated - count_prepared : 0;
		};

		server_system::sHOST_WEB_STATUS web_status;
		web_status.web_connections = f_connections(::service::pacceptor_web);
		web_status.web_timeout_request = ::service::limits_web.count_timeout.certify.load();
		web_status.web_timeout_idle = ::service::limits_web.count_timeout.idle.load();
		web_status.websocket_connections = f_connections(::service::pacceptor_websocket);
		web_status.websocket_timeout_request = ::service::limits_websocket.count_timeout.certify.load();
		web_status.websocket_timeout_idle = ::service::limits_websocket.count_timeout.idle.load();

		::service::psocket_to_admin_server->send_if_certified(server_system::make_message_SERVER_ADMIN_HOST_UPDATE_STATUS(phost_instance_self->m_instance.info.setting.iid, status_now, &web_status));
	}

	// 7) Update한 정보를 전송한다.
	::service::pgroup_peer_client->send(::service::pmanager_instance->message_server_system_HOST_STATUS_UPDATE());
	::service::pgroup_websocket->notify_to_web_controls_INSTANCE_STATUS_UPDATE();
}

object_ptr<server_system::Iinstance> server_system::instance_manager::process_register_instance_host(own_ptr<Iinstance>&& _pinstance)
{
	// 1) get instance
	auto pinstance_host = dynamic_pointer_cast<instance_host>(_pinstance);

	// declare)
	object_ptr<Iinstance> pinstance;
	own_ptr<Iinstance> pinstance_old;

	// check) 
	CGASSERT_ERROR(pinstance_host.exist());

	SCOPED_LOCK(this->m_lockable_instance)
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
	::service::pgroup_peer_client->send(server_system::make_message_SERVER_ADMIN_INSTANCE_REGISTER(pinstance_host));
	::service::pgroup_websocket->notify_to_web_controls_INSTANCE_ENTER(pinstance_host);

	// 6) 접속 종료 처리
	if (pinstance_old.exist())
	{
		//pinstance_old->destroy();
	}

	// return) 
	return pinstance;
}

object_ptr<server_system::instance_service_local> server_system::instance_manager::_process_find_instance_by_pid(uint64_t _pid)
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
		if (pinstance_service_local->m_instance.info.info.pid == _pid)
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

object_ptr<server_system::instance_service_local> server_system::instance_manager::_process_find_instance_by_rank(server_system::sRANK _rank)
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
		if (pinstance_service_local->m_instance.info.setting.rank == _rank)
		{
			pinstance_find = std::move(pinstance_service_local);
			break;
		}
	}

	// return)
	return pinstance_find;
}

object_ptr<server_system::instance_service_local> server_system::instance_manager::process_find_instance_by(uint64_t _pid, server_system::sRANK _rank)
{
	// declare)
	object_ptr<instance_service_local> pinstance_find;

	SCOPED_LOCK(this->m_lockable_instance)
	{
		// 1) pid로 찾기
		//    - host agent에서 initialize시킨 경우 이것으로 확인 가능하다.
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


object_ptr<server_system::Iinstance> server_system::instance_manager::process_register_instance_service(own_ptr<Iinstance>&& _pinstance)
{
	// 1) get instance
	auto pinstance_service = dynamic_pointer_cast<instance_service>(_pinstance);

	// check) 
	CGASSERT_ERROR(pinstance_service.exist());

	SCOPED_LOCK(this->m_lockable_instance)
	{
		// declare)
		object_ptr<instance_service_local> pinstance_find;
		
		// 2) pid로 찾기
		//    - host agent에서 initialize시킨 경우 이것으로 확인 가능하다.
		//    - host agent를 종료한 후 다시 띄워서 접속될 경우 이것으로 확인이 불가능하다.
		pinstance_find = this->_process_find_instance_by_pid(pinstance_service->m_instance.info.info.pid);

		// 3) rank로 찾기
		//    - host agent로 initialize 시킨 이후에 host agent를 종료시켰다가 다시 섭속되었을 때 확인하는 용도다.
		//    - host agent로 initialize 시킨 경우에만 rank정보를 전달하므로 이 경우에 해당할 것이다.
		if (pinstance_find.exist())
		{
			LOG_INFO << "  - INSTANCE('"sv << pinstance_service->get_name() << "') > certified by pid (pid:"sv << pinstance_service->m_instance.info.info.pid << ")"sv;
		}
		else
		{
			pinstance_find = this->_process_find_instance_by_rank(pinstance_service->m_instance.info.setting.rank);

			// log) 
			if (pinstance_find.exist())
			{
				LOG_INFO << "  - INSTANCE('"sv << pinstance_service->get_name() << "') > certified by RANK (rank:"sv << pinstance_service->m_instance.info.setting.rank.to_string() << ")"sv;
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
			THROW_IF(iter != this->m_map_instance.end(), throwable::failure(eRESULT::ALREADY_EXIST, 0, ""sv));

			// - insert
			this->m_map_instance.insert({ iid, std::move(_pinstance) });

			// log) 
			LOG_INFO << "  - INSTANCE('"sv << pinstance_service->get_name() << "') > registered [LOCAL,'instance id':"sv << iid << "]"sv;

			// - instance가 추가되었음을 notify한다.
			::service::pgroup_peer_client->send(server_system::make_message_SERVER_ADMIN_INSTANCE_REGISTER(pinstance_service));
			::service::pgroup_websocket->notify_to_web_controls_INSTANCE_ENTER(pinstance_service);
		}
		else
		{
			// - set socket and get old socket
			auto psocket_old = pinstance_find->set_socket(pinstance_service->move_out_socket());

			// - unflag service
			if (psocket_old.exist())
			{
				// - erase instance
				SCOPED_LOCK(psocket_old->m_lockable_instance)
				{
					auto iter_find = std::find(psocket_old->m_vector_instance.begin(), psocket_old->m_vector_instance.end(), pinstance_find);

					if (iter_find != psocket_old->m_vector_instance.end())
					{
						psocket_old->m_vector_instance.erase(iter_find);
					}
				}

				// - request destroy
				psocket_old->send(server_system::make_message_SERVER_ADMIN_REQUEST_DESTROY_SERVICE(pinstance_find->m_instance.info.setting.iid));

				// - disconnect
				psocket_old->disconnect();
			}

			// - get iid
			pinstance_find->set_instance_info(std::move(pinstance_service->m_instance.info));

			// - set
			pinstance_service = pinstance_find;

			// log) 
			LOG_INFO << "  - INSTANCE('"sv << pinstance_service->get_name() << "') > connected [LOCAL,'instance id':"sv << pinstance_service->get_iid() << "]"sv;

			// - instance update 되었음을 notify한다.
			//::service::pgroup_peer_client->send(server_system::make_message_SERVER_ADMIN_INSTANCE_REGISTER(pinstance_service));
			::service::pgroup_websocket->notify_to_web_controls_INSTANCE_STATUS_UPDATE(pinstance_service);
		}
	}

	// return) 
	return pinstance_service;
}

shared_buffer server_system::instance_manager::message_server_system_HOST_STATUS_UPDATE()
{
	SCOPED_LOCK(this->m_lockable_instance)
	{
		//return server_system::message_server_system_HOST_STATUS_UPDATE(server_system::instance_manager::self()->m_instance.info.status);
	}

	return shared_buffer();
}

