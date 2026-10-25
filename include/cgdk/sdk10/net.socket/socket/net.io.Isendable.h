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
// net::io::Isendable
//
// 1. net::io::Isendable은~
//    1) 데이터를 전송을 I/O함수를 사용하여 구현한 저수준 처리를 정의하는 함수.
//    2) process_send()는 데이터의 전송을 요청하는 처리 과정을 정의하는 함수.
//    3) process_complete_sendable()는 데이터의 전송이 완료되었을 때 호출되어 처리
//       과정을 정의하는 함수이다.
//
//-----------------------------------------------------------------------------
class net::io::Isendable : virtual public Ireferenceable
{
public:
	//! @brief 전송 처리 과정을 정의하는 함수. @param _array_buffer 전송할 버퍼 배열 @param _count_buffer 버퍼의 개수 @param _powner 전송 중 참조계수를 유지할 객체 전송 중일 때에는 참조계수가 1 증가하며 전송이 완료되면 1 감소시킨다. @param _paddress 전송할 주소(UDP전용)  @return true 성공  @return false 실패
	virtual	bool				process_sendable( const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_message, const net::sockaddr& _address) PURE;
	//! @brief 전송이 완료되었을 때 처리를 정의하는 함수.
	virtual	void				process_complete_sendable(uintptr_t _result, std::size_t _transferred, std::size_t _count_message) PURE;

#if defined(_SYS_EPOLL_H)
	virtual	uint32_t			process_recover_sendable( intptr_t _event, std::size_t _param) PURE;
	//! @brief EPOLLERR : reads the error queue of the socket (MSG_ERRQUEUE). @return true the event was MSG_ZEROCOPY notifications only, or stale (the error queue empty and no SO_ERROR : read already) : handled, not an error @return false an error of the socket (the caller closes it)
	virtual	bool				process_complete_error_queue() noexcept { return false; }
	//! @brief io_uring completion : this sendable waits for the socket buffer with the SEND op of the io_context instead of EPOLLOUT (sendable::Ntcp_async_gather). false : readiness only
	virtual	bool				is_io_completion_supported() const noexcept { return false; }
	//! @brief the lock the direct send system calls are made under (the close swaps the handle under it after process_get_lockable_sending()). none : nullptr
	virtual	lock_object*		process_get_lockable_send_call() noexcept { return nullptr; }
#endif
	//! @brief socket을 닫기 전에 부른다. 모아 둔 송신(send_defer : 그룹 락 안 / send cork)이 있으면 어느 thread가 모은 것이든 지금 보낸다.
	virtual	void				process_flush_deferred_send_on_close() noexcept {}

	//! @brief 송신 lock (close가 handle을 바꿀 때 잡는다 : 송신은 같은 lock 안에서 handle을 읽고 send한다). 없으면 nullptr
	virtual	lock_object*		process_get_lockable_sending() noexcept { return nullptr; }
	//! @brief 초기화 될 때 호출되는 함수.
	virtual	void				process_reset_sendable() noexcept PURE;
};

#if defined(_SYS_EPOLL_H)
//-----------------------------------------------------------------------------
//
// net::io::send_cork (epoll)
//
// 1. socket이 받은 message들을 처리하는 동안(send_cork 객체의 범위) 처리하는 thread가
//    그 socket에 보내는 것은 바로 보내지 않고 sendable이 모아 두었다가
//    범위가 끝날 때 한번에(sendmsg 한번) 보낸다.
//    (한번에 받은 여러 message의 응답을 message마다 send하지 않는다 : 시스템콜을 줄인다)
// 2. 이 thread가 이 socket에 보내는 것만 모은다. 다른 thread나 다른 socket의 송신은 그대로다.
//    (모으는 동안 다른 thread가 보낸 것은 그 뒤에 쌓였다가 함께 보내진다 : 순서는 유지된다)
// 3. 범위 안에서 socket을 닫으면 닫기 전에 모아 둔 것을 먼저 보낸다. (send 후 closesocket)
//    exception으로 범위를 벗어날 때도 모아 둔 것은 보낸다.
// 4. 모은 것이 많거나 오래되면 범위가 끝나기 전에 보낸다. (sendable이 정한다)
// 5. 모으는 sendable : net::io::sendable::Ntcp_async_gather, Ntcp_async_gather_buffered
//    (다른 sendable은 모으지 않는다 : 바로 보낸다)
// 6. 모으는 socket은 send_defer에 설정한다. (CGDK::send_defer : 그룹 락 안의 송신과 같은 목록에 모은다)
//    - 범위 안에서 그룹 락을 잡고 이 socket에 보낸 것도 이 범위가 끝날 때 보낸다.
//
//-----------------------------------------------------------------------------
namespace net::io
{
	class send_cork
	{
	public:
		explicit send_cork(const Isocket* _psocket) noexcept
		{
			// - send_defer에 모으는 socket을 설정한다. (바깥 범위에서 send_defer로 모아 둔 것은 먼저 보낸다)
			this->m_psocket_before = send_defer::begin_cork(s_enable.load(std::memory_order_relaxed) ? _psocket : nullptr);
		}
		~send_cork() noexcept
		{
			// - send_defer로 모아 둔 것을 보낸다.
			send_defer::end_cork(this->m_psocket_before);
		}
		send_cork(const send_cork&) = delete;
		send_cork& operator=(const send_cork&) = delete;

	public:
		//! @brief 이 thread가 지금 모으고 있는 socket (nullptr : 없음)
		[[nodiscard]] static const Isocket* corking_socket() noexcept { return static_cast<const Isocket*>(send_defer::cork_socket()); }
		//! @brief 모아 둔 것을 지금 보낸다. (이 thread가 send_defer로 모아 둔 것 모두)
		static void			flush() noexcept { send_defer::flush(); }
		//! @brief 사용 여부를 설정한다. (기본 사용)
		static void			enable(bool _enable = true) noexcept { s_enable.store(_enable, std::memory_order_relaxed); }
		[[nodiscard]] static bool is_enabled() noexcept { return s_enable.load(std::memory_order_relaxed); }

	private:
				const void*		m_psocket_before{ nullptr };
		static inline std::atomic<bool> s_enable{ true };
	};
}
#endif


}