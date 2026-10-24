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

class Igroup_user :
	public	CGDK::group::list_sender<socket_user>,
	public	object::Ninitializable_startable,
	public	object::Nnameable,
	public	net::io::messageable::Nbase
{
public:
	virtual ~Igroup_user() noexcept {}

public:
			int					m_idx_list = -1;
};
