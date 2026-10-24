//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                        network socket http classes                        *
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
// net::io::connect_requestable::Ntcp_reconnect_ssl
//
//-----------------------------------------------------------------------------
class net::io::connect_requestable::Ntcp_reconnect_ssl :
	public						net::io::connect_requestable::Ntcp_reconnect,
	virtual public				Issl_cryptable
{
protected:
			Ntcp_reconnect_ssl() noexcept;
	virtual	~Ntcp_reconnect_ssl() noexcept;

public:
			// - _host_name: 접속할 서버 이름 (SNI + 서버 인증서 hostname 검증, 비어 있으면 인증서 chain만 검증한다.)
			void				prepare_ssl(bool _flag_enable, std::string_view _host_name = {});

protected:
	virtual	void				process_reset_connector() noexcept override;
	virtual	void				process_complete_connecting(uintptr_t _result) override;

protected:
			object_ptr<crypt::ssl_context> m_pssl_context;
			std::string			m_ssl_host_name;
};


}
