#include "pch.h"

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
	RETURN_IF(size < 2, result);

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
				&& (pos[4] == 't' || pos[4] == 'E')
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
					result = eHTTP_SCHEME_TYPE::SOCKS_;
				}
			}
			break;

	// case mail
	case	'm':
	case	'M':
			if (size == 4)
			{
				if((pos[1] == 'a' || pos[1] == 'A')
				&& (pos[2] == 'i' || pos[2] == 'I')
				&& (pos[3] == 'l' || pos[3] == 'L')
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
				&& (pos[4] == 's' || pos[3] == 'S')
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
				&& (pos[4] == 'r' || pos[3] == 'R')
				&& (pos[4] == 'i' || pos[3] == 'I')
				&& (pos[4] == 'p' || pos[3] == 'P')
				&& (pos[4] == 't' || pos[3] == 'T')
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

bool _crack_url(URL_COMPONENT& _url_parsed, const std::string_view _str_url, eHTTP_METHOD _type)
{
	// declare) 
	auto str_now = _str_url.data();
	auto str_end = str_now + _str_url.size();

	// 1) Protocol 얻기
	for(auto str_begin = str_now; *str_now != '/' && str_now < str_end; ++str_now)
	{
		// Case) Protocol을 찾음(Protocol에 복사)
		if(*str_now ==':')
		{
			// - get scheme string && scheme type
			_url_parsed.str_scheme = std::string_view(str_begin, str_now - str_begin);
			_url_parsed.scheme = _get_scheme_type(_url_parsed.str_scheme);

			// check) ...
			THROW_IF(_url_parsed.scheme == eHTTP_SCHEME_TYPE::NOT_DEFINED_, throwable::failure(eRESULT::INVALID_ARGUMENT));

			// - set port string
			_url_parsed.str_port = _is_use_ssl(_url_parsed.scheme) ? STRING_HTTPS_PORT : STRING_HTTP_PORT;

			// - next character (skip ':')
			++str_now;

			// break) 
			break;
		}
	}

	if(str_now < str_end  && (*str_now == '/'))
	{
		// - Slash는 통과
		++str_now;

		// 2) Address- Slash 연달아 두개'//' 여야지만 주소다. (한개면 path)
		if(str_now < str_end && *str_now == '/')
		{
			// - skip '/'
			++str_now;

			// - read host name
			for(auto str_begin = str_now; ; ++str_now)
			{
				char char_now = *str_now;

				// Case) Host Name을 찾음! - Path로 감!
				if(char_now == '/'
				|| char_now == ' '
				|| char_now == '?'
				|| char_now == '#'
				|| char_now == ':'
				|| str_now  >= str_end)
				{
					// - stroe host name
					_url_parsed.str_hostname = std::string_view(str_begin, str_now - str_begin);

					// - break
					break;
				}
			}
		}

		// 3) Path
		if(*str_now ==':')
		{
			// - Colon는 통과
			++str_now;

			for(auto str_begin = str_now; ; ++str_now)
			{
				char char_now = *str_now;

				// Case) Host Name을 찾음! - Path로 감!
				if(char_now == '/'
				|| char_now == ' '
				|| char_now == '?'
				|| char_now == '#'
				|| str_now  >= str_end)
				{
					// - store port string
					_url_parsed.str_port = std::string_view(str_begin, str_now - str_begin);

					// break)
					break;
				}
			}
		}

		if (_type == eHTTP_METHOD::GET_)
		{
			// - Path
			if(str_now < str_end && *str_now =='/')
			{
				for(auto str_begin = str_now; ; ++str_now)
				{
					char char_now = *str_now;

					// Case) Host Name을 찾음! - Path로 감!
					if(char_now==' ' 
					|| char_now=='\t' 
					|| str_now >= str_end)
					{
						// - store 
						_url_parsed.str_object = std::string_view(str_begin, str_now - str_begin);

						// break;)
						break;
					}
				}
			}
		}
		else
		{
			// - Path
			if(str_now < str_end && *str_now =='/')
			{
				for(auto str_begin = str_now; ; ++str_now)
				{
					char char_now = *str_now;

					// Case) Host Name을 찾음! - Path로 감!
					if(char_now == ' ' 
					|| char_now == '\t' 
					|| char_now == '?' 
					|| char_now == '#' 
					|| str_now  >= str_end)
					{
						// store object string
						_url_parsed.str_object = std::string_view(str_begin, str_now - str_begin);

						// break)
						break;
					}
				}
			}

			// - optional (param)
			if(str_now < str_end && (*str_now =='?' || *str_now == '#'))
			{
				// - '?'는 제거한다.
				++str_now;

				for(auto str_begin = str_now; ; ++str_now)
				{
					char char_now = *str_now;

					// Case) Host Name을 찾음! - Path로 감!
					if(char_now == ' ' 
					|| char_now == '\t' 
					|| str_now  >  str_end)
					{
						// 복사한다.
						_url_parsed.str_optional = std::string_view(str_begin, str_now - str_begin);

						// break)
						break;
					}
				}
			}
		}
	}

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
	//			GET / HTTP/1.1
	//			Host: localhost:19000
	//			Connection: keep-alive
	//			Cache-control: max-age=0
	//			Upgrade-Insecure-Requests: 1
	//			User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/68.0.3440.106 Safari/537.36
	//			DNT: 1
	//			Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8
	//			Accept-Encoding: gzip, deflate, br
	//			Accept-Language: ko-KR,ko;q=0.9,en-US;q=0.8,en;q=0.7
	//

	// check)
	THROW_IF(_str_url.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT));

	// declare) 
	URL_COMPONENT temp_cracked_URL;

	// 1) URL을 parsing한다.
	_crack_url(temp_cracked_URL, _str_url, _type);

	// 2) _type이 DEFAULT_일 경우 Default Method를 얻는다.
	if(_type == eHTTP_METHOD::DEFAULT_)
	{
		_type = _get_default_HTTP_method(temp_cracked_URL.scheme);
	}

	// declare)
	std::string_view str_version;
	bool is_use_SSL = _is_use_ssl(temp_cracked_URL.scheme);		// SSL의 적용 여부
	uint16_t port = __from_string<uint16_t>(temp_cracked_URL.str_port);
	std::vector<std::string_view> vector_header;
	std::vector<std::string_view> vector_accept;
	std::string_view str_referer;
	buffer_view buffer_optional = to_buffer_view(temp_cracked_URL.str_optional);

	// 3) websocket
	if (temp_cracked_URL.scheme == eHTTP_SCHEME_TYPE::WS_ || temp_cracked_URL.scheme == eHTTP_SCHEME_TYPE::WSS_)
	{
		// declare)
		auto temp_key = "12ag4df59aa3wtd1"sv;
		m_keys.resize(24);

		// - Base64로 Encoding
		auto result = base64_encode(&m_keys.front(), 25, temp_key);

		// check) ...
		THROW_IF(result == false, throwable::failure(eRESULT::FAIL));

		// - set version
		str_version = "HTTP/1.1";	// 'HTTP/1.1'

		char temp_str_websocket_key[64] { 0 };
		__sprintf(temp_str_websocket_key, "Sec-WebSocket-Key: %s"sv, m_keys.c_str());
		vector_header.push_back("Origin: null"sv);
		vector_header.push_back(temp_str_websocket_key);
		vector_header.push_back("Connection: Upgrade"sv);
		vector_header.push_back("Upgrade: websocket"sv);
		vector_header.push_back("Sec-Websocket-Version: 13"sv);

		//if (temp_cracked_URL.scheme == eHTTP_SCHEME_TYPE::WS_)
		//{
		//	vector_header.push_back("Sec-WebSocket-Protocol: ws");
		//}
		//else
		//{
		//	vector_header.push_back("Sec-WebSocket-Protocol: wss");
		//}
		//vector_header.push_back("Sec-WebSocket-Extensions: 13");
	}
	else
	{
		// - set version
		str_version = to_string<char>(temp_cracked_URL.scheme);	// 'HTTP/1.1'
	}

	// 4) 기본 header를 설정한다.
	//vector_header.push_back("Connection: keep-Alive");
	vector_header.push_back("User-Agent: CGCII http 10.0"sv);
	if (temp_cracked_URL.str_optional.empty() == false || _type != eHTTP_METHOD::GET_)
	{
		char temp_str_content_length[64]{ 0 };

		// - content length을 작성한다.
		__sprintf(temp_str_content_length, "Content-Length: %d"sv, temp_cracked_URL.str_optional.size());

		// - header에 추가한다.
		vector_header.push_back("Content-Type: application/x-www-form-urlencoded");
		vector_header.push_back(temp_str_content_length);
	}

	//// 4) Accept type을 설정한다.
	//vector_accept.push_back("text/html"sv);
	//vector_accept.push_back("application/xhtml+xml"sv);
	//vector_accept.push_back("application/xml;q=0.9"sv);
	//vector_accept.push_back("image/webp"sv);
	//vector_accept.push_back("image/apng"sv);
	//vector_accept.push_back("*/*;q=0.8"sv);
	vector_accept.push_back("*/*");

	// 5) Host address를 얻는다.
	m_address_host = from_string(temp_cracked_URL.str_hostname, __from_string<uint16_t>(temp_cracked_URL.str_port));

	set_request(
		_type,							// eHTTP_METHOD					_method_type,
		temp_cracked_URL.str_hostname,	// const char*					_host_name,
		port,							// uint16_t						_host_port,
		str_version,					// const char*					_version,
		temp_cracked_URL.str_object,	// const char*					_object,
		str_referer,					// const char*					_referer,
		vector_header,					// const vector<const char*>&	_vector_header,
		buffer_optional,				// const shared_buffer&		_buffer_optinal,
		vector_accept,					// const vector<const char*>&	_vector_accept_type,
		is_use_SSL
	);
}

void net::http_request::set_request (const net::sockaddr& _address_host, const shared_buffer& _buffer)
{
	// check) 잘못된 Address인지 검사한다.
	CGASSERT_ERROR(_address_host.exist());

	// check) 잘못된 Address인지 검사한다.
	THROW_IF(_address_host.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 1) Host Address를 설정한다.
	m_address_host	 = _address_host;

	// 2) shared_buffer를 설정한다.
	m_buffer_request = _buffer;
}

void _append_host(shared_buffer& _buffer, const std::string_view _host_name, uint16_t _host_port) noexcept
{
	if (_host_port != 0)
	{
		_buffer.append_text<char>("Host:{}:{}\r\n"sv, _host_name.data(), (int)_host_port);
	}
	else
	{
		_buffer.append_text<char>("Host:{}\r\n"sv, _host_name.data());
	}
}

void _append_referer(shared_buffer& _buffer, const std::string_view _referer) noexcept
{
	// check)
	RETURN_IF(_referer.empty());

	// 1) 추갛ㄴ다.
	_buffer.append_text<char>("Referer:"sv);
	_buffer.append_text<char>(_referer);
	_buffer.append_text<char>("\r\n"sv);
}

void _append_headers(shared_buffer& _buffer, const std::vector<std::string_view>& _vector_header) noexcept
{
	for (auto& iter : _vector_header)
	{
		_buffer.append_text<char>(iter);
		_buffer.append_text<char>("\r\n"sv);
	}
}

void _append_accept_type(shared_buffer& _buffer, const std::vector<std::string_view>& _vector_accept_type) noexcept
{
	// check) 
	RETURN_IF(_vector_accept_type.empty());

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
	//       Accept-Encoding: br;q=1.0, gzie;q=0.8, *;q=0.1
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
		const buffer_view				_buffer_optinal,
		const std::vector<std::string_view>& _vector_accept_type,
		bool							_is_crypt
	)
{
	// 1) request message를 작성하낟.
	auto buf_temp = alloc_shared_buffer(8192LL + _buffer_optinal.size());

	// 2) message Header
	buf_temp.append_text<char>(to_string<char>(_method_type));				// GET/POST/ ...
	buf_temp.append_text<char>(" "sv);										// space
	buf_temp.append_text<char>((_object.empty() == false) ? _object : "/"sv);	// space
	buf_temp.append_text<char>(" "sv);										// space
	buf_temp.append_text<char>(_version);									// HTTP/1.1
	buf_temp.append_text<char>("\r\n"sv);

	// 3) headers를 추가한다.
	_append_headers(buf_temp, _vector_header);

	// 4) host and port를 설정한다.
	_append_host(buf_temp, _host_name, _host_port);

	// 5) referer를 추가한다.
	_append_referer(buf_temp, _referer);

	// 65) accept type을 추가한다.
	_append_accept_type(buf_temp, _vector_accept_type);
	_append_accept_encoding(buf_temp);
	_append_accept_lnaguae(buf_temp);
	_append_accept_charset(buf_temp);

	// 7) end header
	buf_temp.append_text<char>("\r\n"sv);

	// 8) add optional
	if (_buffer_optinal.size() != 0 || _method_type != eHTTP_METHOD::GET_)
	{
		// - Add Context
		buf_temp.append(_buffer_optinal.size(), _buffer_optinal.data());
		buf_temp.append_text<char>("\r\n\r\n"sv);
	}

	// 9) End Optinal
	buf_temp[buf_temp.size()] = 0;

	// 10) Address를 얻는다.
	net::sockaddr addr_host = from_string(_host_name, _host_port);

	// 11) 버퍼를 설정한다.
	m_buffer_request = std::move(buf_temp);
	m_address_host = addr_host;
	m_is_crypt = _is_crypt;
}

result_code net::http_request::process_request()
{
	// 1) connector를 생성한다.
	auto psocket_http = make_object<net::socket::http_client>();

	// 2) Flag에 따라...
	psocket_http->prepare_ssl(m_is_crypt);

	// 3) connect를 요청한다.
	auto result = psocket_http->connect(this);

	// check) 실패했으면 종료...
	RETURN_IF(result == false, eRESULT::FAIL);

	// return) 
	return eRESULT::SUCCESS;
}

int net::http_request::process_response(sMESSAGE& _msg)
{
	// check) 
	CGASSERT_ERROR(_msg.message == eMESSAGE::SYSTEM::HTTP_RESPONSE);

	// 1) MESSAGE를 casting한다.
	auto& msg_http = static_cast<sMESSAGE_HTTP_RESPONSE&>(_msg);

	// 2) 설정
	m_buffer_response = msg_http.buf_message;
	msg_http.method = m_http_method;

	// 3) function을 호출한다.
	if(m_function_on_response)
	{
		m_function_on_response(msg_http);
	}

	// 4) notify completion
	if (msg_http.is_completed)
	{
		m_cv_complete.notify_one();
	}

	// return)  
	return 1;
}
