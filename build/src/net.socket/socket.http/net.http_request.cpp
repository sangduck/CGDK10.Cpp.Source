#include "pch.h"
#include <charconv>
#include <cstring>
#include <random>

#if defined(_WIN32)
	#include <shlwapi.h>
#elif defined(__linux__)
#else
#endif

using namespace CGDK;

#define	STRING_HTTP_PORT	"80"sv
#define	STRING_HTTPS_PORT	"443"sv

struct URL_COMPONENT
{
public:
	eHTTP_SCHEME_TYPE	scheme = eHTTP_SCHEME_TYPE::NOT_DEFINED_;
	std::string_view	str_scheme;
	std::string_view	str_port;
	std::string_view	str_hostname;
	std::string_view	str_object;
	std::string_view	str_optional;
	std::string_view	str_path_query;		// - the path and the query (the object of GET)
};

bool _is_use_ssl(eHTTP_SCHEME_TYPE _scheme)  noexcept
{
	return	(_scheme == eHTTP_SCHEME_TYPE::HTTPS_) || (_scheme == eHTTP_SCHEME_TYPE::WSS_);
}

eHTTP_SCHEME_TYPE _get_scheme_type(const std::string_view _str_scheme) noexcept
{
	// declare) 
	eHTTP_SCHEME_TYPE result = eHTTP_SCHEME_TYPE::NOT_DEFINED_;

	// 1) 
	auto pos  = _str_scheme.data();
	auto size = _str_scheme.size();

	// check)
	if(size < 2)
		return result;

	switch (*pos)
	{
	// case ftp
	case	'f':
	case	'F':
			if (size == 3 
			&& (pos[1] == 't' || pos[1] == 'T')
			&& (pos[2] == 'p' || pos[2] == 'P')
			)
			{
				result = eHTTP_SCHEME_TYPE::FTP_;
			}
			break;

	// case gopher
	case	'g':
	case	'G':
			if (size == 6)
			{
				if((pos[1] == 'o' || pos[1] == 'O')
				&& (pos[2] == 'p' || pos[2] == 'P')
				&& (pos[3] == 'h' || pos[3] == 'H')
				&& (pos[4] == 'e' || pos[4] == 'E')
				&& (pos[5] == 'r' || pos[5] == 'R')
				)
				{
					result = eHTTP_SCHEME_TYPE::GOPHER_;
				}
			}
			break;

	// case http/https
	case	'h':
	case	'H':
			if (size == 4)
			{
				if((pos[1] == 't' || pos[1] == 'T')
				&& (pos[2] == 't' || pos[2] == 'T')
				&& (pos[3] == 'p' || pos[3] == 'P')
				)
				{
					result = eHTTP_SCHEME_TYPE::HTTP_;
				}
			}
			else if (size == 5)
			{
				if((pos[1] == 't' || pos[1] == 'T')
				&& (pos[2] == 't' || pos[2] == 'T')
				&& (pos[3] == 'p' || pos[3] == 'P')
				&& (pos[4] == 's' || pos[4] == 'S')
				)
				{
					result = eHTTP_SCHEME_TYPE::HTTPS_;
				}
			}
			break;

	// case news
	case	'n':
	case	'N':
			if (size == 4)
			{
				if((pos[1] == 'e' || pos[1] == 'E')
				&& (pos[2] == 'w' || pos[2] == 'W')
				&& (pos[3] == 's' || pos[3] == 'S')
				)
				{
					result = eHTTP_SCHEME_TYPE::NEWS_;
				}
			}
			break;

	// "javascript"
	case	'j':
	case	'J':
			if (size == 10)
			{
				if((pos[1] == 'a' || pos[1] == 'A')
				&& (pos[2] == 'v' || pos[2] == 'V')
				&& (pos[3] == 'a' || pos[3] == 'A')
				&& (pos[4] == 's' || pos[4] == 'S')
				&& (pos[5] == 'c' || pos[5] == 'C')
				&& (pos[6] == 'r' || pos[6] == 'R')
				&& (pos[7] == 'i' || pos[7] == 'I')
				&& (pos[8] == 'p' || pos[8] == 'P')
				&& (pos[9] == 't' || pos[9] == 'T')
				)
				{
					result = eHTTP_SCHEME_TYPE::JAVASCRIPT_;
				}
			}
			break;

	// case "mailto"
	case	'm':
	case	'M':
			if (size == 6)
			{
				if((pos[1] == 'a' || pos[1] == 'A')
				&& (pos[2] == 'i' || pos[2] == 'I')
				&& (pos[3] == 'l' || pos[3] == 'L')
				&& (pos[4] == 't' || pos[4] == 'T')
				&& (pos[5] == 'o' || pos[5] == 'O')
				)
				{
					result = eHTTP_SCHEME_TYPE::MAILTO_;
				}
			}
			break;

	// case "res"
	case	'r':
	case	'R':
			if (size == 3)
			{
				if((pos[1] == 'e' || pos[1] == 'E')
				&& (pos[2] == 's' || pos[2] == 'S')
				)
				{
					result = eHTTP_SCHEME_TYPE::RES_;
				}
			}
			break;


	// case 'socks'
	case	's':
	case	'S':
			if (size == 5)
			{
				if((pos[1] == 'o' || pos[1] == 'O')
				&& (pos[2] == 'c' || pos[2] == 'C')
				&& (pos[3] == 'k' || pos[3] == 'K')
				&& (pos[4] == 's' || pos[4] == 'S')
				)
				{
					result = eHTTP_SCHEME_TYPE::SOCKS_;
				}
			}
			break;


	// case "vbscript"
	case	'v':
	case	'V':
			if (size == 8)
			{
				if((pos[1] == 'b' || pos[1] == 'B')
				&& (pos[2] == 's' || pos[2] == 'S')
				&& (pos[3] == 'c' || pos[3] == 'C')
				&& (pos[4] == 'r' || pos[4] == 'R')
				&& (pos[5] == 'i' || pos[5] == 'I')
				&& (pos[6] == 'p' || pos[6] == 'P')
				&& (pos[7] == 't' || pos[7] == 'T')
				)
				{
					result = eHTTP_SCHEME_TYPE::VBSCRIPT_;
				}
			}
			break;

	// case "wss" / "ws"
	case	'w':
	case	'W':
			switch(size)
			{
			case	2:
					if((pos[1] == 's' || pos[1] == 'S')
					)
					{
						result = eHTTP_SCHEME_TYPE::WS_;
					}
					break;

			case	3:
					if ((pos[1] == 's' || pos[1] == 'S')
					 && (pos[2] == 's' || pos[2] == 'S')
					)
					{
						result = eHTTP_SCHEME_TYPE::WSS_;
					}
					break;
			}
			break;
	}

	// return)
	return	result;
}

// - "scheme://host[:port][/path][?query][#fragment]" (the host can be "[ipv6]")
//   str_object : the path ("/" if empty), str_optional : the query (without '?'), the fragment is not sent
bool _crack_url(URL_COMPONENT& _url_parsed, const std::string_view _str_url)
{
	// declare)
	std::string_view rest = _str_url;

	// - trim the spaces
	while (rest.empty() == false && (rest.front() == ' ' || rest.front() == '\t'))
		rest.remove_prefix(1);
	while (rest.empty() == false && (rest.back() == ' ' || rest.back() == '\t' || rest.back() == '\r' || rest.back() == '\n'))
		rest.remove_suffix(1);

	// 1) scheme ("scheme:")
	if (const auto pos_scheme = rest.find(':'); pos_scheme != std::string_view::npos && rest.substr(0, pos_scheme).find('/') == std::string_view::npos)
	{
		_url_parsed.str_scheme = rest.substr(0, pos_scheme);
		_url_parsed.scheme = _get_scheme_type(_url_parsed.str_scheme);

		// check)
		if (_url_parsed.scheme == eHTTP_SCHEME_TYPE::NOT_DEFINED_)
			throw throwable::failure(eRESULT::INVALID_ARGUMENT);

		// - the default port
		_url_parsed.str_port = _is_use_ssl(_url_parsed.scheme) ? STRING_HTTPS_PORT : STRING_HTTP_PORT;

		rest.remove_prefix(pos_scheme + 1);
	}

	// 2) authority ("//host[:port]")
	if (rest.size() >= 2 && rest[0] == '/' && rest[1] == '/')
	{
		rest.remove_prefix(2);

		const auto pos_end = rest.find_first_of("/?# \t"sv);
		auto authority = rest.substr(0, pos_end);
		rest = (pos_end == std::string_view::npos) ? std::string_view() : rest.substr(pos_end);

		// - user info ("user:password@") is not used
		if (const auto pos_at = authority.rfind('@'); pos_at != std::string_view::npos)
			authority.remove_prefix(pos_at + 1);

		// - "[ipv6]:port"
		if (authority.empty() == false && authority.front() == '[')
		{
			const auto pos_close = authority.find(']');
			if (pos_close == std::string_view::npos)
				throw throwable::failure(eRESULT::INVALID_ARGUMENT);

			_url_parsed.str_hostname = authority.substr(1, pos_close - 1);
			authority.remove_prefix(pos_close + 1);

			if (authority.empty() == false)
			{
				if (authority.front() != ':')
					throw throwable::failure(eRESULT::INVALID_ARGUMENT);
				_url_parsed.str_port = authority.substr(1);
			}
		}
		// - "host:port"
		else
		{
			const auto pos_colon = authority.find(':');
			_url_parsed.str_hostname = authority.substr(0, pos_colon);
			if (pos_colon != std::string_view::npos)
				_url_parsed.str_port = authority.substr(pos_colon + 1);
		}
	}

	// 3) path, query, fragment (a space ends the url)
	rest = rest.substr(0, rest.find_first_of(" \t"sv));
	rest = rest.substr(0, rest.find('#'));

	const auto pos_query = rest.find('?');
	_url_parsed.str_object = rest.substr(0, pos_query);
	_url_parsed.str_optional = (pos_query == std::string_view::npos) ? std::string_view() : rest.substr(pos_query + 1);
	_url_parsed.str_path_query = rest;

	// return)
	return true;
}

eHTTP_METHOD _get_default_HTTP_method(eHTTP_SCHEME_TYPE _scheme) noexcept
{
	// declare) 
	eHTTP_METHOD method;

	switch (_scheme)
	{
	case	eHTTP_SCHEME_TYPE::FTP_:		method = eHTTP_METHOD::GET_; break;
	case	eHTTP_SCHEME_TYPE::GOPHER_:		method = eHTTP_METHOD::GET_; break;
	case	eHTTP_SCHEME_TYPE::HTTP_:		method = eHTTP_METHOD::GET_; break;
	case	eHTTP_SCHEME_TYPE::HTTPS_:		method = eHTTP_METHOD::GET_; break;
	case	eHTTP_SCHEME_TYPE::FILE_:		method = eHTTP_METHOD::GET_; break;
	case	eHTTP_SCHEME_TYPE::NEWS_:		method = eHTTP_METHOD::GET_; break;
	case	eHTTP_SCHEME_TYPE::MAILTO_:		method = eHTTP_METHOD::GET_; break;
	case	eHTTP_SCHEME_TYPE::SOCKS_:		method = eHTTP_METHOD::GET_; break;
	case	eHTTP_SCHEME_TYPE::JAVASCRIPT_:	method = eHTTP_METHOD::GET_; break;
	case	eHTTP_SCHEME_TYPE::VBSCRIPT_:	method = eHTTP_METHOD::GET_; break;
	case	eHTTP_SCHEME_TYPE::RES_:		method = eHTTP_METHOD::GET_; break;
	case	eHTTP_SCHEME_TYPE::WS_:			method = eHTTP_METHOD::GET_; break;
	case	eHTTP_SCHEME_TYPE::WSS_:		method = eHTTP_METHOD::GET_; break;
	default:								CGASSERT_ERROR(false); method = eHTTP_METHOD::NONE_;
	}

	// return)
	return	method;
}

bool base64_encode(char* _dest, size_t _dest_size, const std::string_view _source) noexcept;

void net::http_request::set_request(const std::string_view _str_url, eHTTP_METHOD _type)
{
	// Example)
	//			GET /path?query HTTP/1.1
	//			Host: localhost:19000
	//			User-Agent: CGCII http 10.0
	//			Accept: */*
	//			Connection: close
	//

	// check)
	if(_str_url.empty())
		throw throwable::failure(eRESULT::INVALID_ARGUMENT);

	// declare)
	URL_COMPONENT temp_cracked_URL;

	// 1) parse the url
	_crack_url(temp_cracked_URL, _str_url);

	// check) the host
	if (temp_cracked_URL.str_hostname.empty())
		throw throwable::failure(eRESULT::INVALID_ARGUMENT);

	// 2) the default method (before the object : the query of GET is in the path)
	if(_type == eHTTP_METHOD::DEFAULT_)
	{
		_type = _get_default_HTTP_method(temp_cracked_URL.scheme);
	}

	// declare)
	std::string_view str_version;
	bool is_use_SSL = _is_use_ssl(temp_cracked_URL.scheme);		// - ssl
	uint16_t port = 0;
	if (const auto [ptr, ec] = std::from_chars(temp_cracked_URL.str_port.data(), temp_cracked_URL.str_port.data() + temp_cracked_URL.str_port.size(), port); ec != std::errc() || ptr != temp_cracked_URL.str_port.data() + temp_cracked_URL.str_port.size())
		throw throwable::failure(eRESULT::INVALID_ARGUMENT);
	std::vector<std::string_view> vector_header;
	std::vector<std::string_view> vector_accept;
	std::string_view str_referer;
	std::string str_websocket_key;		// - the strings of vector_header (kept until set_request below)

	// 3) GET (and HEAD, DELETE, ...) : the query is in the path / the others : the query is the body (form)
	const bool is_query_body = (_type == eHTTP_METHOD::POST_ || _type == eHTTP_METHOD::PUT_ || _type == eHTTP_METHOD::PATCH_);
	const std::string_view str_object = is_query_body ? temp_cracked_URL.str_object : temp_cracked_URL.str_path_query;
	buffer_view buffer_optional = is_query_body ? to_buffer_view(temp_cracked_URL.str_optional) : buffer_view();

	// 4) websocket
	if (temp_cracked_URL.scheme == eHTTP_SCHEME_TYPE::WS_ || temp_cracked_URL.scheme == eHTTP_SCHEME_TYPE::WSS_)
	{
		// - Sec-WebSocket-Key : a random 16 byte nonce in base64 (RFC 6455 4.1)
		std::array<uint32_t, 4> nonce{};
		{
			std::random_device random_device;
			for (auto& value : nonce)
				value = static_cast<uint32_t>(random_device());
		}

		char temp_key[32]{ 0 };
		if (base64_encode(temp_key, sizeof(temp_key), std::string_view(reinterpret_cast<const char*>(nonce.data()), nonce.size() * sizeof(uint32_t))) == false)
			throw throwable::failure(eRESULT::FAIL);
		m_keys = std::string_view(temp_key);

		// - set version
		str_version = "HTTP/1.1";	// 'HTTP/1.1'

		str_websocket_key = "Sec-WebSocket-Key: ";
		str_websocket_key += temp_key;
		vector_header.push_back("Origin: null"sv);
		vector_header.push_back(str_websocket_key);
		vector_header.push_back("Connection: Upgrade"sv);
		vector_header.push_back("Upgrade: websocket"sv);
		vector_header.push_back("Sec-WebSocket-Version: 13"sv);
	}
	else
	{
		// - set version
		str_version = to_string(temp_cracked_URL.scheme);	// 'HTTP/1.1'
	}

	// 5) the default headers (Content-Length is written by set_request from the body)
	vector_header.push_back("User-Agent: CGCII http 10.0"sv);
	if (buffer_optional.size() != 0)
		vector_header.push_back("Content-Type: application/x-www-form-urlencoded"sv);

	// 6) accept
	vector_accept.push_back("*/*");

	// 7) request (the address is resolved by it)
	set_request(
		_type,							// eHTTP_METHOD					_method_type,
		temp_cracked_URL.str_hostname,	// const char*					_host_name,
		port,							// uint16_t						_host_port,
		str_version,					// const char*					_version,
		str_object,						// const char*					_object,
		str_referer,					// const char*					_referer,
		vector_header,					// const vector<const char*>&	_vector_header,
		buffer_optional,				// const shared_buffer&		_buffer_optional,
		vector_accept,					// const vector<const char*>&	_vector_accept_type,
		is_use_SSL
	);
}

void net::http_request::set_request (const net::sockaddr& _address_host, const shared_buffer& _buffer)
{
	// check) 잘못된 Address인지 검사한다.
	CGASSERT_ERROR(_address_host.exist());

	// check) 잘못된 Address인지 검사한다.
	if(_address_host.empty())
		throw throwable::failure(eRESULT::INVALID_ARGUMENT);

	// 1) Host Address를 설정한다.
	m_address_host	 = _address_host;

	// 2) shared_buffer를 설정한다.
	m_buffer_request = _buffer;
}

void _append_host(shared_buffer& _buffer, const std::string_view _host_name, uint16_t _host_port) noexcept
{
	// - an ipv6 address is in "[]"
	const bool is_ipv6 = (_host_name.find(':') != std::string_view::npos);

	_buffer.append_text<char>("Host: "sv);
	if (is_ipv6)
		_buffer.append_text<char>("["sv);
	_buffer.append_text<char>(_host_name);
	if (is_ipv6)
		_buffer.append_text<char>("]"sv);
	if (_host_port != 0)
	{
		char temp_port[16]{ 0 };
		const auto length = std::snprintf(temp_port, std::size(temp_port), ":%u", static_cast<unsigned>(_host_port));
		_buffer.append_text<char>(std::string_view(temp_port, static_cast<size_t>(length)));
	}
	_buffer.append_text<char>("\r\n"sv);
}

void _append_referer(shared_buffer& _buffer, const std::string_view _referer) noexcept
{
	// check)
	if(_referer.empty())
		return;

	// 1) 추갛ㄴ다.
	_buffer.append_text<char>("Referer:"sv);
	_buffer.append_text<char>(_referer);
	_buffer.append_text<char>("\r\n"sv);
}

// - "Content-Length" header (case insensitive)
bool _is_content_length_header(std::string_view _header) noexcept
{
	constexpr std::string_view name = "content-length";

	// check)
	if (_header.size() < name.size())
		return false;

	for (size_t i = 0; i < name.size(); ++i)
	{
		if (static_cast<char>(std::tolower(static_cast<unsigned char>(_header[i]))) != name[i])
			return false;
	}

	// return) "Content-Length:" or "Content-Length :"
	return _header.size() == name.size() || _header[name.size()] == ':' || _header[name.size()] == ' ';
}

// - the header _name is in _vector_header (case insensitive, _name : lower case)
bool _has_header(const std::vector<std::string_view>& _vector_header, std::string_view _name) noexcept
{
	for (auto& iter : _vector_header)
	{
		if (iter.size() <= _name.size() || iter[_name.size()] != ':')
			continue;

		bool is_same = true;
		for (size_t i = 0; i < _name.size() && is_same; ++i)
			is_same = (static_cast<char>(std::tolower(static_cast<unsigned char>(iter[i]))) == _name[i]);

		if (is_same)
			return true;
	}
	return false;
}

void _append_headers(shared_buffer& _buffer, const std::vector<std::string_view>& _vector_header) noexcept
{
	for (auto& iter : _vector_header)
	{
		// - "Content-Length" is written by set_request from the size of the body (a mismatch isn't possible)
		if (_is_content_length_header(iter))
			continue;

		_buffer.append_text<char>(iter);
		_buffer.append_text<char>("\r\n"sv);
	}
}

void _append_accept_type(shared_buffer& _buffer, const std::vector<std::string_view>& _vector_accept_type) noexcept
{
	// check) 
	if(_vector_accept_type.empty())
		return;

	// 1) Add key["Accept"]
	_buffer.append_text<char>("Accept:"sv);

	// 2) Add Values
	for (auto& iter: _vector_accept_type)
	{
		_buffer.append_text<char>(iter);
		_buffer.append_text<char>(","sv);
	}

	// 3) 마지막의 ','를 제거한다.
	_buffer.dec_size();

	// 4) End...
	_buffer.append_text<char>("\r\n"sv);
}

void _append_accept_encoding(shared_buffer& /*_buffer*/)
{
	//
	// Accept-Encoding 파라미터
	//
	// 클라이언트(요청자)쪽에서 이해 가 능한 인코딩을 알려준다.
	// 고부하 상태(CPU 80%이상을 사용하는 상황)에서는 사용하지 않기를 권장한다.
	//
	//	"gzip"		32bit CRC와 함께 LZW 압축
	//	"compress"	LZW 압축
	//	"deflate"	deflate 압축과 함께 zlib 구조를 사용해 압축
	//	"br"		Brotli 알고리즘을 사용해 압축
	//	"identity"	식별 함수(압축하지 않거나 수정하지 않은 경우)를  말한다. 이 값은 명시하지 않아도 항상 설정된다. "identity;q=0"으로 설정하지 않는한 무조건 유효하다.
	//	"*"			어떤 인코딩 방법이라도 받아 들인다. 헤더가 존재하지 않는 경우 이 값이 기본값이 된다.
	//				모든 알고리즘을 지원한다는 것을 의미하지 않는다. 단지 표현된 선호 대상이 없다는 것을 의미한다.
	//	";q=[w]"	w는 상대적 우선순위값으로 이 갑이 높으면 더 선호한다는 의미이다.
	//
	// 여러 값들이 ','로 구분하여 여러 개를 동시에 설정할 수 있다.
	// identity
	//
	//
	// 예)
	//       Accept-Encoding: gzip
	//       Accept-Encoding: gzip, compress, br
	//       Accept-Encoding: br;q=1.0, gzip;q=0.8, *;q=0.1
	//

	//// 1) Add key["Accept-encoding"sv]
	//_buffer.append_text<char>("Accept-Encoding: "sv);

	//// 2) Add key["Accept"sv]
	//_buffer.append_text<char>("*"sv);

	//// 2) End...
	//_buffer.append_text<char>("\r\n"sv);
}

void _append_accept_lnaguae(shared_buffer& /*_buffer*/)
{
	//
	// Accept-Language 파라미터
	//
	// 클라이언트(요청자)쪽에서 이해 가능한 언어를 알려준다. 
	// 고부하 상태(CPU 80%이상을 사용하는 상황)에서는 사용하지 않기를 권장한다.
	// 제안된 언어 중 지원 가능한 것이 없을 경우 406(Not Acceptable) 에러를 발생한다.
	//
	// 예)
	//       Accept-Language: kr
	//       Accept-Language: *
	//       Accept-Language: kr;q=1.0, en;q=0.8, jp;q=0.1

	//// 1) Add key["Accept-encoding"sv]
	//_buffer.append_text<char>("Accept-Language: "sv);

	//// 2) Add key["Accept"sv]
	//_buffer.append_text<char>("*"sv);

	//// 2) End...
	//_buffer.append_text<char>("\r\n"sv);
}

void _append_accept_charset(shared_buffer& /*_buffer*/)
{
	//
	// Accept-Charset 파라미터
	//
	// 클라이언트(요청자)가 이해할 수 있는 캐릭터셋을 알려준다. 
	// 서버측은 이 제안된 것 중 하나를 선택해 'Content-Type' 응답 헤더로 선택된 캐릭터셋을 알려준다.
	// 제안된 캐릭터셋 중 지원 가능한 것이 없을 경우 406(Not Acceptable) 에러를 발생한다.
	//
	// 예)
	//       Accept-Charset: utf-8
	//       Accept-Charset: utf-8, iso-8859-1
	//       Accept-Charset: utf-8, iso-8859-1;q=0.5, *;q=0.1

	//// 1) Add key["Accept-encoding"sv]
	//_buffer.append_text<char>("Accept-Language: "sv);

	//// 2) Add key["Accept"sv]
	//_buffer.append_text<char>("*"sv);

	//// 2) End...
	//_buffer.append_text<char>("\r\n"sv);
}

void net::http_request::set_request(
		eHTTP_METHOD					_method_type,
		const std::string_view			_host_name,
		uint16_t						_host_port,
		const std::string_view			_version,
		const std::string_view			_object,
		const std::string_view			_referer,
		const std::vector<std::string_view>& _vector_header,
		const buffer_view				_buffer_optional,
		const std::vector<std::string_view>& _vector_accept_type,
		bool							_is_crypt
	)
{
	// 1) size of the request (request line + headers + body)
	size_t size_request = 128 + _version.size() + _object.size() + _host_name.size() + _referer.size() + _buffer_optional.size();
	for (auto& iter : _vector_header)
		size_request += iter.size() + 2;
	for (auto& iter : _vector_accept_type)
		size_request += iter.size() + 1;

	auto buf_temp = alloc_shared_buffer(size_request);

	// 2) request line
	buf_temp.append_text<char>(to_string(_method_type));						// GET/POST/ ...
	buf_temp.append_text<char>(" "sv);
	buf_temp.append_text<char>((_object.empty() == false) ? _object : "/"sv);
	buf_temp.append_text<char>(" "sv);
	buf_temp.append_text<char>(_version);										// HTTP/1.1
	buf_temp.append_text<char>("\r\n"sv);

	// 3) headers (except "Content-Length" : written below from the body)
	_append_headers(buf_temp, _vector_header);

	// 4) host and port
	_append_host(buf_temp, _host_name, _host_port);

	// 5) referer
	_append_referer(buf_temp, _referer);

	// 6) accept types
	_append_accept_type(buf_temp, _vector_accept_type);
	_append_accept_encoding(buf_temp);
	_append_accept_lnaguae(buf_temp);
	_append_accept_charset(buf_temp);

	// 7) "Content-Length" : the size of the body exactly (POST/PUT/PATCH always : "Content-Length: 0" if no body)
	if (_buffer_optional.size() != 0 || _method_type == eHTTP_METHOD::POST_ || _method_type == eHTTP_METHOD::PUT_ || _method_type == eHTTP_METHOD::PATCH_)
	{
		char temp_content_length[64]{ 0 };
		const auto length = std::snprintf(temp_content_length, std::size(temp_content_length), "Content-Length: %zu\r\n", _buffer_optional.size());
		buf_temp.append_text<char>(std::string_view(temp_content_length, static_cast<size_t>(length)));
	}

	// 8) "Connection: close" if not given (the client closes the connection after the response)
	if (_has_header(_vector_header, "connection"sv) == false)
		buf_temp.append_text<char>("Connection: close\r\n"sv);

	// 9) end of the header
	buf_temp.append_text<char>("\r\n"sv);

	// 10) body (as it is : nothing is appended after it, Content-Length is its size)
	if (_buffer_optional.size() != 0)
		buf_temp.append(_buffer_optional.size(), _buffer_optional.data());

	// 11) the address of the host
	net::sockaddr addr_host = from_string(_host_name, _host_port);

	// 12) set
	m_buffer_request = std::move(buf_temp);
	m_address_host = addr_host;
	m_is_crypt = _is_crypt;
	m_host_name = _host_name;
	m_http_method = _method_type;		// - the response of HEAD has no body
}

result_code net::http_request::process_request()
{
	// 1) not completed (a request object can be requested again)
	m_is_completed.store(false, std::memory_order_release);

	// declare)
	bool result = false;

	try
	{
		// 2) the socket
		auto psocket_http = make_object<net::socket::http_client>();

		// 3) ssl
		psocket_http->prepare_ssl(m_is_crypt, m_host_name);

		// 4) connect (the response comes by process_response)
		result = psocket_http->connect(this);
	}
	catch (...)
	{
		// - completed (wait() returns)
		_process_complete();

		// reraise)
		throw;
	}

	// check) failed : completed without the response (wait() returns)
	if(result == false)
	{
		_process_complete();
		return eRESULT::FAIL;
	}

	// return)
	return eRESULT::SUCCESS;
}

void net::http_request::_process_complete() noexcept
{
	// - the flag is set in the lock : a waiter checking the flag can't miss the notification
	{
		std::lock_guard<lock_object> cs(m_lockable_wait);
		m_is_completed.store(true, std::memory_order_release);
	}
	m_cv_complete.notify_all();
}

int net::http_request::process_response(sMESSAGE& _msg)
{
	// check)
	CGASSERT_ERROR(_msg.message == eMESSAGE::SYSTEM::HTTP_RESPONSE);

	// 1) cast
	auto& msg_http = static_cast<sMESSAGE_HTTP_RESPONSE&>(_msg);

	// 2) set
	m_buffer_response = msg_http.buf_message;
	msg_http.method = m_http_method;

	// 3) call the function
	if(m_function_on_response)
	{
		m_function_on_response(msg_http);
	}

	// 4) notify completion (after the function)
	if (msg_http.is_completed)
	{
		_process_complete();
	}

	// return)
	return 1;
}
