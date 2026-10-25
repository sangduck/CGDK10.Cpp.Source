#include "pch.h"

using namespace CGDK;

void  net::io::Ntcp_crypt::process_on_connect()
{
	// 1) call 'on_connecting'
	this->on_connecting();

	// 2) client
	if (this->is_client_socket())
	{
		this->process_request_certification();
	}
}

void net::io::Ntcp_crypt::process_on_disconnect(uint64_t _disconnect_reason, eSOCKET_STATE _socket_state)
{
	if (_socket_state == eSOCKET_STATE::CERTIFIED)
	{
		this->on_disconnect(_disconnect_reason);
	}
	else
	{
		this->process_on_fail_connect(_disconnect_reason);
	}

	// 2) drop the cryptor : a disconnected socket can be returned to the pool and reused
	//    for another connection, so the previous session's cryptor must not survive.
	//    (process_prepare_socket resets it again on reuse. noexcept)
	this->process_reset_cryptor();
}

void net::io::Ntcp_crypt::process_on_fail_connect(uint64_t _disconnect_reason)
{
	this->on_fail_connect(_disconnect_reason);
}


void net::io::Ntcp_crypt::process_certify_client_info(const buffer_view& _pclient_info)
{
	// check) 이미 cryptor가 생성되어 있으면 예외를 던진다.
	THROW_IF(this->get_cryptor().exist(), throwable::failure(eRESULT::NOT_EXIST));

	// 1) make cryptor & get initialize_info
	auto [pcryptor, buf_initialize_info] = this->process_create_cryptor_for_server(_pclient_info);

	// 2) make message and send to client
	this->send(message_NETWORK_SECURITY_ACCEPT_CERTIFICATION(buf_initialize_info));

	// 3) set cryptor (반드시 를 전송 후 Setting 해야 한다. 그렇지 않으면 이 message로 암호화되어 전송될 것이기 때문...)
	this->process_set_cryptor(std::move(pcryptor));

	// 4) set socket statue (CAS : ESTABLISHED일 때만. 그 사이 닫혔으면(CLOSED) 덮어쓰지 않고 끝낸다)
	RETURN_IF(this->set_socket_state_if(eSOCKET_STATE::ESTABLISHED, eSOCKET_STATE::CERTIFIED) == false);

	// 5) call on_connect
	this->on_connect();
}

std::pair<object_ptr<Icryptor>, shared_buffer> net::io::Ntcp_crypt::process_create_cryptor_for_server(const buffer_view& _pclient_info)
{
	switch (this->cryption_type())
	{
	case	eCRYPTION_TYPE::PNCODE:
		return process_create_cryptor_for_server_pncode(_pclient_info);

	default:
		return process_create_cryptor_for_server_none(_pclient_info);
	};
}

std::pair<object_ptr<Icryptor>, shared_buffer> net::io::Ntcp_crypt::process_create_cryptor_for_server_pncode(const buffer_view& /*_pclient_info*/)
{
	// 1) key를 generation 한다.
	auto keys_encrypt = crypt::pn_code::generate_key();
	auto keys_decrypt = crypt::pn_code::generate_key();

	// 2) get cryptor info
	auto pcryptor = crypt::pn_code::make_cryptor(keys_encrypt, keys_decrypt);

	// 3) make message
	auto buf_initialize_info = alloc_shared_buffer(32768);
	buf_initialize_info.append<eCRYPTION_TYPE>(eCRYPTION_TYPE::PNCODE);
	buf_initialize_info.append<crypt::pn_code::key_t>(keys_decrypt);
	buf_initialize_info.append<crypt::pn_code::key_t>(keys_encrypt);

	// return) 
	return { std::move(pcryptor), std::move(buf_initialize_info) };
}

std::pair<object_ptr<Icryptor>, shared_buffer> net::io::Ntcp_crypt::process_create_cryptor_for_server_none(const buffer_view& /*_pclient_info*/)
{
	// 1) make message
	auto buf_initialize_info = alloc_shared_buffer(32);
	buf_initialize_info.append<eCRYPTION_TYPE>(eCRYPTION_TYPE::NONE);

	// return) 
	return { {}, std::move(buf_initialize_info) };
}

void net::io::Ntcp_crypt::process_initialize_cryptor_for_client(const shared_buffer& _buffer)
{
	// 1) get crypt init
	auto buf_crypt_init = _buffer + offset(8);

	// 2) get crypto type
	auto crypt_type = buf_crypt_init.extract<eCRYPTION_TYPE>();

	// declare)
	object_ptr<Icryptor> pcryptor;

	// 3) make cryptor
	switch (crypt_type)
	{
	case	eCRYPTION_TYPE::PNCODE:
		pcryptor = process_initialize_cryptor_for_client_pncode(buf_crypt_init);
		break;

	default:
		pcryptor = process_initialize_cryptor_for_client_none(buf_crypt_init);
		break;
	};

	// 4) set crypto
	this->process_set_cryptor(std::move(pcryptor));
}

object_ptr<Icryptor> net::io::Ntcp_crypt::process_initialize_cryptor_for_client_pncode(buffer_view _buf_initlaizion)
{
	// 1) extract crypto keys
	auto keys_encrypt = _buf_initlaizion.extract<crypt::pn_code::key_t>();
	auto keys_decrypt = _buf_initlaizion.extract<crypt::pn_code::key_t>();

	// 2) get cryptor and return
	return crypt::pn_code::make_cryptor(keys_encrypt, keys_decrypt);
}

object_ptr<Icryptor> net::io::Ntcp_crypt::process_initialize_cryptor_for_client_none(buffer_view /*_buf_initlaizion*/)
{
	return {};
}

result_code net::io::Ntcp_crypt::process_message(sMESSAGE& _msg)
{
	// 1) casting to sMESSAGE_NETWORK
	sMESSAGE_NETWORK& msg_network = static_cast<sMESSAGE_NETWORK&>(_msg);

	// check) msg_network.buf_message가 nullptr이면 그냥 Return한다.
	RETURN_IF(msg_network.buf_message.get_source().empty(), eRESULT::BYPASS);

	// 2) message
	if(this->socket_state() == eSOCKET_STATE::ESTABLISHED)
	{
		// 1) message를 얻는다.
		uint32_t message = msg_network.buf_message.front<uint32_t>(4);

		switch(message)
		{
		// Case) 인증을 확인받지 못했을 경우.
		//      - 이런 경우는 Server와 Client가 서로 일치하지 않거나 해서 인증을 받지 못한 경우이다.
		//        OnFailCertify함수를 호출한다.
		case	eMESSAGE::NETWORK::SECURITY::ACCEPT_CERTIFICATION:
				{
					this->on_message_NETWORK_SECURITY_ACCEPT_CERTIFICATION(msg_network.buf_message);
				}
				return eRESULT::DONE;

		case	eMESSAGE::NETWORK::SECURITY::REQUEST_CERTIFICATION:
				{
					this->on_message_NETWORK_SECURITY_REQUEST_CERTIFICATION(msg_network.buf_message);
				}
				return eRESULT::DONE;


		// default) 이 외의 message가 왔을 때는 접속을 끊는다.
		default:
				{
					// - set disconnect reason
					this->m_disconnect_reason |= 0;

					// - call 'on_fail_connect'
					this->on_fail_connect(m_disconnect_reason);

					// - disconnect한다.
					this->closesocket();
				}
				return eRESULT::BYPASS;
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

void net::io::Ntcp_crypt::process_request_certification()
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

void net::io::Ntcp_crypt::on_message_NETWORK_SECURITY_ACCEPT_CERTIFICATION(const shared_buffer& _buffer)
{
	// 1) eSOCKET_STATE::ESTABLISHED상태일 때만 eSOCKET_STATE::CERTIFIED로 변경한다.
	bool is_changed = this->set_socket_state_if(eSOCKET_STATE::ESTABLISHED, eSOCKET_STATE::CERTIFIED);

	// check) socket의 상태가 변하지 않았다면 그냥 리턴.
	RETURN_IF(is_changed == false);

	// 2) set cryptor
	this->process_initialize_cryptor_for_client(_buffer);

	// - the bytes received with this message after it were taken as plain but the server encrypted them (it sends encrypted right after
	//   this message) : decrypted now, before the next message of them is processed
	this->process_decrypt_received_after(_buffer.data() + _buffer.size());

	// 3) on_connect()를 호출한다. (상태는 1)에서 CAS로 이미 CERTIFIED다 : 다시 쓰면 그 사이의 CLOSED를 덮어쓸 수 있다)
	// 4) on_connect()를 호출한다.
	this->on_connect();
}

void net::io::Ntcp_crypt::on_message_NETWORK_SECURITY_REQUEST_CERTIFICATION(const shared_buffer& _buffer)
{
	// check) _buffer.data_가 nullptr이어서는 안된다.
	CGASSERT_ERROR(_buffer.data() != nullptr)

	// check) _array_buffer->size_가 0이어서는 안된다.
	CGASSERT_ERROR(_buffer.size() != 0)

	try
	{
		// check) 
		CGASSERT_ERROR(!this->is_client_socket());

		// check)
		THROW_IF(this->is_client_socket(), throwable::failure(eRESULT::INVALID_REQUEST));

		// 1) certify client info
		this->process_certify_client_info(_buffer);
	}
	catch (...)
	{
		// - 실패하면 접속을 끊어버린다.
		this->closesocket();
	}
}

shared_buffer net::io::Ntcp_crypt::message_NETWORK_SECURITY_ACCEPT_CERTIFICATION(buffer_view _buf_initialize_info)
{
	// 1) alloc shared_buffer
	auto buf_message = alloc_shared_buffer(8 + sizeof(SIZE_OF_CRC) + _buf_initialize_info.size());

	// 2) message의 head부분을 설정한다.
	buf_message.append<uint32_t>();
	buf_message.append<uint32_t>(eMESSAGE::NETWORK::SECURITY::ACCEPT_CERTIFICATION);

	// 3) append data
	buf_message.append(_buf_initialize_info.size(), _buf_initialize_info.data());

	// 4) message를 마무리한다.(Size를 써넣고 CRC를 붙인다.)
	buf_message.front<uint32_t>() = static_cast<uint32_t>(buf_message.size() + SIZE_OF_CRC);
	buf_message.append<CRC_T>(edc::default_crc.get_crc(buf_message));

	// return) 
	return buf_message;
}
