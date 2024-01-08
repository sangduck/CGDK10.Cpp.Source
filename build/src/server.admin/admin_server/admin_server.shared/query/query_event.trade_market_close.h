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

@class		query_event::makret_close

@brief
@todo
@ref
@details

*/
//-----------------------------------------------------------------------------
namespace query_event
{

class makret_close :
	public						query::rdb::query
{
public:
			makret_close() noexcept;
	virtual ~makret_close() noexcept;

public:
	virtual void				process_execute_query() override;
			
public:
			object_ptr<server_entity::Isocket> m_psocket;
};


}