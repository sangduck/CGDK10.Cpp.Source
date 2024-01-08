//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                          Server Admin Host Agent                          *
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

// ----------------------------------------------------------------------------
//
// socket_web
//
// 1. socket_web는...
//    1) Server에서 HostAgent로 연결을 받는 Socket임.
//    2) 일반 Server에서 접속해오는 Socket임
//
// ----------------------------------------------------------------------------
class socket_websocket :
// inherited classes)
	public						net::socket::http_server,
	public						Nmessage_transmitter,
	public						Igroupable<socket_websocket>,
	public						Iobserver<socket_websocket>,
	public						Npoolable<socket_websocket>
{
// constructor/destructor)
public:
			socket_websocket();
	virtual ~socket_websocket();

// frameworks) 
public:
	virtual void				on_connect(net::io::Iconnective* _pconnective) override;
	virtual void				on_disconnect(uint64_t _disconnect_reason) override;

// message)
protected:
	virtual result_code			on_message(sMESSAGE_HTTP_REQUEST& _msg) override;
	virtual result_code			on_message(sMESSAGE_HTTP_WEBSOCKET& _msg) override;
	virtual result_code			on_message(sMESSAGE_NETWORK& _msg) override;

public:
			shared_buffer		make_webmessage_set_main_page(std::string_view _command, std::string_view _page);

public:
			object_ptr<Iobservable_websocket> m_webpage_ptarget;
			char				m_webpage_selected = 0;
			int					m_position = 0;
			uint64_t			m_flag_set = 0xffffffffffffffff;
};
