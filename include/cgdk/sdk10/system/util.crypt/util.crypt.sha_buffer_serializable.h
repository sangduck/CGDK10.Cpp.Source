//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                          Utinity.Crypto Classes                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
//*  Best for Game Developement and Optimized for Game Developement.          *
//*                                                                           *
//*                (c) 2003. Cho Sanghyun. All right reserved.                *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************

#pragma once

namespace CGDK
{

struct sha224
{
public:
	sha224() noexcept : str_source(nullptr) {}
	sha224(const char* _str_source) noexcept : str_source(_str_source) {}

public:
	const char* str_source;
};

struct sha256
{
public:
	sha256() noexcept : str_source(nullptr) {}
	sha256(const char* _str_source) noexcept : str_source(_str_source) {}

public:
	const char* str_source;
};

struct sha384
{
public:
	sha384() noexcept : str_source(nullptr) {}
	sha384(const char* _str_source) noexcept : str_source(_str_source) {}

public:
	const char* str_source;
};

struct sha512
{
public:
	sha512() noexcept : str_source(nullptr) {}
	sha512(const char* _str_source) noexcept : str_source(_str_source) {}

public:
	const char* str_source;
};

buffer_view		_append_sha224		(buffer& _source, const sha224& _data);
buffer_view		_append_sha256		(buffer& _source, const sha256& _data);
buffer_view		_append_sha384		(buffer& _source, const sha384& _data);
buffer_view		_append_sha512		(buffer& _source, const sha512& _data);

// - append한 형식([COUNT_T 길이][digest])을 그대로 읽는다. (digest는 binary이므로 NUL 종단 문자열로 읽으면 안된다.)
template<class S>
std::string		_extract_sha_digest(S& _s, std::size_t _length)
{
	// check)
	if (static_cast<std::size_t>(_s.size()) < sizeof(COUNT_T) + _length)
		throw throwable::failure(eRESULT::INVALID_ARGUMENT, 0, std::string_view("sha: buffer too short"));

	// 1) length
	COUNT_T count = 0;
	memcpy(&count, _s.data(), sizeof(COUNT_T));

	// check)
	if (count != static_cast<COUNT_T>(_length))
		throw throwable::failure(eRESULT::INVALID_ARGUMENT, 0, std::string_view("sha: invalid digest length"));

	// 2) digest
	std::string result(reinterpret_cast<const char*>(_s.data()) + sizeof(COUNT_T), _length);

	// 3) advance
	_s += offset(sizeof(COUNT_T) + _length);

	// return)
	return result;
}

template<class S>
std::string		_front_sha_digest(const S& _s, int64_t& _offset, std::size_t _length)
{
	// check)
	if (_offset < 0 || static_cast<std::size_t>(_s.size()) < static_cast<std::size_t>(_offset) + sizeof(COUNT_T) + _length)
		throw throwable::failure(eRESULT::INVALID_ARGUMENT, 0, std::string_view("sha: buffer too short"));

	// 1) length
	COUNT_T count = 0;
	memcpy(&count, reinterpret_cast<const char*>(_s.data()) + _offset, sizeof(COUNT_T));

	// check)
	if (count != static_cast<COUNT_T>(_length))
		throw throwable::failure(eRESULT::INVALID_ARGUMENT, 0, std::string_view("sha: invalid digest length"));

	// 2) digest
	std::string result(reinterpret_cast<const char*>(_s.data()) + _offset + sizeof(COUNT_T), _length);

	// 3) advance offset
	_offset += static_cast<int64_t>(sizeof(COUNT_T) + _length);

	// return)
	return result;
}

// class for PUSH_BACK)
template<class B> class serializer_append<B, sha224> { public: using type = _buffer_view<typename B::element_t>; template<class S> static type _do_append(S& _s, const sha224& _data) { return _append_sha224(_s, _data);} };
template<class B> class serializer_append<B, sha256> { public: using type = _buffer_view<typename B::element_t>; template<class S> static type _do_append(S& _s, const sha256& _data) { return _append_sha256(_s, _data);} };
template<class B> class serializer_append<B, sha384> { public: using type = _buffer_view<typename B::element_t>; template<class S> static type _do_append(S& _s, const sha384& _data) { return _append_sha384(_s, _data);} };
template<class B> class serializer_append<B, sha512> { public: using type = _buffer_view<typename B::element_t>; template<class S> static type _do_append(S& _s, const sha512& _data) { return _append_sha512(_s, _data);} };
// class for POP_FRONT)								   		  		   
template<class B> class serializer_extract<B, sha224> { public: using type = std::string; template<class S> static type _do_extract(S& _s) { return _extract_sha_digest(_s, crypt::DIGEST_LENGTH_SHA224); } template<class S> static void _do_extract(type& _dest, S& _s) { _dest = _extract_sha_digest(_s, crypt::DIGEST_LENGTH_SHA224);} };
template<class B> class serializer_extract<B, sha256> { public: using type = std::string; template<class S> static type _do_extract(S& _s) { return _extract_sha_digest(_s, crypt::DIGEST_LENGTH_SHA256); } template<class S> static void _do_extract(type& _dest, S& _s) { _dest = _extract_sha_digest(_s, crypt::DIGEST_LENGTH_SHA256);} };
template<class B> class serializer_extract<B, sha384> { public: using type = std::string; template<class S> static type _do_extract(S& _s) { return _extract_sha_digest(_s, crypt::DIGEST_LENGTH_SHA384); } template<class S> static void _do_extract(type& _dest, S& _s) { _dest = _extract_sha_digest(_s, crypt::DIGEST_LENGTH_SHA384);} };
template<class B> class serializer_extract<B, sha512> { public: using type = std::string; template<class S> static type _do_extract(S& _s) { return _extract_sha_digest(_s, crypt::DIGEST_LENGTH_SHA512); } template<class S> static void _do_extract(type& _dest, S& _s) { _dest = _extract_sha_digest(_s, crypt::DIGEST_LENGTH_SHA512);} };
// class for front)									   		  		   
template<class B> class serializer_peek<B, sha224> { public: using type = std::string; template<class S> static type _do_peek(const S& _s, int64_t& _offset) { return _front_sha_digest(_s, _offset, crypt::DIGEST_LENGTH_SHA224); } template<class S> static void _do_peek(type& _dest, const S& _s, int64_t& _offset) { _dest = _front_sha_digest(_s, _offset, crypt::DIGEST_LENGTH_SHA224);} };
template<class B> class serializer_peek<B, sha256> { public: using type = std::string; template<class S> static type _do_peek(const S& _s, int64_t& _offset) { return _front_sha_digest(_s, _offset, crypt::DIGEST_LENGTH_SHA256); } template<class S> static void _do_peek(type& _dest, const S& _s, int64_t& _offset) { _dest = _front_sha_digest(_s, _offset, crypt::DIGEST_LENGTH_SHA256);} };
template<class B> class serializer_peek<B, sha384> { public: using type = std::string; template<class S> static type _do_peek(const S& _s, int64_t& _offset) { return _front_sha_digest(_s, _offset, crypt::DIGEST_LENGTH_SHA384); } template<class S> static void _do_peek(type& _dest, const S& _s, int64_t& _offset) { _dest = _front_sha_digest(_s, _offset, crypt::DIGEST_LENGTH_SHA384);} };
template<class B> class serializer_peek<B, sha512> { public: using type = std::string; template<class S> static type _do_peek(const S& _s, int64_t& _offset) { return _front_sha_digest(_s, _offset, crypt::DIGEST_LENGTH_SHA512); } template<class S> static void _do_peek(type& _dest, const S& _s, int64_t& _offset) { _dest = _front_sha_digest(_s, _offset, crypt::DIGEST_LENGTH_SHA512);} };
													   		  
// class for get_size_of)								   		  
template<> class serializer_size_of<sha224> 
{ public:
	template <class T>
	constexpr static std::size_t _get_append_size(const _basic_buffer<T>&) { return crypt::DIGEST_LENGTH_SHA224 + sizeof(COUNT_T);}
	template<class S> 
	constexpr static std::size_t _get_extract_size(const S& _buffer, int64_t& _offset) { std::size_t size = sizeof(COUNT_T) + crypt::DIGEST_LENGTH_SHA224; if ((_buffer.size() - _offset) < size) throw std::exception(); _offset += size; return size; }
};
template<> class serializer_size_of<sha256>
{ public:
	template <class T>
	constexpr static std::size_t _get_append_size(const _basic_buffer<T>&) { return crypt::DIGEST_LENGTH_SHA256 + sizeof(COUNT_T);}
	template<class S> 
	constexpr static std::size_t _get_extract_size(const S& _buffer, int64_t& _offset) { std::size_t size = sizeof(COUNT_T) + crypt::DIGEST_LENGTH_SHA256; if ((_buffer.size() - _offset) < size) throw std::exception(); _offset += size; return size; }
};
template<> class serializer_size_of<sha384>
{ public:
	template <class T>
	constexpr static std::size_t _get_append_size(const _basic_buffer<T>&) { return crypt::DIGEST_LENGTH_SHA384 + sizeof(COUNT_T);}
	template<class S> 
	constexpr static std::size_t _get_extract_size(const S& _buffer, int64_t& _offset) { std::size_t size = sizeof(COUNT_T) + crypt::DIGEST_LENGTH_SHA384; if ((_buffer.size() - _offset) < size) throw std::exception(); _offset += size; return size; }
};
template<> class serializer_size_of<sha512>
{ public: 
	template <class T>
	constexpr static std::size_t _get_append_size(const _basic_buffer<T>&) { return crypt::DIGEST_LENGTH_SHA512 + sizeof(COUNT_T);}
	template<class S> 
	constexpr static std::size_t _get_extract_size(const S& _buffer, int64_t& _offset) { std::size_t size = sizeof(COUNT_T) + crypt::DIGEST_LENGTH_SHA512; if ((_buffer.size() - _offset) < size) throw std::exception(); _offset += size; return size; }
};

}
