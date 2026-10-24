#include "pch.h"

using namespace CGDK;


bool net::io::Ibinder::bind(const net::sockaddr& _address)
{
	return process_request_bind(_address);
}

void net::io::Ibinder::register_binder(net::io::statistics::Nsocket* _pstatistics_socket)
{
	SCOPED_LOCK(m_lockable_manager_binder)
	{
		// 1) bind manager를 얻는다.
		binder::manager* pmanagerBinder	 = net::io::binder::manager::instance();

		// 2) 등록한다.
		pmanagerBinder->join(this, _pstatistics_socket);

		// 3) 설정
		m_pmanager_binder = pmanagerBinder;
	}
}

void net::io::Ibinder::unregister_binder() noexcept
{
	SCOPED_LOCK(m_lockable_manager_binder)
	{
		// check)
		RETURN_IF(m_pmanager_binder == nullptr);

		// 1) unregister한다.
		m_pmanager_binder->leave(this);

		// 3) reset한다.
		m_pmanager_binder = nullptr;
	}
}

bool net::io::Ibinder::bind( u_short _port)
{
	// check) p_Port는 IPPORT_RESERVED번 이하는 사용하지 않는 것이 좋습니다. 
	//         Reservice된 이하의 포트을 요구할 경우 경고한다.
	CGASSERT_ERROR(_port == 0 || _port > IPPORT_RESERVED);

	// 1) bind한다.
	return	process_request_bind({ net::ip::address_v4::any(), _port });
}
