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
//-----------------------------------------------------------------------------
//
// net::io::sendable::Ntcp_sync
//
//    1) TCP용 send로 동기식 i/o 방식으로 수행한다.
//	  2) 가장 기본적인 형태인 블로킹 전송을 수행한다.
// 	     - linux
// 	       send함수를 사용해 전송한다.
// 	     - windows
// 	       Overlapped i/o를 사용한다.
//	   	   Overlapped를 사용한 비동기 송수신의 경우 적은 데이터를 빈번하지 않게 전송할 경우 오히려 오버헤드가 더 클 수 있다.
//		   이 sendable 클래스는 전송 과정에서 복잡한 처리 과정이 일절 없기 때문에 전송 트래픽이 높지 않다면 오히려 더 좋은 효율을 낼 수 있다.
//		   전송 상태에 대한 아무런 동작도 처리하지 않기 때문에 대용량의 전송에는 비효율적일 수 있으며 전송량이 많으면 접속이 끊어질 수도 있다.
//    3) 간혹 한번씩 송신하는 socket의 경우 이 방법이 효율적일 수 있다.
//
//-----------------------------------------------------------------------------
class net::io::sendable::Ntcp_sync :
	virtual public				net::io::Isendable,
	virtual public				net::io::Iconnectable
{
protected:
	virtual	bool				process_sendable(const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_message, const net::sockaddr& _paddress) override;
	virtual	void				process_complete_sendable(uintptr_t _result, std::size_t _transferred_bytes, std::size_t _transferred_message) override;
	virtual	void				process_reset_sendable() noexcept override;

#if defined(_SYS_EPOLL_H)
	virtual	uint32_t			process_recover_sendable(intptr_t _event, std::size_t _param) override;
#endif

	#if defined(_WINSOCK2API_)
			bool				process_sendable_async(CGSOCKET _handle_socket, const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_message);
	#endif
};


}