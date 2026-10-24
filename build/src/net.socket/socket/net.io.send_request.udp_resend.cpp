#include "pch.h"

using namespace CGDK;


class net::io::send_request::Nudp_resend::executable_resend : 
	public			schedulable::Iexecutable
{
public:
			explicit executable_resend(net::io::send_request::Nudp_resend* _presend) : m_pObject(_presend) {}
public:
	virtual	intptr_t process_execute( intptr_t, std::size_t) override;
			void	reset_object();
public:
			std::mutex	m_lockable_object;
			net::io::send_request::Nudp_resend*	m_pObject;
};

intptr_t net::io::send_request::Nudp_resend::executable_resend::process_execute( intptr_t, std::size_t)
{	
	// declare) 
	object_ptr<net::io::send_request::Nudp_resend> pexecutor;
				
	SCOPED_LOCK(this->m_lockable_object)
	{
		// - 값을 복사한다.
		pexecutor = this->m_pObject;

		// check) 
		RETURN_IF(pexecutor.empty(), intptr_t(1));
	}

	pexecutor->progress_resend();

	// return) 
	return	intptr_t(0);
}

void net::io::send_request::Nudp_resend::executable_resend::reset_object()
{
	SCOPED_LOCK(this->m_lockable_object)
	{
		this->m_pObject = nullptr;
	}
}

net::io::send_request::Nudp_resend::Nudp_resend() :
	m_size_slinding_window(256),
	m_tick_resend_interval(200),
	m_tick_resend_next(chrono::tick::now())
{
	// 1) Function 설정
	this->m_pexecutable_resend = make_object<executable_resend>(this);

	// 2) shared_buffer를 생성한다.
	this->create_buffers();

	// 3) executor에 붙인다.
	system_executor::register_schedulable(this->m_pexecutable_resend);
}

net::io::send_request::Nudp_resend::~Nudp_resend() noexcept
{
	// 1) 값을 reset한다.
	this->m_pexecutable_resend->reset_object();

	// 2) 재전송 Thread에서 떼낸다.
	system_executor::unregister_schedulable(this->m_pexecutable_resend);

	// 3) 모두 해제하기 위해 reset한다.
	this->destroy_buffers();
}

void net::io::send_request::Nudp_resend::reset_send_request() noexcept
{
	// 1) executor를 설정한다.
	this->m_pexecutable_resend->executor(system_executor::executor());

	// 2) shared_buffer를 지운다.
	this->clear_buffers();
}

void net::io::send_request::Nudp_resend::create_buffers()
{
	// 1) shared_buffer를 생성한다.
	this->m_queue_sent.resize(SEND_QUEUE_SIZE);
}

void net::io::send_request::Nudp_resend::destroy_buffers() noexcept
{
	// 1) shared_buffer를 지운다.
	this->clear_buffers();
}

void net::io::send_request::Nudp_resend::clear_buffers() noexcept
{
	SCOPED_LOCK(this->m_lockable_send)
	{
		// 1) Tick reset
		this->m_tick_resend_next = chrono::tick::now();

		// 2) send
		this->m_serial_send = 0;
		this->m_last_serial_send_acked = 0;
		this->m_last_serial_sent = 0;
		this->m_nSent = 0;

		// 3) 재전송 횟수를 0으로...
		this->m_retransmitted = 0;

		// 4) Sent Queue
		for(int i=0; i<SEND_QUEUE_SIZE; ++i)
		{
			this->m_queue_sent.at(i).powner.reset();
		}

		// 6) SendQueue를 clear
		this->m_list_send_queue.clear();
	}
}

#if defined(_MSC_VER)
	#pragma warning(disable:4100)
#endif

bool net::io::send_request::Nudp_resend::request_send( const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_message, const net::sockaddr& _address)
{
	// check) _buffer.data_가 nullptr이 아닌가?
	CGASSERT(_buffer.data() != nullptr || _buffer.size() == 0, false)


	//-----------------------------------------------------------------
	// 1. Resend를 위한 Stack처리.
	//
	//    - Resend하는 부분은 Command부분에 Resend Flag가 붙어 올 
	//      경우 Queue에 저장시킨 후 ACK신호가 돌아오지 않으면 일정시간 
	//      간격으로 재전송을 수행한다.
	//    - executable의 ProcssCompleteSend의 경우 ACK를 받아 Resend 
	//      Queue에서 재거될 때 호출되어 진다.
	//    - ACK가 돌아오지 않아 재전송될시 그 재전송 횟수는 무한정이지 
	//      않으면 처음에  설정한 횟수를 초과하게 되면 SendFail을 내게 
	//      된다.
	//
	//    * 주의: Resend로 저장하는 이 부분은 반드시 process_send()함수 
	//            전에 설정되어야 한다.
	//            process_send()한 완료로 인해 처리가 꼬일 수도 있기 
	//            때문이다.
	//-----------------------------------------------------------------
	// 1) Reliable Flag가 Check되어 있으면 처리한다. (원본 버퍼는 읽기만 한다.)
	bool bResent = _buffer.size() >= sizeof(uint32_t) * 2 && (_buffer.front<uint32_t>(4) & eMESSAGE::SYSTEM::FLAG_RELIABLE) != 0;

	// Case) Resend일 경우... 
	if(bResent)
	{
		// 2) 재전송 queue에 보관할 복사본을 만든다. (호출자의 원본 버퍼는 수정하지 않는다.)
		auto temp_buffer = alloc_shared_buffer(_buffer.size() + sizeof(uint32_t) * 2);

		// - 원본 내용을 복사한다.
		temp_buffer.append(_buffer.size(), _buffer.data());

		// - Reliable Flag를 제거한다.
		temp_buffer.front<uint32_t>(4) &= (~eMESSAGE::SYSTEM::FLAG_RELIABLE);

		SCOPED_LOCK(this->m_lockable_send)
		{
			// - Packet Serial과 tail Command를 붙인다.
			temp_buffer.append<uint32_t>(m_serial_send);
			temp_buffer.append<uint32_t>(eMESSAGE::NETWORK::REQUEST_ACK);

			// Case) Sliding Window 밖이면 일단 Stack한다.(되도록이면 이곳으로 오면 안된다.)
			if((this->m_last_serial_send_acked + this->m_size_slinding_window) < m_serial_send)
			{
				// declare) 
				SENDED_PACKET temp_send_message;

				// 2) Packet shared_buffer를 설정한다.
				temp_send_message.buffer = temp_buffer;
				temp_send_message.powner = temp_buffer.get_source();

				 //3) Tick count를 설정한다.
				temp_send_message.iSendOut = 16;
				temp_send_message.addr_destination = _address;
				temp_send_message.count_message = _count_message;

				// 4) 리스트에 추가한다.
				this->m_list_send_queue.push_back(temp_send_message);

				// 5) send Packet Serial을 증가시킨다.
				++this->m_serial_send;

				// return) 성공...
				return true;
			}

			// Case) Sliding Window 안에 들어올 경우 바로 send한다.
			uint32_t Offset = this->m_serial_send & SEND_QUEUE_INDEX_MASK;

			// 1) 일단 Packet을 Queueing한다.
			SENDED_PACKET& temp_send_message = this->m_queue_sent.at(Offset);

			// 2) Packet shared_buffer를 설정한다.
			temp_send_message.buffer = temp_buffer;
			temp_send_message.powner = temp_buffer.get_source();

			 //3) Tick count를 설정한다.
			temp_send_message.iSendOut = 16;
			temp_send_message.addr_destination = _address;
			temp_send_message.count_message = _count_message;

			// 4) send Packet Serial을 증가시킨다.
			++this->m_serial_send;

			// 5) 보낸 마지막 Serial을 증가시킨다.
			++this->m_last_serial_sent;

			// 6) Sent된 횟수를 증가시킨다.
			++this->m_nSent;
		}

		// 3) 복사본을 전송한다.
		return this->process_sendable(temp_buffer, temp_buffer.move_out_source(), _count_message, _address);
	}

	// 3) call 'process_sendable'
	return this->process_sendable(_buffer, std::move(_powner), _count_message, _address);
}

#if defined(_MSC_VER)
	#pragma warning(default:4100)
#endif

void net::io::send_request::Nudp_resend::progress_resend()
{
	SCOPED_LOCK(this->m_lockable_send)
	{
		//-----------------------------------------------------------------
		//  1. Retransmit Check
		//-----------------------------------------------------------------
		// Re-send할 때에는 Time-Out으로 계산하지는 않는다. 단지 다만...
		// 재전송횟수로 계산을 하여 재전송횟수가 일정 이상이면 그냥 지워
		// 버린다.
		// 1) 현재 Tick을 구한다.
		auto tick_now = chrono::tick::now();

		// check) 
		RETURN_IF(tick_now < this->m_tick_resend_next);

		for(uint32_t i = this->m_last_serial_send_acked; i < this->m_last_serial_sent; ++i)
		{
			// declare) Sent Packet
			SENDED_PACKET& temp_sent = this->m_queue_sent.at(i & SEND_QUEUE_INDEX_MASK);

			// check) 이미 ack된 것이라면 다음...
			CONTINUE_IF(temp_sent.powner.empty());

			// check) 이미 ack된 것이라면 다음...
			CONTINUE_IF((tick_now-temp_sent.tick_last_send) < this->m_tick_resend_interval);

			// 1) Counter를 줄인다.
			if(temp_sent.iSendOut == 0)
			{
				// - free해버린다.
				this->ack(i, false);

				// - 다음...
				continue;
			}

			// 2) SendOut을 줄인다.
			--temp_sent.iSendOut;

			// 3) 전송한다.
			auto result = this->process_sendable(temp_sent.buffer, object_ptr<Ireferenceable>(temp_sent.powner), 1, temp_sent.addr_destination);

			// check) 전송 제대로 했나???
			if(result == false)
			{
				// - free해버린다.
				this->ack(i, false);

				// - 다음...
				continue;
			}

			// - Retransmitted개수를 증가시킨다.
			++this->m_retransmitted;

			// - 마지막 재전송한 시간을 Update한다.
			temp_sent.tick_last_send = tick_now;
		}

		// 3) Next Tick 계산.
		this->m_tick_resend_next = chrono::tick::now() + this->m_tick_resend_interval;
	}
}

bool net::io::send_request::Nudp_resend::ack( uint32_t p_dwPacketSerial, bool /*_result*/)
{
	// declare) 
	SENDED_PACKET* psent_packet = nullptr;

	SCOPED_LOCK(this->m_lockable_send)
	{
		// check) 마지막 SendAck보다 작으면 그냥 끝낸다.
		RETURN_IF(p_dwPacketSerial < this->m_last_serial_send_acked, true);

		// check) 마지막 SendAck보다 작으면 그냥 끝낸다.
		RETURN_IF(p_dwPacketSerial > this->m_last_serial_sent, true);


		//-----------------------------------------------------------------
		//  1. ack한만큼 전송한다.
		//-----------------------------------------------------------------
		// declare) Sent Packet
		SENDED_PACKET& temp_sent = this->m_queue_sent.at(p_dwPacketSerial & SEND_QUEUE_INDEX_MASK);

		// check)
		RETURN_IF(temp_sent.powner.empty(), false);

		// 1) 해제한다.
		temp_sent.powner.reset();

		// 2) ack를 당긴다.
		while(this->m_last_serial_sent>m_last_serial_send_acked)
		{
			BREAK_IF(this->m_queue_sent.at(this->m_last_serial_send_acked & SEND_QUEUE_INDEX_MASK).powner.exist());

			++this->m_last_serial_send_acked;
		}

		// 4) Sent된 개수를 줄인다.
		--this->m_nSent;


		//-----------------------------------------------------------------
		//  2. 새 message을 하나 더 send한다.
		//-----------------------------------------------------------------
		if(this->m_list_send_queue.empty() == false)
		{
			// - Queuing된 것 하나를 얻는다.
			SENDED_PACKET& tempSentS = this->m_list_send_queue.front();

			// declare) Sent Packet
			SENDED_PACKET& tempSentD = this->m_queue_sent.at(m_last_serial_sent & SEND_QUEUE_INDEX_MASK);

			// - 
			tempSentD = tempSentS;
			tempSentD.tick_last_send = chrono::tick::now();

			// - ...
			++this->m_last_serial_sent;

			// 
			psent_packet = &tempSentD;

			this->m_list_send_queue.pop_front();
		}
	}


	//-----------------------------------------------------------------
	//  3. 전송하기.
	//-----------------------------------------------------------------
	if(psent_packet != nullptr)
	{
		// Declare) 
		bool result;

		// 1) 전송하기...
		result = this->process_sendable(psent_packet->buffer, object_ptr<Ireferenceable>(psent_packet->powner), 1, psent_packet->addr_destination);

		// check) 전송에 실패했다.
		RETURN_IF(result == false, false);
	}


	// return) 성공!!
	return true;
}
