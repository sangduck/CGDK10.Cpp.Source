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

@class		query::makret_open

@brief
@todo
@ref
@details

*/
//-----------------------------------------------------------------------------
namespace query_event
{

class time_event_update :
	public						Iquery_event
{
public:
			time_event_update() noexcept;
	virtual ~time_event_update() noexcept;

public:
	virtual void				process_execute_query() override;
			
public:
			vector<sSERVER_TIME_EVENT_INSERT>			m_vector_server_time_event;
			object_ptr<time_event_server::operation>	m_pserver_operation;

			DEFINE_BUFFER_SERIALIZE(m_vector_server_time_event, m_pserver_operation);
};


}