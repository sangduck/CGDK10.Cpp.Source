//*****************************************************************************
//*                                                                           *
//*                             CGCII Trade System                            *
//*                                                                           *
//*                               Control Server                              *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                  ⓒ2017. Cho SangHyun. All rights reserved                *
//*                          http://www.CGCIII.co.kr                          *
//*                                                                           *
//*****************************************************************************

#pragma once
//-----------------------------------------------------------------------------
//
// server::socket
//
//-----------------------------------------------------------------------------
class server_entity::socket :
	public						Isocket,
	public						Igroupable<socket>
{
public:
			socket() noexcept;
	virtual ~socket() noexcept;

public:
			void				request_service_start(const std::string& _str_setting);
			void				request_service_stop();
			void				request_service_destroy();

protected:
	// 1) Connect/disconnect시
	virtual void				on_request_connect	(const net::sockaddr& _paddr_peer) override;
	virtual void				on_connect			(net::io::Iconnective* _pconnective) override;
	virtual void				on_fail_connect		(net::io::Iconnective* _pconnective, uint64_t _reason) override;
	virtual	void				on_disconnect		(uint64_t _reason) override;

	// 2) Message받았을 시
	virtual result_code			on_message			(sMESSAGE_NETWORK& _msg) override;

public:
	// 3) Server State
			uint64_t			m_pid;
			object_ptr<instance> m_pserver_entity;

	// statistics)
			uint32_t			m_idx_list = static_cast<uint32_t>(-1);
			std::string			m_str_setting;

};
