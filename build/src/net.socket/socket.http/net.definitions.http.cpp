#include "pch.h"
#include "cgdk/sdk10/system/object/util.cpu_feature.h"

using namespace CGDK;


std::string_view CGDK::to_string(CGDK::eHTTP_PROTOCOL _protocol) noexcept
{
	// declare) 
	std::string_view str_protocol;

	switch (_protocol)
	{
	case	eHTTP_PROTOCOL::NOT_DEFINED_:	str_protocol = "NDT DEFINED"sv; break;
	case	eHTTP_PROTOCOL::HTTP_:			str_protocol = "HTTP"sv; break;
	case	eHTTP_PROTOCOL::WEBSOCKET_:		str_protocol = "WEBSOCKET"sv; break;
	default: break;
	};

	// return) 
	return	str_protocol;
}

std::string_view CGDK::to_string(eHTTP_SCHEME_TYPE _web_scheme) noexcept
{
	// declare) 
	std::string_view str_scheme;

	switch (_web_scheme)
	{
	case	eHTTP_SCHEME_TYPE::FTP_:		str_scheme = "ftp"sv; break;
	case	eHTTP_SCHEME_TYPE::GOPHER_:		str_scheme = "gopher"sv; break;
	case	eHTTP_SCHEME_TYPE::HTTP_:		str_scheme = "HTTP/1.1"sv; break;
	case	eHTTP_SCHEME_TYPE::HTTPS_:		str_scheme = "HTTP/1.1"sv; break;
	case	eHTTP_SCHEME_TYPE::FILE_:		str_scheme = "file"sv; break;
	case	eHTTP_SCHEME_TYPE::NEWS_:		str_scheme = "news"sv; break;
	case	eHTTP_SCHEME_TYPE::MAILTO_:		str_scheme = "mail"sv; break;
	case	eHTTP_SCHEME_TYPE::SOCKS_:		str_scheme = "socks"sv; break;
	case	eHTTP_SCHEME_TYPE::JAVASCRIPT_:	str_scheme = "javascript"sv; break;
	case	eHTTP_SCHEME_TYPE::VBSCRIPT_:	str_scheme = "vbscript"sv; break;
	case	eHTTP_SCHEME_TYPE::RES_:		str_scheme = "res"sv; break;
	case	eHTTP_SCHEME_TYPE::WS_:			str_scheme = "ws"sv; break;
	case	eHTTP_SCHEME_TYPE::WSS_:		str_scheme = "wss"sv; break;
	default:	break;
	};

	// return) 
	return str_scheme;
}

std::string_view CGDK::to_string(CGDK::eHTTP_METHOD _http_method) noexcept
{
	// declare) 
	std::string_view str_scheme;

	switch (_http_method)
	{
	case	eHTTP_METHOD::NONE_:			str_scheme = "GET"sv; break;
	case	eHTTP_METHOD::GET_:				str_scheme = "GET"sv; break;
	case	eHTTP_METHOD::POST_:			str_scheme = "POST"sv; break;
	case	eHTTP_METHOD::PUT_:				str_scheme = "PUT"sv; break;
	case	eHTTP_METHOD::PATCH_:			str_scheme = "PATCH"sv; break;
	case	eHTTP_METHOD::DELETE_:			str_scheme = "DELETE"sv; break;
	case	eHTTP_METHOD::COPY_:			str_scheme = "COPY"sv; break;
	case	eHTTP_METHOD::HEAD_:			str_scheme = "HEAD"sv; break;
	case	eHTTP_METHOD::OPTIONS_:			str_scheme = "OPTIONS"sv; break;
	case	eHTTP_METHOD::LINK_:			str_scheme = "LINK"sv; break;
	case	eHTTP_METHOD::UNLINK_:			str_scheme = "UNLINK"sv; break;
	case	eHTTP_METHOD::PURGE_:			str_scheme = "PURGE"sv; break;
	case	eHTTP_METHOD::LOCK_:			str_scheme = "LOCK"sv; break;
	case	eHTTP_METHOD::UNLOCK_:			str_scheme = "UNLOCK"sv; break;
	case	eHTTP_METHOD::PROPFIND_:		str_scheme = "PROPFIND"sv; break;
	case	eHTTP_METHOD::VIEW_:			str_scheme = "VIEW"sv; break;
	case	eHTTP_METHOD::TRACE_:			str_scheme = "TRACE"sv; break;
	case	eHTTP_METHOD::BINARY_:			str_scheme = "BINARY"sv; break;
	case	eHTTP_METHOD::TEXT_:			str_scheme = "TEXT"sv; break;
	default: break;
	};

	// return) 
	return str_scheme;
}

std::string_view CGDK::to_string(eHTTP_STATUS_CODE _http_status_code) noexcept
{
	// declare) 
	std::string_view str_http_status_code;

	switch (_http_status_code)
	{
	case	eHTTP_STATUS_CODE::NOT_DEFINED_:							str_http_status_code = "Not Defined"sv; break;

	case	eHTTP_STATUS_CODE::CONTINUE_:								str_http_status_code = "Continue"sv; break;

	case	eHTTP_STATUS_CODE::SWITCHING_PROTOCOLS_:					str_http_status_code = "Switching Protocols"sv; break;
	case	eHTTP_STATUS_CODE::PROCESSING_:								str_http_status_code = "Processing"sv; break;
	case	eHTTP_STATUS_CODE::EARLY_HINTS_:							str_http_status_code = "Early Hints"sv; break;

	case	eHTTP_STATUS_CODE::OK_:										str_http_status_code = "OK"sv; break;
	case	eHTTP_STATUS_CODE::CREATED_:								str_http_status_code = "Created"sv; break;
	case	eHTTP_STATUS_CODE::ACCEPTED_:								str_http_status_code = "Accepted"sv; break;
	case	eHTTP_STATUS_CODE::NON_AUTHORITATIVE_INFORMATION_:			str_http_status_code = "Non-Authoritative Information"sv; break;
	case	eHTTP_STATUS_CODE::NO_CONTENT_:								str_http_status_code = "No Content"sv; break;
	case	eHTTP_STATUS_CODE::RESET_CONTENT_:							str_http_status_code = "Reset Content"sv; break;
	case	eHTTP_STATUS_CODE::PARTIAL_CONTENT_:						str_http_status_code = "Partial Content"sv; break;
	case	eHTTP_STATUS_CODE::MULTI_STATUS_:							str_http_status_code = "Multi-Status"sv; break;
	case	eHTTP_STATUS_CODE::ALREADY_REPORTED_:						str_http_status_code = "Already Reported"sv; break;
	case	eHTTP_STATUS_CODE::THIS_IS_FINE_:							str_http_status_code = "This is fine"sv; break;
	case	eHTTP_STATUS_CODE::IM_USED_:								str_http_status_code = "IM Used"sv; break;

	case	eHTTP_STATUS_CODE::MULTIPLE_CHOICES_:						str_http_status_code = "Multiple Choices"sv; break;
	case	eHTTP_STATUS_CODE::MOVED_PERMANENTLY_:						str_http_status_code = "Moved Permanently"sv; break;
	case	eHTTP_STATUS_CODE::FOUND_:									str_http_status_code = "Found"sv; break;
	case	eHTTP_STATUS_CODE::SEE_OTHER_:								str_http_status_code = "See Other"sv; break;
	case	eHTTP_STATUS_CODE::NOT_MODIFIED_:							str_http_status_code = "Not Modified"sv; break;
	case	eHTTP_STATUS_CODE::USE_PROXY_:								str_http_status_code = "Use Proxy"sv; break;
	case	eHTTP_STATUS_CODE::TEMPORARY_REDIRECT_:						str_http_status_code = "Temporary Redirect"sv; break;
	case	eHTTP_STATUS_CODE::PERMANENT_REDIRECT_:						str_http_status_code = "Permanent Redirect"sv; break;

	case	eHTTP_STATUS_CODE::BAD_REQUEST_:							str_http_status_code = "Bad Request"sv; break;
	case	eHTTP_STATUS_CODE::UNAUTHORIZED_:							str_http_status_code = "Unauthorized"sv; break;
	case	eHTTP_STATUS_CODE::PAYMENT_REQUIRED_:						str_http_status_code = "Payment Required"sv; break;
	case	eHTTP_STATUS_CODE::FORBIDDEN_:								str_http_status_code = "Forbidden"sv; break;
	case	eHTTP_STATUS_CODE::NOT_FOUND_:								str_http_status_code = "Not Found"sv; break;
	case	eHTTP_STATUS_CODE::METHOD_NOT_ALLOWED_:						str_http_status_code = "Method Not Allowed"sv; break;
	case	eHTTP_STATUS_CODE::NOT_ACCEPTABLE_:							str_http_status_code = "Not Acceptable"sv; break;
	case	eHTTP_STATUS_CODE::PROXY_AUTHENTICATION_REQUIRED_:			str_http_status_code = "Proxy Authentication Required"sv; break;
	case	eHTTP_STATUS_CODE::REQUEST_TIMEOUT_:						str_http_status_code = "Request Timeout"sv; break;
	case	eHTTP_STATUS_CODE::CONFLICT_:								str_http_status_code = "Conflict"sv; break;
	case	eHTTP_STATUS_CODE::GONE_:									str_http_status_code = "Gone"sv; break;
	case	eHTTP_STATUS_CODE::LENGTH_REQUIRED_:						str_http_status_code = "Length Required"sv; break;
	case	eHTTP_STATUS_CODE::PRECONDITION_FAILED_:					str_http_status_code = "Precondition Failed"sv; break;
	case	eHTTP_STATUS_CODE::PAYLOAD_TOO_LARGE_:						str_http_status_code = "Payload Too Large"sv; break;
	case	eHTTP_STATUS_CODE::URI_TOO_LONG_:							str_http_status_code = "URI Too Long"sv; break;
	case	eHTTP_STATUS_CODE::UNSUPPORTED_MEDIA_TYPE_:					str_http_status_code = "Unsupported Media Type"sv; break;
	case	eHTTP_STATUS_CODE::RANGE_NOT_SATISFIABLE_:					str_http_status_code = "Range Not Satisfiable"sv; break;
	case	eHTTP_STATUS_CODE::EXPECTATION_FAILED_:						str_http_status_code = "Expectation Failed"sv; break;
	case	eHTTP_STATUS_CODE::PAGE_EXPIRED_:							str_http_status_code = "Page Expired"sv; break;
	case	eHTTP_STATUS_CODE::METHOD_FAILURE_:							str_http_status_code = "Method Failure/Enhance YOur Calm"sv; break;
	case	eHTTP_STATUS_CODE::MISDIRECTED_REQUEST_:					str_http_status_code = "Misdirected Request"sv; break;
	case	eHTTP_STATUS_CODE::UNPROCESSABLE_ENTITY_:					str_http_status_code = "Unprocessable Entity"sv; break;
	case	eHTTP_STATUS_CODE::LOCKED_:									str_http_status_code = "Locked"sv; break;
	case	eHTTP_STATUS_CODE::FAILED_DEPENDENCY_:						str_http_status_code = "Failed Dependency"sv; break;
	case	eHTTP_STATUS_CODE::TOO_EARLY_:								str_http_status_code = "Too Early"sv; break;
	case	eHTTP_STATUS_CODE::UPGRADE_REQUIRED_:						str_http_status_code = "Upgrade Required"sv; break;
	case	eHTTP_STATUS_CODE::PRECONDITION_REQUIRED_:					str_http_status_code = "Precondition Required"sv; break;
	case	eHTTP_STATUS_CODE::TOO_MANY_REQUESTS_:						str_http_status_code = "Too Many Requests"; break;
	case	eHTTP_STATUS_CODE::REQUEST_HEADER_FIELDS_TOO_LARGE_SHOPIFY_:str_http_status_code = "Request Header Fields Too Large (Shopify)"sv; break;
	case	eHTTP_STATUS_CODE::REQUEST_HEADER_FIELDS_TOO_LARGE_:		str_http_status_code = "Request Header Fields Too Large"sv; break;
	case	eHTTP_STATUS_CODE::LOGIN_TIME_OUT_:							str_http_status_code = "Login Time-out"sv; break;
	case	eHTTP_STATUS_CODE::NO_RESPONSE_:							str_http_status_code = "No Response"sv; break;
	case	eHTTP_STATUS_CODE::RETRY_WWITH_:							str_http_status_code = "Retry With"sv; break;
	case	eHTTP_STATUS_CODE::BLOCKED_BY_WINDOWS_PARENTAL_CONTROL_:	str_http_status_code = "Blocked by Windows Parental Controls"sv; break;

	case	eHTTP_STATUS_CODE::UNAVAILABLE_FOR_LEGAL_REASONS_:			str_http_status_code = "Unavailable For Legal Reasons"sv; break;
	case	eHTTP_STATUS_CODE::AWS_ELASTIC_460:							str_http_status_code = "ASW_ELASTIC_460"sv; break;
	case	eHTTP_STATUS_CODE::AWS_ELASTIC_463:							str_http_status_code = "ASW_ELASTIC_463"sv; break;
	case	eHTTP_STATUS_CODE::REQUEST_HEADER_TOO_LARGE_:				str_http_status_code = "Request header too large"sv; break;
	case	eHTTP_STATUS_CODE::SSL_CERTIFICATED_ERROR_:					str_http_status_code = "SSL Certificate Error"sv; break;
	case	eHTTP_STATUS_CODE::SSL_CERTIFICATED_REQUIRED_:				str_http_status_code = "SSL Certificate Requiredsv"; break;
	case	eHTTP_STATUS_CODE::HTTP_REQUEST_SEND_TO_HTTPS_PORT_:		str_http_status_code = "HTTP Request Send to HTTPS Port"sv; break;
	case	eHTTP_STATUS_CODE::INVALID_TOKEN_:							str_http_status_code = "Invalid Token"sv; break;
	case	eHTTP_STATUS_CODE::CLIENT_CLOSED_REQUEST_:					str_http_status_code = "Client Closed Request"sv; break;

	case	eHTTP_STATUS_CODE::INTERNAL_SERVER_ERROR_:					str_http_status_code = "Internal Server Error"sv; break;
	case	eHTTP_STATUS_CODE::NOT_IMPLEMENTED_:						str_http_status_code = "Not Implemented"sv; break;
	case	eHTTP_STATUS_CODE::BAD_GATEWAY_:							str_http_status_code = "Bad Gateway"sv; break;
	case	eHTTP_STATUS_CODE::SERVICE_UNAVAILABLE_:					str_http_status_code = "Service Unavailable"sv; break;
	case	eHTTP_STATUS_CODE::GATEWAY_TIMEOUT_:						str_http_status_code = "Gateway Timeout"sv; break;
	case	eHTTP_STATUS_CODE::HTTP_VERSION_NOT_SUPPORTED_:				str_http_status_code = "HTTP Version Not Supported"sv; break;
	case	eHTTP_STATUS_CODE::VARIANT_ALSO_NEGOTIATES_:				str_http_status_code = "Variant Also Negotiates"sv; break;
	case	eHTTP_STATUS_CODE::INSUFFICIENT_STORAGE_:					str_http_status_code = "Insufficient Storage"sv; break;
	case	eHTTP_STATUS_CODE::LOOP_DETECTED_:							str_http_status_code = "Loop Detected"sv; break;
	case	eHTTP_STATUS_CODE::BANDWIDTH_LIMIT_EXCEEDED_:				str_http_status_code = "Bandwidth Limit Exceeded"sv; break;
	case	eHTTP_STATUS_CODE::NOT_EXTENDED_:							str_http_status_code = "Not Extended"sv; break;
	case	eHTTP_STATUS_CODE::NETWORK_AUTHENTICATION_REQUIRED_:		str_http_status_code = "Network Authentication Required"sv; break;
	case	eHTTP_STATUS_CODE::UNKNOWN_ERROR_:							str_http_status_code = "Unknown Error"sv; break;
	case	eHTTP_STATUS_CODE::WEB_SERVER_IS_DOWN_:						str_http_status_code = "Web Server id Down"sv; break;
	case	eHTTP_STATUS_CODE::CONNECTION_TIMED_OUT_:					str_http_status_code = "Connection Timed Out"sv; break;
	case	eHTTP_STATUS_CODE::ORIGIN_IS_UNREACHABLE_:					str_http_status_code = "Origin is Unreachable"sv; break;
	case	eHTTP_STATUS_CODE::A_TIMEOUT_OCCURRED_:						str_http_status_code = "A Timeout Occurred"sv; break;
	case	eHTTP_STATUS_CODE::SSL_HAND_SHAKE_FAILED_:					str_http_status_code = "SSL Handshake Failed"sv; break;
	case	eHTTP_STATUS_CODE::INVALID_SSL_CERTIFICATE_:				str_http_status_code = "Invalid SSL Certificate"sv; break;
	case	eHTTP_STATUS_CODE::RAILGUN_ERROR_:							str_http_status_code = "Railgun Error"sv; break;
	case	eHTTP_STATUS_CODE::ORIGIN_DNS_ERROR_:						str_http_status_code = "Origin DNS Error"sv; break;
	case	eHTTP_STATUS_CODE::NETWORK_READ_TIMEOUT_ERROR_:				str_http_status_code = "Network read timeout error"sv; break;

	default: break;
	};

	// return) 
	return str_http_status_code;
}

//-----------------------------------------------------------------------------
// websocket mask (RFC 6455 5.3)
//
//  - data[i] ^= mask[i % 4]
//  - 32 bytes per iteration with AVX2 (x64, runtime detected), or 8 bytes
//-----------------------------------------------------------------------------
#if defined(CGDK_SIMD_AVX2)
// return) processed bytes (multiple of 32)
CGDK_TARGET_AVX2 static std::size_t _apply_websocket_mask_avx2(uint8_t* _data, std::size_t _size, uint32_t _mask) noexcept
{
	const __m256i mask = _mm256_set1_epi32(static_cast<int>(_mask));

	std::size_t processed = 0;
	for (; _size - processed >= 32; processed += 32)
	{
		auto p = reinterpret_cast<__m256i*>(_data + processed);
		_mm256_storeu_si256(p, _mm256_xor_si256(_mm256_loadu_si256(p), mask));
	}

	// - avoid AVX-SSE transition penalty
	_mm256_zeroupper();

	return processed;
}
#endif

void apply_websocket_mask(char* _data, std::size_t _size, const uint8_t* _mask) noexcept
{
	// 1) prepare (mask32 : _mask[0..3] in memory order)
	auto data = reinterpret_cast<uint8_t*>(_data);
	uint32_t mask32;
	memcpy(&mask32, _mask, sizeof(mask32));

#if defined(CGDK_SIMD_AVX2)
	// 2) 32 bytes (processed bytes are multiple of 4, so the mask phase is kept)
	if (_size >= 32 && cpu::is_avx2_supported())
	{
		const auto processed = _apply_websocket_mask_avx2(data, _size, mask32);
		data += processed;
		_size -= processed;
	}
#endif

	// 3) 8 bytes
	const uint64_t mask64 = (static_cast<uint64_t>(mask32) << 32) | mask32;
	for (; _size >= 8; data += 8, _size -= 8)
	{
		uint64_t value;
		memcpy(&value, data, sizeof(value));
		value ^= mask64;
		memcpy(data, &value, sizeof(value));
	}

	// 4) remained bytes
	for (std::size_t i = 0; i < _size; ++i)
		data[i] ^= _mask[i & 3];
}