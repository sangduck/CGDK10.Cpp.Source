#include "pch.h"


namespace CGDK
{
namespace net
{
namespace ip
{

// make_network)
network_v4 make_network_v4(const char* /*_str*/)
{
	return network_v4();
}

network_v4 make_network_v4(const char* /*_str*/, std::error_code& /*_error_code*/) noexcept
{
	return network_v4();
}

network_v4 make_network_v4(const std::string& /*_str*/)
{
	return network_v4();
}

network_v4 make_network_v4(const std::string& /*_str*/, std::error_code& /*_error_code*/) noexcept
{
	return network_v4();
}

network_v4 make_network_v4(std::string_view /*_str*/)
{
	return network_v4();
}

network_v4 make_network_v4(std::string_view /*_str*/, std::error_code& /*_error_code*/) noexcept
{
	return network_v4();
}



}
}
}