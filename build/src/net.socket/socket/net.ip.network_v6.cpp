#include "pch.h"


namespace CGDK
{
namespace net
{
namespace ip
{

//address_v6_range network_v6::hosts() const noexcept
//{
//  address_v6::bytes_type	begin_bytes(address_.to_bytes());
//  address_v6::bytes_type	end_bytes(address_.to_bytes());
//
//  for (std::size_t i = 0; i < 16; ++i)
//  {
//    if (prefix_length_ <= i * 8)
//    {
//      begin_bytes[i] = 0;
//      end_bytes[i] = 0xFF;
//    }
//    else if (prefix_length_ < (i + 1) * 8)
//    {
//      begin_bytes[i] &= 0xFF00 >> (prefix_length_ % 8);
//      end_bytes[i] |= 0xFF >> (prefix_length_ % 8);
//    }
//  }
//  return address_v6_range(
//      address_v6_iterator(address_v6(begin_bytes, address_.scope_id())),
//      ++address_v6_iterator(address_v6(end_bytes, address_.scope_id())));
//}

//std::string network_v6::to_string() const
//{
//	std::error_code ec;
//	std::string addr = to_string(ec);
//	std::experimental::net::v1::detail::throw_error(ec);
//	return addr;
//}

//std::string network_v6::to_string(std::error_code& _error_code) const
//{
//  using namespace std; // For sprintf.
//  _error_code = std::error_code();
//  char prefix_len[16];
//#if defined(NET_TS_HAS_SECURE_RTL)
//  sprintf_s(prefix_len, sizeof(prefix_len), "/%u", prefix_length_);
//#else // defined(NET_TS_HAS_SECURE_RTL)
//  sprintf(prefix_len, "/%u", prefix_length_);
//#endif // defined(NET_TS_HAS_SECURE_RTL)
//  return address_.to_string() + prefix_len;
//}

//network_v6 make_network_v6(const std::string& str)
//{
//  std::error_code ec;
//  network_v6 net = make_network_v6(str, ec);
//  std::experimental::net::v1::detail::throw_error(ec);
//  return net;
//}

//network_v6 make_network_v6(const std::string& str,
//    std::error_code& _error_code)
//{
//  std::string::size_type pos = str.find_first_of("/");
//
//  if (pos == std::string::npos)
//  {
//    _error_code = std::experimental::net::v1::error::invalid_argument;
//    return network_v6();
//  }
//
//  if (pos == str.size() - 1)
//  {
//    _error_code = std::experimental::net::v1::error::invalid_argument;
//    return network_v6();
//  }
//
//  std::string::size_type end = str.find_first_not_of("0123456789", pos + 1);
//  if (end != std::string::npos)
//  {
//    _error_code = std::experimental::net::v1::error::invalid_argument;
//    return network_v6();
//  }
//
//  const address_v6 addr = make_address_v6(str.substr(0, pos), _error_code);
//  if (_error_code)
//    return network_v6();
//
//  const int prefix_len = std::atoi(str.substr(pos + 1).c_str());
//  if (prefix_len < 0 || prefix_len > 128)
//  {
//    _error_code = std::experimental::net::v1::error::invalid_argument;
//    return network_v6();
//  }
//
//  return network_v6(addr, static_cast<unsigned short>(prefix_len));
//}

}
}
}