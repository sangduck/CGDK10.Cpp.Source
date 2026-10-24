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

//-----------------------------------------------------------------------------
//
// net::io::connect_requestable::Ntcp (TCP 연결자)
//
// 1. net::io::connect_requestable::Ntcp이란!
//    1) TCP socket을 Client 입장에서 접속을...
//    2) 
//    3) 기본적으로 Inameable을 상속받아 이름을 설정할 수 있다.
//
// 2. 중요 함수.
//    1) 일반적인 사용은 net::io::Iconnect_requestable에 정의된 다양한 형태의 connect(...)
//		 함수를 활용한다.
//    2) 만약 connect를 요청했을 때 처리되어야 하는 것이 있다면 다음의 함수를
//       재정의하여 구현할 수 있다.
//
//    주의) Iexecutable을 상속받아 처리하므로 이 클래스를 상속받은 클래스에서
//          process_execute()를 재정의하면 접속처리가 되지 않을 수 있다.
// 
// 3. initialize/start string
//
//	  1) "remote_endpoint"
//	     - "address_format"	address format ("ipv4", "ipv6", "ipv6_prefer", "ipv4_prefer", "default") (optional:default="ipv4")
//       - "address"			접속할 주소
//	     - "port"				접속할 포트
//
//	  2) "local_endpoint" or "bind_endpoint"
//	     - "address_format"	address format ("ipv4", "ipv6", "ipv6_prefer", "ipv4_prefer", "default") (optional:default="ipv4")
//       - "address"			bind 주소
//	     - "port"				bind 포트
//
//	  3) "block_until_connecting_completed"
//	     - "true"				접속처리가 완료될 때까지 기다린다.(blocking한다.)
//       - "false"				기본값. 접속처리 완료를 기다리지 않고 바로 진행한다.(non-blocking)
//	     - "default"			기본값(false)
//
//	  4) "socket_option" (optional) : the tcp socket option of this socket (sSOCKET_OPTION_TCP. applied when the connect completes. nothing but TCP_NODELAY by default)
//	     - "tcp_nodelay"			TCP_NODELAY (true/false, default true)
//	     - "tcp_quickack"			delayed ACK off (true/false, default false. linux : TCP_QUICKACK after every recv, windows : SIO_TCP_SET_ACK_FREQUENCY = 1)
//	     - "busy_poll"				SO_BUSY_POLL in microseconds (linux. 0/default : not set. CAP_NET_ADMIN to raise it. windows : ignored)
//	     - "tcp_notsent_lowat"		TCP_NOTSENT_LOWAT in bytes ("64k", "1m" ok. linux. 0/default : not set. windows : ignored)
//	     - "tcp_fastopen"			TCP Fast Open of the connect (true/false, default false)
//								windows : TCP_FASTOPEN before ConnectEx (the TFO cookie is requested on the first connect)
//								linux   : TCP_FASTOPEN_CONNECT. connect() returns at once and the SYN goes with the first send().
//										  on_connect() is called before the handshake (remote_endpoint() is not available in it) and
//										  a refused connect is reported as a disconnect. needs sysctl net.ipv4.tcp_fastopen & 1
//	     (START_PARAMETER : socket_option + flag_socket_option = 1. by code : socket_option() before start())
//
//		   context_setting["foo"]["socket_option"]["tcp_quickack"]	 = true;
//		   context_setting["foo"]["socket_option"]["tcp_fastopen"]	 = true;
//
//    예)  객체명이 "foo"이라고 한다면....
//
//		   sMESSAGE msg_context;
//         context_setting["foo"][remote_endpoint]["address_format"] = "ipv6";	// "ipv4", "ipv4_prefer", "ipv6", "ipv6_prefer"
//         context_setting["foo"][remote_endpoint]["address"]		 = "192.168.0.100";
//         context_setting["foo"][remote_endpoint]["port"]			 = 20000;
//         context_setting["foo"][block_until_connecting_completed]			 = true;
//
//
//-----------------------------------------------------------------------------
class CGDK::net::io::connect_requestable::Ntcp :
	public						object::Nnameable,
	public						object::Ninitializable_startable,
	public						net::io::binder::Ntcp,
	virtual public				net::io::Iconnect_requestable,
	virtual public				net::io::Iconnectable
{
public:
			using end_point_type = net::ip::tcp::endpoint;
			using START_PARAMETER = sSOCKET_CONNECT_REQUEST_PARAMETER;

protected:
			Ntcp();
	virtual	~Ntcp() noexcept;

public:
			result_code			start() { return this->object::Nstartable::start();}
			result_code			start(const START_PARAMETER& _parameter) { return this->object::Nstartable::start<START_PARAMETER>(_parameter); }
			result_code			start(const context& _context) { context context_copy(_context); return this->start(context_copy); }
	virtual	result_code			start(context& _context) override { return this->object::Ninitializable_startable::start(_context); }

			result_code			wait_complete_connecting(chrono::tick::duration _duration_wait= chrono::seconds(30));
			result_code			wait_complete_closed(chrono::tick::duration _duration_wait = chrono::seconds(30));

			void				enable_block_until_connecting_completed(bool _flag_enable = true) noexcept { this->m_enable_block_until_connecting_completed = _flag_enable; }
			void				disable_block_until_connecting_completed() noexcept { this->m_enable_block_until_connecting_completed = false; }
	[[nodiscard]] bool			block_until_connecting_completed() const noexcept { return this->m_enable_block_until_connecting_completed; }

			void				enable_block_until_disconnecting_completed(bool _flag_enable = true) noexcept { this->m_enable_block_until_disconnecting_completed = _flag_enable; }
			void				disable_block_until_disconnecting_completed() noexcept { this->m_enable_block_until_disconnecting_completed = false; }
	[[nodiscard]] bool			block_until_disconnecting_completed() const noexcept { return this->m_enable_block_until_disconnecting_completed; }

	//! @brief (linux) connecting to a loopback address tries the UDS of the port first ("\0cgdk.tcp.<port>" : an acceptor with enable_uds), TCP if not. (default false : TCP. turn it on with enable_uds_loopback(true) when the server listens with enable_uds)
	static	void				enable_uds_loopback(bool _enable = true) noexcept { s_enable_uds_loopback.store(_enable, std::memory_order_relaxed); }
	[[nodiscard]] static bool	is_enable_uds_loopback() noexcept { return s_enable_uds_loopback.load(std::memory_order_relaxed); }

protected:
	virtual void				on_request_connect(const net::sockaddr& /*_address*/) {}

public:
	virtual	void				process_reset_connector() noexcept override;
	virtual bool				process_request_connecting(const net::sockaddr& _address) override;
	virtual	void				process_complete_connecting( uintptr_t _result) override;

	virtual	result_code			process_start(context& _context) override;
	virtual	result_code			process_stop() noexcept override;

	virtual	uint16_t			process_get_default_port() noexcept { return 0; }

#if defined(_SYS_EPOLL_H)
protected:
	// - (epoll) process_complete_connect() is processed like an epoll event (m_epoll_event_processing_now, set in process_complete_connecting()) :
	//   a close in on_connect, or a close of the SYN socket by another thread (process_close_socket() : EPOLLHUP), is queued and done
	//   after it. the socket executable is set only while the handle is this socket's
			bool				_process_end_complete_connect(bool _is_connected) noexcept;

	// - io_uring completion : the connect is a CONNECT op (submit_connect) : no readiness registration. its completion calls
	//   process_complete_connecting() with EPOLLOUT / EPOLLERR|EPOLLHUP like the poll, and the sequence of the connect request in the
	//   upper 32 bits (CONNECT_SEQ_SHIFT) : the -ECANCELED of a request closed and restarted meanwhile (the fd number may be the same)
	//   is dropped by the sequence (0 : not checked. the close (EPOLLHUP) and the readiness poll)
public:
	static	constexpr unsigned	CONNECT_SEQ_SHIFT = 32;
	static	constexpr uintptr_t	CONNECT_EVENT_MASK = 0xffffffffu;
			bool				process_submit_connect_op(net::io::executable_connect_requestable* _pexecutable) noexcept;
protected:
			std::atomic<uint32_t> m_seq_connecting{ 0 };
			bool				_process_check_connecting_seq(uintptr_t& _result) const noexcept
			{
				const auto seq = static_cast<uint32_t>(_result >> CONNECT_SEQ_SHIFT);
				_result &= CONNECT_EVENT_MASK;
				return (seq == 0) || (seq == this->m_seq_connecting.load(std::memory_order_acquire));
			}
#endif

protected:
			result_code			m_result_connecting = eRESULT::NOT_DEFINED;
			bool				m_enable_block_until_connecting_completed = false;
			bool				m_enable_block_until_disconnecting_completed = true;
			bool				m_enable_progress_log = true;

			// - (linux) tries the UDS for a loopback address (false when a bind endpoint is given)
			bool				m_try_uds_loopback = true;
	static inline std::atomic<bool> s_enable_uds_loopback{ false };
};

namespace CGDK
{
	inline object_ptr<net::io::Iconnectable> net::io::connective::Iconnector::request_connect(end_point_type _endpoint_remote, end_point_type _endpoint_bind)
	{
		sSOCKET_CONNECT_REQUEST_PARAMETER parameter;
		parameter.remote_endpoint = _endpoint_remote;
		parameter.bind_endpoint = _endpoint_bind;

		context context_connect;
		context_connect = buffer_view((char*)&parameter, sizeof(parameter));
		return this->request_connect(context_connect);
	}
	inline bool net::io::connective::Iconnector::request_connect(Iconnectable* _pconnectable, end_point_type _endpoint_remote, end_point_type _endpoint_bind)
	{
		sSOCKET_CONNECT_REQUEST_PARAMETER parameter;
		parameter.remote_endpoint = _endpoint_remote;
		parameter.bind_endpoint = _endpoint_bind;

		context context_connect;
		context_connect = buffer_view((char*)&parameter, sizeof(parameter));
		return this->process_connecting(_pconnectable, context_connect);
	}

	sSOCKET_CONNECT_REQUEST_PARAMETER get_parameter_connection(const context& _context, bool _enable_progress_log = true);
}