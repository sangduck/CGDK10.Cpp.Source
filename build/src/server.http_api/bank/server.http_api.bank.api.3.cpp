#include "pch.h"

#if !defined(__linux__)
	#pragma warning (disable:5054)
	#pragma warning (disable:6340)
	#pragma warning (disable:6313)
	//#pragma diag_suppress 177 // supress #177-D function  was declared but never referenced
	#include "../../common/rapidjson/document.h"
	#include "../../common/rapidjson/writer.h"
	#include "../../common/rapidjson/stringbuffer.h"
	#include "../../common/rapidjson/filereadstream.h"
	#include "../../common/rapidjson/filewritestream.h"
	#pragma warning (default:6340)
	#pragma warning (default:6313)
	#pragma warning (default:5054)
#else
	#include "../../common/rapidjson/document.h"
	#include "../../common/rapidjson/writer.h"
	#include "../../common/rapidjson/stringbuffer.h"
	#include "../../common/rapidjson/filereadstream.h"
	#include "../../common/rapidjson/filewritestream.h"
#endif

namespace CGDK
{

chrono::date_time from_string_date_time_millsec(std::string_view _string);
chrono::date_time from_string_date_time(std::string_view _string);
chrono::date_time from_string_date(std::string_view _string);
chrono::date_time from_string_time(std::string_view _string);

http_api::bank::RESPONSE_ACCOUNT_TRANSACTION_LIST http_api::bank::request(const REQUEST_ACCOUNT_TRANSACTION_LIST& _request)
{
	// ------------------------------------------------------------------
	// 
	// 사용자정보조회 API
	//
	// 요청 메시지 URL
	//   
	//   2) HTTP URL		/v1.0/account/transaction_list
	//	 3) HTTP Method		GET
	//	 4) Content-Type	"Content-Type: application/x-www-form-urlencoded; charset=UTF-8"
	//   5) Header
	//		- Authorization				Bearer<access_token>	오픈플랫폼으로부터 전송받은 access_token을 HTTP Header에 추가 [scope=login]
	//
	//   6) Parameters
	//		- fintech_use_num			"12345678901234567890"	AN(24) 핀테크이용번호
	//		- inquiry_type				"A"						A(1) 조회구분콛드 A:All, I:입금, O:입금
	//		- from_date					"20160310101921"		N(14) 조회시작일자
	//		- to_date					"20160310101921"		N(14) 조회종료일자
	//		- sort_order				"D"						A(1) 정렬순서 D:Descending, A:Ascending
	//		- page_index				"00001"					N(5) 페이지번호
	//		- tran_dtime				"20160310101921"		N(14) 요청일시
	//		- before_inquiry_trace_info	"123"					AN(20) 직전 조회 추적 정보
	//		- list_tran_seqno			"123"					N(10) 최종 거래내역 순번
	//
	// ------------------------------------------------------------------
	// declare) 
	const std::size_t SIZE_BUFFER = 8192;

	// check)
	CGASSERT_ERROR(_request.str_url.empty() == false);

	// check)
	THROW_IF(_request.str_url.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
	auto prequest_http = make_object<query::http::query>();

	// declare)
	auto bufObject = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));

	// 2) 시간을 얻는다.
	auto& from_time = _request.from_time;
	auto& to_time = _request.to_time;
	auto& tran_dtime = _request.tran_dtime;

	// 3) HTTP URL을 설정한다.(URL중 Path부분만...)
	auto size_write = __sprintf(bufObject.data<char>(), SIZE_BUFFER, "/v1.0/account/transaction_list?fintech_use_num=%s&inquiry_type=%s&from_date=%04u%02u%02u&to_date=%04u%02u%02u&sort_order=%s&page_index=%05u&tran_dtime=%04u%02u%02u%02u%02u%02u&befor_inquiry_trace_info=%llu&list_tran_seqno=%llu"sv,
		_request.fintech_use_num.c_str(),
		to_string<char>(_request.inquiry_type).data(),
		static_cast<uint32_t>(from_time.t_year),
		static_cast<uint32_t>(from_time.t_month),
		static_cast<uint32_t>(from_time.t_day),
		static_cast<uint32_t>(to_time.t_year),
		static_cast<uint32_t>(to_time.t_month),
		static_cast<uint32_t>(to_time.t_day),
		to_string<char>(_request.sort_order).data(),
		_request.page_index,
		static_cast<uint32_t>(tran_dtime.t_year),
		static_cast<uint32_t>(tran_dtime.t_month),
		static_cast<uint32_t>(tran_dtime.t_day),
		static_cast<uint32_t>(tran_dtime.t_hour),
		static_cast<uint32_t>(tran_dtime.t_minute),
		static_cast<uint32_t>(tran_dtime.t_second),
		_request.before_inquiry_trace_info,
		_request.list_tran_seqno
	);

	// check) sprintf가 오류가 났으면 Exception을 던진다.
	THROW_IF(size_write < 0, throwable::failure(eRESULT::FAIL));

	// 4) HTTP Method를 설정한다.
	auto http_method = eHTTP_METHOD::GET_;

	// declare)
	auto buf_header = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));

	// 5) HTTP URL을 설정한다.(URL중 Path부분만...)
	size_write = __sprintf(buf_header.data<char>(), SIZE_BUFFER, "Authorization: Bearer %s"sv, _request.authorization.c_str());

	// check) sprintf가 오류가 났으면 Exception을 던진다.
	THROW_IF(size_write < 0, throwable::failure(eRESULT::FAIL));

	// 6) Header를 설정한다 - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
	std::vector<std::string_view> vector_header{ "Content-Type: application/x-www-form-urlencoded; charset=UTF-8"sv, buf_header.data<char>() };

	// declare) 
	shared_buffer buf_option;

	// 7) Accept 받을 것을 설정.
	std::vector<std::string_view> vector_accept{ "*/*"sv, "text/plain"sv };
	//std::vector<std::string_view> vector_accept{ "*/*"sv, "application/json"sv };

	// 8) request를 Set한다.
	prequest_http->set_request(
		http_method,
		_request.str_url,
		443,
		"HTTP/1.1"sv,
		bufObject.data<char>(),
		std::string_view(),
		std::move(vector_header),
		buf_option,
		std::move(vector_accept),
		1		// 0: normal, 1:ssl
	);

	// declare) 
	http_api::bank::RESPONSE_ACCOUNT_TRANSACTION_LIST	response;

	// 9) 기본값을 설정...
	response.rsp_code = eRESPONSE_CODE::A0000_;
	response.rsp_message.clear();

	// 10) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
	prequest_http->set_function([&](sMESSAGE_HTTP_RESPONSE& _msg)
	{
		// check) Result가 0이면 실패이므로 끝낸다.
		if (_msg.status_code != eHTTP_STATUS_CODE::OK_)
		{
			// - 내부처리 중 Error을 명시
			response.rsp_code = eRESPONSE_CODE::FAIL_;
			response.rsp_message = _msg.status_message;

			// - buffer를 clear한다.
			response.buf_response.clear();

			// Return)
			return;
		}

		// - 마지막을 null로 바꾼다.
		_msg.contents[0].append<char>(0);

		// - buffer를 넣는다.
		response.buf_response = _msg.contents[0];

		// declare) 
		rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;

		// - Parsing한다.
		const auto&	result = json_data.Parse(_msg.contents[0].data<char>());

		// check) Parsing에 실패했으면 false를 리턴한다.
		RETURN_IF(result.IsNull())

		//// declare) 
		//int	page_record_cnt = 0;

		// - 값을 설정한다.
		for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
		{
			auto str_name = (*iter).name.GetString();
			auto& value = (*iter).value;

			// - rsp_code
			if(__strcmp(str_name, "rsp_code"sv)==0)
			{
				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
			}
			// - rsp_message
			else if(__strcmp(str_name, "rsp_message"sv) == 0)
			{
				response.rsp_message = convert_utf8_to_multibyte(value.GetString());
			}

			// - "api_tran_id"
			else if (__strcmp(str_name, "api_tran_id"sv) == 0)
			{
				response.api_tran_id = value.GetString();
			}
			// - "api_tran_dtm"
			else if (__strcmp(str_name, "api_tran_dtm"sv) == 0)
			{
				response.api_tran_dtm = from_string_date_time_millsec(value.GetString());
			}
			// - "bank_tran_id"
			else if (__strcmp(str_name, "bank_tran_id"sv) == 0)
			{
				response.bank_tran_id = value.GetString();
			}
			// - "bank_tran_date"
			else if (__strcmp(str_name, "bank_tran_date"sv) == 0)
			{
				response.bank_tran_date = from_string_date(value.GetString());
			}
			// - "bank_code_tran"
			else if (__strcmp(str_name, "bank_code_tran"sv) == 0)
			{
				response.bank_code_tran = value.GetString();
			}
			// - "bank_rsp_code"
			else if (__strcmp(str_name, "bank_rsp_code"sv) == 0)
			{
				response.bank_rsp_code = get_enum_OAUTH_BANK_CODE<char>(value.GetString());
			}
			// - "bank_rsp_message"
			else if (__strcmp(str_name, "bank_rsp_message"sv) == 0)
			{
				convert_utf8_to_multibyte(response.bank_rsp_message, value.GetString());
			}
			// - "fintech_use_num"
			else if (__strcmp(str_name, "fintech_use_num"sv) == 0)
			{
				response.fintech_use_num = value.GetString();
			}
			// - "balance_amt"
			else if (__strcmp(str_name, "balance_amt"sv) == 0)
			{
				response.balance_amt = __from_string<int64_t>(value.GetString());
			}
			// - "page_index_use_yn"
			else if (__strcmp(str_name, "page_index_use_yn"sv) == 0)
			{
				response.page_index_use_yn = get_enum_OAUTH_YESNO<char>(value.GetString());
			}
			// - "page_index"
			else if (__strcmp(str_name, "page_index"sv) == 0)
			{
				response.page_index = __from_string<int64_t>(value.GetString());
			}
			// - "total_record_cnt"
			else if (__strcmp(str_name, "total_record_cnt"sv) == 0)
			{
				response.total_record_cnt = __from_string<int64_t>(value.GetString());
			}
			// - "page_record_cnt"
			else if (__strcmp(str_name, "page_record_cnt"sv) == 0)
			{
				//page_record_cnt = __from_string<int>(value.GetString());
			}
			// - "next_page_yn"
			else if (__strcmp(str_name, "next_page_yn"sv) == 0)
			{
				response.next_page_yn = get_enum_OAUTH_YESNO<char>(value.GetString());
			}
			// - "befor_inquiry_trace_info"
			else if (__strcmp(str_name, "befor_inquiry_trace_info"sv) == 0)
			{
				response.befor_inquiry_trace_info = value.GetString();
			}
			// - "list_tran_seqno"
			else if (__strcmp(str_name, "list_tran_seqno"sv) == 0)
			{
				response.list_tran_seqno = value.GetString();
			}
			// - "res_list"
			else if (__strcmp(str_name, "res_list"sv) == 0)
			{
				for (auto iterRes = value.Begin(); iterRes != value.End(); ++iterRes)
				{
					// declare) 
					http_api::bank::RESPONSE_ACCOUNT_TRANSACTION_LIST::TRANSACTION	transaction;

					for (auto iterNest = iterRes->MemberBegin(); iterNest != iterRes->MemberEnd(); ++iterNest)
					{
						auto str_name_nested = iterNest->name.GetString();
						auto& value_Nested = iterNest->value;

						// - "tran_date"
						if (__strcmp(str_name_nested, "tran_date"sv) == 0)
						{
							transaction.tran_date = from_string_date(value_Nested.GetString());
						}
						// - "tran_time"
						else if (__strcmp(str_name_nested, "tran_time"sv) == 0)
						{
							transaction.tran_time = from_string_time(value_Nested.GetString());
						}
						// - "inout_type"
						else if (__strcmp(str_name_nested, "inout_type"sv) == 0)
						{
							// declare) 
							static_string<64> temp_string;

							// - 문자열 변환
							convert_utf8_to_multibyte(temp_string, value_Nested.GetString());

							// - "access_token"
							if (__strcmp(temp_string.c_str(), "입금"sv) == 0)
							{
								transaction.inout_type = eOAUTH_INOUT_TYPE::IN_;
							}
							// - "token_type"
							else if (__strcmp(temp_string.c_str(), "출금"sv) == 0)
							{
								transaction.inout_type = eOAUTH_INOUT_TYPE::OUT_;
							}
							else
							{
								transaction.inout_type = eOAUTH_INOUT_TYPE::NOT_DEFINED_;
							}
						}
						// - "tran_type"
						else if (__strcmp(str_name_nested, "tran_type"sv) == 0)
						{
							convert_utf8_to_multibyte(transaction.tran_type, value_Nested.GetString());
						}
						// - "print_content"
						else if (__strcmp(str_name_nested, "print_content"sv) == 0)
						{
							convert_utf8_to_multibyte(transaction.print_content, value_Nested.GetString());
						}
						// - "tran_amt"
						else if (__strcmp(str_name_nested, "tran_amt"sv) == 0)
						{
							transaction.tran_amt = __from_string<int64_t>(value_Nested.GetString());
						}
						// - "after_balance_amt"
						else if (__strcmp(str_name_nested, "after_balance_amt"sv) == 0)
						{
							transaction.after_balance_amt = __from_string<int64_t>(value_Nested.GetString());
						}
						// - "branch_name"
						else if (__strcmp(str_name_nested, "branch_name"sv) == 0)
						{
							convert_utf8_to_multibyte(transaction.branch_name, value_Nested.GetString());
						}
					}

					response.res_list.push_back(transaction);
				}
			}
		}
	});

	// 11) request를 요청한다.
	prequest_http->request();

	// return) 
	return	response;
}
http_api::bank::RESPONSE_TRANSFER_WITHDRAW http_api::bank::request(const REQUEST_TRANSFER_WITHDRAW& _request)
{
	// ------------------------------------------------------------------
	// 
	// 사용자정보조회 API
	//
	// 요청 메시지 URL
	//   
	//   2) HTTP URL		/v1.0/transfer/withdraw
	//	 3) HTTP Method		POST
	//	 4) Content-Type	"Content-Type: application/json; charset=UTF-8"
	//   5) Header
	//		- Authorization				Bearer<access_token>	오픈플랫폼으로부터 전송받은 access_token을 HTTP Header에 추가 [scope=login]
	//
	//   6) Parameters
	//		- dps_print_content			"입금계좌인자내역"		AH(20) 입금계좌인자내역
	//		- fintech_use_num			"12345678901234567890"	AN(24) 핀테크이용번호
	//		- tran_amt					"10000"					N(12) 거래금액
	//		- tran_dtime				"20160310101921"		N(14) 요청일시
	//
	// ------------------------------------------------------------------
	// declare) 
	const size_t SIZE_BUFFER = 8192;

	// check)
	CGASSERT_ERROR(_request.str_url.empty() == false);

	// check)
	THROW_IF(_request.str_url.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
	auto prequest_http = make_object<query::http::query>();

	// declare)
	auto str_object = "/v1.0/transfer/withdraw"sv;

	// 2) HTTP Method를 설정한다.
	auto http_method = eHTTP_METHOD::POST_;

	// declare)
	auto buf_header = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));

	// 3) HTTP URL을 설정한다.(URL중 Path부분만...)
	auto size_write = __sprintf(buf_header.data<char>(), SIZE_BUFFER, "Authorization: Bearer %s"sv, _request.authorization.c_str());

	// check) sprintf가 오류가 났으면 Exception을 던진다.
	THROW_IF(size_write < 0, throwable::failure(eRESULT::FAIL));

	// 4) Header를 설정한다 - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
	std::vector<std::string_view> vector_header{ "Content-Type: application/x-www-form-urlencoded; charset=UTF-8"sv, buf_header.data<char>() };

	// 5) parameter를 설정한다.
	auto buf_option = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));

	// declare)
	char strDpsPrintContent[256];

	// 6) 문자열변환을 수행한다.
	convert_multibyte_to_utf8(strDpsPrintContent, _request.dps_print_content.c_str());

	// 7) Option
	size_write = __sprintf(buf_option.data<char>(), SIZE_BUFFER, "{\"dps_print_content\": \"%s\", \"fintech_use_num\": \"%s\", \"tran_amt\": \"%lld\", \"tran_dtime\": \"%04u%02u%02u%02u%02u%02u\"}"sv,
		strDpsPrintContent,
		_request.fintech_use_num.c_str(),
		_request.tran_amt,
		static_cast<uint32_t>(_request.tran_dtime.t_year),
		static_cast<uint32_t>(_request.tran_dtime.t_month),
		static_cast<uint32_t>(_request.tran_dtime.t_day),
		static_cast<uint32_t>(_request.tran_dtime.t_hour),
		static_cast<uint32_t>(_request.tran_dtime.t_minute),
		static_cast<uint32_t>(_request.tran_dtime.t_second)
		);

	// check) sprintf가 오류가 났으면 Exception을 던진다.
	THROW_IF(size_write < 0, throwable::failure(eRESULT::FAIL));

	// 8) Option의 길이를 저장한다.
	buf_option.set_size(size_write);

	// 9) Accept 받을 것을 설정.
	std::vector<std::string_view> vector_accept{ "*/*"sv, "text/plain"sv };
	//std::vector<std::string_view> vector_accept{ "*/*"sv, "application/json"sv };

	// 10) request를 Set한다.
	prequest_http->set_request(
		http_method,
		_request.str_url,
		443,
		"HTTP/1.1"sv,
		str_object,
		std::string_view(),
		std::move(vector_header),
		buf_option,
		std::move(vector_accept),
		1		// 0: normal, 1:ssl
	);

	// declare) 
	http_api::bank::RESPONSE_TRANSFER_WITHDRAW	response;

	// 11) 기본값을 설정...
	response.rsp_code = eRESPONSE_CODE::A0000_;
	response.rsp_message.clear();

	// 12) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
	prequest_http->set_function([&](sMESSAGE_HTTP_RESPONSE& _msg)
	{
		// check) Result가 0이면 실패이므로 끝낸다.
		if (_msg.status_code != eHTTP_STATUS_CODE::OK_)
		{
			// - 내부처리 중 Error을 명시
			response.rsp_code = eRESPONSE_CODE::FAIL_;
			response.rsp_message = _msg.status_message;

			// - buffer를 clear한다.
			response.buf_response.clear();

			// Return)
			return;
		}

		// - 마지막을 null로 바꾼다.
		_msg.contents[0].append<char>(0);

		// - buffer를 넣는다.
		response.buf_response = _msg.contents[0];

		// declare) 
		rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;

		// - Parsing한다.
		const auto&	result = json_data.Parse(_msg.contents[0].data<char>());

		// check) Parsing에 실패했으면 false를 리턴한다.
		RETURN_IF(result.IsNull())

		// - 값을 설정한다.
		for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
		{
			auto str_name = (*iter).name.GetString();
			auto& value = (*iter).value;

			// - rsp_code
			if(__strcmp(str_name, "rsp_code"sv)==0)
			{
				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
			}
			// - rsp_message
			else if(__strcmp(str_name, "rsp_message"sv) == 0)
			{
				response.rsp_message = convert_utf8_to_multibyte(value.GetString());
			}

			// - "api_tran_id"
			else if (__strcmp(str_name, "api_tran_id"sv) == 0)
			{
				response.api_tran_id = value.GetString();
			}
			// - "api_tran_dtm"
			else if (__strcmp(str_name, "api_tran_dtm"sv) == 0)
			{
				response.api_tran_dtm = from_string_date_time_millsec(value.GetString());
			}
			// - "dps_bank_code_std"
			else if (__strcmp(str_name, "dps_bank_code_std"sv) == 0)
			{
				response.dps_bank_code_std = get_enum_OAUTH_BANK_CODE<char>(value.GetString());
			}
			// - "dps_bank_code_sub"
			else if (__strcmp(str_name, "dps_bank_code_sub"sv) == 0)
			{
				response.dps_bank_code_sub = __from_string<int64_t>(value.GetString());
			}
			// - "dps_bank_name"
			else if (__strcmp(str_name, "dps_bank_name"sv) == 0)
			{
				convert_utf8_to_multibyte(response.dps_bank_name, value.GetString());
			}
			// - "dps_account_num_masked"
			else if (__strcmp(str_name, "dps_account_num_masked"sv) == 0)
			{
				response.dps_account_num_masked = value.GetString();
			}
			// - "dps_print_content"
			else if (__strcmp(str_name, "dps_print_content"sv) == 0)
			{
				convert_utf8_to_multibyte(response.dps_print_content, value.GetString());
			}
			// - "dps_account_holder_name"
			else if (__strcmp(str_name, "dps_account_holder_name"sv) == 0)
			{
				convert_utf8_to_multibyte(response.dps_account_holder_name, value.GetString());
			}
			// - "bank_tran_id"
			else if (__strcmp(str_name, "bank_tran_id"sv) == 0)
			{
				response.bank_tran_id = value.GetString();
			}
			// - "bank_tran_date"
			else if (__strcmp(str_name, "bank_tran_date"sv) == 0)
			{
				response.bank_tran_date = from_string_date(value.GetString());
			}
			// - "bank_code_tran"
			else if (__strcmp(str_name, "bank_code_tran"sv) == 0)
			{
				response.bank_code_tran = value.GetString();
			}
			// - "bank_rsp_code"
			else if (__strcmp(str_name, "bank_rsp_code"sv) == 0)
			{
				response.bank_rsp_code = get_enum_OAUTH_BANK_CODE<char>(value.GetString());
			}
			// - "bank_rsp_message"
			else if (__strcmp(str_name, "bank_rsp_message"sv) == 0)
			{
				convert_utf8_to_multibyte(response.bank_rsp_message, value.GetString());
			}
			// - "fintech_use_num"
			else if (__strcmp(str_name, "fintech_use_num"sv) == 0)
			{
				response.fintech_use_num = value.GetString();
			}
			// - "account_alias"
			else if (__strcmp(str_name, "account_alias"sv) == 0)
			{
				convert_utf8_to_multibyte(response.account_alias, value.GetString());
			}
			// - "bank_code_std"
			else if (__strcmp(str_name, "bank_code_std"sv) == 0)
			{
				response.bank_code_std = get_enum_OAUTH_BANK_CODE<char>(value.GetString());
			}
			// - "bank_code_sub"
			else if (__strcmp(str_name, "bank_code_sub"sv) == 0)
			{
				response.bank_code_sub = __from_string<int64_t>(value.GetString());
			}
			// - "bank_name"
			else if (__strcmp(str_name, "bank_name"sv) == 0)
			{
				convert_utf8_to_multibyte(response.bank_name, value.GetString());
			}
			// - "account_num_masked"
			else if (__strcmp(str_name, "account_num_masked"sv) == 0)
			{
				response.account_num_masked = value.GetString();
			}
			// - "print_content"
			else if (__strcmp(str_name, "print_content"sv) == 0)
			{
				convert_utf8_to_multibyte(response.print_content, value.GetString());
			}
			// - "account_holder_name"
			else if (__strcmp(str_name, "account_holder_name"sv) == 0)
			{
				convert_utf8_to_multibyte(response.account_holder_name, value.GetString());
			}
			// - "tran_amt"
			else if (__strcmp(str_name, "tran_amt"sv) == 0)
			{
				response.tran_amt = __from_string<int64_t>(value.GetString());
			}
		}
	});

	// 13) request를 요청한다.
	prequest_http->request();

	// return) 
	return	response;
}
http_api::bank::RESPONSE_TRANSFER_DEPOSIT http_api::bank::request(const REQUEST_TRANSFER_DEPOSIT& _request)
{
	// ------------------------------------------------------------------
	// 
	// 사용자정보조회 API
	//
	// 요청 메시지 URL
	//   
	//   2) HTTP URL		/v1.0/transfer/deposit
	//	 3) HTTP Method		POST
	//	 4) Content-Type	"Content-Type: application/json; charset=UTF-8"
	//   5) Header
	//		- Authorization				Bearer<access_token> 오픈플랫폼으로부터 전송받은 access_token을 HTTP Header에 추가 [scope=login]
	//
	//   6) Parameters
	//		- ws_pass_pharase			"ASDFQWERZXCVTYUI"	AN(64) 이용기관 입금이체용 암호문구
	//		- ws_print_content			"출금계좌인자내역"	AH(20) 출금계좌인자내역
	//		- req_cnt					"25"				N(5)   입금요청건수
	//		- req_list					<object>			       입금요청목록
	//		- --tran_no					"1"					N(5)   거래순번
	//		- --fintech_use_num			"1234567890123456"	N(24)  핀테크이용번호
	//		- --print_content			"입금계좌인자내역"	AH(20) 입금계좌인자내역
	//		- --tran_amt				"10000"				N(12) 거래금액
	//		- tran_dtime				"20160310101921"	N(14) 요청일시
	//
	// ------------------------------------------------------------------
	// declare) 
	const std::size_t SIZE_BUFFER = 8192;

	// check)
	CGASSERT_ERROR(_request.str_url.empty() == false);

	// check)
	THROW_IF(_request.str_url.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
	auto prequest_http = make_object<query::http::query>();

	// declare)
	auto str_object = "/v1.0/transfer/deposit"sv;

	// 2) HTTP Method를 설정한다.
	auto http_method = eHTTP_METHOD::POST_;

	// declare)
	auto buf_header = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));

	// 3) HTTP URL을 설정한다.(URL중 Path부분만...)
	auto size_write = __sprintf(buf_header.data<char>(), SIZE_BUFFER, "Authorization: Bearer %s"sv, _request.authorization.c_str());

	// check) sprintf가 오류가 났으면 Exception을 던진다.
	THROW_IF(size_write < 0, throwable::failure(eRESULT::FAIL));

	// 4) Header를 설정한다 - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
	std::vector<std::string_view> vector_header{ "Content-Type: application/json; charset=UTF-8"sv, buf_header.data<char>() };

	// 5) parameter를 설정한다.
	auto buf_option = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));

	// declare) 
	char str_print_content[256];

	// 6) 문자열변환을 수행한다.
	convert_multibyte_to_utf8(str_print_content, _request.wd_print_content.c_str());

	// - ...
	size_write = __sprintf(buf_option.data<char>(), SIZE_BUFFER, "{\"wd_pass_phrase\": \"%s\", \"wd_print_content\": \"%s\", \"req_cnt\": \"%llu\", \"req_list\": ["sv,
		_request.wd_pass_phrase.c_str(),
		str_print_content,
		_request.req_list.size()
		);

	// check) sprintf가 오류가 났으면 Exception을 던진다.
	THROW_IF(size_write < 0, throwable::failure(eRESULT::FAIL));

	// - 길이를 추가한다.
	buf_option.add_size(size_write);

	// - req_list를 추가한다.
	for(auto iter: _request.req_list)
	{
		// - 문자열변환을 수행한다.
		convert_multibyte_to_utf8(str_print_content, iter.print_content.c_str());

		// - 뒷부분을 추가한다.
		auto sizeWrite2 = __sprintf(buf_option.data<char>(buf_option.size()), SIZE_BUFFER - buf_option.size(), "{ \"tran_no\": \"%d\", \"fintech_use_num\": \"%s\", \"print_content\": \"%s\", \"tran_amt\": \"%lld\"}"sv,
			iter.tran_no,					// tran_no
			iter.fintech_use_num.c_str(),	// fintech_use_num
			str_print_content,				// print_content
			iter.tran_amt					// tran_amt
		);

		// check) sprintf가 오류가 났으면 Exception을 던진다.
		THROW_IF(sizeWrite2 < 0, throwable::failure(eRESULT::FAIL));

		// - 길이를 추가한다.
		buf_option.add_size(sizeWrite2);
	}

	// - 뒷부분을 추가한다.
	size_write = __sprintf(buf_option.data<char>(buf_option.size()), SIZE_BUFFER- buf_option.size(), "], \"tran_dtime\": \"%04u%02u%02u%02u%02u%02u\"}"sv,
		static_cast<uint32_t>(_request.tran_dtime.t_year),
		static_cast<uint32_t>(_request.tran_dtime.t_month),
		static_cast<uint32_t>(_request.tran_dtime.t_day),
		static_cast<uint32_t>(_request.tran_dtime.t_hour),
		static_cast<uint32_t>(_request.tran_dtime.t_minute),
		static_cast<uint32_t>(_request.tran_dtime.t_second)
		);

	// check) sprintf가 오류가 났으면 Exception을 던진다.
	THROW_IF(size_write < 0, throwable::failure(eRESULT::FAIL));

	// 7) Option의 길이를 더한다.
	buf_option.add_size(size_write);

	// 8) Accept 받을 것을 설정.
	std::vector<std::string_view> vector_accept{ "*/*"sv, "text/plain"sv };
	//std::vector<std::string_view>	vector_accept{ "*/*"sv, "application/json"sv };

	// 9) request를 Set한다.
	prequest_http->set_request(
		http_method,
		_request.str_url,
		443,
		"HTTP/1.1"sv,
		str_object,
		std::string_view(),
		std::move(vector_header),
		buf_option,
		std::move(vector_accept),
		1		// 0: normal, 1:ssl
	);

	// declare) 
	http_api::bank::RESPONSE_TRANSFER_DEPOSIT	response;

	// 10) 기본값을 설정...
	response.rsp_code = eRESPONSE_CODE::A0000_;
	response.rsp_message.clear();

	// 11) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
	prequest_http->set_function([&](sMESSAGE_HTTP_RESPONSE& _msg)
	{
		// check) Result가 0이면 실패이므로 끝낸다.
		if (_msg.status_code != eHTTP_STATUS_CODE::OK_)
		{
			// - 내부처리 중 Error을 명시
			response.rsp_code = eRESPONSE_CODE::FAIL_;
			response.rsp_message = _msg.status_message;

			// - buffer를 clear한다.
			response.buf_response.clear();

			// Return)
			return;
		}

		// - 마지막을 null로 바꾼다.
		_msg.contents[0].append<char>(0);

		// - buffer를 넣는다.
		response.buf_response = _msg.contents[0];

		// declare) 
		rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;

		// - Parsing한다.
		const auto&	result = json_data.Parse(_msg.contents[0].data<char>());

		// check) Parsing에 실패했으면 false를 리턴한다.
		RETURN_IF(result.IsNull())

		// declare)
		[[maybe_unused]] std::size_t res_cnt = 0;

		// - 값을 설정한다.
		for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
		{
			auto str_name = (*iter).name.GetString();
			auto& value = (*iter).value;

			// - rsp_code
			if(__strcmp(str_name, "rsp_code"sv) == 0)
			{
				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
			}
			// - rsp_message
			else if(__strcmp(str_name, "rsp_message"sv) == 0)
			{
				response.rsp_message = convert_utf8_to_multibyte(value.GetString());
			}

			// - "api_tran_id"
			else if (__strcmp(str_name, "api_tran_id"sv) == 0)
			{
				response.api_tran_id = value.GetString();
			}
			// - "api_tran_dtm"
			else if (__strcmp(str_name, "api_tran_dtm"sv) == 0)
			{
				response.api_tran_dtm = from_string_date_time_millsec(value.GetString());
			}
			// - "wd_bank_code_std"
			else if (__strcmp(str_name, "wd_bank_code_std"sv) == 0)
			{
				response.wd_bank_code_std = get_enum_OAUTH_BANK_CODE<char>(value.GetString());
			}
			// - "wd_bank_code_sub"
			else if (__strcmp(str_name, "wd_bank_code_sub"sv) == 0)
			{
				response.wd_bank_code_sub = __from_string<int64_t>(value.GetString());
			}
			// - "wd_bank_name"
			else if (__strcmp(str_name, "wd_bank_name"sv) == 0)
			{
				convert_utf8_to_multibyte(response.wd_bank_name, value.GetString());
			}
			// - "wd_account_num_masked"
			else if (__strcmp(str_name, "wd_account_num_masked"sv) == 0)
			{
				response.wd_account_num_masked = value.GetString();
			}
			// - "wd_print_content"
			else if (__strcmp(str_name, "wd_print_content"sv) == 0)
			{
				convert_utf8_to_multibyte(response.wd_print_content, value.GetString());
			}
			// - "wd_account_holder_name"
			else if (__strcmp(str_name, "wd_account_holder_name"sv) == 0)
			{
				convert_utf8_to_multibyte(response.wd_account_holder_name, value.GetString());
			}
			else if(__strcmp(str_name, "res_cnt"sv) == 0)
			{
				res_cnt	 = __from_string<size_t>(value.GetString());
			}
			// - "res_list"
			else if (__strcmp(str_name, "res_list"sv) == 0)
			{
				for (auto iterRes = value.Begin(); iterRes != value.End(); ++iterRes)
				{
					// declare) 
					http_api::bank::RESPONSE_TRANSFER_DEPOSIT::TRANSACTION	transaction;

					for (auto iterNest = iterRes->MemberBegin(); iterNest != iterRes->MemberEnd(); ++iterNest)
					{
						auto str_name_nested = iterNest->name.GetString();
						auto& value_Nested = iterNest->value;

						// - "tran_no"
						if (__strcmp(str_name_nested, "tran_no"sv) == 0)
						{
							transaction.tran_no = __from_string<int>(value_Nested.GetString());
						}
						// - "bank_tran_id"
						else if (__strcmp(str_name_nested, "bank_tran_id"sv) == 0)
						{
							transaction.bank_tran_id = value_Nested.GetString();
						}
						// - "bank_tran_date"
						else if (__strcmp(str_name_nested, "bank_tran_date"sv) == 0)
						{
							transaction.bank_tran_date = from_string_date(value_Nested.GetString());
						}
						// - "bank_code_tran"
						else if (__strcmp(str_name_nested, "bank_code_tran"sv) == 0)
						{
							transaction.bank_code_tran = value_Nested.GetString();
						}
						// - "bank_rsp_code"
						else if (__strcmp(str_name_nested, "bank_rsp_code"sv) == 0)
						{
							transaction.bank_rsp_code = get_enum_OAUTH_BANK_CODE<char>(value_Nested.GetString());
						}
						// - "bank_rsp_message"
						else if (__strcmp(str_name_nested, "bank_rsp_message"sv) == 0)
						{
							convert_utf8_to_multibyte(transaction.bank_rsp_message, value_Nested.GetString());
						}
						// - "fintech_use_num"
						else if (__strcmp(str_name_nested, "fintech_use_num"sv) == 0)
						{
							transaction.fintech_use_num = value_Nested.GetString();
						}
						// - "account_alias"
						else if (__strcmp(str_name_nested, "account_alias"sv) == 0)
						{
							convert_utf8_to_multibyte(transaction.account_alias, value_Nested.GetString());
						}
						// - "bank_code_std"
						else if (__strcmp(str_name_nested, "bank_code_std"sv) == 0)
						{
							transaction.bank_code_std = get_enum_OAUTH_BANK_CODE<char>(value_Nested.GetString());
						}
						// - "bank_code_sub"
						else if (__strcmp(str_name_nested, "bank_code_sub"sv) == 0)
						{
							transaction.bank_code_sub = __from_string<int64_t>(value_Nested.GetString());
						}
						// - "bank_name"
						else if (__strcmp(str_name_nested, "bank_name"sv) == 0)
						{
							convert_utf8_to_multibyte(transaction.bank_name, value_Nested.GetString());
						}
						// - "account_num_masked"
						else if (__strcmp(str_name_nested, "account_num_masked"sv) == 0)
						{
							transaction.account_num_masked = value_Nested.GetString();
						}
						// - "print_content"
						else if (__strcmp(str_name_nested, "print_content"sv) == 0)
						{
							convert_utf8_to_multibyte(transaction.print_content, value_Nested.GetString());
						}
						// - "account_holder_name"
						else if (__strcmp(str_name_nested, "account_holder_name"sv) == 0)
						{
							convert_utf8_to_multibyte(transaction.account_holder_name, value_Nested.GetString());
						}
						// - "tran_amt"
						else if (__strcmp(str_name_nested, "tran_amt"sv) == 0)
						{
							transaction.tran_amt = __from_string<int64_t>(value_Nested.GetString());
						}
					}

					response.res_list.push_back(transaction);
				}

				// check) 
				CGASSERT_ERROR(response.res_list.size() == res_cnt);
			}
		}
	});

	// 12) request를 요청한다.
	prequest_http->request();

	// return) 
	return	response;
}
http_api::bank::RESPONSE_TRANSFER_DEPOSIT2 http_api::bank::request(const REQUEST_TRANSFER_DEPOSIT2& _request)
{
	// ------------------------------------------------------------------
	// 
	// 사용자정보조회 API
	//
	// 요청 메시지 URL
	//   
	//   2) HTTP URL		/v1.0/transfer/deposit2
	//	 3) HTTP Method		POST
	//	 4) Content-Type	"Content-Type: application/json; charset=UTF-8"
	//   5) Header
	//		- Authorization				Bearer<access_token>	오픈플랫폼으로부터 전송받은 access_token을 HTTP Header에 추가 [scope=login]
	//
	//   6) Parameters
	//		- ws_pass_pharase			"ASDFQWERZXCVTYUI"	AN(64) 이용기관 입금이체용 암호문구
	//		- ws_print_content			"출금계좌인자내역"	AH(20) 출금계좌인자내역
	//		- req_cnt					"25"				N(5)   입금요청건수
	//		- req_list					<object>			       입금요청목록
	//		- --tran_no					"1"					N(5)   거래순번
	//		- --bank_code_std			"097"				AN(3)  입금은행.표준코드
	//		- --account_num				"001230000678"		AN(16) 계좌번호
	//		- --account_holder_name		"홍길동"			AH(20) 입금계좌예금주명
	//		- --print_content			"입금계좌인자내역"	AH(20) 입금계좌인자내역
	//		- --tran_amt				"10000"				N(12) 거래금액
	//		- tran_dtime				"20160310101921"	N(14) 요청일시
	//
	// ------------------------------------------------------------------
	// declare) 
	const std::size_t SIZE_BUFFER = 8192;

	// check)
	CGASSERT_ERROR(_request.str_url.empty() == false);

	// check)
	THROW_IF(_request.str_url.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
	auto prequest_http = make_object<query::http::query>();

	// declare)
	auto str_object = "/v1.0/transfer/deposit2"sv;

	// 2) HTTP Method를 설정한다.
	auto http_method = eHTTP_METHOD::POST_;

	// declare)
	auto buf_header = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));

	// 3) HTTP URL을 설정한다.(URL중 Path부분만...)
	auto size_write = __sprintf(buf_header.data<char>(), SIZE_BUFFER, "Authorization: Bearer %s"sv, _request.authorization.c_str());

	// check) sprintf가 오류가 났으면 Exception을 던진다.
	THROW_IF(size_write < 0, throwable::failure(eRESULT::FAIL));

	// 4) Header를 설정한다 - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
	std::vector<std::string_view> vector_header{ "Content-Type: application/json; charset=UTF-8"sv, buf_header.data<char>() };

	// 5) parameter를 설정한다.
	auto buf_option = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));

	// declare) 
	char str_wd_print_content[256];

	// 6) 문자열변환을 수행한다.
	convert_multibyte_to_utf8(str_wd_print_content, _request.wd_print_content.c_str());

	// - ...
	size_write = __sprintf(buf_option.data<char>(), SIZE_BUFFER, "{\"wd_pass_phrase\": \"%s\", \"wd_print_content\": \"%s\", \"req_cnt\": \"%llu\", \"req_list\": ["sv,
		_request.wd_pass_phrase.c_str(),
		str_wd_print_content,
		_request.req_list.size()
		);

	// check) sprintf가 오류가 났으면 Exception을 던진다.
	THROW_IF(size_write < 0, throwable::failure(eRESULT::FAIL));

	// - 길이를 추가한다.
	buf_option.add_size(size_write);

	// -) req_list를 추가한다.
	for(auto iter: _request.req_list)
	{
		// declare) 
		char strAccountHolderName[256];
		char str_print_content[256];

		// 6) 문자열변환을 수행한다.
		convert_multibyte_to_utf8(strAccountHolderName, iter.account_holder_name.c_str());
		convert_multibyte_to_utf8(str_print_content, iter.print_content.c_str());

		// - 뒷부분을 추가한다.
		size_write = __sprintf(buf_option.data<char>(buf_option.size()), SIZE_BUFFER - buf_option.size(), "{ \"tran_no\": \"%d\", \"bank_code_std\": \"%03d\", \"account_num\": \"%s\", \"account_holder_name\": \"%s\", \"print_content\": \"%s\", \"tran_amt\": \"%lld\"}"sv,
			iter.tran_no,							// tran_no
			static_cast<int>(iter.bank_code_std),	// bank_code_std
			iter.accoount_num.c_str(),				// account_num
			strAccountHolderName,					// account_holder_name
			str_print_content,						// print_content
			iter.tran_amt							// tran_amt
		);

		// check) sprintf가 오류가 났으면 Exception을 던진다.
		THROW_IF(size_write < 0, throwable::failure(eRESULT::FAIL));

		// - 길이를 추가한다.
		buf_option.add_size(size_write);
	}

	// - 뒷부분을 추가한다.
	size_write = __sprintf(buf_option.data<char>(buf_option.size()), SIZE_BUFFER- buf_option.size(), "], \"tran_dtime\": \"%04u%02u%02u%02u%02u%02u\"}"sv,
		static_cast<uint32_t>(_request.tran_dtime.t_year),
		static_cast<uint32_t>(_request.tran_dtime.t_month),
		static_cast<uint32_t>(_request.tran_dtime.t_day),
		static_cast<uint32_t>(_request.tran_dtime.t_hour),
		static_cast<uint32_t>(_request.tran_dtime.t_minute),
		static_cast<uint32_t>(_request.tran_dtime.t_second)
		);

	// check) sprintf가 오류가 났으면 Exception을 던진다.
	THROW_IF(size_write < 0, throwable::failure(eRESULT::FAIL));

	// 7) Option의 길이를 저장한다.
	buf_option.add_size(size_write);

	// 8) Accept 받을 것을 설정.
	std::vector<std::string_view> vector_accept{ "*/*"sv, "text/plain"sv };
	//std::vector<std::string_view> vector_accept{ "*/*"sv, "application/json"sv };

	// 9) request를 Set한다.
	prequest_http->set_request(
		http_method,
		_request.str_url,
		443,
		"HTTP/1.1"sv,
		str_object,
		std::string_view(),
		std::move(vector_header),
		buf_option,
		std::move(vector_accept),
		1		// 0: normal, 1:ssl
	);

	// declare) 
	http_api::bank::RESPONSE_TRANSFER_DEPOSIT2	response;

	// 10) 기본값을 설정...
	response.rsp_code = eRESPONSE_CODE::A0000_;
	response.rsp_message.clear();

	// 11) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
	prequest_http->set_function([&](sMESSAGE_HTTP_RESPONSE& _msg)
	{
		// check) Result가 0이면 실패이므로 끝낸다.
		if (_msg.status_code != eHTTP_STATUS_CODE::OK_)
		{
			// - 내부처리 중 Error을 명시
			response.rsp_code = eRESPONSE_CODE::FAIL_;
			response.rsp_message = _msg.status_message;

			// - buffer를 clear한다.
			response.buf_response.clear();

			// Return)
			return;
		}

		// - 마지막을 null로 바꾼다.
		_msg.contents[0].append<char>(0);

		// - buffer를 넣는다.
		response.buf_response = _msg.contents[0];

		// declare) 
		rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;

		// - Parsing한다.
		const auto&	result = json_data.Parse(_msg.contents[0].data<char>());

		// check) Parsing에 실패했으면 false를 리턴한다.
		RETURN_IF(result.IsNull())

		// declare)
		[[maybe_unused]] std::size_t res_cnt = 0;

		// - 값을 설정한다.
		for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
		{
			auto str_name = (*iter).name.GetString();
			auto& value = (*iter).value;

			// - rsp_code
			if(__strcmp(str_name, "rsp_code"sv) == 0)
			{
				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
			}
			// - rsp_message
			else if(__strcmp(str_name, "rsp_message"sv) == 0)
			{
				response.rsp_message = convert_utf8_to_multibyte(value.GetString());
			}

			// - "api_tran_id"
			else if (__strcmp(str_name, "api_tran_id"sv) == 0)
			{
				response.api_tran_id = value.GetString();
			}
			// - "api_tran_dtm"
			else if (__strcmp(str_name, "api_tran_dtm"sv) == 0)
			{
				response.api_tran_dtm = from_string_date_time_millsec(value.GetString());
			}
			// - "wd_bank_code_std"
			else if (__strcmp(str_name, "wd_bank_code_std"sv) == 0)
			{
				response.wd_bank_code_std = get_enum_OAUTH_BANK_CODE<char>(value.GetString());
			}
			// - "wd_bank_code_sub"
			else if (__strcmp(str_name, "wd_bank_code_sub"sv) == 0)
			{
				response.wd_bank_code_sub = __from_string<int64_t>(value.GetString());
			}
			// - "wd_bank_name"
			else if (__strcmp(str_name, "wd_bank_name"sv) == 0)
			{
				convert_utf8_to_multibyte(response.wd_bank_name, value.GetString());
			}
			// - "wd_account_num_masked"
			else if (__strcmp(str_name, "wd_account_num_masked"sv) == 0)
			{
				response.wd_account_num_masked = value.GetString();
			}
			// - "wd_print_content"
			else if (__strcmp(str_name, "wd_print_content"sv) == 0)
			{
				convert_utf8_to_multibyte(response.wd_print_content, value.GetString());
			}
			// - "wd_account_holder_name"
			else if (__strcmp(str_name, "wd_account_holder_name"sv) == 0)
			{
				convert_utf8_to_multibyte(response.wd_account_holder_name, value.GetString());
			}
			else if (__strcmp(str_name, "res_cnt"sv) == 0)
			{
				res_cnt = __from_string<size_t>(value.GetString());
			}
			// - "res_list"
			else if (__strcmp(str_name, "res_list"sv) == 0)
			{
				for (auto iterRes = value.Begin(); iterRes != value.End(); ++iterRes)
				{
					// declare) 
					http_api::bank::RESPONSE_TRANSFER_DEPOSIT2::TRANSACTION transaction;

					for (auto iterNest = iterRes->MemberBegin(); iterNest != iterRes->MemberEnd(); ++iterNest)
					{
						auto str_name_nested = iterNest->name.GetString();
						auto& value_Nested = iterNest->value;

						// - "tran_no"
						if (__strcmp(str_name_nested, "tran_no"sv) == 0)
						{
							transaction.tran_no = __from_string<int>(value_Nested.GetString());
						}
						// - "bank_tran_id"
						else if (__strcmp(str_name_nested, "bank_tran_id"sv) == 0)
						{
							transaction.bank_tran_id = value_Nested.GetString();
						}
						// - "bank_tran_date"
						else if (__strcmp(str_name_nested, "bank_tran_date"sv) == 0)
						{
							transaction.bank_tran_date = from_string_date(value_Nested.GetString());
						}
						// - "bank_code_tran"
						else if (__strcmp(str_name_nested, "bank_code_tran"sv) == 0)
						{
							transaction.bank_code_tran = value_Nested.GetString();
						}
						// - "bank_rsp_code"
						else if (__strcmp(str_name_nested, "bank_rsp_code"sv) == 0)
						{
							transaction.bank_rsp_code = get_enum_OAUTH_BANK_CODE<char>(value_Nested.GetString());
						}
						// - "bank_rsp_message"
						else if (__strcmp(str_name_nested, "bank_rsp_message"sv) == 0)
						{
							convert_utf8_to_multibyte(transaction.bank_rsp_message, value_Nested.GetString());
						}
						// - "bank_code_std"
						else if (__strcmp(str_name_nested, "bank_code_std"sv) == 0)
						{
							transaction.bank_code_std = get_enum_OAUTH_BANK_CODE<char>(value_Nested.GetString());
						}
						// - "bank_code_sub"
						else if (__strcmp(str_name_nested, "bank_code_sub"sv) == 0)
						{
							transaction.bank_code_sub = __from_string<int64_t>(value_Nested.GetString());
						}
						// - "bank_name"
						else if (__strcmp(str_name_nested, "bank_name"sv) == 0)
						{
							convert_utf8_to_multibyte(transaction.bank_name, value_Nested.GetString());
						}
						// - "account_num"
						else if (__strcmp(str_name_nested, "account_num"sv) == 0)
						{
							transaction.account_num = value_Nested.GetString();
						}
						// - "account_num_masked"
						else if (__strcmp(str_name_nested, "account_num_masked"sv) == 0)
						{
							transaction.account_num_masked = value_Nested.GetString();
						}
						// - "print_content"
						else if (__strcmp(str_name_nested, "print_content"sv) == 0)
						{
							convert_utf8_to_multibyte(transaction.print_content, value_Nested.GetString());
						}
						// - "account_holder_name"
						else if (__strcmp(str_name_nested, "account_holder_name"sv) == 0)
						{
							convert_utf8_to_multibyte(transaction.account_holder_name, value_Nested.GetString());
						}
						// - "tran_amt"
						else if (__strcmp(str_name_nested, "tran_amt"sv) == 0)
						{
							transaction.tran_amt = value_Nested.GetString();
						}
					}

					response.res_list.push_back(transaction);
				}

				// check) 
				CGASSERT_ERROR(response.res_list.size() == res_cnt);
			}
		}
	});

	// 12) request를 요청한다.
	prequest_http->request();

	// return) 
	return	response;
}
http_api::bank::RESPONSE_TRANSFER_RESULT http_api::bank::request(const REQUEST_TRANSFER_RESULT& _request)
{
	// ------------------------------------------------------------------
	// 
	// 사용자정보조회 API
	//
	// 요청 메시지 URL
	//   
	//   2) HTTP URL		/v1.0/transfer/result
	//	 3) HTTP Method		POST
	//	 4) Content-Type	"Content-Type: application/json; charset=UTF-8"
	//   5) Header
	//		- Authorization				Bearer<access_token>	오픈플랫폼으로부터 전송받은 access_token을 HTTP Header에 추가 [scope=login]
	//
	//   6) Parameters
	//		- check_type				"1"					AN(1) 1:출금이체, 2:입금이체
	//		- req_cnt					"25"					N(5)   입금요청건수
	//		- req_list					<object>				       입금요청목록
	//		- --tran_no					"1"					N(5)   거래순번
	//		- --org_bank_tran_id		"123456712345"		AN(20) 원거래, 거래고유번호(참가기관)
	//		- --org_bank_tran_date		"20160310"			N(8)   원거래 거래일자(참가기관)
	//		- --org_tran_amt			"10000"				N(12)  원거래 거래금액
	//		- tran_dtime				"20160310101921"		N(14)  요청일시
	//
	// ------------------------------------------------------------------
	// declare) 
	const std::size_t SIZE_BUFFER = 8192;

	// check)
	CGASSERT_ERROR(_request.str_url.empty() == false);

	// check)
	THROW_IF(_request.str_url.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
	auto prequest_http = make_object<query::http::query>();

	// declare)
	auto str_object = "/v1.0/transfer/result"sv;

	// 2) HTTP Method를 설정한다.
	auto http_method = eHTTP_METHOD::POST_;

	// declare)
	auto buf_header = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));

	// 3) HTTP URL을 설정한다.(URL중 Path부분만...)
	auto size_write = __sprintf(buf_header.data<char>(), SIZE_BUFFER, "Authorization: Bearer %s"sv, _request.authorization.c_str());

	// check) sprintf가 오류가 났으면 Exception을 던진다.
	THROW_IF(size_write < 0, throwable::failure(eRESULT::FAIL));

	// 4) Header를 설정한다 - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
	std::vector<std::string_view> vector_header{ "Content-Type: application/json; charset=UTF-8"sv, buf_header.data<char>() };

	// 5) parameter를 설정한다.
	auto buf_option = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));

	// - ...
	size_write = __sprintf(buf_option.data<char>(), SIZE_BUFFER, "{\"check_type\": \"%s\", \"req_cnt\": \"%llu\", \"req_list\": ["sv,
		to_string<char>(_request.check_type).data(),
		_request.req_list.size()
		);

	// check) sprintf가 오류가 났으면 Exception을 던진다.
	THROW_IF(size_write < 0, throwable::failure(eRESULT::FAIL));

	// - 길이를 추가한다.
	buf_option.add_size(size_write);


	// -) req_list를 추가한다.
	for(auto iter: _request.req_list)
	{
		const auto& org_bank_tran_date = iter.org_bank_tran_date;

		// - 뒷부분을 추가한다.
		auto sizeWrite2 = __sprintf(buf_option.data<char>(buf_option.size()), SIZE_BUFFER - buf_option.size(), "{ \"tran_no\": \"%d\", \"org_bank_tran_id\": \"%s\", \"org_bank_tran_date\": \"%04u%02u%02u\", \"org_tran_amt\": \"%lld\"}"sv,
			iter.tran_no,						// tran_no
			iter.org_bank_tran_id.c_str(),		// org_bank_tran_id
			static_cast<uint32_t>(org_bank_tran_date.t_year),
			static_cast<uint32_t>(org_bank_tran_date.t_month),
			static_cast<uint32_t>(org_bank_tran_date.t_day),	// org_bank_tran_date
			iter.org_tran_amt					// org_tran_amt
		);

		// check) sprintf가 오류가 났으면 Exception을 던진다.
		THROW_IF(sizeWrite2 < 0, throwable::failure(eRESULT::FAIL));

		// - 길이를 추가한다.
		buf_option.add_size(sizeWrite2);
	}

	// - 뒷부분을 추가한다.
	size_write = __sprintf(buf_option.data<char>(buf_option.size()), SIZE_BUFFER- buf_option.size(), "], \"tran_dtime\": \"%04u%02u%02u%02u%02u%02u\"}"sv,
		static_cast<uint32_t>(_request.tran_dtime.t_year),
		static_cast<uint32_t>(_request.tran_dtime.t_month),
		static_cast<uint32_t>(_request.tran_dtime.t_day),
		static_cast<uint32_t>(_request.tran_dtime.t_hour),
		static_cast<uint32_t>(_request.tran_dtime.t_minute),
		static_cast<uint32_t>(_request.tran_dtime.t_second)
		);

	// check) sprintf가 오류가 났으면 Exception을 던진다.
	THROW_IF(size_write < 0, throwable::failure(eRESULT::FAIL));

	// 6) Option의 길이를 저장한다.
	buf_option.add_size(size_write);

	// 7) Accept 받을 것을 설정.
	std::vector<std::string_view> vector_accept{ "*/*"sv, "text/plain"sv };
	//std::vector<std::string_view> vector_accept{ "*/*"sv, "application/json"sv };

	// 8) request를 Set한다.
	prequest_http->set_request(
		http_method,
		_request.str_url,
		443,
		"HTTP/1.1"sv,
		str_object,
		std::string_view(),
		std::move(vector_header),
		buf_option,
		std::move(vector_accept),
		1		// 0: normal, 1:ssl
	);

	// declare) 
	http_api::bank::RESPONSE_TRANSFER_RESULT response;

	// 9) 기본값을 설정...
	response.rsp_code = eRESPONSE_CODE::A0000_;
	response.rsp_message.clear();

	// 10) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
	prequest_http->set_function([&](sMESSAGE_HTTP_RESPONSE& _msg)
	{
		// check) Result가 0이면 실패이므로 끝낸다.
		if (_msg.status_code != eHTTP_STATUS_CODE::OK_)
		{
			// - 내부처리 중 Error을 명시
			response.rsp_code	 = eRESPONSE_CODE::FAIL_;
			response.rsp_message = _msg.status_message;

			// - buffer를 clear한다.
			response.buf_response.clear();

			// Return)
			return;
		}

		// - 마지막을 null로 바꾼다.
		_msg.contents[0].append<char>(0);

		// - buffer를 넣는다.
		response.buf_response = _msg.contents[0];

		// declare) 
		rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;

		// - Parsing한다.
		const auto&	result = json_data.Parse(_msg.contents[0].data<char>());

		// check) Parsing에 실패했으면 false를 리턴한다.
		RETURN_IF(result.IsNull())

		// declare)
		[[maybe_unused]] std::size_t res_cnt = 0;

		// - 값을 설정한다.
		for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
		{
			auto str_name = (*iter).name.GetString();
			auto& value = (*iter).value;

			// - rsp_code
			if(__strcmp(str_name, "rsp_code"sv) == 0)
			{
				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
			}
			// - rsp_message
			else if(__strcmp(str_name, "rsp_message"sv) == 0)
			{
				response.rsp_message = convert_utf8_to_multibyte(value.GetString());
			}

			// - "api_tran_id"
			else if (__strcmp(str_name, "api_tran_id"sv) == 0)
			{
				response.api_tran_id = value.GetString();
			}
			// - "api_tran_dtm"
			else if (__strcmp(str_name, "api_tran_dtm"sv) == 0)
			{
				response.api_tran_dtm = from_string_date_time_millsec(value.GetString());
			}
			else if (__strcmp(str_name, "res_cnt"sv) == 0)
			{
				res_cnt = __from_string<size_t>(value.GetString());
			}
			// - "res_list"
			else if (__strcmp(str_name, "res_list"sv) == 0)
			{
				for (auto iterRes = value.Begin(); iterRes != value.End(); ++iterRes)
				{
					// declare) 
					http_api::bank::RESPONSE_TRANSFER_RESULT::TRANSACTION transaction;

					for (auto iterNest = iterRes->MemberBegin(); iterNest != iterRes->MemberEnd(); ++iterNest)
					{
						auto  str_name_nested = iterNest->name.GetString();
						auto& value_Nested = iterNest->value;

						// - "tran_no"
						if (__strcmp(str_name_nested, "tran_no"sv) == 0)
						{
							transaction.tran_no = __from_string<int>(value_Nested.GetString());
						}
						// - "bank_tran_id"
						else if (__strcmp(str_name_nested, "bank_tran_id"sv) == 0)
						{
							transaction.bank_tran_id = value_Nested.GetString();
						}
						// - "bank_tran_date"
						else if (__strcmp(str_name_nested, "bank_tran_date"sv) == 0)
						{
							transaction.bank_tran_date = from_string_date(value_Nested.GetString());
						}
						// - "bank_code_tran"
						else if (__strcmp(str_name_nested, "bank_code_tran"sv) == 0)
						{
							transaction.bank_code_tran = value_Nested.GetString();
						}
						// - "bank_rsp_code"
						else if (__strcmp(str_name_nested, "bank_rsp_code"sv) == 0)
						{
							transaction.bank_rsp_code = get_enum_OAUTH_BANK_CODE<char>(value_Nested.GetString());
						}
						// - "bank_rsp_message"
						else if (__strcmp(str_name_nested, "bank_rsp_message"sv) == 0)
						{
							convert_utf8_to_multibyte(transaction.bank_rsp_message, value_Nested.GetString());
						}
						// - "wd_bank_code_std"
						else if (__strcmp(str_name_nested, "wd_bank_code_std"sv) == 0)
						{
							transaction.wd_bank_code_std = get_enum_OAUTH_BANK_CODE<char>(value_Nested.GetString());
						}
						// - "wd_bank_code_sub"
						else if (__strcmp(str_name_nested, "wd_bank_code_sub"sv) == 0)
						{
							transaction.wd_bank_code_sub = __from_string<int64_t>(value_Nested.GetString());
						}
						// - "wd_bank_name"
						else if (__strcmp(str_name_nested, "wd_bank_name"sv) == 0)
						{
							convert_utf8_to_multibyte(transaction.wd_bank_name, value_Nested.GetString());
						}
						// - "wd_fintech_use_num"
						else if (__strcmp(str_name_nested, "wd_fintech_use_num"sv) == 0)
						{
							transaction.wd_fintech_use_num = value_Nested.GetString();
						}
						// - "wd_account_num_masked"
						else if (__strcmp(str_name_nested, "wd_account_num_masked"sv) == 0)
						{
							transaction.wd_account_num_masked = value_Nested.GetString();
						}
						// - "wd_print_content"
						else if (__strcmp(str_name_nested, "wd_print_content"sv) == 0)
						{
							convert_utf8_to_multibyte(transaction.wd_print_content, value_Nested.GetString());
						}
						// - "wd_account_holder_name"
						else if (__strcmp(str_name_nested, "wd_account_holder_name"sv) == 0)
						{
							convert_utf8_to_multibyte(transaction.wd_account_holder_name, value_Nested.GetString());
						}
						// - "dps_bank_code_std"
						else if (__strcmp(str_name_nested, "dps_bank_code_std"sv) == 0)
						{
							transaction.dps_bank_code_std = get_enum_OAUTH_BANK_CODE<char>(value_Nested.GetString());
						}
						// - "dps_bank_code_sub"
						else if (__strcmp(str_name_nested, "dps_bank_code_sub"sv) == 0)
						{
							transaction.dps_bank_code_sub = __from_string<int64_t>(value_Nested.GetString());
						}
						// - "dps_bank_name"
						else if (__strcmp(str_name_nested, "dps_bank_name"sv) == 0)
						{
							convert_utf8_to_multibyte(transaction.dps_bank_name, value_Nested.GetString());
						}
						// - "dps_fintech_use_num"
						else if (__strcmp(str_name_nested, "dps_fintech_use_num"sv) == 0)
						{
							transaction.dps_fintech_use_num = value_Nested.GetString();
						}
						// - "dps_account_num_masked"
						else if (__strcmp(str_name_nested, "dps_account_num_masked"sv) == 0)
						{
							transaction.dps_account_num_masked = value_Nested.GetString();
						}
						// - "dps_print_content"
						else if (__strcmp(str_name_nested, "dps_print_content"sv) == 0)
						{
							convert_utf8_to_multibyte(transaction.dps_print_content, value_Nested.GetString());
						}
						// - "dps_account_holder_name"
						else if (__strcmp(str_name_nested, "dps_account_holder_name"sv) == 0)
						{
							convert_utf8_to_multibyte(transaction.dps_account_holder_name, value_Nested.GetString());
						}
						// - "tran_amt"
						else if (__strcmp(str_name_nested, "tran_amt"sv) == 0)
						{
							transaction.tran_amt = __from_string<int64_t>(value_Nested.GetString());
						}
					}

					response.res_list.push_back(transaction);
				}

				// check) 
				CGASSERT_ERROR(response.res_list.size() == res_cnt);
			}
		}
	});

	// 11) request를 요청한다.
	prequest_http->request();

	// return) 
	return	response;
}
http_api::bank::RESPONSE_TRANSFER_RECHECK http_api::bank::request(const REQUEST_TRANSFER_RECHECK& _request)
{
	// ------------------------------------------------------------------
	// 
	// 사용자정보조회 API
	//
	// 요청 메시지 URL
	//   
	//   2) HTTP URL		/v1.0/transfer/recheck
	//	 3) HTTP Method		POST
	//	 4) Content-Type	"Content-Type: application/json; charset=UTF-8"
	//   5) Header
	//		- Authorization				Bearer<access_token>	오픈플랫폼으로부터 전송받은 access_token을 HTTP Header에 추가 [scope=login]
	//
	//   6) Parameters
	//		- check_type				"1"					AN(1)  1:출금이체, 2:입금이체
	//		- req_cnt					"25"				N(5)   입금요청건수
	//		- req_list					<object>			       입금요청목록
	//		-   tran_no					"1"					N(5)   거래순번
	//		-   org_bank_tran_dtime		"20160725100010"	AN(14) 원거래요청일시
	//		-   org_req_gubun			"1"					AN(1)  원거래요청계좌지정방식구분 1:핀테크이ㅣ용번호, 2:계좌번호
	//		-   print_content			"입금계좌인자내역"	AH(20) 원거래 입출금계좌인자내역
	//		-   fintech_use_num			"1234567890123456"	AN(24) 핀테크이용번호
	//		-   org_tran_amt			"10000"				N(12)  원거래 거래금액
	//		- tran_dtime				"20160310101921"	N(14)  요청일시
	//
	// ------------------------------------------------------------------
	// declare) 
	const size_t SIZE_BUFFER = 8192;

	// check)
	CGASSERT_ERROR(_request.str_url.empty() == false);

	// check)
	THROW_IF(_request.str_url.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
	auto prequest_http = make_object<query::http::query>();

	// declare)
	auto str_object	= "/v1.0/transfer/recheck"sv;

	// 2) HTTP Method를 설정한다.
	auto http_method = eHTTP_METHOD::POST_;

	// declare)
	auto buf_header = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));

	// 3) HTTP URL을 설정한다.(URL중 Path부분만...)
	auto size_write = __sprintf(buf_header.data<char>(), SIZE_BUFFER, "Authorization: Bearer %s"sv, _request.authorization.c_str());

	// check) sprintf가 오류가 났으면 Exception을 던진다.
	THROW_IF(size_write < 0, throwable::failure(eRESULT::FAIL));

	// 4) Header를 설정한다 - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
	std::vector<std::string_view> vector_header{ "Content-Type: application/json; charset=UTF-8"sv, buf_header.data<char>() };

	// 5) parameter를 설정한다.
	auto buf_option = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));

	// - ...
	size_write = __sprintf(buf_option.data<char>(), SIZE_BUFFER, "{\"check_type\": \"%s\", \"req_cnt\": \"%llu\", \"req_list\": ["sv,
		to_string<char>(_request.check_type).data(),
		_request.req_list.size()
		);

	// check) sprintf가 오류가 났으면 Exception을 던진다.
	THROW_IF(size_write < 0, throwable::failure(eRESULT::FAIL));

	// - 길이를 추가한다.
	buf_option.add_size(size_write);

	// - req_list를 추가한다.
	for(auto iter: _request.req_list)
	{
		// declare) 
		char str_print_content[256];

		// - 문자열변환을 수행한다.
		convert_multibyte_to_utf8(str_print_content, iter.print_content.c_str());

		// - 
		const auto& org_tran_dtime = iter.org_tran_dtime;

		// - 뒷부분을 추가한다.
		auto sizeWrite2 = __sprintf(buf_option.data<char>(buf_option.size()), SIZE_BUFFER - buf_option.size(), "{ \"tran_no\": \"%d\", \"org_tran_dtime\": \"%04u%02u%02u%02u%02u%02u\", \"org_req_gubun\": \"%s\", \"bank_code_std\": \"%03d\", \"account_num\": \"%s\", \"print_content\": \"%s\", \"fintech_use_num\": \"%s\", \"org_tran_amt\": \"%lld\"}"sv,
			iter.tran_no,					// tran_no
			static_cast<uint32_t>(org_tran_dtime.t_year),
			static_cast<uint32_t>(org_tran_dtime.t_month),
			static_cast<uint32_t>(org_tran_dtime.t_day),
			static_cast<uint32_t>(org_tran_dtime.t_hour),
			static_cast<uint32_t>(org_tran_dtime.t_minute),
			static_cast<uint32_t>(org_tran_dtime.t_second),
			to_string<char>(iter.org_req_gubun).data(), // org_req_gubun
			iter.bank_code_std,				// bank_code_std
			iter.accoount_num.c_str(),		// account_num
			str_print_content,				// print_content
			iter.fintech_use_num.c_str(),	// fintech_use_num
			iter.org_tran_amt				// org_tran_amt
		);

		// check) sprintf가 오류가 났으면 Exception을 던진다.
		THROW_IF(sizeWrite2 < 0, throwable::failure(eRESULT::FAIL));

		// - 길이를 추가한다.
		buf_option.add_size(sizeWrite2);
	}

	// - 뒷부분을 추가한다.
	size_write = __sprintf(buf_option.data<char>(buf_option.size()), SIZE_BUFFER- buf_option.size(), "], \"tran_dtime\": \"%04u%02u%02u%02u%02u%02u\"}"sv,
		static_cast<uint32_t>(_request.tran_dtime.t_year),
		static_cast<uint32_t>(_request.tran_dtime.t_month),
		static_cast<uint32_t>(_request.tran_dtime.t_day),
		static_cast<uint32_t>(_request.tran_dtime.t_hour),
		static_cast<uint32_t>(_request.tran_dtime.t_minute),
		static_cast<uint32_t>(_request.tran_dtime.t_second)
		);

	// check) sprintf가 오류가 났으면 Exception을 던진다.
	THROW_IF(size_write < 0, throwable::failure(eRESULT::FAIL));

	// 5) Option의 길이를 저장한다.
	buf_option.add_size(size_write);

	// 6) Accept 받을 것을 설정.
	std::vector<std::string_view> vector_accept{ "*/*"sv, "text/plain"sv };
	//std::vector<std::string_view> vector_accept{ "*/*"sv, "application/json"sv };

	// 7) request를 Set한다.
	prequest_http->set_request(
		http_method,
		_request.str_url,
		443,
		"HTTP/1.1"sv,
		str_object,
		std::string_view(),
		std::move(vector_header),
		buf_option,
		std::move(vector_accept),
		1		// 0: normal, 1:ssl
	);

	// declare) 
	http_api::bank::RESPONSE_TRANSFER_RECHECK response;

	// 8) 기본값을 설정...
	response.rsp_code = eRESPONSE_CODE::A0000_;
	response.rsp_message.clear();

	// 9) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
	prequest_http->set_function([&](sMESSAGE_HTTP_RESPONSE& _msg)
	{
		// check) Result가 0이면 실패이므로 끝낸다.
		if (_msg.status_code != eHTTP_STATUS_CODE::OK_)
		{
			// - 내부처리 중 Error을 명시
			response.rsp_code = eRESPONSE_CODE::FAIL_;
			response.rsp_message = _msg.status_message;

			// - buffer를 clear한다.
			response.buf_response.clear();

			// Return)
			return;
		}

		// - 마지막을 null로 바꾼다.
		_msg.contents[0].append<char>(0);

		// - buffer를 넣는다.
		response.buf_response = _msg.contents[0];

		// declare) 
		rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;

		// - Parsing한다.
		const auto&	result = json_data.Parse(_msg.contents[0].data<char>());

		// check) Parsing에 실패했으면 false를 리턴한다.
		RETURN_IF(result.IsNull())

		// declare)
		[[maybe_unused]] std::size_t res_cnt = 0;

		// - 값을 설정한다.
		for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
		{
			auto str_name = (*iter).name.GetString();
			auto& value = (*iter).value;

			// - rsp_code
			if(__strcmp(str_name, "rsp_code"sv) == 0)
			{
				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
			}
			// - rsp_message
			else if(__strcmp(str_name, "rsp_message"sv) == 0)
			{
				response.rsp_message = convert_utf8_to_multibyte(value.GetString());
			}

			// - "api_tran_id"
			else if (__strcmp(str_name, "api_tran_id"sv) == 0)
			{
				response.api_tran_id = value.GetString();
			}
			// - "api_tran_dtm"
			else if (__strcmp(str_name, "api_tran_dtm"sv) == 0)
			{
				response.api_tran_dtm = from_string_date_time_millsec(value.GetString());
			}
			else if (__strcmp(str_name, "res_cnt"sv) == 0)
			{
				res_cnt = __from_string<size_t>(value.GetString());
			}
			// - "res_list"
			else if (__strcmp(str_name, "res_list"sv) == 0)
			{
				for (auto iterRes = value.Begin(); iterRes != value.End(); ++iterRes)
				{
					// declare) 
					http_api::bank::RESPONSE_TRANSFER_RECHECK::TRANSACTION	transaction;

					for (auto iterNest = iterRes->MemberBegin(); iterNest != iterRes->MemberEnd(); ++iterNest)
					{
						auto	str_name_nested = iterNest->name.GetString();
						auto&	value_Nested = iterNest->value;

						// - "tran_no"
						if (__strcmp(str_name_nested, "tran_no"sv) == 0)
						{
							transaction.tran_no = __from_string<int>(value_Nested.GetString());
						}
						// - "bank_tran_id"
						else if (__strcmp(str_name_nested, "bank_tran_id"sv) == 0)
						{
							transaction.bank_tran_id = value_Nested.GetString();
						}
						// - "bank_tran_date"
						else if (__strcmp(str_name_nested, "bank_tran_date"sv) == 0)
						{
							transaction.bank_tran_date = from_string_date(value_Nested.GetString());
						}
						// - "bank_code_tran"
						else if (__strcmp(str_name_nested, "bank_code_tran"sv) == 0)
						{
							transaction.bank_code_tran = value_Nested.GetString();
						}
						// - "bank_rsp_code"
						else if (__strcmp(str_name_nested, "bank_rsp_code"sv) == 0)
						{
							transaction.bank_rsp_code = get_enum_OAUTH_BANK_CODE<char>(value_Nested.GetString());
						}
						// - "bank_rsp_message"
						else if (__strcmp(str_name_nested, "bank_rsp_message"sv) == 0)
						{
							convert_utf8_to_multibyte(transaction.bank_rsp_message, value_Nested.GetString());
						}
						// - "wd_bank_code_std"
						else if (__strcmp(str_name_nested, "wd_bank_code_std") == 0)
						{
							transaction.wd_bank_code_std = get_enum_OAUTH_BANK_CODE<char>(value_Nested.GetString());
						}
						// - "wd_bank_code_sub"
						else if (__strcmp(str_name_nested, "wd_bank_code_sub"sv) == 0)
						{
							transaction.wd_bank_code_sub = __from_string<int64_t>(value_Nested.GetString());
						}
						// - "wd_bank_name"
						else if (__strcmp(str_name_nested, "wd_bank_name"sv) == 0)
						{
							convert_utf8_to_multibyte(transaction.wd_bank_name, value_Nested.GetString());
						}
						// - "wd_fintech_use_num"
						else if (__strcmp(str_name_nested, "wd_fintech_use_num"sv) == 0)
						{
							transaction.wd_fintech_use_num = value_Nested.GetString();
						}
						// - "wd_account_num_masked"
						else if (__strcmp(str_name_nested, "wd_account_num_masked"sv) == 0)
						{
							transaction.wd_account_num_masked = value_Nested.GetString();
						}
						// - "wd_print_content"
						else if (__strcmp(str_name_nested, "wd_print_content"sv) == 0)
						{
							convert_utf8_to_multibyte(transaction.wd_print_content, value_Nested.GetString());
						}
						// - "wd_account_holder_name"
						else if (__strcmp(str_name_nested, "wd_account_holder_name"sv) == 0)
						{
							convert_utf8_to_multibyte(transaction.wd_account_holder_name, value_Nested.GetString());
						}
						// - "dps_bank_code_std"
						else if (__strcmp(str_name_nested, "dps_bank_code_std"sv) == 0)
						{
							transaction.dps_bank_code_std = get_enum_OAUTH_BANK_CODE<char>(value_Nested.GetString());
						}
						// - "dps_bank_code_sub"
						else if (__strcmp(str_name_nested, "dps_bank_code_sub"sv) == 0)
						{
							transaction.dps_bank_code_sub = __from_string<int64_t>(value_Nested.GetString());
						}
						// - "dps_bank_name"
						else if (__strcmp(str_name_nested, "dps_bank_name"sv) == 0)
						{
							convert_utf8_to_multibyte(transaction.dps_bank_name, value_Nested.GetString());
						}
						// - "dps_fintech_use_num"
						else if (__strcmp(str_name_nested, "dps_fintech_use_num"sv) == 0)
						{
							transaction.dps_fintech_use_num = value_Nested.GetString();
						}
						// - "dps_account_num_masked"
						else if (__strcmp(str_name_nested, "dps_account_num_masked"sv) == 0)
						{
							transaction.dps_account_num_masked = value_Nested.GetString();
						}
						// - "dps_print_content"
						else if (__strcmp(str_name_nested, "dps_print_content"sv) == 0)
						{
							convert_utf8_to_multibyte(transaction.dps_print_content, value_Nested.GetString());
						}
						// - "dps_account_holder_name"
						else if (__strcmp(str_name_nested, "dps_account_holder_name"sv) == 0)
						{
							convert_utf8_to_multibyte(transaction.dps_account_holder_name, value_Nested.GetString());
						}
						// - "tran_amt"
						else if (__strcmp(str_name_nested, "tran_amt"sv) == 0)
						{
							transaction.tran_amt = __from_string<int64_t>(value_Nested.GetString());
						}
					}

					response.res_list.push_back(transaction);
				}

				// check) 
				CGASSERT_ERROR(response.res_list.size() == res_cnt);
			}
		}
	});

	// 10) request를 요청한다.
	prequest_http->request();

	// return) 
	return	response;
}
http_api::bank::RESPONSE_INQUIRY_REAL_NAME http_api::bank::request(const REQUEST_INQUIRY_REAL_NAME& _request)
{
	// ------------------------------------------------------------------
	// 
	// 사용자정보조회 API
	//
	// 요청 메시지 URL
	//   
	//   2) HTTP URL		/v1.0/inquiry/realname
	//	 3) HTTP Method		POST
	//	 4) Content-Type	"Content-Type: application/json; charset=UTF-8"
	//   5) Header
	//		- Authorization				Bearer<access_token>	오픈플랫폼으로부터 전송받은 access_token을 HTTP Header에 추가 [scope=login]
	//
	//   6) Parameters
	//		- bank_code_std				"097"					AN(3)  개설기관 표준코드
	//		- account_num				"1234567890123123"	AN(16) 개좌번호
	//		- account_holder_info		"8801012"				AN(7)  예금주 인증정보 YYYYMMDD(6) + 식별정보(1)
	//		- tran_dtime				"20160310101921"		N(14)  요청일시
	//
	// ------------------------------------------------------------------
	// declare) 
	const std::size_t SIZE_BUFFER = 8192;

	// check)
	CGASSERT_ERROR(_request.str_url.empty() == false);

	// check)
	THROW_IF(_request.str_url.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
	auto prequest_http = make_object<query::http::query>();

	// declare)
	auto str_object = "/v1.0/inquiry/real_name"sv;

	// 2) HTTP Method를 설정한다.
	auto http_method = eHTTP_METHOD::POST_;

	// declare)
	auto buf_header = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));

	// 3) HTTP URL을 설정한다.(URL중 Path부분만...)
	auto size_write = __sprintf(buf_header.data<char>(), SIZE_BUFFER, "Authorization: Bearer %s"sv, _request.authorization.c_str());

	// check) sprintf가 오류가 났으면 Exception을 던진다.
	THROW_IF(size_write < 0, throwable::failure(eRESULT::FAIL));

	// 4) Header를 설정한다 - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
	std::vector<std::string_view> vector_header{ "Content-Type: application/json; charset=UTF-8"sv, buf_header.data<char>() };

	// 5) parameter를 설정한다.
	auto buf_option = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));

	// decalre) 
	auto& tran_dtime = _request.tran_dtime;

	// - ...
	size_write = __sprintf(buf_option.data<char>(), SIZE_BUFFER, "{\"bank_code_std\": \"%03d\", \"account_num\": \"%s\", \"account_holder_info\": \"%s\", \"tran_dtime\": \"%04u%02u%02u%02u%02u%02u\"}"sv,
		_request.bank_code_std,
		_request.accoount_num.c_str(),
		_request.accoount_holder_info.c_str(),
		static_cast<uint32_t>(tran_dtime.t_year),
		static_cast<uint32_t>(tran_dtime.t_month),
		static_cast<uint32_t>(tran_dtime.t_day),
		static_cast<uint32_t>(tran_dtime.t_hour),
		static_cast<uint32_t>(tran_dtime.t_minute),
		static_cast<uint32_t>(tran_dtime.t_second)
	);

	// check) sprintf가 오류가 났으면 Exception을 던진다.
	THROW_IF(size_write < 0, throwable::failure(eRESULT::FAIL));

	// - 길이를 추가한다.
	buf_option.set_size(size_write);

	// 6) Accept 받을 것을 설정.
	std::vector<std::string_view> vector_accept{ "*/*"sv, "text/plain"sv };
	//std::vector<std::string_view>	vector_accept{ "*/*"sv, "application/json"sv };

	// 7) request를 Set한다.
	prequest_http->set_request(
		http_method,
		_request.str_url,
		443,
		"HTTP/1.1",
		str_object,
		std::string_view(),
		std::move(vector_header),
		buf_option,
		std::move(vector_accept),
		1		// 0: normal, 1:ssl
	);

	// declare) 
	http_api::bank::RESPONSE_INQUIRY_REAL_NAME	response;

	// 7) 기본값을 설정...
	response.rsp_code = eRESPONSE_CODE::A0000_;
	response.rsp_message.clear();

	// 8) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
	prequest_http->set_function([&](sMESSAGE_HTTP_RESPONSE& _msg)
	{
		// check) Result가 0이면 실패이므로 끝낸다.
		if (_msg.status_code != eHTTP_STATUS_CODE::OK_)
		{
			// - 내부처리 중 Error을 명시
			response.rsp_code = eRESPONSE_CODE::FAIL_;
			response.rsp_message = _msg.status_message;

			// - buffer를 clear한다.
			response.buf_response.clear();

			// Return)
			return;
		}

		// - 마지막을 null로 바꾼다.
		_msg.contents[0].append<char>(0);

		// - buffer를 넣는다.
		response.buf_response = _msg.contents[0];

		// declare) 
		rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;

		// - Parsing한다.
		const auto&	result = json_data.Parse(_msg.contents[0].data<char>());

		// check) Parsing에 실패했으면 false를 리턴한다.
		RETURN_IF(result.IsNull())

		// - 값을 설정한다.
		for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
		{
			auto str_name = (*iter).name.GetString();
			auto& value = (*iter).value;

			// - rsp_code
			if(__strcmp(str_name, "rsp_code"sv)==0)
			{
				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
			}
			// - rsp_message
			else if(__strcmp(str_name, "rsp_message"sv) == 0)
			{
				response.rsp_message = convert_utf8_to_multibyte(value.GetString());
			}

			// - "api_tran_id"
			else if (__strcmp(str_name, "api_tran_id"sv) == 0)
			{
				response.api_tran_id = value.GetString();
			}
			// - "api_tran_dtm"
			else if (__strcmp(str_name, "api_tran_dtm"sv) == 0)
			{
				response.api_tran_dtm = from_string_date_time_millsec(value.GetString());
			}

			// - "bank_tran_id"
			else if (__strcmp(str_name, "bank_tran_id"sv) == 0)
			{
				response.bank_tran_id = value.GetString();
			}
			// - "bank_tran_date"
			else if (__strcmp(str_name, "bank_tran_date"sv) == 0)
			{
				response.bank_tran_date = from_string_date(value.GetString());
			}
			// - "bank_code_tran"
			else if (__strcmp(str_name, "bank_code_tran"sv) == 0)
			{
				response.bank_code_tran = value.GetString();
			}
			// - "bank_rsp_code"
			else if (__strcmp(str_name, "bank_rsp_code"sv) == 0)
			{
				response.bank_rsp_code = get_enum_OAUTH_BANK_CODE<char>(value.GetString());
			}
			// - "bank_rsp_message"
			else if (__strcmp(str_name, "bank_rsp_message"sv) == 0)
			{
				convert_utf8_to_multibyte(response.bank_rsp_message, value.GetString());
			}
			// - "bank_code_std"
			else if (__strcmp(str_name, "bank_code_std"sv) == 0)
			{
				response.bank_code_std = get_enum_OAUTH_BANK_CODE<char>(value.GetString());
			}
			// - "bank_code_sub"
			else if (__strcmp(str_name, "bank_code_sub"sv) == 0)
			{
				response.bank_code_sub = __from_string<int64_t>(value.GetString());
			}
			// - "bank_name"
			else if (__strcmp(str_name, "bank_name"sv) == 0)
			{
				convert_utf8_to_multibyte(response.bank_name, value.GetString());
			}
			// - "account_num"
			else if (__strcmp(str_name, "account_num"sv) == 0)
			{
				response.account_num = value.GetString();
			}
			// - "account_holder_info"
			else if (__strcmp(str_name, "account_holder_info"sv) == 0)
			{
				response.account_holder_info = value.GetString();
			}
			// - "account_holder_name"
			else if (__strcmp(str_name, "account_holder_name"sv) == 0)
			{
				convert_utf8_to_multibyte(response.account_holder_name, value.GetString());
			}
		}
	});

	// 9) request를 요청한다.
	prequest_http->request();

	// return) 
	return	response;
}
http_api::bank::RESPONSE_BANK_STATUS http_api::bank::request(const REQUEST_BANK_STATUS& _request)
{
	// ------------------------------------------------------------------
	// 
	// 참가기관상태조회
	//
	// 요청 메시지 URL
	//   
	//   2) HTTP URL		/v1.0/bank/status
	//	 3) HTTP Method		GET
	//	 4) Content-Type	"Content-Type: application/x-www-form-urlencoded; charset=UTF-8"
	//   5) Header
	//		- Authorization				Bearer<access_token>	오픈플랫폼으로부터 전송받은 access_token을 HTTP Header에 추가 [scope=login]
	//
	// ------------------------------------------------------------------
	// declare)
	const std::size_t SIZE_BUFFER = 8192;

	// check)
	CGASSERT_ERROR(_request.str_url.empty() == false);

	// check)
	THROW_IF(_request.str_url.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
	auto prequest_http = make_object<query::http::query>();

	// 2) HTTP URL을 설정한다.(URL중 Path부분만...)
	auto str_object = "/v1.0/bank/status"sv;

	// 3) HTTP Method를 설정한다.
	auto http_method = eHTTP_METHOD::GET_;

	// declare)
	auto buf_header = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));

	// 4) HTTP URL을 설정한다.(URL중 Path부분만...)
	auto size_write = __sprintf(buf_header.data<char>(), SIZE_BUFFER, "Authorization: Bearer %s"sv, _request.authorization.c_str());

	// check) sprintf가 오류가 났으면 Exception을 던진다.
	THROW_IF(size_write < 0, throwable::failure(eRESULT::FAIL));

	// 5) Header를 설정한다 - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
	std::vector<std::string_view> vector_header{ "Content-Type: application/x-www-form-urlencoded; charset=UTF-8"sv, buf_header.data<char>() };

	// declare) 
	shared_buffer buf_option;

	// 6) Accept 받을 것을 설정.
	std::vector<std::string_view> vector_accept{ "*/*"sv, "text/plain"sv };

	// 7) request를 Set한다.
	prequest_http->set_request(
		http_method,
		_request.str_url,
		443,
		"HTTP/1.1"sv,
		str_object,
		std::string_view(),
		std::move(vector_header),
		buf_option,
		std::move(vector_accept),
		1		// 0: normal, 1:ssl
	);

	// declare) 
	http_api::bank::RESPONSE_BANK_STATUS response;

	// 8) 기본값을 설정...
	response.rsp_code = eRESPONSE_CODE::A0000_;
	response.rsp_message.clear();

	// 9) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
	prequest_http->set_function([&](sMESSAGE_HTTP_RESPONSE& _msg)
	{
		// check) Result가 0이면 실패이므로 끝낸다.
		if (_msg.status_code != eHTTP_STATUS_CODE::OK_)
		{
			// - 내부처리 중 Error을 명시
			response.rsp_code = eRESPONSE_CODE::FAIL_;
			response.rsp_message = _msg.status_message;

			// - buffer를 clear한다.
			response.buf_response.clear();

			// Return)
			return;
		}

		// - 마지막을 null로 바꾼다.
		_msg.contents[0].append<char>(0);

		// - buffer를 넣는다.
		response.buf_response = _msg.contents[0];

		// declare) 
		rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;

		// - Parsing한다.
		const auto&	result = json_data.Parse(_msg.contents[0].data<char>());

		// check) Parsing에 실패했으면 false를 리턴한다.
		RETURN_IF(result.IsNull())

		// - 값을 설정한다.
		for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
		{
			auto str_name = (*iter).name.GetString();
			auto& value = (*iter).value;

			// - rsp_code
			if(__strcmp(str_name, "rsp_code")==0)
			{
				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
			}
			// - rsp_message
			else if(__strcmp(str_name, "rsp_message"sv) == 0)
			{
				response.rsp_message = convert_utf8_to_multibyte(value.GetString());
			}

			// - "api_tran_id"
			else if (__strcmp(str_name, "api_tran_id"sv) == 0)
			{
				response.api_tran_id = value.GetString();
			}
			// - "api_tran_dtm"
			else if (__strcmp(str_name, "api_tran_dtm"sv) == 0)
			{
				response.api_tran_dtm = from_string_date_time_millsec(value.GetString());
			}
			// - "res_list"
			else if (__strcmp(str_name, "res_list"sv) == 0)
			{
				for (auto iterRes = value.Begin(); iterRes != value.End(); ++iterRes)
				{
					// declare) 
					http_api::bank::RESPONSE_BANK_STATUS::BANK_STATUS	bankStatus;

					for (auto iterNest = iterRes->MemberBegin(); iterNest != iterRes->MemberEnd(); ++iterNest)
					{
						auto	str_name_nested = iterNest->name.GetString();
						auto&	value_Nested = iterNest->value;

						// - "bank_code_std"
						if (__strcmp(str_name_nested, "bank_code_std"sv) == 0)
						{
							bankStatus.bank_code_std = __from_string<int>(value_Nested.GetString());
						}
						// - "bank_name"
						else if (__strcmp(str_name_nested, "bank_name"sv) == 0)
						{
							convert_utf8_to_multibyte(bankStatus.bank_name, value_Nested.GetString());
						}
						// - "bank_status"
						else if (__strcmp(str_name_nested, "bank_status"sv) == 0)
						{
							bankStatus.bank_status = get_enum_OAUTH_BANK_STATUS<char>(value_Nested.GetString());
						}
					}

					// - ...
					response.res_list.push_back(bankStatus);
				}
			}
		}
	});

	// 10) request를 요청한다.
	prequest_http->request();

	// return) 
	return	response;
}


}