#include "pch.h"

own_ptr<service> g_pservice;

service::service()
{
	// "CGCII game field server"sv, std::string_view(), CONFIGURE_FILE

	// 1) area manager
	m_parea_mamanger = make_own<area::manager>();
	m_parea_mamanger->name("area_manager"sv);
	attach_child_object(m_parea_mamanger);

	// 2) user manager
	m_paccount_manager = make_own<account_manager>();
	m_paccount_manager->name("account_manager"sv);
	attach_child_object(m_paccount_manager);

	// 3) area manager
	pgroup_user_immigration = make_own<group_user_immigration>();
	pgroup_user_immigration->name("user immigration"sv);
	attach_child_object(pgroup_user_immigration);

	// 4) user socket acceptor
	pacceptor_user = make_own<net::acceptor<socket_user>>();
	pacceptor_user->name("acceptor_user"sv);
	attach_child_object(pacceptor_user);
}
