#include "pch.h"


namespace CGDK
{
namespace net
{
namespace ip
{

bool address_v6::is_v4_compatible() const noexcept
{
  return ((sin6_addr.s6_addr[0] == 0) && (sin6_addr.s6_addr[1] == 0)
      && (sin6_addr.s6_addr[2] == 0) && (sin6_addr.s6_addr[3] == 0)
      && (sin6_addr.s6_addr[4] == 0) && (sin6_addr.s6_addr[5] == 0)
      && (sin6_addr.s6_addr[6] == 0) && (sin6_addr.s6_addr[7] == 0)
      && (sin6_addr.s6_addr[8] == 0) && (sin6_addr.s6_addr[9] == 0)
      && (sin6_addr.s6_addr[10] == 0) && (sin6_addr.s6_addr[11] == 0)
      && !((sin6_addr.s6_addr[12] == 0) && (sin6_addr.s6_addr[13] == 0)
      && (sin6_addr.s6_addr[14] == 0)
      && ((sin6_addr.s6_addr[15] == 0) || (sin6_addr.s6_addr[15] == 1))));
}

// - get_v4_mapped() is defined in the header (constexpr)

address_v6 make_address_v6(const std::string_view _str, std::error_code& _error_code) noexcept
{
	// check) _address_peer가 nullptr이라 connect() 수행불가.
	CGASSERT_ERROR(_str.empty() == false);

	// declare)
	address_v6 result;

	// check)
	if (_str.empty())
	{
		// - set error_code
		_error_code = make_error_code(std::errc::invalid_argument);

		// return) 
		return result;
	}

	// 1) "localhost" - ":1"
	if (_str == "localhost" || _str == "loopback")
	{
		result = address_v6::loopback();
	}
	// 2) "any" - "0"
	else if (_str == "any")
	{
		result = address_v6::any();
	}
	else
	{
		// 1) to addr
		auto ret = inet_pton(AF_INET6, _str.data(), &result.sin6_addr);

		// check) 
		if (ret <= 0)
		{
			// - set error_code
			_error_code = make_error_code(std::errc::invalid_argument);
		}
	}
	
	// return)
	return result;
}



}
}
}