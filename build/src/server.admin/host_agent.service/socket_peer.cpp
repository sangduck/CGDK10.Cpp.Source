#include "pch.h"
#include "socket_peer.h"


void socket_peer::on_connect()
{
	// check) 허용된 주소가 아니면 끊는다.
	if (::service::access_policy_server.get()->is_address_allowed(this->remote_endpoint().address()) == false)
	{
		LOG_WARNING << "(warn) service connection rejected (not allowed address: "sv << this->remote_endpoint().address().to_string() << ")"sv;
		this->disconnect();
		return;
	}

	// 1) set socket_peer configure
	maximum_message_buffer_size(256 * 1024 * 1024);

	// 2) HostAgent group에 Enter한다.
	::service::pgroup_peer_immigration->enter(this);
}

void socket_peer::on_disconnect(uint64_t /*_disconnect_reason*/)
{
	// 1) 현재 붙어 있는 group에서 빠져나온다.
	this->leave_group();

	// 2) unregister server instance
	{
		// declare)
		std::vector<object_ptr<server_system::Iinstance>> vector_instance;

		// - move vector instance_id
		SCOPED_LOCK(this->m_lockable_instance)
		{
			vector_instance = std::move(this->m_vector_instance);
		}

		// - unregister
		for (auto iter : vector_instance)
		{
			::service::pmanager_instance->unregister_instance(iter->get_iid(), this);
		}
	}
}

result_code socket_peer::on_message(sMESSAGE_NETWORK& _msg)
{
	// 1) set source
	_msg.set_source(this);

	// 2) dispatch message
	TRANSMIT_MESSAGE(_msg);

	// return)
	return eRESULT::BYPASS;
}

