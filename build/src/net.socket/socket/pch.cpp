#include "pch.h"
#include "net.io.connectable.disconnect_manager.h"
#include "net.io.add_on.manager_socket_sustain.h"
#include "net.io.connect_requestable.tcp_reconnect.manager_reconnector.h"
#include <iostream>


//-----------------------------------------------------------------
// 1. Init_seg(lib)
//-----------------------------------------------------------------
// Static) 
#if defined(_MSC_VER)
	#pragma warning(disable:4073)
	#pragma init_seg(lib)
#endif

namespace CGDK
{
	// 1) statistics
	std::size_t								g_max_accept_socket = std::numeric_limits<std::size_t>::max();
	statistics::unit_connective				__ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL2 + 0)	net::io::statistics::Nconnective::m_statistics_connection_total;
	statistics::unit_traffic				__ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL2 + 0)	net::io::statistics::Nsocket::m_statistics_traffic_total;

	// 2) socket
	std::atomic<size_t>						api::socket::m_socket_count = SIZE_MAX;
	size_t									api::socket::m_socket_count_max = SIZE_MAX;
	volatile long							api::socket::m_naddress_local;
	std::array<net::sockaddr, 64>			api::socket::m_addr_local;
#if defined(_WINSOCK2API_)
	RIO_EXTENSION_FUNCTION_TABLE			api::socket::m_rio_functions;
	int										(PASCAL FAR* api::socket::WSAcceptEx)(IN CGSOCKET _sListenSocket, IN CGSOCKET _sAcceptSocket, IN PVOID _lpOutputBuffer, IN uint32_t _dwReceivedDataLength, IN uint32_t _dwLocalAddressLength, IN uint32_t _dwRemoteAddressLength, OUT uint32_t* _lpdwBytesReceived, IN LPOVERLAPPED _lpOverlapped);
	int										(PASCAL FAR* api::socket::WSConnectEx)(IN CGSOCKET _s, IN const struct sockaddr FAR* _name, IN int _namelen, IN void* _lpSendBuffer OPTIONAL, IN uint32_t _dwSentDataLength, OUT uint32_t* _lpdwBytesSent, IN LPOVERLAPPED _lpOverlapped);
	int										(PASCAL FAR* api::socket::WSDisconnectEx)(IN CGSOCKET _s, IN LPOVERLAPPED _lpOverlapped, IN uint32_t _dwFlags, IN uint32_t _dwReserved);
#endif

	// 3) connective manager
	std::mutex								__ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL2 + 2)	net::io::connective::manager::m_lockable_create;
	own_ptr<net::io::connective::manager>	__ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL2 + 3)	net::io::connective::manager::m_pinstance;

	// 4) binder manager
	std::mutex								__ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL2 + 4)	net::io::binder::manager::m_lockable_create;
	own_ptr<net::io::binder::manager>		__ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL2 + 5)	net::io::binder::manager::m_pinstance;

	// 5) socket i/o logger
	namespace net
	{
		namespace io
		{
			bool							g_enable_plogger_socket_debug = false;
			object_ptr<Ilogger>				__ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL1 + 6)	g_plogger_socket_debug;
		}
	}

	// 6) socket_sustain manager
	std::mutex								__ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL2 + 8)	net::io::add_on::manager_socket_sustain::m_lockable_create;
	own_ptr<net::io::add_on::manager_socket_sustain> __ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL2 + 9)	net::io::add_on::manager_socket_sustain::m_pinstance;

	// 7) disconnect manager
#if defined(_WINSOCK2API_)
	std::mutex								__ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL2 + 10)	net::io::connectable::manager_disconnect::m_lockable_create;
	own_ptr<net::io::connectable::manager_disconnect> __ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL2 + 11)	net::io::connectable::manager_disconnect::m_pinstance;
#endif

	// 8) restore manager
	std::mutex								__ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL2 + 12)	net::io::restore_manager::m_lockable_create;
	own_ptr<net::io::restore_manager>		__ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL2 + 13)	net::io::restore_manager::m_pinstance;

	// 9) reconnector manager
	std::mutex								__ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL2 + 14)	net::io::connect_requestable::Ntcp_reconnect::manager_reconnector::m_lockable_create;
	own_ptr<net::io::connect_requestable::Ntcp_reconnect::manager_reconnector>	__ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL2 + 15)	net::io::connect_requestable::Ntcp_reconnect::manager_reconnector::m_pinstance;
}

#if defined(_MSC_VER)
	#pragma warning(default:4073)
#endif

//-----------------------------------------------------------------
// 2. CGDK Info
//-----------------------------------------------------------------
#define	_FREE_RELEASE_MAX_ACCEPT_SOCKET		128
#define	_INDIE_RELEASE_MAX_ACCEPT_SOCKET	1024

namespace CGDK
{
	// * CGDK Component Info
	CGDK_INFO_CLASS	g_info_net_socket;
}