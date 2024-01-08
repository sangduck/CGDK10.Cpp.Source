#include "pch.h"

using namespace CGDK;

bool net::io::Isocket::closesocket(uint64_t _disconnect_reason) noexcept
{
	// check) DISCONNECT_REASON_MASK에 해당하는 부분의 Flag를 유저가 설정해서는 안됀다.
	CGASSERT_ERROR((_disconnect_reason & DISCONNECT_REASON_MASK) == 0);

	// 1) Active disconnect 속성 추가!
	_disconnect_reason &= ~DISCONNECT_REASON_MASK;
	_disconnect_reason |= DISCONNECT_REASON_ACTIVE;
	_disconnect_reason |= DISCONNECT_REASON_ABORTIVE;

	// 2) 강제 종료를 진행한다.
	return process_close_socket(_disconnect_reason);
}

net::sockaddr net::io::Isocket::process_get_peer_address(std::error_code& _error_code) const noexcept
{
	// declare) 
	net::sockaddr address_socket;

	// 1) set length
	address_socket.length = sizeof(net::sockaddr);
	
	// 2) get peer address
	auto result = api::socket::get_peer_address(native_handle(), reinterpret_cast<::sockaddr*>(&address_socket), &address_socket.length);

	// check) 
	if (result != false)
	{
		_error_code = std::error_code();
	}
	else
	{
		_error_code = std::make_error_code(std::errc::bad_address);
		address_socket.length = 0;
	}

	// return)
	return address_socket;
}

net::sockaddr net::io::Isocket::process_get_socket_address(std::error_code& _error_code) const noexcept
{
	// declare) 
	net::sockaddr address_socket;

	// 1) set length
	address_socket.length = sizeof(net::sockaddr);

	// 2) get socket address
	auto result = api::socket::get_socket_address(native_handle(), reinterpret_cast<::sockaddr*>(&address_socket), &address_socket.length);

	// check) 
	if (result != false)
	{
		_error_code = std::error_code();
	}
	else
	{
		_error_code = std::make_error_code(std::errc::bad_address);
		address_socket.length = 0;
	}

	// return) 
	return address_socket;
}

net::sockaddr net::io::Isocket::socket_address_in_public(std::error_code& _error_code) const noexcept
{
	// declare) 
	net::sockaddr address_socket;

	// 1) ...
	address_socket.length = sizeof(address_socket);

	// 2) Address를 얻는다.
	auto result = api::socket::get_socket_address(native_handle(), reinterpret_cast<::sockaddr*>(&address_socket), &address_socket.length);

	// 3) address가 존재하며 BIND_ANY일 경우 local을 설정한다.
	if (result == true && address_socket.length != 0 && !address_socket.is_bind_any())
	{
		auto port = address_socket.u_v4.sin_port;
		api::socket::instance().get_local_address_public_first(address_socket);
		address_socket.u_v4.sin_port = port;

		// - success!
		_error_code = std::error_code();
	}
	else
	{
		_error_code = std::make_error_code(std::errc::bad_address);
	}

	// return) 
	return address_socket;
}

void net::io::Isocket::maximum_send_buffer_size(std::size_t _size) noexcept
{
	// 1) saturate max
	if (_size > MAX_TCP_SEND_BUFFER_SIZE)
	{
		_size = MAX_TCP_SEND_BUFFER_SIZE;
	}

	// 1) saturate max
	if (_size < MIN_TCP_SEND_BUFFER_SIZE)
	{
		_size = MIN_TCP_SEND_BUFFER_SIZE;
	}

	// check) return if same
	RETURN_IF(_size == m_maximum_send_buffer_size);

	// 2) set value
	m_maximum_send_buffer_size = _size;
}

void net::io::Isocket::maximum_receive_buffer_size(std::size_t _size) noexcept
{
	// 1) saturate max
	if (_size > MAX_TCP_RECEIVE_BUFFER_SIZE)
	{
		_size = MAX_TCP_RECEIVE_BUFFER_SIZE;
	}

	if (_size < MIN_TCP_RECEIVE_BUFFER_SIZE)
	{
		_size = MIN_TCP_RECEIVE_BUFFER_SIZE;
	}

	// check) return if same
	RETURN_IF(_size == m_maximum_receive_buffer_size);

	// 2) set value
	m_maximum_receive_buffer_size = _size;
}

#if defined(_SYS_EPOLL_H)

intptr_t net::io::Isocket::process_execute_epoll([[maybe_unused]] intptr_t _event, [[maybe_unused]] std::size_t _param)
{
	return 0;
}

CGHANDLE net::io::Isocket::detach_epoll_handle() noexcept
{
	// 1) get 
	auto epoll_handle = m_epoll_handle;

	// check)
	RETURN_IF(epoll_handle == INVALID_SOCKET, epoll_handle);

	// 1) reset epoll handle
	m_epoll_handle = INVALID_SOCKET;

	// 3) reset executor
	object_ptr<executor::Iio_context> pio_context = std::move(m_pio_context);

	// check)
	CGASSERT_ERROR(pio_context.exist());

	// 4) detach handle
	pio_context->detach(epoll_handle);

	// return)
	return epoll_handle;
}

void net::io::Isocket::reset_epoll_event_flag() noexcept
{
	m_epoll_event_flag = 0;
	m_epoll_event_flag_queued = 0;
	m_epoll_event_processing_now = false;
}

void net::io::Isocket::post_epoll_event_EPOLLOUT() noexcept
{
	// 1) get executor
	auto pio_context = io_context();

	// check) 
	RETURN_IF(pio_context.empty());

	scoped_lock(m_epoll_event_cs)
	{
		//  설명) EPOLLOUT event를 설정한 후 이미 EPOLLIN으로 event가 진행중이면
		//        플래그만 변경시켜놓고 끝낸다.
		//        근데 EPOLLIN event가 진행중이 아니면 직접 set_event()를 건다.

		// check) EPOLLOUT이 이미 설정된 상태면 안됀다.
		CGASSERT_ERROR((this->m_epoll_event_flag & EPOLLOUT) == 0);

		// 2) set EPOLLOUT flag
		add_epoll_event_flag(EPOLLOUT);

		// cheeck) epoll_event가 진행중일 경우 m_epoll_event_flag만 update하고 종료한다.
		RETURN_IF(m_epoll_event_processing_now == true);

		// 3) epoll event를 업데이트한다.
		pio_context->set_event(get_epoll_handle(), m_epoll_event_flag);
	}
}

#endif

namespace CGDK
{
	extern std::shared_mutex g_cs_map_message_type_info;
	extern std::map<uint64_t, std::string> g_map_message_type_info;

namespace net
{
namespace io
{
	extern bool g_enable_plogger_socket_debug;
	extern object_ptr<Ilogger> g_plogger_socket_debug;

	void enable_default_socket_debug_logging(bool _enable) noexcept
	{
		scoped_lock(g_cs_map_message_type_info)
		{
			// check)
			RETURN_IF(g_enable_plogger_socket_debug == _enable);

			g_enable_plogger_socket_debug = _enable;
		}

		// log)
		if (_enable)
			LOG_PROGRESS << "@ enable  default socket logger"sv;
		else
			LOG_PROGRESS << "@ disable  default socket logger"sv;
	}

	void disable_default_socket_debug_logging() noexcept
	{
		enable_default_socket_debug_logging(false);
	}

	bool is_default_socket_debug_logging_enabled() noexcept
	{
		return g_enable_plogger_socket_debug;
	}

	void set_default_socket_debug_logger(Ilogger* _logger) noexcept
	{
		scoped_lock(g_cs_map_message_type_info)
		{
			// check)
			RETURN_IF(g_plogger_socket_debug == _logger);

			// 1) set default socket logger
			g_plogger_socket_debug = _logger;
		}

		// 2) get name
		auto pnameable = dynamic_cast<object::Inameable*>(_logger);

		// log) 
		if (pnameable != nullptr)
			LOG_PROGRESS << "@ set default socket logger ("sv << pnameable->name() << "')"sv;
		else
			LOG_PROGRESS << "@ set default socket logger"sv;
	}

	object_ptr<Ilogger> reset_default_socket_debug_logger() noexcept
	{
		// declare)
		object_ptr<Ilogger> plogger_socket_debug;

		scoped_lock(g_cs_map_message_type_info)
		{
			// check)
			RETURN_IF(g_plogger_socket_debug.empty(), plogger_socket_debug);

			// 1) reset default socket logger
			plogger_socket_debug = std::move(g_plogger_socket_debug);
		}

		// 2) get name
		auto pnameable = dynamic_cast<object::Inameable*>(plogger_socket_debug.get());

		// log) 
		if (pnameable != nullptr)
			LOG_PROGRESS << "@ reset default socket logger ("sv << pnameable->name() << "')"sv;
		else
			LOG_PROGRESS << "@ reset default socket logger"sv;

		// return)
		return plogger_socket_debug;
	}

	object_ptr<Ilogger> get_default_socket_debug_logger() noexcept
	{
		std::lock_guard<std::shared_mutex> cs(g_cs_map_message_type_info);

		return g_plogger_socket_debug;
	}

	void _debug_log_socket_connected(CGSOCKET _hsocket, net::sockaddr _sockaddr)
	{
		// declare)
		object_ptr<Ilogger> plogger_socket_debug;

		scoped_lock(g_cs_map_message_type_info)
		{
			plogger_socket_debug = g_plogger_socket_debug;
		}

		// check)
		RETURN_IF(plogger_socket_debug.empty());

		// trace)
		LOG(plogger_socket_debug, log_type_t::debug) << " socket["sv << _hsocket << "] connected from '"sv << to_string<char>(_sockaddr) << "'"sv;
	}

	void _debug_log_socket_disconnected(CGSOCKET _hsocket, uint64_t _reason)
	{
		// declare)
		object_ptr<Ilogger> plogger_socket_debug;

		scoped_lock(g_cs_map_message_type_info)
		{
			plogger_socket_debug = g_plogger_socket_debug;
		}

		// check)
		RETURN_IF(plogger_socket_debug.empty());

		// declare)
		std::string str_reason_system;
		std::string_view str_reason_user;

		// 1) system reason
		auto reason_system = _reason & DISCONNECT_REASON_MASK;

		if (reason_system != 0)
		{
			if (reason_system & DISCONNECT_REASON_FAIL)
			{
				str_reason_system = "<FAIL>";
			}

			if (reason_system & DISCONNECT_REASON_ACTIVE)
			{
				str_reason_system += "<ACTIVE>";
			}

			if (reason_system & DISCONNECT_REASON_ABORTIVE)
			{
				str_reason_system += "<ABORTIVE>";
			}

			if (reason_system & DISCONNECT_REASON_EXCEPTION)
			{
				str_reason_system += "<EXCEPTION>";
			}
		}

		// 2) user reason
		auto reason_user = _reason & DISCONNECT_REASON_MASK_USER;

		if (reason_user != 0 && reason_user < DISCONNECT_REASON_OPEN_SOCKET_LIMITS)
		{
			switch (reason_user)
			{
			case	DISCONNECT_REASON_OVERFLOW_MAX_MESSAGES:		str_reason_user = "OVERFLOW_MAX_MESSAGES"sv; break;		// send시 message 수의 Overflow로 접속을 종료했다.				
			case	DISCONNECT_REASON_OVERFLOW_MAX_BYTES:			str_reason_user = "OVERFLOW_MAX_BYTES"sv; break;		// send시 Bytes 수의 Overflow로 접속을 종료했다.
			case	DISCONNECT_REASON_MESSAGE_LENGTH_TOO_LONG:		str_reason_user = "MESSAGE_LENGTH_TOO_LONG"sv; break;	// message의 길이가 너무 길다
			case	DISCONNECT_REASON_MESSAGE_LENGTH_TOO_SHORT:		str_reason_user = "MESSAGE_LENGTH_TOO_SHORT"sv; break;	// message의 길이가 너무 짧다
			case	DISCONNECT_REASON_MESSAGE_UNHANDLED_EXCEPTION:	str_reason_user = "MESSAGE_UNHANDLED_EXCEPTION"sv; break;// message처리 중 핸들링 되지 않은 Exception
			case	DISCONNECT_REASON_BUFFER_IS_NULL:				str_reason_user = "BUFFER_IS_NULL"sv; break;			// shared_buffer의 buf값이 nullptr이다.
			case	DISCONNECT_REASON_INVALID_STATE:				str_reason_user = "INVALID_STATE"sv; break;				// 처리 과정상 있을 수 없는 상태이다.
			case	DISCONNECT_REASON_INVALID_SOCKET_HANDLE:		str_reason_user = "INVALID_SOCKET_HANDLE"sv; break;		// socket handle이 INVALID_SOCKET_HANDLE이다.
			case	DISCONNECT_REASON_FAIL_TO_SOCKET_PREPARE:		str_reason_user = "FAIL_TO_SOCKET_PREPARE"sv; break;	// socket의 prepare단계에서 실패했다.
			case	DISCONNECT_REASON_FAIL_TO_SOCKET_ACCEPT:		str_reason_user = "FAIL_TO_SOCKET_ACCEPT"sv; break;		// socket의 Accept 과정에서실패했다.
			case	DISCONNECT_REASON_FAIL_TO_SOCKET_CONNECT:		str_reason_user = "FAIL_TO_SOCKET_CONNECT"sv; break;	// socket의 connect 단계에서 실패했다.
			case	DISCONNECT_REASON_FAIL_TO_SOCKET_BIND:			str_reason_user = "FAIL_TO_SOCKET_BIND"sv; break;		// socket의 최대 생성 수를 초과해 접속이 종료되었다.
			case	DISCONNECT_REASON_KEEP_ALIVE_TIME_OUT:			str_reason_user = "KEEP_ALIVE_TIME_OUT"sv; break;		// socket의 prepare단계에서 실패했다.
			case	DISCONNECT_REASON_DISABLED:						str_reason_user = "DISABLED"sv; break;					// acceptor나 connector가 disabled된 상태이다.
			case	DISCONNECT_REASON_CONNECTION_FULL:				str_reason_user = "CONNECTION_FULL"sv; break;			// 최대접속수를 초과해 접속종료되어다.
			case	DISCONNECT_REASON_EXCEPTION_OCCURED:			str_reason_user = "EXCEPTION_OCCURED"sv; break;			// 처리중 예외가 발생해 접속이 종료되었다.
			default:												str_reason_user = "OTHERS"sv; break;					// 처리중 예외가 발생해 접속이 종료되었다.
			}
		}

		// trace)
		LOG(plogger_socket_debug, log_type_t::debug) << " socket["sv << _hsocket << "] disconnected reason:"sv << NFMT::format("0x{:x}", _reason) << "("sv << str_reason_system << " "sv << str_reason_user << ")"sv;
	}

	void _debug_log_socket_received(CGSOCKET _hsocket, uint64_t _length, uint64_t _stacked)
	{
		// declare)
		object_ptr<Ilogger> plogger_socket_debug;

		scoped_lock(g_cs_map_message_type_info)
		{
			plogger_socket_debug = g_plogger_socket_debug;
		}

		// check)
		RETURN_IF(plogger_socket_debug.empty());

		// trace)
		LOG(plogger_socket_debug, log_type_t::debug) << " socket["sv << _hsocket << "] received:"sv << _length << "bytes, queued:"sv << _stacked << "bytes"sv;
	}

	void _debug_log_socket_on_message(CGSOCKET _hsocket, buffer_view _buf_message)
	{
		// declare)
		object_ptr<Ilogger> plogger_socket_debug;

		scoped_lock(g_cs_map_message_type_info)
		{
			plogger_socket_debug = g_plogger_socket_debug;
		}

		// check)
		RETURN_IF(plogger_socket_debug.empty());

		// trace)
		if (_buf_message.size() >= 8)
		{
			// - get message type
			auto message_type = _buf_message.front<uint32_t>(4);

			auto iter_find = g_map_message_type_info.find(message_type);

			if (iter_find != g_map_message_type_info.end())
			{
				LOG(plogger_socket_debug, log_type_t::debug) << " socket["sv << _hsocket << "] message received (length:"sv << _buf_message.size() << "bytes, message_head:"sv << iter_find->second << ")";
			}
			else
			{
				LOG(plogger_socket_debug, log_type_t::debug) << " socket["sv << _hsocket << "] message received (length:"sv << _buf_message.size() << "bytes, message_head: unknowned["sv << message_type << "])";
			}
		}
		else
		{
			LOG(plogger_socket_debug, log_type_t::debug) << " socket["sv << _hsocket << "] message received (length:"sv << _buf_message.size() << "bytes)"sv;
		}
	}

	void _debug_log_socket_send_message(net::io::Isocket* _psocket, buffer_view _buf_message)
	{
		// declare)
		object_ptr<Ilogger> plogger_socket_debug;

		scoped_lock(g_cs_map_message_type_info)
		{
			plogger_socket_debug = g_plogger_socket_debug;
		}

		// check)
		RETURN_IF(plogger_socket_debug.empty());

		// declare) 
		std::string str_hsocket;

		// 1) socket handle
		if (_psocket != nullptr)
			str_hsocket = std::to_string((uint64_t)_psocket->native_handle());
		else
			str_hsocket = " - ";

		// trace)
		if (_buf_message.size() >= 8)
			LOG(plogger_socket_debug, log_type_t::debug) << " socket["sv << str_hsocket << "] send message (length:"sv << _buf_message.size() << "bytes, message_head:"sv << _buf_message.front<uint64_t>(4) << ")"sv;
		else
			LOG(plogger_socket_debug, log_type_t::debug) << " socket["sv << str_hsocket << "] send message (length:"sv << _buf_message.size() << "bytes)"sv;
	}
}
}
}