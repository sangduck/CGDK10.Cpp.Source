//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                          network socket classes                           *
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
// net::io::Isocket
//
// 1. net::io::Isocket
//    1) socket Interface로 socket의 Handle을 가지고 있다.
//    2) socket의 상태값(eSOCKET_STATE)을 가지고 있다.
//    3) socket의 생성과 소멸을 위한 함수를 가지고 있다.
//    4) socket을 재사용 시 reset하는 함수를 가지고 있다.
//
// 2. net::io::Isocket의 주요 함수
//    1) ProcessResetSocket()	socket의 생성과정을 정의하는 함수이다.
//    2) process_close_socket(uint64_t _disconnect_reason)	socket의 파괴과정을 정의하는 함수이다.
//    3) ResetSocket()			socket의 재사용을 위해 reset하는 과정을 정의하는 함수이다.
//    4) 기타함수들...
//       - socket의 상태를 확인하는 함수.
//       - socket의 Handle에 관련된 함수.
//
//
//-----------------------------------------------------------------------------
class net::io::Isocket :
	virtual public				Iaddress,
	private						non_copyable
{
public:
			Isocket() {}
	virtual	~Isocket() noexcept {}

public:
	// 1) socket을 준비하고 닫을 때 호출하는 함수.
	virtual	void				process_create_socket(ADDRESS_FAMILY _address_family = AF_INET) PURE;
	virtual	void				process_prepare_socket(ADDRESS_FAMILY _address_family = AF_INET) PURE;
	virtual void				process_dispose_socket() noexcept PURE;
	virtual	bool				process_close_socket(uint64_t _disconnect_reason = DISCONNECT_REASON_NONE) noexcept PURE;

public:
	// 2) prepare socket
			void				prepare_socket(ADDRESS_FAMILY _address_family = AF_INET) { this->process_prepare_socket(_address_family);}
			bool				closesocket(uint64_t _disconnect_reason = DISCONNECT_REASON_NONE) noexcept;

	// 3) native handle
			CGSOCKET			native_handle( CGSOCKET _handle_socket) noexcept{ return this->m_native_handle.exchange(_handle_socket);}
	[[nodiscard]] CGSOCKET		native_handle() const noexcept { return this->m_native_handle;}
	[[nodiscard]] bool			is_valid_native_handle() const noexcept { return this->m_native_handle != INVALID_SOCKET; }
	[[nodiscard]] bool			is_invalid_native_handle() const noexcept { return this->m_native_handle == INVALID_SOCKET;}

	// 4) executor
			void				io_context(executor::Iio_context* _pio_context) noexcept { this->m_pio_context = _pio_context;}
	[[nodiscard]] auto			io_context() const noexcept { return this->m_pio_context;}
			void				reset_io_context() noexcept { this->m_pio_context.reset(); }

	// 5) socket state
			eSOCKET_STATE		socket_state( eSOCKET_STATE _status) noexcept {	return this->m_socket_status.exchange(_status);}
	[[nodiscard]] eSOCKET_STATE	socket_state() const noexcept {	return this->m_socket_status;}
			bool				set_socket_state_if( eSOCKET_STATE _status_comparand, eSOCKET_STATE _status_new) noexcept { return this->m_socket_status.compare_exchange_strong(_status_comparand, _status_new);}
			bool				exchange_socket_state_if( eSOCKET_STATE& _status_comparand, eSOCKET_STATE _status_new) noexcept { return this->m_socket_status.compare_exchange_strong(_status_comparand, _status_new);}

	[[nodiscard]] bool			is_disconnected() const noexcept { return (this->m_socket_status == eSOCKET_STATE::CLOSED);}	// 0 : 접속이 종료된 상태.
	[[nodiscard]] bool			is_binded() const noexcept { return (this->m_socket_status >= eSOCKET_STATE::BINDED);}			// 1 : listen중인가?
	[[nodiscard]] bool			is_connected() const noexcept { return (this->m_socket_status >= eSOCKET_STATE::ESTABLISHED);}	// 2 : 접속만 된 상태.
	[[nodiscard]] bool			is_certified() const noexcept { return (this->m_socket_status >= eSOCKET_STATE::CERTIFIED);}	// 3 : 접속이 되고 Client검증이 끝난 상태.
	[[nodiscard]] bool			is_logined() const noexcept { return (this->m_socket_status >= eSOCKET_STATE::LOGINED);}		// 4 : ID와 Password를 넣고 Log-In이 된 상태.
	[[nodiscard]] bool			is_listening() const noexcept { return (this->m_socket_status == eSOCKET_STATE::LISTEN);}		// 9 : listen중인가?
	[[nodiscard]] bool			is_client_socket() const noexcept { return this->m_is_client_socket; }

	// 6) socket reason (disconnect reason)
			void				disconnect_reason( uint64_t _disconnect_reason) noexcept { this->m_disconnect_reason = _disconnect_reason;}
	[[nodiscard]] uint64_t		disconnect_reason() const noexcept { return this->m_disconnect_reason;}

	// 7) socket address
			void				address_family(ADDRESS_FAMILY _af) noexcept { this->m_address_family = _af;}
			auto				address_family() const noexcept { return this->m_address_family;}
	[[nodiscard]] net::sockaddr	socket_address_in_public(std::error_code& _error_code) const noexcept;
	[[nodiscard]] net::sockaddr	socket_address_in_public() const noexcept { std::error_code error_code; return socket_address_in_public(error_code);}

	// 9) options
			void				maximum_send_buffer_size(std::size_t _size) noexcept;
	[[nodiscard]] std::size_t	maximum_send_buffer_size() const noexcept { return this->m_maximum_send_buffer_size;}

			void				maximum_receive_buffer_size(std::size_t _size) noexcept;
	[[nodiscard]] std::size_t	maximum_receive_buffer_size() const noexcept { return this->m_maximum_receive_buffer_size;}

			void				exception_on_send_overflow(bool _enable) noexcept { this->m_is_exception_on_send_overflow = _enable; }
	[[nodiscard]] bool			exception_on_send_overflow() const noexcept { return this->m_is_exception_on_send_overflow;}

	// - 송신 queue가 넘쳤을 때 처리한다. (송신 lock 안에서 호출된다.)
	//   . exception_on_send_overflow(true)면 exception을 던진다.
	//   . 아니면(기본) 접속을 끊는다. (메시지를 버리면 상대는 다음 메시지부터 잘못 해석하게 된다. 느린 상대는 끊는다.)
			void				process_on_send_overflow();

	// 10) tcp socket option (sSOCKET_OPTION_TCP : TCP_NODELAY, TCP_QUICKACK, SO_BUSY_POLL, TCP_NOTSENT_LOWAT, TCP Fast Open of a connect)
	//     - applied to the socket handle when the connect/accept completes (apply_socket_option() in connectable::Ntcp::process_complete_connect).
	//       set it before : the constructor of the socket class, acceptor::on_accept() (the acceptor's one is set right before it), connector::on_alloc_connectable(), a client before start()
	//     - the acceptor ("socket_option" : the accepted sockets), the connector ("socket_option" : the allocated sockets) and the
	//       connect_requestable ("socket_option" : itself) set it from their parameters. nothing but TCP_NODELAY is applied by default
			void				socket_option(const sSOCKET_OPTION_TCP& _option) noexcept { this->m_socket_option = _option; }
	[[nodiscard]] sSOCKET_OPTION_TCP& socket_option() noexcept { return this->m_socket_option; }
	[[nodiscard]] const sSOCKET_OPTION_TCP& socket_option() const noexcept { return this->m_socket_option; }
			bool				apply_socket_option() noexcept;

	[[nodiscard]] int			get_last_socket_error() const noexcept { return m_last_socket_error; }

// implementation)
protected:
	virtual	net::sockaddr		process_get_socket_address(std::error_code& _error_code) const noexcept override;
	virtual	net::sockaddr		process_get_peer_address(std::error_code& _error_code) const noexcept override;

			bool				m_is_client_socket = false;

			std::atomic<eSOCKET_STATE> m_socket_status{ eSOCKET_STATE::CLOSED };
			object_ptr<executor::Iio_context> m_pio_context;

public:		int					m_last_socket_error{ 0 };
protected:
	// - disconnect reason : 수신 thread(오류), 송신 thread, close 호출 thread가 동시에 |= 한다.
	//   - 상위(DISCONNECT_REASON_MASK 등)는 flag bit : OR로 모은다.
	//   - 하위(DISCONNECT_REASON_MASK_USER)는 bit가 아니라 번호(code)다 : 처음 들어온 code만 남긴다.
	//     (OR로 섞으면 다른 code가 된다. ex. OVERFLOW_MAX_BYTES(0x02) | SUSTAIN_TIMEOUT(0x15) = 0x17)
	//   - CAS (relaxed : 값만 모으고 다른 메모리와의 순서는 필요 없다)
	struct sDISCONNECT_REASON
	{
		std::atomic<uint64_t>	value{ 0 };

		sDISCONNECT_REASON& operator = (uint64_t _value) noexcept { this->value.store(_value, std::memory_order_relaxed); return *this; }
		sDISCONNECT_REASON& operator |= (uint64_t _value) noexcept
		{
			uint64_t value_old = this->value.load(std::memory_order_relaxed);
			for (;;)
			{
				const uint64_t code = ((value_old & DISCONNECT_REASON_MASK_USER) != 0) ? (value_old & DISCONNECT_REASON_MASK_USER) : (_value & DISCONNECT_REASON_MASK_USER);
				const uint64_t value_new = ((value_old | _value) & ~DISCONNECT_REASON_MASK_USER) | code;
				if (value_new == value_old || this->value.compare_exchange_weak(value_old, value_new, std::memory_order_relaxed))
					break;
			}
			return *this;
		}
		[[nodiscard]] operator uint64_t() const noexcept { return this->value.load(std::memory_order_relaxed); }
	};
			sDISCONNECT_REASON	m_disconnect_reason;

	// - 수신 lock (windows iocp/rio, linux io_uring completion) : 수신은 이 lock 안에서 handle을 읽고 recv까지 한다. close는 같은 lock 안에서 handle을 INVALID_SOCKET으로 바꾼다.
	//   (닫은 뒤 같은 handle 값으로 다시 만들어진 다른 socket에서 받지 않도록)
			std::mutex			m_lockable_receiving;
public:
			std::mutex&			get_lockable_receiving() noexcept { return this->m_lockable_receiving; }
protected:
#if defined(_WINSOCK2API_)
	// - 수신 lock 안에서 handle을 읽어 _function(handle)을 부른다. (handle이 INVALID_SOCKET이면 WSAENOTSOCK으로 false)
			template <class F>
			bool				_call_receiving(F&& _function)
			{
				std::lock_guard lock(this->m_lockable_receiving);
				const auto handle_socket = this->native_handle();
				if (handle_socket == INVALID_SOCKET)
				{
					::WSASetLastError(WSAENOTSOCK);
					return false;
				}
				return _function(handle_socket);
			}
#endif
			ADDRESS_FAMILY		m_address_family{ AF_UNSPEC };

			std::size_t			m_maximum_send_buffer_size{ 0 };
			std::size_t			m_maximum_receive_buffer_size{ 0 };

			bool				m_is_exception_on_send_overflow {false};
			sSOCKET_OPTION_TCP	m_socket_option;
			std::atomic<bool>	m_is_send_overflow_closing{ false };

			std::atomic<CGSOCKET> m_native_handle{ INVALID_SOCKET };

#if defined(_SYS_EPOLL_H)
public:
			class executable_socket : virtual public Iexecutable, public Npoolable<executable_socket>, public factory::_traits_system
			{
			public:
				virtual ~executable_socket() noexcept {}
			public:
				virtual	intptr_t process_execute( intptr_t _result, std::size_t _param) override { auto psocket = this->m_psocket; RETURN_IF(psocket == nullptr, 0); return psocket->process_execute_epoll(_result, _param); }
				virtual	void on_final_release() noexcept override { this->reset_socket(); }
				void set_socket(Isocket* _psocket) noexcept { if (this->m_psocket == _psocket) return; if (this->m_psocket != nullptr) this->m_psocket->release(); this->m_psocket = _psocket; _psocket->add_ref();}
				void reset_socket() noexcept { auto psocket = this->m_psocket; if (psocket != nullptr) { this->m_psocket = nullptr; psocket->release(); } }
				Isocket* get_socket() const noexcept { return this->m_psocket; }
			private:
				Isocket* m_psocket = nullptr;
			};

			object_ptr<executable_socket> m_pexecutable_socket;
	mutable	lock_object			m_epoll_event_cs;
			std::atomic<CGHANDLE>			m_epoll_handle{ INVALID_SOCKET };
			uint32_t			m_epoll_event_flag = 0;
			uint32_t			m_epoll_event_flag_queued = 0;
			bool				m_epoll_event_processing_now = false;

			[[nodiscard]] CGHANDLE get_epoll_handle() const noexcept { return this->m_epoll_handle.load(std::memory_order_acquire); }
			void				set_epoll_handle(CGHANDLE _handle_epoll) noexcept { this->m_epoll_handle.store(_handle_epoll, std::memory_order_release); }
			CGHANDLE			reset_epoll_handle() noexcept { return this->m_epoll_handle.exchange(INVALID_SOCKET, std::memory_order_acq_rel); }
			CGHANDLE			detach_epoll_handle() noexcept;
			[[nodiscard]] uint32_t get_epoll_event_flag() noexcept { return this->m_epoll_event_flag; }
			void				set_epoll_event_flag(uint32_t _event) noexcept { this->m_epoll_event_flag = _event; }
			void				add_epoll_event_flag(uint32_t _event) noexcept { this->m_epoll_event_flag |= _event; }
			void				remove_epoll_event_flag(uint32_t _event) noexcept { this->m_epoll_event_flag &= (~_event); }
			void				reset_epoll_event_flag() noexcept;
			void				post_epoll_event_EPOLLOUT() noexcept;
			void				process_prepare_executable_socket() { this->m_pexecutable_socket = make_object<executable_socket>(); }
			void				process_reset_executable_socket() noexcept { this->m_pexecutable_socket.reset(); }
	virtual	intptr_t			process_execute_epoll(intptr_t _event, std::size_t _param);

	// - io_uring completion (executor::io_backend::io_uring_completion) : the socket does its I/O with the ops of the io_context
	//   (submit_recv/submit_sendmsg ... : one RECV in flight, the completions are delivered to the op executables) instead of the
	//   readiness registration (attach/set_event/process_execute_epoll). decided once per connection (process_decide_io_completion :
	//   before on_connect(), so a close in on_connect or right after sees the mode) : the io_context is a completion one and the
	//   receivable/sendable of the socket support it (is_io_completion_supported()). otherwise readiness.
	//   - m_is_io_completion : set in process_decide_io_completion() (connectable::Ntcp::process_complete_connect), cleared when the socket is prepared again (process_prepare_socket)
	//   - m_count_io_inflight : the ops of this socket posted and not completed yet (the executables hold the socket meanwhile : a pooled socket is not reused)
	//   - m_is_receiving_op : a RECV op is in flight (under m_lockable_receiving) : its completion completes the disconnect of a closed socket.
	//     none in flight => process_close_socket() completes it itself (like the epoll branch when no event is being processed)
	//   - process_start_io() : called once after on_connect() (the acceptor, the connector, connect_requestable) with the io_context of the socket.
	//     _is_attached : the handle is registered already (the connect poll of the readiness connect : set_event. completion : never)
	[[nodiscard]] bool			is_io_completion() const noexcept { return this->m_is_io_completion; }
			bool				process_start_io(executor::Iio_context* _pio_context, bool _is_attached) noexcept;
			void				process_decide_io_completion() noexcept;
	[[nodiscard]] int			io_inflight_count() const noexcept { return this->m_count_io_inflight.load(std::memory_order_acquire); }
protected:
	virtual	bool				process_is_io_completion_supported() const noexcept { return false; }
	virtual	bool				process_start_io_completion() noexcept { return false; }
			bool				m_is_io_completion = false;
			bool				m_is_receiving_op = false;
			std::atomic<int>	m_count_io_inflight{ 0 };
public:
#endif
};

namespace net
{
namespace io
{
	void set_default_socket_debug_logger(Ilogger* _logger) noexcept;
	object_ptr<Ilogger> reset_default_socket_debug_logger() noexcept;
	[[nodiscard]] object_ptr<Ilogger> get_default_socket_logger() noexcept;
	void enable_default_socket_debug_logging(bool _enable = true) noexcept;
	void disable_default_socket_debug_logging() noexcept;
	[[nodiscard]] bool is_default_socket_debug_logging_enabled() noexcept;

	void _debug_log_socket_connected(CGSOCKET _hsocket, net::sockaddr _sockaddr);
	void _debug_log_socket_disconnected(CGSOCKET _hsocket, uint64_t _reason);
	void _debug_log_socket_received(CGSOCKET _hsocket, uint64_t _length, uint64_t _stacked);
	void _debug_log_socket_on_message(CGSOCKET _hsocket, buffer_view _buf_message);
	void _debug_log_socket_send_message(net::io::Isocket* _psocket, buffer_view _buf_message);
}
}

}
