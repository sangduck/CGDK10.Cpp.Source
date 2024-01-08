#include "pch.h"


template <>
std::string_view to_string<char>(CGDK::log_type_t _log_type) noexcept
{
	// check) 
	RETURN_IF(_log_type < CGDK::log_type_t::info || _log_type >= CGDK::log_type_t::max_, "");

	static std::array<std::string_view,8> str_type
	{
		"INFO",			// 0
		"PROGRESS",		// 1
		"DEBUG",		// 2
		"EXCEPTION",	// 3
		"ERROR",		// 4
		"WARNING"		// 5
		"USER",			// 6
		"SYSTEM"		// 7
	};

	return str_type.at(static_cast<size_t>(_log_type));
}

template <>
std::wstring_view to_string<wchar_t>(CGDK::log_type_t _log_type) noexcept
{
	// check) 
	RETURN_IF(_log_type < CGDK::log_type_t::info || _log_type >= CGDK::log_type_t::max_, L"");

	static std::array<std::wstring_view, 8> str_type
	{
		L"INFO",		// 0
		L"PROGRESS",	// 1
		L"DEBUG",		// 2
		L"EXCEPTION",	// 3
		L"ERROR",		// 4
		L"WARNING"		// 5
		L"USER",		// 6
		L"SYSTEM"		// 7
	};

	return str_type.at(static_cast<size_t>(_log_type));
}

namespace CGDK
{

extern std::shared_mutex g_cs_default_logger;
extern object_ptr<Ilogger> g_pdefault_logger;

object_ptr<Ilogger> get_default_logger()
{
	// locked)
	{
		// lock) 
		std::shared_lock<std::shared_mutex> cs(g_cs_default_logger);

		// return) 
		return g_pdefault_logger;
	}
}

object_ptr<Ilogger> set_default_logger(Ilogger* _plogger)
{
	// declare) 
	object_ptr<Ilogger>	plogger;

	scoped_unique_lock(g_cs_default_logger)
	{
		if (_plogger != nullptr && g_pdefault_logger != _plogger)
		{
			// - move
			plogger = std::move(g_pdefault_logger);

			// - set logger
			g_pdefault_logger = _plogger;
		}
	}

	// return)
	return plogger;
}

object_ptr<Ilogger> reset_default_logger(Ilogger* _plogger)
{
	// declare) 
	object_ptr<Ilogger>	plogger;

	scoped_unique_lock(g_cs_default_logger)
	{
		// check) _plogger이 특정한 객체를 지칭하고 있다면 해당 객체와 동일 객체인지를 확인한다.
		//        동일 객체가 아닐 경우 그냥 리턴한다.
		RETURN_IF(_plogger != nullptr && _plogger != g_pdefault_logger, plogger);

		// 1) 먼저 복사하고 Default Logger를 reset한다.
		plogger = std::move(g_pdefault_logger);
	}

	// return) 
	return plogger;
}

#if defined(_WIN32)
void CGOUTPUT_DEBUG_STRING(LOG_RECORD* _plog_record)
{
	// 1) 
	buffer_view buf_temp = _plog_record->buf_message;

	// check)
	RETURN_IF(buf_temp.size() < 8);

	// 2) get line count
	auto line_count = buf_temp.extract<uint32_t>();

	// 3) output message
	switch (_plog_record->char_encoding)
	{
	case	eCHARACTER_SET::MBCS:
			for (; line_count > 0; --line_count)
			{
				// - extract message string
				auto str_message = buf_temp.extract<std::string>();

				// - output
				OutputDebugStringA(str_message.data());
				OutputDebugStringA("\n");
			}
			break;

	case	eCHARACTER_SET::UCS2:
			for (; line_count > 0; --line_count)
			{
				// - extract message string
				auto str_message = buf_temp.extract<std::wstring>();

				// - output
				OutputDebugStringW(str_message.data());
				OutputDebugStringW(L"\n");
			}
			break;

	default:
			break;
	}
}

void CGOUTPUT_DEBUG_STRING(log_type_t /*_log_type*/, log_level_t /*_log_level*/, const std::string_view _log_message)
{
	OutputDebugStringA(_log_message.data());
	OutputDebugStringA("\n");
}

void CGOUTPUT_DEBUG_STRING(log_type_t /*_log_type*/, log_level_t /*_log_level*/, const std::wstring_view _log_message)
{
	OutputDebugStringW(_log_message.data());
	OutputDebugStringW(L"\n");
}

#elif defined(__ANDROID__)
#include <android/log.h>

android_LogPriority get_log_android_priority(log_type_t _log_type)
{
	// declare) 
	android_LogPriority	priority;

	// 1) get log type
	switch (_log_type)
	{
	case	log_type_t::info:
			priority = android_LogPriority::ANDROID_LOG_INFO;
			break;
	case	log_type_t::progress:
			priority = android_LogPriority::ANDROID_LOG_VERBOSE;
			break;
	case	log_type_t::debug:
			priority = android_LogPriority::ANDROID_LOG_DEBUG;
			break;
	case	log_type_t::fatal:
			priority = android_LogPriority::ANDROID_LOG_FATAL;
			break;
	case	log_type_t::error:
			priority = android_LogPriority::ANDROID_LOG_ERROR;
			break;
	case	log_type_t::user:
			priority = android_LogPriority::ANDROID_LOG_UNKNOWN;
			break;
	case	log_type_t::system:
			priority = android_LogPriority::ANDROID_LOG_INFO;
			break;
	case	log_type_t::continue_:
			priority = android_LogPriority::ANDROID_LOG_DEFAULT;
			break;
	case	log_type_t::undefined:
			priority = android_LogPriority::ANDROID_LOG_UNKNOWN;
			break;
	default:
			priority = android_LogPriority::ANDROID_LOG_UNKNOWN;
			break;
	}

	// return) 
	return priority;
}

void CGOUTPUT_DEBUG_STRING(LOG_RECORD* _plog_record)
{
	// 1) get priority
	auto priority = get_log_android_priority(_plog_record->type);

	// 2) ...
	buffer_view buf_temp = _plog_record->buf_message;

	// check)
	RETURN_IF(buf_temp.size() < 8);

	// 3) get line count
	auto line_count = buf_temp.extract<uint32_t>();

	// 4) output message
	switch (_plog_record->char_encoding)
	{
	case	 eCHARACTER_SET::MBCS;
			for (; line_count > 0; --line_count)
			{
				// - extract message string
				auto str_message = buf_temp.extract<std::string>();

				// - output
				__android_log_print(priority, "CGDK", "%s", str_message.data());
			}
			break;

	case	eCHARACTER_SET::UCS2;
			break;

	default:
			break;
	}
}

void CGOUTPUT_DEBUG_STRING(log_type_t _log_type, log_level_t /*_log_level*/, const std::string_view _log_message)
{
	// 1) get priority
	auto priority = get_log_android_priority(_log_type);

	// 2) output message
	__android_log_print(priority, "CGDK", "%s", _log_message.data());
}

void CGOUTPUT_DEBUG_STRING(log_type_t _log_type, log_level_t /*_log_level*/, const std::wstring_view /*_log_message*/)
{
	//// 1) get priority
	//auto priority = get_log_android_priority(_log_type);

	//// 2) output message
}

#elif defined(__APPLE__)
void CGOUTPUT_DEBUG_STRING(LOG_RECORD* /*_plog_record*/)
{
}
#elif defined(__linux__)
#include <syslog.h>

int get_syslog_priority(log_type_t _log_type)
{
	// declare) 
	int	priority;

	// 1) get log type
	switch (_log_type)
	{
	case	log_type_t::info:
			priority = LOG_INFO | LOG_LOCAL0; /* informational */
			break;

	case	log_type_t::progress:
			priority = LOG_NOTICE | LOG_LOCAL0;
			break;

	case	log_type_t::debug:
			priority = LOG_DEBUG | LOG_LOCAL0;
			break;

	case	log_type_t::fatal:
			priority = LOG_CRIT | LOG_LOCAL0; // LOG_CRIT - critical conditions
			break;

	case	log_type_t::error:
			priority = LOG_ERR | LOG_LOCAL0; /* error conditions */
			break;

	case	log_type_t::user:
			priority = LOG_INFO | LOG_LOCAL0;
			break;

	case	log_type_t::system:
			priority = LOG_NOTICE | LOG_LOCAL0;
			break;

	case	log_type_t::continue_:
			priority = LOG_DEBUG | LOG_LOCAL0;
			break;

	case	log_type_t::undefined:
			priority = LOG_DEBUG | LOG_LOCAL0;
			break;

	default:
			priority = LOG_DEBUG | LOG_LOCAL0;
			break;
	}

	// return) 
	return priority;
}

void CGOUTPUT_DEBUG_STRING(LOG_RECORD* /*_plog_record*/)
{
	//// 1) 
	//buffer_view buf_temp = _plog_record->buf_message;

	//// check)
	//RETURN_IF(buf_temp.size() < 8);

	//// 2) get line count
	//auto line_count = buf_temp.extract<uint32_t>();

	//// 3) output message
	//switch (_plog_record->char_encoding)
	//{
	//case	 eCHARACTER_SET::MBCS:
	//		for (; line_count > 0; --line_count)
	//		{
	//			// - extract message string
	//			auto str_message = buf_temp.extract<std::string>();

	//			// - output
	//			//syslog(get_syslog_priority(_plog_record->type), "%s", str_message.data());
	//			std::cerr << str_message.data() << std::endl;
	//		}
	//		break;

	//case	eCHARACTER_SET::UCS2:
	//		for (; line_count > 0; --line_count)
	//		{
	//			// - extract message string
	//			auto str_message = buf_temp.extract<std::wstring>();

	//			// - output
	//			////syslog(get_syslog_priority(_plog_record->type), "%S", str_message.data());
	//			std::wcerr << str_message.data() << std::endl;
	//		}
	//		break;

	//default:
	//		break;
	//}
}

void CGOUTPUT_DEBUG_STRING(log_type_t /*_log_type*/, log_level_t /*_log_level*/, const std::string_view /*_log_message*/)
{
	//syslog(get_syslog_priority(_log_type), "%s", _log_message.data());
	//std::cerr << _log_message.data() << std::endl;
}

void CGOUTPUT_DEBUG_STRING(log_type_t /*_log_type*/, log_level_t /*_log_level*/, const std::wstring_view /*_log_message*/)
{
	//syslog(get_syslog_priority(_log_type), "%S", _log_message.data());
	//std::wcerr << _log_message.data() << std::endl;
}

#else
	#error "CGDK Not supported platform"
#endif

void _log(Ilogger* _plogger, LOG_RECORD* _plog_record) noexcept
{
	// check)
	CGASSERT_ERROR(_plog_record != nullptr);

	// check)
	RETURN_IF(_plog_record == nullptr);

	// 1) output trace Message
#if defined(_DEBUG) && !defined(NDEBUG)
	CGOUTPUT_DEBUG_STRING(_plog_record);
#endif

	// 2) get log target
	auto plogger = (_plogger != nullptr) ? object_ptr<Ilogger>(_plogger) : get_default_logger();

	// check)
	RETURN_IF(plogger.empty());

	try
	{
		// 2) trace log record
		plogger->trace(_plog_record);
	}
	catch (...)
	{
	}
}

void _log(Ilogger* _plogger, const std::vector<object_ptr<LOG_RECORD>>& _vector_plog_record) noexcept
{
	// check)
	RETURN_IF(_vector_plog_record.empty());

	// 1) output trace Message
#if defined(_DEBUG) && !defined(NDEBUG)
	for (auto& iter : _vector_plog_record)
	{
		// check) 
		CGASSERT_ERROR(iter.exist());

		// check) 
		CONTINUE_IF(iter.empty());

		// - output debug string
		CGOUTPUT_DEBUG_STRING(iter);
	}
#endif

	// 2) get log target
	auto plogger = (_plogger != nullptr) ? object_ptr<Ilogger>(_plogger) : get_default_logger();

	// check)
	RETURN_IF(plogger.empty());

	// 3) trace log record
	plogger->trace(_vector_plog_record);
}

void _log(Ilogger* _plogger, log_type_t _log_type, log_level_t _log_level, eCHARACTER_SET _character_set, const std::string_view _log_message) noexcept
{
	try
	{
		// 1) output trace Message
	#if defined(_DEBUG) && !defined(NDEBUG)
		CGOUTPUT_DEBUG_STRING(_log_type, _log_level, _log_message);
	#endif

		// 2) get log target
		auto plogger = (_plogger != nullptr) ? object_ptr<Ilogger>(_plogger) : get_default_logger();

		// check)
		RETURN_IF(plogger.empty());

		// 3) allocate LOG_RECORD object
		auto plog_record = _make_object<LOG_RECORD>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL);

		// 4) set log info
		plog_record->type = _log_type;
		plog_record->level = _log_level;
		plog_record->set_message(_log_message, _character_set);
		plog_record->time_occure = chrono::time::utc::now();

		// 5) trace log record
		plogger->trace(std::move(plog_record));
	}
	catch (...)
	{
	}
}

void _log(Ilogger* _plogger, log_type_t _log_type, log_level_t _log_level, eCHARACTER_SET _character_set, const std::wstring_view _log_message) noexcept
{
	try
	{
		// 1) output trace Message
	#if defined(_DEBUG) && !defined(NDEBUG)
		CGOUTPUT_DEBUG_STRING(_log_type, _log_level, _log_message);
	#endif

		// 2) get log target
		auto plogger = (_plogger != nullptr) ? object_ptr<Ilogger>(_plogger) : get_default_logger();

		// check)
		RETURN_IF(plogger.empty());

		// 3) alloc LOG_RECORD object
		auto plog_record = _make_object<LOG_RECORD>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL);

		// 4) set log info
		plog_record->type = _log_type;
		plog_record->level = _log_level;
		plog_record->set_message(_log_message, _character_set);
		plog_record->time_occure = chrono::time::utc::now();

		// 5) trace log record
		plogger->trace(std::move(plog_record));
	}
	catch (...)
	{
	}
}


}