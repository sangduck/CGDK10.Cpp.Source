#include "pch.h"


server_admin::Iinstance::~Iinstance() noexcept
{
}

bool server_admin::Iinstance::process_sservice_disconnected(socket_server* _psocket)
{
	scoped_lock(this->m_cs_instance)
	{
		// check)
		RETURN_IF(this->m_psocket != _psocket, true);

		// 1) socket reset
		this->m_psocket.reset();

		// 2) set instance state
		this->set_instance_state(eOBJECT_STATE::NONE);
	}

	// return) 
	return false;
}

bool server_admin::Iinstance::process_send(shared_buffer&& _buffer, std::size_t _count_message, uint64_t _option)
{
	// declare) 
	object_ptr<socket_server> psocket;

	// 1) socket
	scoped_lock(this->m_cs_instance)
	{
		psocket = this->m_psocket;
	}

	// check)
	RETURN_IF(psocket.empty(), false);

	// 2) send
	return psocket->process_send(std::move(_buffer), _count_message, _option);
}
