#include "pch.h"
#include "openssl/sha.h"
#include "openssl/hmac.h"
#include "openssl/aes.h"
#include <iomanip>

namespace CGDK
{

bool crypt::sha1(buffer_view _output, const_buffer_view _source) noexcept
{
	// check)
	CGASSERT_ERROR(_output.size() >= SHA_DIGEST_LENGTH);

	// check)
	RETURN_IF(_output.size() < SHA_DIGEST_LENGTH, false);

	// 1) get sha1 code and return result
	return ::SHA1(_source.data<const unsigned char>(), _source.size(), _output.data<unsigned char>()) != nullptr;
}

shared_buffer crypt::sha1(const_buffer_view _source)
{
	// declare) 
	auto buf_output = alloc_shared_buffer(SHA_DIGEST_LENGTH) ^ SHA_DIGEST_LENGTH;

	// return)
	auto result = sha1(buf_output, _source);

	// check)
	THROW_IF(result == false, throwable::failure(eRESULT::FAIL));

	// return)
	return buf_output;
}

bool crypt::sha224(buffer_view _output, const_buffer_view _source) noexcept
{
	// check)
	CGASSERT_ERROR(_output.size() >= SHA224_DIGEST_LENGTH);

	// check)
	RETURN_IF(_output.size() < SHA224_DIGEST_LENGTH, false);

	// 1) get sha224 code and return result
	return ::SHA224(_source.data<const unsigned char>(), _source.size(), _output.data<unsigned char>()) != nullptr;
}

shared_buffer crypt::sha224(const_buffer_view _source)
{
	// declare) 
	auto buf_output = alloc_shared_buffer(SHA224_DIGEST_LENGTH) ^ SHA224_DIGEST_LENGTH;

	// return)
	auto result = sha224(buf_output, _source);

	// check)
	THROW_IF(result == false, throwable::failure(eRESULT::FAIL));

	// return)
	return buf_output;
}

bool crypt::sha256(buffer_view _output, const_buffer_view _source) noexcept
{
	// check)
	CGASSERT_ERROR(_output.size() >= SHA224_DIGEST_LENGTH);

	// check)
	RETURN_IF(_output.size() < SHA224_DIGEST_LENGTH, false);

	// 1) get sha256 code and return result
	return ::SHA256(_source.data<const unsigned char>(), _source.size(), _output.data<unsigned char>()) != nullptr;
}

shared_buffer crypt::sha256(const_buffer_view _source)
{
	// declare) 
	auto buf_output = alloc_shared_buffer(SHA256_DIGEST_LENGTH) ^ SHA256_DIGEST_LENGTH;

	// return)
	auto result = sha256(buf_output, _source);

	// check)
	THROW_IF(result == false, throwable::failure(eRESULT::FAIL));

	// return)
	return buf_output;
}

bool crypt::sha384(buffer_view _output, const_buffer_view _source) noexcept
{
	// check)
	CGASSERT_ERROR(_output.size() >= SHA384_DIGEST_LENGTH);

	// check)
	RETURN_IF(_output.size() < SHA384_DIGEST_LENGTH, false);

	// 1) get sha384 code and return result
	return ::SHA384(_source.data<const unsigned char>(), _source.size(), _output.data<unsigned char>()) != nullptr;
}

shared_buffer crypt::sha384(const_buffer_view _source)
{
	// declare) 
	auto buf_output = alloc_shared_buffer(SHA384_DIGEST_LENGTH) ^ SHA384_DIGEST_LENGTH;

	// return)
	auto result = sha384(buf_output, _source);

	// check)
	THROW_IF(result == false, throwable::failure(eRESULT::FAIL));

	// return)
	return buf_output;
}

bool crypt::sha512(buffer_view _output, const_buffer_view _source) noexcept
{
	// check)
	CGASSERT_ERROR(_output.size() >= SHA512_DIGEST_LENGTH);

	// check)
	RETURN_IF(_output.size() < SHA512_DIGEST_LENGTH, false);

	// 1) get sha512 code and return result
	return ::SHA512(_source.data<const unsigned char>(), _source.size(), _output.data<unsigned char>()) != nullptr;
}

shared_buffer crypt::sha512(const_buffer_view _source)
{
	// declare) 
	auto buf_output = alloc_shared_buffer(SHA512_DIGEST_LENGTH) ^ SHA512_DIGEST_LENGTH;

	// return)
	auto result = sha512(buf_output, _source);

	// check)
	THROW_IF(result == false, throwable::failure(eRESULT::FAIL));

	// return)
	return buf_output;
}


buffer_view crypt::hmac_md5(buffer_view _output, const_buffer_view _key, const_buffer_view _source) noexcept
{
	// 1) 
	unsigned int output_length = _output.size<unsigned int>();

	// 2) hmac
	auto result = ::HMAC(EVP_md5(), _key.data<void>(), _key.size<int>(), _source.data<const unsigned char>(), _source.size<int>(), _output.data<unsigned char>(), &output_length);

	// check) 
	RETURN_IF(result == nullptr, buffer_view());

	// return) 
	return	_output ^ output_length;
}
shared_buffer crypt::hmac_md5(const_buffer_view _source, const_buffer_view _key)
{
	// 1) alloc buffer 
	auto buf_output = alloc_shared_buffer(1024) ^ 1024;

	// 2) hmac_md5
	buf_output ^= hmac_md5(buf_output, _source, _key);

	// return)
	return buf_output;
}

buffer_view crypt::hmac_sha1(buffer_view _output, const_buffer_view _key, const_buffer_view _source) noexcept
{
	unsigned int output_length = _output.size<unsigned int>();

	// 2) hmac
	auto result = ::HMAC(EVP_sha1(), _key.data<void>(), _key.size<int>(), _source.data<const unsigned char>(), _source.size<int>(), _output.data<unsigned char>(), &output_length);

	// check) 
	RETURN_IF(result == nullptr, buffer_view());

	// return) 
	return	_output ^ output_length;
}
shared_buffer crypt::hmac_sha1(const_buffer_view _source, const_buffer_view _key)
{
	// 1) alloc buffer 
	auto buf_output = alloc_shared_buffer(1024) ^ 1024;

	// 2) hmac_sha1
	buf_output ^= hmac_sha1(buf_output, _source, _key);

	// return)
	return buf_output;
}

buffer_view crypt::hmac_sha224(buffer_view _output, const_buffer_view _key, const_buffer_view _source) noexcept
{
	unsigned int output_length = _output.size<unsigned int>();

	// 2) hmac
	auto result = ::HMAC(EVP_sha224(), _key.data<void>(), _key.size<int>(), _source.data<const unsigned char>(), _source.size<int>(), _output.data<unsigned char>(), &output_length);

	// check) 
	RETURN_IF(result == nullptr, buffer_view());

	// return) 
	return	_output ^ output_length;
}
shared_buffer crypt::hmac_sha224(const_buffer_view _source, const_buffer_view _key)
{
	// 1) alloc buffer 
	auto buf_output = alloc_shared_buffer(1024) ^ 1024;

	// 2) hmac_sha224
	buf_output ^= hmac_sha224(buf_output, _source, _key);

	// return)
	return buf_output;
}

buffer_view crypt::hmac_sha256(buffer_view _output, const_buffer_view _key, const_buffer_view _source) noexcept
{
	unsigned int output_length = _output.size<unsigned int>();

	// 2) hmac
	auto result = ::HMAC(EVP_sha256(), _key.data<void>(), _key.size<int>(), _source.data<const unsigned char>(), _source.size<int>(), _output.data<unsigned char>(), &output_length);

	// check) 
	RETURN_IF(result == nullptr, buffer_view());

	// return) 
	return	_output ^ output_length;
}
shared_buffer crypt::hmac_sha256(const_buffer_view _source, const_buffer_view _key)
{
	// 1) alloc buffer 
	auto buf_output = alloc_shared_buffer(1024) ^ 1024;

	// 2) 
	buf_output ^= hmac_sha256(buf_output, _source, _key);

	// return)
	return buf_output;
}

buffer_view crypt::hmac_sha384(buffer_view _output, const_buffer_view _key, const_buffer_view _source) noexcept
{
	unsigned int output_length = _output.size<unsigned int>();

	// 2) hmac
	auto result = ::HMAC(EVP_sha384(), _key.data<void>(), _key.size<int>(), _source.data<const unsigned char>(), _source.size<int>(), _output.data<unsigned char>(), &output_length);

	// check) 
	RETURN_IF(result == nullptr, buffer_view());

	// return) 
	return	_output ^ output_length;
}
shared_buffer crypt::hmac_sha384(const_buffer_view _source, const_buffer_view _key)
{
	// 1) alloc buffer 
	auto buf_output = alloc_shared_buffer(1024) ^ 1024;

	// 2) 
	buf_output ^= hmac_sha384(buf_output, _source, _key);

	// return)
	return buf_output;
}

buffer_view crypt::hmac_sha512(buffer_view _output, const_buffer_view _key, const_buffer_view _source) noexcept
{
	unsigned int output_length = _output.size<unsigned int>();

	// 2) hmac
	auto result = ::HMAC(EVP_sha512(), _key.data<void>(), _key.size<int>(), _source.data<const unsigned char>(), _source.size<int>(), _output.data<unsigned char>(), &output_length);

	// check) 
	RETURN_IF(result == nullptr, buffer_view());

	// return) 
	return	_output ^ output_length;
}
shared_buffer crypt::hmac_sha512(const_buffer_view _source, const_buffer_view _key)
{
	// 1) alloc buffer 
	auto buf_output = alloc_shared_buffer(1024) ^ 1024;

	// 2) hmac
	auto result = hmac_sha512(buf_output, _source, _key);

	// return)
	return buf_output ^ result;
}

buffer_view crypt::aes_encrypt(buffer_view _out, buffer_view _tag, aes_key _key, const_buffer_view _source, const_buffer_view _source_add)
{
	// check)
	CGASSERT_ERROR(_key.key.size() != 0);

	// check)
	CGASSERT_ERROR(_key.iv.size() != 0);

	// declare) 
	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();

    // 1) create and initialise the context
    THROW_IF(ctx == nullptr, throwable::failure(eRESULT::FAIL, ERR_get_error(), std::string_view(ERR_error_string(ERR_get_error(), nullptr))));

	// declare) 
	int ciphertext_len = 0;

	try
	{
		// 2) initialise the encryption operation.
		{
			// - ...
			auto result = EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr);

			// check) 
			THROW_IF(result != 1, throwable::failure(eRESULT::FAIL, ERR_get_error(), std::string_view(ERR_error_string(ERR_get_error(), nullptr))));
		}

		{
			//result = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 16, nullptr);
			auto result = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, _key.iv.size<int>(), nullptr);

			// check) 
			THROW_IF(result != 1, throwable::failure(eRESULT::FAIL, ERR_get_error(), std::string_view(ERR_error_string(ERR_get_error(), nullptr))));
		}

		// 3) Initialise key and IV
		{
			// - ...
			auto result = EVP_EncryptInit_ex(ctx, nullptr, nullptr, _key.key.data<unsigned char>(), _key.iv.data<unsigned char>());

			// check) 
			THROW_IF(result != 1, throwable::failure(eRESULT::FAIL, ERR_get_error(), std::string_view(ERR_error_string(ERR_get_error(), nullptr))));
		}

		// 4) Provide any AAD data. This can be called zero or more times as required
		if (_source_add.size() != 0)
		{
			// declare) 
			int len = 0;

			// - ....
			auto result = EVP_EncryptUpdate(ctx, nullptr, &len, _source_add.data<unsigned char>(), _source_add.size<int>());

			// check) 
			THROW_IF(result != 1, throwable::failure(eRESULT::FAIL, ERR_get_error(), std::string_view(ERR_error_string(ERR_get_error(), nullptr))));
		}

		// 5) Provide the message to be encrypted, and obtain the encrypted output. EVP_EncryptUpdate can be called multiple times if necessary
		if (_source.size() != 0)
		{
			// declare) 
			int len = 0;

			// - ...
			auto result = EVP_EncryptUpdate(ctx, _out.data<unsigned char>(), &len, _source.data<unsigned char>(), _source.size<int>());

			// check) 
			THROW_IF(result != 1, throwable::failure(eRESULT::FAIL, ERR_get_error(), std::string_view(ERR_error_string(ERR_get_error(), nullptr))));

			// - set length
			ciphertext_len = len;
		}

		// 6) finalise the encryption. Normally ciphertext bytes may be written at this stage, but this does not occur in GCM mode
		{
			// declare) 
			int len = ciphertext_len;

			// - 
			auto result = EVP_EncryptFinal_ex(ctx, _out.data<unsigned char>() + ciphertext_len, &len);

			// check) 
			THROW_IF(result != 1, throwable::failure(eRESULT::FAIL, ERR_get_error(), std::string_view(ERR_error_string(ERR_get_error(), nullptr))));

			// - ...
			ciphertext_len += len;
		}

		// 8) get the tag
		if (_tag.size() != 0)
		{
			// - get tag
			auto result = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, _tag.data<unsigned char>());

			// check) 
			THROW_IF(result != 1, throwable::failure(eRESULT::FAIL, ERR_get_error(), std::string_view(ERR_error_string(ERR_get_error(), nullptr))));
		}

		// 9) clean up
		EVP_CIPHER_CTX_free(ctx);
	}
	catch (...)
	{
		// - clean up
		EVP_CIPHER_CTX_free(ctx);

		// reraise)
		throw;
	}

	// return) 
    return _out ^ ciphertext_len;
}

buffer_view crypt::aes_decrypt(buffer_view _out, buffer_view _tag, aes_key _key, const_buffer_view _source, const_buffer_view _source_add)
{
	// check)
	CGASSERT_ERROR(_key.key.size() != 0);

	// check)
	CGASSERT_ERROR(_key.iv.size() != 0);

	// declare) 
	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();

	// 1) create and initialise the context
	THROW_IF(ctx == nullptr, throwable::failure(eRESULT::FAIL, ERR_get_error(), std::string_view(ERR_error_string(ERR_get_error(), nullptr))));

	// declare) 
	int plaintext_len = 0;
	
	try
	{
		// declare) 
		const size_t TAG_SIZE = 16;

		// 2) initialise the decryption operation.
		{
			// - ...
			auto result = EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr);

			// check) 
			THROW_IF(result != 1, throwable::failure(eRESULT::FAIL, ERR_get_error(), std::string_view(ERR_error_string(ERR_get_error(), nullptr))));
		}

		// 3) set IV length. Not necessary if this is 12 bytes (96 bits)
		{
			// - ...
			//result = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 16, nullptr);
			auto result = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, _key.iv.size<int>(), nullptr);

			// check) 
			THROW_IF(result != 1, throwable::failure(eRESULT::FAIL, ERR_get_error(), std::string_view(ERR_error_string(ERR_get_error(), nullptr))));
		}

		// 4) initialise key and IV
		{
			auto result = EVP_DecryptInit_ex(ctx, nullptr, nullptr, _key.key.data<unsigned char>(), _key.iv.data<unsigned char>());

			// check) 
			THROW_IF(result != 1, throwable::failure(eRESULT::FAIL, ERR_get_error(), std::string_view(ERR_error_string(ERR_get_error(), nullptr))));
		}

		// 5) Provide any AAD data. This can be called zero or more times as required
		if(_source_add.size() != 0)
		{
			// declare) 
			int len = 0;

			// - ...
			auto result = EVP_DecryptUpdate(ctx, nullptr, &len, _source_add.data<unsigned char>(), _source_add.size<int>());

			// check) 
			THROW_IF(result != 1, throwable::failure(eRESULT::FAIL, ERR_get_error(), std::string_view(ERR_error_string(ERR_get_error(), nullptr))));
		}

		// 6) provide the message to be decrypted, and obtain the plaintext output. EVP_DecryptUpdate can be called multiple times if necessary
		if(_source.size() != 0)
		{
			// declare) 
			int len = 0;

			// - ...
			auto result = EVP_DecryptUpdate(ctx, _out.data<unsigned char>(), &len, _source.data<unsigned char>(), _source.size<int>());

			// check) 
			THROW_IF(result != 1, throwable::failure(eRESULT::FAIL, ERR_get_error(), std::string_view(ERR_error_string(ERR_get_error(), nullptr))));

			// - set ...
			plaintext_len = len;
		}

		// 7) Set expected tag value. Works in OpenSSL 1.0.1d and later
		{
			// declare)
			unsigned char tag[TAG_SIZE]{ 0 };

			// 2) alloc tag buffer
			if (_tag.size() == 0)
			{
				_tag = buffer_view{ tag, TAG_SIZE };
			}

			// - get tag
			auto result = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, _tag.size<int>(), _tag.data<void>());

			// check) 
			THROW_IF(result != 1, throwable::failure(eRESULT::FAIL, ERR_get_error(), std::string_view(ERR_error_string(ERR_get_error(), nullptr))));
		}

		// 8) Finalise the decryption. A positive return value indicates success, anything else is a failure - the plaintext is not trustworthy.
		{
			// declare) 
			int len = 0;

			// - ...
			auto result = EVP_DecryptFinal_ex(ctx, _out.data<unsigned char>() + plaintext_len, &len);

			if (result == 1)
			{
				plaintext_len += len;
			}
		}

		// 9) clean up
		EVP_CIPHER_CTX_free(ctx);
	}
	catch (...)
	{
		// - clean up
		EVP_CIPHER_CTX_free(ctx);

		// reraise)
		throw;
	}

	// return)
	return _out ^ plaintext_len;
}


}