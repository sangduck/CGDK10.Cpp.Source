//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                          server.query.rdb.odbc                            *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
//*  Best for Game Developement and Optimized for Game Developement.          *
//*                                                                           *
//*                (c) 2003. Cho Sanghyun. All right reserved.                *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************

#pragma once

namespace CGDK
{
//-----------------------------------------------------------------------------
//
// query::rdb::odbc::data_source
//
//
//-----------------------------------------------------------------------------
class query::rdb::odbc::data_source :
// inherited classes)
	virtual public				Ireferenceable
{
public:
			data_source() noexcept;
	virtual	~data_source() noexcept;

public:
	void	open(const std::string_view _str_initialization);

public:
			SQLHENV				m_handle_env;
			SQLUSMALLINT		m_driver_completion;
			std::string			m_str_initialization;
			uint32_t			m_session_wait_timeout = 0;	// (seconds) 'SET SESSION wait_timeout' on connecting (MySQL/MariaDB, 0: not set)
};


}