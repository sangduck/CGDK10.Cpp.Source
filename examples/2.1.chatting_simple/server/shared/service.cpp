#include "pch.h"

own_ptr<service> g_pservice;


service::service()
{
	//("service", "chatting_server_setting.log", "chatting_server_setting.json")

	// 1) manager
	this->m_pmanager_user = make_own<user::manager>();
	this->m_pmanager_user->name("user_manager"sv);
	this->attach_child_object(this->m_pmanager_user);

	this->m_pmanager_channel = make_own<channel::manager>();
	this->m_pmanager_channel->name("channel_manager"sv);
	this->attach_child_object(this->m_pmanager_channel);

	// 2) user immigration group
	this->pgroup_user_immigration = make_own<user::group_immigration>();
	this->pgroup_user_immigration->name("group_immigration_user"sv);
	this->attach_child_object(this->pgroup_user_immigration);

	// 3) user acceptor
	this->pacceptor_user = make_own<net::acceptor<user::socket>>();
	this->pacceptor_user->name("acceptor_user"sv);
	this->attach_child_object(this->pacceptor_user);
}

service::~service() noexcept
{
}
