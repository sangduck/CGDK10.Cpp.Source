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

class time_event_process :
	public						Iquery_event
{
public:
			time_event_process() noexcept;
	virtual ~time_event_process() noexcept;

public:
	virtual void				process_execute_query() override;
			
public:
			uint64_t										m_id_event;
			object_ptr<time_event_server::server_operation>	m_pserver_operation;

			DEFINE_BUFFER_SERIALIZE(m_id_event, m_pserver_operation);
};


}