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
// socket_peer
// 
// - server와 client 모두에 사용되는 socket이다.
// - 접속 후 인증과정에서 host_agent나 server일 경우와 
//   client를 구분하여 다른 group에 입장한다.
//   
//      server, host_agent => group_peer_server
//      client => group_peer_client
//
// ----------------------------------------------------------------------------
class socket_peer : 
	public						net::socket::tcp<>,
	public						Igroupable<socket_peer>,
	public						Nmessage_transmitter,
	public						Npoolable<socket_peer>
{
private:
	virtual void				on_connect() override;
	virtual void				on_disconnect(uint64_t _disconnect_reason) override;
	virtual result_code			on_message(sMESSAGE_NETWORK& _msg) override;

public:
			server_system::sCERTIFY_INFO	m_certify_info;

			lock_object			m_lockable_instance;
			std::vector<object_ptr<server_system::Iinstance>> m_vector_instance;
};
