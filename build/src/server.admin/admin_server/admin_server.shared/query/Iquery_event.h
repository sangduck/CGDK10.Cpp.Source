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

@class		query_event::server_close

@brief
@todo
@ref
@details

*/
//-----------------------------------------------------------------------------
class Iquery_event :
	public						query::rdb::query,
	public						Ibuffer_serializable
{
public:
			Iquery_event() : m_check_duplicate(false) {}
	virtual	~Iquery_event() {}

public:
			bool				m_check_duplicate;
};
