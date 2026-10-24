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
	virtual public Ireferenceable
{
public:
			account();
	virtual	~account() noexcept;

public:
			ACCOUNT_INDEX	m_account_index;
};
