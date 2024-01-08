#include "pch.h"


server_entity::host::group::group(std::string_view _name) :
	group::list_sender<socket>(),
	net::io::messageable::Nbase(),
	Nmessage_transmitter(),
	Nobject_state(),
	Ninitializable(),
	Nstartable(),
	Nnameable(_name)
{
}

server_entity::host::group::~group()
{
}

void server_entity::host::group::on_initializing(context& /*_context*/)
{
}

void server_entity::host::group::on_initialize(context& /*_context*/)
{
}

void server_entity::host::group::on_destroying()
{
}

void server_entity::host::group::on_destroy()
{
}

void server_entity::host::group::on_starting(context& /*_context*/)
{
}

void server_entity::host::group::on_start(context& /*_context*/)
{
	// 1) 입장을 받기 시작~
	enable_member_enter(true);
}

void server_entity::host::group::on_stopping()
{
	// 1) 입장을 금지
	disable_member_enter();

	// 2) 모든 Group에 접속한 모든 Member를 모두 접속 종료시킨다.
	for_each_member([=](server_entity::host::socket* _pmember)->bool
	{
		_pmember->closesocket(); 
		return true;
	});

	// 3) Leavel All Memberl
	leave_all();
}

void server_entity::host::group::on_stop()
{
}

result_code server_entity::host::group::on_member_entering(server_entity::host::socket* /*_pmember*/, sMESSAGE& /*_msg*/)
{
	// return) 
	return eRESULT::SUCCESS;
}

void server_entity::host::group::on_member_entered(server_entity::host::socket* _pmember, sMESSAGE& /*_msg*/)
{
	// log) 
	LOG_PROGRESS_IMPORTANT << "@ HOST['" << _pmember->m_info_host.host_name << "'] registered!";

	//// declare)
	//vector<server_admin::sSERVER_REGISTER> vector_server_register;

	//// 1) Register 정보를 복사해 놓는다.
	//scoped_lock(_pmember->m_vector_server_register)
	//{
	//	vector_server_register = _pmember->m_vector_server_register;
	//}

	//// 2) 등록한다.
	//for(auto& iter: vector_server_register)
	//{
	//	register_instance(_pmember, iter);
	//}

	// 2) _pmember의 Mediator를 this로 설정한다.
	_pmember->register_messageable(this);
}

uintptr_t server_entity::host::group::on_member_leaving(server_entity::host::socket* _pmember, sMESSAGE& /*_msg*/)
{
	//// 1) unregister 한다.
	//unregister_instance(_pmember);

	// 2) unregister Mediator한다.
	_pmember->unregister_messageable(this);

	// log) 
	LOG_PROGRESS_IMPORTANT << "@ HOST['" << _pmember->m_info_host.host_name << "'] unregistered!";

	// return) 
	return 0;
}

void server_entity::host::group::on_member_leaved(server_entity::host::socket* /*_pmember*/, sMESSAGE& /*_msg*/, uintptr_t /*_result*/)
{
}

result_code server_entity::host::group::on_message(sMESSAGE_NETWORK& _msg)
{
	try
	{
		// 1) Message Map
		EXECUTE_CGMESSAGE_MAP(_msg);
	}
	catch (...)
	{
		// - Exception을 처리한다.
	}

	// return) Message 처리를 하지 못했다!!!
	return eRESULT::BYPASS;
}

BEGIN_CGMESSAGE_MAP(server_entity::host::group)
	CGMESSAGE_TYPE_SUB(eMESSAGE::SYSTEM::NETWORK, ((sMESSAGE_BUFFER&)_msg).buf_message.front<uint32_t>(4))
		// 1) System
		//ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::SEND_SERVER_SETTING,				on_message_SERVER_ADMIN_SEND_SERVER_SETTING,			sMESSAGE_NETWORK);

		// 2) host Info
		ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::HOST_STATUS_UPDATE,				on_message_SERVER_ADMIN_HOST_STATUS_UPDATE,				sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::SERVICE_STATE_UPDATE,				on_message_SERVER_ADMIN_SERVICE_STATE_UPDATE,			sMESSAGE_NETWORK);

		//// 3) server info		
		//ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::UPDATE_SERVER_INFO,				on_message_SERVER_ADMIN_UPDATE_SERVER_INFO,				sMESSAGE_NETWORK);

		// 4) server_entity info
		ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::SERVER_INSTANCE_REGISTER,			on_message_SERVER_ADMIN_SERVER_INSTANCE_REGISTER,		sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::SERVER_INSTANCE_UNREGISTER,		on_message_SERVER_ADMIN_SERVER_INSTANCE_UNREGISTER,		sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::SERVER_INSTANCE_ENTER,			on_message_SERVER_ADMIN_SERVER_INSTANCE_ENTER,			sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::SERVER_INSTANCE_LEAVE,			on_message_SERVER_ADMIN_SERVER_INSTANCE_LEAVE,			sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::SERVER_INSTANCE_UPDATE,			on_message_SERVER_ADMIN_SERVER_INSTANCE_UPDATE,			sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::SERVER_INSTANCE_SETTING_UPDATE,	on_message_SERVER_ADMIN_SERVER_INSTANCE_SETTING_UPDATE,	sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::SERVER_INSTANCE_STATUS_UPDATE,	on_message_SERVER_ADMIN_SERVER_INSTANCE_STATUS_UPDATE,	sMESSAGE_NETWORK);

		// 5) binder
		ON_CGMESSAGE(eMESSAGE::NETWORK::SOCKET::BINDER_REGISTER,				on_message_NETWORK_SOCKET_BINDER_REGISTER,				sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE::NETWORK::SOCKET::BINDER_UNREGISTER,				on_message_NETWORK_SOCKET_BINDER_UNREGISTER,			sMESSAGE_NETWORK);

		// 6) time_event info
		ON_CGMESSAGE(eMESSAGE::SERVER::TIME_EVENT::EVENT_REGISTER,				on_message_SERVER_TIME_EVENT_EVENT_REGISTER,			sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE::SERVER::TIME_EVENT::EVENT_UNREGISTER,			on_message_SERVER_TIME_EVENT_EVENT_UNREGISTER,			sMESSAGE_NETWORK);

		//ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::TRAFFIC_INFO_UPDATE,				on_message_SERVER_ADMIN_TRAFFIC_INFO_UPDATE,			sMESSAGE_NETWORK);

		// 7) server update
		ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::SERVER_PID_UPDATE,				on_message_SERVER_ADMIN_SERVER_PID_UPDATE,				sMESSAGE_NETWORK);

		// 8) log
		ON_CGMESSAGE(eMESSAGE::SYSTEM::LOG_,									on_message_SYSTEM_LOG,									sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::ERROR_,							on_message_SERVER_ADMIN_ERROR,							sMESSAGE_NETWORK);

	END_SUB_CGMESSAGE_MAP
END_CGMESSAGE_MAP

result_code server_entity::host::group::on_message_SERVER_ADMIN_HOST_STATUS_UPDATE(sMESSAGE_NETWORK& _msg)
{
	// 1) Socket 객체를 얻는다.
	auto psource = _msg.get_source<server_entity::host::socket>();

	// 2) Register Infof를 얻는다.
	auto host_status = _msg.buf_message.front<server_admin::sHOST_STATUS>(8);

	// 3) 설정한다.
	psource->set_host_status(host_status);

	// return) 
	return eRESULT::DONE;
}

result_code server_entity::host::group::on_message_SERVER_ADMIN_SERVICE_STATE_UPDATE(sMESSAGE_NETWORK& /*_msg*/)
{
	//// 1) Socket 객체를 얻는다.
	//auto		psource = _msg.get_source<server_entity::host::socket>();

	// notify) Notify한다.
	//g_pservice->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::TIME_EVENT::EVENT_UPDATE, error_code));
	//g_pservice->m_pgroup_client->send(MESSAGE_ADMIN_SERVER_SETTING_UPDATE::MAKE_BUFFER(error_code));

	// return) 
	return eRESULT::DONE;
}

//result_code server_entity::host::group::on_message_SERVER_ADMIN_UPDATE_SERVER_INFO(sMESSAGE_NETWORK& _msg)
//{
//	// check) Message의 길이를 검사한다.
//	CGASSERT_ERROR(_msg.buf_message.size() > 8);
//
//	// check) Message의 길이를 검사한다
//	THROW_IF(_msg.buf_message.size() <= 8, throwable::failure(eRESULT::INVALID_MESSAGE_LENGTH, 0, "invalid message length" __FUNCTION__));
//
//	// 1) 읽기용 버퍼 포인터를 생성한다.
//	auto buf_read = _msg.buf_message + offset(8);
//
//	// declare)
//	object_ptr<server_entity::manager_instance> pmanager_server_instance = g_pservice->m_pmanager_server_instance;
//
//	// check)
//	CGASSERT_ERROR(pmanager_server_instance.exist())
//
//	// check)
//	RETURN_IF(pmanager_server_instance.empty(), eRESULT::DONE)
//
//	// 2) read data
//	auto temp_info = buf_read.extract<server_admin::server_info_update>();
//
//	// 3) request update to server entity manager
//	pmanager_server_instance->update_instance(std::move(temp_info));
//
//	// return) 
//	return eRESULT::DONE;
//}

result_code server_entity::host::group::on_message_SERVER_ADMIN_SERVER_INSTANCE_REGISTER(sMESSAGE_NETWORK& _msg)
{
	// 1) Socket 객체를 얻는다.
	auto psource = _msg.get_source<server_entity::host::socket>();

	// 2) 읽기용 버퍼 포인터를 생성한다.
	auto buf_read = _msg.buf_message + offset(8);

	// 3) Server setting 정보를 읽는다.
	auto server_setting_info = buf_read.extract<server_admin::sSERVER_REGISTER>();

	// 4) get server instance manager
	auto pserver_instance_manager = g_pservice->m_pmanager_server_instance;

	// 5) register server_setting 
	pserver_instance_manager->register_instance(server_setting_info, psource);

	// return) 
	return eRESULT::DONE;
}

result_code server_entity::host::group::on_message_SERVER_ADMIN_SERVER_INSTANCE_UNREGISTER(sMESSAGE_NETWORK& _msg)
{
	// check) 
	CGASSERT_ERROR(_msg.buf_message.size() == (8 + sizeof(server_admin::sHOST_ID) + sizeof(server_admin::sID)));

	// check) 
	THROW_IF(_msg.buf_message.size() != (8 + sizeof(server_admin::sHOST_ID) + sizeof(server_admin::sID)), throwable::failure(eRESULT::LENGTH_ERROR));

	// declare)
	auto buf_read = _msg.buf_message + offset(8);

	// 2) host ID/ Server ID
	auto hid = buf_read.extract<server_admin::sHOST_ID>();
	auto id_server = buf_read.extract<server_admin::sID>();

	// 4) get server instance manager
	auto pserver_instance_manager = g_pservice->m_pmanager_server_instance;

	// 5) register server_setting 
	pserver_instance_manager->unregister_instance(id_server);

	// return) 
	return eRESULT::DONE;
}

result_code server_entity::host::group::on_message_SERVER_ADMIN_SERVER_INSTANCE_ENTER(sMESSAGE_NETWORK& _msg)
{
	// 1) get source
	auto psource = _msg.get_source<server_entity::host::socket>();

	// 2) extract server inifo
	auto temp_server_info = (_msg.buf_message + offset(8)).extract<server_admin::server_info>();

	// 3) register server info to manager
	g_pservice->m_pmanager_server_instance->enter_instance(std::move(temp_server_info), psource);

	// notify)
	//g_pservice->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::SERVER_INSTANCE_ENTER, reinterpret_cast<uintptr_t>(pserver_entity_info.get())));
	g_pservice->m_pgroup_client->send(_msg.buf_message);

	// return) 
	return eRESULT::DONE;
}

result_code server_entity::host::group::on_message_SERVER_ADMIN_SERVER_INSTANCE_LEAVE(sMESSAGE_NETWORK& /*_msg*/)
{
	//// 1) Socket 객체를 얻는다.
	//auto		psource		 = _msg.get_source<server_entity::host::socket>();

	//// 2) 읽기용 버퍼 포인터를 생성한다.
	//shared_buffer	buf_read		 = _msg.buf_message + 8;

	//// 3) host ID/ Server ID
	//auto		hid			 = buf_read.extract<server_admin::sHOST_ID>();
	//auto		id_server	 = buf_read.extract<server_admin::sID>();

	//// declare) 
	//object_ptr<server_entity::instance>	pserver_entity_info;

	//// 4) Enter처리를 한다.
	//scoped_lock(psource->m_listServerRegisterSetting)
	//{
	//	auto	iter	 = psource->m_listServerRegisterSetting.begin();
	//	auto	iterEnd	 = psource->m_listServerRegisterSetting.end();

	//	for(; iter!=iterEnd; ++iter)
	//	{
	//		if((*iter)->registered.server_setting.id== id_server)
	//		{
	//			// - 찾은 객체를 저장!
	//			pserver_entity_info = *iter;

	//			// - 등록된 서버가 아닐 경우 지우고 등록된 경우 Socket 접속 정보만 Reset한다.
	//			if(pserver_entity_info->registered.bRegisterted)
	//			{
	//				pserver_entity_info->status_server.id.code  = 0;
	//				pserver_entity_info->status_server.state		 = eOBJECT_STATE::NONE;
	//			}
	//			// - 제거한다.
	//			else
	//			{
	//				psource->m_listServerRegisterSetting.erase(iter);
	//			}

	//			// Break) 
	//			break;
	//		}
	//	}
	//}

	//// check) 
	//RETURN_IF(pserver_entity_info.empty(), 0);

	//// notify) Server setting이 삭제되었음을 Notify한다.
	////g_pservice->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::SERVER_INSTANCE_LEAVE, reinterpret_cast<uintptr_t>(pserver_entity_info.get())));
	//g_pservice->m_pgroup_client->send(MESSAGE_CONTROL_SERVER_LEAVE::MAKE_BUFFER(pserver_entity_info));

	// return) 
	return eRESULT::DONE;
}

result_code server_entity::host::group::on_message_SERVER_ADMIN_SERVER_INSTANCE_UPDATE(sMESSAGE_NETWORK& _msg)
{
	// 1) Socket 객체를 얻는다.
	auto psource = _msg.get_source<server_entity::host::socket>();

	// 2) 읽기용 버퍼 포인터를 생성한다.
	auto buf_read = _msg.buf_message + offset(8);

	// 3) host ID/ Server ID
	/*auto hid =*/ buf_read.extract<server_admin::sHOST_ID>();
	auto id_server = buf_read.extract<server_admin::sID>();

	// 4) Server setting 정보를 읽는다.
	auto server_setting_info = buf_read.extract<server_admin::sSERVER_REGISTER>();

	// 5) 생성하여 값을 복사한다.
	bool bResult = psource->update_server_setting(server_setting_info);

	// check)
	RETURN_IF(bResult == false, eRESULT::DONE);

	// return) 
	return eRESULT::DONE;
}

result_code server_entity::host::group::on_message_SERVER_ADMIN_SERVER_INSTANCE_SETTING_UPDATE(sMESSAGE_NETWORK& _msg)
{
	// 1) Socket 객체를 얻는다.
	auto psource = _msg.get_source<server_entity::host::socket>();

	// declare)
	auto buf_recv = _msg.buf_message + offset(sizeof(MSG_HEADER));
	auto info_server_setting = buf_recv.extract<server_admin::sSERVER_SETTING_INFO>();
	auto infoServer = buf_recv.extract<shared_buffer>();

	scoped_lock(psource->m_lockable_data)
	{
		// 2) Server setting Info를 설정한다.
		if (info_server_setting.id.key != 0)
		{
			// - idCode 값의 유지를 위해 저장해 놓는다.
			info_server_setting.id.code = psource->m_info_server_setting.id.code;

			// - 저장한다.
			psource->m_info_server_setting = info_server_setting;
		}

		// - infoServer값을 저장한다.
		psource->m_info_server = infoServer;
	}

	// notify)
	g_pservice->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, CGNOTIFY_HOST_SOCKET_UPDATE, reinterpret_cast<uintptr_t>(psource)));
	g_pservice->m_pgroup_client->send(MESSAGE_ADMIN_HOST_SOCKET_UPDATE::MAKE_BUFFER(psource->m_info_host, psource->m_status_host));

	// return)
	return eRESULT::DONE;
}

result_code server_entity::host::group::on_message_SERVER_ADMIN_SERVER_INSTANCE_STATUS_UPDATE(sMESSAGE_NETWORK& /*_msg*/)
{
//	//// 2) Message Parsing
//	//shared_buffer buf_read = _msg.buf_message + offset(8);
//
//	//// 3) host ID/ Server ID
//	//auto hid	   = buf_read.extract<server_admin::sHOST_ID>();
//	//auto id_server = buf_read.extract<server_admin::sID>();
//
//	//// 4) server_entity info를 얻는다.
//	//auto status_server = buf_read.extract<server_admin::sSERVER_STATUS>();
//
//	//// declare) 
//	//auto pserver_entity_info = g_pservice->m_pgroup_server::host->find_instance(hid, id_server);
//
//	//// check) 
//	//RETURN_IF(pserver_entity_info.empty(), 1);
//
//	//// check) 
//	//RETURN_IF(pserver_entity_info->psocket_server.exist(), 1);
//
//	//// 5) 값을 복사한다.
//	//pserver_entity_info->status_server = status_server;

	// return) 
	return eRESULT::DONE;
}

//result_code server_entity::host::group::on_message_SERVER_ADMIN_SERVER_STATUS_UPDATE(sMESSAGE_NETWORK& /*_msg*/)
//{
	//// 1) Socket 객체를 얻는다.
	//auto psource = _msg.get_source<server_entity::host::socket>();

	//// 2) Message Parsing
	//auto buf_read = _msg.buf_message + offset(8);

	//// 3) host ID/ Server ID
	//auto hid = buf_read.extract<server_admin::sHOST_ID>();
	//int count = buf_read.extract<int>();

	//// 4) Server의 갯수만큼 데이터를 얻어낸다.
	//scoped_lock(psource->m_lockable_data)
	//{
	//	for (int i = 0; i<count; ++i)
	//	{
	//		// - Server Status를 얻는다.
	//		auto server_status = buf_read.extract<server_admin::sSERVER_STATUS>();

	//		// - 해당 ServerEntity를 얻는다.
	//		auto pserver_entity = g_pservice->m_pgroup_server::host->find_instance(hid, server_status.id);

	//		// check)
	//		CONTINUE_IF(pserver_entity.empty());

	//		// - status를 변경한ㄷ다.
	//		pserver_entity->status_server = server_status;

	//		// notify) Server가 추가되었음을 Notify한다.
	//		{
	//			g_pservice->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::SERVER_INSTANCE_UPDATE, reinterpret_cast<uintptr_t>(pserver_entity.get())));
	//			g_pservice->m_pgroup_client->send(pserver_entity->message_SERVER_ADMIN_SERVER_INSTANCE_UPDATE());
	//		}
	//	}
	//}
//
//	// return) 
//	return eRESULT::DONE1;
//}

result_code server_entity::host::group::on_message_NETWORK_SOCKET_BINDER_REGISTER(sMESSAGE_NETWORK& /*_msg*/)
{
	//// 1) Socket 객체를 얻는다.
	//auto psource = _msg.get_source<server_entity::host::socket>();

	//// 2) Event 정보를 얻는다.
	//auto event_info = _msg.buf_message.front<time_event::sEVENT_SETTING>(8);

	//// 3) Event를 추가한다.
	//psource->add_event(event_info);

	// return) 
	return eRESULT::DONE;
}

result_code server_entity::host::group::on_message_NETWORK_SOCKET_BINDER_UNREGISTER(sMESSAGE_NETWORK& /*_msg*/)
{
	//// 1) Socket 객체를 얻는다.
	//auto psource = _msg.get_source<server_entity::host::socket>();

	//// 2) Event 정보를 얻는다.
	//auto event_info = _msg.buf_message.front<time_event::sEVENT_SETTING>(8);

	//// 3) Event를 추가한다.
	//psource->remove_event(event_info);

	// return) 
	return eRESULT::DONE;
}

result_code server_entity::host::group::on_message_SERVER_TIME_EVENT_EVENT_REGISTER(sMESSAGE_NETWORK& _msg)
{
	// 1) Socket 객체를 얻는다.
	auto psource = _msg.get_source<server_entity::host::socket>();

	// 2) Event 정보를 얻는다.
	auto event_info = _msg.buf_message.front<time_event::sEVENT_SETTING>(8);

	// 3) Event를 추가한다.
	psource->add_event(event_info);

	// return) 
	return eRESULT::DONE;
}

result_code server_entity::host::group::on_message_SERVER_TIME_EVENT_EVENT_UNREGISTER(sMESSAGE_NETWORK& _msg)
{
	// 1) Socket 객체를 얻는다.
	auto psource = _msg.get_source<server_entity::host::socket>();

	// 2) Event 정보를 얻는다.
	auto event_info = _msg.buf_message.front<time_event::sEVENT_SETTING>(8);

	// 3) Event를 추가한다.
	psource->remove_event(event_info);

	// return) 
	return eRESULT::DONE;
}


result_code server_entity::host::group::on_message_SERVER_ADMIN_SERVER_PID_UPDATE(sMESSAGE_NETWORK& /*_msg*/)
{
	//// 1) shared_buffer를 얻는다.
	//auto bufMessage = _msg.buf_message + offset(8);

	//// 2) 값을 읽어온다
	///*auto&	hid =*/ bufMessage.extract<server_admin::sHOST_ID>();
	//auto& id = bufMessage.extract<server_admin::sID>();
	//auto pid = bufMessage.extract<uint64_t>();

	//// check) 
	//CGASSERT_ERROR(pid!=0);

	//// declare) 
	//object_ptr<server_entity::instance> pserver_entity;
	//object_ptr<server_entity::instance> pserver_entity_remove;

	//// 3) 업데이트한다.
	//scoped_lock(m_lockable_register)
	//{
	//	// - 해당 Server setting을 얻는다.
	//	auto iter_find = m_map_server_register.find(id.key);

	//	// check) 찾지 못했으면 그냥 끝낸다.
	//	RETURN_IF(iter_find == m_map_server_register.end(), 1);

	//	// declare)
	//	pserver_entity = iter_find->second;

	//	// - pid를 업데이트한다.
	//	pserver_entity->registered.pid	 = pid;

	//	// - 같은 host에 같은 pid가 존재하는지 확인한다.
	//	if(pid!=0)
	//	{
	//		auto iterUR = m_vector_server_unregister.begin();
	//		auto iterUREnd = m_vector_server_unregister.end();

	//		for(;iterUR != iterUREnd; ++iterUR)
	//		{
	//			// check)
	//			CGASSERT_ERROR((*iterUR)->psocket_server.exist());

	//			if((*iterUR)->psocket_server->m_pid == pid)
	//			{
	//				pserver_entity_remove = *iterUR;
	//				m_vector_server_unregister.erase(iterUR);
	//				break;
	//			}
	//		}

	//		// - 찾았다면 설정...
	//		if(pserver_entity_remove.exist())
	//		{
	//			pserver_entity->psocket_server = std::move(pserver_entity_remove->psocket_server);
	//			pserver_entity->status_server  = pserver_entity_remove->status_server;

	//			pserver_entity->psocket_server->m_pserver_entity = pserver_entity;
	//		}
	//	}

	//	// notify) ...
	//	if (pserver_entity_remove.exist())
	//	{
	//		g_pservice->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::SERVER_INSTANCE_UNREGISTER, reinterpret_cast<uintptr_t>(pserver_entity_remove.get())));
	//		g_pservice->m_pgroup_client->send(MESSAGE_ADMIN_SERVER_SETTING_REMOVE::MAKE_BUFFER(pserver_entity_remove->registered.server_setting.id.key));
	//	}

	//	// notify) Notify한다.
	//	g_pservice->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::SERVER_INSTANCE_UPDATE, reinterpret_cast<uintptr_t>(pserver_entity.get())));
	//	g_pservice->m_pgroup_client->send(pserver_entity->message_SERVER_ADMIN_SERVER_INSTANCE_UPDATE());
	//}

	// return) 
	return eRESULT::DONE;
}

result_code server_entity::host::group::on_message_SERVER_ADMIN_ERROR(sMESSAGE_NETWORK& _msg)
{
	// 1) Message Parsing
	auto buf_read = _msg.buf_message + offset(8);

	// 2) host ID/ Server ID
	auto hid = buf_read.extract<server_admin::sHOST_ID>();
	auto id_server = buf_read.extract<server_admin::sID>();

	// 3) Error Code를 읽는다.
	auto error_code = buf_read.extract<int>();

	// notify) Notify한다.
	g_pservice->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::ERROR_, error_code));
	g_pservice->m_pgroup_client->send(MESSAGE_ADMIN_ERROR_::MAKE_BUFFER(error_code));

	// return) 
	return eRESULT::DONE;
}

//result_code server_entity::host::group::on_message_SERVER_ADMIN_REQUEST_LOG(sMESSAGE_NETWORK& _msg)
//{
//	// 1) Socket 객체를 얻는다.
//	auto psource = _msg.get_source<server_entity::host::socket>();
//
//	// 2) Message Parsing
//	auto buf_read = _msg.buf_message + offset(8);
//
//	// 3) host ID/ Server ID
//	auto hid	   = buf_read.extract<server_admin::sHOST_ID>();
//	auto id_server = buf_read.extract<server_admin::sID>();
//
//	// 4) Login Index를 읽는다.
//	auto iLogIndex = buf_read.extract<int64_t>();
//
//	// 5) LogIndex가 0보다 작으면 Log Index를 Reset한다.
//	psource->set_log_index(iLogIndex);
//
//	// return) 
//	return eRESULT::DONE;
//}

result_code server_entity::host::group::on_message_SYSTEM_LOG(sMESSAGE_NETWORK& _msg)
{
	// check)
	RETURN_IF(g_plogger.empty(), eRESULT::DONE);

	// 1) 버퍼를 임시로...
	auto buf_read = _msg.buf_message + offset(8);

	// 2) 생성한다.
	auto ptemp_log_record = make_object<LOG_RECORD>();
	ptemp_log_record->type = buf_read.extract<eLOG_TYPE>();
	ptemp_log_record->level = buf_read.extract<int>();
	ptemp_log_record->result = buf_read.extract<eRESULT>();
	ptemp_log_record->attribute = buf_read.extract<uint64_t>();;
	ptemp_log_record->time_occure = buf_read.extract<chrono::time::utc::time_point>();
	ptemp_log_record->buf_message = buf_read;

	// 3) Message
	g_plogger->trace(ptemp_log_record);

	// return) 
	return eRESULT::DONE;
}
