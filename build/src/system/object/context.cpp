#include "pch.h"

#pragma push_macro("new")

#undef new

#ifdef _WIN32
	#pragma warning(disable:6102)
	#pragma warning(disable:6313)
	#pragma warning(disable:6385)
	#pragma warning(disable:4996)

#endif


#if !defined(__linux__)
	#pragma warning(disable:5054)
	//#pragma diag_suppress 177 // supress #177-D function  was declared but never referenced
	#include "rapidjson/document.h"
	#include "rapidjson/writer.h"
	#include "rapidjson/stringbuffer.h"
	#include "rapidjson/filereadstream.h"
	#include "rapidjson/filewritestream.h"
	#pragma warning(default:5054)
#else
	#include "../../common/rapidjson/document.h"
	#include "../../common/rapidjson/writer.h"
	#include "../../common/rapidjson/stringbuffer.h"
	#include "../../common/rapidjson/filereadstream.h"
	#include "../../common/rapidjson/filewritestream.h"
#endif

#ifdef _WIN32
	#pragma warning(default:6102)
	#pragma warning(default:6313)
	#pragma warning(default:6385)
	#pragma warning(default:4996)
#endif
#pragma pop_macro("new")


namespace CGDK
{


template<class TTYPE, class TDOCUMENT = rapidjson::UTF8<TTYPE>>
void add_json_array(int _index, context& _context, rapidjson::GenericValue<TDOCUMENT>& _value);

template<class TTYPE, class TDOCUMENT=rapidjson::UTF8<TTYPE>>
void add_json_member(context& _context, typename rapidjson::GenericDocument<TDOCUMENT>::ConstMemberIterator& _jsonMember)
{
	auto str_name = (*_jsonMember).name.GetString();
	auto& json_value = (*_jsonMember).value;
	
	switch (json_value.GetType())
	{
	case	rapidjson::Type::kNullType:      //!< null
			{
				_context.insert(str_name, eCONTEXT_VALUE_TYPE::NULL_);
			}
			break;

	case	rapidjson::Type::kFalseType:     //!< false
			{
				_context.insert_bool(str_name, false);
			}
			break;

	case	rapidjson::Type::kTrueType:      //!< true
			{
				_context.insert_bool(str_name, true);
			}
			break;

	case	rapidjson::Type::kObjectType:    //!< object
			{
				auto& newContext = _context.insert(str_name, eCONTEXT_VALUE_TYPE::OBJECT_);

				for (auto iter = json_value.MemberBegin(), iter_end = json_value.MemberEnd(); iter != iter_end; ++iter)
				{
					add_json_member<TTYPE, TDOCUMENT>(newContext, iter);
				}
			}
			break;

	case	rapidjson::Type::kArrayType:     //!< array 
			{
				auto& newContext = _context.insert(str_name, eCONTEXT_VALUE_TYPE::ARRAY_);
				int iCount = 0;

				for (auto iter = json_value.Begin(); iter != json_value.End(); ++iter, ++iCount)
				{
					add_json_array<TTYPE, TDOCUMENT>(iCount, newContext, *iter);
				}
			}
			break;

	case	rapidjson::Type::kStringType:    //!< string
			{
				_context.insert_string(str_name, json_value.GetString());
			}
			break;

	case	rapidjson::Type::kNumberType:     //!< number
			{
				_context.insert_integer(str_name, json_value.GetInt64());

				//GetDouble()
			}
			break;

	default:
			throw	std::exception();
	};
}

template<class TTYPE, class TDOCUMENT>
void add_json_array(int _index, context& _context, const rapidjson::GenericValue<TDOCUMENT>& _value)
{
	// declare) 
	TTYPE str_name[256]{0};

	// 1) ...
	__to_string(str_name, _index);

	switch (_value.GetType())
	{
	case	rapidjson::Type::kNullType:      //!< null
			{
				_context.insert(str_name, eCONTEXT_VALUE_TYPE::NULL_);
			}
			break;

	case	rapidjson::Type::kFalseType:     //!< false
			{
				_context.insert_bool(str_name, false);
			}
			break;

	case	rapidjson::Type::kTrueType:      //!< true
			{
				_context.insert_bool(str_name, true);
			}
			break;

	case	rapidjson::Type::kObjectType:    //!< object
			{
				auto& newContext = _context.insert(str_name, eCONTEXT_VALUE_TYPE::OBJECT_);

				for (auto iter = _value.MemberBegin(), iter_end = _value.MemberEnd(); iter != iter_end; ++iter)
				{
					add_json_member<TTYPE, TDOCUMENT>(newContext, iter);
				}
			}
			break;

	case	rapidjson::Type::kArrayType:     //!< array 
			{
				auto& newContext = _context.insert(str_name, eCONTEXT_VALUE_TYPE::ARRAY_);
				int iCount = 0;

				for (auto iter = _value.Begin(); iter != _value.End(); ++iter, ++iCount)
				{
					add_json_array<TTYPE, TDOCUMENT>(iCount, newContext, *iter);
				}
			}
			break;

	case	rapidjson::Type::kStringType:    //!< string
			{
				_context.insert_string(str_name, _value.GetString());
			}
			break;

	case	rapidjson::Type::kNumberType:     //!< number
			{
				_context.insert_integer(str_name, _value.GetInt64());
			}
			break;

	default:
			throw	std::exception();
	};
}

std::string_view json_error_string(rapidjson::ParseErrorCode _JSON_error_code)
{
	// declare) 
	std::string_view result;

	switch(_JSON_error_code)
	{
	case	rapidjson::ParseErrorCode::kParseErrorNone:
			result	 = "No error (kParseErrorNone)";
			break;

	case	rapidjson::ParseErrorCode::kParseErrorDocumentEmpty:
			result	 = "The document is empty (kParseErrorDocumentEmpty)";
			break;
	case	rapidjson::ParseErrorCode::kParseErrorDocumentRootNotSingular:
			result = "The document root must not follow by other values (kParseErrorDocumentRootNotSingular)";
			break;

	case	rapidjson::ParseErrorCode::kParseErrorValueInvalid:
			result = "Invalid value (kParseErrorValueInvalid)";
			break;

	case	rapidjson::ParseErrorCode::kParseErrorObjectMissName:
			result = "Missing a name for object member (kParseErrorObjectMissName)";
			break;
	case	rapidjson::ParseErrorCode::kParseErrorObjectMissColon:
			result = "Missing a colon after a name of object member (kParseErrorObjectMissColon)";
			break;
	case	rapidjson::ParseErrorCode::kParseErrorObjectMissCommaOrCurlyBracket:
			result = "Missing a comma or '}' after an object member (kParseErrorObjectMissCommaOrCurlyBracket)";
			break;

	case	rapidjson::ParseErrorCode::kParseErrorArrayMissCommaOrSquareBracket:
			result = "Missing a comma or ']' after an array element (kParseErrorArrayMissCommaOrSquareBracket)";
			break;

	case	rapidjson::ParseErrorCode::kParseErrorStringUnicodeEscapeInvalidHex:
			result = "Incorrect hex digit after \\u escape in string (kParseErrorStringUnicodeEscapeInvalidHex)";
			break;
	case	rapidjson::ParseErrorCode::kParseErrorStringUnicodeSurrogateInvalid:
			result = "The surrogate pair in string is invalid (kParseErrorStringUnicodeSurrogateInvalid)";
			break;
	case	rapidjson::ParseErrorCode::kParseErrorStringEscapeInvalid:
			result = "Invalid escape character in string (kParseErrorStringEscapeInvalid)";
			break;
	case	rapidjson::ParseErrorCode::kParseErrorStringMissQuotationMark:
			result = "Missing a closing quotation mark in string (kParseErrorStringMissQuotationMark)";
			break;
	case	rapidjson::ParseErrorCode::kParseErrorStringInvalidEncoding:
			result = "Invalid encoding in string (kParseErrorStringInvalidEncoding)";
			break;

	case	rapidjson::ParseErrorCode::kParseErrorNumberTooBig:
			result = "Number too big to be stored in double (kParseErrorNumberTooBig)";
			break;
	case	rapidjson::ParseErrorCode::kParseErrorNumberMissFraction:
			result = "Miss fraction part in number (kParseErrorNumberMissFraction)";
			break;
	case	rapidjson::ParseErrorCode::kParseErrorNumberMissExponent:
			result = "Miss exponent in number (kParseErrorNumberMissExponent)";
			break;

	case	rapidjson::ParseErrorCode::kParseErrorTermination:
			result = "Parsing was terminated (kParseErrorTermination)";
			break;
	case	rapidjson::ParseErrorCode::kParseErrorUnspecificSyntaxError:
			result = "Unspecific syntax error (kParseErrorUnspecificSyntaxError)";
			break;
	default:
			result = "Unknowned error";
			break;
	}

	// return)
	return	result;
}

bool context::read_from_json_file(const std::string_view _filename, [[maybe_unused]] eFILE_LOCATION _file_location)
{
	// Declare
	bool bReturn = true;

	shared_buffer read_buffer;

#if 0
	// 1) File open
	auto fp = api::file::open(_filename, eFILE_OPERATION::READ, _file_location);

	// check)
	RETURN_IF(fp == INVALID_FILE_HANDLE, false);
#else
#ifdef _WIN32
	std::FILE* fp = nullptr ;
	
	auto error_result = fopen_s(&fp, _filename.data(), "rb");

	// check)
	RETURN_IF(error_result != 0, false);
#else
	std::FILE* fp = std::fopen(_filename.data(), "rb");

	// check)
	RETURN_IF(fp == nullptr, false);
#endif
#endif

	// 2) 버퍼를 할당한다.
	try
	{
		read_buffer = alloc_shared_buffer(65536);
	}
	catch(...)
	{
		// - 
		api::file::close(fp);

		// return) 실패를 리턴한다.
		return	false;
	}

	try
	{
		// 3) file Stream을 연다.
		rapidjson::FileReadStream is(fp, read_buffer.data(), read_buffer.get_source()->size());

		// 4) Text의 첫 Type를 읽는다.
		switch(static_cast<uint8_t>(is.Peek()))
		{
		// Case) UCS-2LE or UTP-16 Little Endian
		case	0xff:
				{
					// - 첫번째 바이트를 끄집어 낸다.
					is.Take();

					// - 두번째 바이트를 읽는다.
					auto const p = static_cast<uint8_t>(is.Take());

					// - 처음 두바이트가 'FF FE' 일 경우 -> UCS-2LE or UTP-16 Little Endian
					if (p != 0xfe)
						throw std::exception();

					// check) 아직 지원하도록 작업하지 않음.
					CGASSERT_ERROR(false);

					//// - UTF16LE를 Source로 하는 Document를 생성한다.
					//rapidjson::GenericDocument<rapidjson::UTF16LE<char>> json_data;

					//// - Parsing한다.
					//auto&	bResult = json_data.ParseStream(is);

					//// check) Parsing에 실패했을 경우 Exception을 던진다.
					//THROW_IF(bResult.IsNull(), std::exception());

					//// - 읽어들인 JSON값들을 저장한다.
					//for (auto iter = json_data.MemberBegin(), iter_end = json_data.MemberEnd(); iter != iter_end; ++iter)
					//{
					//	add_json_member<char, rapidjson::UTF16LE<char>>(*this, iter);
					//}
				}
				break;

		// Case) UCS-2BE or UTF-16 Big Endian
		case	0xfe:
				{
					// - 첫번째 바이트를 끄집어 낸다.
					is.Take();

					// - 두번째 바이트를 읽는다.
					const auto p = static_cast<uint8_t>(is.Take());

					// - 처음 두바이트가 'FE FF' 일 경우 -> UCS-2BE or UTP-16BE(Big Endian)
					if (p != 0xff)
						throw std::exception();

					// check) 아직 지원하도록 작업하지 않음.
					CGASSERT_ERROR(false);

					//// - UTF16BE를 Source로 하는 Document를 생성한다.
					//rapidjson::GenericDocument<rapidjson::UTF16BE<char>> json_data;

					//// - Parsing한다.
					//auto&	bResult = json_data.ParseStream(is);

					//// check) Parsing에 실패했을 경우 Exception을 던진다.
					//THROW_IF(bResult.IsNull(), std::exception());

					//// - 읽어들인 JSON값들을 저장한다.
					//for (auto iter = json_data.MemberBegin(), iter_end = json_data.MemberEnd(); iter != iter_end; ++iter)
					//{
					//	add_json_member<char, rapidjson::UTF16BE<char>>(*this, iter);
					//}
				}
				break;

		// Case) UTF8
		case	0xef:
				{
					// - 첫번째 바이트를 끄집어 낸다.
					is.Take();

					// - 두번째 바이트를 읽는다.
					auto p = static_cast<uint8_t>(is.Take());

					// - 두번쩨 바이트를 확인한다.
					if (p != 0xbb )
						throw std::exception();

					// - 세번째 바이트를 확인한다.
					p = static_cast<uint8_t>(is.Take());

					if (p != 0xbf)
						throw std::exception();

					//rapidjson::UTF8<char>

					// - UTF8를 Source로 하는 Document를 생성한다.
					rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;

					// - Parsing한다.
					const auto&	result = json_data.ParseStream(is);

					// check) Parsing에 실패했을 경우 Exception을 던진다.
					THROW_IF(result.IsNull(), std::exception(), LOG_ERROR << "(err ) fail to read JSON file '" << json_error_string(result.GetParseError()) << "' [offset:" << result.GetErrorOffset() << "]");

					// - 읽어들인 JSON값들을 저장한다.
					for (typename rapidjson::GenericDocument<rapidjson::UTF8<char>>::ConstMemberIterator iter = json_data.MemberBegin(), iter_end = json_data.MemberEnd(); iter != iter_end; ++iter)
					{
						add_json_member<char, rapidjson::UTF8<char>>(*this, iter);
					}
				}
				break;

		// Default) 기타...
		default:
				{
					// declare) 
					rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;

					// - UTF8를 Source로 하는 Document를 생성한다.
					const auto&	result = json_data.ParseStream(is);

					// check) Parsing에 실패했을 경우 Exception을 던진다.
					THROW_IF(result.IsNull(), std::exception(), LOG_ERROR << "(err ) fail to read JSON file '" << json_error_string(result.GetParseError()) << "' [offset:" << result.GetErrorOffset() << "]");

					// - 읽어들인 JSON값들을 저장한다.
					for (typename rapidjson::GenericDocument<rapidjson::UTF8<char>>::ConstMemberIterator iter = json_data.MemberBegin(), iter_end = json_data.MemberEnd(); iter != iter_end; ++iter)
					{
						add_json_member<char, rapidjson::UTF8<char>>(*this, iter);
					}
				}
				break;
		};

	}
	catch(...)
	{
		bReturn	 = false;
	}

	// 6) file을 닫는다.

#if 0
	api::file::close(fp);
#else
	std::fclose(fp);
#endif

	// return) 
	return	bReturn;
}

bool context::write_to_json_file(const std::string_view _filename, eFILE_LOCATION /*_file_location*/)
{
	// check)
	RETURN_IF(_filename.empty(), false);

	// 1) open file
	auto fp = api::file::open(_filename, eFILE_OPERATION::WRITE);

	// check)
	RETURN_IF(fp == INVALID_FILE_HANDLE, false);

	try
	{
		// 2) 버퍼를 할당한다.
		auto writeBuffer = write_to_json_string<char>(true);

#if defined(_WIN32)
		//writeBuffer = reinterpret_cast<char*>(MALLOC(65536 CGNEW_DEBUG_INFO_COMMA CGNEW_DEBUG_INFO_PARAMETERS_MALLOC));
#else
		//writeBuffer = reinterpret_cast<char*>(MALLOC(65536));
#endif

		// 3) 스트링을 써넣는다.
		api::file::write(fp, writeBuffer.size(), writeBuffer.data());
	}
	catch (...)
	{
		// return) 실패를 리턴한다.
		return false;
	}

	// 4) 파일 닫기
	api::file::close(fp);

	// return) 
	return true;
}

void _write_json_string_char(const std::string_view _str_key, context& _context, shared_buffer& _buffer, int _indent, bool _output_key=true, bool _is_file=false)
{
	// check)
	RETURN_IF(_context.v.type == eCONTEXT_VALUE_TYPE::NULL_);

	// Case) File일 경우...
	if (_is_file)
	{
		// declare) 
		const char* str_indent = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";

		// - ...
		_buffer.append_text<char>(str_indent + 32 - _indent);
	}

	// 1) Key를 출력한다.
	if (_output_key == true)
	{
		// - """를 추가.
		_buffer.append<char>('\"');

		// - Key를 저장한다.
		_buffer.append_text<char>(_str_key);

		// - "":"를 추가.
		_buffer.append<char>('\"');
		_buffer.append<char>(':');
		_buffer.append<char>(' ');
	}

	// 2) Value값을 출력한다.
	switch(_context.v.type)
	{
	case	eCONTEXT_VALUE_TYPE::STRING_:
			{
				// - """를 써넣는다.
				_buffer.append<char>('\"');

				// - Value값을 써넣는다.
				_buffer.append_text<char>(_context.v.value_string->c_str());

				// - """를 써넣는다.
				_buffer.append<char>('\"');
			}
			break;

	case	eCONTEXT_VALUE_TYPE::BOOL_:
			{
				// - Value값을 써넣는다.
				_buffer.append_text<char>(_context.v.value_uint64 != 0 ? "true" : "false");
			}
			break;

	case	eCONTEXT_VALUE_TYPE::NUMBER_:
	case	eCONTEXT_VALUE_TYPE::OBJECT_:
			{
				// - Value값을 써넣는다.
				_buffer.append_text<char>(_context.v.value_string->c_str());
			}
			break;

	case	eCONTEXT_VALUE_TYPE::ARRAY_:
			{
				// - "["를 추가.
				_buffer.append<char>('[');

				// Case) File일 경우...
				if (_is_file)
				{
					_buffer.append<char>('\r');
					_buffer.append<char>('\n');
				}

				// - 첫번째 추가...
				_write_json_string_char(_context.map_node.begin()->first, _context.map_node.begin()->second, _buffer, _indent + 1, false, _is_file);

				// - 알맹이 추가.
				for (auto iter = ++_context.map_node.begin(); iter != _context.map_node.end(); ++iter)
				{
					// check)
					CONTINUE_IF(iter->second.v.type == eCONTEXT_VALUE_TYPE::NULL_);

					// - ","를 추가.
					_buffer.append<char>(',');

					// Case) File일 경우...
					if (_is_file)
					{
						_buffer.append<char>('\r');
						_buffer.append<char>('\n');
					}

					// - 첫번째 추가...
					_write_json_string_char(iter->first, iter->second, _buffer, _indent + 1, false, _is_file);
				}

				// Case) File일 경우...
				if (_is_file)
				{
					_buffer.append<char>('\r');
					_buffer.append<char>('\n');
				}

				// - "]"를 추가.
				_buffer.append<char>(']');
			}
			break;

	case	eCONTEXT_VALUE_TYPE::NODE_:
			{
				// check)
				CGASSERT_ERROR(_context.v.value_string == nullptr);

				// - "{"를 추가.
				_buffer.append<char>('{');

				// Case) File일 경우...
				if (_is_file)
				{
					_buffer.append<char>('\r');
					_buffer.append<char>('\n');
				}

				if (_context.map_node.empty() == false)
				{
					// - 첫번째 추가...
					_write_json_string_char(_context.map_node.begin()->first, _context.map_node.begin()->second, _buffer, _indent + 1, true, _is_file);

					// - 알맹이 추가.
					for (auto iter = ++_context.map_node.begin(); iter != _context.map_node.end(); ++iter)
					{
						// check)
						CONTINUE_IF(iter->second.v.type == eCONTEXT_VALUE_TYPE::NULL_);

						// - ","를 추가.
						_buffer.append<char>(',');

						// Case) File일 경우...
						if (_is_file)
						{
							_buffer.append<char>('\r');
							_buffer.append<char>('\n');
						}

						// - 첫번째 추가...
						_write_json_string_char(iter->first, iter->second, _buffer, _indent + 1, true, _is_file);
					}

					// Case) File일 경우...
					if (_is_file)
					{
						_buffer.append<char>('\r');
						_buffer.append<char>('\n');
					}
				}

				// - "{}"를 추가.
				_buffer.append<char>('}');

				// Case) File일 경우...
				if (_is_file)
				{
					_buffer.append<char>('\r');
					_buffer.append<char>('\n');
				}
			}
			break;
					
	case	eCONTEXT_VALUE_TYPE::NULL_:
			CGASSERT_ERROR(false);
			break;

	case	eCONTEXT_VALUE_TYPE::CHRONO_DURATION_:
			_buffer.append_text<char>(to_string<char>(static_cast<chrono::time::duration>(_context)));
			break;

	case	eCONTEXT_VALUE_TYPE::CHRONO_TIME_POINT_:
			_buffer.append_text<char>(to_string<char>(static_cast<chrono::time::utc::time_point>(_context)));
			break;

	case	eCONTEXT_VALUE_TYPE::BUFFER_:
	case	eCONTEXT_VALUE_TYPE::PTR_:
			CGASSERT_ERROR(false);
			throw throwable::failure(eRESULT::INVALID_ARGUMENT);
			break;

	default:
			CGASSERT_ERROR(false);
			throw throwable::failure(eRESULT::INVALID_ARGUMENT);
			break;
	}
}

void _write_json_string_wchar(const std::string_view _str_key, context& _context, shared_buffer& _buffer, int _indent, bool _output_key = true, bool _is_file = false)
{
	// check)
	RETURN_IF(_context.v.type == eCONTEXT_VALUE_TYPE::NULL_);

	// Case) File일 경우...
	if (_is_file)
	{
		// declare) 
		const wchar_t* str_indent = L"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";

		_buffer.append_text<wchar_t>(str_indent + 32 - _indent);
	}

	// declare) 
	shared_buffer temp = alloc_shared_buffer(8192 * sizeof(wchar_t));

	// 1) Key를 출력한다.
	if (_output_key == true)
	{
		// - '"'를 추가.
		_buffer.append<wchar_t>('\"');

		// - Key를 저장한다.
		_buffer.append_text<wchar_t>(encode_string(temp.data<wchar_t>(), _str_key));

		// - ':'를 추가.
		_buffer.append<wchar_t>('\"');
		_buffer.append<wchar_t>(':');
		_buffer.append<wchar_t>(' ');
	}

	// 2) 
	switch (_context.v.type)
	{
	case	eCONTEXT_VALUE_TYPE::STRING_:
			{
				// - """를 써넣는다.
				_buffer.append<wchar_t>('\"');

				// - Value값을 써넣는다.
				_buffer.append_text<wchar_t>(encode_string(temp.data<wchar_t>(), _context.v.value_string->c_str()));

				// - """를 써넣는다.
				_buffer.append<wchar_t>('\"');
			}
			break;

	case	eCONTEXT_VALUE_TYPE::BOOL_:
			{
				// - Value값을 써넣는다.
				_buffer.append_text<wchar_t>(_context.v.value_uint64 != 0 ? L"true" : L"false");
			}
			break;

	case	eCONTEXT_VALUE_TYPE::NUMBER_:
	case	eCONTEXT_VALUE_TYPE::OBJECT_:
			{
				// - Value값을 써넣는다.
				_buffer.append_text<wchar_t>(encode_string(temp.data<wchar_t>(), _context.v.value_string->c_str()));
			}
			break;

	case	eCONTEXT_VALUE_TYPE::ARRAY_:
			{
				// - "["를 추가.
				_buffer.append<wchar_t>('[');

				// - 첫번째 추가...
				_write_json_string_wchar(_context.map_node.begin()->first, _context.map_node.begin()->second, _buffer, _indent + 1, false, _is_file);

				// - 알맹이 추가.
				for (auto iter = ++_context.map_node.begin(); iter != _context.map_node.end(); ++iter)
				{
					// check)
					CONTINUE_IF(iter->second.v.type == eCONTEXT_VALUE_TYPE::NULL_);

					// - ","를 추가.
					_buffer.append<wchar_t>(',');

					// Case) File일 경우...
					if (_is_file)
					{
						_buffer.append<wchar_t>('\r');
						_buffer.append<wchar_t>('\n');
					}

					// - 첫번째 추가...
					_write_json_string_wchar(iter->first, iter->second, _buffer, _indent + 1, false, _is_file);
				}

				// - "]"를 추가.
				_buffer.append<wchar_t>(']');
			}
			break;

	case	eCONTEXT_VALUE_TYPE::NODE_:
			{
				// check)
				CGASSERT_ERROR(_context.v.value_string == nullptr);

				// - "{"를 추가.
				_buffer.append<wchar_t>('{');

				// Case) File일 경우...
				if (_is_file)
				{
					_buffer.append<wchar_t>('\r');
					_buffer.append<wchar_t>('\n');
				}

				if (_context.map_node.empty() == false)
				{
					// - 첫번째 추가...
					_write_json_string_wchar(_context.map_node.begin()->first, _context.map_node.begin()->second, _buffer, _indent + 1, true, _is_file);

					// - 알맹이 추가.
					for (auto iter = ++_context.map_node.begin(); iter != _context.map_node.end(); ++iter)
					{
						// check)
						CONTINUE_IF(iter->second.v.type == eCONTEXT_VALUE_TYPE::NULL_);

						// - ","를 추가.
						_buffer.append<wchar_t>(',');

						// Case) File일 경우...
						if (_is_file)
						{
							_buffer.append<wchar_t>('\r');
							_buffer.append<wchar_t>('\n');
						}
						// - 첫번째 추가...
						_write_json_string_wchar(iter->first, iter->second, _buffer, _indent + 1, true, _is_file);
					}

					// Case) File일 경우...
					if (_is_file)
					{
						_buffer.append<wchar_t>('\r');
						_buffer.append<wchar_t>('\n');
					}
				}

				// - "}"를 추가.
				_buffer.append<wchar_t>('}');

				// Case) File일 경우...
				if (_is_file)
				{
					_buffer.append<wchar_t>('\r');
					_buffer.append<wchar_t>('\n');
				}
			}
			break;


	case	eCONTEXT_VALUE_TYPE::NULL_:
			CGASSERT_ERROR(false);
			break;

	case	eCONTEXT_VALUE_TYPE::CHRONO_DURATION_:
			_buffer.append_text<wchar_t>(to_string<wchar_t>(static_cast<chrono::time::duration>(_context)));
			break;

	case	eCONTEXT_VALUE_TYPE::CHRONO_TIME_POINT_:
			_buffer.append_text<wchar_t>(to_string<wchar_t>(static_cast<chrono::time::utc::time_point>(_context)));
			break;

	case	eCONTEXT_VALUE_TYPE::BUFFER_:
	case	eCONTEXT_VALUE_TYPE::PTR_:
			CGASSERT_ERROR(false);
			throw throwable::failure(eRESULT::INVALID_ARGUMENT);
			break;

	default:
			CGASSERT_ERROR(false);
			throw throwable::failure(eRESULT::INVALID_ARGUMENT);
			break;
	}
}

void _write_json_string_u16char(const std::string_view _str_key, context& _context, shared_buffer& _buffer, int _indent, bool _output_key = true, bool _is_file = false)
{
	// check)
	RETURN_IF(_context.v.type == eCONTEXT_VALUE_TYPE::NULL_);

	// Case) File일 경우...
	if (_is_file)
	{
		// declare) 
		const char16_t* str_indent = u"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";

		// - 
		_buffer.append_text<char16_t>(str_indent + 32 - _indent);
	}

	// declare) 
	shared_buffer temp = alloc_shared_buffer(8192 * sizeof(char16_t));

	// 1) Key를 출력한다.
	if (_output_key == true)
	{
		// - '"'를 추가.
		_buffer.append<char16_t>('\"');

		// - Key를 저장한다.
		_buffer.append_text<char16_t>(encode_string(temp.data<char16_t>(), _str_key));

		// - ':'를 추가.
		_buffer.append<char16_t>('\"');
		_buffer.append<char16_t>(':');
		_buffer.append<char16_t>(' ');
	}

	// 2) 
	switch(_context.v.type)
	{
	case	eCONTEXT_VALUE_TYPE::STRING_:
			{
				// check)
				CGASSERT_ERROR(_context.v.value_string != nullptr);

				// - """를 써넣는다.
				_buffer.append<char16_t>('\"');

				// - Value값을 써넣는다.
				_buffer.append_text<char16_t>(encode_string(temp.data<char16_t>(), _context.v.value_string->c_str()));

				// - """를 써넣는다.
				_buffer.append<char16_t>('\"');
			}
			break;

	case	eCONTEXT_VALUE_TYPE::BOOL_:
			{
				// - Value값을 써넣는다.
				_buffer.append_text<char16_t>(_context.v.value_uint64 != 0 ? u"true" : u"false");
			}
			break;

	case	eCONTEXT_VALUE_TYPE::NUMBER_:
	case	eCONTEXT_VALUE_TYPE::OBJECT_:
			{
				// check)
				CGASSERT_ERROR(_context.v.value_string != nullptr);

				// - Value값을 써넣는다.
				_buffer.append_text<char16_t>(encode_string(temp.data<char16_t>(), _context.v.value_string->c_str()));
			}
			break;

	case	eCONTEXT_VALUE_TYPE::ARRAY_:
			{
				// check)
				CGASSERT_ERROR(_context.v.value_string != nullptr);

				// - "["를 추가.
				_buffer.append<char16_t>('[');

				// - 첫번째 추가...
				_write_json_string_u16char(_context.map_node.begin()->first, _context.map_node.begin()->second, _buffer, _indent + 1, false, _is_file);

				// - 알맹이 추가.
				for (auto iter = ++_context.map_node.begin(); iter != _context.map_node.end(); ++iter)
				{
					// check)
					CONTINUE_IF(iter->second.v.type == eCONTEXT_VALUE_TYPE::NULL_);

					// - ","를 추가.
					_buffer.append<char16_t>(',');

					// Case) File일 경우...
					if (_is_file)
					{
						_buffer.append<char16_t>('\r');
						_buffer.append<char16_t>('\n');
					}

					// - 첫번째 추가...
					_write_json_string_u16char(iter->first, iter->second, _buffer, _indent + 1, false, _is_file);
				}

				// - "]"를 추가.
				_buffer.append<char16_t>(']');
			}
			break;

	case	eCONTEXT_VALUE_TYPE::NODE_:
			{
				// check)
				CGASSERT_ERROR(_context.v.value_string == nullptr);

				// - "{"를 추가.
				_buffer.append<char16_t>('{');

				// Case) File일 경우...
				if (_is_file)
				{
					_buffer.append<char16_t>('\r');
					_buffer.append<char16_t>('\n');
				}

				if (_context.map_node.empty() == false)
				{
					// - 첫번째 추가...
					_write_json_string_u16char(_context.map_node.begin()->first, _context.map_node.begin()->second, _buffer, _indent + 1, true, _is_file);

					// - 알맹이 추가.
					for (auto iter = ++_context.map_node.begin(); iter != _context.map_node.end(); ++iter)
					{
						// check)
						CONTINUE_IF(iter->second.v.type == eCONTEXT_VALUE_TYPE::NULL_);

						// - ","를 추가.
						_buffer.append<char16_t>(',');

						// Case) File일 경우...
						if (_is_file)
						{
							_buffer.append<char16_t>('\r');
							_buffer.append<char16_t>('\n');
						}
						// - 첫번째 추가...
						_write_json_string_u16char(iter->first, iter->second, _buffer, _indent + 1, true, _is_file);
					}

					// Case) File일 경우...
					if (_is_file)
					{
						_buffer.append<char16_t>('\r');
						_buffer.append<char16_t>('\n');
					}
				}

				// - "{}"를 추가.
				_buffer.append<char16_t>('}');

				// Case) File일 경우...
				if (_is_file)
				{
					_buffer.append<char16_t>('\r');
					_buffer.append<char16_t>('\n');
				}
			}
			break;

	case	eCONTEXT_VALUE_TYPE::NULL_:
			CGASSERT_ERROR(false);
			break;

	case	eCONTEXT_VALUE_TYPE::CHRONO_DURATION_:
			_buffer.append_text<char16_t>(to_string<char16_t>(static_cast<chrono::time::duration>(_context)));
			break;

	case	eCONTEXT_VALUE_TYPE::CHRONO_TIME_POINT_:
			_buffer.append_text<char16_t>(to_string<char16_t>(static_cast<chrono::time::time_point>(_context)));
			break;

	case	eCONTEXT_VALUE_TYPE::BUFFER_:
	case	eCONTEXT_VALUE_TYPE::PTR_:
			CGASSERT_ERROR(false);
			throw throwable::failure(eRESULT::INVALID_ARGUMENT);
			break;

	default:
			CGASSERT_ERROR(false);
			throw throwable::failure(eRESULT::INVALID_ARGUMENT);
			break;
	}
}

void _write_json_string_u32char(const std::string_view _str_key, context& _context, shared_buffer& _buffer, int _indent, bool _output_key = true, bool _is_file = false)
{
	// check)
	RETURN_IF(_context.v.type == eCONTEXT_VALUE_TYPE::NULL_);

	// Case) File일 경우...
	if (_is_file)
	{
		// declare) 
		const char32_t* str_indent = U"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";

		// - 
		_buffer.append_text<char32_t>(str_indent + 32 - _indent);
	}

	// declare) 
	shared_buffer temp = alloc_shared_buffer(8192 * sizeof(char32_t));

	// 1) Key를 출력한다.
	if(_output_key==true)
	{
		// - """를 추가.
		_buffer.append<char32_t>('\"');

		// - Key를 추가.
		_buffer.append_text<char32_t>(encode_string(temp.data<char32_t>(), _str_key));

		// - ":"를 추가.
		_buffer.append<char32_t>('\"');
		_buffer.append<char32_t>(':');
		_buffer.append<char32_t>(' ');
	}

	// 2) 
	switch(_context.v.type)
	{
	case	eCONTEXT_VALUE_TYPE::STRING_:
			{
				// check)
				CGASSERT_ERROR(_context.v.value_string != nullptr);

				// - """를 써넣는다.
				_buffer.append<char32_t>('\"');

				// - Value값을 써넣는다.
				_buffer.append_text<char32_t>(encode_string(temp.data<char32_t>(), _context.v.value_string->c_str()));

				// - """를 써넣는다.
				_buffer.append<char32_t>('\"');
			}
			break;

	case	eCONTEXT_VALUE_TYPE::BOOL_:
			{
				// - Value값을 써넣는다.
				_buffer.append_text<char32_t>(_context.v.value_uint64 != 0 ? U"true" : U"false");
			}
			break;

	case	eCONTEXT_VALUE_TYPE::NUMBER_:
	case	eCONTEXT_VALUE_TYPE::OBJECT_:
			{
				// check)
				CGASSERT_ERROR(_context.v.value_string != nullptr);

				// - Value값을 써넣는다.
				_buffer.append_text<char32_t>(encode_string(temp.data<char32_t>(), _context.v.value_string->c_str()));
			}
			break;

	case	eCONTEXT_VALUE_TYPE::ARRAY_:
			{
				// check)
				CGASSERT_ERROR(_context.v.value_string != nullptr);

				// - "["를 추가.
				_buffer.append<char32_t>('[');

				// Case) File일 경우...
				if (_is_file)
				{
					_buffer.append<char32_t>('\r');
					_buffer.append<char32_t>('\n');
				}

				// - 첫번째 추가...
				_write_json_string_u32char(_context.map_node.begin()->first, _context.map_node.begin()->second, _buffer, _indent + 1, false, _is_file);

				// - 알맹이 추가.
				for (auto iter = ++_context.map_node.begin(); iter != _context.map_node.end(); ++iter)
				{
					// check)
					CONTINUE_IF(iter->second.v.type == eCONTEXT_VALUE_TYPE::NULL_);

					// - ","를 추가.
					_buffer.append<char32_t>(',');

					// Case) File일 경우...
					if (_is_file)
					{
						_buffer.append<char32_t>('\r');
						_buffer.append<char32_t>('\n');
					}

					// - 첫번째 추가...
					_write_json_string_u32char(iter->first, iter->second, _buffer, _indent + 1, false, _is_file);
				}

				// Case) File일 경우...
				if (_is_file)
				{
					_buffer.append<char32_t>('\r');
					_buffer.append<char32_t>('\n');
				}

				// - "]"를 추가.
				_buffer.append<char32_t>(']');
			}
			break;

	case	eCONTEXT_VALUE_TYPE::NODE_:
			{
				// check)
				CGASSERT_ERROR(_context.v.value_string == nullptr);

				// - "{"를 추가.
				_buffer.append<char32_t>('{');

				// Case) File일 경우...
				if (_is_file)
				{
					_buffer.append<char32_t>('\r');
					_buffer.append<char32_t>('\n');
				}

				if (_context.map_node.empty() == false)
				{
					// check)
					CGASSERT_ERROR(_context.v.type == eCONTEXT_VALUE_TYPE::NODE_);

					// - 첫번째 추가...
					_write_json_string_u32char(_context.map_node.begin()->first, _context.map_node.begin()->second, _buffer, _indent + 1, true, _is_file);

					// - 알맹이 추가.
					for (auto iter = ++_context.map_node.begin(); iter != _context.map_node.end(); ++iter)
					{
						// check)
						CONTINUE_IF(iter->second.v.type == eCONTEXT_VALUE_TYPE::NULL_);

						// - ","를 추가.
						_buffer.append<char32_t>(',');

						// Case) File일 경우...
						if (_is_file)
						{
							_buffer.append<char32_t>('\r');
							_buffer.append<char32_t>('\n');
						}

						// - 첫번째 추가...
						_write_json_string_u32char(iter->first, iter->second, _buffer, _indent + 1, true, _is_file);
					}

					// Case) File일 경우...
					if (_is_file)
					{
						_buffer.append<char16_t>('\r');
						_buffer.append<char16_t>('\n');
					}
				}

				// - "{}"를 추가.
				_buffer.append<char32_t>('}');

				// Case) File일 경우...
				if (_is_file)
				{
					_buffer.append<char32_t>('\r');
					_buffer.append<char32_t>('\n');
				}
			}
			break;

	case	eCONTEXT_VALUE_TYPE::NULL_:
			CGASSERT_ERROR(false);
			break;

	case	eCONTEXT_VALUE_TYPE::CHRONO_DURATION_:
			_buffer.append_text<char32_t>(to_string<char32_t>(static_cast<chrono::time::duration>(_context)));
			break;

	case	eCONTEXT_VALUE_TYPE::CHRONO_TIME_POINT_:
			_buffer.append_text<char32_t>(to_string<char32_t>(static_cast<chrono::time::time_point>(_context)));
			break;

	case	eCONTEXT_VALUE_TYPE::BUFFER_:
	case	eCONTEXT_VALUE_TYPE::PTR_:
			CGASSERT_ERROR(false);
			throw throwable::failure(eRESULT::INVALID_ARGUMENT);
			break;

	default:
			CGASSERT_ERROR(false);
			throw throwable::failure(eRESULT::INVALID_ARGUMENT);
			break;
	}
}

shared_buffer context::write_to_json_string_char(bool _is_file)
{
	// 1) shared_buffer를 할당받는다.
	shared_buffer buf_temp = alloc_shared_buffer(65536*4);

	// 2) "{"를 추가.
	buf_temp.append<char>('{');

	// Case) File일 경우...
	if (_is_file)
	{
		buf_temp.append<char>('\r');
		buf_temp.append<char>('\n');
	}

	// 3) 값 저장.
	if(map_node.empty()==false)
	{
		// declare)
		int	indent = 0;

		// 3) 첫번째 추가...
		_write_json_string_char(map_node.begin()->first, map_node.begin()->second, buf_temp, indent + 1, true, _is_file);

		// 4) 알맹이 추가.
		for (auto iter = ++map_node.begin(); iter != map_node.end(); ++iter)
		{
			// check)
			CONTINUE_IF(iter->second.v.type == eCONTEXT_VALUE_TYPE::NULL_);

			// - ","를 추가.
			buf_temp.append<char>(',');

			// Case) File일 경우...
			if (_is_file)
			{
				buf_temp.append<char>('\r');
				buf_temp.append<char>('\n');
			}

			// - 첫번째 추가...
			_write_json_string_char(iter->first, iter->second, buf_temp, indent + 1, true, _is_file);
		}
	}

	// Case) File일 경우...
	if (_is_file)
	{
		buf_temp.append<char>('\r');
		buf_temp.append<char>('\n');
	}

	// 4) "}"를 추가.
	buf_temp.append<char>('}');

	// 5) NULL을 추가한다.
	if (_is_file == false)
	{
		buf_temp.append<char>(0);
	}
	else
	{
		buf_temp.append<char>('\r');
		buf_temp.append<char>('\n');
	}

	// return) 
	return	buf_temp;
}

shared_buffer context::write_to_json_string_wchar(bool _is_file)
{
	// 1) shared_buffer를 할당받는다.
	shared_buffer buf_temp = alloc_shared_buffer(65536 * 4);

	// 2) "{"를 추가.
	buf_temp.append<wchar_t>('{');

	// Case) File일 경우...
	if (_is_file)
	{
		buf_temp.append<wchar_t>('\r');
		buf_temp.append<wchar_t>('\n');
	}

	// 3) 값 저장.
	if(map_node.empty()==false)
	{
		// declare)
		int	indent = 0;

		// 3) 첫번째 추가...
		_write_json_string_wchar(map_node.begin()->first, map_node.begin()->second, buf_temp, indent + 1, true, _is_file);

		// 4) 알맹이 추가.
		for (auto iter = ++map_node.begin(); iter != map_node.end(); ++iter)
		{
			// check)
			CONTINUE_IF(iter->second.v.type == eCONTEXT_VALUE_TYPE::NULL_);

			// - ","를 추가.
			buf_temp.append<wchar_t>(',');

			// Case) File일 경우...
			if (_is_file)
			{
				buf_temp.append<wchar_t>('\r');
				buf_temp.append<wchar_t>('\n');
			}

			// - 첫번째 추가...
			_write_json_string_wchar(iter->first, iter->second, buf_temp, indent + 1, true, _is_file);
		}
	}

	// Case) File일 경우...
	if (_is_file)
	{
		buf_temp.append<wchar_t>('\r');
		buf_temp.append<wchar_t>('\n');
	}

	// 4) "}"를 추가.
	buf_temp.append<wchar_t>('}');

	// 5) 
	if (_is_file == false)
	{
		buf_temp.append<wchar_t>(0);
	}
	else
	{
		buf_temp.append<wchar_t>('\r');
		buf_temp.append<wchar_t>('\n');
	}

	// return) 
	return buf_temp;
}

shared_buffer context::write_to_json_string_u16char(bool _is_file)
{
	// 1) shared_buffer를 할당받는다.
	shared_buffer buf_temp = alloc_shared_buffer(65536 * 4);

	// 2) "{"를 추가.
	buf_temp.append<char16_t>('{');

	// Case) File일 경우...
	if (_is_file)
	{
		buf_temp.append<char16_t>('\r');
		buf_temp.append<char16_t>('\n');
	}

	// 3) 값 저장.
	if(map_node.empty()==false)
	{
		// declare)
		int	indent = 0;

		// 3) 첫번째 추가...
		_write_json_string_u16char(map_node.begin()->first, map_node.begin()->second, buf_temp, indent + 1, true, _is_file);

		// 4) 알맹이 추가.
		for (auto iter = ++map_node.begin(); iter != map_node.end(); ++iter)
		{
			// check)
			CONTINUE_IF(iter->second.v.type == eCONTEXT_VALUE_TYPE::NULL_);

			// - ","를 추가.
			buf_temp.append<char16_t>(',');

			// Case) File일 경우...
			if (_is_file)
			{
				buf_temp.append<char16_t>('\r');
				buf_temp.append<char16_t>('\n');
			}

			// - 첫번째 추가...
			_write_json_string_u16char(iter->first, iter->second, buf_temp, indent + 1, true, _is_file);
		}
	}

	// Case) File일 경우...
	if (_is_file)
	{
		buf_temp.append<char16_t>('\r');
		buf_temp.append<char16_t>('\n');
	}

	// 4) "}"를 추가.
	buf_temp.append<char16_t>('}');

	// 5) 
	if (_is_file == false)
	{
		buf_temp.append<char16_t>(0);
	}
	else
	{
		buf_temp.append<char16_t>('\r');
		buf_temp.append<char16_t>('\n');
	}

	// return) 
	return	buf_temp;
}

shared_buffer context::write_to_json_string_u32char(bool _is_file)
{
	// 1) shared_buffer를 할당받는다.
	shared_buffer buf_temp = alloc_shared_buffer(65536 * 4);

	// 2) "{"를 추가.
	buf_temp.append<char32_t>('{');

	// Case) File일 경우...
	if (_is_file)
	{
		buf_temp.append<char32_t>('\r');
		buf_temp.append<char32_t>('\n');
	}

	// 3) 값 저장.
	if (map_node.empty() == false)
	{
		// declare)
		int	indent = 0;

		// 3) 첫번째 추가...
		_write_json_string_u32char(map_node.begin()->first, map_node.begin()->second, buf_temp, indent + 1, true, _is_file);

		// 4) 알맹이 추가.
		for (auto iter = ++map_node.begin(); iter != map_node.end(); ++iter)
		{
			// check)
			CONTINUE_IF(iter->second.v.type == eCONTEXT_VALUE_TYPE::NULL_);

			// - ","를 추가.
			buf_temp.append<char32_t>(',');

			// Case) File일 경우...
			if (_is_file)
			{
				buf_temp.append<char32_t>('\r');
				buf_temp.append<char32_t>('\n');
			}

			// - 첫번째 추가...
			_write_json_string_u32char(iter->first, iter->second, buf_temp, indent + 1, true, _is_file);
		}
	}

	// Case) File일 경우...
	if (_is_file)
	{
		buf_temp.append<char32_t>('\r');
		buf_temp.append<char32_t>('\n');
	}

	// 4) "}"를 추가.
	buf_temp.append<char32_t>('}');

	// 5) 
	if (_is_file == false)
	{
		buf_temp.append<char32_t>(0);
	}
	else
	{
		buf_temp.append<char32_t>('\r');
		buf_temp.append<char32_t>('\n');
	}

	// return) 
	return buf_temp;
}


bool context::read_from_json_string(const std::string_view _data)
{
	// declare) 
	rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;

	// 1) Parse한다.
	const auto&	result = json_data.Parse<rapidjson::kParseStopWhenDoneFlag>(_data.data());

	// check) Parsing에 실패했으면 false를 리턴한다.
	if(result.IsNull())
	{
		return	false;
	}

	// 2) 값을 설정한다.
	for (typename rapidjson::GenericDocument<rapidjson::UTF8<char>>::ConstMemberIterator iter = json_data.MemberBegin(), iter_end = json_data.MemberEnd(); iter != iter_end; ++iter)
	{
		add_json_member<char, rapidjson::UTF8<char>>(*this, iter);
	}

	// return) 
	return true;
}

context& context::insert(const std::string_view _key, eCONTEXT_VALUE_TYPE _value_type)
{
	// 1) key를 lovwer case로 변환한다.
	char temp_key[256]{};
	__to_lower<char>(temp_key, _key.data());

	// 2) 추가한다.
	auto& node_new = map_node.emplace(temp_key, context()).first->second;

	// 3) delete old data
	node_new.v._free_value_object();

	// 4) set value
	node_new.v.value_string = nullptr;
	node_new.v.type = _value_type;
	node_new.pnode_parent = this;
	//node_new.map_node;
	 
	// 5) 실존 node로 설정한다.
	node_new._validate_child();

	// return) 
	return node_new;
}

context& context::insert_string(const std::string_view _key, const std::string_view _value)
{
	// 1) key를 lovwer case로 변환한다.
	char temp_key[256]{};
	__to_lower<char>(temp_key, _key.data());

	// 2) 추가한다.
	auto& node_new = map_node.emplace(temp_key, context()).first->second;

	// 3) delete old data
	node_new.v._free_value_object();

	// 4) set value
	node_new.v.value_string = new std::string(_value);
	node_new.v.type = eCONTEXT_VALUE_TYPE::STRING_;
	node_new.pnode_parent = this;
	//node_new.map_node;

	// 5) 실존 node로 설정한다.
	node_new._validate_child();

	// return) 
	return node_new;
}

context& context::insert_bool(const std::string_view _key, bool _value)
{
	// 1) key를 lovwer case로 변환한다.
	char temp_key[256]{};
	__to_lower<char>(temp_key, _key.data());

	// 2) 추가한다.
	auto& node_new = map_node.emplace(temp_key, context()).first->second;

	// 3) delete old data
	node_new.v._free_value_object();

	// 4) set value
	node_new.v.value_int64 = (_value) ? 1 : 0;
	node_new.v.type = eCONTEXT_VALUE_TYPE::BOOL_;
	node_new.pnode_parent = this;

	// 5) 실존 node로 설정한다.
	node_new._validate_child();

	// return) 
	return node_new;
}

context& context::insert_integer(const std::string_view _key, int64_t _value)
{
	// 1) key를 lovwer case로 변환한다.
	char temp_key[256]{};
	__to_lower<char>(temp_key, _key.data());

	// 2) 추가한다.
	auto& node_new = map_node.emplace(temp_key, context()).first->second;

	// 3) delete old data
	node_new.v._free_value_object();

	// 4) set value
	node_new.v.value_int64 = _value;
	node_new.v.type = eCONTEXT_VALUE_TYPE::NUMBER_;
	node_new.pnode_parent = this;

	// 5) 실존 node로 설정한다.
	node_new._validate_child();

	// return) 
	return node_new;
}

context& context::insert_double(const std::string_view _key, double _value)
{
	// 1) key를 lovwer case로 변환한다.
	char temp_key[256]{};
	__to_lower<char>(temp_key, _key.data());

	// 2) 추가한다.
	auto& node_new = map_node.emplace(temp_key, context()).first->second;

	// 3) delete old data
	node_new.v._free_value_object();

	// 4) set value
	node_new.v.value_double = _value;
	node_new.v.type = eCONTEXT_VALUE_TYPE::DOUBLE_;
	node_new.pnode_parent = this;

	// 5) 실존 node로 설정한다.
	node_new._validate_child();

	// return) 
	return node_new;
}

bool context::erase(const std::string_view _key) noexcept
{
	// check)
	RETURN_IF(_key.empty(), false);

	// return) 
	return map_node.erase(_key.data()) != 0;
}

bool context::has_key(const std::string_view _key) const noexcept
{
	// check)
	RETURN_IF(_key.empty(), false);

	// 1) 복사
	char temp_key[256]{};
	__to_lower<char>(temp_key, _key.data());

	// 2) 해당 key를 찾는다.
	auto iter_find = map_node.find(temp_key);

	// check) 
	RETURN_IF(iter_find == map_node.end(), false);

	// return) 
	return iter_find->second.is_valid();
}

context& context::find(const std::string_view _key)
{
	// 1) 복사
	char temp_key[256]{};
	__to_lower<char>(temp_key, _key.data());

	// 2) 찾는다.
	auto iter_find = map_node.find(temp_key);

	// 3) 없으면 신규 추가해서 리턴한다.
	if(iter_find == map_node.end())
	{
		auto result = map_node.emplace(temp_key, context());
		result.first->second.pnode_parent = this;
		iter_find = result.first;
	}

	return iter_find->second;
}

const context& context::find(const std::string_view _key) const
{
	// 1) to lower
	char temp_key[256]{};
	__to_lower<char>(temp_key, _key.data());

	// 2) find
	auto iter_find = map_node.find(temp_key);

	// check) 없으면 temporary empty context object를 리턴한다.
	if (iter_find == map_node.end())
	{
		static context empty_context;
		return empty_context;
	}

	// return) 
	return iter_find->second;
}

context& context::operator [] (int _index)
{
	// 1) to string
	char str_temp[256]{0};
	__to_string(str_temp, _index);

	// check) 
	if(v.type == eCONTEXT_VALUE_TYPE::NULL_)
		v.type = eCONTEXT_VALUE_TYPE::ARRAY_;
	else if(v.type != eCONTEXT_VALUE_TYPE::ARRAY_)
		throw throwable::failure(eRESULT::INVALID_ARGUMENT);

	// return) 
	return find(str_temp);
}

const context& context::operator [] (int _index) const
{
	// 1) _index를 문자열로 변환한다.
	char str_temp[256]{0};
	__to_string(str_temp, _index);

	// check) 
	THROW_IF(v.type == eCONTEXT_VALUE_TYPE::NULL_ || v.type == eCONTEXT_VALUE_TYPE::ARRAY_, throwable::failure(eRESULT::INVALID_ARGUMENT));

	// return) 
	return find(str_temp);
}

context& context::operator = (const std::string_view _rhs)
{
	// 1) prepare value object
	v._free_value_object();

	// 2) set value
	v.value_string = new std::string();
	(*v.value_string) = _rhs;
	v.type = eCONTEXT_VALUE_TYPE::STRING_;

	// 3) validate hierarchy
	_validate_child();

	// return)  
	return *this;
}

context& context::operator = (std::string&& _rhs)
{
	// 1) free exist value object
	v._free_value_object();

	// 2) set value
	v.value_string = new std::string();
	(*v.value_string) = std::move(_rhs);
	v.type = eCONTEXT_VALUE_TYPE::STRING_;

	// 3) validate child
	_validate_child();

	// return)  
	return *this;
}

context& context::operator =	(bool _rhs)
{
	// 1) free existing value object
	v._free_value_object();

	// 2) set value
	v.value_uint64 = (_rhs) ? 1 : 0;
	v.type = eCONTEXT_VALUE_TYPE::BOOL_;

	// 3) validate child
	_validate_child();

	// return)  
	return *this;
}

context& context::operator =	(char _rhs)
{
	// 1) free existing value object
	v._free_value_object();

	// 2) set value
	v.value_uint64 = static_cast<uint64_t>(_rhs);
	v.type = eCONTEXT_VALUE_TYPE::NUMBER_;

	// 3) validate child
	_validate_child();

	// return)  
	return *this;
}

context& context::operator =	(unsigned char _rhs)
{
	// 1) free existing value object
	v._free_value_object();

	// 2) set value
	v.value_uint64 = static_cast<int64_t>(_rhs);
	v.type = eCONTEXT_VALUE_TYPE::NUMBER_;

	// 3) validate child
	_validate_child();

	// return)  
	return *this;
}

context& context::operator =	(int16_t _rhs)
{
	// 1) free existing value object
	v._free_value_object();

	// 2) set value
	v.value_int64 = static_cast<int64_t>(_rhs);
	v.type = eCONTEXT_VALUE_TYPE::NUMBER_;

	// 3) validate child
	_validate_child();

	// return)  
	return *this;
}

context& context::operator =	(uint16_t _rhs)
{
	// 1) free existing value object
	v._free_value_object();

	// 2) set value
	v.value_uint64 = static_cast<int64_t>(_rhs);
	v.type = eCONTEXT_VALUE_TYPE::NUMBER_;

	// 3) validate child
	_validate_child();

	// return)  
	return *this;
}

context& context::operator =	(int32_t _rhs)
{
	// 1) free existing value object
	v._free_value_object();

	// 2) set value
	v.value_int64 = static_cast<int64_t>(_rhs);
	v.type = eCONTEXT_VALUE_TYPE::NUMBER_;

	// 3) validate child
	_validate_child();

	// return)  
	return *this;
}

context& context::operator =	(uint32_t _rhs)
{
	// 1) free existing value object
	v._free_value_object();

	// 2) set value
	v.value_uint64 = static_cast<int64_t>(_rhs);
	v.type = eCONTEXT_VALUE_TYPE::NUMBER_;

	// 3) validate child
	_validate_child();

	// return)  
	return *this;
}

context& context::operator =	(int64_t _rhs)
{
	// 1) free existing value object
	v._free_value_object();

	// 2) set value
	v.value_int64 = static_cast<int64_t>(_rhs);
	v.type = eCONTEXT_VALUE_TYPE::NUMBER_;

	// 3) validate child
	_validate_child();

	// return)  
	return *this;
}

context& context::operator =	(uint64_t _rhs)
{
	// 1) free existing value object
	v._free_value_object();

	// 2) set value
	v.value_uint64 = static_cast<int64_t>(_rhs);
	v.type = eCONTEXT_VALUE_TYPE::NUMBER_;

	// 3) validate child
	_validate_child();

	// return)  
	return *this;
}

context& context::operator =	(float _rhs)
{
	// 1) free existing value object
	v._free_value_object();

	// 2) set value
	v.value_double = _rhs;
	v.type = eCONTEXT_VALUE_TYPE::DOUBLE_;

	// 3) validate child
	_validate_child();

	// return)  
	return *this;
}

context& context::operator =	(double _rhs)
{
	// 1) free existing value object
	v._free_value_object();

	// 2) set value
	v.value_double = _rhs;
	v.type = eCONTEXT_VALUE_TYPE::DOUBLE_;

	// 3) validate child
	_validate_child();

	// return)  
	return *this;
}

context& context::operator =	(const buffer_view& _rhs) noexcept
{
	// 1) free existing value object
	v._free_value_object();

	// 2) set value
	v.value_buffer = _rhs;
	v.type = eCONTEXT_VALUE_TYPE::BUFFER_;
	
	// 3) validate child
	if (v.type != eCONTEXT_VALUE_TYPE::NULL_)
	{
		_validate_child();
	}

	// return)  
	return *this;
}

context& context::operator = (const context& _rhs)
{
	// 1) set
	v = _rhs.v;
	pnode_parent = _rhs.pnode_parent;
	map_node = _rhs.map_node;

	// 3) validate child
	if (v.type != eCONTEXT_VALUE_TYPE::NULL_)
	{
		_validate_child();
	}

	// return)  
	return	*this;
}
context& context::operator = (context&& _rhs) noexcept
{
	// 1) value move
	v = std::move(_rhs.v);
	pnode_parent = _rhs.pnode_parent;
	map_node = std::move(_rhs.map_node);

	// 3) validate child
	if (v.type != eCONTEXT_VALUE_TYPE::NULL_)
	{
		_validate_child();
	}

	// return)  
	return *this;
}

}
