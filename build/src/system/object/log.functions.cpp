#include "pch.h"


std::string_view to_string(CGDK::log_type_t _log_type) noexcept
{
	// check) 
	if(_log_type < CGDK::log_type_t::info || _log_type >= CGDK::log_type_t::max_)
		return "";

	static std::array<std::string_view,8> str_type
	{
		"INFO",			// 0
		"PROGRESS",		// 1
		"DEBUG",		// 2
		"EXCEPTION",	// 3
		"ERROR",		// 4
		"WARNING",		// 5
		"USER",			// 6
		"SYSTEM"		// 7
	};

	return str_type.at(static_cast<size_t>(_log_type));
}

namespace CGDK
{

extern std::shared_mutex g_cs_default_logger;
extern object_ptr<Ilogger> g_pdefault_logger;

//-----------------------------------------------------------------------------
// default logger
//
//  - every thread caches the default logger with the generation
//    (no lock and no shared reference count on the cache hit)
//  - the generation is increased when the default logger is changed
//  - a cache keeps the old logger alive until the thread gets the default logger again
//-----------------------------------------------------------------------------
namespace
{
	std::atomic<uint64_t> g_generation_default_logger{ 1 };

	struct sDEFAULT_LOGGER_CACHE
	{
		uint64_t			generation = 0;
		object_ptr<Ilogger>	plogger;
		object_ptr<Ilogger>	plogger_prev;	// - a stream started before the update may still use it (a nested log)
	};
	thread_local sDEFAULT_LOGGER_CACHE t_default_logger;
}

Ilogger* _get_default_logger_cached() noexcept
{
	// 1) cache hit
	auto& cache = t_default_logger;
	const auto generation = g_generation_default_logger.load(std::memory_order_acquire);
	if (cache.generation == generation)
		return cache.plogger.get();

	// 2) update the cache
	//    - the released logger is destroyed out of the lock (its destruction may reset the default logger)
	auto prelease = std::move(cache.plogger_prev);
	{
		// lock)
		std::shared_lock<std::shared_mutex> cs(g_cs_default_logger);

		cache.plogger_prev = std::move(cache.plogger);
		cache.plogger = g_pdefault_logger;
		cache.generation = g_generation_default_logger.load(std::memory_order_relaxed);
	}

	// return)
	return cache.plogger.get();
}

object_ptr<Ilogger> get_default_logger()
{
	// return)
	return object_ptr<Ilogger>(_get_default_logger_cached());
}

object_ptr<Ilogger> set_default_logger(object_ptr<Ilogger>&& _plogger)
{
	// declare) 
	object_ptr<Ilogger>	plogger;

	SCOPED_UNIQUE_LOCK(g_cs_default_logger)
	{
		if (_plogger != nullptr && g_pdefault_logger != _plogger)
		{
			// - get old logger
			plogger = std::move(g_pdefault_logger);

			// - set logger
			g_pdefault_logger = std::move(_plogger);

			// - invalidate the caches
			g_generation_default_logger.fetch_add(1, std::memory_order_release);
		}
	}

	// return)
	return plogger;
}

object_ptr<Ilogger> reset_default_logger(Ilogger* _plogger)
{
	// declare) 
	object_ptr<Ilogger>	plogger;

	SCOPED_UNIQUE_LOCK(g_cs_default_logger)
	{
		// check) _plogger이 특정한 객체를 지칭하고 있다면 해당 객체와 동일 객체인지를 확인한다.
		//        동일 객체가 아닐 경우 그냥 리턴한다.
		if(_plogger != nullptr && _plogger != g_pdefault_logger)
			return plogger;

		// 1) 먼저 복사하고 Default Logger를 reset한다.
		plogger = std::move(g_pdefault_logger);

		// - invalidate the caches
		g_generation_default_logger.fetch_add(1, std::memory_order_release);
	}

	// return)
	return plogger;
}

#if defined(_WIN32)
void CGOUTPUT_DEBUG_STRING(LOG_RECORD* _plog_record)
{
	// check) no record (the end of the process)
	if (_plog_record == nullptr)
		return;

	// 1) 
	buffer_view buf_temp = _plog_record->buf_message;

	// check)
	if(buf_temp.size() < 8)
		return;

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
	OutputDebugStringA(std::string(_log_message).c_str());
	OutputDebugStringA("\n");
}

void CGOUTPUT_DEBUG_STRING(log_type_t /*_log_type*/, log_level_t /*_log_level*/, const std::wstring_view _log_message)
{
	OutputDebugStringW(std::wstring(_log_message).c_str());
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
	// check) no record (the end of the process)
	if (_plog_record == nullptr)
		return;

	// 1) get priority
	auto priority = get_log_android_priority(_plog_record->type);

	// 2) ...
	buffer_view buf_temp = _plog_record->buf_message;

	// check)
	if(buf_temp.size() < 8)
		return;

	// 3) get line count
	auto line_count = buf_temp.extract<uint32_t>();

	// 4) output message
	switch (_plog_record->char_encoding)
	{
	case	 eCHARACTER_SET::MBCS:
			for (; line_count > 0; --line_count)
			{
				// - extract message string
				auto str_message = buf_temp.extract<std::string>();

				// - output
				__android_log_print(priority, "CGDK", "%s", str_message.data());
			}
			break;

	case	eCHARACTER_SET::UCS2:
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
	__android_log_print(priority, "CGDK", "%.*s", static_cast<int>(_log_message.size()), _log_message.data());
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
	//if(buf_temp.size() < 8)
	//	return;

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
	if(_plog_record == nullptr)
		return;

	// 1) output trace Message
#if defined(_DEBUG) && !defined(NDEBUG)
	CGOUTPUT_DEBUG_STRING(_plog_record);
#endif

	// 2) get log target
	auto plogger = (_plogger != nullptr) ? object_ptr<Ilogger>(_plogger) : get_default_logger();

	// check)
	if(plogger.empty())
		return;

	try
	{
		// 2) trace log record
		plogger->trace(object_ptr<LOG_RECORD>(_plog_record));
	}
	catch (...)
	{
	}
}

void _log(Ilogger* _plogger, const std::vector<object_ptr<LOG_RECORD>>& _vector_plog_record) noexcept
{
	// check)
	if(_vector_plog_record.empty())
		return;

	// 1) output trace Message
#if defined(_DEBUG) && !defined(NDEBUG)
	for (auto& iter : _vector_plog_record)
	{
		// check) 
		CGASSERT_ERROR(iter.exist());

		// check) 
		if(iter.empty())
			continue;

		// - output debug string
		CGOUTPUT_DEBUG_STRING(iter);
	}
#endif

	// 2) get log target
	auto plogger = (_plogger != nullptr) ? object_ptr<Ilogger>(_plogger) : get_default_logger();

	// check)
	if(plogger.empty())
		return;

	// 3) trace log record
	for (auto& iter : _vector_plog_record)
		plogger->trace(object_ptr<LOG_RECORD>(iter));
}

void _log(Ilogger* _plogger, Ilog_source* plog_source, log_source_t _source, log_type_t _log_type, log_level_t _log_level, eCHARACTER_SET _character_set, const std::string_view _log_message) noexcept
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
		if(plogger.empty())
			return;

		// 3) allocate LOG_RECORD object
		auto plog_record = LOG_RECORD::_alloc();

		// check) no pool (the end of the process) : the log is dropped
		if (plog_record.empty())
			return;

		// 4) set log info
		plog_record->type = _log_type;
		plog_record->level = _log_level;
		plog_record->source = _source;
		plog_record->set_message(_log_message, _character_set);
		plog_record->time_occurred = chrono::time::utc::now();

		// 5) set log source
		plog_record->plog_source = plog_source;
		if(plog_source != nullptr)
		{ 
			plog_source->store_log({ plog_record });
		}

		// 6) trace log record
		plogger->trace(std::move(plog_record));
	}
	catch (...)
	{
	}
}

void _log(Ilogger* _plogger, Ilog_source* plog_source, log_source_t _source, log_type_t _log_type, log_level_t _log_level, eCHARACTER_SET _character_set, const std::wstring_view _log_message) noexcept
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
		if(plogger.empty())
			return;

		// 3) alloc LOG_RECORD object
		auto plog_record = LOG_RECORD::_alloc();

		// check) no pool (the end of the process) : the log is dropped
		if (plog_record.empty())
			return;

		// 4) set log info
		plog_record->type = _log_type;
		plog_record->level = _log_level;
		plog_record->source = _source;
		plog_record->set_message(_log_message, _character_set);
		plog_record->time_occurred = chrono::time::utc::now();

		// 5) set log source
		plog_record->plog_source = plog_source;
		if (plog_source != nullptr)
		{
			plog_source->store_log({ plog_record });
		}

		// 6) trace log record
		plogger->trace(std::move(plog_record));
	}
	catch (...)
	{
	}
}


}