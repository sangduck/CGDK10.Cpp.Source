//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                          Server Admin Host Agent                          *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
//*  Best for Game Developement and Optimized for Game Developement.          *
//*                                                                           *
//*                (c) 2003. Cho Sanghyun. All right reserved.                *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************

#pragma once

// ----------------------------------------------------------------------------
//
// socket_server (server -> host_agent)
//
// ----------------------------------------------------------------------------
class socket_server : 
	public						net::socket::tcp<>,
	public						Igroupable<socket_server>,
	public						Nmessage_transmitter,
	public						Npoolable<socket_server>
{
private:
	virtual void				on_connect() override;
	virtual void				on_disconnect(uint64_t _disconnect_reason) override;
	virtual result_code			on_message(sMESSAGE_NETWORK& _msg) override;

public:
			server_admin::sCERTIFY_INFO	m_certify_info;

			lockable<>			m_cs_instance;
			std::vector<object_ptr<server_admin::Iinstance>> m_vector_instance;
};
