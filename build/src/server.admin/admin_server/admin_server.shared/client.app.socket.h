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
// client::admin::socket
//
//-----------------------------------------------------------------------------
class client::app::socket :
	public						net::socket::tcp_buffered<>,
	public						Igroupable<socket>,
	public						Nmessage_transmitter
{
public:
			socket() noexcept;
	virtual ~socket() noexcept;

protected:
	// 1) Connect/disconnect시
	virtual void				on_request_connect	(const net::sockaddr& _paddr_peer) override;
	virtual void				on_connect			(net::io::Iconnective* _pconnective) override;
	virtual void				on_fail_connect		(net::io::Iconnective* _pconnective, uint64_t _reason) override;
	virtual	void				on_disconnect		(uint64_t _reason) override;

	// 2) Message받았을 시
	virtual result_code			on_message			(sMESSAGE_NETWORK& _msg) override;

public:

	// statistics)
			int					m_idx_list;
};
