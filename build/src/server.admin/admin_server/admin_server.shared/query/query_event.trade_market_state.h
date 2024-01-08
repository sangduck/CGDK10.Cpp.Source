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

@class		query::market_state

@brief
@todo
@ref
@details

*/
//-----------------------------------------------------------------------------
namespace query_event
{

class market_state :
	public						Iquery_event
{
public:
			market_state() noexcept;
	virtual ~market_state() noexcept;

public:
	virtual void				process_execute_query() override;
			
public:
			uint64_t							m_uid;
			server_admin::sSERVER_SETTING_INFO	m_info_server_setting;
			net::sockaddr						m_peer_address;
			eOBJECT_STATE						m_state;
			object_ptr<server_entity::socket>	m_psocket;

			DEFINE_BUFFER_SERIALIZE(m_uid, m_state, m_info_server_setting, m_peer_address, m_state);
};


}