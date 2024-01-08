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

@class		query_event::market_state

@brief
@todo
@ref
@details

*/
//-----------------------------------------------------------------------------
namespace query_event
{

class market_status :
	public						Iquery_event
{
public:
			market_status() noexcept;
	virtual ~market_status() noexcept;

public:
	virtual void				process_execute_query() override;
			
public:
			uint64_t							m_uid;
			server_admin::sSERVER_SETTING_INFO	m_info_server_setting;
			net::sockaddr						m_peer_address;
			object_ptr<server_entity::socket>	m_psocket;

			DEFINE_BUFFER_SERIALIZE(m_uid, m_info_server_setting, m_peer_address);
};


}