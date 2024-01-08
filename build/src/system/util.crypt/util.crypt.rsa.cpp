#include "pch.h"

#include <openssl/rsa.h>
//#include <openssl/rand.h>

namespace CGDK
{

// RAII classes) 
class safe_BIO
{
public:
	BIO* v;
	
	safe_BIO(BIO* _v) noexcept : v(_v) {}
	~safe_BIO() noexcept 
	{
		if (v)
			BIO_vfree(v);
	}
	
	bool empty() const noexcept
	{
		return v == nullptr; 
	}
};

class safe_EVP_PKEY 
{
public:
	EVP_PKEY* v;

	safe_EVP_PKEY(EVP_PKEY* _v) noexcept : v(_v) {}
	~safe_EVP_PKEY() noexcept
	{
		if (v)
			EVP_PKEY_free(v);
	}
	bool empty() const noexcept
	{
		return v == nullptr;
	}
 };

class safe_RSA 
{
public: RSA* v;
	  safe_RSA(RSA* _v) noexcept : v(_v) {}
	  safe_RSA(safe_RSA&& _v) noexcept : v(_v.v) { _v.v = nullptr; }
	  ~safe_RSA() noexcept { if (v) RSA_free(v); }
	  bool empty() const noexcept { return v == nullptr; } 
	  safe_RSA& operator = (safe_RSA&& _v) noexcept { v = _v.v; _v.v = nullptr; return *this; }
private:
	  safe_RSA(const safe_RSA&) = delete;
	  safe_RSA& operator = (const safe_RSA& _v) = delete;
};

safe_RSA create_rsa_by_pem(crypt::rsa_public_key _public_key)
{
	// 1) alloc bio buffer
	safe_BIO buf_key = BIO_new_mem_buf(_public_key.key.data(), -1);

	// check) 
	THROW_IF(buf_key.empty(), throwable::failure(eRESULT::BAD_ALLOC));

	// declare) 
	RSA* rsa = nullptr;

	// 2) ...
	rsa = PEM_read_bio_RSA_PUBKEY(buf_key.v, &rsa, NULL, NULL);

	// check)
	THROW_IF(rsa == nullptr, throwable::failure(eRESULT::BAD_ALLOC));

	// return) 
	return safe_RSA(rsa);
}

safe_RSA create_rsa_by_pem(crypt::rsa_private_key _private_key)
{
	// 1) alloc bio buffer
	safe_BIO buf_key = BIO_new_mem_buf(_private_key.key.data(), -1);

	// check) 
	THROW_IF(buf_key.empty(), throwable::failure(eRESULT::BAD_ALLOC));

	// declare) 
	RSA* rsa = nullptr;

	// 2)  ...
	rsa = PEM_read_bio_RSAPrivateKey(buf_key.v, &rsa, NULL, NULL);

	// check)
	THROW_IF(rsa == nullptr, throwable::failure(eRESULT::BAD_ALLOC));

	// return) 
	return safe_RSA(rsa);
}

shared_buffer crypt::encrypt_rsa(crypt::rsa_public_key _encryption_key, buffer_view _source)
{
	// 1) get size
	const auto buf_length = _source.size() * 2 + 128 + 2;

	// 2) shared_buffer를 할당받는다.
	shared_buffer buf_result = alloc_shared_buffer(buf_length) ^ buf_length;

	// 3) encode...
	return encrypt_rsa(buf_result, _encryption_key, _source);
}

shared_buffer crypt::encrypt_rsa(shared_buffer _buf_dest, crypt::rsa_public_key _encryption_key, buffer_view _buf_source)
{
	// declare) 
	shared_buffer buf_dest;

	// 1) create RSA
	auto rsa_encryption_key = create_rsa_by_pem(_encryption_key);

	// declare)
	const int MAX_SIZE = 64;

	// declare)
	buffer_view buf_source = _buf_source;
	buf_dest = _buf_dest ^ 0;

	// 2) B의 공개키로 데이터 암호화
	while (buf_source.size() != 0)
	{
		// - get size
		auto size_encrypt = __select_min<int>(MAX_SIZE, buf_source.size<int>());

		// - encrypt
		auto result_size = RSA_public_encrypt(size_encrypt, buf_source.data<const unsigned char>(), reinterpret_cast<unsigned char*>(buf_dest.back()), rsa_encryption_key.v, RSA_PKCS1_PADDING);

		// check)
		THROW_IF(result_size < 0, throwable::failure(eRESULT::FAIL));

		// - append
		buf_source += offset(size_encrypt);
		buf_dest += size(result_size); 
	}

	// return)
	return buf_dest;
}

shared_buffer crypt::encrypt_rsa(crypt::rsa_private_key _encryption_key, buffer_view _source)
{
	// 1) get size
	const auto buf_length = _source.size() * 2 + 128 + 2;

	// 2) shared_buffer를 할당받는다.
	shared_buffer buf_result = alloc_shared_buffer(buf_length) ^ buf_length;

	// 3) encode...
	return encrypt_rsa(buf_result, _encryption_key, _source);
}

shared_buffer crypt::encrypt_rsa(shared_buffer _buf_dest, crypt::rsa_private_key _encryption_key, buffer_view _buf_source)
{
	// 1) create RSA
	auto rsa_encryption_key = create_rsa_by_pem(_encryption_key);

	// declare)
	const int MAX_SIZE = 64;

	// declare)
	buffer_view buf_source = _buf_source;
	shared_buffer buf_dest = _buf_dest ^ 0;

	// 2) B의 공개키로 데이터 암호화
	while (buf_source.size() != 0)
	{
		// - get size
		auto size_encrypt = __select_min<int>(MAX_SIZE, buf_source.size<int>());

		// - encrypt
		auto result_size = RSA_private_encrypt(size_encrypt, buf_source.data<const unsigned char>(), reinterpret_cast<unsigned char*>(buf_dest.back()), rsa_encryption_key.v, RSA_PKCS1_PADDING);

		// check)
		THROW_IF(result_size < 0, throwable::failure(eRESULT::FAIL));

		// - append
		buf_source += offset(size_encrypt);
		buf_dest += size(result_size);
	}

	// return)
	return buf_dest;
}


shared_buffer crypt::decrypt_rsa(crypt::rsa_public_key _decryption_key, buffer_view _source)
{
	// 1) shared_buffer를 할당받는다.
	shared_buffer buf_result = alloc_shared_buffer(_source.size() * 2 + 2);

	// return) 
	return decrypt_rsa(buf_result, _decryption_key, _source);
}

shared_buffer crypt::decrypt_rsa(shared_buffer _buf_dest, crypt::rsa_public_key _decryption_key, buffer_view _buf_source)
{
	// 1) create RSA
	auto rsa_decryption_key = create_rsa_by_pem(_decryption_key);

	// declare)
	const int MAX_SIZE = 128;

	// declare)
	buffer_view buf_source = _buf_source;
	shared_buffer buf_dest = _buf_dest ^ 0;

	// 2) B의 공개키로 데이터 암호화
	while (buf_source.size() != 0)
	{
		// 2) get size
		auto size_encrypt = __select_min<int>(MAX_SIZE, buf_source.size<int>());

		// 3) decryp
		auto result_size = RSA_public_decrypt(size_encrypt, buf_source.data<const unsigned char>(), reinterpret_cast<unsigned char*>(buf_dest.back()), rsa_decryption_key.v, RSA_PKCS1_PADDING);

		// check)
		THROW_IF(result_size < 0, throwable::failure(eRESULT::FAIL));

		// 4) append
		buf_source += offset(size_encrypt);
		buf_dest += size(result_size);
	}

	// return)
	return buf_dest;
}

shared_buffer crypt::decrypt_rsa(crypt::rsa_private_key _decryption_key, buffer_view _source)
{
	// 1) shared_buffer를 할당받는다.
	shared_buffer buf_result = alloc_shared_buffer(_source.size() * 2 + 2);

	// return) 
	return	decrypt_rsa(buf_result, _decryption_key, _source);
}

shared_buffer crypt::decrypt_rsa(shared_buffer _buf_dest, crypt::rsa_private_key _decryption_key, buffer_view _buf_source)
{
	// 1) create RSA
	auto rsa_decryption_key = create_rsa_by_pem(_decryption_key);

	// declare)
	const int MAX_SIZE = 128;

	// declare)
	buffer_view buf_source = _buf_source;
	shared_buffer buf_dest = _buf_dest ^ 0;

	// 2) B의 공개키로 데이터 암호화
	while (buf_source.size() != 0)
	{
		// 2) get size
		auto size_encrypt = __select_min<int>(MAX_SIZE, buf_source.size<int>());

		// 3) decryp
		auto result_size = RSA_private_decrypt(size_encrypt, buf_source.data<const unsigned char>(), reinterpret_cast<unsigned char*>(buf_dest.back()), rsa_decryption_key.v, RSA_PKCS1_PADDING);

		// check)
		THROW_IF(result_size < 0, throwable::failure(eRESULT::FAIL));

		// 4) append
		buf_source += offset(size_encrypt);
		buf_dest += size(result_size);
	}

	// return)
	return buf_dest;
}


}