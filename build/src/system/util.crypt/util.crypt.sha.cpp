#include "pch.h"
#include <openssl/sha.h>


namespace CGDK
{
	// - sha 직렬화의 원문 (const char* -> 길이가 있는 view. nullptr이면 빈 문자열)
	static const_buffer_view _to_sha_source(const char* _str_source) noexcept
	{
		if (_str_source == nullptr)
			return const_buffer_view();

		std::string_view str(_str_source);
		return const_buffer_view(str.data(), str.size());
	}

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
		CGASSERT_ERROR(_output.size() >= SHA256_DIGEST_LENGTH);

		// check) (SHA256은 32 byte를 쓴다.)
		RETURN_IF(_output.size() < SHA256_DIGEST_LENGTH, false);

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

	buffer_view _append_sha224(buffer& _source, const sha224& _data)
	{
		// check) 새로 추가되는 크기는 Upper Bound를 넘어서는 안 된다.
		_CGD_BUFFER_BOUND_CHECK((_source.data() + _source.size() + sizeof(COUNT_T) + crypt::DIGEST_LENGTH_SHA224) <= _source.get_upper_bound());

		// declare)
		char* ptr_dest = _source.data() + _source.size();

		// 1) sha256값을 써넣는다.
		auto result = crypt::sha224(buffer_view{ ptr_dest + sizeof(COUNT_T), crypt::DIGEST_LENGTH_SHA224 }, _to_sha_source(_data.str_source));

		// check) 
		THROW_IF(result == false, throwable::failure(eRESULT::FAIL));

		// 2) 메시지 길이를 써넣는다.
		*(COUNT_T*)ptr_dest = static_cast<COUNT_T>(crypt::DIGEST_LENGTH_SHA224);

		// 3) 길이를 늘려준다.
		_source.add_size(crypt::DIGEST_LENGTH_SHA224 + sizeof(COUNT_T));

		// return) 
		return buffer_view(ptr_dest, crypt::DIGEST_LENGTH_SHA224 + sizeof(COUNT_T));
	}

	buffer_view _append_sha256(buffer& _source, const sha256& _data)
	{
		// check) 새로 추가되는 크기는 Upper Bound를 넘어서는 안 된다.
		_CGD_BUFFER_BOUND_CHECK((_source.data() + _source.size() + sizeof(COUNT_T) + crypt::DIGEST_LENGTH_SHA256) <= _source.get_upper_bound());

		// declare)
		char* ptr_dest = _source.data() + _source.size();

		// 1) sha256값을 써넣는다.
		auto result = crypt::sha256(buffer_view{ ptr_dest + sizeof(COUNT_T), crypt::DIGEST_LENGTH_SHA256 }, _to_sha_source(_data.str_source));

		// check) 
		THROW_IF(result == false, throwable::failure(eRESULT::FAIL));

		// 2) 메시지 길이를 써넣는다.
		*(COUNT_T*)ptr_dest = static_cast<COUNT_T>(crypt::DIGEST_LENGTH_SHA256);

		// 3) 길이를 늘려준다.
		_source.add_size(crypt::DIGEST_LENGTH_SHA256 + sizeof(COUNT_T));

		// return) 
		return buffer_view(ptr_dest, crypt::DIGEST_LENGTH_SHA256 + sizeof(COUNT_T));
	}

	buffer_view _append_sha384(buffer& _source, const sha384& _data)
	{
		// check) 새로 추가되는 크기는 Upper Bound를 넘어서는 안 된다.
		_CGD_BUFFER_BOUND_CHECK((_source.data() + _source.size() + sizeof(COUNT_T) + crypt::DIGEST_LENGTH_SHA384) <= _source.get_upper_bound());

		// declare)
		char* ptr_dest = _source.data() + _source.size();

		// 1) sha256값을 써넣는다.
		auto result = crypt::sha384(buffer_view{ ptr_dest + sizeof(COUNT_T), crypt::DIGEST_LENGTH_SHA384 }, _to_sha_source(_data.str_source));

		// check) 
		THROW_IF(result == false, throwable::failure(eRESULT::FAIL));

		// 2) 메시지 길이를 써넣는다.
		*(COUNT_T*)ptr_dest = static_cast<COUNT_T>(crypt::DIGEST_LENGTH_SHA384);

		// 3) 길이를 늘려준다.
		_source.add_size(crypt::DIGEST_LENGTH_SHA384 + sizeof(COUNT_T));

		// return) 
		return buffer_view(ptr_dest, crypt::DIGEST_LENGTH_SHA384 + sizeof(COUNT_T));
	}

	buffer_view _append_sha512(buffer& _source, const sha512& _data)
	{
		// check) 새로 추가되는 크기는 Upper Bound를 넘어서는 안 된다.
		_CGD_BUFFER_BOUND_CHECK((_source.data() + _source.size() + sizeof(COUNT_T) + crypt::DIGEST_LENGTH_SHA512) <= _source.get_upper_bound());

		// declare)
		char* ptr_dest = _source.data() + _source.size();

		// 1) sha256값을 써넣는다.
		auto result = crypt::sha512(buffer_view{ ptr_dest + sizeof(COUNT_T), crypt::DIGEST_LENGTH_SHA512 }, _to_sha_source(_data.str_source));

		// check) 
		THROW_IF(result == false, throwable::failure(eRESULT::FAIL));

		// 2) 메시지 길이를 써넣는다.
		*(COUNT_T*)ptr_dest = static_cast<COUNT_T>(crypt::DIGEST_LENGTH_SHA512);

		// 3) 길이를 늘려준다.
		_source.add_size(crypt::DIGEST_LENGTH_SHA512 + sizeof(COUNT_T));

		// return) 
		return buffer_view(ptr_dest, crypt::DIGEST_LENGTH_SHA512 + sizeof(COUNT_T));
	}

}
