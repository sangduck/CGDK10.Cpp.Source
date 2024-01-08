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

class makret_open :
	public						query::rdb::query,
	public						Ibuffer_serializable
{
public:
			makret_open() noexcept;
	virtual ~makret_open() noexcept;

public:
	virtual void				process_execute_query() override;
			
public:
			object_ptr<server_entity::Isocket> m_psocket;
			uint64_t			m_dump;

			DEFINE_BUFFER_SERIALIZE(m_dump);
};


}