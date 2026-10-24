//*****************************************************************************
//*                                                                           *
//*                       CGCII Sample Login System (1)                       *
//*                                                                           *
//*                                Login Server                               *
//*                                                                           *
//*  Programmed by Cho SangHyun(sangduck@cgcii.co.kr) 2016/02/01.             *
//*                                                                           *
//*  Updated : 2016/02/01                                                     *
//*                                                                           *
//*****************************************************************************

#pragma once

class service : public server_system::Xservice
{
public:
			service();

	static	own_ptr<user::manager> pmanager_account;
	static	own_ptr<server::group_channel> pgroup_server_channel;
	static	own_ptr<server::group_immigration> pgroup_server_Immigration;
	static	own_ptr<net::acceptor<server::socket>> pacceptor_server_lobby;
	static	own_ptr<user::group_immigration> pgroup_user_immigration;
	static	own_ptr<net::acceptor<user::socket>> pacceptor_user;
};
