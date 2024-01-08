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
// socket_server (server -> admin_server)
//
// - server에서 admin_server로 들어 오는 연결을 받는 socket
//
// ----------------------------------------------------------------------------
class socket_server : 
// inherited classes)
	public						net::socket::tcp<>,
	public						Igroupable<socket_server>,
	public						Nmessage_transmitter,
	public						Npoolable<socket_server>
{
// constructor/destructor)
public:
			socket_server();
	virtual ~socket_server();

// public)
public:
			eOBJECT_STATE		set_server_state(eOBJECT_STATE _eState) noexcept { RETURN_IF(m_pinfo_server_entity.empty(), eOBJECT_STATE::NONE); return	m_pinfo_server_entity->set_server_state(_eState);}
			auto				get_server_entity_info() const noexcept { return m_pinfo_server_entity;}
			void				set_server_entity_info(server_entity* _pserver_register_info) noexcept;
			void				reset_server_entity_info() noexcept;

// frameworks) 
public:
	virtual void				on_connect(net::io::Iconnective* _pconnective) override;
	virtual void				on_disconnect(uint64_t _disconnect_reason) override;

// message) 
protected:
	virtual result_code			on_message(sMESSAGE_NETWORK& _msg) override;

// send functions
public:
			shared_buffer		message_SERVER_ADMIN_SEND_SERVER_SETTING();
			shared_buffer		message_SERVER_ADMIN_SERVER_ENTER();
			shared_buffer		message_SERVER_ADMIN_SERVER_LEAVE();
			shared_buffer		message_SERVER_ADMIN_SERVER_UPDATE();

// implementation)
public:
			object_ptr<server_entity> m_pinfo_server_entity;
};
