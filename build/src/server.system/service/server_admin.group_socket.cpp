#include "pch.h"

using namespace CGDK;

result_code server_system::host::group_socket::on_member_entering(server_system::host::socket* _pmember, sMESSAGE& /*_msg*/)
{
	// 1) Register Messageable한다.
	_pmember->register_messageable(this);

	// trace) 
	LOG_PROGRESS << "> SERVER['"sv << "'] connected ("sv << _pmember->remote_endpoint().to_string() << ")"sv;

	// return) 
	return eRESULT::SUCCESS;
}

void server_system::host::group_socket::on_member_entered(server_system::host::socket* /*_pmember*/, sMESSAGE& /*_msg*/)
{
}

uintptr_t server_system::host::group_socket::on_member_leaving(server_system::host::socket* _pmember, uintptr_t _param)
{
	// 1) unregister Mediator한다.
	_pmember->unregister_messageable(this);

	// trace)
	LOG_PROGRESS << "< SERVER['"sv << "'] disconnected"sv;

	// 2) remove instances
	this->_process_remove_instances_in_socket(_pmember);

	// return) 
	return _param;
}

void server_system::host::group_socket::on_member_leaved(member_t* /*_pmember*/, uintptr_t /*_result*/)
{
}


void server_system::host::group_socket::_process_remove_instances_in_socket(server_system::host::socket* _pmember)
{
	// 1) get service
	auto pservice = dynamic_pointer_cast<server_system::Xservice>(server_system::get_service());

	// check)
	RETURN_IF(pservice.empty(), );

	// 2) remove service instances from socket
	auto map_service_instances = _pmember->unregister_service_instance_all();

	// 3) register service instance
	auto pservice_instance_manager = pservice->get_service_instance_manager();
	if (pservice_instance_manager.exist())
	{
		for (auto& iter : map_service_instances)
		{
			pservice_instance_manager->unregister_instance(iter.first);
		}
	}

	// 4) remove host instances from socket
	auto map_host_instances = _pmember->unregister_host_instance_all();

	// 5) unregister fron host instance manager
	auto phost_instance_manager = pservice->get_host_instance_manager();
	if (phost_instance_manager.exist())
	{
		for (auto& iter : map_host_instances)
		{
			phost_instance_manager->unregister_instance(iter.first);
		}
	}
}

result_code server_system::host::group_socket::on_message(sMESSAGE_NETWORK& _msg)
{
	try
	{
		BEGIN_NESTED_CGMESSAGE_MAP
			CGMESSAGE_TYPE_SUB(eMESSAGE::SYSTEM::NETWORK, ((sMESSAGE_BUFFER&)_msg).buf_message.front<uint32_t>(4))
				ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::INSTANCE_UPDATE_STATE, on_message_SERVER_ADMIN_INSTANCE_UPDATE_STATE, sMESSAGE_NETWORK);
				ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::INSTANCE_UPDATE_INFO, on_message_SERVER_ADMIN_INSTANCE_UPDATE_INFO, sMESSAGE_NETWORK);
				ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::INSTANCE_UPDATE_STATUS, on_message_SERVER_ADMIN_INSTANCE_UPDATE_STATUS, sMESSAGE_NETWORK);
				ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_RESULT, on_message_SERVER_ADMIN_REQUEST_RESULT, sMESSAGE_NETWORK);
				ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::HOST_UPDATE_STATUS, on_message_SERVER_ADMIN_HOST_UPDATE_STATUS, sMESSAGE_NETWORK);
			END_SUB_CGMESSAGE_MAP
		END_NESTED_CGMESSAGE_MAP
	}
	catch (...)
	{
		// - 잘못된 메시지 (짧거나 깨진 메시지 : 원격 입력이므로 assert하지 않는다. 무시한다.)
		//   (log는 10초에 한 번만 남긴다 : 계속 보내는 상대가 있어도 log가 넘치지 않도록)
		static server_system::log_throttle s_log_invalid_message(std::chrono::seconds(10));
		if (auto suppressed = s_log_invalid_message.try_log(); suppressed.has_value())
			LOG_WARNING << "(warn) invalid admin message ignored (host group socket)"sv << ((*suppressed != 0) ? " (+"s + std::to_string(*suppressed) + " suppressed for 10s)" : std::string());
	}

	// return) 
	return eRESULT::BYPASS;
}

result_code server_system::host::group_socket::on_message_SERVER_ADMIN_INSTANCE_UPDATE_STATE(sMESSAGE_NETWORK& _msg)
{
	// 1) get buffer
	auto buf_recv = _msg.buf_message + offset(8);

	try
	{
		// 2) get service
		auto pservice = dynamic_pointer_cast<server_system::Xservice>(server_system::get_service());

		// check)
		THROW_IF(pservice.empty(), throwable::failure(eRESULT::NOT_READY, 0, "service is not ready!"sv));

		// 3) get service instance manager
		auto pservice_instance_manager = pservice->get_service_instance_manager();

		// check)
		RETURN_IF(pservice_instance_manager.empty(), eRESULT::DONE);

		// 4) get iid
		auto iid_service = buf_recv.extract<server_system::iid_t>();

		// 5) get service instance
		auto pservice_instance = pservice_instance_manager->find_instance(iid_service);

		// check) 
		RETURN_IF(pservice_instance.empty(), eRESULT::DONE);

		// 7) get services state
		auto state_new = buf_recv.extract<eOBJECT_STATE>();
		auto state_old = buf_recv.extract<eOBJECT_STATE>();
		auto has_service_info = buf_recv.extract<bool>();

		if (has_service_info)
		{
			auto service_info = buf_recv.extract<server_system::sSERVICE>();;
			pservice_instance_manager->update_instance_info(iid_service, std::move(service_info));
		}

		// 8) update service info
		pservice_instance_manager->update_instance_state(iid_service, state_new, state_old);

		// 9) get admin socket
		auto psocket_to_admin = pservice->get_socket_to_admin();

		// 10) notify to admin
		if (psocket_to_admin.exist())
		{
			psocket_to_admin->send(_msg.buf_message);
		}
	}
	catch (...)
	{

	}

	// return) 
	return eRESULT::DONE;
}

result_code server_system::host::group_socket::on_message_SERVER_ADMIN_INSTANCE_UPDATE_INFO(sMESSAGE_NETWORK& _msg)
{
	// 1) get buffer
	auto buf_recv = _msg.buf_message + offset(8);

	try
	{
		// 2) get service
		auto pservice = dynamic_pointer_cast<server_system::Xservice>(server_system::get_service());

		// check)
		THROW_IF(pservice.empty(), throwable::failure(eRESULT::NOT_READY, 0, "service is not ready!"sv));

		// 3) get service instance manager
		auto pservice_instance_manager = pservice->get_service_instance_manager();

		// check)
		RETURN_IF(pservice_instance_manager.empty(), eRESULT::DONE);

		// 4) get iid
		auto iid_service = buf_recv.extract<server_system::iid_t>();

		// 5) get service instance
		auto pservice_instance = pservice_instance_manager->find_instance(iid_service);

		// check) 
		RETURN_IF(pservice_instance.empty(), eRESULT::DONE);

		// 6) get services state
		auto service_info = buf_recv.extract<server_system::sSERVICE>();

		// 7) update service info
		pservice_instance_manager->update_instance_info(iid_service, std::move(service_info));

		// 8) get admin socket
		auto psocket_to_admin = pservice->get_socket_to_admin();

		// 9) notify to admin
		if (psocket_to_admin.exist())
		{
			psocket_to_admin->send(_msg.buf_message);
		}
	}
	catch (...)
	{

	}

	// return) 
	return eRESULT::DONE;
}

result_code server_system::host::group_socket::on_message_SERVER_ADMIN_HOST_UPDATE_STATUS(sMESSAGE_NETWORK& _msg)
{
	//-----------------------------------------------------------------
	// host system status (cpu/memory/threads/processes/handles/network)
	//  - message: iid_t(host), SYSTEM::sSTATUS (make_message_SERVER_ADMIN_HOST_UPDATE_STATUS)
	//-----------------------------------------------------------------
	auto buf_recv = _msg.buf_message + offset(8);

	try
	{
		auto pservice = dynamic_pointer_cast<server_system::Xservice>(server_system::get_service());
		RETURN_IF(pservice.empty(), eRESULT::DONE);

		auto phost_instance_manager = pservice->get_host_instance_manager();
		RETURN_IF(phost_instance_manager.empty(), eRESULT::DONE);

		auto iid_host = buf_recv.extract<server_system::iid_t>();
		auto phost_instance = phost_instance_manager->find_instance(iid_host);
		RETURN_IF(phost_instance.empty(), eRESULT::DONE);

		phost_instance->update_system_status(server_system::extract_system_status(buf_recv));

		// - web ui status (없으면 이전 host_agent)
		if (auto web_status = server_system::extract_host_web_status(buf_recv); web_status.has_value())
			phost_instance->update_web_status(*web_status);
	}
	catch (...)
	{
	}

	// return)
	return eRESULT::DONE;
}

result_code server_system::host::group_socket::on_message_SERVER_ADMIN_INSTANCE_UPDATE_STATUS(sMESSAGE_NETWORK& _msg)
{
	//-----------------------------------------------------------------
	// 설명) service의 status(factory/executor/connective/traffic...) 갱신
	//  - message: iid_t, sSERVICE_STATUS_UPDATE
	//-----------------------------------------------------------------

	// 1) get buffer
	auto buf_recv = _msg.buf_message + offset(8);

	try
	{
		// 2) get service
		auto pservice = dynamic_pointer_cast<server_system::Xservice>(server_system::get_service());

		// check)
		THROW_IF(pservice.empty(), throwable::failure(eRESULT::NOT_READY, 0, "service is not ready!"sv));

		// 3) get service instance manager
		auto pservice_instance_manager = pservice->get_service_instance_manager();

		// check)
		RETURN_IF(pservice_instance_manager.empty(), eRESULT::DONE);

		// 4) get iid
		auto iid_service = buf_recv.extract<server_system::iid_t>();

		// 5) get service instance
		auto pservice_instance = pservice_instance_manager->find_instance(iid_service);

		// check) 
		RETURN_IF(pservice_instance.empty(), eRESULT::DONE);

		// 6) update status
		pservice_instance->update_instance_status(buf_recv.extract<server_system::sSERVICE_STATUS_UPDATE>());

		// 7) notify to admin (상위 admin server가 있으면 전달한다.)
		auto psocket_to_admin = pservice->get_socket_to_admin();

		if (psocket_to_admin.exist())
		{
			psocket_to_admin->send(_msg.buf_message);
		}
	}
	catch (...)
	{
	}

	// return) 
	return eRESULT::DONE;
}

result_code server_system::host::group_socket::on_message_SERVER_ADMIN_REQUEST_RESULT(sMESSAGE_NETWORK& _msg)
{
	//-----------------------------------------------------------------
	// 설명) REQUEST_xxx의 처리 결과
	//  - message: iid_t, uint32_t request, uint64_t request_id, eRESULT, std::string_view message
	//-----------------------------------------------------------------

	// 1) get buffer
	auto buf_recv = _msg.buf_message + offset(8);

	try
	{
		// 2) get service
		auto pservice = dynamic_pointer_cast<server_system::Xservice>(server_system::get_service());

		// check)
		THROW_IF(pservice.empty(), throwable::failure(eRESULT::NOT_READY, 0, "service is not ready!"sv));

		// 3) get service instance manager
		auto pservice_instance_manager = pservice->get_service_instance_manager();

		// check)
		RETURN_IF(pservice_instance_manager.empty(), eRESULT::DONE);

		// 4) extract
		auto iid_service = buf_recv.extract<server_system::iid_t>();
		auto request = buf_recv.extract<uint32_t>();
		auto request_id = buf_recv.extract<uint64_t>();
		auto result = buf_recv.extract<eRESULT>();
		auto message = buf_recv.extract<std::string_view>();

		// 5) notify to manager
		pservice_instance_manager->update_request_result(iid_service, request, request_id, result, message);

		// 6) notify to admin (상위 admin server가 있으면 전달한다.)
		auto psocket_to_admin = pservice->get_socket_to_admin();

		if (psocket_to_admin.exist())
		{
			psocket_to_admin->send(_msg.buf_message);
		}
	}
	catch (...)
	{
	}

	// return) 
	return eRESULT::DONE;
}

//result_code server_system::socket::client::on_message_SERVER_ADMIN_INSTANCE_UPDATE(sMESSAGE_NETWORK& _msg)
//{
//	// 1) get buffer
//	buffer_view buf_temp = _msg.buf_message + offset(8);
//
//	// 2) get iid
//	auto iid = buf_temp.extract<server_system::iid_t>();
//
//	try
//	{
//		// 3) extract data
//		auto info = buf_temp.extract<server_system::sSERVICE_STATUS_UPDATE>();
//
//		SCOPED_LOCK(this->m_lockable_pservice)
//		{
//			// check)
//			RETURN_IF(this->m_pservice_instance_info_manager.empty(), eRESULT::BYPASS);
//
//			// 4) find instance
//			auto pinstance = this->m_pservice_instance_info_manager->find_service_instance(iid);
//
//			// check)
//			RETURN_IF(pinstance.empty(), eRESULT::DONE)
//
//			//// 5) call hook function
//			//this->on_SERVER_ADMIN_INSTANCE_UPDATE(info);
//
//			// 6) update
//			//pinstance->process_update_instance_info_status(std::move(info));
//		}
//
//		// 7) call hook function
//		//this->on_SERVER_ADMIN_INSTANCE_UPDATE_post(_msg.buf_message);
//
//	}
//	catch (...)
//	{
//	}
//
//	// return) 
//	return eRESULT::DONE;
//}
//
//result_code server_system::socket::client::on_message_SERVER_TIME_EVENT_EVENT_LIST_UPDATE(sMESSAGE_NETWORK& _msg)
//{
//	// 1) get buffer
//	buffer_view buf_temp = _msg.buf_message + offset(8);
//
//	// 2) get iid
//	auto iid = buf_temp.extract<server_system::iid_t>();
//
//	try
//	{
//		// 3) get info
//		auto info = buf_temp.extract<std::vector<server_system::time_event_update_snap>>();
//
//		SCOPED_LOCK(this->m_lockable_pservice)
//		{
//			// check)
//			RETURN_IF(this->m_pservice_instance_info_manager.empty(), eRESULT::BYPASS);
//
//			// 4) find instance
//			auto pinstance = this->m_pservice_instance_info_manager->find_service_instance(iid);
//
//			// check)
//			RETURN_IF(pinstance.empty(), eRESULT::DONE)
//
//			//// 5) call hook function
//			//this->on_SERVER_TIME_EVENT_EVENT_LIST_UPDATE(info);
//
//			// 6) update info
//			//pinstance->update_time_event_info(std::move(info));
//		}
//
//		//// 7) call hook function
//		//this->on_SERVER_TIME_EVENT_EVENT_LIST_UPDATE_post(_msg.buf_message);
//	}
//	catch (...)
//	{
//	}
//
//	// return)
//	return eRESULT::DONE;
//}
//
//result_code server_system::socket::client::on_message_SERVER_TIME_EVENT_EVENT_REGISTER(sMESSAGE_NETWORK& _msg)
//{
//	// 1) get buffer
//	buffer_view buf_temp = _msg.buf_message + offset(8);
//
//	// 2) get iid
//	auto iid = buf_temp.extract<server_system::iid_t>();
//
//	try
//	{
//		// 3) get info
//		auto info = buf_temp.extract<server_system::time_event_snap>();
//
//		// 4) call hook function
//
//		// 5) set info
//		SCOPED_LOCK(this->m_lockable_pservice)
//		{
//			// check)
//			RETURN_IF(this->m_pservice_instance_info_manager.empty(), eRESULT::BYPASS);
//
//			// 4) find instance
//			auto pinstance = this->m_pservice_instance_info_manager->find_service_instance(iid);
//
//			// check)
//			RETURN_IF(pinstance.empty(), eRESULT::DONE)
//
//			//// 5) call hook function
//			//this->on_SERVER_TIME_EVENT_EVENT_REGISTER(info);
//
//			// 6) update info
//			//pinstance->register_time_event_info(std::move(info));
//		}
//
//		//// 7) call hook function
//		//this->on_SERVER_TIME_EVENT_EVENT_REGISTER_post(_msg.buf_message);
//	}
//	catch (...)
//	{
//	}
//
//	// return)
//	return eRESULT::DONE;
//}
//
//result_code server_system::socket::client::on_message_SERVER_TIME_EVENT_EVENT_UNREGISTER(sMESSAGE_NETWORK& _msg)
//{
//	// 1) get buffer
//	buffer_view buf_temp = _msg.buf_message + offset(8);
//
//	// 2) get iid
//	auto iid = buf_temp.extract<server_system::iid_t>();
//
//	try
//	{
//		// 3) get info
//		auto event_id = buf_temp.extract<uint64_t>();
//
//		SCOPED_LOCK(this->m_lockable_pservice)
//		{
//			// check)
//			RETURN_IF(this->m_pservice_instance_info_manager.empty(), eRESULT::BYPASS);
//
//			// 4) find instance
//			auto pinstance = this->m_pservice_instance_info_manager->find_service_instance(iid);
//
//			// check)
//			RETURN_IF(pinstance.empty(), eRESULT::DONE)
//
//			//// 5) call hook function
//			//this->on_SERVER_TIME_EVENT_EVENT_UNREGISTER(event_id);
//
//			// 6) update info
//			//pinstance->unregister_time_event_info(event_id);
//		}
//
//		//// 7) call hook function
//		//this->on_SERVER_TIME_EVENT_EVENT_UNREGISTER_post(_msg.buf_message);
//	}
//	catch (...)
//	{
//	}
//
//	// return)
//	return eRESULT::DONE;
//}
//
//result_code server_system::socket::client::on_message_FACTORY_FACTORY_LIST_UPDATE(sMESSAGE_NETWORK& _msg)
//{
//	// 1) get buffer
//	buffer_view buf_temp = _msg.buf_message + offset(8);
//
//	// 2) get iid
//	auto iid = buf_temp.extract<server_system::iid_t>();
//
//	try
//	{
//		// 3) get info
//		auto vector_info = buf_temp.extract<std::vector<server_system::factory_update_snap>>();
//
//		SCOPED_LOCK(this->m_lockable_pservice)
//		{
//			// check)
//			RETURN_IF(this->m_pservice_instance_info_manager.empty(), eRESULT::BYPASS);
//
//			// 4) find instance
//			auto pinstance = this->m_pservice_instance_info_manager->find_service_instance(iid);
//
//			// check)
//			RETURN_IF(pinstance.empty(), eRESULT::DONE)
//
//			//// 5) call hook function
//			//this->on_FACTORY_FACTORY_LIST_UPDATE(vector_info);
//
//			// 6) update info
//			//pinstance->update_factory_info(std::move(vector_info));
//		}
//
//		//// 7) call hook function
//		//this->on_FACTORY_FACTORY_LIST_UPDATE_post(_msg.buf_message);
//	}
//	catch (...)
//	{
//	}
//
//	// return)
//	return eRESULT::DONE;
//}
//
//result_code server_system::socket::client::on_message_FACTORY_FACTORY_REGISTER(sMESSAGE_NETWORK& _msg)
//{
//	// 1) get buffer
//	buffer_view buf_temp = _msg.buf_message + offset(8);
//
//	// 2) get iid
//	auto iid = buf_temp.extract<server_system::iid_t>();
//
//	try
//	{
//		// 3) get info
//		auto info = buf_temp.extract<server_system::factory_snap>();
//
//		SCOPED_LOCK(this->m_lockable_pservice)
//		{
//			// check)
//			RETURN_IF(this->m_pservice_instance_info_manager.empty(), eRESULT::BYPASS);
//
//			// 4) find instance
//			auto pinstance = this->m_pservice_instance_info_manager->find_service_instance(iid);
//
//			// check)
//			RETURN_IF(pinstance.empty(), eRESULT::DONE)
//
//			//// 5) call hook function
//			//this->on_FACTORY_FACTORY_REGISTER(info);
//
//			// 6) register
//			//pinstance->register_factory_info(std::move(info));
//		}
//
//		//// 7) call hook function
//		//this->on_FACTORY_FACTORY_REGISTER_post(_msg.buf_message);
//	}
//	catch (...)
//	{
//	}
//
//	// return)
//	return eRESULT::DONE;
//}
//
//result_code server_system::socket::client::on_message_FACTORY_FACTORY_UNREGISTER(sMESSAGE_NETWORK& _msg)
//{
//	// 1) get buffer
//	buffer_view buf_temp = _msg.buf_message + offset(8);
//
//	// 2) get iid
//	auto iid = buf_temp.extract<server_system::iid_t>();
//
//	try
//	{
//		// 3) get info
//		auto factory_id = buf_temp.extract<uint64_t>();
//
//		SCOPED_LOCK(this->m_lockable_pservice)
//		{
//			// check)
//			RETURN_IF(this->m_pservice_instance_info_manager.empty(), eRESULT::BYPASS);
//
//			// 4) find instance
//			auto pinstance = this->m_pservice_instance_info_manager->find_service_instance(iid);
//
//			// check)
//			RETURN_IF(pinstance.empty(), eRESULT::DONE)
//
//			//// 5) call hook function
//			//this->on_FACTORY_FACTORY_UNREGISTER(factory_id);
//
//			// 6) erase
//			//pinstance->unregister_factory_info(factory_id);
//		}
//
//		//// 7) call hook function
//		//this->on_FACTORY_FACTORY_UNREGISTER_post(_msg.buf_message);
//	}
//	catch (...)
//	{
//	}
//
//	// return)
//	return eRESULT::DONE;
//}
//
//result_code server_system::socket::client::on_message_EXECUTOR_LIST_UPDATE(sMESSAGE_NETWORK& _msg)
//{
//	// 1) get buffer
//	buffer_view buf_temp = _msg.buf_message + offset(8);
//
//	// 2) get iid
//	auto iid = buf_temp.extract<server_system::iid_t>();
//
//	try
//	{
//		// 3) extract data
//		auto vector_info = buf_temp.extract<std::vector<server_system::executor_update_snap>>();
//
//		SCOPED_LOCK(this->m_lockable_pservice)
//		{
//			// check)
//			RETURN_IF(this->m_pservice_instance_info_manager.empty(), eRESULT::BYPASS);
//
//			// 4) find instance
//			auto pinstance = this->m_pservice_instance_info_manager->find_service_instance(iid);
//
//			// check)
//			RETURN_IF(pinstance.empty(), eRESULT::DONE)
//
//			//// 5) call hook function
//			//this->on_EXECUTOR_LIST_UPDATE(vector_info);
//
//			// 6) update
//			//pinstance->update_executor_info(std::move(vector_info));
//		}
//
//		//// 6) call hook function
//		//this->on_EXECUTOR_LIST_UPDATE_post(_msg.buf_message);
//	}
//	catch (...)
//	{
//	}
//
//	// return)
//	return eRESULT::DONE;
//}
//
//result_code server_system::socket::client::on_message_EXECUTOR_REGISTER(sMESSAGE_NETWORK& _msg)
//{
//	// 1) get buffer
//	buffer_view buf_temp = _msg.buf_message + offset(8);
//
//	// 2) get iid
//	auto iid = buf_temp.extract<server_system::iid_t>();
//
//	try
//	{
//		// 3) get info
//		auto info = buf_temp.extract<server_system::executor_snap>();
//
//		SCOPED_LOCK(this->m_lockable_pservice)
//		{
//			// check)
//			RETURN_IF(this->m_pservice_instance_info_manager.empty(), eRESULT::BYPASS);
//
//			// 4) find instance
//			auto pinstance = this->m_pservice_instance_info_manager->find_service_instance(iid);
//
//			// check)
//			RETURN_IF(pinstance.empty(), eRESULT::DONE)
//
//			//// 5) call hook function
//			//this->on_EXECUTOR_REGISTER(info);
//
//			// 6) register
//			//pinstance->register_executor_info(std::move(info));
//		}
//
//		//// 7) call hook function
//		//this->on_EXECUTOR_REGISTER_post(_msg.buf_message);
//	}
//	catch (...)
//	{
//	}
//
//	// return)
//	return eRESULT::DONE;
//}
//
//result_code server_system::socket::client::on_message_EXECUTOR_UNREGISTER(sMESSAGE_NETWORK& _msg)
//{
//	// 1) get buffer
//	buffer_view buf_temp = _msg.buf_message + offset(8);
//
//	// 2) get iid
//	auto iid = buf_temp.extract<server_system::iid_t>();
//
//	try
//	{
//		// 3) get info
//		auto executor_id = buf_temp.extract<uint64_t>();
//
//		SCOPED_LOCK(this->m_lockable_pservice)
//		{
//			// check)
//			RETURN_IF(this->m_pservice_instance_info_manager.empty(), eRESULT::BYPASS);
//
//			// 4) find instance
//			auto pinstance = this->m_pservice_instance_info_manager->find_service_instance(iid);
//
//			// check)
//			RETURN_IF(pinstance.empty(), eRESULT::DONE)
//
//			//// 5) call hook function
//			//this->on_EXECUTOR_UNREGISTER(executor_id);
//
//			// 6) unregister 
//			//pinstance->unregister_executor_info(executor_id);
//		}
//
//		//// 7) call hook function
//		//this->on_EXECUTOR_UNREGISTER_post(_msg.buf_message);
//	}
//	catch (...)
//	{
//	}
//
//	// return)
//	return eRESULT::DONE;
//}
//
//result_code server_system::socket::client::on_message_NETWORK_SOCKET_CONNECTIVE_LIST_UPDATE(sMESSAGE_NETWORK& _msg)
//{
//	// 1) get buffer
//	buffer_view buf_temp = _msg.buf_message + offset(8);
//
//	// 2) get iid
//	auto iid = buf_temp.extract<server_system::iid_t>();
//
//	try
//	{
//		// 3) get info
//		auto vector_info = buf_temp.extract<std::vector<server_system::connective_info_update_snap>>();
//
//		SCOPED_LOCK(this->m_lockable_pservice)
//		{
//			// check)
//			RETURN_IF(this->m_pservice_instance_info_manager.empty(), eRESULT::BYPASS);
//
//			// 4) find instance
//			auto pinstance = this->m_pservice_instance_info_manager->find_service_instance(iid);
//
//			// check)
//			RETURN_IF(pinstance.empty(), eRESULT::DONE)
//
//			//// 5) call hook function
//			//this->on_NETWORK_SOCKET_CONNECTIVE_LIST_UPDATE(vector_info);
//
//			// 6) update info
//			//pinstance->update_connective_info(std::move(vector_info));
//		}
//
//		//// 6) call hook function
//		//this->on_NETWORK_SOCKET_CONNECTIVE_LIST_UPDATE_post(_msg.buf_message);
//	}
//	catch (...)
//	{
//	}
//
//	// return)
//	return eRESULT::DONE;
//}
//
//result_code server_system::socket::client::on_message_NETWORK_SOCKET_CONNECTIVE_REGISTER(sMESSAGE_NETWORK& _msg)
//{
//	// 1) get buffer
//	buffer_view buf_temp = _msg.buf_message + offset(8);
//
//	// 2) get iid
//	auto iid = buf_temp.extract<server_system::iid_t>();
//
//	try
//	{
//		// 3) get info
//		auto info = buf_temp.extract<server_system::connective_info_snap>();
//
//		SCOPED_LOCK(this->m_lockable_pservice)
//		{
//			// check)
//			RETURN_IF(this->m_pservice_instance_info_manager.empty(), eRESULT::BYPASS);
//
//			// 4) find instance
//			auto pinstance = this->m_pservice_instance_info_manager->find_service_instance(iid);
//
//			// check)
//			RETURN_IF(pinstance.empty(), eRESULT::DONE)
//
//			//// 5) call hook function
//			//this->on_NETWORK_SOCKET_CONNECTIVE_REGISTER(info);
//
//			// - erase
//			//pinstance->register_connective_info(std::move(info));
//		}
//
//		//// 6) call hook function
//		//this->on_NETWORK_SOCKET_CONNECTIVE_REGISTER_post(_msg.buf_message);
//	}
//	catch (...)
//	{
//	}
//
//	// return)
//	return eRESULT::DONE;
//}
//
//result_code server_system::socket::client::on_message_NETWORK_SOCKET_CONNECTIVE_UNREGISTER(sMESSAGE_NETWORK& _msg)
//{
//	// 1) get buffer
//	buffer_view buf_temp = _msg.buf_message + offset(8);
//
//	// 2) get iid
//	auto iid = buf_temp.extract<server_system::iid_t>();
//
//	try
//	{
//		// 3) get info
//		auto connective_id = buf_temp.extract<uint64_t>();
//
//		SCOPED_LOCK(this->m_lockable_pservice)
//		{
//			// check)
//			RETURN_IF(this->m_pservice_instance_info_manager.empty(), eRESULT::BYPASS);
//
//			// 4) find instance
//			auto pinstance = this->m_pservice_instance_info_manager->find_service_instance(iid);
//
//			// check)
//			RETURN_IF(pinstance.empty(), eRESULT::DONE)
//
//			//// 5) call hook function
//			//this->on_NETWORK_SOCKET_CONNECTIVE_UNREGISTER(connective_id);
//
//			// 6) erase
//			//pinstance->unregister_connective_info(connective_id);
//		}
//
//		//// 7) call hook function
//		//this->on_NETWORK_SOCKET_CONNECTIVE_UNREGISTER_post(_msg.buf_message);
//	}
//	catch (...)
//	{
//	}
//
//	// return)
//	return eRESULT::DONE;
//}
//
//result_code server_system::socket::client::on_message_NETWORK_SOCKET_BINDER_LIST_UPDATE(sMESSAGE_NETWORK& _msg)
//{
//	// 1) get buffer
//	buffer_view buf_temp = _msg.buf_message + offset(8);
//
//	// 2) get iid
//	auto iid = buf_temp.extract<server_system::iid_t>();
//
//	try
//	{
//		// 3) get info
//		auto vector_info = buf_temp.extract<std::vector<server_system::socket_info_update_snap>>();
//
//		SCOPED_LOCK(this->m_lockable_pservice)
//		{
//			// check)
//			RETURN_IF(this->m_pservice_instance_info_manager.empty(), eRESULT::BYPASS);
//
//			// 4) find instance
//			auto pinstance = this->m_pservice_instance_info_manager->find_service_instance(iid);
//
//			// check)
//			RETURN_IF(pinstance.empty(), eRESULT::DONE)
//
//			//// 5) call hook function
//			//this->on_NETWORK_SOCKET_BINDER_LIST_UPDATE(vector_info);
//
//			// 6) erase
//			//pinstance->update_binder_info(std::move(vector_info));
//		}
//
//		//// 7) call hook function
//		//this->on_NETWORK_SOCKET_BINDER_LIST_UPDATE_post(_msg.buf_message);
//	}
//	catch (...)
//	{
//	}
//
//	// return)
//	return eRESULT::DONE;
//}
//
//result_code server_system::socket::client::on_message_NETWORK_SOCKET_BINDER_REGISTER(sMESSAGE_NETWORK& _msg)
//{
//	// 1) get buffer
//	buffer_view buf_temp = _msg.buf_message + offset(8);
//
//	// 2) get iid
//	auto iid = buf_temp.extract<server_system::iid_t>();
//
//	try
//	{
//		// 3) get info
//		auto info = buf_temp.extract<server_system::socket_info_snap>();
//
//		SCOPED_LOCK(this->m_lockable_pservice)
//		{
//			// check)
//			RETURN_IF(this->m_pservice_instance_info_manager.empty(), eRESULT::BYPASS);
//
//			// 4) find instance
//			auto pinstance = this->m_pservice_instance_info_manager->find_service_instance(iid);
//
//			// check)
//			RETURN_IF(pinstance.empty(), eRESULT::DONE)
//
//			//// 5) call hook function
//			//this->on_NETWORK_SOCKET_BINDER_REGISTER(info);
//
//			// 6) register
//			//pinstance->register_binder_info(std::move(info));
//		}
//
//		//// 7) call hook function
//		//this->on_NETWORK_SOCKET_BINDER_REGISTER_post(_msg.buf_message);
//	}
//	catch (...)
//	{
//	}
//
//	// return)
//	return eRESULT::DONE;
//}
//
//result_code server_system::socket::client::on_message_NETWORK_SOCKET_BINDER_UNREGISTER(sMESSAGE_NETWORK& _msg)
//{
//	// 1) get buffer
//	buffer_view buf_temp = _msg.buf_message + offset(8);
//
//	// 2) get iid
//	auto iid = buf_temp.extract<server_system::iid_t>();
//
//	try
//	{
//		// 3) get instance
//		auto binder_id = buf_temp.extract<uint64_t>();
//
//		//// 4) call hook function
//		//this->on_NETWORK_SOCKET_BINDER_UNREGISTER(binder_id);
//
//		// 5) set data
//		SCOPED_LOCK(this->m_lockable_pservice)
//		{
//			// check)
//			RETURN_IF(this->m_pservice_instance_info_manager.empty(), eRESULT::BYPASS);
//
//			// 4) find instance
//			auto pinstance = this->m_pservice_instance_info_manager->find_service_instance(iid);
//
//			// check)
//			RETURN_IF(pinstance.empty(), eRESULT::DONE)
//
//			// 5) register
//			//pinstance->m_instance.unregister_binder_info(binder_id);
//		}
//
//		//// 6) call hook function
//		//this->on_NETWORK_SOCKET_BINDER_UNREGISTER_post(_msg.buf_message);
//	}
//	catch (...)
//	{
//	}
//
//	// return)
//	return eRESULT::DONE;
//}
//
//result_code server_system::socket::client::on_message_NETWORK_SOCKET_CONNECTIVE_UPDATE(sMESSAGE_NETWORK& _msg)
//{
//	// 1) get buffer
//	buffer_view buf_temp = _msg.buf_message + offset(8);
//
//	// 2) get iid
//	auto iid = buf_temp.extract<server_system::iid_t>();
//
//	try
//	{
//		// 3) extract info
//		auto info = buf_temp.extract<statistics::unit_connective_snap>();
//
//		SCOPED_LOCK(this->m_lockable_pservice)
//		{
//			// check)
//			RETURN_IF(this->m_pservice_instance_info_manager.empty(), eRESULT::BYPASS);
//
//			// 4) find instance
//			auto pinstance = this->m_pservice_instance_info_manager->find_service_instance(iid);
//
//			// check)
//			RETURN_IF(pinstance.empty(), eRESULT::DONE)
//
//			//// 4) call hook function
//			//this->on_NETWORK_SOCKET_CONNECTIVE_UPDATE(info);
//
//			// - register
//			//pinstance->update_connective_statistics(std::move(info));
//		}
//
//		//// 6) call hook function
//		//this->on_NETWORK_SOCKET_CONNECTIVE_UPDATE_post(_msg.buf_message);
//	}
//	catch (...)
//	{
//	}
//
//	// return)
//	return eRESULT::DONE;
//}
//
//result_code server_system::socket::client::on_message_SYSTEM_LOG(sMESSAGE_NETWORK& _msg)
//{
//	// 1) get buffer
//	buffer_view buf_temp = _msg.buf_message + offset(8);
//
//	// 2) get iid
//	auto iid = buf_temp.extract<server_system::iid_t>();
//
//	// 3) extract log_record
//	auto list_log_record = buf_temp.extract<circular_list<object_ptr<LOG_RECORD>>>();
//
//	// 3) notify to web
//	//this->pobservable_websocket_peer_overview_info->add_log_records(list_log_record);
//
//	//// 5) call hook function
//	//this->on_SYSTEM_LOG(list_log_record);
//
//	// 6) set data
//	SCOPED_LOCK(this->m_lockable_pservice)
//	{
//		// check)
//		RETURN_IF(this->m_pservice_instance_info_manager.empty(), eRESULT::BYPASS);
//
//		// 4) find instance
//		auto pinstance = this->m_pservice_instance_info_manager->find_service_instance(iid);
//
//		// check)
//		RETURN_IF(pinstance.empty(), eRESULT::DONE)
//
//		// - add log
//		//pinstance->add_log_records(std::move(list_log_record));
//	}
//
//	//// 7) call hook function
//	//this->on_SYSTEM_LOG_post(_msg.buf_message);
//
//	// return)
//	return eRESULT::DONE;
//}
//
//result_code server_system::socket::client::on_message_DUMP(sMESSAGE_NETWORK& /*_msg*/)
//{
//	//// 1) call hook function
//	//this->on_DUMP();
//
//	// return)
//	return eRESULT::DONE;
//}

//void server_system::socket::client::process_update_instance_info_status(service_instance* /*_pinstance*/, server_system::sSERVICE_STATUS_UPDATE&& /*_service_info_update*/)
//{
//	// 1) update data
//	SCOPED_LOCK(this->m_lockable_pservice)
//	{
//		//_pinstance->status.vector_process_status.push_back(std::move(_service_info_update.process_status));
//		//_pinstance->update_factory_info(std::move(_service_info_update.vector_factory_update));
//		//_pinstance->update_executor_info(std::move(_service_info_update.vector_executor_update));
//		//_pinstance->update_schedulable_info(std::move(_service_info_update.vector_schedulable_update));
//		//_pinstance->update_connective_info(std::move(_service_info_update.vector_connective_update));
//		//_pinstance->update_binder_info(std::move(_service_info_update.vector_binder_update));
//		//_pinstance->update_connective_statistics(std::move(_service_info_update.connective_info));
//		//_pinstance->update_traffic_statistics(std::move(_service_info_update.traffic_info));
//	}
//
//	//// notify) to alll
//	//this->on_update_instance_info();
//}

//eOBJECT_STATE server_system::socket::client::process_set_instance_state(eOBJECT_STATE _state) noexcept
//{
//	// declare)
//	eOBJECT_STATE state_old{};
//
//	SCOPED_LOCK(this->m_lockable_pservice)
//	{
//		// 1) get old state
//		state_old = this->m_instance.status.state;
//
//		// check) 
//		RETURN_IF(state_old == _state, state_old)
//
//		// 2) change
//		this->m_instance.status.state = _state;
//	}
//
//	// return) 
//	return state_old;
//}

