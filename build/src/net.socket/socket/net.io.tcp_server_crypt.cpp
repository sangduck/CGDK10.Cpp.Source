#include "pch.h"

using namespace CGDK;

void net::io::Ntcp_server_crypt::process_on_connect()
{
	on_connecting();
}

void net::io::Ntcp_server_crypt::process_on_disconnect(uint64_t _disconnect_reason, eSOCKET_STATE _socket_state)
{
	if (_socket_state == eSOCKET_STATE::CERTIFIED)
	{
		this->on_disconnect(_disconnect_reason);
	}
	else
	{
		this->on_fail_connect(_disconnect_reason);
	}
}

result_code net::io::Ntcp_server_crypt::process_message(sMESSAGE& _msg)
{
	// 1) casting to 'sMESSAGE_NETWORK'
	sMESSAGE_NETWORK& message_network = static_cast<sMESSAGE_NETWORK&>(_msg);

	// check) _pBuffer가 nullptr이면 그냥 Return한다.
	RETURN_IF(message_network.buf_message.get_source() == nullptr, eRESULT::BYPASS);

	// 2) process system message
	if(this->socket_state()== eSOCKET_STATE::ESTABLISHED)
	{
		// 1) message를 얻는다.
		uint32_t message = message_network.buf_message.front<uint32_t>(4);

		switch(message)
		{
		// Case) Cryption Info를 받았을 경우.(최초에 Client Info를 받을 때에만 한번 처리된다.)
		case	eMESSAGE::NETWORK::SECURITY::REQUEST_CERTIFICATION:
				{
					this->on_message_NETWORK_SERUCITY_REQUEST_CERTIFICATION(message_network.buf_message);
				}
				return eRESULT::DONE;

		// Case) Decoder info를 받았을 경우.(최초에 Client Info를 받을 때에만 한번 처리된다.)
		case	eMESSAGE::NETWORK::SECURITY::SEND_DECODER_INFO:
				{
					this->on_message_NETWORK_SECURITY_SEND_DECODER_INFO(message_network.buf_message);
				}
				return eRESULT::DONE;

		// Default) 이 외의 message가 왔을 때는 접속을 끊는다.
		default:
				{
					// - disconnect한다.
					this->closesocket();

					// return) 0를 return한다.
					return eRESULT::BYPASS;
				}
		};
	}

	// 3) call 'on_message'
	if(this->on_message(message_network) != eRESULT::BYPASS)
	{
		return eRESULT::DONE;
	}

	// return) message is NOT!! processed
	return eRESULT::BYPASS;
}

void net::io::Ntcp_server_crypt::on_message_NETWORK_SERUCITY_REQUEST_CERTIFICATION(const shared_buffer& _buffer)
{
	// check) _buffer.data_가 nullptr이어서는 안된다.
	CGASSERT_ERROR(_buffer.data() != nullptr)

	// check) _array_buffer->size_가 0이어서는 안된다.
	CGASSERT_ERROR(_buffer.size() != 0)

	// 1) Certify를 확인해서 False이면 끝낸다.
	if(this->process_certify_client_info(_buffer) == false)
	{
		// - 실패하면 접속을 끊어버린다.
		this->closesocket();

		// return)
		return;
	}

	// 2) receive Cryptor를 생성한다.
	this->initialize_encoder();
}

void net::io::Ntcp_server_crypt::on_message_NETWORK_SECURITY_SEND_DECODER_INFO(const shared_buffer& _buffer)
{
	// check) _buffer.data_가 nullptr이어서는 안된다.
	CGASSERT_ERROR(_buffer.data() != nullptr)

	// check) _array_buffer->size_가 0이어서는 안된다.
	CGASSERT_ERROR(_buffer.size() != 0)

	// 1) ServerInfo의 message를 얻어낸다.
	shared_buffer buffer_server_info = _buffer ^ buffer_view{ _buffer.data() + 8, _buffer.size() - 8 - SIZE_OF_CRC };
	
	// 2) Encryption설정화는 과정을 수행한다.
	if(this->process_initialize_encoder(buffer_server_info)==false)
	{
		// - 접속을 끊어버린다.
		this->closesocket();

		// return) 
		return;
	}

	// 3) eSOCKET_STATE::ESTABLISHED 상태일 때만 eSOCKET_STATE::CERTIFIED상태로 변경한다.
	bool is_changed = this->set_socket_state_if(eSOCKET_STATE::ESTABLISHED, eSOCKET_STATE::CERTIFIED);

	// check) socket이 이미 Closed상태라면 돌려보낸다.
	RETURN_IF(is_changed == false);

	// 4) 성공 패킷을 전송한다.
	{
		auto buffer_accept = alloc_shared_buffer(32);

		buffer_accept.append<uint32_t>(8 + SIZE_OF_CRC);
		buffer_accept.append<uint32_t>(eMESSAGE::NETWORK::SECURITY::ACCEPT_CERTIFICATION);
		buffer_accept.append(edc::default_crc.get_crc(buffer_accept));

		this->send(buffer_accept);
	}

	// 5) on_connect()함수를 호출한다.
	this->process_on_connect();
}

