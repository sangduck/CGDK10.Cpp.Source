#include "pch.h"

// - CGDK: enable rapidjson SSE4.2 SIMD on x86-64 (skip whitespace/scan strings with _mm_cmpestri)
#if (defined(_M_X64) || defined(__x86_64__)) && !defined(RAPIDJSON_SSE42) && !defined(RAPIDJSON_SSE2) && !defined(RAPIDJSON_NEON)
	#if defined(_MSC_VER) || defined(__SSE4_2__)
		#define RAPIDJSON_SSE42		// msvc compiles sse4.2 intrinsics without a flag; gcc needs -msse4.2 (__SSE4_2__)
	#else
		#define RAPIDJSON_SSE2		// sse2 is baseline on x86-64 (gcc enables it without a flag)
	#endif
#endif
#include <charconv>
#include <cmath>
#include <cstdio>

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
	#pragma warning(disable:33010)
	//#pragma diag_suppress 177 // suppress #177-D function  was declared but never referenced
	#include "rapidjson/document.h"
	#include "rapidjson/writer.h"
	#include "rapidjson/stringbuffer.h"
	#include "rapidjson/filereadstream.h"
	#include "rapidjson/filewritestream.h"
	#pragma warning(default:5054)
	#pragma warning(default:33010)
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

// list of environment variables (used by '@merge_keys_from_env' on non-Windows platforms)
#if !defined(_WIN32)
	extern "C" char** environ;
#endif

namespace CGDK
{

std::string_view json_error_string(rapidjson::ParseErrorCode _JSON_error_code);

//-----------------------------------------------------------------------------
// special keys of json object
//
//   members of an object are applied in order. a later key overwrites the
//   same key applied before it. objects are merged recursively (only the same
//   keys are overwritten). arrays and other values are replaced.
//
//   1) "@merge_keys_from_env": "ENV_NAME"
//      - the value of the environment variable is parsed as a json object and
//        applied at the position of "@merge_keys_from_env".
//          . keys before "@merge_keys_from_env" are overwritten by the environment variable.
//          . keys after "@merge_keys_from_env" overwrite the environment variable.
//      - if the environment variable is not set (or empty), nothing is applied.
//      - invalid json (or not an object) throws an exception.
//
//   2) "@add_keys_from_env": "ENV_NAME"
//      - the value of the environment variable is parsed as a json object and
//        its keys are ADDED at the position of "@add_keys_from_env".
//      - if a key already exists in the object (or appears twice in the json of
//        the environment variable), an exception is thrown. (nothing is overwritten)
//      - keys after "@add_keys_from_env" follow the normal rule. (overwrite)
//      - if the environment variable is not set (or empty), nothing is applied.
//      - invalid json (or not an object) throws an exception.
//
//   3) "@del_key": "key"  or  "@del_key": ["key1", "key2"]
//      - deletes the keys of the object at the position of "@del_key".
//      - it can be used in the json of the environment variable too.
//
//   * names of special keys and environment variables are case-insensitive.
//
//   example)
//      "socket_to_admin": {
//          "remote_endpoint": { "address": "192.168.0.83", "port": 10001 },
//          "reconnection": true,
//          "reconnection_interval": 5,
//          "@merge_keys_from_env": "SOCKET_TO_ADMIN"
//      }
//
//      SOCKET_TO_ADMIN={"remote_endpoint":{"address":"10.0.0.5"},"@del_key":"reconnection_interval"}
//
//      => "socket_to_admin": {
//             "remote_endpoint": { "address": "10.0.0.5", "port": 10001 },
//             "reconnection": true
//         }
//-----------------------------------------------------------------------------
namespace
{
	constexpr std::string_view	JSON_KEY_MERGE_KEYS_FROM_ENV	= "@merge_keys_from_env";
	constexpr std::string_view	JSON_KEY_ADD_KEYS_FROM_ENV		= "@add_keys_from_env";
	constexpr std::string_view	JSON_KEY_DEL_KEY				= "@del_key";
	constexpr int				FROM_ENV_MAX_DEPTH	= 8;
	thread_local int			g_from_env_depth	= 0;

	// the name of environment variable is case-insensitive.
	//  - Windows: case-insensitive by itself.
	//  - others : an exact match is used first. if there is none, a case-insensitive
	//             match is searched. (more than one match throws an exception)
	bool _get_environment_variable(const std::string& _name, std::string& _value)
	{
	#if defined(_WIN32)
		char* pbuffer = nullptr;
		std::size_t length = 0;

		if (_dupenv_s(&pbuffer, &length, _name.c_str()) != 0 || pbuffer == nullptr)
			return false;

		_value = pbuffer;
		free(pbuffer);
	#else
		// 1) exact match
		if (const char* pvalue = std::getenv(_name.c_str()); pvalue != nullptr)
		{
			_value = pvalue;
			return true;
		}

		// 2) case-insensitive match
		const std::string name_lower = __to_lower(_name);
		const char* pvalue_found = nullptr;
		std::string_view name_found;

		for (char** penv = ::environ; penv != nullptr && *penv != nullptr; ++penv)
		{
			const std::string_view env_entry(*penv);
			const auto pos_equal = env_entry.find('=');

			// check)
			if (pos_equal == std::string_view::npos || pos_equal != name_lower.size())
				continue;

			const auto env_name = env_entry.substr(0, pos_equal);

			// check)
			if (__to_lower(env_name) != name_lower)
				continue;

			// check) ambiguous
			THROW_IF(pvalue_found != nullptr, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "'@merge_keys_from_env': environment variable '" + _name + "' is ambiguous ('" + std::string(name_found) + "', '" + std::string(env_name) + "')"));

			pvalue_found = *penv + pos_equal + 1;
			name_found = env_name;
		}

		// check) not found
		if (pvalue_found == nullptr)
			return false;

		_value = pvalue_found;
	#endif
		return true;
	}

	std::string_view _trim_json_string(std::string_view _value) noexcept
	{
		const auto pos_begin = _value.find_first_not_of(" \t\r\n");

		if (pos_begin == std::string_view::npos)
			return {};

		const auto pos_end = _value.find_last_not_of(" \t\r\n");

		return _value.substr(pos_begin, pos_end - pos_begin + 1);
	}

	// prevent infinite recursion ('@merge_keys_from_env'/'@add_keys_from_env' in the json of environment variable)
	struct from_env_depth_guard
	{
		from_env_depth_guard(std::string_view _key)
		{
			THROW_IF(g_from_env_depth >= FROM_ENV_MAX_DEPTH, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "'" + std::string(_key) + "' is nested too deep"));
			++g_from_env_depth;
		}
		~from_env_depth_guard() noexcept
		{
			--g_from_env_depth;
		}
	};

	template<class TDOCUMENT>
	std::string_view _get_json_string(const rapidjson::GenericValue<TDOCUMENT>& _value) noexcept
	{
		return std::string_view(_value.GetString(), _value.GetStringLength());
	}

	// read the environment variable named by '@merge_keys_from_env'/'@add_keys_from_env' and parse it into '_json_env'
	// return) false if the environment variable is not set (or empty)
	template<class TDOCUMENT>
	bool _read_from_env(std::string_view _key, const rapidjson::GenericValue<TDOCUMENT>& _value_from_env, rapidjson::GenericDocument<TDOCUMENT>& _json_env, std::string& _env_name)
	{
		// declare)
		const std::string key(_key);

		// check)
		THROW_IF(_value_from_env.IsString() == false, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "'" + key + "' must be a string (name of environment variable)"));

		// 1) get the name of environment variable
		_env_name = _trim_json_string(_get_json_string(_value_from_env));

		// check)
		if (_env_name.empty())
			return false;

		// 2) read environment variable (not set or empty: nothing is applied)
		std::string env_value_raw;

		if (_get_environment_variable(_env_name, env_value_raw) == false)
			return false;

		const std::string env_value(_trim_json_string(env_value_raw));

		if (env_value.empty())
			return false;

		// 3) parse
		_json_env.Parse(env_value.c_str());

		// check)
		THROW_IF(_json_env.HasParseError(), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "'" + key + "': invalid json in environment variable '" + _env_name + "' (" + std::string(json_error_string(_json_env.GetParseError())) + ")"));
		THROW_IF(_json_env.IsObject() == false, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "'" + key + "': json in environment variable '" + _env_name + "' must be an object"));

		// return)
		return true;
	}

	// '@del_key': "key" or ["key1", "key2", ...]
	template<class TDOCUMENT>
	void _apply_json_del_key(context& _context, const rapidjson::GenericValue<TDOCUMENT>& _value)
	{
		// 1) "key"
		if (_value.IsString())
		{
			_context.erase(__to_lower(_get_json_string(_value)));
			return;
		}

		// check)
		THROW_IF(_value.IsArray() == false, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "'@del_key' must be a string or an array of strings"sv));

		// 2) ["key1", "key2", ...]
		for (auto iter = _value.Begin(), iter_end = _value.End(); iter != iter_end; ++iter)
		{
			// check)
			THROW_IF(iter->IsString() == false, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "'@del_key' must be a string or an array of strings"sv));

			_context.erase(__to_lower(_get_json_string(*iter)));
		}
	}
}

// _padd_only_env_name: not nullptr when the object is the json of '@add_keys_from_env'.
//                      (keys of the object are only added. existing key throws an exception)
template<class TTYPE, class TDOCUMENT>
void apply_json_object(context& _context, const rapidjson::GenericValue<TDOCUMENT>& _object, const std::string* _padd_only_env_name);

template<class TTYPE, class TDOCUMENT>
void set_json_value(context& _context, std::string_view _name, const rapidjson::GenericValue<TDOCUMENT>& _value)
{
	// 1) object + existing object: merge (only the same keys are overwritten)
	if (_value.IsObject())
	{
		auto iter_find = _context.map_nodes.find(__to_lower(_name));

		if (iter_find != _context.map_nodes.end() && iter_find->second.type() == context::value_type_t::node_)
		{
			apply_json_object<TTYPE, TDOCUMENT>(iter_find->second, _value, nullptr);
			return;
		}
	}

	// 2) remove old value (insert() does not remove the child nodes of old value)
	_context.erase(__to_lower(_name));

	// 3) set new value
	switch (_value.GetType())
	{
	case	rapidjson::Type::kNullType:      //!< null
			{
				_context.insert(_name, context::value_type_t::null_);
			}
			break;

	case	rapidjson::Type::kFalseType:     //!< false
			{
				_context.insert_bool(_name, false);
			}
			break;

	case	rapidjson::Type::kTrueType:      //!< true
			{
				_context.insert_bool(_name, true);
			}
			break;

	case	rapidjson::Type::kObjectType:    //!< object
			{
				auto& new_context = _context.insert(_name, context::value_type_t::node_);

				apply_json_object<TTYPE, TDOCUMENT>(new_context, _value, nullptr);
			}
			break;

	case	rapidjson::Type::kArrayType:     //!< array (replaced, not merged)
			{
				auto& new_context = _context.insert(_name, context::value_type_t::node_);
				int index = 0;

				for (auto iter = _value.Begin(), iter_end = _value.End(); iter != iter_end; ++iter, ++index)
				{
					TTYPE str_index[32]{0};
					*std::to_chars(str_index, str_index + (sizeof(str_index) / sizeof(str_index[0])) - 1, static_cast<int64_t>(index)).ptr = 0;

					set_json_value<TTYPE, TDOCUMENT>(new_context, str_index, *iter);
				}
			}
			break;

	case	rapidjson::Type::kStringType:    //!< string
			{
				_context.insert_string(_name, _get_json_string(_value));
			}
			break;

	case	rapidjson::Type::kNumberType:     //!< number
			{
				if (_value.IsInt64())
					_context.insert_integer(_name, _value.GetInt64());
				else if (_value.IsUint64())
					_context.insert_integer(_name, static_cast<int64_t>(_value.GetUint64()));	// - stored as int64 (bits are kept)
				else
					_context.insert_double(_name, _value.GetDouble());							// - 1.5, 1e10 ...
			}
			break;

	default:
			throw	std::exception();
	};
}

template<class TTYPE, class TDOCUMENT>
void apply_json_object(context& _context, const rapidjson::GenericValue<TDOCUMENT>& _object, const std::string* _padd_only_env_name)
{
	// - apply members in order
	for (auto iter = _object.MemberBegin(), iter_end = _object.MemberEnd(); iter != iter_end; ++iter)
	{
		const auto name = _get_json_string(iter->name);
		const auto name_lower = __to_lower(name);

		// 1) '@merge_keys_from_env': merge the json of environment variable at this position
		if (name_lower == JSON_KEY_MERGE_KEYS_FROM_ENV)
		{
			rapidjson::GenericDocument<TDOCUMENT> json_env;
			std::string env_name;

			if (_read_from_env<TDOCUMENT>(JSON_KEY_MERGE_KEYS_FROM_ENV, iter->value, json_env, env_name))
			{
				from_env_depth_guard guard(JSON_KEY_MERGE_KEYS_FROM_ENV);

				apply_json_object<TTYPE, TDOCUMENT>(_context, json_env, nullptr);
			}
		}
		// 2) '@add_keys_from_env': add keys of the json of environment variable at this position
		else if (name_lower == JSON_KEY_ADD_KEYS_FROM_ENV)
		{
			rapidjson::GenericDocument<TDOCUMENT> json_env;
			std::string env_name;

			if (_read_from_env<TDOCUMENT>(JSON_KEY_ADD_KEYS_FROM_ENV, iter->value, json_env, env_name))
			{
				from_env_depth_guard guard(JSON_KEY_ADD_KEYS_FROM_ENV);

				apply_json_object<TTYPE, TDOCUMENT>(_context, json_env, &env_name);
			}
		}
		// 3) '@del_key': delete keys
		else if (name_lower == JSON_KEY_DEL_KEY)
		{
			_apply_json_del_key<TDOCUMENT>(_context, iter->value);
		}
		// 4) value
		else
		{
			// check) '@add_keys_from_env' does not overwrite existing key
			THROW_IF(_padd_only_env_name != nullptr && _context.map_nodes.find(name_lower) != _context.map_nodes.end(),
				throwable::failure(eRESULT::ALREADY_EXIST, 0, "'@add_keys_from_env': key '" + std::string(name) + "' of environment variable '" + *_padd_only_env_name + "' already exists"));

			set_json_value<TTYPE, TDOCUMENT>(_context, name, iter->value);
		}
	}
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

//-----------------------------------------------------------------------------
// json writer
//  - strings/keys are escaped (", \, control characters)
//  - number: integer, double: shortest round-trip text, bool: true/false
//  - chrono duration/time_point: string
//  - null nodes are skipped. buffer/ptr values can't be written as json and are skipped.
//-----------------------------------------------------------------------------
static void _append_json_escaped(std::string& _out, std::string_view _str)
{
	_out.push_back('\"');

	// - characters which need no escape are appended at once (run)
	const char* pch_run = _str.data();
	const char* pch_now = _str.data();
	const char* pch_end = _str.data() + _str.size();

	for (; pch_now != pch_end; ++pch_now)
	{
		const auto ch = static_cast<unsigned char>(*pch_now);

		// check) no escape
		if (ch >= 0x20 && ch != '\"' && ch != '\\')
			continue;

		// 1) append run
		_out.append(pch_run, pch_now);
		pch_run = pch_now + 1;

		// 2) append escape
		switch (ch)
		{
		case	'\"':	_out.append("\\\"", 2); break;
		case	'\\':	_out.append("\\\\", 2); break;
		case	'\b':	_out.append("\\b", 2); break;
		case	'\f':	_out.append("\\f", 2); break;
		case	'\n':	_out.append("\\n", 2); break;
		case	'\r':	_out.append("\\r", 2); break;
		case	'\t':	_out.append("\\t", 2); break;
		default:
				{
					constexpr char hex[] = "0123456789abcdef";
					const char temp[6] = { '\\', 'u', '0', '0', hex[ch >> 4], hex[ch & 0x0f] };
					_out.append(temp, 6);
				}
				break;
		}
	}

	// 3) append the last run
	_out.append(pch_run, pch_end);

	_out.push_back('\"');
}

static bool _is_json_writable(const context& _context) noexcept
{
	switch (_context.v.type)
	{
	case	context::value_type_t::null_:
	case	context::value_type_t::buffer_:
	case	context::value_type_t::ptr_:
			return false;

	default:
			return true;
	}
}

static void _append_json_indent(std::string& _out, int _indent)
{
	_out.append(static_cast<std::size_t>(_indent), '\t');
}

static void _write_json_value(std::string& _out, const context& _context, int _indent, bool _is_indent)
{
	switch (_context.v.type)
	{
	case	context::value_type_t::string_:
			{
				// check)
				THROW_IF(_context.v.value_string == nullptr, throwable::failure(eRESULT::INVALID_VALUE))

				_append_json_escaped(_out, *_context.v.value_string);
			}
			break;

	case	context::value_type_t::bool_:
			_out.append(_context.v.value_int64 != 0 ? "true" : "false");
			break;

	case	context::value_type_t::number_:
			{
				char temp[32];
				auto result = std::to_chars(temp, temp + sizeof(temp), _context.v.value_int64);
				_out.append(temp, result.ptr);
			}
			break;

	case	context::value_type_t::double_:
			{
				// - json has no NaN/Infinity
				if (std::isfinite(_context.v.value_double) == false)
				{
					_out.append("null");
					break;
				}

				char temp[64];
				auto result = std::to_chars(temp, temp + sizeof(temp), _context.v.value_double);
				_out.append(temp, result.ptr);
			}
			break;

	case	context::value_type_t::chrono_duration_:
			_append_json_escaped(_out, to_string(static_cast<chrono::time::duration>(_context)));
			break;

	case	context::value_type_t::chrono_time_point_:
			_append_json_escaped(_out, to_string(static_cast<chrono::time::utc::time_point>(_context)));
			break;

	case	context::value_type_t::node_:
			{
				_out.push_back('{');

				bool is_first = true;
				for (const auto& iter : _context.map_nodes)
				{
					// check) null/buffer/ptr nodes are skipped
					if (_is_json_writable(iter.second) == false)
						continue;

					if (is_first == false)
						_out.push_back(',');
					is_first = false;

					if (_is_indent)
					{
						_out.append("\r\n");
						_append_json_indent(_out, _indent + 1);
					}

					_append_json_escaped(_out, iter.first);
					_out.append(_is_indent ? ": " : ":");
					_write_json_value(_out, iter.second, _indent + 1, _is_indent);
				}

				if (_is_indent && is_first == false)
				{
					_out.append("\r\n");
					_append_json_indent(_out, _indent);
				}

				_out.push_back('}');
			}
			break;

	default:
			// - null/buffer/ptr: not written (filtered by the caller)
			_out.append("null");
			break;
	}
}

shared_buffer context::to_json() const
{
	// declare) working buffer reused per thread (no reallocation on every call. released if it grows too big)
	thread_local std::string t_str_json;
	auto& str_json = t_str_json;
	str_json.clear();

	struct shrink_guard
	{
		std::string& str;
		~shrink_guard() { if (str.capacity() > 1024 * 1024) { std::string().swap(str); } }
	} guard{ str_json };

	// 1) root is always an object
	if (this->v.type == context::value_type_t::node_)
	{
		_write_json_value(str_json, *this, 0, false);
	}
	else
	{
		str_json = "{}";
	}

	// 2) copy to shared_buffer (exact size. no terminating NUL)
	auto buf_temp = alloc_shared_buffer(str_json.size());
	if (str_json.empty() == false)
		memcpy(buf_temp.data(), str_json.data(), str_json.size());
	buf_temp.set_size(str_json.size());

	// return)
	return buf_temp;
}

void context::from_json(const std::string_view _data)
{
	// declare)
	rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;

	// 1) parse (with length: _data is not null-terminated)
	json_data.Parse<rapidjson::kParseStopWhenDoneFlag>(_data.data(), _data.size());

	// check) throw exception if parsing failed
	THROW_IF(json_data.HasParseError(), throwable::failure(eRESULT::FAIL, 0, "fail to parse json string (" + std::string(json_error_string(json_data.GetParseError())) + " at " + std::to_string(json_data.GetErrorOffset()) + ")"));

	// check) root must be an object
	THROW_IF(json_data.IsObject() == false, throwable::failure(eRESULT::FAIL, 0, "root of json must be an object"sv));

	// 2) apply members in order ('@merge_keys_from_env', '@del_key' are processed)
	apply_json_object<char, rapidjson::UTF8<char>>(*this, json_data, nullptr);
}

}
