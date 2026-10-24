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
// net::io::Nhttp_client
//
// 1. net::io::Nhttp_server은~
//     web client용 socket
//     http와 websocket를 지원한다.
//
//-----------------------------------------------------------------------------
class net::io::Nhttp_client :
// inherited classes)
	public						net::io::connect_requestable::Nhttp,
	public						net::io::connect_notify::Nbase,
	public						net::io::socket::tcp::Ndefault,
	public						net::io::connectable::tcp::Ndefault,
	public						net::io::Nsocket_ssl,
	public						net::io::send_request::tcp::Ndefault,
	public						net::io::sender::Nhttp,
	public						net::io::add_on::Nsocket_sustain,
	virtual public				net::io::Ipacketable,
	virtual protected			Imessageable
{
// public) 
public:
			Nhttp_client();

	// 1) for websocket
			bool				send(buffer_view _buffer, eHTTP_METHOD _method = eHTTP_METHOD::BINARY_);
			bool				send(const std::string_view _string);

	// 3) 
			bool				is_websocket() const noexcept { return m_msg_http_response.protocol == eHTTP_PROTOCOL::WEBSOCKET_; }

	// 4) 크기 제한 (넘으면 접속을 끊는다. 0: 제한 없음)
	//    - header : status line + header (기본 64KB)
	//    - body   : Content-Length / chunked 합계 / websocket frame 하나 (기본 64MB)
			void				set_http_limits(std::size_t _max_header_size, std::size_t _max_body_size) noexcept { this->m_max_header_size = _max_header_size; this->m_max_body_size = _max_body_size; }
	[[nodiscard]] std::size_t	get_max_header_size() const noexcept { return this->m_max_header_size; }
	[[nodiscard]] std::size_t	get_max_body_size() const noexcept { return this->m_max_body_size; }

	// 5) 응답 대기 시간 (요청을 보낸 뒤 이 시간 동안 받은 것이 없으면 끊고 '504 Gateway Timeout'으로 끝낸다. 기본 60초, 0: 기다린다.)
	//    - 받는 중에는 받을 때마다 다시 센다. (websocket으로 바뀐 뒤에는 쓰지 않는다.)
			void				set_response_timeout(chrono::tick::duration _timeout) noexcept { this->time_out_receive(_timeout); }
	[[nodiscard]] auto			get_response_timeout() const noexcept { return this->time_out_receive(); }

	// 6) 접속 시간 제한 (접속 요청 후 이 시간 안에 접속되지 않으면 끊고 '504 Gateway Timeout'으로 끝낸다. 기본 10초, 0: OS 기본값)
	//    (응답하지 않는 주소는 OS 기본값(windows 약 21초, linux 약 2분)까지 기다린다.)
			void				set_connect_timeout(chrono::tick::duration _timeout) noexcept { this->time_out_connecting(_timeout); }
	[[nodiscard]] auto			get_connect_timeout() const noexcept { return this->time_out_connecting(); }

// implementation)
protected:
	virtual	result_code			on_message(sMESSAGE_HTTP_RESPONSE& _msg);
	virtual	result_code			on_message(sMESSAGE_HTTP_WEBSOCKET& _msg);
	virtual result_code			on_message(sMESSAGE_NETWORK& /*_msg*/) { return 0; }

// implementation)
protected:
	virtual	void				process_pre_message (sMESSAGE_NETWORK& /*_msg*/) {}
	virtual	void				process_reset_packetable () noexcept override { m_msg_http_response.reset(); m_size_head_scanned = 0; this->_reset_chunk(); this->m_ws_fragments.clear(); this->m_ws_fragment_opcode = 0; this->m_is_body_until_close = false; this->m_body_until_close.clear(); }
	virtual	size_t				process_packet ( shared_buffer& _buffer, const net::sockaddr& _address) override;
			size_t				process_packet_http( shared_buffer& _buffer, const net::sockaddr& _psocket_address);
			size_t				process_packet_websocket(shared_buffer& _buffer, const net::sockaddr& _psocket_address);
			size_t				process_parsing_http (shared_buffer _buffer);
	static	void				process_parsing_http_head (sMESSAGE_HTTP_RESPONSE& _msg_http_response);
	virtual	result_code			process_message (sMESSAGE& _msg) override;

	virtual void				process_on_connect () override;
	virtual	void				process_on_fail_connect (uint64_t _disconnect_reason) override;
	virtual	void				process_on_disconnect (uint64_t _disconnect_reason, eSOCKET_STATE _socket_state) override;
	virtual	void				on_timeout_receive (chrono::tick::duration _tick_differ) override;

protected:
			// declare) 
			sMESSAGE_HTTP_RESPONSE m_msg_http_response;
			std::size_t			m_size_head_scanned{ 0 };		// - head 끝(\r\n\r\n)을 찾아본 위치 (head가 나뉘어 올 때)
			std::size_t			m_max_header_size{ 64 * 1024 };
			std::size_t			m_max_body_size{ 64 * 1024 * 1024 };

			[[noreturn]] void	_throw_too_large(std::string_view _what, std::size_t _size, std::size_t _max);

			// - chunked body (size line -> data + CRLF -> ... -> 0 -> trailer -> empty line)
			//   chunk마다 is_completed == false인 message(buf_message: 그 chunk)로 전달하고, 끝나면 is_completed == true(buf_message: 전체 body)로 전달한다.
			std::string			m_chunked_body;
			std::size_t			m_chunk_size{ 0 };
			int					m_chunk_state{ 0 };		// - 0: size line, 1: data, 2: trailer
			void				_reset_chunk() noexcept { this->m_chunked_body.clear(); this->m_chunk_size = 0; this->m_chunk_state = 0; }

			// - websocket 조각(fragment) : FIN이 아닌 text/binary로 시작해서 continuation(0)으로 이어지고 FIN으로 끝난다. (모아서 하나로 전달한다.)
			std::string			m_ws_fragments;
			uint16_t			m_ws_fragment_opcode{ 0 };
			void				_process_websocket_data(uint16_t _opcode, bool _is_fin, const shared_buffer& _buffer, const buffer_view& _payload);

			// - the body without Content-Length and chunked : until the connection is closed (completed in process_on_disconnect)
			bool				m_is_body_until_close{ false };
			std::string			m_body_until_close;

			// - the method of the request (the response of HEAD has no body)
			eHTTP_METHOD		m_request_method{ eHTTP_METHOD::NONE_ };

			// - the request (copied in the lock)
			object_ptr<Ihttp_request> _get_request() const noexcept { std::lock_guard cs(this->m_lock_prequest); return this->m_prequest; }
};


}