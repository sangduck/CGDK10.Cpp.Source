#include "pch.h"
#include "socket_server.h"


void socket_server::on_connect()
{
	// 1) set socket_server configure
	maximum_message_buffer_size(256 * 1024 * 1024);

	// 2) HostAgent group에 Enter한다.
	service::pgroup_server_immigration->enter(this);
}

void socket_server::on_disconnect(uint64_t /*_disconnect_reason*/)
{
	// 1) 현재 붙어 있는 group에서 빠져나온다.
	this->leave_group();

	// 2) unregister server instancce
	{
		// declare)
		std::vector<object_ptr<server_admin::Iinstance>> vector_instance;

		// - move vector instance_id
		scoped_lock(this->m_cs_instance)
		{
			vector_instance = std::move(this->m_vector_instance);
		}

		// - unregister
		for (auto iter : vector_instance)
		{
			service::pmanager_instance->unregister_instance(iter->get_iid(), this);
		}
	}
}

result_code socket_server::on_message(sMESSAGE_NETWORK& _msg)
{
	// 1) set source
	_msg.set_source(this);

	// 2) dispatch message
	TRANSMIT_MESSAGE(_msg);

	// return)
	return eRESULT::BYPASS;
}

