#include "pch.h"

//-----------------------------------------------------------------------------
// from_string (address string -> net::sockaddr)
//
//  - "any" : the any address (v6 by default) with the port given. (the hints form returned port 0 before)
//  - "localhost" : the loopback address with the port given.
//  - the forms with the hints / the preferred address family / a wide string give the same port.
//-----------------------------------------------------------------------------
namespace CGDK
{

TEST(sockaddr_from_string, any_keeps_the_port)
{
	const auto addr = from_string("any", 20000);
	EXPECT_TRUE(addr.is_v6());
	EXPECT_TRUE(addr.is_bind_any());
	EXPECT_EQ(addr.port(), 20000);

	const auto addr_upper = from_string("ANY", 20001);
	EXPECT_TRUE(addr_upper.is_bind_any());
	EXPECT_EQ(addr_upper.port(), 20001);

	const auto addr_wide = from_string(L"any", 20002);
	EXPECT_TRUE(addr_wide.is_bind_any());
	EXPECT_EQ(addr_wide.port(), 20002);

	const auto addr_zero = from_string("any");
	EXPECT_TRUE(addr_zero.is_bind_any());
	EXPECT_EQ(addr_zero.port(), 0);
}

TEST(sockaddr_from_string, any_same_as_af_prefer_form)
{
	const auto addr_hints = from_string("any", 20010);
	const auto addr_prefer = from_string("any", 20010, static_cast<ADDRESS_FAMILY>(AF_INET6));
	EXPECT_EQ(addr_hints, addr_prefer);

	const auto addr_prefer_v4 = from_string("any", 20011, static_cast<ADDRESS_FAMILY>(AF_INET));
	EXPECT_TRUE(addr_prefer_v4.is_v4());
	EXPECT_TRUE(addr_prefer_v4.is_bind_any());
	EXPECT_EQ(addr_prefer_v4.port(), 20011);
}

TEST(sockaddr_from_string, localhost_keeps_the_port)
{
	const auto addr = from_string("localhost", 20020);
	EXPECT_TRUE(addr.is_v4());
	EXPECT_TRUE(addr.is_loopback());
	EXPECT_EQ(addr.port(), 20020);

	const auto addr_numeric = from_string("127.0.0.1", 20021);
	EXPECT_TRUE(addr_numeric.is_loopback());
	EXPECT_EQ(addr_numeric.port(), 20021);
}

}
