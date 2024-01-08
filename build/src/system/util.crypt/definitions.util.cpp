#include "pch.h"

namespace CGDK
{

#if !defined(__linux__)
namespace crypt
{
	bool sha224(buffer_view _output, const_buffer_view _source) noexcept;
	bool sha256(buffer_view _output, const_buffer_view _source) noexcept;
	bool sha384(buffer_view _output, const_buffer_view _source) noexcept;
	bool sha512(buffer_view _output, const_buffer_view _source) noexcept;
}

buffer_view _append_sha224(basic_buffer& _source, const sha224& _data)
{
	// check) 새로 추가되는 크기는 Upper Bound를 넘어서는 안됀다.
	_CGD_BUFFER_BOUND_CHECK((_source.data() + _source.size() + sizeof(COUNT_T) + crypt::DIGEST_LENGTH_SHA224) <= _source.get_upper_bound());

	// declare)
	char* ptr_dest = _source.data() + _source.size();

	// 1) sha256값을 써넣는다.
	auto result = crypt::sha224(buffer_view{ ptr_dest + sizeof(COUNT_T), crypt::DIGEST_LENGTH_SHA224 }, _data.str_source);

	// check) 
	THROW_IF(result == false, throwable::failure(eRESULT::FAIL));

	// 2) 메시지 길이를 써넣는다.
	*(COUNT_T*)ptr_dest = static_cast<COUNT_T>(crypt::DIGEST_LENGTH_SHA224);

	// 3) 길이를 늘려준다.
	_source.add_size(crypt::DIGEST_LENGTH_SHA224 + sizeof(COUNT_T));

	// return) 
	return buffer_view(ptr_dest, crypt::DIGEST_LENGTH_SHA224 + sizeof(COUNT_T));
}

buffer_view _append_sha256(basic_buffer& _source, const sha256& _data)
{
	// check) 새로 추가되는 크기는 Upper Bound를 넘어서는 안됀다.
	_CGD_BUFFER_BOUND_CHECK((_source.data() + _source.size() + sizeof(COUNT_T) + crypt::DIGEST_LENGTH_SHA256) <= _source.get_upper_bound());

	// declare)
	char* ptr_dest = _source.data() + _source.size();

	// 1) sha256값을 써넣는다.
	auto result = crypt::sha256(buffer_view{ ptr_dest + sizeof(COUNT_T), crypt::DIGEST_LENGTH_SHA256 }, _data.str_source);

	// check) 
	THROW_IF(result == false, throwable::failure(eRESULT::FAIL));

	// 2) 메시지 길이를 써넣는다.
	*(COUNT_T*)ptr_dest = static_cast<COUNT_T>(crypt::DIGEST_LENGTH_SHA256);

	// 3) 길이를 늘려준다.
	_source.add_size(crypt::DIGEST_LENGTH_SHA256 + sizeof(COUNT_T));

	// return) 
	return buffer_view(ptr_dest, crypt::DIGEST_LENGTH_SHA256 + sizeof(COUNT_T));
}

buffer_view _append_sha384(basic_buffer& _source, const sha384& _data)
{
	// check) 새로 추가되는 크기는 Upper Bound를 넘어서는 안됀다.
	_CGD_BUFFER_BOUND_CHECK((_source.data() + _source.size() + sizeof(COUNT_T) + crypt::DIGEST_LENGTH_SHA384) <= _source.get_upper_bound());

	// declare)
	char* ptr_dest = _source.data() + _source.size();

	// 1) sha256값을 써넣는다.
	auto result = crypt::sha384(buffer_view{ ptr_dest + sizeof(COUNT_T), crypt::DIGEST_LENGTH_SHA384 }, _data.str_source);

	// check) 
	THROW_IF(result == false, throwable::failure(eRESULT::FAIL));

	// 2) 메시지 길이를 써넣는다.
	*(COUNT_T*)ptr_dest = static_cast<COUNT_T>(crypt::DIGEST_LENGTH_SHA384);

	// 3) 길이를 늘려준다.
	_source.add_size(crypt::DIGEST_LENGTH_SHA384 + sizeof(COUNT_T));

	// return) 
	return buffer_view(ptr_dest, crypt::DIGEST_LENGTH_SHA384 + sizeof(COUNT_T));
}

buffer_view _append_sha512(basic_buffer& _source, const sha512& _data)
{
	// check) 새로 추가되는 크기는 Upper Bound를 넘어서는 안됀다.
	_CGD_BUFFER_BOUND_CHECK((_source.data() + _source.size() + sizeof(COUNT_T) + crypt::DIGEST_LENGTH_SHA512) <= _source.get_upper_bound());

	// declare)
	char* ptr_dest = _source.data() + _source.size();

	// 1) sha256값을 써넣는다.
	auto result = crypt::sha512(buffer_view{ ptr_dest + sizeof(COUNT_T), crypt::DIGEST_LENGTH_SHA512 }, _data.str_source);

	// check) 
	THROW_IF(result == false, throwable::failure(eRESULT::FAIL));

	// 2) 메시지 길이를 써넣는다.
	*(COUNT_T*)ptr_dest = static_cast<COUNT_T>(crypt::DIGEST_LENGTH_SHA512);

	// 3) 길이를 늘려준다.
	_source.add_size(crypt::DIGEST_LENGTH_SHA512 + sizeof(COUNT_T));

	// return) 
	return buffer_view(ptr_dest, crypt::DIGEST_LENGTH_SHA512 + sizeof(COUNT_T));
}

std::string _extract_sha224(buffer_view _source)
{
	return _source._extract_string<char>();
}

std::string _extract_sha256(buffer_view _source)
{
	return _source._extract_string<char>();
}

std::string _extract_sha384(buffer_view _source)
{
	return _source._extract_string<char>();
}

std::string _extract_sha512(buffer_view _source)
{
	return _source._extract_string<char>();
}

std::string _front_sha224(buffer_view _source, int64_t& _offset)
{
	return _source._front_string<char>(_offset);
}

std::string _front_sha256(buffer_view _source, int64_t& _offset)
{
	return _source._front_string<char>(_offset);
}

std::string _front_sha384(buffer_view _source, int64_t& _offset)
{
	return _source._front_string<char>(_offset);
}

std::string _front_sha512(buffer_view _source, int64_t& _offset)
{
	return _source._front_string<char>(_offset);
}

#endif


}