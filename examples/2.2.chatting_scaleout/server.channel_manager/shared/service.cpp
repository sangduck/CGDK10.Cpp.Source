#include "pch.h"
#include "service.h"

own_ptr<service> g_pservice;

own_ptr<user::manager> service::pmanager_account;
own_ptr<server::group_channel> service::pgroup_server_channel;
own_ptr<server::group_immigration> service::pgroup_server_Immigration;
own_ptr<net::acceptor<server::socket>> service::pacceptor_server_lobby;
own_ptr<user::group_immigration> service::pgroup_user_immigration;
own_ptr<net::acceptor<user::socket>> service::pacceptor_user;

service::service()
{
	//, LOG_FILE, CONFIGURE_FILE

	//"channel manager server"sv

	// 1) account manager
	pmanager_account = make_own<user::manager>();
	pmanager_account->name("manager"sv);
	this->attach_child_object(pmanager_account);

	// 2) server
	pgroup_server_channel = make_own<server::group_channel>();
	pgroup_server_channel->name("group_server_channel"sv);
	this->attach_child_object(pgroup_server_channel);

	pgroup_server_Immigration = make_own<server::group_immigration>();
	pgroup_server_Immigration->name("group_server_immigration"sv);
	this->attach_child_object(pgroup_server_Immigration);

	pacceptor_server_lobby = make_own<net::acceptor<server::socket>>();
	pacceptor_server_lobby->name("acceptor_server"sv);
	this->attach_child_object(pacceptor_server_lobby);

	// 3) user
	pgroup_user_immigration = make_own<user::group_immigration>();
	pgroup_user_immigration->name("group_user_immigration"sv);
	this->attach_child_object(pgroup_user_immigration);

	pacceptor_user = make_own<net::acceptor<user::socket>>();
	pacceptor_user->name("acceptor_user"sv);
	this->attach_child_object(pacceptor_user);
}
