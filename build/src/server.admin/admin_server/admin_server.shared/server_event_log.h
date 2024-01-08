//*****************************************************************************
//*                                                                           *
//*                             CGCII Trade System                            *
//*                                                                           *
//*                               Control Server                              *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                  ¨Ï2017. Cho SangHyun. All rights reserved                *
//*                          http://www.CGCIII.co.kr                          *
//*                                                                           *
//*****************************************************************************

#pragma once
//-----------------------------------------------------------------------------
//
// server::group_trade
//
//-----------------------------------------------------------------------------
class server_event_log :
	public						sSERVER_EVENT_LOG,
	virtual public				Ireferenceable
{
public:
			server_event_log() noexcept;
	virtual	~server_event_log() noexcept;
};
