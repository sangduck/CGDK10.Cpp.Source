#include "include.h"

own_ptr<service> g_pservice;


service::service()
{
	//("service"sv, "logfile.log"sv, "server_setting.json"sv)
	// 1) set name
	this->name("tut.server.simple_service.console"sv);

	// 2) create acceptor
	this->pacceptor_user = make_own<net::acceptor<socket_tcp_user>>();
	this->pacceptor_user->name("acceptor_user"sv);
	this->attach_child_object(pacceptor_user);

	// 3) create socket
	this->psocket_tcp = make_own<socket_tcp_server>();
	this->psocket_tcp->name("socket_tcp"sv);
	this->attach_child_object(psocket_tcp);
}
