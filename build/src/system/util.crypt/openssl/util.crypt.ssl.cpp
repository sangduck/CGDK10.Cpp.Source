#include "pch.h"

#include <openssl/rsa.h>
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <thread>


namespace CGDK
{

// internal helpers
namespace
{
	std::string _get_ssl_error_string(int _ssl_error)
	{
		char buffer[256]{};
		auto error = ERR_get_error();
		if (error != 0)
			ERR_error_string_n(error, buffer, sizeof(buffer));
		else
			snprintf(buffer, sizeof(buffer), "ssl error (SSL_get_error:%d)", _ssl_error);
		ERR_clear_error();
		return std::string(buffer);
	}

	// - handshake가 끝나면 socket BIO를 memory BIO로 바꾼다.
	void _attach_memory_bio(crypt::ssl* _pssl)
	{
		// 1) BIO 생성
		auto pbio_receive = BIO_new(BIO_s_mem());
		auto pbio_send = BIO_new(BIO_s_mem());

		// check)
		if (pbio_receive == nullptr || pbio_send == nullptr)
		{
			if (pbio_receive != nullptr) BIO_free(pbio_receive);
			if (pbio_send != nullptr) BIO_free(pbio_send);
			throw throwable::failure(eRESULT::BAD_ALLOC, 0, "fail to create BIO"sv);
		}

		// 2) BIO를 설정한다. (기존 socket BIO는 SSL_set_bio에서 해제된다.)
		SSL_set_bio(_pssl->m_ssl, pbio_receive, pbio_send);

		// 3) BIO 설정
		_pssl->m_pbio_receive = pbio_receive;
		_pssl->m_pbio_send = pbio_send;
	}
}

crypt::ssl::ssl() noexcept :
	m_ssl(nullptr),
	m_pbio_receive(nullptr),
	m_pbio_send(nullptr),
	m_pssl_context()
{
}

crypt::ssl::~ssl() noexcept
{
	this->destroy();
}

void crypt::ssl::set_host_name(std::string_view _host_name)
{
	// check)
	if(this->m_ssl == nullptr)
		throw throwable::failure(eRESULT::NOT_READY, 0, "ssl is not allocated"sv);

	// check)
	if(_host_name.empty())
		return;

	// - NUL로 끝나는 문자열로 복사한다.
	const std::string host_name(_host_name);

	// 1) IP 주소면 IP로 검증한다. (IP는 SNI로 보내지 않는다.)
	if (X509_VERIFY_PARAM_set1_ip_asc(SSL_get0_param(this->m_ssl), host_name.c_str()) == 1)
		return;

	// - IP가 아니므로 생긴 error는 지운다.
	ERR_clear_error();

	// 2) SNI (server name indication)
	if(SSL_set_tlsext_host_name(this->m_ssl, host_name.c_str()) != 1)
		throw throwable::failure(eRESULT::FAIL, 0, _get_ssl_error_string(0));

	// 3) 인증서의 host name 검증
	if(SSL_set1_host(this->m_ssl, host_name.c_str()) != 1)
		throw throwable::failure(eRESULT::FAIL, 0, _get_ssl_error_string(0));
}

void crypt::ssl::initialize_handshake_server(Issl_cryptable* _pcryptable_ssl)
{
	// check)
	CGASSERT_ERROR(this->m_ssl != nullptr);

	// check)
	if(this->m_ssl == nullptr)
		return;

	// check)
	CGASSERT_ERROR(_pcryptable_ssl != nullptr);

	// check)
	if(_pcryptable_ssl == nullptr)
		throw throwable::failure(eRESULT::FAIL);

	// 설명) handshake를 여기서 기다리지 않는다. (비동기 handshake)
	//       - socket BIO 대신 memory BIO를 사용하고 server 상태로 설정한다.
	//       - ClientHello를 받으면 수신 처리(process_receive)에서 handshake를 진행한다.

	// 1) memory BIO로 바꾼다.
	_attach_memory_bio(this);

	// 2) server로 handshake한다.
	SSL_set_accept_state(this->m_ssl);

	// 3) 설정한다.
	_pcryptable_ssl->set_ssl_cryptor(this);
}

void crypt::ssl::initialize_handshake_client(Issl_cryptable* _pcryptable_ssl)
{
	// check)
	CGASSERT_ERROR(this->m_ssl != nullptr);

	// check)
	if(this->m_ssl == nullptr)
		return;

	// check)
	CGASSERT_ERROR(_pcryptable_ssl != nullptr);

	// check)
	if(_pcryptable_ssl == nullptr)
		throw throwable::failure(eRESULT::FAIL);

	// 설명) handshake를 여기서 기다리지 않는다. (비동기 handshake)
	//       - socket BIO 대신 memory BIO를 사용하고 client 상태로 설정한다.
	//       - 접속 후 start_handshake()로 ClientHello를 보내고, 응답은 수신 처리(process_receive)에서 진행한다.
	//       - 서버 인증서 검증(SSL_VERIFY_PEER)에 실패하면 handshake가 실패한다.

	// 1) memory BIO로 바꾼다.
	_attach_memory_bio(this);

	// 2) client로 handshake한다.
	SSL_set_connect_state(this->m_ssl);

	// 3) 설정한다.
	_pcryptable_ssl->set_ssl_cryptor(this);
}

bool crypt::ssl::is_handshake_completed() const noexcept
{
	return this->m_ssl != nullptr && SSL_is_init_finished(this->m_ssl);
}

void crypt::ssl::_do_handshake()
{
	// check) 이미 끝났으면 할 것이 없다.
	if(SSL_is_init_finished(this->m_ssl))
		return;

	// 1) handshake를 진행한다.
	auto result = SSL_do_handshake(this->m_ssl);

	// 2) 아직 진행 중이거나 실패
	if (result != 1)
	{
		auto error_code = SSL_get_error(this->m_ssl, result);

		// - 상대의 데이터를 더 받아야 한다. (memory BIO이므로 WANT_WRITE는 생기지 않는다.)
		if(error_code == SSL_ERROR_WANT_READ || error_code == SSL_ERROR_WANT_WRITE)
			return;

		// - 인증서 검증 실패면 이유를 남긴다.
		auto verify_result = SSL_get_verify_result(this->m_ssl);

		if (!SSL_is_server(this->m_ssl) && verify_result != X509_V_OK)
			throw throwable::failure(eRESULT::FAIL, error_code, std::string("ssl certificate verify failed: ") + X509_verify_cert_error_string(verify_result));

		throw throwable::failure(eRESULT::FAIL, error_code, std::string("ssl handshake failed: ") + _get_ssl_error_string(error_code));
	}

	// 3) (client) 인증서 검증 결과를 한번 더 확인한다.
	if (!SSL_is_server(this->m_ssl) && (SSL_get_verify_mode(this->m_ssl) & SSL_VERIFY_PEER))
	{
		// - 서버 인증서가 있어야 한다.
		X509* pcert = SSL_get1_peer_certificate(this->m_ssl);
		if(pcert == nullptr)
			throw throwable::failure(eRESULT::FAIL, 0, "ssl: no server certificate"sv);
		X509_free(pcert);

		// - 검증 결과
		auto verify_result = SSL_get_verify_result(this->m_ssl);
		if(verify_result != X509_V_OK)
			throw throwable::failure(eRESULT::FAIL, 0, std::string("ssl certificate verify failed: ") + X509_verify_cert_error_string(verify_result));
	}

	// 4) handshake가 끝났으므로 보관했던 평문을 암호화한다.
	this->_flush_pending_send();
}

void crypt::ssl::_flush_pending_send()
{
	// 1) 보관한 평문을 순서대로 암호화한다.
	for (auto& iter : this->m_vector_pending_send)
	{
		buffer_view buf_source = iter;

		while (buf_source.size() != 0)
		{
			auto bytes_write = static_cast<int>(__select_min<std::size_t>(buf_source.size(), static_cast<std::size_t>(INT_MAX)));
			auto result_write = SSL_write(this->m_ssl, buf_source.data(), bytes_write);

			// check)
			if (result_write <= 0)
			{
				auto error_code = SSL_get_error(this->m_ssl, result_write);
				throw throwable::failure(eRESULT::FAIL, error_code, _get_ssl_error_string(error_code));
			}

			buf_source += offset(result_write);
		}
	}

	// 2) clear
	this->m_vector_pending_send.clear();
}

shared_buffer crypt::ssl::_read_send_bio()
{
	// 1) 보낼 데이터의 크기
	auto bytes_pending = BIO_ctrl_pending(this->m_pbio_send);

	// check) 없으면 빈 buffer
	if (bytes_pending == 0)
		return shared_buffer();

	// 2) 크기에 맞게 할당한다.
	auto buf_dest = alloc_shared_buffer(bytes_pending);

	// 3) 읽어들인다.
	std::size_t bytes_read = 0;
	while (bytes_read < bytes_pending)
	{
		auto result_read = BIO_read(this->m_pbio_send, buf_dest.data() + bytes_read, static_cast<int>(bytes_pending - bytes_read));

		// check)
		if(result_read <= 0)
			throw throwable::failure(eRESULT::FAIL, 0, "ssl: fail to read encrypted data"sv);

		bytes_read += static_cast<std::size_t>(result_read);
	}

	// 4) 크기를 설정한다.
	buf_dest.set_size(bytes_read);

	// return)
	return buf_dest;
}

shared_buffer crypt::ssl::start_handshake()
{
	// check)
	if(this->m_ssl == nullptr || this->m_pbio_send == nullptr)
		throw throwable::failure(eRESULT::NOT_READY, 0, "ssl is not ready"sv);

	// 1) handshake를 진행한다. (client면 ClientHello가 만들어진다.)
	this->_do_handshake();

	// return) 보낼 데이터
	return this->_read_send_bio();
}

shared_buffer crypt::ssl::encode_buffer(const buffer_view& _source)
{
	// check)
	if(this->m_ssl == nullptr || this->m_pbio_send == nullptr)
		throw throwable::failure(eRESULT::NOT_READY, 0, "ssl is not ready"sv);

	// 1) handshake 중이면 평문을 보관한다. (handshake가 끝나면 _flush_pending_send에서 암호화한다.)
	if (!SSL_is_init_finished(this->m_ssl))
	{
		// - 복사해서 보관한다. (호출한 쪽의 buffer는 바로 해제될 수 있다.)
		if (_source.size() != 0)
		{
			auto buf_copy = alloc_shared_buffer(_source.size());
			buf_copy.append(_source.size(), _source.data());
			this->m_vector_pending_send.push_back(std::move(buf_copy));
		}

		// - handshake를 진행한다. (client의 첫 송신이면 ClientHello가 만들어진다.)
		this->_do_handshake();

		// return) 보낼 데이터 (handshake 데이터 또는 없음)
		return this->_read_send_bio();
	}

	// 2) SSL_write는 연결된 memory BIO에 암호화된 데이터를 써넣는다.
	//    (memory BIO는 크기 제한이 없으므로 오류가 아니면 모두 써진다.)
	auto buf_source = _source;

	while (buf_source.size() != 0)
	{
		// - 한번에 최대 INT_MAX까지만 쓴다.
		auto bytes_write = static_cast<int>(__select_min<std::size_t>(buf_source.size(), static_cast<std::size_t>(INT_MAX)));

		// - SSL_write를 한다.
		auto result_write = SSL_write(this->m_ssl, buf_source.data(), bytes_write);

		// check) 실패하면 exception을 던진다. (데이터를 조용히 버리지 않는다.)
		if (result_write <= 0)
		{
			auto error_code = SSL_get_error(this->m_ssl, result_write);
			throw throwable::failure(eRESULT::FAIL, error_code, _get_ssl_error_string(error_code));
		}

		// - 쓴 만큼 진행한다.
		buf_source += offset(result_write);
	}

	// return) 암호화된 데이터 (SSL_read 중 생긴 응답(TLS 1.3 key update 등)도 포함된다.)
	return this->_read_send_bio();
}

bool crypt::ssl::process_receive(buffer_view& _dest, const_buffer_view& _source, shared_buffer& _send_out)
{
	// check)
	if(this->m_ssl == nullptr || this->m_pbio_receive == nullptr)
		throw throwable::failure(eRESULT::NOT_READY, 0, "ssl is not ready"sv);

	// declare)
	buffer_view temp = _dest;
	std::size_t length_write = 0;

	// 1) 받은 데이터를 모두 SSL(memory BIO)에 넣는다.
	while (_source.size() != 0)
	{
		auto bytes_write = static_cast<int>(__select_min<std::size_t>(_source.size(), static_cast<std::size_t>(INT_MAX)));
		auto result_write = BIO_write(this->m_pbio_receive, _source.data(), bytes_write);

		// check)
		if(result_write <= 0)
			throw throwable::failure(eRESULT::FAIL, 0, "ssl: fail to write received data"sv);

		_source += offset(result_write);
	}

	// 2) handshake 중이면 진행한다. (끝나면 보관한 평문도 암호화된다.)
	if (!SSL_is_init_finished(this->m_ssl))
		this->_do_handshake();

	// 3) handshake가 끝났으면 복호화한다.
	if (SSL_is_init_finished(this->m_ssl))
	{
		while (true)
		{
			// check) 복호화 버퍼가 가득 찼다. (복호화된 데이터를 SSL에 남겨두면 처리가 멈추므로 실패 처리한다.)
			if(temp.size() == 0)
				throw throwable::failure(eRESULT::FAIL, 0, "ssl: decode buffer too small"sv);

			// - 데이터를 읽어들인다.
			auto result_read = SSL_read(this->m_ssl, temp.data(), temp.size<int>());

			// check)
			if (result_read <= 0)
			{
				auto error_code = SSL_get_error(this->m_ssl, result_read);

				// - 데이터를 더 받아야 한다.
				if(error_code == SSL_ERROR_WANT_READ)
					break;

				// - 상대가 close_notify를 보냈다. (지금까지 읽은 데이터는 처리하고 접속 종료는 TCP 종료로 처리된다.)
				if(error_code == SSL_ERROR_ZERO_RETURN)
					break;

				// raise)
				throw throwable::failure(eRESULT::FAIL, error_code, _get_ssl_error_string(error_code));
			}

			// - 버퍼를 줄인다.
			temp += offset(result_read);

			// - 길이를 더해준다.
			length_write += static_cast<std::size_t>(result_read);
		}
	}

	// 4) 보낼 데이터 (handshake 응답, 보관했던 평문, TLS 1.3 post-handshake 응답 등)
	_send_out = this->_read_send_bio();

	// 5) 복호화된 길이를 설정한다.
	_dest.set_size(length_write);

	// return)
	return length_write != 0;
}

bool crypt::ssl::encode(buffer_view& _dest, const const_buffer_view* _source, int _count)
{
	// check)
	if(this->m_ssl == nullptr || this->m_pbio_send == nullptr)
		throw throwable::failure(eRESULT::NOT_READY, 0, "ssl is not ready"sv);

	// 1) SSL_write는 연결된 BIO 메모리에 암호화된 데이터를 써넣는다.
	for(int i=0; i<_count; ++i)
	{
		// - 버퍼를 임시로 복사한다.
		auto buf_source = _source[i];

		while(buf_source.size() != 0)
		{
			// - SSLWrite를 한다.
			auto result_write = SSL_write(this->m_ssl, buf_source.data(), buf_source.size<int>());

			// check) 실패하면 exception을 던진다. (데이터를 조용히 버리지 않는다.)
			if (result_write <= 0)
			{
				auto error_code = SSL_get_error(this->m_ssl, result_write);
				throw throwable::failure(eRESULT::FAIL, error_code, _get_ssl_error_string(error_code));
			}

			// check)
			CGASSERT_ERROR(buf_source.size<int>() >= result_write);

			// - 쓴 data_를 증가시킨다.
			buf_source += offset(result_write);
		}
	}

	// check) 암호화된 데이터가 _dest에 모두 들어가야 한다. (일부만 보내면 TLS stream이 깨진다.)
	auto bytes_pending = BIO_ctrl_pending(this->m_pbio_send);
	THROW_IF(bytes_pending > _dest.size(), throwable::failure(eRESULT::FAIL, 0, "ssl: encode buffer too small"sv));

	// 2) 버퍼에서 암호화된 데이터를 읽어들인다.
	buffer_view temp = _dest;
	size_t length_read = 0;

	while (length_read < bytes_pending)
	{
		// - 읽어들인다.
		auto result_read = BIO_read(this->m_pbio_send, temp.data(), temp.size<int>());

		// check)
		if(result_read <= 0)
			throw throwable::failure(eRESULT::FAIL, 0, "ssl: fail to read encrypted data"sv);

		// - 버퍼를 줄인다.
		temp += offset(result_read);

		// - 읽은 량을 증가시킨다
		length_read += result_read;
	}

	// 3) 값을 Update한다.
	_dest.set_size(length_read);

	// return)
	return length_read != 0;
}

bool crypt::ssl::decode(buffer_view& _dest, const_buffer_view* _source, int _count)
{
	// check)
	if(this->m_ssl == nullptr || this->m_pbio_receive == nullptr)
		throw throwable::failure(eRESULT::NOT_READY, 0, "ssl is not ready"sv);

	// declare)
	// 2) 복호화된 내용을 읽어 target 메모리에 복사한다.
	buffer_view temp = _dest;
	std::size_t length_write = 0;
	size_t length_read = 0;
	bool is_closed = false;

	// 1) BIO_write는 source 메모리의 내용을 복호화하여 연결한 SSL에서 읽을 수 있도록 한다.
	for(int i = 0; i<_count; ++i)
	{
		// - 버퍼를 임시로 복사한다.
		auto buf_source = _source[i];

		while(buf_source.size() != 0)
		{
			// - 읽어들인다.
			auto result_write = BIO_write(this->m_pbio_receive, buf_source.data(), buf_source.size<int>());

			// check) 읽어들인 데이터가 0이거나 0보다 작으면 끝낸다.
			if(result_write <= 0)
				throw throwable::failure(eRESULT::FAIL, 0, "ssl: fail to write received data"sv);

			// check)
			CGASSERT_ERROR(buf_source.size<int>() >= result_write);

			// - 읽은 데이터를 증가시킨다.
			length_read	+= result_write;
			buf_source += offset(result_write);

			// - 상대가 close_notify를 보낸 후의 데이터는 읽지 않는다.
			if (is_closed)
				continue;

			// - 암호화한 데이터를 읽어들인다.
			while (true)
			{
				// check) 복호화 버퍼가 가득 찼다. (복호화된 데이터를 SSL에 남겨두면 처리가 멈추므로 실패 처리한다.)
				if(temp.size() == 0)
					throw throwable::failure(eRESULT::FAIL, 0, "ssl: decode buffer too small"sv);

				// - 데이터를 읽어들인다.
				auto result_read = SSL_read(this->m_ssl, temp.data(), temp.size<int>());

				// check) SSL_read()가 리턴한 값이 0보다 작으면 SSL_get_error(ssl, size_)로 에러코드를 확인한다.
				if(result_read <= 0)
				{
					// - Error code를 얻는다.
					auto error_code = SSL_get_error(this->m_ssl, result_read);

					// check) SSL_ERROR_WANT_READ면 추후 다시 Read하란 뜻이므로 일단 break한다.
					if(error_code == SSL_ERROR_WANT_READ)
						break;

					// check) 상대가 close_notify를 보냈다. (지금까지 읽은 데이터는 처리하고 접속 종료는 TCP 종료로 처리된다.)
					if (error_code == SSL_ERROR_ZERO_RETURN)
					{
						is_closed = true;
						break;
					}

					// Raise)
					throw throwable::failure(eRESULT::FAIL, error_code, _get_ssl_error_string(error_code));
				}

				// check)
				CGASSERT_ERROR(temp.size<int>() >= result_read);

				// - 버퍼를 줄인다.
				temp += offset(result_read);

				// - 길이를 더해준다.
				length_write += result_read;
			}
		}

		// - 변경된 버퍼 내용을 적용한다.
		_source[i] = buf_source;
	}

	// 3) 길이를 써넣는다.
	for (int i = 0; i<_count; ++i)
	{
		auto& shared_buffer = _source[i];

		if (shared_buffer.size() < length_read)
		{
			length_read -= shared_buffer.size();
			shared_buffer += offset(shared_buffer.size());
		}
		else
		{
			shared_buffer += offset(length_read);
			length_read = 0;

			break;
		}
	}

	// 4) Destination shared_buffer의 길이를 써넣는다.
	_dest.set_size(length_write);

	// return)
	return length_write != 0;
}

result_code crypt::ssl::destroy() noexcept
{
	if(this->m_ssl != nullptr)
	{
		// - (memory BIO에 쓰여진 close_notify는 전송되지 않는다. 접속 종료는 TCP 종료로 처리된다.)
		SSL_free(this->m_ssl);
		this->m_ssl = nullptr;

		// - ...
		this->m_pbio_receive = nullptr;
		this->m_pbio_send = nullptr;

		// - ...
		this->m_pssl_context.reset();
	}

	// return)
	return result_code(eRESULT::SUCCESS);
}


}
