#include "include.h"

own_ptr<service> g_pservice;


service::service()
{
	// 1) set name
	this->name("tut.server.simple_service.console"sv);

	// 2) create acceptor
	this->pacceptor_user = make_own<net::acceptor<socket_tcp_user>>();
	this->pacceptor_user->name("acceptor_user"sv);

	// 3) create socket
	this->psocket_tcp = make_own<socket_tcp_server>();
	this->psocket_tcp->name("socket_tcp"sv);
}

void service::on_initialize_request(context& /*_context*/)
{
	std::cout << "on initinaize request"sv << '\n';
}

void service::on_initializing(context& /*_context*/)
{
	std::cout << "on initinaizing"sv << '\n';
}

void service::on_initialize(context& /*_context*/)
{
	std::cout << "on initinaize"sv << '\n';
}

void service::on_start_request(context& /*_context*/)
{
	std::cout << "on start request"sv << '\n';
}

void service::on_starting(context& /*_context*/)
{
	std::cout << "on starting"sv << '\n';

	// 1) 20000번 포트에 listen을 시작한다.
	this->pacceptor_user->start({{net::ip::tcp::v4(), 20000}});

	// 2) localhost:21000번 포트에 접속을 시도한다.
	this->psocket_tcp->start({{ net::ip::make_address_v4("localhost"sv), 21000 }});
}

void service::on_start(context& /*_context*/)
{
	std::cout << "on start"sv << '\n';
}

void service::on_stop_request()
{
	std::cout << "on stop request"sv << '\n';
}

void service::on_stopping()
{
	std::cout << "on stopping"sv << '\n';

	// 1) socket의 접속을 종료한다.
	this->psocket_tcp->stop();

	// 1) acceptor를 중지한다.
	this->pacceptor_user->stop();
}

void service::on_stop()
{
	std::cout << "on stop"sv << '\n';
}

void service::on_destroy_request()
{
	std::cout << "on destroy request"sv << '\n';
}

void service::on_destroying()
{
	std::cout << "on destroying"sv << '\n';
}

void service::on_destroy()
{
	std::cout << "on destroy"sv << '\n';
}
