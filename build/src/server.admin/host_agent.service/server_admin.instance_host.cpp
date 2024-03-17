#include "pch.h"


void server_admin::instance_host::register_service_instance(object_ptr<instance_Iservice>&& /*_pinstance*/)
{
}

void server_admin::instance_host::unregister_service_instance() noexcept
{
}

result_code server_admin::instance_host::on_message(sMESSAGE_NETWORK& _msg)
{
	// 1) execute message
	BEGIN_NESTED_CGMESSAGE_MAP
		// - soekct
		CGMESSAGE_TYPE_SUB(eMESSAGE::SYSTEM::NETWORK, ((sMESSAGE_BUFFER&)_msg).buf_message.front<uint32_t>(4))
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::INSTANCE_UPDATE_STATE,	on_message_SERVER_ADMIN_INSTANCE_UPDATE_STATE,		sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::INSTANCE_UPDATE_INFO,		on_message_SERVER_ADMIN_INSTANCE_UPDATE,			sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::SERVER::TIME_EVENT::EVENT_LIST_UPDATE,	on_message_SERVER_TIME_EVENT_EVENT_LIST_UPDATE,		sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::SERVER::TIME_EVENT::EVENT_REGISTER,		on_message_SERVER_TIME_EVENT_EVENT_REGISTER,		sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::SERVER::TIME_EVENT::EVENT_UNREGISTER,	on_message_SERVER_TIME_EVENT_EVENT_UNREGISTER,		sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::SYSTEM::LOG_,							on_message_SYSTEM_LOG,								sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::DUMP,									on_message_DUMP,									sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::FACTORY::FACTORY_LIST_UPDATE,			on_message_FACTORY_FACTORY_LIST_UPDATE,				sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::FACTORY::FACTORY_REGISTER,				on_message_FACTORY_FACTORY_REGISTER,				sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::FACTORY::FACTORY_UNREGISTER,				on_message_FACTORY_FACTORY_UNREGISTER,				sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::EXECUTOR_LIST_UPDATE,					on_message_EXECUTOR_LIST_UPDATE,					sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::EXECUTOR_REGISTER,						on_message_EXECUTOR_REGISTER,						sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::EXECUTOR_UNREGISTER,						on_message_EXECUTOR_UNREGISTER,						sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::NETWORK::SOCKET::CONNECTIVE_LIST_UPDATE,	on_message_NETWORK_SOCKET_CONNECTIVE_LIST_UPDATE,	sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::NETWORK::SOCKET::CONNECTIVE_REGISTER,	on_message_NETWORK_SOCKET_CONNECTIVE_REGISTER,		sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::NETWORK::SOCKET::CONNECTIVE_UNREGISTER,	on_message_NETWORK_SOCKET_CONNECTIVE_UNREGISTER,	sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::NETWORK::SOCKET::BINDER_LIST_UPDATE,		on_message_NETWORK_SOCKET_BINDER_LIST_UPDATE,		sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::NETWORK::SOCKET::BINDER_REGISTER,		on_message_NETWORK_SOCKET_BINDER_REGISTER,			sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::NETWORK::SOCKET::BINDER_UNREGISTER,		on_message_NETWORK_SOCKET_BINDER_UNREGISTER,		sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::NETWORK::SOCKET::CONNECTIVE_UPDATE,		on_message_NETWORK_SOCKET_CONNECTIVE_UPDATE,		sMESSAGE_NETWORK);
		END_SUB_CGMESSAGE_MAP

		// - websocket
		ON_CGMESSAGE(eMESSAGE::SYSTEM::WEBSOCKET,	on_message_http_websocket_REQUEST, sMESSAGE_HTTP_WEBSOCKET);
	END_NESTED_CGMESSAGE_MAP

	// return) 
	return eRESULT::BYPASS;
}

eOBJECT_STATE server_admin::instance_host::set_instance_state(eOBJECT_STATE /*_state*/) noexcept
{
	// error) 
	CGASSERT_ERROR(false);

	// return) 
	return eOBJECT_STATE::NONE;
}

result_code server_admin::instance_host::on_message_SERVER_ADMIN_INSTANCE_UPDATE_STATE(sMESSAGE_NETWORK& _msg)
{
	// check) 
	CGASSERT_ERROR(_msg.buf_message.size() == (8 + sizeof(server_admin::iid_t) + sizeof(eOBJECT_STATE) + sizeof(eOBJECT_STATE)));

	// check) 
	THROW_IF(_msg.buf_message.size() != (8 + sizeof(server_admin::iid_t) + sizeof(eOBJECT_STATE) + sizeof(eOBJECT_STATE)), throwable::failure());

	// 1) find service and state
	auto iid = _msg.buf_message.front<server_admin::iid_t>(8);

	// check)
	CGASSERT_ERROR(iid == this->get_iid());

	// check)
	RETURN_IF(iid != this->get_iid(), eRESULT::BYPASS);

	try
	{
		auto state_new = _msg.buf_message.front<eOBJECT_STATE>(8 + sizeof(server_admin::iid_t));
		auto state_old = _msg.buf_message.front<eOBJECT_STATE>(8 + sizeof(server_admin::iid_t) + sizeof(eOBJECT_STATE));

		// 3) update service_instace_stage update
		this->set_instance_state(state_new);

		// 3) notify message
		service::pgroup_client->send(_msg.buf_message);

		// log) 
		LOG_INFO << "  - INSTANCE('"sv << this->get_name() << "') state changed ["sv << to_string<char>(state_old) << "] -> ["sv << to_string<char>(state_new) << "]"sv;
	}
	catch (...)
	{
	}

	// return) 
	return eRESULT::DONE;
}

result_code server_admin::instance_host::on_message_SERVER_ADMIN_INSTANCE_UPDATE(sMESSAGE_NETWORK& /*_msg*/)
{
	// check)
	CGASSERT_ERROR(false);

	// return)
	return eRESULT::DONE;
}

result_code server_admin::instance_host::on_message_SERVER_TIME_EVENT_EVENT_LIST_UPDATE(sMESSAGE_NETWORK& /*_msg*/)
{
	// check)
	CGASSERT_ERROR(false);

	// return)
	return eRESULT::DONE;
}

result_code server_admin::instance_host::on_message_SERVER_TIME_EVENT_EVENT_REGISTER(sMESSAGE_NETWORK& /*_msg*/)
{
	// check)
	CGASSERT_ERROR(false);

	// return)
	return eRESULT::DONE;
}

result_code server_admin::instance_host::on_message_SERVER_TIME_EVENT_EVENT_UNREGISTER(sMESSAGE_NETWORK& /*_msg*/)
{
	// check)
	CGASSERT_ERROR(false);

	// return)
	return eRESULT::DONE;
}

result_code server_admin::instance_host::on_message_FACTORY_FACTORY_LIST_UPDATE(sMESSAGE_NETWORK& /*_msg*/)
{
	// check)
	CGASSERT_ERROR(false);

	// return)
	return eRESULT::DONE;
}

result_code server_admin::instance_host::on_message_FACTORY_FACTORY_REGISTER(sMESSAGE_NETWORK& /*_msg*/)
{
	// check)
	CGASSERT_ERROR(false);

	// return)
	return eRESULT::DONE;
}

result_code server_admin::instance_host::on_message_FACTORY_FACTORY_UNREGISTER(sMESSAGE_NETWORK& /*_msg*/)
{
	// check)
	CGASSERT_ERROR(false);

	// return)
	return eRESULT::DONE;
}

result_code server_admin::instance_host::on_message_EXECUTOR_LIST_UPDATE(sMESSAGE_NETWORK& /*_msg*/)
{
	// check)
	CGASSERT_ERROR(false);

	// return)
	return eRESULT::DONE;
}

result_code server_admin::instance_host::on_message_EXECUTOR_REGISTER(sMESSAGE_NETWORK& /*_msg*/)
{
	// check)
	CGASSERT_ERROR(false);

	// return)
	return eRESULT::DONE;
}
result_code server_admin::instance_host::on_message_EXECUTOR_UNREGISTER(sMESSAGE_NETWORK& /*_msg*/)
{
	// check)
	CGASSERT_ERROR(false);

	// return)
	return eRESULT::DONE;
}

result_code server_admin::instance_host::on_message_NETWORK_SOCKET_CONNECTIVE_LIST_UPDATE(sMESSAGE_NETWORK& /*_msg*/)
{
	// check)
	CGASSERT_ERROR(false);

	// return)
	return eRESULT::DONE;
}

result_code server_admin::instance_host::on_message_NETWORK_SOCKET_CONNECTIVE_REGISTER(sMESSAGE_NETWORK& /*_msg*/)
{
	// check)
	CGASSERT_ERROR(false);

	// return)
	return eRESULT::DONE;
}
result_code server_admin::instance_host::on_message_NETWORK_SOCKET_CONNECTIVE_UNREGISTER(sMESSAGE_NETWORK& /*_msg*/)
{
	// check)
	CGASSERT_ERROR(false);

	// return)
	return eRESULT::DONE;
}

result_code server_admin::instance_host::on_message_NETWORK_SOCKET_BINDER_LIST_UPDATE(sMESSAGE_NETWORK& /*_msg*/)
{
	// check)
	CGASSERT_ERROR(false);

	// return)
	return eRESULT::DONE;
}

result_code server_admin::instance_host::on_message_NETWORK_SOCKET_BINDER_REGISTER(sMESSAGE_NETWORK& /*_msg*/)
{
	// check)
	CGASSERT_ERROR(false);

	// return)
	return eRESULT::DONE;
}

result_code server_admin::instance_host::on_message_NETWORK_SOCKET_BINDER_UNREGISTER(sMESSAGE_NETWORK& /*_msg*/)
{
	// check)
	CGASSERT_ERROR(false);

	// return)
	return eRESULT::DONE;
}

result_code server_admin::instance_host::on_message_NETWORK_SOCKET_CONNECTIVE_UPDATE(sMESSAGE_NETWORK& /*_msg*/)
{
	// check)
	CGASSERT_ERROR(false);

	// return)
	return eRESULT::DONE;
}

result_code server_admin::instance_host::on_message_SYSTEM_LOG(sMESSAGE_NETWORK& _msg)
{
	// 1) buffer for extracting
	buffer_view buf_temp = _msg.buf_message + offset(8);

	// 2) get iid
	auto iid = buf_temp.extract<server_admin::iid_t>();

	// check)
	CGASSERT_ERROR(iid == get_iid());

	// check)
	RETURN_IF(iid != get_iid(), eRESULT::BYPASS);

	// 3) extract
	auto list_log_record = buf_temp.extract<circular_list<object_ptr<LOG_RECORD>>>();

	//// 5) notify to web
	//this->pobservable_websocket_host_info->process_log(list_log_record);

	//// 6) set data
	//scoped_lock(this->m_cs_instance)
	//{
	//	this->m_instance.add_log_records(list_log_record);
	//}

	//// 4) set data
	//this->update_log_records_add(std::move(list_log_record));

	// 5) send to admin server
	service::pgroup_client->send(_msg.buf_message);

	// return)
	return eRESULT::DONE;
}

result_code server_admin::instance_host::on_message_DUMP(sMESSAGE_NETWORK& /*_msg*/)
{
	// return)
	return eRESULT::DONE;
}

result_code server_admin::instance_host::on_message_http_websocket_REQUEST(sMESSAGE_HTTP_WEBSOCKET& /*_msg*/)
{
	// return)
	return eRESULT::BYPASS;
}
