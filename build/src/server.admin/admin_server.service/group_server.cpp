#include "pch.h"

std::atomic<int> group_server::m_valueIdentity;


group_server::group_server(const std::string_view _name) :
	Nnameable(_name)
{
}

group_server::~group_server()
{
}

void group_server::on_initialize(context& /*_context*/)
{
	// log) 
	LOG_INFO << " - initialize group '" << name() << "'";

	// 1) Register Mediator한다.
	register_messageable(service::get_instance());
}

void group_server::on_destroy()
{
	// log) 
	LOG_INFO << " - destroy group '" << name() << "'";

	// 1) Register Mediator한다.
	unregister_messageable(service::get_instance());
}

void group_server::on_start(context& /*_context*/)
{
	enable_member_enter();
}

void group_server::on_stop()
{
	disable_member_enter();
}

void group_server::update_server_status_info()
{
	// 1) 서버 정보들을 업데이트 한다.
	scoped_lock(m_lockable_group)
	scoped_lock(m_csLock)
	{
		for(auto& iter: m_container_member)
		{
			// - Server 정보를 업데이트한다.
			iter->get_server_entity_info()->update_application_status();
		}
	}

	// 2) AdminServer에 전송한다.
	service::notify_to_controls(message_SERVER_ADMIN_SERVER_STATUS_UPDATE());
	service::get_instance()->m_pgroup_websocket->notify_to_web_controls_SERVER_STATUS_UPDATE();
}

object_ptr<server_entity> group_server::find_server(const server_admin::sID& _id) const noexcept
{
	object_ptr<server_entity> pserver_settingInfo;

	scoped_lock(m_lockable_group)
	scoped_lock(m_csLock)
	{
		for(auto& iter:m_container_member)
		{
			if ((*iter)->m_pinfo_server_entity->info_server_register.server_setting.id==_id)
			{
				pserver_settingInfo = (*iter)->get_server_entity_info();
				break;
			}
		}
	}

	// return) 
	return	pserver_settingInfo;
}

object_ptr<server_entity> group_server::find_server(uint64_t _id_code) const noexcept 
{
	// declare) 
	object_ptr<server_entity> pserver_settingInfo;

	// check) idCode가 0보다 작으면 잘못된 것이다.
	RETURN_IF(_id_code < 0, pserver_settingInfo);

	scoped_lock(m_lockable_group)
	scoped_lock(m_csLock)
	{
		for (const auto& iter : m_container_member)
		{
			if ((*iter)->m_pinfo_server_entity->info_server_register.server_setting.id.key == _id_code)
			{
				pserver_settingInfo = (*iter)->get_server_entity_info();
				break;
			}
		}
	}

	// return) 
	return pserver_settingInfo;
}

void group_server::set_server_register_info(socket_server* _pmember, server_entity* _pserver_entity)
{
	scoped_lock(m_csLock)
	{
		// check) 이미 ServerRegisterInfo가 설정되어 있어서는 안됀다.
		CGASSERT_ERROR(_pmember->get_server_entity_info().empty())

		// 1) ServerRegisterInfof를 설정한다.
		_pmember->set_server_entity_info(_pserver_entity);

		// 2) status_server의 id가 정상값으로 들어가는 것은 해당 Server가 동작중임으로 의미한다.
		_pserver_entity->status_server.state = eOBJECT_STATE::STOPPED;
		_pserver_entity->status_server.id = _pserver_entity->info_server_register.server_setting.id;

		// 3) _pmember를 설정한다.
		_pserver_entity->psocket = _pmember;
	}
}

void group_server::reset_server_register_info(socket_server* _pmember) noexcept
{
	scoped_lock(m_csLock)
	{
		// 1) Server Setting
		auto pserver_entity = _pmember->get_server_entity_info();

		// check) 
		RETURN_IF(pserver_entity.empty());

		// 2) Reset한다.
		pserver_entity->psocket.reset();

		// 3) status_server의 id를 Reset함으로써 해당 Server가 중단 중임을 나타낸다.
		pserver_entity->status_server.id.clear();
		pserver_entity->status_server.state = eOBJECT_STATE::NONE;

		// 4) unregister
		if (pserver_entity->info_server_register.is_registered == false)
		{
			service::get_instance()->unregister_server_setting(pserver_entity);
		}

		// 5) System Status Info를 Reset한다.
		pserver_entity->reset_system_status_info();
	}
}

result_code group_server::on_member_entering(socket_server* /*_pmember*/, sMESSAGE& /*_msg*/)
{
	return	eRESULT::SUCCESS;
}

void group_server::on_member_entered(socket_server* _pmember, sMESSAGE& _msg)
{
	// declare) 
	auto msg = static_cast<sMESSAGE_NETWORK&>(_msg);

	// check) psource가 null이면 안됀다.
	CGASSERT_ERROR(msg.get_source<socket_server>() != nullptr);

	// 1) 임시 Buffer를 선언
	auto buf_temp = msg.buf_message + offset(8);

	// 2) server info를 얻는다.
	auto pid = buf_temp.extract<uint64_t>();
	auto version = buf_temp.extract<sVERSION>();

	// 3) Server Register Info를 등록/설정한다.
	auto pserver_setting = service::get_instance()->init_server_register_info(pid, version);

	// check) Server Registe Info의 설정에 실패했으면 Exception을 던진다.
	THROW_IF(pserver_setting.empty(), throwable::failure());

	// 4) set server info
	pserver_setting->data_factory_info_set(buf_temp.extract<std::vector<server_admin::factory_snap>>());
	pserver_setting->data_executor_info_set(buf_temp.extract<std::vector<server_admin::executor_snap>>());
	pserver_setting->data_schedulable_info_set(buf_temp.extract<std::vector<server_admin::schedulable_snap>>());
	pserver_setting->data_connective_info_set(buf_temp.extract<std::vector<server_admin::connective_info_snap>>());
	pserver_setting->data_binder_info_set(buf_temp.extract<std::vector<server_admin::socket_info_snap>>());
	pserver_setting->data_time_event_info_set(buf_temp.extract<std::vector<server_admin::time_event_snap>>());
	pserver_setting->data_connective_statistics_set(buf_temp.extract<statistics::unit_connective_snap>());
	pserver_setting->data_traffic_statistics_set(buf_temp.extract<statistics::unit_traffic_snap>());
	pserver_setting->data_traffic_log_records_set(buf_temp.extract<circular_list<object_ptr<LOG_RECORD>>>());

	// 5) Socket에 Setting한다.
	set_server_register_info(_pmember, pserver_setting);

	// 6) _pmember의 Mediator를 this로 설정한다.
	_pmember->register_messageable(this);
	
	// 7) Server Setting Info를 전송한다.
	//    - 등록된 Server Setting정보를 검색하여 해당 Server정보를 얻는다.
	//    - 그에 맞는 Server Setting정보를 전송한다.
	try
	{
		// notify) Enter Server Message를 전달한다.
		service::notify_to_controls(_pmember->message_SERVER_ADMIN_SERVER_ENTER());
		service::get_instance()->m_pgroup_websocket->notify_to_web_controls_SERVER_ENTER(_pmember->m_pinfo_server_entity);

		try
		{
			// - Host Info를 전송한다.
			_pmember->send(service::get_instance()->message_SERVER_ADMIN_SEND_HOSTINFO());

			// - Server Setting Info를 전송한다.
			_pmember->send(_pmember->message_SERVER_ADMIN_SEND_SERVER_SETTING());
		}
		catch(...)
		{
			// notify) 다시 빼기 처리한다.
			service::notify_to_controls(_pmember->message_SERVER_ADMIN_SERVER_LEAVE());
			service::get_instance()->m_pgroup_websocket->notify_to_web_controls_SERVER_LEAVE(_pmember->m_pinfo_server_entity);

			// reraise) 
			throw;
		}
	}
	catch(...)
	{
		// - unregister한다.
		_pmember->unregister_messageable(this);

		// - Member의 ServerRegisterInfo를 Reset한다.
		reset_server_register_info(_pmember);

		// reraise) 
		throw;
	}
}

uintptr_t group_server::on_member_leaving(socket_server* _pmember, sMESSAGE& /*_msg*/)
{
	// 2) _pmember의 Mediator에서 떼낸다.
	_pmember->unregister_messageable(this);

	// return) 
	return 0;
}

void group_server::on_member_leaved(socket_server* _pmember, sMESSAGE& /*_msg*/, uintptr_t /*_result*/)
{
	// 1) Register Info를 Reset한다.
	reset_server_register_info(_pmember);

	// 2) unregister 혹은 Update한다.
	service::notify_to_controls(_pmember->message_SERVER_ADMIN_SERVER_LEAVE());
	service::get_instance()->m_pgroup_websocket->notify_to_web_controls_SERVER_LEAVE(_pmember->m_pinfo_server_entity);
}

BEGIN_CGMESSAGE_MAP(group_server)
	CGMESSAGE_TYPE_SUB_BUFFER_NETWORK
		ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::SEND_HOST_INFO,				on_message_SEND_HOST_INFO,								sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::SERVICE_STATE_UPDATE,			on_message_SERVER_ADMIN_SERVICE_STATE_UPDATE,			sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE::FACTORY::FACTORY_LIST_UPDATE,				on_message_FACTORY_FACTORY_LIST_UPDATE,					sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE::FACTORY::FACTORY_REGISTER,					on_message_FACTORY_FACTORY_REGISTER,					sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE::FACTORY::FACTORY_UNREGISTER,					on_message_FACTORY_FACTORY_UNREGISTER,					sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE::EXECUTOR_LIST_UPDATE,						on_message_EXECUTOR_LIST_UPDATE,						sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE::EXECUTOR_REGISTER,							on_message_EXECUTOR_REGISTER,							sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE::EXECUTOR_UNREGISTER,							on_message_EXECUTOR_UNREGISTER,							sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE::SCHEDULABLE_LIST,							on_message_SCHEDULABLE_LIST,							sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE::NETWORK::SOCKET::CONNECTIVE_LIST_UPDATE,		on_message_NETWORK_SOCKET_CONNECTIVE_LIST_UPDATE,		sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE::NETWORK::SOCKET::CONNECTIVE_REGISTER,		on_message_NETWORK_SOCKET_CONNECTIVE_REGISTER,			sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE::NETWORK::SOCKET::CONNECTIVE_UNREGISTER,		on_message_NETWORK_SOCKET_CONNECTIVE_UNREGISTER,		sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE::NETWORK::SOCKET::BINDER_LIST_UPDATE,			on_message_NETWORK_SOCKET_BINDER_LIST_UPDATE,			sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE::NETWORK::SOCKET::BINDER_REGISTER,			on_message_NETWORK_SOCKET_BINDER_REGISTER,				sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE::NETWORK::SOCKET::BINDER_UNREGISTER,			on_message_NETWORK_SOCKET_BINDER_UNREGISTER,			sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE::SERVER::TIME_EVENT::EVENT_LIST_UPDATE,		on_message_SERVER_TIME_EVENT_EVENT_LIST_UPDATE,			sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE::SERVER::TIME_EVENT::EVENT_REGISTER,			on_message_SERVER_TIME_EVENT_EVENT_REGISTER,			sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE::SERVER::TIME_EVENT::EVENT_UNREGISTER,		on_message_SERVER_TIME_EVENT_EVENT_UNREGISTER,			sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::TRAFFIC_INFO_UPDATE,			on_message_SERVER_ADMIN_TRAFFIC_INFO_UPDATE,			sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE::SYSTEM::LOG_,								on_message_SYSTEM_LOG,									sMESSAGE_NETWORK);

		ON_CGMESSAGE(eMESSAGE::DUMP,										on_message_DUMP,										sMESSAGE_NETWORK);
	END_SUB_CGMESSAGE_MAP		
END_CGMESSAGE_MAP

result_code group_server::on_message(sMESSAGE_NETWORK& _msg)
{
	// 1) execute message map
	EXECUTE_CGMESSAGE_MAP(_msg);

	// return) 
	return eRESULT::BYPASS;
}

result_code group_server::on_message_SEND_HOST_INFO(sMESSAGE_NETWORK& /*_msg*/)
{
	// return)
	return eRESULT::DONE;
}

result_code group_server::on_message_SERVER_ADMIN_SERVICE_STATE_UPDATE(sMESSAGE_NETWORK& _msg)
{
	// check) 
	CGASSERT_ERROR(_msg.buf_message.size() == (8 + sizeof(server_admin::sID) + sizeof(server_admin::sID) + sizeof(eOBJECT_STATE)));

	// check) 
	THROW_IF(_msg.buf_message.size() != (8 + sizeof(server_admin::sID) + sizeof(server_admin::sID) + sizeof(eOBJECT_STATE)), throwable::failure());
	
	// 1) socket_server을 얻는다.(_msg가 Ref를 잡고 있으므로 궅이 object_ptr<>를 사용하지 않는다.)
	auto* psource = _msg.get_source<socket_server>();

	// 2) 변경할 새로운 State를 읽어들인다.
	auto state_new = _msg.buf_message.front<eOBJECT_STATE>(8+sizeof(server_admin::sID) + sizeof(server_admin::sID));

	// 3) 새로운 State로 Server의 상태를 바꾼다.
	auto state_old = psource->set_server_state(state_new);

	// check) State의 변경이 없으면 그냥 끝낸다.
	RETURN_IF(state_old == state_new, 1);

	// 4) 상태가 Update되었다면 notify해준다.
	service::notify_to_controls(psource->message_SERVER_ADMIN_SERVER_UPDATE());
	service::get_instance()->m_pgroup_websocket->notify_to_web_controls_SERVER_STATUS_UPDATE(psource->m_pinfo_server_entity);

	// log) 
	LOG_INFO << "-server(" << service::get_instance()->m_setting_host.name << "/" << psource->m_pinfo_server_entity->info_server_register.server_setting.name << ") state changed [" << to_string<char>(state_old) << "] -> [" << to_string<char>(state_new) << "]";

	// return) 
	return eRESULT::DONE;
}

result_code group_server::on_message_FACTORY_FACTORY_LIST_UPDATE(sMESSAGE_NETWORK& _msg)
{
	// 1) get source
	auto* psource = _msg.get_source<socket_server>();

	// 2) get server entity
	auto pinfo_server_entity = psource->m_pinfo_server_entity;

	// check) 
	RETURN_IF(pinfo_server_entity.empty(), eRESULT::DONE)

	// 3) 
	buffer_view buf_temp = _msg.buf_message + offset(8);

	// 4) set data
	pinfo_server_entity->data_factory_info_update(buf_temp.extract<std::vector<server_admin::factory_update_snap>>());

	// return)
	return eRESULT::DONE;
}

result_code group_server::on_message_FACTORY_FACTORY_REGISTER(sMESSAGE_NETWORK& _msg)
{
	// 1) get source
	auto* psource = _msg.get_source<socket_server>();

	// 2) get server entity
	auto pinfo_server_entity = psource->m_pinfo_server_entity;

	// check) 
	RETURN_IF(pinfo_server_entity.empty(), eRESULT::DONE)

	// 3) 
	buffer_view buf_temp = _msg.buf_message + offset(8);

	// 4) set data
	pinfo_server_entity->data_factory_info_register(buf_temp.extract<server_admin::factory_snap>());

	// return)
	return eRESULT::DONE;
}

result_code group_server::on_message_FACTORY_FACTORY_UNREGISTER(sMESSAGE_NETWORK& _msg)
{
	// 1) get source
	auto* psource = _msg.get_source<socket_server>();

	// 2) get server entity
	auto pinfo_server_entity = psource->m_pinfo_server_entity;

	// check) 
	RETURN_IF(pinfo_server_entity.empty(), eRESULT::DONE)

	// 3) 
	buffer_view buf_temp = _msg.buf_message + offset(8);

	// 4) set data
	pinfo_server_entity->data_factory_info_unregister(buf_temp.extract<uint64_t>());

	// return)
	return eRESULT::DONE;
}

result_code group_server::on_message_EXECUTOR_LIST_UPDATE(sMESSAGE_NETWORK& _msg)
{
	// 1) get source
	auto* psource = _msg.get_source<socket_server>();

	// 2) get server entity
	auto pinfo_server_entity = psource->m_pinfo_server_entity;

	// check) 
	RETURN_IF(pinfo_server_entity.empty(), eRESULT::DONE)

	// 3) ...
	buffer_view buf_temp = _msg.buf_message + offset(8);

	// 4) set data
	pinfo_server_entity->data_executor_info_update(buf_temp.extract<std::vector<server_admin::executor_update_snap>>());

	// return)
	return eRESULT::DONE;
}

result_code group_server::on_message_EXECUTOR_REGISTER(sMESSAGE_NETWORK& _msg)
{
	// 1) get source
	auto* psource = _msg.get_source<socket_server>();

	// 2) get server entity
	auto pinfo_server_entity = psource->m_pinfo_server_entity;

	// check) 
	RETURN_IF(pinfo_server_entity.empty(), eRESULT::DONE)

	// 3) ...
	buffer_view buf_temp = _msg.buf_message + offset(8);

	// 4) set data
	pinfo_server_entity->data_executor_info_register(buf_temp.extract<server_admin::executor_snap>());

	// return)
	return eRESULT::DONE;
}
result_code group_server::on_message_EXECUTOR_UNREGISTER(sMESSAGE_NETWORK& _msg)
{
	// 1) get source
	auto* psource = _msg.get_source<socket_server>();

	// 2) get server entity
	auto pinfo_server_entity = psource->m_pinfo_server_entity;

	// check) 
	RETURN_IF(pinfo_server_entity.empty(), eRESULT::DONE)

	// 3) ...
	buffer_view buf_temp = _msg.buf_message + offset(8);

	// 4) set data
	pinfo_server_entity->data_executor_info_unregister(buf_temp.extract<uint64_t>());

	// return)
	return eRESULT::DONE;
}

result_code group_server::on_message_SCHEDULABLE_LIST(sMESSAGE_NETWORK& _msg)
{
	// 1) get source
	auto* psource = _msg.get_source<socket_server>();

	// 2) get server entity
	auto pinfo_server_entity = psource->m_pinfo_server_entity;

	// check) 
	RETURN_IF(pinfo_server_entity.empty(), eRESULT::DONE)

	// 3) 
	buffer_view buf_temp = _msg.buf_message + offset(8);

	// 4) set data
	pinfo_server_entity->data_schedulable_info_set(buf_temp.extract<std::vector<server_admin::schedulable_snap>>());

	// return)
	return eRESULT::DONE;
}

result_code group_server::on_message_NETWORK_SOCKET_CONNECTIVE_LIST_UPDATE(sMESSAGE_NETWORK& _msg)
{
	// 1) get source
	auto* psource = _msg.get_source<socket_server>();

	// 2) get server entity
	auto pinfo_server_entity = psource->m_pinfo_server_entity;

	// check) 
	RETURN_IF(pinfo_server_entity.empty(), eRESULT::DONE)

	// 3) 
	buffer_view buf_temp = _msg.buf_message + offset(8);

	// 4) set data
	pinfo_server_entity->data_connective_info_update(buf_temp.extract<std::vector<server_admin::connective_info_update_snap>>());

	// return)
	return eRESULT::DONE;
}

result_code group_server::on_message_NETWORK_SOCKET_CONNECTIVE_REGISTER(sMESSAGE_NETWORK& _msg)
{
	// 1) get source
	auto* psource = _msg.get_source<socket_server>();

	// 2) get server entity
	auto pinfo_server_entity = psource->m_pinfo_server_entity;

	// check) 
	RETURN_IF(pinfo_server_entity.empty(), eRESULT::DONE)

	// 3) 
	buffer_view buf_temp = _msg.buf_message + offset(8);

	// 4) set data
	pinfo_server_entity->data_connective_info_register(buf_temp.extract<server_admin::connective_info_snap>());

	// return)
	return eRESULT::DONE;
}
result_code group_server::on_message_NETWORK_SOCKET_CONNECTIVE_UNREGISTER(sMESSAGE_NETWORK& _msg)
{
	// 1) get source
	auto* psource = _msg.get_source<socket_server>();

	// 2) get server entity
	auto pinfo_server_entity = psource->m_pinfo_server_entity;

	// check) 
	RETURN_IF(pinfo_server_entity.empty(), eRESULT::DONE)

	// 3) 
	buffer_view buf_temp = _msg.buf_message + offset(8);

	// 4) set data
	pinfo_server_entity->data_connective_info_unregister(buf_temp.extract<uint64_t>());

	// return)
	return eRESULT::DONE;
}

result_code group_server::on_message_NETWORK_SOCKET_BINDER_LIST_UPDATE(sMESSAGE_NETWORK& _msg)
{
	// 1) get source
	auto* psource = _msg.get_source<socket_server>();

	// 2) get server entity
	auto pinfo_server_entity = psource->m_pinfo_server_entity;

	// check) 
	RETURN_IF(pinfo_server_entity.empty(), eRESULT::DONE)

	// 3) 
	buffer_view buf_temp = _msg.buf_message + offset(8);

	// 4) set data
	pinfo_server_entity->data_binder_info_update(buf_temp.extract<std::vector<server_admin::socket_info_update_snap>>());

	// return)
	return eRESULT::DONE;
}

result_code group_server::on_message_NETWORK_SOCKET_BINDER_REGISTER(sMESSAGE_NETWORK& _msg)
{
	// 1) get source
	auto* psource = _msg.get_source<socket_server>();

	// 2) get server entity
	auto pinfo_server_entity = psource->m_pinfo_server_entity;

	// check) 
	RETURN_IF(pinfo_server_entity.empty(), eRESULT::DONE)

	// 3) 
	buffer_view buf_temp = _msg.buf_message + offset(8);

	// 4) set data
	pinfo_server_entity->data_binder_info_register(buf_temp.extract<server_admin::socket_info_snap>());

	// return)
	return eRESULT::DONE;
}

result_code group_server::on_message_NETWORK_SOCKET_BINDER_UNREGISTER(sMESSAGE_NETWORK& _msg)
{
	// 1) get source
	auto* psource = _msg.get_source<socket_server>();

	// 2) get server entity
	auto pinfo_server_entity = psource->m_pinfo_server_entity;

	// check) 
	RETURN_IF(pinfo_server_entity.empty(), eRESULT::DONE)

	// 3) 
	buffer_view buf_temp = _msg.buf_message + offset(8);

	// 4) set data
	pinfo_server_entity->data_binder_info_unregister(buf_temp.extract<uint64_t>());
	
	// return)
	return eRESULT::DONE;
}

result_code group_server::on_message_SERVER_TIME_EVENT_EVENT_LIST_UPDATE(sMESSAGE_NETWORK& _msg)
{
	// 1) get source
	auto* psource = _msg.get_source<socket_server>();

	// 2) get server entity
	auto pinfo_server_entity = psource->m_pinfo_server_entity;

	// check) 
	RETURN_IF(pinfo_server_entity.empty(), eRESULT::DONE)

	// 3) 
	buffer_view buf_temp = _msg.buf_message + offset(8);

	// 4) set data
	pinfo_server_entity->data_time_event_info_update(buf_temp.extract<std::vector<server_admin::time_event_update_snap>>());

	// return)
	return eRESULT::DONE;
}

result_code group_server::on_message_SERVER_TIME_EVENT_EVENT_REGISTER(sMESSAGE_NETWORK& _msg)
{
	// 1) get source
	auto* psource = _msg.get_source<socket_server>();

	// 2) get server entity
	auto pinfo_server_entity = psource->m_pinfo_server_entity;

	// check) 
	RETURN_IF(pinfo_server_entity.empty(), eRESULT::DONE)

	// 3) get buffer
	buffer_view buf_temp = _msg.buf_message + offset(8);

	// 4) set data
	pinfo_server_entity->data_time_event_info_register(buf_temp.extract<server_admin::time_event_snap>());

	// return)
	return eRESULT::DONE;
}

result_code group_server::on_message_SERVER_TIME_EVENT_EVENT_UNREGISTER(sMESSAGE_NETWORK& _msg)
{
	// 1) get source
	auto* psource = _msg.get_source<socket_server>();

	// 2) get server entity
	auto pinfo_server_entity = psource->m_pinfo_server_entity;

	// check) 
	RETURN_IF(pinfo_server_entity.empty(), eRESULT::DONE)

	// 3) get buffer
	buffer_view buf_temp = _msg.buf_message + offset(8);

	// 4) set data
	pinfo_server_entity->data_time_event_info_unregister(buf_temp.extract<uint64_t>());

	// return)
	return eRESULT::DONE;
}

result_code group_server::on_message_SERVER_ADMIN_CONNECTIVE_INFO_UPDATE(sMESSAGE_NETWORK& _msg)
{
	// 1) get source
	auto* psource = _msg.get_source<socket_server>();

	// 2) get server entity
	auto pinfo_server_entity = psource->m_pinfo_server_entity;

	// check) 
	RETURN_IF(pinfo_server_entity.empty(), eRESULT::DONE)

	// 3) 
	buffer_view buf_temp = _msg.buf_message + offset(8);

	// 4) set data
	pinfo_server_entity->data_connective_statistics_update(buf_temp.extract<statistics::unit_connective_snap>());

	// return)
	return eRESULT::DONE;
}

result_code group_server::on_message_SERVER_ADMIN_TRAFFIC_INFO_UPDATE(sMESSAGE_NETWORK& _msg)
{
	// 1) get source
	auto* psource = _msg.get_source<socket_server>();

	// 2) get server entity
	auto pinfo_server_entity = psource->m_pinfo_server_entity;

	// check) 
	RETURN_IF(pinfo_server_entity.empty(), eRESULT::DONE)

	// 3) buffer 
	buffer_view buf_temp = _msg.buf_message + offset(8);

	// 4) set data
	pinfo_server_entity->data_traffic_statistics_update(buf_temp.extract<statistics::unit_traffic_snap>());

	// return)
	return eRESULT::DONE;
}
result_code group_server::on_message_SYSTEM_LOG(sMESSAGE_NETWORK& _msg)
{
	// 1) get source
	auto* psource = _msg.get_source<socket_server>();

	// 2) get server entity
	auto pinfo_server_entity = psource->m_pinfo_server_entity;

	// check) 
	RETURN_IF(pinfo_server_entity.empty(), eRESULT::DONE)

	// 3) buffer 
	buffer_view buf_temp = _msg.buf_message + offset(8);

	// 4) extract
	auto list_log_record = buf_temp.extract<circular_list<object_ptr<LOG_RECORD>>>();

	// 5) set data
	pinfo_server_entity->data_traffic_log_records_add(std::move(list_log_record));

	// return)
	return eRESULT::DONE;
}


result_code group_server::on_message_DUMP(sMESSAGE_NETWORK& /*_msg*/)
{
	// return)
	return eRESULT::DONE;
}

shared_buffer group_server::message_SERVER_ADMIN_SERVER_LIST()
{
	scoped_lock(m_lockable_group)
	{
		// declare) Size Of Message
		size_t size_of_message = 8 + sizeof(server_admin::sHOST_ID) + sizeof(int);

		for (auto& iter : m_container_member)
		{
			// - Server Setting 정보를 써넣는다.
			size_of_message += get_size_of(iter->m_pinfo_server_entity->info_server_register);
			size_of_message += get_size_of(iter->m_pinfo_server_entity->status_server);
		}

		// 1) Buffer를 할당받는다.
		auto buf_temp = alloc_shared_buffer(size_of_message);
			
		// 2) Message를 쓴다...
		buf_temp.append<uint32_t>();
		buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::SERVER_LIST);

		// 3) Host ID
		buf_temp.append<server_admin::sHOST_ID>(service::get_instance()->m_setting_host.hid);

		// 4) Data의 갯수를 써넣는다.
		buf_temp.append<int>(static_cast<int>(m_container_member.size()));

		// 5) Sever 정보를 써넣는다.
		for (auto& iter : m_container_member)
		{
			// - Server Setting 정보를 써넣는다.
			buf_temp.append<server_admin::sSERVER_REGISTER>(iter->m_pinfo_server_entity->info_server_register);
			buf_temp.append<server_admin::sSERVER_STATUS>(iter->m_pinfo_server_entity->status_server);
		}

		// 6) 메시지의 크기를 써넣는다.
		buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

		// check) 
		CGASSERT_ERROR(size_of_message == buf_temp.size());

		// Return)
		return buf_temp;
	}
}

shared_buffer group_server::message_SERVER_ADMIN_SERVER_STATUS_UPDATE()
{
	scoped_lock(m_lockable_group)
	{
		// 1) 서버의 갯수를 얻는다.
		auto countServer = m_container_member.size();

		// declare) Size Of Message
		auto size_of_message = 8 + sizeof(server_admin::sID) + sizeof(int) + sizeof(server_admin::sSERVER_STATUS)*countServer;

		// 2) Buffer를 할당받는다.
		auto buf_temp = alloc_shared_buffer(size_of_message);

		// 3) Message head를 쓴다.
		buf_temp.append<uint32_t>();
		buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::SERVER_STATUS_UPDATE);
		buf_temp.append<server_admin::sHOST_ID>(service::get_instance()->m_setting_host.hid);

		// 4) Data의 갯수를 써넣는다.
		buf_temp.append<int>(static_cast<int>(countServer));

		// 5) Data를 써넣는다.
		for(auto& iter: m_container_member)
		{
			auto pserver_settingInfo = iter->get_server_entity_info();

			CGASSERT_ERROR(pserver_settingInfo->info_server_register.server_setting.id == pserver_settingInfo->status_server.id);

			buf_temp.append<server_admin::sSERVER_STATUS>(pserver_settingInfo->status_server);
		}

		// 5) 길이를 설정한다.
		buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

		// check) 
		CGASSERT_ERROR(size_of_message == buf_temp.size());

		// return) 
		return buf_temp;
	}
}

shared_buffer group_server::message_websocket_SERVER_ADMIN_SERVER_STATUS_UPDATE()
{
	// 1) alloc buffer
	auto buf_temp = alloc_shared_buffer(1024);

	// return) 
	return buf_temp;
}