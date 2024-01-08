#include "pch.h"

using namespace CGDK;

#define	DEFAULT_RECONNECT_INTERVAL	10000


net::io::Ireconnectable::Ireconnectable() :
	m_list_peer_adddress(),
	m_iter_now_peer_address(),
	m_enable_reconnect(false),
	m_tick_reconnection_interval(DEFAULT_RECONNECT_INTERVAL),
	m_tick_disconnected(),
	m_flag_reconnecting(false)
{
	this->m_iter_now_peer_address._Ptr = nullptr;
}

bool net::io::Ireconnectable::add_peer_address( const net::sockaddr& _peer_address)
{
	scoped_lock(this->m_list_peer_adddress)
	{
		// 1) 먼저 등록된 것이 있는지 찾는다.
		auto iter = std::find(this->m_list_peer_adddress.begin(), this->m_list_peer_adddress.end(), _peer_address);

		// check) 이미 등록된 것이 있으면 끝낸다.
		RETURN_IF(iter != this->m_list_peer_adddress.end(), false);

		// 2) 등록된 것이 없으면 등록한다.
		this->m_list_peer_adddress.push_back(_peer_address);

		// 3) 만약 현재 것이 nullptr이라면 현재 값을 설정한다.
		if(this->m_iter_now_peer_address._Ptr == nullptr)
		{
			this->m_iter_now_peer_address = this->m_list_peer_adddress.begin();
		}
	}

	// return) 
	return true;
}

bool operator == ( const sockaddr_in& _lhs, const sockaddr_in& _rhs)
{
#if defined(_WINSOCK2API_)
	return _lhs.sin_addr.S_un.S_addr == _rhs.sin_addr.S_un.S_addr && _lhs.sin_port == _rhs.sin_port;
#else
	return _lhs.sin_addr.s_addr == _rhs.sin_addr.s_addr && _lhs.sin_port == _rhs.sin_port;
#endif
}

bool net::io::Ireconnectable::remove_peer_address( const net::sockaddr& _peer_address) noexcept
{
	scoped_lock(this->m_list_peer_adddress)
	{
		// 1) 먼저 등록된 것이 있는지 찾는다.
		auto iter = std::find(this->m_list_peer_adddress.begin(), this->m_list_peer_adddress.end(), _peer_address);

		// check) 이미 등록된 것이 있으면 끝낸다.
		RETURN_IF(iter != this->m_list_peer_adddress.end(), false);

		// 2) 만약 지우는 것이 현재의 것이라면 현재값을 변경시킨다.
		if(iter == this->m_iter_now_peer_address)
		{
			++this->m_iter_now_peer_address;

			if(this->m_iter_now_peer_address == this->m_list_peer_adddress.end())
			{
				this->m_iter_now_peer_address = this->m_list_peer_adddress.begin();
			}
		}

		// 2) 찾은 것을 제거한다.
		this->m_list_peer_adddress.erase(iter);
	}

	// return) 
	return true;
}

void net::io::Ireconnectable::remove_all_peer_address() noexcept
{
	scoped_lock(this->m_list_peer_adddress)
	{
		// 1) 리스트를 모두 클리어한다.
		this->m_list_peer_adddress.clear();

		// 2) peer Address의 Iterator도 reset한다.
		this->m_iter_now_peer_address._Ptr = nullptr;
	}
}

net::sockaddr net::io::Ireconnectable::move_next_peer_address() noexcept
{
	// declare)
	net::sockaddr address_peer;

	// check) Pointer가 nullptr이라면 하나도 추가가 되지 않은 것이다.
	RETURN_IF(m_iter_now_peer_address._Ptr == nullptr, address_peer);

	// 2) Address를 하나 읽어 접속을 시도한다.
	scoped_lock(this->m_list_peer_adddress)
	{
		// - 현재 Address를 얻고 다음 Address로 변경한다.
		address_peer = *(this->m_iter_now_peer_address++);

		// - 다음 Address가 End면 다시 처음으로 돌린다.
		if (m_iter_now_peer_address == this->m_list_peer_adddress.end())
		{
			this->m_iter_now_peer_address = this->m_list_peer_adddress.begin();
		}
	}

	// return)
	return address_peer;
}
