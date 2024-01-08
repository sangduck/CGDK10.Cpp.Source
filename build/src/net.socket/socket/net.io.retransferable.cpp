#include "pch.h"

using namespace CGDK;


//net::io::NRetransferable::NRetransferable() : 
//	m_list_retransfer(),
//	m_serialTransferQueued(0), 
//	m_serialTransferSended(0)
//{
//}
//
//net::io::NRetransferable::~NRetransferable() noexcept
//{
//}
//
//bool net::io::NRetransferable::send_reliable(const shared_buffer& _buffer)
//{
//	// 1) shared_buffer를 만든다.
//	shared_buffer	buf_temp = alloc_shared_buffer(_buffer.len + 16);
//	buf_temp.append<uint32_t>(buf_temp.len + 16);
//	buf_temp.append<uint32_t>(eMESSAGE::NETWORK::SOCKET::RESTORE);
//	buf_temp.append<uint32_t>(eMESSAGE::NETWORK::SOCKET::SEND_MESSAGE_ACK_REQUIRED);
//	buf_temp.append<uint64_t>();
//	buf_temp.append(_buffer);
//
//	// 2) 메시지 Queueing후 전송한다.
//	scoped_lock(m_list_retransfer)
//	{
//		// - shared_buffer의 Serial 번호를 써넣는다.
//		buf_temp.front<uint64_t>(12)	 = m_serialTransferSended;
//
//		// - Push back
//		m_list_retransfer.push_back(buf_temp);
//
//		// - Transfer Serial을 증가시킨다.
//		++m_serialTransferSended;
//
//		// - 전송한다.
//		send(buf_temp);
//	}
//
//	// return)
//	return true;
//}
//
//void net::io::NRetransferable::reset_retransfer()
//{
//	scoped_lock(m_list_retransfer)
//	{
//		m_list_retransfer.clear();
//		m_serialTransferQueued	 = 0;
//		m_serialTransferSended	 = 0;
//		m_serialTransferReceived = 0;
//	}
//}
//
//bool net::io::NRetransferable::RequestRetransfer()
//{
//	// declare) 
//	shared_buffer	bufMessage;
//
//	scoped_lock(m_list_retransfer)
//	{
//		bufMessage	 = Message_REQUEST_RETRANSMISSION(m_serialTransferReceived);
//	}
//}
//
//
//// ****************************************************************************
//// implementation)
//// ----------------------------------------------------------------------------
//public:
//	virtual	result_code			process_message(sMESSAGE& _msg) override;
//};
//
//result_code net::io::NRetransferable::process_message(sMESSAGE& _msg)
//{
//	if(_msg.message == eMESSAGE::SYSTEM::eMESSAGE)
//	{
//		sMESSAGE_NETWORK& msg = (sMESSAGE_NETWORK&)_msg;
//		uint32_t message = msg.buf_message.front<uint32_t>(4);
//
//		switch(message)
//		{
//		// 1) ACK를 요구하는 메시지를 받았을 경우
//		//    - 일반 메시지로 바꾼다.
//		//    - 받은 ACK 메시지를 전달한다.
//		//    - ACK를 전달한다.
//		case	eMESSAGE::NETWORK::SOCKET::SEND_MESSAGE_ACK_REQUIRED:
//				{
//					// - message를 읽어낸다.
//					uint64_t serializeACK = _msg.buf_message.front<uint64_t>(8);
//					
//					// - Reqcieve ACK를 변경한다.
//					m_ack_received	 = serializeACK;
//				
//					// - ACK 메시지를 전송한다.
//					message_SEND_ACK(serializeACK);
//
//					// - 받은 메시지에서 16Byte만큼 빼내어 본내용 메시지로 복원한다. (이후 정상 진행)
//					_msg.buf_message.extract(16);
//				}
//				break;
//
//		// 2) eMESSAGE::NETWORK::SOCKET::ACK 메시지가 전달되어 오면 그에 해당하는 메시지를 빼낸다.
//		case	eMESSAGE::NETWORK::SOCKET::ACK:
//				scoped_lock(m_list_retransfer)
//				{
//					// - 받은 ACK를 읽어낸다.
//					auto ACKSended = _msg.buf_message.front<uint64_t>(8)+1;
//
//					// check) 만약 받은 ACKSended가 현재 ACKSended보다 크면 재전송의 reset 처리를 한다.
//					if(ACKSended<m_ack_sended)
//					{
//					}
//
//					// - 받은 ACK까지 전송 메시지 Queue에서 메시지를 제거한다.
//					for (auto  i=ACKSended-m_ack_sended; countAcked > 0; --countAcked)
//					{
//						m_list_retransfer.pop_front();
//					}
//
//					// - 전송한 ACK를 변경한다.
//					m_ack_sended = ACKSended;
//				}
//
//				return eRESULT::DONE;
//
//		// 3) 재전송을 초기화하는 메시지를 받았을 경우.
//		case	eMESSAGE::NETWORK::SOCKET::REQUEST_INIT_RETRANSMISSION:
//				{
//					net::io::restore_client;
//					net::io::restore_server;
//
//					// - 일단 Retransmission Source ID를 확인한다.
//
//					// - 일단 Retransmission Source ID가 같은지 확인한다.
//
//					// - Retransmission Server ID가 같으면...
//					// - Retransmission Client ID에 해당하는 IRetransferable 객체가 있는지 찾는다.
//					// - Retransferable 객체를 찾았으면 해당 객체를 설정하고 재전송 복구를 수행한다.
//
//
//
//					ProcssRetransfer();
//				}
//				return eRESULT::DONE;
//		};
//	}
//
//	// return)
//	return on_message(_msg);
//}
//
//void net::io::NRetransferable::process_request_reset_retransfer
//{
//	
//};
//
//shared_buffer net::io::NRetransferable::message_REQUEST_RESTORE(uint64_t _serial_last_received)
//{
//	// 1) shared_buffer를 할당한다.
//	shared_buffer	temp_buffer	 = alloc_shared_buffer(256);
//
//	// 2) 메시지를 작성한다.
//
//
//	// return)
//	return	temp_buffer;
//}
//
//
//shared_buffer net::io::NRetransferable::message_RETRNAMISSION_COMPLETE()
//{
//	// 1) shared_buffer를 할당한다.
//	shared_buffer	temp_buffer = alloc_shared_buffer(256);
//
//	// 2) 메시지를 작성한다.
//	temp_buffer.append<uint32_t>();
//	temp_buffer.append<uint32_t>(eMESSAGE::NETWORK::SOCKET::COMPLETE_IO_RESTORE);
//	temp_buffer.append<uint32_t>(eMESSAGE::NETWORK::SOCKET::RESTORE);
//
//
//	// return)
//	return	temp_buffer;
//}
//
//shared_buffer net::io::NRetransferable::message_SEND_ACK(uint64_t _serial_received)
//{
//	// 1) shared_buffer를 할당한다.
//	shared_buffer	temp_buffer = alloc_shared_buffer(256);
//
//	// 2) 메시지를 작성한다.
//
//
//	// return)
//	return	temp_buffer;
//}
