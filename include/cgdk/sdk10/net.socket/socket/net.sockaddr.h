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

class net::sockaddr
{
public:
			constexpr sockaddr() noexcept
			{
				this->u_v4.sin_family = AF_UNSPEC;
				this->u_v6.sin_port = 0;
				this->u_v6.address_ = net::ip::address_v6{};
				this->length = 0;
			}
			constexpr sockaddr(const ::sockaddr_in& _addr_v4) noexcept
			{
				this->u_v4.sin_family = _addr_v4.sin_family;
				this->u_v4.sin_port = _addr_v4.sin_port;
				this->u_v4.address_ = net::ip::address_v4{ _addr_v4.sin_addr.s_addr, false };
				this->u_v4.zero[0] = 0;
				this->length = sizeof(::sockaddr_in);
			}
			constexpr sockaddr(net::ip::address_v4 _addr_v4, uint16_t _port = 0) noexcept
			{
				this->u_v4.sin_family = AF_INET;
				this->u_v4.sin_port = constexpr_htons(_port);
				this->u_v4.address_ = _addr_v4;
				this->u_v4.zero[0] = 0;
				this->length = sizeof(::sockaddr_in);
			}
			constexpr sockaddr(const ::sockaddr_in6& _addr_v6) noexcept
			{
				this->u_v6.sin_family = AF_INET6;
				this->u_v6.sin_port = _addr_v6.sin6_port;
				this->u_v6.address_ = net::ip::address_v6{ _addr_v6.sin6_addr, _addr_v6.sin6_scope_id };
				this->length = sizeof(::sockaddr_in6);
			}
			constexpr sockaddr(net::ip::address_v6 _addr_v6, uint16_t _port = 0) noexcept : u_v6{ AF_INET6, constexpr_htons(_port), _addr_v6 }, length(sizeof(::sockaddr_in6)) 
			{
			}
			constexpr sockaddr(const net::ip::address& _rhs, uint16_t _port = 0) noexcept : u_v6{}
			{
				if (_rhs.is_v6())
				{
					u_v6.sin_family = AF_INET6;
					u_v6.sin_port = constexpr_htons(_port);
					u_v6.address_ = _rhs.ipv6_address_;
					length = sizeof(::sockaddr_in6);
				}
				else if (_rhs.is_v4())
				{
					u_v4.sin_family = AF_INET;
					u_v4.sin_port = constexpr_htons(_port);
					u_v4.address_ = _rhs.ipv4_address_;
					length = sizeof(::sockaddr_in);
				}
				else
				{
					u_v6.sin_family = AF_UNSPEC;
					u_v6.sin_port = constexpr_htons(_port);
					length = 0;
				}
			}
			constexpr sockaddr(const net::sockaddr& _copy) noexcept : bytes_{ _copy.bytes_ }, length(_copy.length) {}
			template <typename TPROTOCOL>
			constexpr sockaddr(const TPROTOCOL& _protocol, unsigned short _port) noexcept : u_v6{ static_cast<ADDRESS_FAMILY>(_protocol.family()), constexpr_htons(_port), net::ip::address_v6{} }, length(_protocol.size()) { if (_protocol.family() == AF_INET) u_v4.address_ = net::ip::address_v4::any(); else u_v6.address_ = net::ip::address_v6::any(); }
			template <class TPROTOCOL>
			constexpr sockaddr(const net::ip::basic_endpoint<TPROTOCOL>& _end_point);

public:
	[[nodiscard]] constexpr bool empty() const noexcept { return length == 0;}
	[[nodiscard]] constexpr bool exist() const noexcept { return length != 0;}
			void				clear() noexcept { u_v6.sin_family = AF_UNSPEC; length = 0; }
	[[nodiscard]] constexpr bool is_v4() const noexcept { return u_v6.sin_family == AF_INET;}
	[[nodiscard]] constexpr bool is_v6() const noexcept { return u_v6.sin_family == AF_INET6;}
	[[nodiscard]] constexpr bool is_unspecified() const noexcept { return u_v6.sin_family == AF_UNSPEC;}
	//! @brief a UDS (Unix domain socket) : a socket connected through the UDS of the same host (enable_uds of the acceptor). only the family is kept
	[[nodiscard]] constexpr bool is_unix() const noexcept { return u_v6.sin_family == AF_UNIX;}
	[[nodiscard]] constexpr bool is_bind_any() const noexcept { return is_v4() ? u_v4.address_.is_unspecified() : (is_v6() ? u_v6.address_.is_unspecified() : false);}
	//! @brief loopback address (a UDS peer is on the same host too)
	[[nodiscard]] constexpr bool is_loopback() const noexcept { return is_v4() ? u_v4.address_.is_loopback() : (is_v6() ? u_v6.address_.is_loopback() : is_unix());}
	[[nodiscard]] constexpr bool is_multicast() const noexcept { return is_v4() ? u_v4.address_.is_multicast() : (is_v6() ? u_v6.address_.is_multicast() : false);}

	[[nodiscard]] constexpr bool is_private() const noexcept { return is_v4() ? u_v4.address_.is_private() : (is_v6() ? u_v6.address_.is_private() : false);}
	[[nodiscard]] constexpr bool is_public() const noexcept { return !is_private();}

	[[nodiscard]] constexpr auto to_address_v4() const { if (is_v4() == false) throw std::bad_cast(); return u_v4.address_;}
	[[nodiscard]] constexpr auto to_address_v6() const { if (is_v6() == false) throw std::bad_cast(); return u_v6.address_;}

	[[nodiscard]] constexpr auto to_v4() const { if (is_v4() == false) throw std::bad_cast(); return sockaddr_v4;}
	[[nodiscard]] constexpr auto to_v6() const { if (is_v6() == false) throw std::bad_cast(); return sockaddr_v6;}

			constexpr void		address(const ::sockaddr_in& _addr) noexcept { this->operator=(_addr); }
			constexpr void		address(const ::sockaddr_in6& _addr) noexcept { this->operator=(_addr); }
			constexpr void		address(const net::ip::address_v4& _addr) noexcept { this->operator=(_addr); }
			constexpr void		address(const net::ip::address_v6& _addr) noexcept { this->operator=(_addr); }
			constexpr void		address(const net::ip::address& _addr) noexcept { this->operator=(_addr); }
			constexpr void		address(const net::sockaddr& _addr) noexcept { this->operator=(_addr); }
	[[nodiscard]] constexpr auto address() const { if (is_v4()) return ip::address(u_v4.address_); else return ip::address(u_v6.address_);}
	[[nodiscard]] constexpr auto& address_v4() { if (is_v4() == false) throw std::bad_cast(); return u_v4.address_;}
	[[nodiscard]] constexpr auto& address_v6() { if (is_v6() == false) throw std::bad_cast(); return u_v6.address_;}

	[[nodiscard]] constexpr auto family() const noexcept { return u_v6.sin_family;}
	[[nodiscard]] constexpr auto port() const noexcept { return constexpr_htons(u_v6.sin_port);}
			void				port(unsigned short _port) noexcept { u_v6.sin_port = htons(_port);}
	[[nodiscard]] constexpr auto size() const noexcept { return length;}

	[[nodiscard]] std::string to_string() const;

public:
			constexpr sockaddr&	operator = (const ::sockaddr_in& _rhs) noexcept
			{
				this->sockaddr_v4 = _rhs;
				this->length = sizeof(::sockaddr_in);
				return *this;
			}
			constexpr sockaddr&	operator = (const ::sockaddr_in6& _rhs) noexcept 
			{ 
				this->sockaddr_v6 = _rhs;
				this->length = sizeof(::sockaddr_in6);
				return *this;
			}
			constexpr sockaddr&	operator = (const net::ip::address_v4& _rhs) noexcept
			{
				this->u_v4.address_ =_rhs;
				this->u_v4.sin_family = AF_INET;
				this->length = sizeof(::sockaddr_in);
				return *this;
			}
			constexpr sockaddr&	operator = (const net::ip::address_v6& _rhs) noexcept
			{
				this->u_v6.address_ = _rhs;
				this->u_v6.sin_family = AF_INET6;
				this->length = sizeof(::sockaddr_in6); 
				return *this;
			}
			constexpr sockaddr&	operator = (const net::ip::address& _rhs) noexcept
			{
				if(_rhs.is_v6())
				{
					this->u_v6.address_ = _rhs.ipv6_address_;
					this->u_v6.sin_family = AF_INET6;
					this->length = sizeof(::sockaddr_in6);
				}
				else
				{
					this->u_v4.address_ = _rhs.ipv4_address_;
					this->u_v4.sin_family = AF_INET;
					this->length = sizeof(::sockaddr_in);
				}

				return *this;
			}
			constexpr sockaddr&	operator = (const net::sockaddr& _rhs) noexcept 
			{
				if (this == &_rhs)
					return *this;

				if (_rhs.is_v6())
					this->u_v6 = _rhs.u_v6;
				else if (_rhs.is_v4())
					this->u_v4 = _rhs.u_v4;
				else if (_rhs.is_unix())
				{
					this->u_v6 = {};
					this->u_v6.sin_family = AF_UNIX;
				}
				else
					this->u_v6 = {}; // 안전하게 초기화

				this->length = _rhs.length;

				return *this;
			}
								 
	[[nodiscard]] constexpr bool operator == (const net::sockaddr& _rhs) const noexcept{ return is_v4() ? ((u_v4.address_ == _rhs.u_v4.address_) && (u_v4.sin_port == _rhs.u_v4.sin_port)) : (is_v6() ? ((u_v6.address_ == _rhs.u_v6.address_) && (u_v6.sin_port == _rhs.u_v6.sin_port)) : (is_unix() && _rhs.is_unix()));}

	[[nodiscard]] constexpr operator ::sockaddr& () noexcept { return sockaddr_;}
	[[nodiscard]] constexpr operator const ::sockaddr&() const noexcept { return sockaddr_;}
	[[nodiscard]] constexpr operator ::sockaddr_in& () noexcept { return sockaddr_v4;}
	[[nodiscard]] constexpr operator const ::sockaddr_in&() const noexcept { return sockaddr_v4;}
	[[nodiscard]] constexpr operator net::ip::address_v4& () noexcept { return u_v4.address_;}
	[[nodiscard]] constexpr operator const net::ip::address_v4&() const noexcept { return u_v4.address_;}
	[[nodiscard]] constexpr operator ::sockaddr_in6&() noexcept { return sockaddr_v6;}
	[[nodiscard]] constexpr operator const ::sockaddr_in6&() const noexcept { return sockaddr_v6;}
	[[nodiscard]] constexpr operator net::ip::address_v6& () noexcept { return u_v6.address_;}
	[[nodiscard]] constexpr operator const net::ip::address_v6&() const noexcept{ return u_v6.address_;}

	[[nodiscard]] const void* get_paddr() const { return is_v4() ? reinterpret_cast<const void*>(&u_v4.address_) : reinterpret_cast<const void*>(&u_v6.address_);}

public:
		struct U_DATA_V4
		{
		public:
			constexpr U_DATA_V4() noexcept : sin_family{ AF_UNSPEC }, sin_port{ 0 }, address_{}, zero{} {}
			constexpr U_DATA_V4(const U_DATA_V4& _copy) : sin_family{ _copy.sin_family }, sin_port{ _copy.sin_port }, address_{ _copy.address_ }, zero{} {}
			constexpr U_DATA_V4(ADDRESS_FAMILY _sin_family, uint16_t _sin_port, const net::ip::address_v4& _v4_) : sin_family{ _sin_family }, sin_port{ _sin_port }, address_{ _v4_ }, zero{} {}
			constexpr U_DATA_V4& operator = (const U_DATA_V4& _rhs) noexcept { sin_family = _rhs.sin_family; sin_port = _rhs.sin_port; address_ = _rhs.address_; return *this;}

		public:
			ADDRESS_FAMILY		sin_family;
			uint16_t			sin_port;
			net::ip::address_v4	address_;
			char				zero[8];
		};

		struct U_DATA_V6
		{
		public:
			constexpr U_DATA_V6() : sin_family{ AF_UNSPEC }, sin_port{ 0 }, sin6_flowinfo{ 0 }, address_{} {}
			constexpr U_DATA_V6(const U_DATA_V6& _copy) : sin_family{ _copy.sin_family }, sin_port{ _copy.sin_port }, sin6_flowinfo{ _copy.sin6_flowinfo }, address_{ _copy.address_ }  {}
			constexpr U_DATA_V6(ADDRESS_FAMILY _sin_family, uint16_t _sin_port, const net::ip::address_v6& _v6_) : sin_family{ _sin_family }, sin_port{ _sin_port }, sin6_flowinfo{ 0 }, address_{ _v6_ } {}
			constexpr U_DATA_V6& operator = (const U_DATA_V6& _rhs) noexcept { sin_family = _rhs.sin_family; sin_port = _rhs.sin_port; sin6_flowinfo = _rhs.sin6_flowinfo; address_ = _rhs.address_; return *this; }

		public:
			ADDRESS_FAMILY		sin_family;
			uint16_t			sin_port;
			uint32_t			sin6_flowinfo;
			net::ip::address_v6	address_;
		};

		// check) 
		static_assert(sizeof(::sockaddr_in6) == 28);

		union
		{
			::sockaddr			sockaddr_;
			::sockaddr_in		sockaddr_v4;
			U_DATA_V4			u_v4;
			::sockaddr_in6		sockaddr_v6;
			U_DATA_V6			u_v6;
			std::array<unsigned char, 32> bytes_; // 32 bytes for 8byte alignment 
		};
		int		length{ 0 };
		int		_dummy{ 0 };
};

//[[nodiscard]] inline bool operator == (const net::sockaddr& _lhs, const net::sockaddr& _rhs) noexcept { return _lhs.is_v4() ? ((_lhs.u_v4.address_ == _rhs.u_v4.address_) && (_lhs.u_v4.sin_port == _rhs.u_v4.sin_port)) : (_lhs.is_v6() ? ((_lhs.u_v6.address_ == _rhs.u_v6.address_) && (_lhs.u_v6.sin_port == _rhs.u_v6.sin_port)) : false); }
//[[nodiscard]] inline bool operator != (const net::sockaddr& _lhs, const net::sockaddr& _rhs) noexcept { return !(_lhs == _rhs); }
//[[nodiscard]] inline bool operator <  (const net::sockaddr& _lhs, const net::sockaddr& _rhs) noexcept
//{
//	if (_lhs.is_v4())
//	{
//		if (_lhs.u_v4.address_ < _rhs.u_v4.address_)
//			return true;
//
//		if (_lhs.u_v4.address_ > _rhs.u_v4.address_)
//			return false;
//
//		return _lhs.u_v4.sin_port < _rhs.u_v4.sin_port;
//	}
//	else if (_lhs.is_v6())
//	{
//		if (_lhs.u_v6.address_ < _rhs.u_v6.address_)
//			return true;
//
//		if (_lhs.u_v6.address_ > _rhs.u_v6.address_)
//			return false;
//
//		return _lhs.u_v6.sin_port < _rhs.u_v6.sin_port;
//	}
//	else
//	{
//		return false;
//	}
//}
//[[nodiscard]] inline bool operator >  (const net::sockaddr& _lhs, const net::sockaddr& _rhs) noexcept { return _rhs < _lhs; }
//[[nodiscard]] inline bool operator <= (const net::sockaddr& _lhs, const net::sockaddr& _rhs) noexcept { return !(_rhs < _lhs); }
//[[nodiscard]] inline bool operator >= (const net::sockaddr& _lhs, const net::sockaddr& _rhs) noexcept { return !(_lhs < _rhs); }


// ----------------------------------------------------------------------------
// Structures
// ----------------------------------------------------------------------------
struct SEND
{
	SEND(net::io::Isender* _psender) { m_psender = _psender;}
	SEND(net::io::Isender& _psender) { m_psender = &_psender;}

	net::io::Isender*	m_psender;
};

struct SEND_TO
{
	SEND_TO(net::io::Isender_datagram* _psender, const net::sockaddr& _address) :m_psender_datagram(_psender), m_sockaddr(_address) {}
	SEND_TO(net::io::Isender_datagram& _psender, const net::sockaddr& _address) :m_psender_datagram(&_psender), m_sockaddr(_address) {}

	net::io::Isender_datagram*	m_psender_datagram;
	net::sockaddr				m_sockaddr;
};


}


