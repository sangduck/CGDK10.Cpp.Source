#include "pch.h"


void group_peer_server::on_initialize(context& /*_context*/)
{
	// log) 
	LOG_INFO << " - initialize group '"sv << name() << "'"sv;

	// 1) register mediator
	this->register_messageable(::service::instance());
}

void group_peer_server::on_destroy()
{
	// log) 
	LOG_INFO << " - destroy group '"sv << name() << "'"sv;

	// 1) register mediator
	this->unregister_messageable(::service::instance());
}

void group_peer_server::on_start(context& /*_context*/)
{
	this->enable_member_enter();
}

void group_peer_server::on_stop()
{
	this->disable_member_enter();
}

result_code group_peer_server::on_member_entering(socket_peer* /*_pmember*/, sMESSAGE& /*_param*/)
{
	return eRESULT::SUCCESS;
}

shared_buffer message_server_system_ERROR(int _error_code, uint64_t _handle_process)
{
	// 1) Buffer를 할당받는다.
	auto buf_temp = alloc_shared_buffer(8 + sizeof(int) + sizeof(uint64_t));

	// 2) Message를 작성한다.
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::ERROR_);
	buf_temp.append<int>(_error_code);
	buf_temp.append<uint64_t>(_handle_process);

	// 3) message 길이를  써넣는다.
	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

	// 4) 전송한다.
	return buf_temp;
}

void group_peer_server::on_member_entered(socket_peer* _pmember, sMESSAGE& /*_param*/)
{
	// 1) _pmember의 Mediator를 this로 설정한다.
	_pmember->register_messageable(this);
}

uintptr_t group_peer_server::on_member_leaving(socket_peer* _pmember, uintptr_t /*_param*/)
{
	// 2) _pmember의 Mediator에서 떼낸다.
	_pmember->unregister_messageable(this);

	// return) 
	return 0;
}

void group_peer_server::on_member_leaved(socket_peer* /*_pmember*/, uintptr_t /*_result*/)
{
	// 1) unregister 혹은 update한다.
	//::service::pgroup_peer_client->send(_pmember->server_system::make_message_SERVER_ADMIN_INSTANCE_LEAVE());
}

result_code group_peer_server::process_message(sMESSAGE& _msg) 
{
	// 1) execute message
	BEGIN_NESTED_CGMESSAGE_MAP
		CGMESSAGE_TYPE_SUB(eMESSAGE::SYSTEM::NETWORK, ((sMESSAGE_BUFFER&)_msg).buf_message.front<uint32_t>(4))
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::INSTANCE_UPDATE_STATE,	on_message_passby_to_instance,	sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::INSTANCE_UPDATE_INFO,		on_message_passby_to_instance,	sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::SERVER::TIME_EVENT::EVENT_LIST_UPDATE,	on_message_passby_to_instance,	sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::SERVER::TIME_EVENT::EVENT_REGISTER,		on_message_passby_to_instance,	sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::SERVER::TIME_EVENT::EVENT_UNREGISTER,	on_message_passby_to_instance,	sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::SYSTEM::LOG_,							on_message_passby_to_instance,	sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::DUMP,									on_message_passby_to_instance,	sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::FACTORY::FACTORY_LIST_UPDATE,			on_message_passby_to_instance,	sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::FACTORY::FACTORY_REGISTER,				on_message_passby_to_instance,	sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::FACTORY::FACTORY_UNREGISTER,				on_message_passby_to_instance,	sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::EXECUTOR_LIST_UPDATE,					on_message_passby_to_instance,	sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::EXECUTOR_REGISTER,						on_message_passby_to_instance,	sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::EXECUTOR_UNREGISTER,						on_message_passby_to_instance,	sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::NETWORK::SOCKET::CONNECTIVE_LIST_UPDATE,	on_message_passby_to_instance,	sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::NETWORK::SOCKET::CONNECTIVE_REGISTER,	on_message_passby_to_instance,	sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::NETWORK::SOCKET::CONNECTIVE_UNREGISTER,	on_message_passby_to_instance,	sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::NETWORK::SOCKET::BINDER_LIST_UPDATE,		on_message_passby_to_instance,	sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::NETWORK::SOCKET::BINDER_REGISTER,		on_message_passby_to_instance,	sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::NETWORK::SOCKET::BINDER_UNREGISTER,		on_message_passby_to_instance,	sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::NETWORK::SOCKET::CONNECTIVE_UPDATE,		on_message_passby_to_instance,	sMESSAGE_NETWORK);
			//ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::TRAFFIC_INFO_UPDATE,		on_message_server_system_TRAFFIC_INFO_UPDATE,		sMESSAGE_NETWORK);
		END_SUB_CGMESSAGE_MAP
	END_NESTED_CGMESSAGE_MAP

	// return) 
	return eRESULT::BYPASS;
}

object_ptr<server_system::instance_service> _get_service_instance(server_system::iid_t _iid) noexcept
{
	// 1) get instance manager
	auto pmanager_instance = ::service::pmanager_instance.get_object_ptr();

	// check)
	RETURN_IF(pmanager_instance.empty(), object_ptr<server_system::instance_service>());

	// 2) find 
	auto pinstance = pmanager_instance->find_instance(_iid);

	// check)
	RETURN_IF(pinstance.empty(), object_ptr<server_system::instance_service>());

	// return) 
	return dynamic_pointer_cast<server_system::instance_service>(pinstance);
}

result_code group_peer_server::on_message_passby_to_instance(sMESSAGE_NETWORK& _msg)
{
	// 1) 
	buffer_view buf_temp = _msg.buf_message + offset(8);

	// 2) get instance
	auto iid = buf_temp.extract<server_system::iid_t>();

	// 3) send to group_parent


	// 3) get service instance
	auto pinstance_service = _get_service_instance(iid);

	// check)
	RETURN_IF(pinstance_service.empty(), eRESULT::DONE);

	// return) passby
	return pinstance_service->request_process_message(_msg);
}
