#include "pch.h"


namespace CGDK
{
namespace net
{
namespace ip
{

address_v4 make_address_v4(const std::string_view _str, std::error_code& _error_code) noexcept
{
	// check) _address_peer가 nullptr이라 connect() 수행불가.
	CGASSERT_ERROR(_str.empty() == false);

	// declare)
	address_v4 result;

	// check)
	if (_str.empty())
	{
		// - set error_code
		_error_code = make_error_code(std::errc::invalid_argument);

		// return) 
		return result;
	}

	// 1) "localhost" - "127.0.0.1"
	if (_str == "localhost"sv || _str == "loopback"sv)
	{
		result = address_v4::loopback();
	}
	// 2) "any" - "0.0.0.0"
	else if (_str == "any"sv)
	{
		result = address_v4::any();
	}
	// 3) ~
	else
	{
		// - to addr
		auto result_addr = inet_addr(_str.data());

		// check) 
		if (result_addr == INADDR_NONE)
		{
			// - set error_code
			_error_code = make_error_code(std::errc::invalid_argument);

			// return)
			return result;
		}

		// - set
		result.S_addr = result_addr;
	}
	
	// return)
	return result;
}

template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& /*_output_stream*/, const address_v4& /*_address*/)
{
	std::basic_ostream<CharT, Traits>();
}

}
}
}