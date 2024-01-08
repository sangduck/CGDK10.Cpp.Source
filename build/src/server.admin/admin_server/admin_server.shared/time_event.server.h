//*****************************************************************************
//*                                                                           *
//*                             CGCII Trade System                            *
//*                                                                           *
//*                               Trading Server                              *
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
/**

@class		time_event::CMarketClose

@brief
@todo
@ref
@details

*/
//-----------------------------------------------------------------------------
namespace time_event_server
{

class operation :
	virtual public				time_event::event
{
public:
			operation() noexcept;
	virtual ~operation() noexcept;

private:
	//virtual	void				process_done_time_event() noexcept override;

public:
			uint64_t			m_idSource;
			int					m_idx_list;
};


}