//*****************************************************************************
//*                                                                           *
//*                       CGCII Sample Login System (1)                       *
//*                                                                           *
//*                                Lobby Server                               *
//*                                                                           *
//*  Programmed by Cho SangHyun(sangduck@cgcii.co.kr)                         *
//*                                                                           *
//*  Updated : 2016/02/01                                                     *
//*                                                                           *
//*****************************************************************************

#pragma once

class service : public server_system::Xservice
{
public:
			service();

	static	own_ptr<server_channel_manager::socket>			psocket_server_channel_manager;
	static	own_ptr<user::group>					pgroup_user;
	static	own_ptr<user::group_immigration>		pgroup_user_immigration;
	static	own_ptr<net::acceptor<user::socket>>	pacceptor_user;
	static	SERVER_INFO								server_info;
	static	lock_object								lock_server_info;

			shared_buffer	message_REQUEST_CERTIFICATION();
			shared_buffer	message_SEND_LOBBYSERVER_STATUS();
			void			update_player_count();

};
