#include "pch.h"
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/rand.h>

namespace CGDK
{
namespace crypt
{
namespace aes
{

// internal helpers
namespace
{
	// - AES-256-GCM
	constexpr std::size_t KEY_SIZE = 32;
	constexpr std::size_t TAG_SIZE = 16;
	constexpr std::size_t TAG_SIZE_MIN = 12;

	// - OpenSSL error queue의 오류를 한번만 읽어 exception을 만든다. (thread safe buffer 사용)
	[[noreturn]] void _throw_openssl_error(std::string_view _what)
	{
		auto error = ERR_get_error();
		char buffer[256]{};
		ERR_error_string_n(error, buffer, sizeof(buffer));
		ERR_clear_error();
		throw throwable::failure(eRESULT::FAIL, static_cast<uint64_t>(error), std::string("aes: ") + std::string(_what) + " (" + buffer + ")");
	}

	// - EVP_CIPHER_CTX 자동 해제
	struct cipher_ctx
	{
		EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
		~cipher_ctx() { if (ctx != nullptr) EVP_CIPHER_CTX_free(ctx); }
	};

	void _check_key(const aes_key& _key)
	{
		// check) AES-256 key는 32 byte여야 한다. (짧으면 key 범위 밖을 읽는다.)
		THROW_IF(_key.key.size() != KEY_SIZE, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "aes: key must be 32 bytes (AES-256)"sv));

		// check) IV(nonce)가 있어야 한다.
		THROW_IF(_key.iv.size() == 0, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "aes: iv is empty"sv));
	}
}

//-----------------------------------------------------------------------------
// encrypt (AES-256-GCM)
//
//  - _tag가 있으면 _tag에 16 byte 인증 tag를 쓴다. (_tag.size() >= 16)
//  - _tag가 비어 있으면 인증 tag를 암호문 뒤에 붙인다. (결과 = 암호문 + tag(16 byte))
//    (tag를 버리면 복호화할 때 변조를 검증할 수 없다.)
//  - 같은 key로 같은 IV를 다시 사용하면 안된다.
//-----------------------------------------------------------------------------
buffer_view encrypt(buffer_view _out, buffer_view _tag, aes_key _key, const_buffer_view _source, const_buffer_view _source_add)
{
	// check)
	_check_key(_key);

	// declare)
	const bool is_tag_appended = (_tag.size() == 0);

	// check) tag buffer
	THROW_IF(!is_tag_appended && _tag.size() < TAG_SIZE, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "aes: tag buffer must be at least 16 bytes"sv));

	// check) output buffer (GCM 암호문 크기 = 원문 크기)
	THROW_IF(_out.size() < _source.size() + (is_tag_appended ? TAG_SIZE : 0), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "aes: output buffer too small"sv));

	// check)
	THROW_IF(_source.size() > static_cast<std::size_t>(INT_MAX) || _source_add.size() > static_cast<std::size_t>(INT_MAX), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "aes: data too large"sv));

	// 1) create and initialise the context
	cipher_ctx context;
	if (context.ctx == nullptr)
		_throw_openssl_error("EVP_CIPHER_CTX_new");

	// declare)
	int ciphertext_len = 0;

	// 2) initialise the encryption operation.
	if (EVP_EncryptInit_ex(context.ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1)
		_throw_openssl_error("EVP_EncryptInit_ex");

	// 3) set IV length
	if (EVP_CIPHER_CTX_ctrl(context.ctx, EVP_CTRL_GCM_SET_IVLEN, _key.iv.size<int>(), nullptr) != 1)
		_throw_openssl_error("EVP_CTRL_GCM_SET_IVLEN");

	// 4) Initialise key and IV
	if (EVP_EncryptInit_ex(context.ctx, nullptr, nullptr, _key.key.data<unsigned char>(), _key.iv.data<unsigned char>()) != 1)
		_throw_openssl_error("EVP_EncryptInit_ex(key)");

	// 5) Provide any AAD data.
	if (_source_add.size() != 0)
	{
		int len = 0;
		if (EVP_EncryptUpdate(context.ctx, nullptr, &len, _source_add.data<unsigned char>(), _source_add.size<int>()) != 1)
			_throw_openssl_error("EVP_EncryptUpdate(aad)");
	}

	// 6) Provide the message to be encrypted, and obtain the encrypted output.
	if (_source.size() != 0)
	{
		int len = 0;
		if (EVP_EncryptUpdate(context.ctx, _out.data<unsigned char>(), &len, _source.data<unsigned char>(), _source.size<int>()) != 1)
			_throw_openssl_error("EVP_EncryptUpdate");

		ciphertext_len = len;
	}

	// 7) finalise the encryption. (GCM에서는 추가 출력이 없다.)
	{
		int len = 0;
		if (EVP_EncryptFinal_ex(context.ctx, _out.data<unsigned char>() + ciphertext_len, &len) != 1)
			_throw_openssl_error("EVP_EncryptFinal_ex");

		ciphertext_len += len;
	}

	// 8) get the tag
	{
		unsigned char* ptag = is_tag_appended ? (_out.data<unsigned char>() + ciphertext_len) : _tag.data<unsigned char>();

		if (EVP_CIPHER_CTX_ctrl(context.ctx, EVP_CTRL_GCM_GET_TAG, static_cast<int>(TAG_SIZE), ptag) != 1)
			_throw_openssl_error("EVP_CTRL_GCM_GET_TAG");

		if (is_tag_appended)
			ciphertext_len += static_cast<int>(TAG_SIZE);
	}

	// return)
	return _out ^ static_cast<std::size_t>(ciphertext_len);
}

//-----------------------------------------------------------------------------
// decrypt (AES-256-GCM)
//
//  - _tag가 있으면 _tag로 검증한다. (12~16 byte)
//  - _tag가 비어 있으면 _source의 마지막 16 byte를 tag로 사용한다. (encrypt에서 tag를 붙인 형식)
//  - 인증 tag 검증에 실패하면(변조, 잘못된 key/IV) 복호화한 내용을 지우고 exception을 던진다.
//-----------------------------------------------------------------------------
buffer_view decrypt(buffer_view _out, buffer_view _tag, aes_key _key, const_buffer_view _source, const_buffer_view _source_add)
{
	// check)
	_check_key(_key);

	// 1) tag
	if (_tag.size() == 0)
	{
		// check) tag가 붙어 있어야 한다.
		THROW_IF(_source.size() < TAG_SIZE, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "aes: source is too short (no tag)"sv));

		// - 마지막 16 byte가 tag
		_tag = buffer_view(const_cast<char*>(_source.data<char>()) + (_source.size() - TAG_SIZE), TAG_SIZE);
		_source = const_buffer_view(_source.data<char>(), _source.size() - TAG_SIZE);
	}

	// check) tag size
	THROW_IF(_tag.size() < TAG_SIZE_MIN || _tag.size() > TAG_SIZE, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "aes: invalid tag size"sv));

	// check) output buffer
	THROW_IF(_out.size() < _source.size(), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "aes: output buffer too small"sv));

	// check)
	THROW_IF(_source.size() > static_cast<std::size_t>(INT_MAX) || _source_add.size() > static_cast<std::size_t>(INT_MAX), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "aes: data too large"sv));

	// 2) create and initialise the context
	cipher_ctx context;
	if (context.ctx == nullptr)
		_throw_openssl_error("EVP_CIPHER_CTX_new");

	// declare)
	int plaintext_len = 0;

	// 3) initialise the decryption operation.
	if (EVP_DecryptInit_ex(context.ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1)
		_throw_openssl_error("EVP_DecryptInit_ex");

	// 4) set IV length.
	if (EVP_CIPHER_CTX_ctrl(context.ctx, EVP_CTRL_GCM_SET_IVLEN, _key.iv.size<int>(), nullptr) != 1)
		_throw_openssl_error("EVP_CTRL_GCM_SET_IVLEN");

	// 5) initialise key and IV
	if (EVP_DecryptInit_ex(context.ctx, nullptr, nullptr, _key.key.data<unsigned char>(), _key.iv.data<unsigned char>()) != 1)
		_throw_openssl_error("EVP_DecryptInit_ex(key)");

	// 6) Provide any AAD data.
	if (_source_add.size() != 0)
	{
		int len = 0;
		if (EVP_DecryptUpdate(context.ctx, nullptr, &len, _source_add.data<unsigned char>(), _source_add.size<int>()) != 1)
			_throw_openssl_error("EVP_DecryptUpdate(aad)");
	}

	// 7) provide the message to be decrypted, and obtain the plaintext output.
	if (_source.size() != 0)
	{
		int len = 0;
		if (EVP_DecryptUpdate(context.ctx, _out.data<unsigned char>(), &len, _source.data<unsigned char>(), _source.size<int>()) != 1)
			_throw_openssl_error("EVP_DecryptUpdate");

		plaintext_len = len;
	}

	// 8) Set expected tag value.
	if (EVP_CIPHER_CTX_ctrl(context.ctx, EVP_CTRL_GCM_SET_TAG, _tag.size<int>(), _tag.data<void>()) != 1)
		_throw_openssl_error("EVP_CTRL_GCM_SET_TAG");

	// 9) Finalise the decryption. (tag 검증. 실패하면 복호화한 내용은 신뢰할 수 없다.)
	{
		int len = 0;
		auto result = EVP_DecryptFinal_ex(context.ctx, _out.data<unsigned char>() + plaintext_len, &len);

		// check) 인증 실패 - 복호화한 내용을 지우고 실패를 던진다.
		if (result != 1)
		{
			OPENSSL_cleanse(_out.data<void>(), static_cast<std::size_t>(plaintext_len));
			ERR_clear_error();
			throw throwable::failure(eRESULT::FAIL, 0, "aes: authentication failed (tag mismatch)"sv);
		}

		plaintext_len += len;
	}

	// return)
	return _out ^ static_cast<std::size_t>(plaintext_len);
}

//-----------------------------------------------------------------------------
// cryptor (AES-256-GCM)
//
//  - 형식: [IV 12 byte][암호문][tag 16 byte]
//  - 암호화할 때마다 무작위 IV를 만든다. (같은 key로 IV를 재사용하면 GCM은 안전하지 않다.)
//-----------------------------------------------------------------------------
cryptor::~cryptor() noexcept
{
	OPENSSL_cleanse(this->m_key.data(), this->m_key.size());
}

void cryptor::initialize(buffer_view _key)
{
	// check) AES-256 key는 32 byte여야 한다.
	THROW_IF(_key.size() != KEY_SIZE, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "aes::cryptor: key must be 32 bytes"sv));

	// 1) key를 복사한다.
	memcpy(this->m_key.data(), _key.data(), KEY_SIZE);
	this->m_is_initialized = true;
}

shared_buffer cryptor::encrypt(const_buffer_view _source)
{
	// check)
	THROW_IF(!this->m_is_initialized, throwable::failure(eRESULT::NOT_READY, 0, "aes::cryptor: key is not set"sv));

	// 1) 무작위 IV
	uint8_t iv[IV_SIZE];
	THROW_IF(RAND_bytes(iv, static_cast<int>(IV_SIZE)) != 1, throwable::failure(eRESULT::FAIL, 0, "aes::cryptor: RAND_bytes failed"sv));

	// 2) 결과 buffer ([IV][암호문][tag])
	const std::size_t size_total = IV_SIZE + _source.size() + TAG_SIZE;
	auto buf_result = alloc_shared_buffer(size_total);
	memcpy(buf_result.data(), iv, IV_SIZE);

	// 3) 암호화 (tag는 암호문 뒤에 붙는다.)
	aes_key key{ buffer_view(reinterpret_cast<char*>(this->m_key.data()), KEY_SIZE), buffer_view(reinterpret_cast<char*>(iv), IV_SIZE) };
	auto result = crypt::aes::encrypt(buffer_view(buf_result.data() + IV_SIZE, _source.size() + TAG_SIZE), buffer_view(), key, _source, const_buffer_view());

	// 4) 크기를 설정한다.
	buf_result.set_size(IV_SIZE + result.size());

	// return)
	return buf_result;
}

shared_buffer cryptor::decrypt(const_buffer_view _source)
{
	// check)
	THROW_IF(!this->m_is_initialized, throwable::failure(eRESULT::NOT_READY, 0, "aes::cryptor: key is not set"sv));

	// check) [IV][tag]는 있어야 한다.
	THROW_IF(_source.size() < IV_SIZE + TAG_SIZE, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "aes::cryptor: source is too short"sv));

	// 1) IV
	aes_key key{ buffer_view(reinterpret_cast<char*>(this->m_key.data()), KEY_SIZE), buffer_view(const_cast<char*>(_source.data<char>()), IV_SIZE) };

	// 2) 결과 buffer
	const std::size_t size_plain = _source.size() - IV_SIZE - TAG_SIZE;
	auto buf_result = alloc_shared_buffer(__select_max<std::size_t>(size_plain, 1));

	// 3) 복호화 (tag 검증에 실패하면 exception)
	auto result = crypt::aes::decrypt(buffer_view(buf_result.data(), size_plain), buffer_view(), key, const_buffer_view(_source.data<char>() + IV_SIZE, _source.size() - IV_SIZE), const_buffer_view());

	// 4) 크기를 설정한다.
	buf_result.set_size(result.size());

	// return)
	return buf_result;
}

object_ptr<Icryptor> make_cryptor(const_buffer_view _key)
{
	auto pcryptor = make_object<cryptor>();
	pcryptor->initialize(buffer_view(const_cast<char*>(_key.data<char>()), _key.size()));
	return pcryptor;
}

shared_buffer generate_key()
{
	auto buf_key = alloc_shared_buffer(cryptor::KEY_SIZE);
	THROW_IF(RAND_bytes(reinterpret_cast<unsigned char*>(buf_key.data()), static_cast<int>(cryptor::KEY_SIZE)) != 1, throwable::failure(eRESULT::FAIL, 0, "aes: RAND_bytes failed"sv));
	buf_key.set_size(cryptor::KEY_SIZE);
	return buf_key;
}


}
}
}
