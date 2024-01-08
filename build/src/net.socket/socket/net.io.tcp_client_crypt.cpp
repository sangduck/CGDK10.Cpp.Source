#include "pch.h"

using namespace CGDK;


void  net::io::Ntcp_client_crypt::process_on_connect()
{
	// 1) on connecting
	this->on_connecting();

	// 2) 전송한다.
	this->process_request_certification();
}

void  net::io::Ntcp_client_crypt::process_on_disconnect(uint64_t _disconnect_reason, eSOCKET_STATE _socket_state)
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

result_code net::io::Ntcp_client_crypt::process_message(sMESSAGE& _msg)
{
	// 1) casting to sMESSAGE_NETWORK
	sMESSAGE_NETWORK& msg_network = static_cast<sMESSAGE_NETWORK&>(_msg);

	// check) msg_network.buf_message가 nullptr이면 그냥 Return한다.
	RETURN_IF(msg_network.buf_message.get_source().empty(), eRESULT::BYPASS);

	// 2) CRC를 Check한다.
	if(edc::default_crc.get_crc(msg_network.buf_message) != 0)
	{
		// trace) 
		LOG_DEBUG << "(err ) CGDK.net.socket: CRC or message is invalid "sv << edc::default_crc.get_crc(msg_network.buf_message) << " " __CGFUNCTION__ ""sv;

		// - disconnect한다.
		this->closesocket();

		// return) eRESULT::BYPASS를 return한다.
		return eRESULT::BYPASS;
	}

	// 3) message
	if(socket_state()== eSOCKET_STATE::ESTABLISHED)
	{
		// 1) message를 얻는다.
		uint32_t message = msg_network.buf_message.front<uint32_t>(4);

		switch(message)
		{
		// Case) Decoder info를 받았을 경우.(최초에 Client Info를 받을 때에만 한번 처리된다.)
		case	eMESSAGE::NETWORK::SECURITY::SEND_DECODER_INFO:
				{
					this->on_message_NETWORK_SECURITY_SEND_DECODER_INFO(msg_network.buf_message);
				}
				return eRESULT::DONE;

		// Case) 인증을 확인받지 못했을 경우.
		//      - 이런 경우는 Server와 Client가 서로 일치하지 않거나 해서 인증을 받지 못한 경우이다.
		//        OnFailCertify함수를 호출한다.
		case	eMESSAGE::NETWORK::SECURITY::ACCEPT_CERTIFICATION:
				{
					this->on_message_NETWORK_SECURITY_ACCEPT_CERTIFICATION(msg_network.buf_message);
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

	// 4) call 'on_message'
	if(this->on_message(msg_network) != eRESULT::BYPASS)
	{
		return eRESULT::DONE;
	}

	// return) message is NOT!! processed
	return eRESULT::BYPASS;
}

void net::io::Ntcp_client_crypt::process_request_certification()
{
	// 1) shared_buffer를 할당한다.
	auto temp_buffer = alloc_shared_buffer(1024);

	// 2) message를 만든다.
	temp_buffer.append<uint32_t>(8 + SIZE_OF_CRC);
	temp_buffer.append<uint32_t>(eMESSAGE::NETWORK::SECURITY::REQUEST_CERTIFICATION);
	temp_buffer.append(edc::default_crc.get_crc(temp_buffer));

	// 3) 전송한다.
	this->send(temp_buffer);
}

void net::io::Ntcp_client_crypt::on_message_NETWORK_SECURITY_SEND_DECODER_INFO(const shared_buffer& _buffer)
{
	// check) _buffer.data_가 0이어서는 안된다.
	CGASSERT_ERROR(_buffer.data() != nullptr)

	// check) _array_buffer->size_가 0이어서는 안된다.
	CGASSERT_ERROR(_buffer.size() != 0)

	// 1) ServerInfo의 message를 얻어낸다.
	shared_buffer tempServerInfo = _buffer ^ buffer_view(_buffer.data() + 8, _buffer.size() - 8 - SIZE_OF_CRC);
	
	// 2) Encryption설정화는 과정을 수행한다.
	if(this->process_initialize_encoder(tempServerInfo) == false)
	{
		// - 접속을 끊어버린다.
		this->closesocket();

		// return) 
		return;
	}

	// 3) initialize dncoder (여기서 SYSTEM_SEND_DECODER_INFO를 전송한다.)
	this->initialize_encoder();
}

void net::io::Ntcp_client_crypt::on_message_NETWORK_SECURITY_ACCEPT_CERTIFICATION(const shared_buffer& /*_buffer*/)
{
	// 1) eSOCKET_STATE::ESTABLISHED상태일 때만 eSOCKET_STATE::CERTIFIED로 변경한다.
	bool is_changed = this->set_socket_state_if(eSOCKET_STATE::ESTABLISHED, eSOCKET_STATE::CERTIFIED);

	// check) socket의 상태가 변하지 않았다면 그냥 리턴.
	RETURN_IF(is_changed == false);

	// 2) on_connect()를 호출한다.
	this->process_on_connect();
}
