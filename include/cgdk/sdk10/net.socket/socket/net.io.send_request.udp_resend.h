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
// net::io::send_request::Nudp_resend!!!!  ( ` ~`)b
//
// 1. net::io::send_request::Nudp_resend란!
//    1) Reliable UDP send/receive를 구현하기 위한 기반 Class이다.
//    2) 이 Class에서는 전송에 대한 Reliable을 구현하고 있다.
//    3) 이 Class를 바탕으로 Reliable과 ReliableSequenced를 제작한다.
//
// 2. Reliable에 추가된 함수들...
//    1) peer key
//       - peer간의 전송에 있어서 서로 확인을 위한 key를 의미한다.
//       - 잘못전달되거나 여러 다른 전송과의 구분을 위해 peer key를 설정한다.
//       - peer key는 send와 receive key가 있으며 send key는 connect시 상대
//         로 부터 전달받는다.
//
//    2) Re-send Interval
//	     - 일정시간 ACK가 돌아오지 않으면 재전송을 수행한다.  
//         GetReSendInterval()
//         SetReSendInterval(uint32_t p_dwResendInterval)
//
//    3) 기타
//       GetSendSequence()			전송 Sequence를 구한다.
//       GetRetransmittedCount()	재전송한 총 횟수를 구한다.
//		 GetSendQueud()				send를 위해 Queued된 Packet의 수를 얻는 함수.
//
// 3. 주의 사항
//    1) 이 Class은 완성된 것이 아니며 net::io::CReliable과 CReliableSequenced
//       를 사용하도록 한다.
//
//
//-----------------------------------------------------------------------------
#define		SEND_QUEUE_SIZE				1024
#define		SEND_QUEUE_INDEX_MASK		0x3ff

class net::io::send_request::Nudp_resend : 
	virtual public				net::io::Isendable,
	virtual public				net::io::Isend_request,
	virtual public				Ireferenceable
{
protected:
			Nudp_resend();
	virtual	~Nudp_resend() noexcept;

public:
	virtual	bool				request_send(const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_message, const net::sockaddr& _paddress) override;

private:
	virtual	void				reset_send_request() noexcept override;

			struct SENDED_PACKET
			{
			public:
				int							iSendOut;
				chrono::tick::time_point	tick_last_send;
				buffer_view					buffer;
				object_ptr<Ireferenceable>	powner;
				net::sockaddr				addr_destination;
				std::size_t					count_message;
			public:
				SENDED_PACKET() noexcept :iSendOut(0), count_message(0){}
			};

			class executable_resend;

			object_ptr<executable_resend>	m_pexecutable_resend;

			void				progress_resend();

			uint32_t			m_size_slinding_window;			// Sliding Window의 크기

			uint32_t			m_serial_send;					// send Serial
			uint32_t			m_last_serial_send_acked;		// 완료 ack받은 마지막 packet
			uint32_t			m_last_serial_sent;			// 마지막으로 보낸 Serial.

			int					m_nSent;
			std::vector<SENDED_PACKET> m_queue_sent;			// 이미 보낸 Packet을 Array로 저장함.
			circular_list<SENDED_PACKET> m_list_send_queue;		// 아직 보내지 않은 Packet을 보관한 send Queue

	mutable	lock_object			m_lockable_send;				// send Queue용 Critical section

			chrono::tick::duration	 m_tick_resend_interval;
			chrono::tick::time_point m_tick_resend_next;

			int					m_retransmitted;				// 재전송한 횟수.

protected:
			bool				ack( uint32_t p_dwPacketSerial, bool p_bResult=true);

			void				create_buffers();
			void				destroy_buffers() noexcept;
			void				clear_buffers() noexcept;
};


}