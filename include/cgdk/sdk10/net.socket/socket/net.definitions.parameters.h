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
// ----------------------------------------------------------------------------
//  CONNECTION_PAARAMETER
// ----------------------------------------------------------------------------
struct sSOCKET_ENDPOINT
{
	std::string				str_address;
	std::string				str_service{ "0" };
	int						flag{ 0 };
};
sSOCKET_ENDPOINT get_parameter_endpoint(const context& _context, std::string_view _name, bool _is_enable_log = false);
net::ip::tcp::endpoint get_sockaddr(const sSOCKET_ENDPOINT& _socket_endpoint);

// ----------------------------------------------------------------------------
//  sSOCKET_OPTION_TCP (the tcp socket option of a socket)
//
//   - every socket holds one (Isocket::socket_option()). it is applied to the socket handle when the connect/accept completes
//     (Isocket::apply_socket_option() in connectable::Ntcp::process_complete_connect). a UDS socket : skipped.
//   - nothing but TCP_NODELAY (as before) is applied by default : every other option is off unless set.
//   - set from the parameters ("socket_option") of the acceptor (the accepted sockets), the connector (the allocated sockets)
//     and the connect_requestable (itself), or by code before the connect/accept (the constructor of the socket class,
//     acceptor::on_accept() (the acceptor's one is set right before it), connector::on_alloc_connectable(), socket_option() of a client before start())
//
//   tcp_nodelay       : TCP_NODELAY. Nagle off (default true : as before)
//   tcp_quickack      : delayed ACK off (default false)
//                       linux   : TCP_QUICKACK. not permanent : the receivable sets it again after every recv
//                       windows : SIO_TCP_SET_ACK_FREQUENCY = 1 (an ACK for every segment. persistent)
//   busy_poll         : linux : SO_BUSY_POLL in microseconds (default 0 : not set)
//                       a non-blocking recv (epoll) polls the NIC queue once instead of returning EAGAIN at once : the packets
//                       arrived after the event are received without another wake up. (a blocking recv spins up to this time.
//                       epoll_wait itself : executor::set_default_busy_poll() / io_context_threads "busy_poll" (EPIOCSPARAMS of the kernel 6.9+),
//                       or sysctl net.core.busy_poll before 6.9)
//                       to raise it over the current value CAP_NET_ADMIN is required. (sysctl net.core.busy_read : the default)
//                       windows : ignored (warned)
//   tcp_notsent_lowat : linux : TCP_NOTSENT_LOWAT in bytes (default 0 : not set)
//                       the limit of the not yet sent bytes in the socket send buffer. send() returns EAGAIN and EPOLLOUT is
//                       signaled by this limit (not by SO_SNDBUF) : the data stays in the send queue of the socket class
//                       (priority, memory) instead of the kernel. windows : ignored (warned)
//   tcp_fastopen      : TCP Fast Open (RFC 7413) of a connecting socket (connect_requestable / connector. default false)
//                       windows : TCP_FASTOPEN before ConnectEx. the TFO cookie of the server is requested on the first connect.
//                                 (the data in the SYN needs a send buffer in ConnectEx : not used by the connect. the cookie
//                                  is cached by the stack for the next connects)
//                       linux   : TCP_FASTOPEN_CONNECT. connect() returns at once (no SYN yet) and the first send() sends the SYN
//                                 (with the data when a cookie is cached. without a cookie the send() returns EINPROGRESS and
//                                 the sendable waits for EPOLLOUT like EWOULDBLOCK). on_connect() is called before the handshake :
//                                 remote_endpoint() is not available in it, and a refused connect is reported as a disconnect
//                                 (not on_fail_connect). requires sysctl net.ipv4.tcp_fastopen & 1 (the default)
//                       (the listen socket : sSOCKET_ACCEPT_PARAMETER::tcp_fastopen)
// ----------------------------------------------------------------------------
struct sSOCKET_OPTION_TCP
{
	bool					tcp_nodelay{ DEFAULT_ENABLE_TCP_NODELAY };
	bool					tcp_quickack{ false };
	bool					tcp_fastopen{ false };
	uint32_t				busy_poll{ 0 };
	uint32_t				tcp_notsent_lowat{ 0 };
};
//! @brief reads "socket_option" of _context into _option. the keys not given keep their value. @return true if "socket_option" exists
//!        "socket_option": { "tcp_nodelay": true, "tcp_quickack": true, "busy_poll": 50, "tcp_notsent_lowat": "64k", "tcp_fastopen": true }
bool get_parameter_socket_option(const context& _context, sSOCKET_OPTION_TCP& _option, bool _is_enable_log = false);

struct sSOCKET_CONNECT_REQUEST_PARAMETER
{
public:
	net::ip::tcp::endpoint	remote_endpoint;
	net::ip::tcp::endpoint	bind_endpoint;

	chrono::tick::duration	reconnection_interval{ chrono::tick::seconds{5} };
	int						reconnection_enable{ 0 };
	int						flag_wait_complete_connecting{ 0 };
	int						flag_wait_complete_disconnecting{ 0 };
	sSOCKET_OPTION_TCP		socket_option{};			// - the tcp socket option of the socket (applied when flag_socket_option != 0. context : "socket_option")
	int						flag_socket_option{ 0 };	// - 0 : the socket keeps its own socket_option(), 1 : socket_option is set to the socket
	bool					enable_progress_log{ true };

public:
	sSOCKET_CONNECT_REQUEST_PARAMETER() {} 
	sSOCKET_CONNECT_REQUEST_PARAMETER(net::ip::tcp::endpoint _remote_endpoint) : remote_endpoint{ _remote_endpoint } {}
	sSOCKET_CONNECT_REQUEST_PARAMETER(net::ip::tcp::endpoint _remote_endpoint, net::ip::tcp::endpoint _bind_endpoint) : remote_endpoint{ _remote_endpoint }, bind_endpoint{ _bind_endpoint } {}
	sSOCKET_CONNECT_REQUEST_PARAMETER(net::ip::tcp::endpoint _remote_endpoint, net::ip::tcp::endpoint _bind_endpoint, chrono::tick::duration _reconnection_interval, int _reconnection_enable, int _flag_wait_complete_connecting = 0, int _flag_wait_complete_disconnecting = 0, bool _enable_progress_log = true) : remote_endpoint{ _remote_endpoint }, bind_endpoint{ _bind_endpoint }, reconnection_interval{ _reconnection_interval }, reconnection_enable{ _reconnection_enable }, flag_wait_complete_connecting{ _flag_wait_complete_connecting }, flag_wait_complete_disconnecting{ _flag_wait_complete_disconnecting }, enable_progress_log{ _enable_progress_log } {}
	sSOCKET_CONNECT_REQUEST_PARAMETER(net::ip::tcp::endpoint _remote_endpoint, chrono::tick::duration _reconnection_interval, int _reconnection_enable, int _flag_wait_complete_connecting = 0, int _flag_wait_complete_disconnecting = 0, bool _enable_progress_log = true) : remote_endpoint{ _remote_endpoint }, reconnection_interval{ _reconnection_interval }, reconnection_enable{ _reconnection_enable }, flag_wait_complete_connecting{ _flag_wait_complete_connecting }, flag_wait_complete_disconnecting{ _flag_wait_complete_disconnecting }, enable_progress_log{ _enable_progress_log } {}
};
sSOCKET_CONNECT_REQUEST_PARAMETER get_parameter_connection(const context& _context, bool _is_enable_log);

struct sSOCKET_ACCEPT_PARAMETER
{
public:
	net::ip::tcp::endpoint	local_endpoint;
	net::ip::tcp::endpoint	accept_endpoint;
	std::size_t				prepare_on_start{ 0 };
	std::size_t				must_prepare{ 0 };
	uint64_t				flag_option{ 0 };
	bool					reuse_address{ true };
	bool					reallocate_port_at_duplicate_port_error{ true };
	bool					enable_progress_log{ true };
	eCRYPTION_TYPE			cryption_type{ eCRYPTION_TYPE::NONE };
	bool					enable_uds{ false };		// - linux : accepts the same host connections through a UDS (Unix domain socket) too. (connect_requestable to a loopback address uses it)
	bool					uds_only{ false };			// - linux : accepts through the UDS only. (the TCP port is bound but not listened : TCP connects are refused. ignored on windows)
	int						tcp_fastopen{ 0 };			// - TCP Fast Open (RFC 7413) of the listen socket : the data in the SYN of a client with a TFO cookie is accepted (the first recv gets it). 0 : off (context : "tcp_fastopen" true/false/<queue length>)
														//   linux : the max pending TFO connections (true : DEFAULT_TCP_FASTOPEN_QUEUE_LENGTH). sysctl net.ipv4.tcp_fastopen & 2 is required. windows : enable
	sSOCKET_OPTION_TCP		socket_option{};			// - the tcp socket option of the accepted sockets (applied when flag_socket_option != 0. context : "socket_option")
	int						flag_socket_option{ 0 };	// - 0 : the accepted sockets keep their own socket_option(), 1 : socket_option is set to every accepted socket right before on_accept()
	int						tcp_defer_accept{ 0 };		// - linux : TCP_DEFER_ACCEPT seconds. the accept wakes up when the first data of the connection arrives (0 : off. a server that sends first keeps it off)
	bool					enable_cpu_affinity{ false };	// - linux : an accepted socket runs on the io_context pinned to the CPU that received its packets (SO_INCOMING_CPU : one io_context and thread per CPU)

public:
	sSOCKET_ACCEPT_PARAMETER() {}
	sSOCKET_ACCEPT_PARAMETER(uint16_t _port) : local_endpoint{ net::ip::tcp::v6(), _port } {}
	sSOCKET_ACCEPT_PARAMETER(net::ip::tcp::endpoint _local_endpoint) : local_endpoint{ _local_endpoint } {}
	sSOCKET_ACCEPT_PARAMETER(net::ip::tcp::endpoint _local_endpoint, net::ip::tcp::endpoint _accept_endpoint) : local_endpoint{ _local_endpoint }, accept_endpoint{ _accept_endpoint } {}
	sSOCKET_ACCEPT_PARAMETER(net::ip::tcp::endpoint _local_endpoint, net::ip::tcp::endpoint _accept_endpoint, std::size_t _prepare_on_start, std::size_t _must_prepare = 0, uint64_t _flag_option = 0, bool _reuse_address = false, bool _reallocate_port_at_duplicate_port_error = true, bool _enable_progress_log = true) : local_endpoint{ _local_endpoint }, accept_endpoint{ _accept_endpoint }, prepare_on_start{ _prepare_on_start }, must_prepare{ _must_prepare }, flag_option{ _flag_option }, reuse_address{ _reuse_address }, reallocate_port_at_duplicate_port_error(_reallocate_port_at_duplicate_port_error), enable_progress_log{_enable_progress_log} {}
	sSOCKET_ACCEPT_PARAMETER(net::ip::tcp::endpoint _local_endpoint, std::size_t _prepare_on_start, std::size_t _must_prepare = 0, uint64_t _flag_option = 0, bool _reuse_address = false, bool _reallocate_port_at_duplicate_port_error = true, bool _enable_progress_log = true) : local_endpoint{ _local_endpoint }, prepare_on_start{ _prepare_on_start }, must_prepare{ _must_prepare }, flag_option{ _flag_option }, reuse_address{ _reuse_address }, reallocate_port_at_duplicate_port_error(_reallocate_port_at_duplicate_port_error), enable_progress_log{ _enable_progress_log } {}
	sSOCKET_ACCEPT_PARAMETER(uint16_t _port, std::size_t _prepare_on_start, std::size_t _must_prepare = 0, uint64_t _flag_option = 0, bool _reuse_address = false, bool _reallocate_port_at_duplicate_port_error = true, bool _enable_progress_log = true) : local_endpoint{ net::ip::tcp::v6(), _port }, prepare_on_start{ _prepare_on_start }, must_prepare{ _must_prepare }, flag_option{ _flag_option }, reuse_address{ _reuse_address }, reallocate_port_at_duplicate_port_error(_reallocate_port_at_duplicate_port_error), enable_progress_log{ _enable_progress_log } {}
};




}