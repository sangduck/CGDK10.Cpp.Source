#include "pch.h"

own_ptr<service> g_pservice;

service::service()
	
{
	// 1) set server type
	this->set_service_instance(make_service_instance<>(server_system::sRANK{ 0, 0, 0, 0 }));

	// 2) resource manager
	m_presource_manager = make_own<update::resource_manager>();
	m_presource_manager->name("update_resource_mamanger"sv);
	attach_child_object(m_presource_manager);

	// 3) groups
	m_pgroup_socket = make_own<group_socket>();
	attach_child_object(m_pgroup_socket);

	m_pgroup_socket_immigration = make_own<group_socket_immigration>();
	attach_child_object(m_pgroup_socket_immigration);

	// 4) acceptor
	m_pacceptor_tcp = make_own<net::acceptor<socket_tcp>>();
	m_pacceptor_tcp->name("acceptor_user"sv);
	attach_child_object(m_pacceptor_tcp);
}
