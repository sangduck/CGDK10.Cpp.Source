//*****************************************************************************
//*                                                                           *
//*                          CGCII mmog field system                          *
//*                                                                           *
//*                               field server                                *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
//*  Best for Game Developement and Optimized for Game Developement.          *
//*                                                                           *
//*                 ¨Ï 2016 Cho Sanghyun All right reserved.                  *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************
#pragma once

class service : public server_system::Xservice
{
public:
			service();
public:
	// 1) area manager
			own_ptr<area::manager> m_parea_mamanger;

	// 2) account manager
			own_ptr<account_manager> m_paccount_manager;

	// 3) user socket acceptor
			own_ptr<group_user_immigration> pgroup_user_immigration;
			own_ptr<net::acceptor<socket_user>> pacceptor_user;
};
