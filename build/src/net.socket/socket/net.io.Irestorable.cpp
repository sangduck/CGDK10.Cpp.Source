#include "pch.h"

using namespace CGDK;


bool net::io::Irestorable::send_reliable(const shared_buffer& _buffer)
{
	// check) _buffer.size_이 0이면 안됀다.
	CGASSERT_ERROR(_buffer.size() != 0);

	// check) _buffer.size_이 0이면 안됀다.
	RETURN_IF(_buffer.size() == 0, false);

	// declare) message의 길이
	const std::size_t SIZE_OF_MESSAGE_HEAD = sizeof(uint32_t) * 3 + sizeof(RESTORE_SERVER_ID) + sizeof(RESTORE_CLIENT_ID) + sizeof(SEQUENCE_ID);

	// 1) shared_buffer를 만든다.
	shared_buffer buf_temp = alloc_shared_buffer(_buffer.size() + SIZE_OF_MESSAGE_HEAD);
	buf_temp.append<uint32_t>(static_cast<uint32_t>(_buffer.size() + SIZE_OF_MESSAGE_HEAD));
	buf_temp.append<uint32_t>(eMESSAGE::NETWORK::SOCKET::RESTORE);
	buf_temp.append<uint32_t>(eMESSAGE::NETWORK::SOCKET::SEND_MESSAGE_ACK_REQUIRED);
	buf_temp.append<RESTORE_SERVER_ID>(this->m_id_restore_server);
	buf_temp.append<RESTORE_CLIENT_ID>(this->m_id_restore_client);
	buf_temp.append<SEQUENCE_ID>();
	buf_temp.append(_buffer.size(), _buffer.data());

	// 2) 메시지 Queueing후 전송한다.
	scoped_lock(this->m_list_sending)
	{
		// check) socket상태가 전송 불가상태면 여기서 그냥 끝냄.
		RETURN_IF(this->m_flag_suspended == false && this->socket_state() < eSOCKET_STATE::SYN, false)

		// - Sequence ID를 계산한다.
		SEQUENCE_ID	idSequence = this->m_ack_sended + this->m_list_sending.size() + 1;

		// - shared_buffer의 Serial 번호를 써넣는다.
		buf_temp.front<SEQUENCE_ID>(SIZE_OF_MESSAGE_HEAD-sizeof(SEQUENCE_ID)) = idSequence;
	
		// - Push back
		this->m_list_sending.push_back(buf_temp);

		// check) socket상태가 전송 불가상태면 여기서 그냥 끝냄.
		RETURN_IF(this->m_flag_suspended==true, true);
	
		// - 전송한다.
		this->send(std::move(buf_temp));
	}

	// return)
	return true;
}

bool net::io::Irestorable::request_io_restore()
{
	// 1) Restore를 위한 메시지를 전송함.
	this->send(message_REQUEST_IO_RESTORE());

	// return) 
	return true;
}

result_code net::io::Irestorable::process_message(sMESSAGE& _msg)
{
	// declare) 
	result_code result = eRESULT::BYPASS;

	if (is_network_message(_msg.message))
	{
		sMESSAGE_NETWORK& msg_network = (sMESSAGE_NETWORK&)_msg;
		uint32_t message = msg_network.buf_message.front<uint32_t>(8);

		switch (message)
		{
		// 1) ACK를 요구하는 메시지를 받았을 경우
		case	eMESSAGE::NETWORK::SOCKET::SEND_MESSAGE_ACK_REQUIRED:
				result = this->process_message_SEND_MESSAGE_ACK_REQUIRED(msg_network);
				break;

		// 2) eMESSAGE::NETWORK::SOCKET::ACK 메시지가 전달되어 오면 그에 해당하는 메시지를 빼낸다.
		case	eMESSAGE::NETWORK::ACK:
				result = this->process_message_ACK((sMESSAGE_NETWORK&)_msg);
				break;

		// 3) RESPONSE_IO_RESTORE 메시지를 받았을 경우
		//    -REQUEST_IO_RESTORE를 한 결과를 전달받는다.
		case	eMESSAGE::NETWORK::SOCKET::RESPONSE_IO_RESTORE:
				result = this->process_message_RESPONSE_IO_RESTORE(msg_network);
				break;

		// 4) COMPLETE_IO_RESTORE 메시지를 받았을 경우
		case	eMESSAGE::NETWORK::SOCKET::COMPLETE_IO_RESTORE:
				result = this->process_message_COMPLETE_IO_RESTORE(msg_network);
				break;
		};

		// check) result가 0이 아니면 result를 리턴한다.
		RETURN_IF(result != eRESULT::BYPASS, result)

		// return)
		result = this->on_message(msg_network);
	}

	// return)
	return result;
}

result_code net::io::Irestorable::process_message_SEND_MESSAGE_ACK_REQUIRED(sMESSAGE_NETWORK& _msg)
{
	//-----------------------------------------------------------------
	// 설명) [SEND_MESSAGE_ACK_REQUIRED] 메시지 전송
	//
	//-----------------------------------------------------------------
	// check) 메시지의 길이를 확인한다.


	// 1) 데이터를 읽어들이기 위한 임시 버퍼
	auto buf_recv = _msg.buf_message + offset(12);

	// 2) 메시지의 데이터들을 읽어들인다.
	auto id_restore_server = buf_recv.extract<RESTORE_SERVER_ID>();
	auto id_restore_client = buf_recv.extract<RESTORE_CLIENT_ID>();

	// check) RestoreServer와 RestoreClient가 동일하지 않으면 안됀다.
	CGASSERT_ERROR(id_restore_server == this->m_id_restore_server && id_restore_client == this->m_id_restore_client);

	// check) RestoreServer와 RestoreClient가 동일한지 확인한다. (아니면 그냥 0을 리턴)
	RETURN_IF(id_restore_server != this->m_id_restore_server || id_restore_client != this->m_id_restore_client, 0);

	// 3) ack 값을 읽어들인다.
	auto ackReceived = buf_recv.extract<SEQUENCE_ID>();

	// check) 
	CGASSERT_ERROR(this->m_ack_received == ackReceived);

	scoped_lock(this->m_list_sending)
	{
		// 4) Reqcieve ACK를 변경한다.
		this->m_ack_received = ackReceived+1;
	}

	// 5) ACK 메시지를 전송한다.
	this->send(message_SEND_ACK(ackReceived));

	// declare) 
	constexpr std::size_t MESSAGE_SIZE = std::size_t(12) + sizeof(RESTORE_SERVER_ID) + sizeof(RESTORE_CLIENT_ID) + sizeof(SEQUENCE_ID);

	// 6) 받은 메시지에서 16Byte만큼 빼내어 본내용 메시지로 복원한다. (이후 정상 진행)
	auto result_buf = _msg.buf_message.extract(size(MESSAGE_SIZE));

	// check) shared_buffer의 길이가 다르면 exception을 던진다.
	THROW_IF(result_buf.size() != MESSAGE_SIZE, throwable::failure(eRESULT::INVALID_MESSAGE_LENGTH));

	// check) shared_buffer의 길이가 0이면 Exception을 던진다.
	THROW_IF(_msg.buf_message.size()==0, throwable::failure(eRESULT::INVALID_MESSAGE_LENGTH));

	// return) 여기서는 반드시 0을 리턴해야 한다. 정상적인 메시지 진행은 계속 되어야 한다.b
	return eRESULT::BYPASS;
}

result_code	net::io::Irestorable::process_message_ACK(sMESSAGE_NETWORK& _msg)
{
	//-----------------------------------------------------------------
	// 설명) [ACK] 메시지 전송 메시지
	//
	//-----------------------------------------------------------------
	// check) 메시지의 길이를 확인한다.


	// 1) 데이터를 읽어들이기 위한 임시 버퍼
	auto buf_recv = _msg.buf_message + offset(12);

	// 2) 메시지의 데이터들을 읽어들인다.
	auto id_restore_server = buf_recv.extract<RESTORE_SERVER_ID>();
	auto id_restore_client = buf_recv.extract<RESTORE_CLIENT_ID>();

	// check) RestoreServer와 RestoreClient가 동일한지 확인한다. (아니면 그냥 0을 리턴)
	RETURN_IF(id_restore_server != this->m_id_restore_server || id_restore_client != this->m_id_restore_client, 0);

	// 3) ack 값을 읽얻르인다.
	auto ack_sended = buf_recv.extract<SEQUENCE_ID>();

	scoped_lock(this->m_list_sending)
	{
		// 2) 받은 ACK까지 전송 메시지 Queue에서 메시지를 제거한다.
		if (ack_sended > this->m_ack_sended)
		{
			for (auto i = ack_sended - this->m_ack_sended; i > 0; --i)
			{
				this->m_list_sending.pop_front();
			}
		}

		// - 전송한 ACK를 변경한다.
		this->m_ack_sended = ack_sended;
	}

	// return) 
	return eRESULT::DONE;
}

result_code	net::io::Irestorable::process_message_RESPONSE_IO_RESTORE(sMESSAGE_NETWORK& _msg)
{
	//-----------------------------------------------------------------
	// 설명) [RESPONSE_IO_RESTORE](복구 응답) 메시지
	//
	//-----------------------------------------------------------------
	// check) message의 길이를 확인한다.


	// 1) 데이터를 읽어들이기 위한 임시 버퍼
	auto buf_recv = _msg.buf_message + offset(12);

	// 2) 메시지의 데이터들을 읽어들인다.
	auto id_restore_server = buf_recv.extract<RESTORE_SERVER_ID>();
	auto id_restore_client = buf_recv.extract<RESTORE_CLIENT_ID>();

	// 3) RESTORE_SERVER_ID와 RESTORE_CLIENT_ID를 갱신해 넣는다.
	this->m_id_restore_server = id_restore_server;
	this->m_id_restore_client = id_restore_client;

	// 4) Object Result를 얻는다.
	auto result = buf_recv.extract<eRESULT>();
	auto ackReceived = buf_recv.extract<SEQUENCE_ID>();

	// 5) ResponseIORestore를 호출
	this->process_response_io_restore(result, ackReceived);

	// return) 
	return eRESULT::DONE;
}

result_code	net::io::Irestorable::process_message_COMPLETE_IO_RESTORE(sMESSAGE_NETWORK& _msg)
{
	//-----------------------------------------------------------------
	// 설명) [COMPLETE_IO_RESTORE](복구 완료) 메시지
	//
	//-----------------------------------------------------------------
	// check) message의 길이를 확인한다.

	// 1) 데이터를 읽어들이기 위한 임시 버퍼
	auto buf_recv = _msg.buf_message + offset(12);

	// 2) 메시지의 데이터들을 읽어들인다.
	auto id_restore_server = buf_recv.extract<RESTORE_SERVER_ID>();
	auto id_restore_client = buf_recv.extract<RESTORE_CLIENT_ID>();

	// check) RestoreServer와 RestoreClient가 동일한지 확인한다. (아니면 그냥 0을 리턴)
	RETURN_IF(id_restore_server != this->m_id_restore_server || id_restore_client != this->m_id_restore_client, 0);

	// 3) Restored된 것인지 확인한다.
	auto result = buf_recv.extract<eRESULT>();

	// 4) 복구가 완료되었음을 통보한다. (ackReceived2가 0이 아니면 복구를 의미한다.)
	try
	{
		this->process_complete_io_restore(result);
	}
	catch(...)
	{
		// - reset한다.


		// reraise) 
		throw;
	}

	// return) 
	return eRESULT::DONE;
}

void net::io::Irestorable::process_reset_io_restorable() noexcept
{
	scoped_lock(this->m_list_sending)
	{
		this->m_flag_suspended = false;
		this->m_ack_received = 0;
		this->m_ack_sended = 0;
		this->m_list_sending.clear();
	}
}

result_code net::io::Irestorable::process_get_restore_result(const RESTORE_SERVER_ID& _id_restoreserver, const RESTORE_CLIENT_ID& _id_restoreclient, SEQUENCE_ID _ack_received, SEQUENCE_ID _ack_sended)
{
	// check) m_flag_suspended가 false면 SUSPEND가 아니다.
	RETURN_IF(this->m_flag_suspended == false, eRESULT::RESET);

	// check) _ack_received와 iackSended가 모두 0이면 최초 접속이므로 eRESULT::RESET를 리턴한다.
	RETURN_IF(_ack_received == 0 && _ack_sended == 0, eRESULT::RESET);

	// check) Restore Server ID가 다르면 eRESULT::RESET를 리턴한다.
	RETURN_IF(this->m_id_restore_server != _id_restoreserver, eRESULT::RESET);

	// check) 여기까지 왔다면 Restore Client ID가 emptry는 아니어야 한다.
	RETURN_IF(_id_restoreclient.empty(), eRESULT::RESET);

	// check) 상대의 Other._ack_received가 this.ack_sended보다 적어야한다.
	RETURN_IF(_ack_received <= this->m_ack_sended || _ack_received>(this->m_ack_sended + this->m_list_sending.size() + 1), eRESULT::RESET);

	// check) 먼저 Other._ack_sended가 this.ack_sended보다 적어야한다.
	RETURN_IF(this->m_ack_received <= _ack_sended, eRESULT::RESET);

	// return) 모두 통과했으면 RESTORE처리를 수행한다.
	return eRESULT::RESTORE;
}

shared_buffer net::io::Irestorable::message_REQUEST_IO_RESTORE()
{
	// declare)
	constexpr int SIZE_OF_MESSAGE = sizeof(uint32_t) * 2 + sizeof(RESTORE_SERVER_ID) + sizeof(RESTORE_CLIENT_ID) + sizeof(SEQUENCE_ID) * 2;

	// 1) shared_buffer를  할당받는다.
	auto buf_temp = alloc_shared_buffer(SIZE_OF_MESSAGE);

	// 2) message head를 작성한다.
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::NETWORK::SOCKET::REQUEST_IO_RESTORE);

	// 3) Restorable이 존재하면 그 ID 정보를 전달한다.
	buf_temp.append<RESTORE_SERVER_ID>(this->m_id_restore_server);
	buf_temp.append<RESTORE_CLIENT_ID>(this->m_id_restore_client);
	buf_temp.append<SEQUENCE_ID>(this->m_ack_received);
	buf_temp.append<SEQUENCE_ID>(this->m_ack_sended);

	// check)
	CGASSERT_ERROR(SIZE_OF_MESSAGE == buf_temp.size());

	// 4) message의 길이를 써넣는다.
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

	// return)
	return buf_temp;
}

shared_buffer net::io::Irestorable::message_RESPONSE_IO_RESTORE(result_code _result)
{
	// declare)
	constexpr int SIZE_OF_MESSAGE = sizeof(uint32_t) * 3 + sizeof(RESTORE_SERVER_ID) + sizeof(RESTORE_CLIENT_ID) + sizeof(result_code) + sizeof(SEQUENCE_ID);

	// 1) shared_buffer를  할당받는다.
	auto buf_temp = alloc_shared_buffer(SIZE_OF_MESSAGE);

	// 2) message head를 작성한다.
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::NETWORK::SOCKET::RESTORE);
	buf_temp.append<uint32_t>(eMESSAGE::NETWORK::SOCKET::RESPONSE_IO_RESTORE);

	// 3) RESTORE_SERVER_ID와 RESTORE_CLIENT_ID
	buf_temp.append<RESTORE_SERVER_ID>(this->m_id_restore_server);
	buf_temp.append<RESTORE_CLIENT_ID>(this->m_id_restore_client);

	// 4) Result
	buf_temp.append<result_code>(_result);

	// 5) Restorable이 존재하면 그 ID 정보를 전달한다.
	buf_temp.append<SEQUENCE_ID>(this->m_ack_received);

	// check)
	CGASSERT_ERROR(SIZE_OF_MESSAGE == buf_temp.size());

	// 6) message의 길이를 설정한다.
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

	// return)
	return buf_temp;
}

shared_buffer net::io::Irestorable::message_RESTORE_COMPLETE(result_code _result)
{
	// declare)
	constexpr int SIZE_OF_MESSAGE = sizeof(uint32_t) * 3 + sizeof(RESTORE_SERVER_ID) + sizeof(RESTORE_CLIENT_ID) + sizeof(result_code);

	// 1) shared_buffer를  할당받는다.
	auto buf_temp = alloc_shared_buffer(SIZE_OF_MESSAGE);

	// 2) message head를 작성한다.
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::NETWORK::SOCKET::RESTORE);
	buf_temp.append<uint32_t>(eMESSAGE::NETWORK::SOCKET::COMPLETE_IO_RESTORE);

	// 3) RESTORE_SERVER_ID와 RESTORE_CLIENT_ID
	buf_temp.append<RESTORE_SERVER_ID>(this->m_id_restore_server);
	buf_temp.append<RESTORE_CLIENT_ID>(this->m_id_restore_client);

	// 5) Restorable이 존재하면 그 ID 정보를 전달한다.
	buf_temp.append<result_code>(_result);

	// check)
	CGASSERT_ERROR(SIZE_OF_MESSAGE == buf_temp.size());

	// 6) message의 길이를 설정한다.
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

	// return)
	return buf_temp;
}

shared_buffer net::io::Irestorable::message_SEND_ACK(SEQUENCE_ID _ack_received)
{
	// declare)
	constexpr int SIZE_OF_MESSAGE = sizeof(uint32_t) * 3 + sizeof(RESTORE_SERVER_ID) + sizeof(RESTORE_CLIENT_ID) + sizeof(SEQUENCE_ID);

	// 1) shared_buffer를  할당받는다.
	auto buf_temp = alloc_shared_buffer(SIZE_OF_MESSAGE);

	// 2) message head를 작성한다.
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::NETWORK::SOCKET::RESTORE);
	buf_temp.append<uint32_t>(eMESSAGE::NETWORK::ACK);

	// 3) RESTORE_SERVER_ID와 RESTORE_CLIENT_ID
	buf_temp.append<RESTORE_SERVER_ID>(this->m_id_restore_server);
	buf_temp.append<RESTORE_CLIENT_ID>(this->m_id_restore_client);

	// 4) ACK 정보
	buf_temp.append<SEQUENCE_ID>(_ack_received);

	// check) 
	CGASSERT_ERROR(SIZE_OF_MESSAGE == buf_temp.size());

	// 5) message의 길이를 써넣는다.
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

	// return)
	return buf_temp;
}
