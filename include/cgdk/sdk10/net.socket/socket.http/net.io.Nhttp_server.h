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
// net::io::Nhttp_server
//
//-----------------------------------------------------------------------------
class net::io::Nhttp_server :
// inherited classes) 
	public						net::io::socket::tcp::Ndefault,
	public						net::io::Nsocket_ssl,
	public						net::io::connectable::tcp::Ndefault,
	public						net::io::connect_notify::Nbase,
	public						net::io::send_request::tcp::Ndefault,
	public						net::io::sender::Nhttp,
	virtual public				net::io::Ipacketable,
	virtual public				Imessageable
{
// public) 
public:
	// 3) flag...
			bool				is_websocket() const noexcept { return m_msg_http_request.protocol == eHTTP_PROTOCOL::WEBSOCKET_; }
			const auto&			get_http_info() const noexcept { return m_msg_http_request; }

	// 4) 크기 제한 (넘으면 접속을 끊는다. 0: 제한 없음)
	//    - header : request line + header (기본 64KB)
	//    - body   : Content-Length / chunked 합계 / websocket frame 하나 (기본 16MB)
			void				set_http_limits(std::size_t _max_header_size, std::size_t _max_body_size) noexcept { this->m_max_header_size = _max_header_size; this->m_max_body_size = _max_body_size; }
	[[nodiscard]] std::size_t	get_max_header_size() const noexcept { return this->m_max_header_size; }
	[[nodiscard]] std::size_t	get_max_body_size() const noexcept { return this->m_max_body_size; }

	// 5) websocket PING (keepalive. 받는 쪽(browser)은 PONG으로 답한다. false: websocket이 아니거나 보낼 수 없음)
			bool				send_websocket_ping();

// frameworks)
protected:
	// 1) on_message
	virtual result_code			on_message(sMESSAGE_HTTP_REQUEST& /*_msg*/) {return 0;}
	virtual result_code			on_message(sMESSAGE_HTTP_WEBSOCKET& /*_msg*/) { return 0; }
	virtual result_code			on_message(sMESSAGE_NETWORK& /*_msg*/) { return 0; }

	// 2) websocket upgrade (called before the handshake response. false: '403 Forbidden' and disconnect)
	//    - ex) check 'Origin' header (browsers can open a websocket to any site)
	virtual bool				on_websocket_upgrade(const sMESSAGE_HTTP_REQUEST& /*_request*/) { return true; }

// implementation)
protected:
	virtual	void				process_pre_message (sMESSAGE_NETWORK& /*_msg*/) {}
	virtual	void				process_reset_packetable () noexcept override { m_msg_http_request.reset(); m_size_head_scanned = 0; this->_reset_chunk(); this->m_ws_fragments.clear(); this->m_ws_fragment_opcode = 0; }
	virtual	std::size_t			process_packet (shared_buffer& _buffer, const net::sockaddr& _address) override;
			std::size_t			process_packet_http (shared_buffer& _buffer, const net::sockaddr& _address);
			std::size_t			process_packet_websocket (shared_buffer& _buffer, const net::sockaddr& _address);
			std::size_t			process_parsing_http (shared_buffer _buffer);
			void				process_parsing_http_head (shared_buffer _buffer);
	virtual	result_code			process_message (sMESSAGE& _msg) override;

	virtual void				process_on_connect () override;
	virtual	void				process_on_fail_connect (uint64_t _disconnect_reason) override;
	virtual	void				process_on_disconnect (uint64_t _disconnect_reason, eSOCKET_STATE _socket_state) override;

protected:
			// declare) 
			sMESSAGE_HTTP_REQUEST m_msg_http_request;

			std::size_t			m_size_head_scanned{ 0 };		// - head 끝(\r\n\r\n)을 찾아본 위치 (head가 나뉘어 올 때)
			std::size_t			m_max_header_size{ 64 * 1024 };
			std::size_t			m_max_body_size{ 16 * 1024 * 1024 };

			[[noreturn]] void	_throw_too_large(std::string_view _what, std::size_t _size, std::size_t _max) const;

			// - chunked body (size line -> data + CRLF -> ... -> 0 -> trailer -> empty line)
			std::string			m_chunked_body;
			std::size_t			m_chunk_size{ 0 };
			int					m_chunk_state{ 0 };		// - 0: size line, 1: data, 2: trailer
			void				_reset_chunk() noexcept { this->m_chunked_body.clear(); this->m_chunk_size = 0; this->m_chunk_state = 0; }

			// - websocket 조각(fragment) : FIN이 아닌 text/binary로 시작해서 continuation(0)으로 이어지고 FIN으로 끝난다. (모아서 하나로 전달한다.)
			std::string			m_ws_fragments;
			uint16_t			m_ws_fragment_opcode{ 0 };
			void				_process_websocket_data(uint16_t _opcode, bool _is_fin, const shared_buffer& _buffer, const buffer_view& _payload);
};

}