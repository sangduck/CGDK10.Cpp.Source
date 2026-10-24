#include "pch.h"
#include "openssl/hmac.h"


namespace CGDK
{

buffer_view crypt::hmac_md5(buffer_view _output, const_buffer_view _key, const_buffer_view _source) noexcept
{
	// 1) 
	unsigned int output_length = _output.size<unsigned int>();

	// check) 출력 버퍼는 digest 크기 이상이어야 한다. (HMAC은 digest 전체를 쓴다.)
	RETURN_IF(_output.size() < static_cast<std::size_t>(EVP_MD_get_size(EVP_md5())), buffer_view());

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
	auto result = hmac_md5(buf_output, _key, _source);

	// check)
	THROW_IF(result.size() == 0, throwable::failure(eRESULT::FAIL, 0, "hmac failed"sv));

	buf_output ^= result.size();

	// return)
	return buf_output;
}

buffer_view crypt::hmac_sha1(buffer_view _output, const_buffer_view _key, const_buffer_view _source) noexcept
{
	unsigned int output_length = _output.size<unsigned int>();

	// check) 출력 버퍼는 digest 크기 이상이어야 한다. (HMAC은 digest 전체를 쓴다.)
	RETURN_IF(_output.size() < static_cast<std::size_t>(EVP_MD_get_size(EVP_sha1())), buffer_view());

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
	auto result = hmac_sha1(buf_output, _key, _source);

	// check)
	THROW_IF(result.size() == 0, throwable::failure(eRESULT::FAIL, 0, "hmac failed"sv));

	buf_output ^= result.size();

	// return)
	return buf_output;
}

buffer_view crypt::hmac_sha224(buffer_view _output, const_buffer_view _key, const_buffer_view _source) noexcept
{
	unsigned int output_length = _output.size<unsigned int>();

	// check) 출력 버퍼는 digest 크기 이상이어야 한다. (HMAC은 digest 전체를 쓴다.)
	RETURN_IF(_output.size() < static_cast<std::size_t>(EVP_MD_get_size(EVP_sha224())), buffer_view());

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
	auto result = hmac_sha224(buf_output, _key, _source);

	// check)
	THROW_IF(result.size() == 0, throwable::failure(eRESULT::FAIL, 0, "hmac failed"sv));

	buf_output ^= result.size();

	// return)
	return buf_output;
}

buffer_view crypt::hmac_sha256(buffer_view _output, const_buffer_view _key, const_buffer_view _source) noexcept
{
	unsigned int output_length = _output.size<unsigned int>();

	// check) 출력 버퍼는 digest 크기 이상이어야 한다. (HMAC은 digest 전체를 쓴다.)
	RETURN_IF(_output.size() < static_cast<std::size_t>(EVP_MD_get_size(EVP_sha256())), buffer_view());

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
	auto result = hmac_sha256(buf_output, _key, _source);

	// check)
	THROW_IF(result.size() == 0, throwable::failure(eRESULT::FAIL, 0, "hmac failed"sv));

	buf_output ^= result.size();

	// return)
	return buf_output;
}

buffer_view crypt::hmac_sha384(buffer_view _output, const_buffer_view _key, const_buffer_view _source) noexcept
{
	unsigned int output_length = _output.size<unsigned int>();

	// check) 출력 버퍼는 digest 크기 이상이어야 한다. (HMAC은 digest 전체를 쓴다.)
	RETURN_IF(_output.size() < static_cast<std::size_t>(EVP_MD_get_size(EVP_sha384())), buffer_view());

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
	auto result = hmac_sha384(buf_output, _key, _source);

	// check)
	THROW_IF(result.size() == 0, throwable::failure(eRESULT::FAIL, 0, "hmac failed"sv));

	buf_output ^= result.size();

	// return)
	return buf_output;
}

buffer_view crypt::hmac_sha512(buffer_view _output, const_buffer_view _key, const_buffer_view _source) noexcept
{
	unsigned int output_length = _output.size<unsigned int>();

	// check) 출력 버퍼는 digest 크기 이상이어야 한다. (HMAC은 digest 전체를 쓴다.)
	RETURN_IF(_output.size() < static_cast<std::size_t>(EVP_MD_get_size(EVP_sha512())), buffer_view());

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
	auto result = hmac_sha512(buf_output, _key, _source);

	// check)
	THROW_IF(result.size() == 0, throwable::failure(eRESULT::FAIL, 0, "hmac failed"sv));

	// return)
	buf_output ^= result.size();
	return buf_output;
}

}
