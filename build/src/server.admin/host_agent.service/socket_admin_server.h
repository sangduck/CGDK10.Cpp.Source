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
// socket_admin_server (HostAgent -> AdminServer)
//
// ----------------------------------------------------------------------------
class socket_admin_server :
	public						net::io::connect_requestable::Ntcp_reconnect,
	public						net::socket::tcp<>,
	public						Nmessage_transmitter
{
private:
	virtual	void				on_initialize(context& _context) override;
	virtual	void				on_destroy() override;
	virtual	void				on_start(context& _context) override;
	virtual	void				on_stop() override;
	virtual void				on_connect() override;
	virtual void				on_disconnect(uint64_t _disconnect_reason) override;
	virtual void				on_certify() {}

	virtual result_code			on_message(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_ADMIN_CERTIFY_RESPONSE(sMESSAGE_NETWORK& _msg);
};