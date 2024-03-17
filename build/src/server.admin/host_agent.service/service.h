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
// service
//
// ----------------------------------------------------------------------------
class service : public server_system::service_application<service>
{
public:
			service();
	virtual	void on_service_setting(context& _context) override;

	static	own_ptr<server_admin::instance_manager>		pmanager_instance;

	static	own_ptr<socket_admin_server>				psocket_to_admin_server;
	static	own_ptr<net::acceptor<socket_peer>>			pacceptor_server;
	static	own_ptr<group_peer_immigration>				pgroup_peer_immigration;
	static	own_ptr<group_server>						pgroup_server;
	static	own_ptr<group_client>						pgroup_client;

	static	own_ptr<net::acceptor<socket_web>>			pacceptor_web;
	static	own_ptr<net::acceptor<socket_websocket>>	pacceptor_websocket;
	static	own_ptr<group_websocket_immigration>		pgroup_websocket_immigration;
	static	own_ptr<group_websocket>					pgroup_websocket;
};
