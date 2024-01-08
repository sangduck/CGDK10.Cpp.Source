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
// web::socket
//
// ----------------------------------------------------------------------------
class client::web::socket :
// inherited classes)
	public						net::socket::http_server,
	public						Nmessage_transmitter,
	public						Igroupable<socket>,
	public						Iobserver<socket>,
	public						Npoolable<socket>
{
// constructor/destructor)
public:
			socket();
	virtual ~socket();

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
			object_ptr<Iobservable> m_webpage_ptarget;
			char				m_webpage_selected = 0;
			int					m_position = 0;
			uint64_t			m_flag_set = 0xffffffffffffffff;
};
