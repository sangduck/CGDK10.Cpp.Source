//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 9.0 / release 2017.01.03                        *
//*                                                                           *
//*                 Distributed Object Storage System Classes                 *
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
//-----------------------------------------------------------------------------
// doss::entity
//
//   
//
//-----------------------------------------------------------------------------
namespace CGDK
{

template <class TSESSION>
struct doss::sCOMMAND
{
	uint32_t		command{ 0 };
	shared_buffer	buf_command;
	object_ptr<TSESSION> psession;
	chrono::time::utc::time_point time_request;
	chrono::time::utc::time_point time_begin;
	chrono::time::utc::time_point time_complete;
	int				process{ 0 };
};

template <class TSESSION_ID = uint64_t>
struct doss::sSESSION
{
	typedef TSESSION_ID session_id_t;

	session_id_t	session_id{ 0 };
	chrono::time::utc::time_point time_checkin;
	chrono::time::utc::time_point time_last_request;
	chrono::time::utc::time_point time_last_complete;

	auto			get_session_id() const noexcept { return session_id; }
	void			reset() noexcept
	{
		this->session_id = 0;
		this->time_checkin = chrono::time::utc::time_point();
		this->time_last_request = chrono::time::utc::time_point();
		this->time_last_complete = chrono::time::utc::time_point();
	}
};

}
