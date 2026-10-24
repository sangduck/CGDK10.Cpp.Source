#include "pch.h"
#include <cstring>

using namespace CGDK;

namespace CGDK
{

std::string_view to_string(eSOCKET_STATE _state) noexcept
{
	// declare)
	std::string_view result;

	switch(_state)
	{
	case	eSOCKET_STATE::CLOSED:
			result = "CLOSED"sv;
			break;

	case	eSOCKET_STATE::BINDED:
			result  = "BINDED"sv;
			break;

	case	eSOCKET_STATE::SYN:
			result = "SYN"sv;
			break;

	case	eSOCKET_STATE::SEND_DISCONNECTED:
			result = "SND_DIS"sv;
			break;

	case	eSOCKET_STATE::FIN_WAIT_1:
			result = "FIN_WAIT_1"sv;
			break;

	case	eSOCKET_STATE::FIN_WAIT_2:
			result = "FIN_WAIT_2"sv;
			break;

	case	eSOCKET_STATE::TIME_WAIT:
			result = "TIME_WAIT"sv;
			break;

	case	eSOCKET_STATE::CLOSE_WAIT:
			result = "CLOSE_WAIT"sv;
			break;

	case	eSOCKET_STATE::LAST_ACK:
			result = "LAST_ACK"sv;
			break;

	case	eSOCKET_STATE::ESTABLISHED:
			result = "ESTABLISHED"sv;
			break;

	case	eSOCKET_STATE::LISTEN:
			result = "LISTEN"sv;
			break;

	case	eSOCKET_STATE::CERTIFIED:
			result = "CERTIFIED"sv;
			break;

	case	eSOCKET_STATE::LOGINED:
			result = "LOGINED"sv;
			break;
	default:
			result = ""sv;
			break;
	};

	// return)
	return result;
}

net::sockaddr from_string(const std::string_view _address, uint16_t _port, ADDRESS_FAMILY _af_prefer)
{
	// 1) 
	api::socket::instance();

	// check)
	THROW_IF(_address.size() > 127, throwable::failure(eRESULT::LENGTH_ERROR));

	// declare)
	static_string<128> address = _address;

	// 2) Hints
	{
#if defined(_WIN32)
		if (_strnicmp(address.c_str(), "localhost", 10) == 0 || _strnicmp(address.c_str(), "loopback", 9) == 0)
#else
		if (strncasecmp(address.c_str(), "localhost", 10) == 0 || strncasecmp(address.c_str(), "loopback", 9) == 0)
#endif
		{
			switch (_af_prefer)
			{
			case	AF_INET:
				return net::sockaddr(net::ip::address_v4::loopback(), _port);
			case	AF_INET6:
				return net::sockaddr(net::ip::address_v6::loopback(), _port);
			default:
				return net::sockaddr(net::ip::address_v6::loopback(), _port);
			}
		}
#if defined(_WIN32)
		else if (_strnicmp(address.c_str(), "any", 4) == 0)
#else
		else if (strncasecmp(address.c_str(), "any", 4) == 0)
#endif
		{
			switch (_af_prefer)
			{
			case	AF_INET:
				return net::sockaddr(net::ip::address_v4::any(), _port);
			case	AF_INET6:
				return net::sockaddr(net::ip::address_v6::any(), _port);
			default:
				return net::sockaddr(net::ip::address_v6::any(), _port);
			}
		}
#if defined(_WIN32)
		else if (_strnicmp(address.c_str(), "broadcast", 10) == 0)
#else
		else if (strncasecmp(address.c_str(), "broadcast", 10) == 0)
#endif
		{
			switch (_af_prefer)
			{
			case	AF_INET:
				return net::sockaddr(net::ip::address_v4::broadcast(), _port);
			case	AF_INET6:
				throw throwable::failure(eRESULT::LENGTH_ERROR);
			default:
				throw throwable::failure(eRESULT::LENGTH_ERROR);
			}
		}
	}

	// declare) 
	net::sockaddr addr_result;
	addrinfo* paddr_info_result = nullptr;

	// 2) get address info
	auto result = ::getaddrinfo(address.c_str(), nullptr, nullptr, &paddr_info_result);

	// check)
	ERROR_RETURN_IF(result != 0, addr_result, LOG_ERROR << log_level_t::important() << "(err ) 'GetAddrInfo' function[Error:"sv << api::get_last_error() << "]"sv);

	// check)
	ERROR_RETURN_IF(paddr_info_result == nullptr, addr_result, LOG_ERROR << log_level_t::important() << "(err ) 'GetAddrInfo' function[error_code:"sv << api::get_last_error() << "]"sv);

	// declare) 
	addrinfo* paddr_info_selected = nullptr;

	// 3) _af_prefer와 동일한 AF가 있는지 찾는다.
	for (auto* piter = paddr_info_result; piter != nullptr; piter = piter->ai_next)
	{
		if (piter->ai_family == _af_prefer)
		{
			paddr_info_selected = piter;
			break;
		}
	}

	// 3) _af_prefer와 동일한 AF가 없으면 그냥 제일 처음 것을 찾는다.
	if (paddr_info_selected == nullptr)
	{
		paddr_info_selected = paddr_info_result;
	}

	// 4) socket Family설정.
	switch (paddr_info_selected->ai_family)
	{
	case	AF_INET:
	{
		CGASSERT_ERROR(paddr_info_selected->ai_addrlen == sizeof(sockaddr_in));

		addr_result.sockaddr_v4 = *reinterpret_cast<sockaddr_in*>(paddr_info_selected->ai_addr);
		addr_result.u_v4.sin_port = htons(_port);
		addr_result.length = static_cast<int>(paddr_info_selected->ai_addrlen);

		// check) 
		CGASSERT_ERROR(addr_result.u_v4.sin_family == AF_INET);
	}
	break;

	case	AF_INET6:
	{
		CGASSERT_ERROR(paddr_info_selected->ai_addrlen == sizeof(sockaddr_in6));

		addr_result.sockaddr_v6 = *reinterpret_cast<sockaddr_in6*>(paddr_info_selected->ai_addr);
		addr_result.u_v6.sin_port = htons(_port);
		addr_result.length = static_cast<int>(paddr_info_selected->ai_addrlen);

		// check) 
		CGASSERT_ERROR(addr_result.u_v6.sin_family == AF_INET6);
	}
	break;

	default:
		if (paddr_info_selected->ai_addrlen <= sizeof(net::sockaddr))
		{
			memcpy(reinterpret_cast<void*>(&addr_result), reinterpret_cast<void*>(paddr_info_selected->ai_addr), paddr_info_selected->ai_addrlen);
			addr_result.u_v6.sin_port = htons(_port);
			addr_result.length = static_cast<int>(paddr_info_selected->ai_addrlen);
		}
		else
		{
		}
		break;
	}

	// 4) 할당해제해준다.
	::freeaddrinfo(paddr_info_selected);

	// return) 
	return	addr_result;
}

net::sockaddr from_string(std::wstring_view _wstr_address, uint16_t _port, ADDRESS_FAMILY _af_prefer)
{
	// 1) 
	api::socket::instance();

	// declare) 
	char str_address[MAX_STRING_LENGTH]{};

	// 2) String을 Convert한다.
	encode_string(str_address, _wstr_address);

	// return)
	return from_string(str_address, _port, _af_prefer);
}

net::sockaddr from_string(const std::string_view _address, uint16_t _port, const addrinfo* _hints)
{
	// check) 
	api::socket::instance();

	// check)
	THROW_IF(_address.size() > 127, throwable::failure(eRESULT::LENGTH_ERROR));

	// declare) 
	addrinfo tempHints;

	// declare)
	static_string<128> address = _address;

	// 1) Hints
	if (_hints == nullptr)
	{
#if defined(_WIN32)
		if (_strnicmp(address.c_str(), "localhost", 10) == 0)
#else
		if (strncasecmp(address.c_str(), "localhost", 10) == 0)
#endif
		{
			std::memset(&tempHints, 0, sizeof(tempHints));
			tempHints.ai_family = AF_INET;
			_hints = &tempHints;
		}
#if defined(_WIN32)
		else if (_strnicmp(address.c_str(), "localhost6", 11) == 0)
#else
		else if (strncasecmp(address.c_str(), "localhost6", 11) == 0)
#endif
		{
			std::memset(&tempHints, 0, sizeof(tempHints));
			tempHints.ai_family = AF_INET6;
			_hints = &tempHints;
		}
#if defined(_WIN32)
		else if (_strnicmp(address.c_str(), "any", 9) == 0)
#else
		else if (strncasecmp(address.c_str(), "any", 9) == 0)
#endif
		{
			return net::sockaddr{ net::ip::address_v6::any(), 0 };
		}
	}

	// declare) 
	net::sockaddr addr_result;
	addrinfo* paddr_info_result = nullptr;

	// 2) Address Info르 얻는다.
	auto result = ::getaddrinfo(address.c_str(), nullptr, _hints, &paddr_info_result);

	// check) tempHostEnt가 nullptr이면 대략 낭패닷!
	ERROR_RETURN_IF(result != 0, addr_result, LOG_ERROR << log_level_t::important() << "(err ) 'GetAddrInfo' function[error:"sv << api::get_last_error() << "]"sv);

	// check) tempHostEnt가 nullptr이면 대략 낭패닷!
	ERROR_RETURN_IF(paddr_info_result == nullptr, addr_result, LOG_ERROR << log_level_t::important() << "(err ) 'GetAddrInfo' function[error_code:"sv << api::get_last_error() << "]"sv);

	// 3) socket Family설정.
	switch (paddr_info_result->ai_family)
	{
	case	AF_INET:
	{
		CGASSERT_ERROR(paddr_info_result->ai_addrlen == sizeof(sockaddr_in));

		addr_result.sockaddr_v4 = *(sockaddr_in*)paddr_info_result->ai_addr;
		addr_result.u_v6.sin_port = htons(_port);
		addr_result.length = static_cast<int>(paddr_info_result->ai_addrlen);
	}
	break;

	case	AF_INET6:
	{
		CGASSERT_ERROR(paddr_info_result->ai_addrlen == sizeof(sockaddr_in6));

		addr_result.sockaddr_v6 = *(sockaddr_in6*)paddr_info_result->ai_addr;
		addr_result.u_v6.sin_port = htons(_port);
		addr_result.length = static_cast<int>(paddr_info_result->ai_addrlen);
	}
	break;

	default:
		if (paddr_info_result->ai_addrlen <= sizeof(net::sockaddr))
		{
			memcpy(reinterpret_cast<void*>(&addr_result), reinterpret_cast<void*>(paddr_info_result->ai_addr), paddr_info_result->ai_addrlen);
			addr_result.u_v6.sin_port = htons(_port);
			addr_result.length = static_cast<int>(paddr_info_result->ai_addrlen);
		}
		else
		{
			addr_result.length = 0;
		}
		break;
	}

	// 4) 할당해제해준다.
	::freeaddrinfo(paddr_info_result);

	// return) 
	return addr_result;
}

net::sockaddr from_string(std::wstring_view _wstr_address, uint16_t _port, const addrinfo* _hints)
{
	// 1) initialize socket
	api::socket::instance();

	// declare) 
	char str_address[MAX_STRING_LENGTH]{};

	// 2) String을 Convert한다.
	encode_string(str_address, _wstr_address);

	// return)
	return from_string(str_address, _port, _hints);
}

net::sockaddr from_string(std::string_view _address, std::string_view _service, int _socket_type, int _protocol)
{
	api::socket::instance();

	addrinfo addrinfoHints{};
	addrinfoHints.ai_family = AF_UNSPEC;
	addrinfoHints.ai_socktype = _socket_type;
	addrinfoHints.ai_protocol = _protocol;

	net::sockaddr addr_result;
	addrinfo* paddr_info_result = nullptr;

	auto result = ::getaddrinfo(_address.data(), _service.data(), &addrinfoHints, &paddr_info_result);
	ERROR_RETURN_IF(result != 0, addr_result, LOG_ERROR << log_level_t::important() << "(err ) 'GetAddrInfo' function[error:"sv << api::get_last_error() << "]"sv);
	ERROR_RETURN_IF(paddr_info_result == nullptr, addr_result, LOG_ERROR << log_level_t::important() << "(err ) 'GetAddrInfo' function[error_code:"sv << api::get_last_error() << "]"sv);

	switch (paddr_info_result->ai_family)
	{
	case AF_INET:
		addr_result = *(sockaddr_in*)paddr_info_result->ai_addr;
		addr_result.length = sizeof(sockaddr_in);
		break;
	case AF_INET6:
		addr_result = *(sockaddr_in6*)paddr_info_result->ai_addr;
		addr_result.length = sizeof(sockaddr_in6);
		break;
	default:
		addr_result.length = 0;
		break;
	}

	::freeaddrinfo(paddr_info_result);
	return addr_result;
}
net::sockaddr from_string(std::wstring_view _wstr_address, std::wstring_view _str_service, int _socket_type, int _protocol)
{
	// 1) 
	api::socket::instance();

	// declare) 
	char str_address[MAX_STRING_LENGTH]{};
	char str_service[MAX_STRING_LENGTH]{};

	// 2) String을 Convert한다.
	encode_string(str_address, _wstr_address);
	encode_string(str_service, _str_service);

	// return)
	return from_string(str_address, str_service, _socket_type, _protocol);
}

std::string to_string(const net::sockaddr& _socket_address)
{
	// declare)
	std::string result;

	if (_socket_address.is_unix())
	{
		result = "unix";
	}
	else if (_socket_address.is_loopback())
	{
		result = "localhost";
	}
	else if (_socket_address.is_bind_any())
	{
		result = "any";
	}
	else
	{
		char str_address[256];

#if defined(_WINSOCK2API_)
		auto temp = InetNtopA(_socket_address.u_v6.sin_family, const_cast<void*>(_socket_address.get_paddr()), str_address, 256);
#else
		auto temp = inet_ntop(_socket_address.u_v6.sin_family, const_cast<void*>(_socket_address.get_paddr()), str_address, 256);
#endif
		if (temp != nullptr)
		{
			result = temp;
		}
	}

	// return)
	return result;
}

std::string to_string(const net::ip::address& _address)
{
	return _address.to_string();
}

std::string to_string(const net::ip::address_v4& _address)
{
	// declare)
	std::string result;

	if (_address.is_loopback())
	{
		result = "localhost";
	}
	else if (_address.is_unspecified())
	{
		result = "any";
	}
	else
	{
		char str_address[256];

	#if defined(_WINSOCK2API_)
		auto temp = InetNtopA(AF_INET, const_cast<void*>(reinterpret_cast<const void*>(&_address)), str_address, 256);
	#else
		auto temp = inet_ntop(AF_INET, const_cast<void*>(reinterpret_cast<const void*>(&_address)), str_address, 256);
	#endif
		if (temp != nullptr)
		{
			result = temp;
		}
	}

	// return)
	return result;
}

std::string to_string(const net::ip::address_v6& _address)
{
	// declare)
	std::string result;

	if (_address.is_loopback())
	{
		result = "localhost";
	}
	else if (_address.is_unspecified())
	{
		result = "any";
	}
	else
	{
		char str_address[256];

	#if defined(_WINSOCK2API_)
		auto temp = InetNtopA(AF_INET6, const_cast<void*>(reinterpret_cast<const void*>(&_address)), str_address, 256);
	#else
		auto temp = inet_ntop(AF_INET6, const_cast<void*>(reinterpret_cast<const void*>(&_address)), str_address, 256);
	#endif
		if (temp != nullptr)
		{
			result = temp;
		}
	}

	// return)
	return result;
}


//std::string_view							to_string(char* _str_address, std::size_t _max_length, const net::sockaddr& _sockaddr);
//inline std::string							to_string(const net::sockaddr& _address) { char temp[256]; return std::string(to_string(temp, 256, _address)); }
//inline std::string							to_string(const net::ip::address& _address) { char temp_str[256]; return std::string(to_string(temp_str, 256, _address)); }
//inline std::string							to_string(const net::ip::address_v4& _address) { char temp_str[256]; return std::string(to_string(temp_str, 256, _address)); }
//inline std::string							to_string(const net::ip::address_v6& _address) { char temp_str[256]; return std::string(to_string(temp_str, 256, _address)); }



}
