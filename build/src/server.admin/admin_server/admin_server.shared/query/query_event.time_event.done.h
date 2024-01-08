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

@class		query_event::makret_open

@brief
@todo
@ref
@details

*/
//-----------------------------------------------------------------------------
namespace query_event
{

class time_event_done :
	public						Iquery_event
{
public:
			time_event_done() noexcept;
	virtual ~time_event_done() noexcept;

public:
	virtual void				process_execute_query() override;
			
public:
			uint64_t									m_id_event;
			object_ptr<time_event_server::operation>	m_pserver_operation;

			DEFINE_BUFFER_SERIALIZE(m_id_event, m_pserver_operation);
};


}