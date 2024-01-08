#include "pch.h"

#include <openssl/rsa.h>
#include <openssl/crypto.h> 
#include <openssl/x509.h> 
#include <openssl/pem.h> 
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <thread>


namespace CGDK
{

crypt::ssl::ssl() noexcept :
	m_pSSL(nullptr),
	m_pbio_receive(nullptr),
	m_pbio_send(nullptr),
	m_pssl_context()
{
}

crypt::ssl::~ssl() noexcept
{
	destroy();
}

void crypt::ssl::initialize_handshake_server(Icryptable* _pcryptable)
{
	// check)
	CGASSERT_ERROR(m_pSSL != nullptr);

	// check)
	RETURN_IF(m_pSSL == nullptr);

	// check)
	CGASSERT_ERROR(_pcryptable != nullptr);

	// check)
	THROW_IF(_pcryptable == nullptr, throwable::failure(eRESULT::FAIL));

	// 1) SSL Handshake를 수행한다.

	while(true)
	{
		// - SSL Handshake를 수행한다.
		auto result = SSL_accept(m_pSSL);

		// check) Break on success!
		BREAK_IF(result == 1);

		// - SSL error code를 얻는다.
		auto error_code = SSL_get_error(m_pSSL, result);

		switch (error_code)
		{
		case	SSL_ERROR_WANT_WRITE:
				break;

		case	SSL_ERROR_WANT_READ:
				break;

		default:
				throw throwable::failure(eRESULT::FAIL, error_code);
		}

		// - sleep
		std::this_thread::sleep_for(1ms);
	}

	// 2) BIO 생성
	BIO* pbioReceive = BIO_new(BIO_s_mem());
	BIO* pbioSend = BIO_new(BIO_s_mem());

	// 3) BIO를 설정한다.
	SSL_set_bio(m_pSSL, pbioReceive, pbioSend);

	// 4) BIO 설정
	m_pbio_receive = pbioReceive;
	m_pbio_send = pbioSend;

	// 5) 설정한다.
	_pcryptable->m_pcrypt = this;
}

void crypt::ssl::initialize_handshake_client(Icryptable* _pcryptable)
{
	// check)
	CGASSERT_ERROR(m_pSSL != nullptr);

	// check)
	RETURN_IF(m_pSSL == nullptr);

	// check)
	CGASSERT_ERROR(_pcryptable != nullptr);

	// check)
	THROW_IF(_pcryptable == nullptr, throwable::failure(eRESULT::FAIL));

	while(true)
	{
		// - SSL Handshake를 수행한다.
		auto result = SSL_connect(m_pSSL);

		// check) Break on success!
		BREAK_IF(result == 1);

		// - ...
		if (result != 1)
		{
			switch (SSL_get_error(m_pSSL, result))
			{
			case	SSL_ERROR_WANT_WRITE:
					break;

			case	SSL_ERROR_WANT_READ:
					break;

			default:
					throw throwable::failure(eRESULT::FAIL);
			}
		}

		// - sleep
		std::this_thread::sleep_for(1ms);
	}

	// 2) BIO 생성
	BIO* pbioReceive = BIO_new(BIO_s_mem());
	BIO* pbioSend = BIO_new(BIO_s_mem());

	// 3) BIO를 설정한다.
	SSL_set_bio(m_pSSL, pbioReceive, pbioSend);

	// 4) BIO 설정
	m_pbio_receive = pbioReceive;
	m_pbio_send = pbioSend;

	// 5) 설정한다.
	_pcryptable->m_pcrypt = this;
}

bool crypt::ssl::encode(buffer_view& _dest, const const_buffer_view* _source, int _count)
{
	// 2) 버퍼에서 암호화된 데이터를 읽어들인다.
	buffer_view temp = _dest;
	size_t length_read = 0;

	// 1) SSL_write는 연결된 BIO 메모리에 암호화된 데이터를 써넣는다.
	for(int i=0; i<_count; ++i)
	{
		// - 버퍼를 임시로 복사한다.
		auto buf_source = _source[i];

		while(buf_source.size() != 0)
		{
			// - SSLWrite를 한다.
			auto result_wirte = SSL_write(m_pSSL, buf_source.data(), buf_source.size<int>());

			// check) SSL_read()가 리턴한 값이 0이거나 0보다 작으면 종료...	// 보다 작으면 SSL_get_error(ssl, size_); 를 호출하여 error code가 SSL_ERROR_WANT_READ인지 확인하고 SSL_ERROR_WANT_READ라면 아직 암호화된 내용이 모두 도착하지 않아 복호화에 실패한 것이다.
			BREAK_IF(result_wirte <= 0);

			// check)
			CGASSERT_ERROR(buf_source.size<int>() >= result_wirte);

			// - 쓴 data_를 증가시킨다.
			buf_source += offset(result_wirte);

			// - 읽어들인다.
			while(true)
			{
				// - 읽어들인다.
				auto result_read = BIO_read(m_pbio_send, temp.data(), temp.size<int>());

				// check) 읽어들인 량이 0이거나 0보다 작으면 종료~
				BREAK_IF(result_read <= 0);

				// check)
				CGASSERT_ERROR(temp.size<int>() >= result_read);

				// - 버퍼를 줄인다.
				temp += offset(result_read);

				// - 읽은 량을 증가시킨다.
				length_read += result_read;
			}
		}
	}

	// 3) 값을 Update한다.
	_dest.set_size(length_read);

	// return)
	return length_read != 0;
}

bool crypt::ssl::decode(buffer_view& _dest, const_buffer_view* _source, int _count)
{
	// declare)
	// 2) 복호화된 내용을 읽어 target 메모리에 복사한다.
	buffer_view temp = _dest;
	auto length_write = 0;
	size_t length_read = 0;

	// 1) BIO_write는 source 메모리의 내용을 복호화하여 연결한 SSL에서 읽을 수 있도록 한다.
	for(int i = 0; i<_count; ++i)
	{
		// - 버퍼를 임시로 복사한다.
		auto buf_source = _source[i];

		while(buf_source.size() != 0)
		{
			// - 읽어들인다.
			auto result_write = BIO_write(m_pbio_receive, buf_source.data(), buf_source.size<int>());

			// check) 읽어들인 데이터가 0이거나 0보다 작으면 끝낸다.
			THROW_IF(result_write <= 0, throwable::failure(eRESULT::FAIL));

			// check)
			CGASSERT_ERROR(buf_source.size<int>() >= result_write);

			// - 읽은 데이터를 증가시킨다.
			length_read	+= result_write;
			buf_source	+= offset(result_write);

			// - 암호화한 데이터를 읽어들인다.
			while (true)
			{
				// - 데이터를 읽어들인다.
				auto result_read = SSL_read(m_pSSL, temp.data(), temp.size<int>());

				// check) SSL_read()가 리턴한 값이 0보다 작으면 SSL_get_error(ssl, size_)로 에러코드를 확인한다.
				if(result_read <= 0)
				{
					// - Error code를 얻는다.
					auto error_code = SSL_get_error(m_pSSL, result_read);

					// check) SSL_ERROR_WANT_READ면 추후 다시 Read하란 뜻이므로 일단 break한다.
					BREAK_IF(error_code == SSL_ERROR_WANT_READ);

					// Raise)
					throw throwable::failure(eRESULT::FAIL, error_code);
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
	if(m_pSSL != nullptr)
	{
		// - 
		SSL_shutdown(m_pSSL);
		SSL_free(m_pSSL);
		m_pSSL = nullptr;

		// - ...
		m_pbio_receive = nullptr;
		m_pbio_send = nullptr;

		// - ...
		m_pssl_context.reset();
	}

	// return)
	return result_code(eRESULT::SUCCESS);
}


}