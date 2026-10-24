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

class account :
	virtual public object::Idestroyable
{
public:
			account();
	virtual	~account() noexcept;

	virtual	result_code		destroy() noexcept override;

public:
			lock_object		cs_account_info;
			sACCOUNT_INFO	account_info;
			AREA_INDEX		area_index_now;
			own_ptr<area::entity_player> parea_entity;
			object_ptr<socket_user> psocket;
};
