#include "pch.h"
#include "service.h"

own_ptr<service> g_pservice;

own_ptr<server_channel_manager::socket> service::psocket_server_channel_manager;
own_ptr<user::group> service::pgroup_user;
own_ptr<user::group_immigration> service::pgroup_user_immigration;
own_ptr<net::acceptor<user::socket>> service::pacceptor_user;
SERVER_INFO service::server_info;
lock_object service::lock_server_info;

service::service()
{
	//("shop server"sv, LOG_FILE, CONFIGURE_FILE)

	// 1) user
	pgroup_user= make_own<user::group>();
	pgroup_user->name("group_user"sv);
	this->attach_child_object(pgroup_user);

	pgroup_user_immigration = make_own<user::group_immigration>();
	pgroup_user_immigration->name("group_user_immigration"sv);
	this->attach_child_object(pgroup_user_immigration);

	pacceptor_user = make_own<net::acceptor<user::socket>>();
	pacceptor_user->name("acceptor_user"sv);
	this->attach_child_object(pacceptor_user);

	// 2) channel server socket
	psocket_server_channel_manager = make_own<server_channel_manager::socket>();
	psocket_server_channel_manager->name("socket_channel_manager"sv);
	this->attach_child_object(psocket_server_channel_manager);
}

void service::update_player_count()
{
	// Check)
	RETURN_IF(this->pacceptor_user.empty(), );

	SCOPED_LOCK(lock_server_info)
	{
		server_info.nServer_Player_Count = pacceptor_user->count();
	}
}

shared_buffer service::message_REQUEST_CERTIFICATION()
{
	// 1) 버퍼를 할당받는다. (작성할 메시지 크기만큼...)
	auto buf_temp = alloc_shared_buffer(8+sizeof(SERVER_INFO));

	// 2) 메시지를 작성한다.
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::REQUEST_CERTIFICATION);

	SCOPED_LOCK(lock_server_info)
	{
		buf_temp.append<SERVER_INFO>(server_info);
		buf_temp.append(this->name());
	}

	// 3) 메시지의 제일 앞에 메시지의 길이를 써넣는다.
	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

	// return) 
	return buf_temp;
}

shared_buffer service::message_SEND_LOBBYSERVER_STATUS()
{
	// 1) 버퍼를 할당받는다. (작성할 메시지 크기만큼...)
	auto buf_temp = alloc_shared_buffer(8+sizeof(SERVER_INFO));

	// 2) 메시지를 작성한다.
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::SEND_LOBBYSERVER_STATUS);
	SCOPED_LOCK(lock_server_info)
	{
		buf_temp.append<SERVER_INFO>(server_info);
	}

	// 3) 메시지의 제일 앞에 메시지의 길이를 써넣는다.
	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

	// return) 
	return buf_temp;
}