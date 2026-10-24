#include "pch.h"
#include <charconv>
#include <cstring>

#if defined(_WIN32)
	#include <Iphlpapi.h>
#elif defined(__linux__)
	#include <cstdio>
	#include <linux/if.h>
	#include <unistd.h>
#endif

using namespace CGDK;

std::string api::get_host_name()
{
	// declare) 
	char str_value[256] {0};

#if defined(_WIN32)
	// declare) 
	DWORD nSize = 256;

	// 1) Computer 이름을 얻어낸다.
	GetComputerNameA(str_value, &nSize);
#else
	::gethostname(str_value, sizeof(str_value)-1);
#endif

	// return) 
	return std::string(str_value);
}
	
std::string api::get_host_id()
{
#if defined(_WIN32)
	// declare)
	HW_PROFILE_INFOA HW_profile_info;

	// 1) Profile을 얻는다.
	auto bResult = GetCurrentHwProfileA(&HW_profile_info);

	// check)
	RETURN_IF(bResult==FALSE, std::string());

	// return) ...
	return std::string((char*)HW_profile_info.szHwProfileGuid);
#else
	return std::string();
#endif
}

std::vector<sMAC_ADDRESS> api::get_mac_address()
{
	// declare)
	std::vector<sMAC_ADDRESS> vector_MAC;

#if defined(_WIN32)
	// declare) 
	auto buffer_adapter_info = alloc_shared_buffer(sizeof(IP_ADAPTER_INFO) *64);
	auto padapter_info = buffer_adapter_info.data<IP_ADAPTER_INFO>();
	DWORD buffer_len = sizeof(IP_ADAPTER_INFO) * 64;

	// 1) Make an initial call to GetAdaptersInfo to get the necessary size into the buffer_len     variable
	auto result = GetAdaptersInfo(padapter_info, &buffer_len);

	// check) 실패했으면 끝...
	RETURN_IF(result != ERROR_SUCCESS, vector_MAC);

	// 2) Adaptor를 얻는다.
	result = GetAdaptersInfo(padapter_info, &buffer_len);

	// check) 
	RETURN_IF(result != NO_ERROR, vector_MAC);

	do
	{
		// declare) 
		sMAC_ADDRESS temp;

		// check) 
		CGASSERT_ERROR(temp.sll_addr.size() == MAX_ADAPTER_ADDRESS_LENGTH);

		// - MAC Address를 복사한다.
		memcpy((void*)temp.sll_addr.data(), (void*)padapter_info->Address, sizeof(uint8_t)*temp.sll_addr.size());

		// - MAC Address를 추가한다.
		vector_MAC.push_back(temp);

		// - 다음...
		padapter_info = padapter_info->Next;
	}while(padapter_info);
#else
	// 1) create socket temporary
    int sock = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

	// check) 
	RETURN_IF(sock == -1, vector_MAC);

	char buf[1024];

	struct ifconf ifc;
	ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;

	// 2) get SOCKGIFCONF
	auto result = ioctl(sock, SIOCGIFCONF, &ifc);

	// check) 
	if (result == -1)
	{
		::close(sock);
		return vector_MAC;
	}

    auto* iter = ifc.ifc_req;
    auto* iter_end = iter + (ifc.ifc_len / sizeof(struct ifreq));

    for (; iter != iter_end; ++iter)
	{
		// declare) 
		struct ifreq ifr;

		// - copy ifr_name
		std::snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", iter->ifr_name);

		// - get... SIOCGIFFLAGS
		result = ioctl(sock, SIOCGIFFLAGS, &ifr);

		// check) 
		CONTINUE_IF(result != 0);

		// check) skip localhost
		CONTINUE_IF(ifr.ifr_flags & IFF_LOOPBACK);

		// - get SIOCGIFHWADDR
		result = ioctl(sock, SIOCGIFHWADDR, &ifr);

		// check) 
		CONTINUE_IF(result != 0);

		// declare) 
		sMAC_ADDRESS temp;

		// check) 
		CGASSERT_ERROR(temp.sll_addr.size() >= 6);

		// - MAC Address를 복사한다.
		memcpy((void*)temp.sll_addr.data(), (void*)ifr.ifr_hwaddr.sa_data, 6);
		temp.sll_addr[6] = 0;
		temp.sll_addr[7] = 0;

		// - MAC Address를 추가한다.
		vector_MAC.push_back(temp);
	}

	::close(sock);
#endif

	// return)
	return vector_MAC;
}

api::socket::socket()
{
	init_socket();
}

api::socket::~socket() noexcept
{
	cleanup_socket();
}

bool api::socket::init_socket()
{
	//-----------------------------------------------------------------
	// 1. WSA StartUp
	//-----------------------------------------------------------------
#if defined(_WINSOCK2API_)
	// declare) 
	WSADATA temp_wsa_data;

	//  1) WSA start Up
	if (WSAStartup(MAKEWORD(2, 2), &temp_wsa_data) != 0)
	{
		// trace) 
		LOG_ERROR << log_level_t::important() << "(err ) CGDK.net.socket: fail to WSAStartup![Winsock 2.2]"sv;

		// m_socket_count를 SIZE_MAX로 설정한다.(초기화 되지 않을 것으로 처리한다.)
		m_socket_count = SIZE_MAX;

		// return) 실패!!
		return false;
	}

	// log) 초기화 시작
	LOG_SYSTEM << log_level_t::important() << " [api.net] start initializing "sv << temp_wsa_data.szDescription << " (system status is "sv << temp_wsa_data.szSystemStatus << ")"sv;
#elif defined(_SYS_EPOLL_H)
	// log) 초기화 시작
	LOG_SYSTEM << log_level_t::important() << " [api.net] start initializing linux BSD socket with epoll"sv;
#else
	#error	"[CGDK10] Not supported Platform"
#endif
	auto license_component = get_component_info(CGDK_COMPONENT_NAME);

	// trace) 성공!! 
	LOG_SYSTEM << "  - CGDK10.version: "sv << _CGDK_VERSION_PACKAGE << "."sv << _CGDK_VERSION_SUB << "."sv << _CGDK_VERSION_BUILD << "."sv << SVN_COMMIT_REVISION;
	LOG_SYSTEM << "  - CGDK10.build: "sv << __DATE__;

	//-----------------------------------------------------------------
	// 2. System Info
	//-----------------------------------------------------------------
	// log) 초기화 시작
	LOG_SYSTEM << log_level_t::important() << " [api.net] system info"sv;

	// 1) reserve max open files as possible as it can
	api::reserve_max_open_files();

	// 2) get cpu_id and max_open_files
	auto cpu_id = api::get_device_unique_id();
	auto max_open_files = api::get_max_open_files();

	// 3) max socket count
	m_socket_count = 0;
#if defined(_WINSOCK2API_)
	m_socket_count_max = (max_open_files != 0) ? max_open_files : SIZE_MAX;
#elif defined(_SYS_EPOLL_H)
	m_socket_count_max = max_open_files - 64;
#endif

	// 4) ...
	LOG_SYSTEM << "  - cpu id: "sv
		<< CGDK::stream_hex(cpu_id[0], 8, true) << ":"
		<< CGDK::stream_hex(cpu_id[1], 8, true) << ":"
		<< CGDK::stream_hex(cpu_id[2], 8, true) << ":"
		<< CGDK::stream_hex(cpu_id[3], 8, true);
	LOG_SYSTEM << "  - os id: x"sv << get_os_unique_id();
	LOG_SYSTEM << "  - core: x"sv << get_core_count();
	LOG_SYSTEM << "  - system memory: "sv << to_string_scaled(api::get_physical_memory_max()) << "bytes"sv;
#if defined(_WINSOCK2API_)
	if(m_socket_count_max > (SIZE_MAX - 64))
		LOG_SYSTEM << "  - maximum socket limits: INFINITE"sv;
	else
		LOG_SYSTEM << "  - maximum socket limits: "sv << m_socket_count_max;
#else
	LOG_SYSTEM << "  - maximum socket limits: "sv << m_socket_count_max << " (open-file limits:"sv << max_open_files << ")"sv;
#endif

	//-----------------------------------------------------------------
	// 3. Extension Function을 얻는다.
	//-----------------------------------------------------------------
	// 1) Extension Function Address 얻기...
	get_extension_function();


	//-----------------------------------------------------------------
	// 4. 자신의 IP를 얻는다.
	//-----------------------------------------------------------------
	// 1) 자신의 IP Address를 얻는다.
	char strLocal[MAX_STRING_LENGTH]; 

	// 2) 자신의 host이름을 얻는다.
	auto result = ::gethostname(strLocal, MAX_STRING_LENGTH);
	
	// check) 못얻었으면 false를 return한다.
	RETURN_IF(result == SOCKET_ERROR, false);

	struct addrinfo hints;
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// declare) 
	addrinfo* addr_result = nullptr;

	// 1) Address Info를 얻는다.
	uint32_t return_value = ::getaddrinfo(strLocal, nullptr, &hints, &addr_result);

	// checK) 구하지 못했다면 이것으로 끝낸다.
	RETURN_IF(return_value != 0, false);

	// log) 
	LOG_SYSTEM << "  - getting ip info";

	// declare) 
	std::vector<net::sockaddr> vector_sockaddr;
	
	// 2) 구한 address 정보를 저장한다.
	for (addrinfo* ptr= addr_result; ptr != NULL; ptr=ptr->ai_next)
	{
		switch (ptr->ai_family) 
		{
		case	AF_UNSPEC:
				break;

		case	AF_INET:
				{
					auto sockaddr_v4 = *(sockaddr_in*)ptr->ai_addr;

					vector_sockaddr.push_back(sockaddr_v4);
					char temp[256]={};
					sockaddr_in* address_socket = (sockaddr_in*)ptr->ai_addr;
					inet_ntop(ptr->ai_family, &address_socket->sin_addr, temp, 256);
					LOG_SYSTEM << "      local ipv4("sv << (int32_t)m_naddress_local;
					//LOG_SYSTEM << "      local ipv4("sv << (long)m_naddress_local << "): "sv /*<< (const char*)temp*/;
				}
				break;

		case	AF_INET6:
				{
					auto sockaddr_v6 = *(sockaddr_in6*)ptr->ai_addr;

					vector_sockaddr.push_back(sockaddr_v6);
					char temp[256]={};
					sockaddr_in6* address_socket = (sockaddr_in6*)ptr->ai_addr;
					inet_ntop(ptr->ai_family, &address_socket->sin6_addr, temp, 256);
					LOG_SYSTEM << "      local ipv6("sv << (long)m_naddress_local << "): "sv << (const char*)temp << "%"sv << address_socket->sin6_scope_id;
				}
				break;

	#if defined(_WINSOCK2API_)
		case	AF_NETBIOS:
				break;
	#endif
		default:
				break;
		}
	}
	
	// 3) 할당해제
	::freeaddrinfo(addr_result);

	// 4) 얻은 address들을 복사한다.
	int i=0;
	for(auto iter = vector_sockaddr.begin(); iter != vector_sockaddr.end(); ++iter, ++i)
	{
		// - 값을 복사한다.
		m_addr_local.at(i) = *iter;
	}
	m_naddress_local = static_cast<int>(vector_sockaddr.size());

	// 2) 초기화에 성공했음!!
	LOG_SYSTEM << log_level_t::important() << " [api.net] complete initializing."sv;

	// return) 성공!!!!
	return true;
}

void api::socket::cleanup_socket() noexcept
{
	// declare) 
	size_t new_value = 0;

	// check) socket의 숫자가 0일 경우만 SIZE_MAX로 변경하며 clean-up한다.
	RETURN_IF(m_socket_count.compare_exchange_strong(new_value, SIZE_MAX) == false);

#if defined(_WINSOCK2API_)
	// 1) Clean UP한다.
	WSACleanup();
#endif

	// 2) Clean Up했음.
	LOG_INFO << "(prgr) CGDK.net.socket: socket cleanup";
}

void api::socket::get_extension_function() noexcept
{
#if defined(_WIN32)
	//  1) 임시로 socket을 생성한다.
#if _MSC_PLATFORM_TOOLSET>=120
	CGSOCKET handle_socket = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
#else
	CGSOCKET handle_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
#endif

	// check) handle_socket이 INVALID_SOCKET이어서는 안된다.(더 이상 진행을 할 수 없음.)
	ERROR_RETURN_IF(handle_socket == INVALID_SOCKET, , LOG_ERROR << log_level_t::important() << "(err ) CGDK.net.socket: fail to create socket[error_code:"sv << api::get_last_error() << "]"sv);

	// 2) accept_async를 얻는다.
	{
		// declare) 
		GUID tempGUID_AcceptEx = WSAID_ACCEPTEX;
		int result;
		uint32_t dwByte;

		// - 함수포인터를 얻는다.
		result	 = WSAIoctl(handle_socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &tempGUID_AcceptEx, sizeof(tempGUID_AcceptEx), &WSAcceptEx, sizeof(WSAcceptEx), reinterpret_cast<LPDWORD>(&dwByte), nullptr, nullptr);

		//// trace) 성공!
		//EXECUTE_IF(result != SOCKET_ERROR, LOG_SYSTEM << "  - 'accept_async' function initialized"sv);
	}

	// 3) connect_async를 얻는다.
	{
		// declare) 
		GUID tempGUID_ConnectEx = WSAID_CONNECTEX;
		int result;
		uint32_t dwByte;

		// - 함수포인터를 얻는다.
		result	 = WSAIoctl(handle_socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &tempGUID_ConnectEx, sizeof(tempGUID_ConnectEx), &WSConnectEx, sizeof(WSConnectEx), reinterpret_cast<LPDWORD>(&dwByte), nullptr, nullptr);

		//// trace) 성공!
		//EXECUTE_IF(result != SOCKET_ERROR, LOG_SYSTEM << "  - 'WSConnectEx' function initialized"sv);
	}

	// 4) disconnect_async를 얻는다.
	{
		// declare) 
		GUID tempGUID_DisconnectEx = WSAID_DISCONNECTEX;
		int result;
		uint32_t dwByte;

		// - 함수포인터를 얻는다.
		result	 = WSAIoctl(handle_socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &tempGUID_DisconnectEx, sizeof(tempGUID_DisconnectEx), &WSDisconnectEx, sizeof(WSDisconnectEx), reinterpret_cast<LPDWORD>(&dwByte), nullptr, nullptr);

		//// trace) 성공!! 
		//EXECUTE_IF(result != SOCKET_ERROR, LOG_SYSTEM << "  - 'disconnect_async' function initialized"sv);
	}

	// 5) RIO 함수 포인터들을 얻는다.
	{
		// declare) 
		GUID tempGUID_RIOFunctions = WSAID_MULTIPLE_RIO;
		uint32_t dwByte;

		// - 함수포인터를 얻는다.
		auto result	 = WSAIoctl(handle_socket, SIO_GET_MULTIPLE_EXTENSION_FUNCTION_POINTER, &tempGUID_RIOFunctions, sizeof(tempGUID_RIOFunctions), &m_rio_functions, sizeof(m_rio_functions), reinterpret_cast<LPDWORD>(&dwByte), nullptr, nullptr);

		// trace) 성공!
		if (result != SOCKET_ERROR)
		{
			LOG_SYSTEM << "  - 'registered io' function initialized"sv;
		}
		else
		{
			m_rio_functions.cbSize = 0;
		}
	}

	// 6) 소캣을 닫는다.
	::closesocket(handle_socket);
#endif
}

#if defined(_WINSOCK2API_)
bool api::socket::io_ctrl(
	CGSOCKET	_handle_socket,
	long		_command,
	void*		_lpv_in_buffer, 
	uint32_t	_cb_in_buffer,
	void*		_pout_buffer,
	uint32_t	_cb_out_buffer,
	uint32_t*	_pbytes_returned) noexcept
{
	// declare) 
	uint32_t byte_required = 0;

	// 1) _pbytes_returned가 nullptr이면 지역변수로 처리한다.
	if(_pbytes_returned == nullptr)
	{
		_pbytes_returned = &byte_required;
	}

	// 2) WSAIoctl을 실행한다.
	return WSAIoctl(_handle_socket, _command, _lpv_in_buffer, _cb_in_buffer, _pout_buffer, _cb_out_buffer, reinterpret_cast<LPDWORD>(_pbytes_returned), nullptr, nullptr)!=SOCKET_ERROR;
}
#else
bool api::socket::io_ctrl(
	CGSOCKET	_handle_socket,
	long		_command,
	void*		_lpv_in_buffer, 
	uint32_t	/*_cb_in_buffer*/,
	void*		/*_pout_buffer*/,
	uint32_t	/*_cb_out_buffer*/,
	uint32_t*	/*_pbytes_returned*/) noexcept
{
	// return)
	return ::ioctl(_handle_socket, _command, _lpv_in_buffer) >= 0;
}
#endif

//-----------------------------------------------------------------------------
// tcp socket options (sSOCKET_OPTION_TCP / Isocket::apply_socket_option())
//-----------------------------------------------------------------------------
#if defined(_WINSOCK2API_)
bool api::socket::set_tcp_quickack(CGSOCKET _handle_socket, bool _enable) noexcept
{
	// - SIO_TCP_SET_ACK_FREQUENCY : the segments per ACK (1 : an ACK for every segment = delayed ACK off, 2 : the default of windows)
	int frequency = (_enable) ? 1 : 2;
	uint32_t bytes_returned = 0;

	return api::socket::io_ctrl(_handle_socket, SIO_TCP_SET_ACK_FREQUENCY, &frequency, sizeof(frequency), nullptr, 0, &bytes_returned);
}

bool api::socket::set_busy_poll(CGSOCKET /*_handle_socket*/, uint32_t /*_usec*/) noexcept
{
	// - linux only (SO_BUSY_POLL)
	WSASetLastError(WSAEOPNOTSUPP);
	return false;
}

bool api::socket::set_tcp_notsent_lowat(CGSOCKET /*_handle_socket*/, uint32_t /*_bytes*/) noexcept
{
	// - linux/macos only (TCP_NOTSENT_LOWAT)
	WSASetLastError(WSAEOPNOTSUPP);
	return false;
}

bool api::socket::set_tcp_fastopen(CGSOCKET _handle_socket, int _queue_length) noexcept
{
#if defined(TCP_FASTOPEN)
	// - windows 10 1607+ : enable (the queue length is not a parameter)
	int enable = (_queue_length > 0) ? 1 : 0;
	return set_sock_opt(_handle_socket, IPPROTO_TCP, TCP_FASTOPEN, &enable, sizeof(enable));
#else
	(void)_handle_socket; (void)_queue_length;
	WSASetLastError(WSAEOPNOTSUPP);
	return false;
#endif
}

bool api::socket::set_tcp_fastopen_connect(CGSOCKET _handle_socket, bool _enable) noexcept
{
#if defined(TCP_FASTOPEN)
	// - the same option on a connecting socket : set before ConnectEx
	int enable = (_enable) ? 1 : 0;
	return set_sock_opt(_handle_socket, IPPROTO_TCP, TCP_FASTOPEN, &enable, sizeof(enable));
#else
	(void)_handle_socket; (void)_enable;
	WSASetLastError(WSAEOPNOTSUPP);
	return false;
#endif
}

int api::socket::get_sysctl_tcp_fastopen() noexcept
{
	// - linux only
	return -1;
}
#else
bool api::socket::set_tcp_quickack(CGSOCKET _handle_socket, bool _enable) noexcept
{
#if defined(TCP_QUICKACK)
	int value = (_enable) ? 1 : 0;
	return set_sock_opt(_handle_socket, IPPROTO_TCP, TCP_QUICKACK, &value, sizeof(value));
#else
	(void)_handle_socket; (void)_enable;
	errno = ENOPROTOOPT;
	return false;
#endif
}

bool api::socket::set_busy_poll(CGSOCKET _handle_socket, uint32_t _usec) noexcept
{
#if defined(SO_BUSY_POLL)
	int value = static_cast<int>(_usec);
	return set_sock_opt(_handle_socket, SOL_SOCKET, SO_BUSY_POLL, &value, sizeof(value));
#else
	(void)_handle_socket; (void)_usec;
	errno = ENOPROTOOPT;
	return false;
#endif
}

bool api::socket::set_tcp_notsent_lowat(CGSOCKET _handle_socket, uint32_t _bytes) noexcept
{
#if defined(TCP_NOTSENT_LOWAT)
	int value = static_cast<int>(_bytes);
	return set_sock_opt(_handle_socket, IPPROTO_TCP, TCP_NOTSENT_LOWAT, &value, sizeof(value));
#else
	(void)_handle_socket; (void)_bytes;
	errno = ENOPROTOOPT;
	return false;
#endif
}

bool api::socket::set_tcp_fastopen(CGSOCKET _handle_socket, int _queue_length) noexcept
{
#if defined(TCP_FASTOPEN)
	// - the max pending TFO connections of the listen socket (capped by somaxconn). the server side needs sysctl net.ipv4.tcp_fastopen & 2 too
	return set_sock_opt(_handle_socket, IPPROTO_TCP, TCP_FASTOPEN, &_queue_length, sizeof(_queue_length));
#else
	(void)_handle_socket; (void)_queue_length;
	errno = ENOPROTOOPT;
	return false;
#endif
}

bool api::socket::set_tcp_fastopen_connect(CGSOCKET _handle_socket, bool _enable) noexcept
{
#if defined(TCP_FASTOPEN_CONNECT)
	// - kernel 4.11+ : connect() returns 0 at once and the first send() sends the SYN (with the data when a cookie is cached)
	int value = (_enable) ? 1 : 0;
	return set_sock_opt(_handle_socket, IPPROTO_TCP, TCP_FASTOPEN_CONNECT, &value, sizeof(value));
#else
	(void)_handle_socket; (void)_enable;
	errno = ENOPROTOOPT;
	return false;
#endif
}

int api::socket::get_sysctl_tcp_fastopen() noexcept
{
#if defined(__linux__)
	// - /proc/sys/net/ipv4/tcp_fastopen (bit 1 : client, bit 2 : server, 0x200 : no cookie on the client, 0x400 : no cookie on the server)
	FILE* pfile = ::fopen("/proc/sys/net/ipv4/tcp_fastopen", "r");

	// check)
	RETURN_IF(pfile == nullptr, -1);

	int value = -1;
	if (::fscanf(pfile, "%d", &value) != 1)
		value = -1;
	::fclose(pfile);

	return value;
#else
	return -1;
#endif
}
#endif

bool api::socket::connect(
			CGSOCKET _handle_socket,
	 const ::sockaddr* _address,
			int _len_sockaddr) noexcept
{
	// check) _address가 nullptr이어서는 안된다.
	CGASSERT(_address != nullptr, false)

	// check) _len_sockaddr가 0이어서는 안된다.
	CGASSERT(_len_sockaddr != 0, false)

	// 1) connect를 수행한다.
#if defined(_WINSOCK2API_)
	auto result = WSAConnect(_handle_socket, _address, _len_sockaddr, nullptr, nullptr, nullptr, nullptr);
#else
	auto result = ::connect(_handle_socket, _address, _len_sockaddr);
#endif

	// check) connect의 실패했을 경우.
	ERROR_RETURN_IF(result == SOCKET_ERROR, false, LOG_DEBUG << "(err ) CGDK.net.socket: fail to WSAConnect socket:" << static_cast<int64_t>(_handle_socket) << "/ error_code:" << api::get_last_error() << " " __CGFUNCTION__);

	// return) 
	return true;
}

std::vector<api::socket::resolver_entry> api::socket::resolve(int _protocol_family, int _protocol_type, int _protocol_protocol, const std::string_view _host_name, u_short _port, int _flag, std::error_code& _error_code) noexcept
{
	// declare) 
	std::string host_name { _host_name };

	// 1) set hints values
	addrinfo hints;
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_flags		 = static_cast<int>(_flag);
	hints.ai_family		 = _protocol_family;
	hints.ai_socktype	 = _protocol_type;
	hints.ai_protocol	 = _protocol_protocol;
	hints.ai_addr		 = nullptr;
	hints.ai_addrlen	 = 0;
	hints.ai_canonname	 = nullptr;
	hints.ai_next		 = nullptr;
	addrinfo* addr_result = nullptr;

	// declare) 
	std::vector<api::socket::resolver_entry> result_addr;

	// 2) get address info
	auto ret_value = ::getaddrinfo(host_name.c_str(), nullptr, &hints, &addr_result);

	// checK) 구하지 못했다면 이것으로 끝낸다.
	if (ret_value != 0)
	{
		// - set error code
		_error_code = std::make_error_code(std::errc(ret_value));

		// return) 
		return result_addr;
	}

	// 3) 구한 Address 정보를 저장한다.
	for (addrinfo* ptr = addr_result; ptr != NULL; ptr = ptr->ai_next)
	{
		switch (ptr->ai_family) 
		{
		case	AF_UNSPEC:
				break;

		case	AF_INET:
				result_addr.push_back(resolver_entry{ {*(sockaddr_in*)ptr->ai_addr}, host_name, "" });
				result_addr.back().addr.length = static_cast<int>(ptr->ai_addrlen);
				result_addr.back().addr.port(_port);
				break;

		case	AF_INET6:
				result_addr.push_back(resolver_entry{ {*(sockaddr_in6*)ptr->ai_addr}, host_name , "" });
				result_addr.back().addr.length = static_cast<int>(ptr->ai_addrlen);
				result_addr.back().addr.port(_port);
				break;

		default:
				break;
		}
	}
	
	// 3) 할당해제
	::freeaddrinfo(addr_result);

	// returns)
	return result_addr;
}

std::vector<api::socket::resolver_entry> api::socket::resolve(int _protocol_family, int _protocol_type, int _protocol_protocol, const std::string_view _host_name, const std::string_view _service_name, int _flag, std::error_code& _error_code) noexcept
{
	// declare) 
	std::string host_name { _host_name };
	std::string service_name{ _service_name };

	// declare) 
	std::vector<api::socket::resolver_entry> result_addr;

	// 1) 
	if (_host_name.empty() == false)
	{
		// 2) set hints values
		addrinfo hints;
		std::memset(&hints, 0, sizeof(hints));
		hints.ai_flags = static_cast<int>(_flag);
		hints.ai_family = _protocol_family;
		hints.ai_socktype = _protocol_type;
		hints.ai_protocol = _protocol_protocol;
		hints.ai_addr = nullptr;
		hints.ai_addrlen = 0;
		hints.ai_canonname = nullptr;
		hints.ai_next = nullptr;
		addrinfo* addr_result = nullptr;

		// 3) get address info
		uint32_t ret_value = ::getaddrinfo(host_name.c_str(), service_name.c_str(), &hints, &addr_result);

		// checK) 구하지 못했다면 이것으로 끝낸다.
		if (ret_value != 0)
		{
			// - set error code
			_error_code = std::make_error_code(std::errc(ret_value));

			// return) 
			return result_addr;
		}

		// 4) 구한 Address 정보를 저장한다.
		for (addrinfo* ptr = addr_result; ptr != NULL; ptr = ptr->ai_next)
		{
			switch (ptr->ai_family) 
			{
			case	AF_UNSPEC:
					break;

			case	AF_INET:
					result_addr.emplace_back(net::sockaddr{*(sockaddr_in*)ptr->ai_addr}, host_name , service_name);
					result_addr.back().addr.length = static_cast<int>(ptr->ai_addrlen);
					break;

			case	AF_INET6:
					result_addr.emplace_back(net::sockaddr{*(sockaddr_in6*)ptr->ai_addr}, host_name , service_name);
					result_addr.back().addr.length = static_cast<int>(ptr->ai_addrlen);
					break;

			default:
					break;
			}
		}
	
		// 3) 할당해제
		::freeaddrinfo(addr_result);
	}
	else
	{
		switch (_protocol_family)
		{
		case	AF_UNSPEC:
				{
					api::socket::resolver_entry temp_address;

					uint16_t service_port = 0;
					std::from_chars(_service_name.data(), _service_name.data() + _service_name.size(), service_port);
					temp_address.addr = net::sockaddr(net::ip::address_v6::any(), service_port);
					temp_address.hostname = "any";
					temp_address.servicename = service_name;

					result_addr.push_back(std::move(temp_address));
				}
				break;

		case	AF_INET:
				{
					api::socket::resolver_entry temp_address;

					uint16_t service_port = 0;
					std::from_chars(_service_name.data(), _service_name.data() + _service_name.size(), service_port);
					temp_address.addr = net::sockaddr(net::ip::address_v4::any(), service_port);
					temp_address.hostname = "any";
					temp_address.servicename = service_name;

					result_addr.push_back(std::move(temp_address));
				}
				break;

		case	AF_INET6:
				{
					api::socket::resolver_entry temp_address;

					uint16_t service_port = 0;
					std::from_chars(_service_name.data(), _service_name.data() + _service_name.size(), service_port);
					temp_address.addr = net::sockaddr(net::ip::address_v6::any(), service_port);
					temp_address.hostname = "any";
					temp_address.servicename = service_name;

					result_addr.push_back(std::move(temp_address));
				}
				break;

		default:
				break;
		}
	}

	// returns)
	return result_addr;
}
std::vector<api::socket::resolver_entry> api::socket::resolve(int _protocol_type, const net::sockaddr& _sockaddr, std::error_code& _error_code) noexcept
{
	// definitions) 
	const std::size_t S1 = 64;
	const std::size_t S2 = 64;

	char host_name[S1];
	char service_name[S2];
	int flags = (_protocol_type != SOCK_DGRAM) ? 0 : NI_DGRAM;

	int result = getnameinfo((const ::sockaddr*)&_sockaddr, _sockaddr.size(), host_name, S1, service_name, S2, flags);

	if (result != 0)
	{
		flags |= NI_NUMERICSERV;
		result = getnameinfo((const ::sockaddr*)&_sockaddr, _sockaddr.size(), host_name, S1, service_name, S2, flags);
	}

	// declare) 
	std::vector<resolver_entry> result_addr;

	// check)
	if (result != 0)
	{
		// - set error code
		_error_code = std::make_error_code(std::errc(result));

		// return) 
		return result_addr;
	}

	// 3) set values
	result_addr.emplace_back(_sockaddr, host_name , service_name);

	// returns) On success, a results object with size() == 1 containing the results of the resolve operation. Otherwise results_type().S
	return result_addr;
}
#if defined(_WINSOCK2API_)
CGSOCKET api::socket::create_socket(
			ADDRESS_FAMILY	_address_family,
			int _socket_type,
			int _protocol_info,
			uint32_t _flags) noexcept
{
	// check) socket limits
	if (m_socket_count >= m_socket_count_max)
	{
		// - error
		WSASetLastError(WSAETOOMANYREFS);

		// return)
		return INVALID_SOCKET;
	}

	// 1) socket을 생성한다.
	#if _MSC_PLATFORM_TOOLSET >= 120
	CGSOCKET handle_socket = WSASocketW(_address_family, _socket_type, _protocol_info, nullptr, 0, _flags);
	#else
	CGSOCKET handle_socket = WSASocket(_address_family, _socket_type, _protocol_info, nullptr, 0, _flags);
	#endif

	// check) INVALID_SOCKET이면 실패한 것이다. 그럼 끝낸다.
	ERROR_RETURN_IF(handle_socket == INVALID_SOCKET, INVALID_SOCKET, LOG_DEBUG << "(err ) CGDK.net.socket: fail to call' socket()' error_code:" << api::get_last_error() << " " __CGFUNCTION__);

	// 2) socket 개수를 증가시킨다.
	++m_socket_count;

	// return) 성공!!!!
	return handle_socket;
}

#else
CGSOCKET api::socket::create_socket(
			ADDRESS_FAMILY	_address_family,
			int _socket_type,
			int _protocol_info,
			uint32_t /*_flags*/) noexcept
{
	// check) socket limits
	if(m_socket_count >= m_socket_count_max)
	{
		// - error
		errno = EMFILE;

		// log)
		LOG_DEBUG << "(err ) CGDK.net.socket: fail to call' socket()' error_code:" << api::get_last_error() << " " __CGFUNCTION__;

		// return)
		return INVALID_SOCKET;
	}

	// 1) listen을 할 socket을 생성한다.
	CGSOCKET handle_socket = ::socket (_address_family, _socket_type | SOCK_CLOEXEC | SOCK_NONBLOCK, _protocol_info);		// - SOCK_CLOEXEC : not inherited by fork/exec. SOCK_NONBLOCK : no fcntl after (every CGDK socket is nonblocking)

	// check) INVALID_SOCKET이면 실패한 것이다. 그럼 끝낸다.
	ERROR_RETURN_IF(handle_socket == INVALID_SOCKET, INVALID_SOCKET, LOG_DEBUG << "(err ) CGDK.net.socket: fail to call' socket()' error_code:" << api::get_last_error() << " " __CGFUNCTION__);

	// 2) socket 개수를 증가시킨다.
	++m_socket_count;

	// return) 성공!!!!
	return handle_socket;
}
#endif

bool api::socket::closesocket( CGSOCKET _handle_socket) noexcept
{
	// 1) socket을 닫음.(socket을 닫는데 Error가 발생했다면 엄청 심각한 것이다.)
#if defined(_WINSOCK2API_)
	int result = ::closesocket(_handle_socket);

	// check) closesocket를 제대로 수행했는지 결과를 Return한다.
	if (result != SOCKET_ERROR || api::get_last_error() == EWOULDBLOCK)
	{
		// declare)
		[[maybe_unused]] size_t socket_count;
		
		// - socket 개수를 감소시킨다.
		socket_count = --m_socket_count;

		// check) 
		CGASSERT_ERROR(socket_count != SIZE_MAX);

		// return) 
		return true;
	}
#else
	int result = ::close(_handle_socket);

	// check) closesocket를 제대로 수행했는지 결과를 Return한다.
	if (result != SOCKET_ERROR)
	{
		// declare)
		[[maybe_unused]] size_t socket_count;

		// - socket 개수를 감소시킨다.
		socket_count = --m_socket_count;

		// check) 
		CGASSERT_ERROR(socket_count != SIZE_MAX);

		// return) 
		return true;
	}
#endif
	else
	{
		// return) 
		return false;
	}
}

bool api::socket::graceful_closesocket(CGSOCKET _handle_socket) noexcept
{
	// 1) disable linger option and graceful close
	set_linger_option(_handle_socket, 0);

	// 2) socket을 닫는다.
	return closesocket(_handle_socket);
}

bool api::socket::abortive_closesocket( CGSOCKET _handle_socket) noexcept
{
	// 1) enable linger option and abortive close
	set_linger_option(_handle_socket, 1);

	// 2) socket을 닫는다.
	return closesocket(_handle_socket);
}

CGSOCKET api::socket::accept(CGSOCKET _handle_socket, ::sockaddr* _paddress, socklen_t* _psocket_address_length) noexcept
{
	// 1) accept
#if defined(_WINSOCK2API_)
	auto handle_socket_accepted = ::accept(_handle_socket, _paddress, _psocket_address_length);
#else
	auto handle_socket_accepted = ::accept4(_handle_socket, _paddress, _psocket_address_length, SOCK_CLOEXEC | SOCK_NONBLOCK);		// - SOCK_CLOEXEC : not inherited by fork/exec. SOCK_NONBLOCK : no fcntl after
#endif

	// check) 
	if (handle_socket_accepted != INVALID_SOCKET)
	{
		// - check socket limits
		if (m_socket_count < m_socket_count_max)
		{
			++m_socket_count;
		}
		else 
		{
			// - enable linger option and abortive close
			set_linger_option(handle_socket_accepted, 1);

		#if defined(_WINSOCK2API_)
			// - close socket handle
			::closesocket(handle_socket_accepted);

			// - set error code
			WSASetLastError(WSAETOOMANYREFS);
		#elif defined(_SYS_EPOLL_H)
			// - close socket handle
			::close(handle_socket_accepted);

			// - set error code
			errno = EMFILE;
		#endif

			// return)
			handle_socket_accepted = INVALID_SOCKET;
		}
	}

	// return)
	return handle_socket_accepted;
}

#if defined(_WINSOCK2API_)
int api::socket::send(
	CGSOCKET							_handle_socket,
	const buffer_view*					_array_buffer,
	int									_count_buffer) noexcept
{
	// check)
	CGASSERT(_array_buffer != nullptr, false)

	// check)
	CGASSERT(_count_buffer != 0, false);

	// declare) 
	DWORD flags = 0;
	DWORD bytes_send = 0;

	// 1) request send
	auto result = WSASend(_handle_socket, reinterpret_cast<LPWSABUF>(const_cast<buffer_view*>(_array_buffer)), static_cast<DWORD>(_count_buffer), &bytes_send, flags, nullptr, nullptr);

	// return) 
	return result;
}

int api::socket::recv(
	CGSOCKET				_handle_socket,
	buffer_view*			_pbuffer,
	uint32_t				_flags) noexcept
{
	// check)
	CGASSERT_FATAL(_pbuffer != nullptr)

	// check)
	CGASSERT_ERROR(_pbuffer->data() != nullptr)

	// check)
	CGASSERT_ERROR(_pbuffer->size() != 0)

	// declare) 
	DWORD bytes_received = 0;

	// 1) WSABUF
	WSABUF wsabuf{ _pbuffer->size<ULONG>(), _pbuffer->data()};

	// 2) 받는다.
	auto result = WSARecv(_handle_socket, &wsabuf, 1, &bytes_received, reinterpret_cast<LPDWORD>(&_flags), nullptr, nullptr);

	// return)
	return result; // (result != SOCKET_ERROR || api::get_last_error() == WSA_IO_PENDING);
}

int api::socket::send_to(
		CGSOCKET			_handle_socket,
		const buffer_view*	_array_buffer,
		int					_count_buffer,
		::sockaddr*			_pto,
		int					_to_length) noexcept
{
	// check)
	CGASSERT(_array_buffer != nullptr, false);

	// check)
	CGASSERT(_array_buffer->data() != nullptr, false);

	// check)
	CGASSERT(_count_buffer != 0, false);

	// check)
	CGASSERT(_pto != nullptr, false);

	// declare)
	DWORD bytes_send = 0;

	// 1) send한다.
	auto result = WSASendTo(_handle_socket, reinterpret_cast<LPWSABUF>(const_cast<buffer_view*>(_array_buffer)), static_cast<DWORD>(_count_buffer), &bytes_send, 0, _pto, _to_length, nullptr, nullptr);

	// return)
	return result; //  (result != SOCKET_ERROR || api::get_last_error() == WSA_IO_PENDING);
}

int api::socket::recv_from(
	CGSOCKET				_handle_socket,
	buffer_view*			_pbuffer,
	uint32_t				_flags,
	::sockaddr*				_pfrom,
	int*					_pfrom_len) noexcept
{
	// check)
	CGASSERT(_pbuffer != nullptr, false);

	// check)
	CGASSERT(_pbuffer->data() != nullptr, false);

	// check)
	CGASSERT(_pfrom != nullptr, false);

	// check)
	CGASSERT(_pfrom_len != nullptr, false);

	// declare)
	DWORD bytes_received = 0;

	// 1) request UDP receive
	auto result = WSARecvFrom(_handle_socket, reinterpret_cast<LPWSABUF>(_pbuffer), 1, &bytes_received, reinterpret_cast<LPDWORD>(&_flags), _pfrom, _pfrom_len, nullptr, nullptr);

	// check) 
	return result;
}

#else
int api::socket::send(
	CGSOCKET							_handle_socket,
	const buffer_view*					_array_buffer,
	[[maybe_unused]] int				_count_buffer) noexcept
{
	// check)
	CGASSERT(_array_buffer != nullptr, false)

	// check)
	CGASSERT(_count_buffer == 1, false);

	// 1) request send
	auto result = ::send(_handle_socket, _array_buffer->data(), _array_buffer->size(), MSG_NOSIGNAL);

	// return)
	return result;
}

int api::socket::recv(
		CGSOCKET			/*_handle_socket*/,
		buffer_view*		/*_pbuffer*/,
		uint32_t			/*_flags*/) noexcept
{
	// error) 
	CGASSERT_ERROR(false);

	// return) 
	return 0;
}

int api::socket::send_to(
		CGSOCKET			/*_handle_socket*/,
		const buffer_view*	/*_array_buffer*/,
		int					/*_count_buffer*/,
		::sockaddr*			/*_pto*/,
		int					/*_to_length*/) noexcept
{
	// error) 
	CGASSERT_ERROR(false);

	// return) 
	return 0;
}

int api::socket::recv_from(
	CGSOCKET			/*_handle_socket*/,
	buffer_view*		/*_array_buffer*/,
	uint32_t			/*_flags*/,
	::sockaddr*			/*_pfrom*/,
	int*				/*_pfrom_len*/) noexcept
{
	// error) 
	CGASSERT_ERROR(false);

	// return) 
	return 0;
}

#endif


#if defined(_WINSOCK2API_)
bool api::socket::accept_async(
	CGSOCKET				_handle_socket,
	CGSOCKET				_handle_accept_socket,
	void*					_poutput_buffer,
	LPWSAOVERLAPPED			_poverlapped) noexcept
{
	// check)
	CGASSERT(_handle_accept_socket != INVALID_SOCKET, false);

	// check)
	CGASSERT(_poutput_buffer != nullptr, false);

	// check)
	CGASSERT(_poverlapped != nullptr, false);

	// 1) request accept
	auto result = WSAcceptEx(_handle_socket, _handle_accept_socket, _poutput_buffer, 0, sizeof(net::sockaddr) + 16, sizeof(net::sockaddr) + 16, nullptr, _poverlapped);

	// return) 
	return (result != 0 || api::get_last_error() == ERROR_IO_PENDING);
}

bool api::socket::connect_async(
	CGSOCKET				_handle_socket,
	Iexecutable*			_pexecutable,
	const ::sockaddr*		_paddress,
	int						_addess_length) noexcept
{
	// check)
	CGASSERT(_pexecutable != nullptr, false);

	// check)
	CGASSERT(_paddress != nullptr, false);

	// check)
	CGASSERT(_addess_length != 0, false);

	// declare) 
	uint32_t lpdwByteSend = 0;

	// 1) request connect
	auto result = WSConnectEx(_handle_socket, _paddress, _addess_length, nullptr, 0, &lpdwByteSend, _pexecutable);

	// return)
	return (result == TRUE || api::get_last_error() == ERROR_IO_PENDING);
}

bool api::socket::disconnect_async(
		CGSOCKET						_handle_socket,
		LPWSAOVERLAPPED					_poverlapped,
		uint32_t						_flags) noexcept
{
	// check)
	CGASSERT_FATAL(WSDisconnectEx != nullptr);

	// 1) request disconnect_async
	auto result = WSDisconnectEx(_handle_socket, _poverlapped, _flags | TF_REUSE_SOCKET, 0);
	
	// return)
	return (result == TRUE || api::get_last_error() == ERROR_IO_PENDING);
}

bool api::socket::send_async(
	CGSOCKET							_handle_socket,
	LPWSABUF							_array_wsabuf,
	DWORD								_count_buffer,
	LPDWORD								_pbytes_send,
	LPWSAOVERLAPPED						_poverlapped) noexcept
{
	// check)
	CGASSERT(_array_wsabuf != nullptr, false)

	// check)
	CGASSERT(_count_buffer != 0, false);

	// declare) 
	DWORD _flags = 0;

	// 1) request send
	auto result = WSASend(_handle_socket, _array_wsabuf, _count_buffer, _pbytes_send, _flags, _poverlapped, nullptr);

	// return)
	return	(result != SOCKET_ERROR || api::get_last_error() == WSA_IO_PENDING);
}

bool api::socket::recv_async(
		CGSOCKET							_handle_socket,
		LPWSABUF							_pwsabuf,
		LPDWORD								_pbytes_received,
		LPDWORD								_pflags,
		LPWSAOVERLAPPED						_poverlapped) noexcept
{
	// check)
	CGASSERT_FATAL(_pwsabuf != nullptr);

	// check)
	CGASSERT_FATAL(_pwsabuf->buf != nullptr);

	// check)
	CGASSERT_ERROR(_pwsabuf->len != 0)

	// 1) 받는다.
	auto result = WSARecv(_handle_socket, _pwsabuf, 1, _pbytes_received, _pflags, _poverlapped, nullptr);

	// return)
	return (result != SOCKET_ERROR || api::get_last_error() == WSA_IO_PENDING);
}

bool api::socket::send_to_async(
		CGSOCKET			_handle_socket,
		LPWSABUF			_array_wsabuf,
		DWORD				_count_buffer,
		LPDWORD				_pbytes_send,
		::sockaddr*			_pto,
		int					_to_length,
		LPWSAOVERLAPPED		_poverlapped) noexcept
{
	// check)
	CGASSERT_FATAL(_array_wsabuf != nullptr);

	// check)
	CGASSERT_FATAL(_array_wsabuf->buf != nullptr);

	// check)
	CGASSERT_ERROR(_count_buffer != 0);

	// check)
	CGASSERT_ERROR(_pto != nullptr);
	
	// 1) send한다.
	auto result = WSASendTo(_handle_socket, _array_wsabuf, _count_buffer, _pbytes_send, 0, _pto, _to_length, _poverlapped, nullptr);

	// return) 결과를 Return한다.
	return (result != SOCKET_ERROR || api::get_last_error() == WSA_IO_PENDING);
}

bool api::socket::recv_from_async(
		CGSOCKET			_handle_socket,
		LPWSABUF			_array_wsabuf,
		DWORD				_count_buffer,
		LPDWORD				_pbytes_received,
		LPDWORD				_pflags,
		::sockaddr*			_pfrom,
		int*				_pfrom_len,
		LPWSAOVERLAPPED		_poverlapped) noexcept
{
	// check)
	CGASSERT_FATAL(_array_wsabuf != nullptr);

	// check)
	CGASSERT_FATAL(_array_wsabuf->buf != nullptr);

	// check)
	CGASSERT_FATAL(_count_buffer != 0);

	// check)
	CGASSERT(_pfrom != nullptr, false);

	// check)
	CGASSERT(_pfrom_len != nullptr, false);

	// check)
	CGASSERT(_pbytes_received != nullptr, false);

	// check)
	if(_pbytes_received == nullptr)
		return false;


	//-----------------------------------------------------------------
	// 1. recv_from을 수행한다.
	//    - 여기서 Error가 발생 시 Error를 p_lpBytesReceive에 넣는다.
	//    - 이유는 receive시 connect reset으로 Error가 발생할 수 있다.
	//      이 때 적절한 처리를 위해 이런 처리를 수행한다.
	//-----------------------------------------------------------------
	// 1) request UDP receive
	auto result = WSARecvFrom(_handle_socket, _array_wsabuf, _count_buffer, _pbytes_received, _pflags, _pfrom, _pfrom_len, _poverlapped, nullptr);

	// check) 
	if(result == SOCKET_ERROR && api::get_last_error() != WSA_IO_PENDING)
	{
		if(_pbytes_received != nullptr)
			*_pbytes_received = api::get_last_error();

		// return) 
		return false;
	}

	// return)
	return true;
}

#endif

bool api::socket::get_local_address_local(net::sockaddr & _socket_address) noexcept
{
	if(_socket_address.u_v4.sin_family == AF_UNSPEC)
		_socket_address.u_v4.sin_family = AF_INET;;

	for(int i=0; i<m_naddress_local; ++i)
	{
		if(m_addr_local.at(i).is_private())
		{
			if (m_addr_local.at(i).u_v4.sin_family == _socket_address.u_v4.sin_family)
			{
				_socket_address = m_addr_local.at(i);
				return true;
			}
		}
	}

	return false;
}

bool api::socket::get_local_address_public(net::sockaddr & _socket_address) noexcept
{
	if (_socket_address.u_v4.sin_family == AF_UNSPEC)
		_socket_address.u_v4.sin_family = AF_INET;;

	for (int i=0; i<m_naddress_local; ++i)
	{
		if (m_addr_local.at(i).is_public())
		{
			if (m_addr_local.at(i).u_v4.sin_family==_socket_address.u_v4.sin_family)
			{
				_socket_address = m_addr_local.at(i);
				return true;
			}
		}
	}

	return false;
}

bool api::socket::get_local_address_public_first(net::sockaddr & _socket_address) noexcept
{
	// 1) 먼저 Public IP를 얻는다.
	auto result = get_local_address_public(_socket_address);

	// 2) Public IP가 없으면 Local IP를 얻는다.
	if(result == false)
		result = get_local_address_local(_socket_address);

	// return) 결과값을 리턴한다.
	return result;
}

#if defined(_WINSOCK2API_)
bool api::socket::rio_is_enable() noexcept
{
	return m_rio_functions.cbSize != 0;
}

BOOL api::socket::rio_receive(
			RIO_RQ		_rqSocket,
			PRIO_BUF	_array_riobuf,
			int			_count_buffer,
			DWORD		_flags,
			void*		_request_context) noexcept
{
	return m_rio_functions.RIOReceive(_rqSocket, _array_riobuf, static_cast<DWORD>(_count_buffer), _flags, _request_context);
}

int api::socket::rio_receive_ex(
			RIO_RQ		_rqSocket,
			PRIO_BUF	_array_riobuf,
			int			_count_buffer,
			PRIO_BUF	_plocal_address,
			PRIO_BUF	_premote_address,
			PRIO_BUF	_pcontrol_context,
			PRIO_BUF	_pflags,
			DWORD		_flags,
			void*		_request_context) noexcept
{
	return m_rio_functions.RIOReceiveEx(_rqSocket, _array_riobuf, static_cast<DWORD>(_count_buffer), _plocal_address, _premote_address, _pcontrol_context, _pflags, _flags, _request_context);
}

BOOL api::socket::rio_send(
			RIO_RQ		_rqSocket,
			PRIO_BUF	_array_riobuf,
			int			_count_buffer,
			DWORD		_flags,
			void*		_request_context) noexcept
{
	return m_rio_functions.RIOSend(_rqSocket, _array_riobuf, static_cast<DWORD>(_count_buffer), _flags, _request_context);
}

BOOL api::socket::rio_send_ex(	
			RIO_RQ		_rqSocket,
			PRIO_BUF	_array_riobuf,
			int			_count_buffer,
			PRIO_BUF	_plocal_address,
			PRIO_BUF	_premote_address,
			PRIO_BUF	_pcontrol_context,
			PRIO_BUF	_pflags,
			DWORD		_flags,
			void*		_request_context) noexcept
{
	return m_rio_functions.RIOSendEx(_rqSocket, _array_riobuf, static_cast<DWORD>(_count_buffer), _plocal_address, _premote_address, _pcontrol_context, _pflags, _flags, _request_context);
}

void api::socket::rio_close_completion_queue( RIO_CQ _CQ) noexcept
{
	return m_rio_functions.RIOCloseCompletionQueue(_CQ);
}


RIO_CQ api::socket::rio_create_completion_queue(
			int			_queue_size,
			PRIO_NOTIFICATION_COMPLETION _notification_completion) noexcept
{
	return m_rio_functions.RIOCreateCompletionQueue(static_cast<DWORD>(_queue_size), _notification_completion);
}

RIO_RQ api::socket::rio_create_request_queue(
			SOCKET		_socket,
			DWORD		_max_outstanding_receive,
			DWORD		_max_receive_data_buffers,
			DWORD		_max_outstanding_send,
			DWORD		_max_send_data_buffers,
			RIO_CQ		_receive_CQ,
			RIO_CQ		_send_CQ,
			void*		_socket_context) noexcept
{
	return m_rio_functions.RIOCreateRequestQueue(_socket, _max_outstanding_receive, _max_receive_data_buffers, _max_outstanding_send, _max_send_data_buffers, _receive_CQ, _send_CQ, _socket_context);
}

ULONG api::socket::rio_dequeue_completion(
			RIO_CQ		_CQ,
			PRIORESULT	_array,
			DWORD		_array_size) noexcept
{
	return m_rio_functions.RIODequeueCompletion(_CQ, _array, _array_size);
}

void api::socket::rio_deregister_buffer( RIO_BUFFERID _BufferId) noexcept
{
	m_rio_functions.RIODeregisterBuffer(_BufferId);
}

INT api::socket::rio_notify( RIO_CQ _CQ) noexcept
{
	return m_rio_functions.RIONotify(_CQ);
}

RIO_BUFFERID api::socket::rio_register_buffer(
			PCHAR		_data_buffer,
			DWORD		_data_length) noexcept
{
	return m_rio_functions.RIORegisterBuffer(_data_buffer, _data_length);
}

BOOL api::socket::rio_resize_completion_queue(
			RIO_CQ		_CQ,
			DWORD		_queue_size) noexcept
{
	return m_rio_functions.RIOResizeCompletionQueue(_CQ, _queue_size);
}

BOOL api::socket::rio_resize_request_queue(
			RIO_RQ		_RQ,
			DWORD		_max_outstanding_receive,
			DWORD		_max_outstanding_send) noexcept
{
	return m_rio_functions.RIOResizeRequestQueue(_RQ, _max_outstanding_receive, _max_outstanding_send);
}
#endif