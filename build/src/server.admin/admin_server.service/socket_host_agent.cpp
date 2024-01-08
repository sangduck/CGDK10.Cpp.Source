#include "pch.h"


host_agent::socket::socket() noexcept :
	m_indexLog(0),
	m_idx_list(-1)
{
}

host_agent::socket::~socket() noexcept
{
}

void host_agent::socket::on_request_connect(const net::sockaddr& /*_paddr_peer*/)
{
}

void host_agent::socket::on_connect(net::io::Iconnective* /*_pconnective*/)
{
	// 1) Reset한다.
	m_info_server.clear();

	// Notify)
	g_pservice->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, CGNOTIFY_HOST_SOCKET_CONNECT, reinterpret_cast<uintptr_t>(this)));
	g_pservice->m_pgroup_admin_client->send(MESSAGE_ADMIN_HOST_SOCKET_ADD::MAKE_BUFFER(m_info_host, m_statusHost));

	// 2) Immigration Group에 Enter시킨다.
	g_pservice->m_pgroup_host_agent_immigration->enter(this);
}

void host_agent::socket::on_fail_connect(net::io::Iconnective* /*_pconnective*/, uint64_t /*_reason*/)
{
}

void host_agent::socket::on_disconnect(uint64_t /*_reason*/)
{
	// 1) Group에서 빠진다.
	leave_group();

	// Notify)
	g_pservice->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, CGNOTIFY_HOST_SOCKET_DISCONNECT, reinterpret_cast<uintptr_t>(this)));
	g_pservice->m_pgroup_admin_client->send(MESSAGE_ADMIN_HOST_SOCKET_REMOVE::MAKE_BUFFER(m_setting_host.hid));
}

result_code host_agent::socket::on_message(sMESSAGE_NETWORK& _msg)
{
	try
	{
		// 1) set source
		_msg.set_source(this);

		// 2) dispatch message
		TRANSMIT_MESSAGE(_msg);

		// error)
		_error_unhandled_message(_msg);
	}
	catch (...)
	{
	}

	// Return)
	return eRESULT::BYPASS;
}

object_ptr<host_agent::server_entity> host_agent::socket::register_server_setting(const server_admin::sSERVER_REGISTER& /*_server_register*/, const server_admin::sSERVER_STATUS& /*_statusServer*/)
{
	// declare) 
	object_ptr<host_agent::server_entity>	pServerRegisterSetting;

	//scoped_lock(m_listServerRegisterSetting)
	//{
	//	// 1) 이미 존재하고 있는 것인가를 확인한다. (이미 존재하고 있으면 값을 Update만 한다.)
	//	pServerRegisterSetting = find_server_entity(_server_register.server_setting.id);

	//	if (pServerRegisterSetting.empty())
	//	{
	//		// - 새로생성한다.
	//		pServerRegisterSetting = make_object<host_agent::server_entity>();

	//		// -추가한다!
	//		m_listServerRegisterSetting.push_back(pServerRegisterSetting);
	//	}

	//	// 2) SERVER SETTING 정보를 설정한다.
	//	pServerRegisterSetting->info_server_register	 = _server_register;	// server_admin::sSERVER_REGISTER
	//	pServerRegisterSetting->status_server		 = _statusServer;
	//}

	//// Notify) Server가 Update 되었음을 Notify한다.
	//g_pservice->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::SERVER_SETTING_REGISTER, reinterpret_cast<uintptr_t>(pServerRegisterSetting.get())));
	//g_pservice->m_pgroup_admin_client->send(pServerRegisterSetting->message_SERVER_ADMIN_SERVER_SETTING_ADD());

	// Return) 
	return	pServerRegisterSetting;
}

object_ptr<host_agent::server_entity> host_agent::socket::register_server_setting(host_agent::socket* /*_psocketHost*/, const server_admin::sSERVER_REGISTER& /*_server_register*/)
{
	// declare) 
	object_ptr<host_agent::server_entity>	pServerRegisterSetting;
	//object_ptr<host_agent::socket>		psocket_host;

	//scoped_lock(m_listServerRegisterSetting)
	//{
	//	// 1) 이미 존재하고 있는 것인가를 확인한다. (이미 존재하고 있으면 값을 Update만 한다.)
	//	pServerRegisterSetting	 = find_server_entity(_server_register.server_setting.id);

	//	// check) 같은 ID의 ServerRegisterSetting이 존재하지 않는다면 신규생성한다.
	//	if (pServerRegisterSetting.empty())
	//	{
	//		// - 새로생성한다.
	//		pServerRegisterSetting = make_object<host_agent::server_entity>();

	//		// - 추가한다!
	//		m_listServerRegisterSetting.push_back(pServerRegisterSetting);
	//	}

	//	// 2) SERVER SETTING 정보를 설정한다.
	//	pServerRegisterSetting->info_server_register		 = _server_register;	// server_admin::sSERVER_REGISTER
	//	psocket_host										 = pServerRegisterSetting->psocket_host;
	//	pServerRegisterSetting->psocket_host				 = _psocketHost;
	//}

	//// 3) 
	//if(psocket_host.exist())
	//{
	//	psocket_host->closesocket();
	//}

	//// Notify) Server가 Update 되었음을 Notify한다.
	//g_pservice->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::SERVER_SETTING_REGISTER, reinterpret_cast<uintptr_t>(pServerRegisterSetting.get())));
	//g_pservice->m_pgroup_admin_client->send(pServerRegisterSetting->message_SERVER_ADMIN_SERVER_SETTING_ADD());

	// Return) 
	return	pServerRegisterSetting;
}

object_ptr<host_agent::server_entity> host_agent::socket::unregister_server_setting(const server_admin::sID& /*_ID*/) noexcept
{
	// declare) 
	object_ptr<host_agent::server_entity>	pserver_entity_info;

	//scoped_lock(m_listServerRegisterSetting)
	//{
	//	auto	iter	 = m_listServerRegisterSetting.begin();
	//	auto	iterEnd	 = m_listServerRegisterSetting.end();

	//	for(; iter!=iterEnd; ++iter)
	//	{
	//		if((*iter)->info_server_register.server_setting.id == _ID)
	//		{
	//			// - 임시로 복사해 놓음.
	//			pserver_entity_info = *iter;

	//			// - 지우기
	//			m_listServerRegisterSetting.erase(iter);

	//			// Break) 
	//			break;
	//		}
	//	}
	//}

	//// check) 
	//RETURN_IF(pserver_entity_info.empty(), object_ptr<host_agent::server_entity>());

	//// 4) Server setting이 삭제되었음을 Notify한다.
	//g_pservice->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::SERVER_SETTING_UNREGISTER, reinterpret_cast<uintptr_t>(pserver_entity_info.get())));
	//g_pservice->m_pgroup_admin_client->send(MESSAGE_ADMIN_SERVER_SETTING_REMOVE::MAKE_BUFFER(pserver_entity_info));

	// Return) 
	return	pserver_entity_info;
}

bool host_agent::socket::unregister_all_server_setting() noexcept
{
	//list<object_ptr<host_agent::server_entity>>	listServerRegisterInfo;

	//scoped_lock(m_listServerRegisterSetting)
	//{
	//	listServerRegisterInfo	 = std::move(m_listServerRegisterSetting);
	//}

	//// 1) 모두 제거한다.
	//for(auto& iter: listServerRegisterInfo)
	//{
	//	// - Socket을 Reset한다.
	//	iter->info_server_register.server_setting.id.i64Code	 = 0;
	//	
	//	// Notify) Server setting이 삭제되었음을 Notify한다.
	//	g_pservice->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::SERVER_SETTING_UNREGISTER, reinterpret_cast<uintptr_t>(iter.get())));
	//	g_pservice->m_pgroup_admin_client->send(MESSAGE_ADMIN_SERVER_SETTING_REMOVE::MAKE_BUFFER(iter.get()));
	//}

	// Return) 
	return	true;
}


bool host_agent::socket::update_server_setting(const server_admin::sSERVER_REGISTER& /*_InfoServerRegister*/)
{
	//// declare) 
	//object_ptr<host_agent::server_entity>	pserver_entity_info;

	//scoped_lock(m_listServerRegisterSetting)
	//{
	//	for (auto& iter : m_listServerRegisterSetting)
	//	{
	//		if (iter->info_server_register.server_setting.id == _InfoServerRegister.server_setting.id)
	//		{
	//			// - 찾은 객체를 저장!
	//			pserver_entity_info = iter;

	//			// - 값을 복사한다.
	//			pserver_entity_info->info_server_register = _InfoServerRegister;

	//			// Break) 
	//			break;
	//		}
	//	}
	//}

	//// check) 
	//RETURN_IF(pserver_entity_info.empty(), false);

	//// 2) 메시지를 전송한다.

	//// 4) Server setting이 삭제되었음을 Notify한다.
	//g_pservice->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::SERVER_SETTING_UPDATE, reinterpret_cast<uintptr_t>(pserver_entity_info.get())));
	//g_pservice->m_pgroup_admin_client->send(MESSAGE_ADMIN_SERVER_SETTING_UPDATE::MAKE_BUFFER(pserver_entity_info));

	// Return) 
	return	true;
}

bool host_agent::socket::request_admin_send_host_setting(const server_admin::sHOST_SETTING& _host_setting)
{
	return	send(server_admin::message_SERVER_ADMIN_SEND_HOST_SETTING(_host_setting));
}

bool host_agent::socket::request_admin_send_admin_server_address(std::string_view _strAdminServerAddress)
{
	return	send(server_admin::message_SERVER_ADMIN_SEND_ADMINSERVERADDRESS(_strAdminServerAddress));
}

void host_agent::socket::set_host_info(const server_admin::sHOST_INFO& _host_info)
{
	// 1) 값을 설정한다.
	scoped_lock(m_csData)
	{
		m_info_host	 = _host_info;
	}

	// Notify) Host Info가 변경되었음으로 통보한다.
	g_pservice->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::SEND_HOST_INFO, reinterpret_cast<uintptr_t>(this)));
	g_pservice->m_pgroup_admin_client->send(server_admin::message_SERVER_ADMIN_SEND_HOSTINFO(_host_info));
}

void host_agent::socket::set_host_setting(const server_admin::sHOST_SETTING& _host_setting)
{
	// 1) 값을 설정한다.
	scoped_lock(m_csData)
	{
		m_setting_host = _host_setting;
	}

	// Notify) Host Info가 변경되었음으로 통보한다.
	g_pservice->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::SEND_HOST_INFO, reinterpret_cast<uintptr_t>(this)));
	g_pservice->m_pgroup_admin_client->send(server_admin::message_SERVER_ADMIN_SEND_HOST_SETTING(_host_setting));
}

void host_agent::socket::set_host_status(const server_admin::sHOST_STATUS& _HostStatus)
{
	// 1) 값을 설정한다.
	scoped_lock(m_csData)
	{
		m_statusHost	 = _HostStatus;
	}

	// Notify) Host Info가 변경되었음으로 통보한다.
	g_pservice->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::HOST_STATUS_UPDATE, reinterpret_cast<uintptr_t>(this)));
	g_pservice->m_pgroup_admin_client->send(server_admin::message_SERVER_ADMIN_HOST_STATUS_UPDATE(_HostStatus));
}

bool host_agent::socket::request_register_server_entity(const server_admin::sSERVER_REGISTER& _server_register, bool _bUpdate)
{
	return	send(server_admin::message_SERVER_ADMIN_SERVER_SETTING_REGISTER(_server_register, _bUpdate));
}

bool host_agent::socket::request_unregister_server_entity(const server_admin::sSERVER_REGISTER& _server_register)
{
	return	send(server_admin::message_SERVER_ADMIN_SERVER_SETTING_UNREGISTER(m_info_host.hid, _server_register.server_setting.id));
}

bool host_agent::socket::request_update_server_entity(const server_admin::sSERVER_REGISTER& _server_register, bool _bUpdate)
{
	return	send(server_admin::message_SERVER_ADMIN_SERVER_INSTANCE_SETTING_UPDATE(m_info_host.hid, _server_register.server_setting.id, _server_register, _bUpdate));
}

bool host_agent::socket::request_version_update(const server_admin::sSERVER_REGISTER& _server_register)
{
	return	send(server_admin::message_SERVER_ADMIN_REQUEST_VERSION_UPDATE(m_info_host.hid, _server_register.server_setting.id));
}

bool host_agent::socket::request_init_process(const server_entity& _serer_register_info)
{
	return	send(server_admin::message_SERVER_ADMIN_REQUEST_INIT_SERVICE(m_info_host.hid, _serer_register_info.info_server_register.server_setting.id));
}

bool host_agent::socket::request_start_process(const server_entity& _serer_register_info, const std::string& _strSetting)
{
	return	send(server_admin::message_SERVER_ADMIN_REQUEST_START_SERVICE(m_info_host.hid, _serer_register_info.info_server_register.server_setting.id, _strSetting));
}

bool host_agent::socket::request_kill_process(const server_entity& _serer_register_info)
{
	return	send(server_admin::message_SERVER_ADMIN_REQUEST_KILL_SERVICE(m_info_host.hid, _serer_register_info.info_server_register.server_setting.id));
}

void host_agent::socket::set_event_list(int /*_count*/, const time_event::sEVENT_SETTING* /*_list_event_info*/)
{
	//// 1) 모두 Reset한다.
	//if(m_pmanager_event.exist())
	//{
	//	m_pmanager_event->unregister_event_all();
	//}

	//// check) 
	//RETURN_IF(_count==0, );

	//// 2) 모두 Add한다.
	//for(int i=0; i<_count; ++i, ++p_listEventInfo)
	//{
	//	add_event(p_listEventInfo);
	//}
}

void host_agent::socket::add_event(const time_event::sEVENT_SETTING& /*_event_info*/)
{
	//// 1) Event 객체를 생성한다.
	//auto	pRegieterEventInfo	 = make_object<server_event>();

	//// 2) 값을 저장한다.
	//pRegieterEventInfo->set_time_event_info(*_event_info);

	//// 3) 추가한다.
	//m_pmanager_event->RegisterEvent(pRegieterEventInfo);

	//// 4) Notify한다.
	//{
	//	sMESSAGE_NOTIFY	msgNotify(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::EVENT::ADD_EVENT, reinterpret_cast<uintptr_t>(pRegieterEventInfo.get()));
	//	transmit_message(msgNotify);
	//}
}

void host_agent::socket::remove_event(const time_event::sEVENT_SETTING& /*_event_info*/)
{
	//// 1) 제거한다.
	//auto	pRegieterEventInfo	 = m_pmanager_event->UnregisterEvent(_event_info->id_event);

	//// check) 지우지 못했으면 끝낸다.
	//RETURN_IF(pRegieterEventInfo.empty(), );

	//// 2) Notify한다.
	//{
	//	sMESSAGE_NOTIFY	msgNotify(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::EVENT::REMOVE_EVENT, reinterpret_cast<uintptr_t>(pRegieterEventInfo.get()));
	//	transmit_message(msgNotify);
	//}
}

void host_agent::socket::update_event(const time_event::sEVENT_SETTING& /*_event_info*/)
{
	//// 1) 객체를 찾는다.
	//auto	pRegieterEventInfo	 = m_pmanager_event->FindEvent(_event_info->id_event);

	//// check) 
	//RETURN_IF(pRegieterEventInfo.empty(), );

	//// 2) 값을 Update한다.
	//pRegieterEventInfo->set_time_event_info(*_event_info);

	//// 3) Notify한다.
	//{
	//	sMESSAGE_NOTIFY	msgNotify(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::EVENT::UPDATE_EVENT, reinterpret_cast<uintptr_t>(pRegieterEventInfo.get()));
	//	transmit_message(msgNotify);
	//}
}

void host_agent::socket::execute_event(const time_event::sEVENT_SETTING& /*_event_info*/)
{
	//// 1) 객체를 찾는다.
	//auto	pRegieterEventInfo	 = m_pmanager_event->FindEvent(_event_info->id_event);

	//// check) 
	//RETURN_IF(pRegieterEventInfo.empty(), );

	//// 2) 값을 Update한다.
	//pRegieterEventInfo->set_time_event_info(*_event_info);

	//// 2) Notify한다.
	//{
	//	sMESSAGE_NOTIFY	msgNotify(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::EVENT::EXECUTE_EVENT, reinterpret_cast<uintptr_t>(pRegieterEventInfo.get()));
	//	transmit_message(msgNotify);
	//}
}


void host_agent::socket::set_authority(uint64_t _authority)
{
	m_authority = _authority;
}

void host_agent::socket::set_log_index(uint64_t _index_logIn)
{
	m_indexLog = _index_logIn;
}

object_ptr<host_agent::server_entity> host_agent::socket::_FindServerEntity(const server_admin::sID& /*_idServer*/)
{
	//for (auto& iter : m_listServerRegisterSetting)
	//{
	//	RETURN_IF(iter->info_server_register.server_setting.id == _idServer, iter);
	//}

	// Return)
	return	object_ptr<host_agent::server_entity>();
}
