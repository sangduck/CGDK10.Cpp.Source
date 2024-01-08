#include "pch.h"

own_ptr<server_admin::instance_manager>		service::pmanager_instance;
own_ptr<net::acceptor<socket_server>>		service::pacceptor_server;
own_ptr<group_server_immigration>			service::pgroup_server_immigration;
own_ptr<group_server>						service::pgroup_server;
own_ptr<group_control>						service::pgroup_control;
own_ptr<net::acceptor<socket_web>>			service::pacceptor_web;
own_ptr<net::acceptor<socket_websocket>>	service::pacceptor_websocket;
own_ptr<group_websocket_immigration>		service::pgroup_websocket_immigration;
own_ptr<group_websocket>					service::pgroup_websocket;
own_ptr<socket_admin_server>				service::psocket_to_admin_server;

service::service()
{
	pmanager_instance = make_own<server_admin::instance_manager>();
	pmanager_instance->set_name("server_entity_manager"sv);
	this->Nattachable<object::Istate>::process_attach(pmanager_instance);

	pgroup_server_immigration = make_own<group_server_immigration>();
	pgroup_server_immigration->set_name("group_server_immigration"sv);
	this->Nattachable<object::Istate>::process_attach(pgroup_server_immigration);
	
	pgroup_server = make_own<group_server>();
	pgroup_server->set_name("group_server"sv);
	this->Nattachable<object::Istate>::process_attach(pgroup_server);
	
	pgroup_control = make_own<group_control>();
	pgroup_control->set_name("group_control"sv );
	this->Nattachable<object::Istate>::process_attach(pgroup_control);

	pacceptor_server = make_own<net::acceptor<socket_server>>();
	pacceptor_server->set_name("acceptor_server"sv);
	this->Nattachable<object::Istate>::process_attach(pacceptor_server);

	pgroup_websocket_immigration = make_own<group_websocket_immigration>();
	pgroup_websocket_immigration->set_name("group_websocket_immigration"sv);
	this->Nattachable<object::Istate>::process_attach(pgroup_websocket_immigration);

	pgroup_websocket = make_own<group_websocket>();
	pgroup_websocket->set_name("group_websocket"sv);
	this->Nattachable<object::Istate>::process_attach(pgroup_websocket);

	pacceptor_web = make_own<net::acceptor<socket_web>>();
	pacceptor_web->set_name("acceptor_web"sv);
	this->Nattachable<object::Istate>::process_attach(pacceptor_web);

	pacceptor_websocket = make_own<net::acceptor<socket_websocket>>();
	pacceptor_websocket->set_name("acceptor_websocket"sv);
	this->Nattachable<object::Istate>::process_attach(pacceptor_websocket);

	psocket_to_admin_server = make_own<socket_admin_server>();
	psocket_to_admin_server->set_name("socket_to_admin_server"sv);
	this->Nattachable<object::Istate>::process_attach(psocket_to_admin_server);
}

void service::on_service_setting(context& _context)
{
	// definition) 
#if defined(_WIN32)
	#define	SETTING_FILE_PATH	"./"sv
	#define	SETTING_FILE_NAME	"host_agent_setting.json"sv
#elif defined(__linux__)
	#define	SETTING_FILE_PATH	"/home/sangduck/projects/CGDK10.C++/src/server.admin/host_agent.service/"sv
	#define	SETTING_FILE_NAME	"host_agent_setting.json"sv
#endif

	// 2) read setting json file
	_context.read_from_json_file(api::file::make_full_path(SETTING_FILE_PATH, SETTING_FILE_NAME));
}
