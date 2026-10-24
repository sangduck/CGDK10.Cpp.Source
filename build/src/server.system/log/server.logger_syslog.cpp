#include "pch.h"

#if defined(__linux__)

#include <syslog.h>


using namespace CGDK;


logger_syslog::logger_syslog()
{
}

logger_syslog::~logger_syslog()
{
	this->destroy();
}

result_code logger_syslog::_process_initializing(context& _context)
{
	// trace) 
	LOG_PROGRESS << "@ <BEGIN> initializing LOGGER_SYSLOG["sv << name() << "]"sv;

	// 1) on_initializing 함수를 호출한다.
	this->on_initializing(_context);

	//// declare) 
	//eCHARACTER_SET char_encoding = DEFAULT_CONSOLE_CHARACTER_SET;

	//// 2) Character type을 읽어들인다.
	//{
	//	// - 'Interval'을 얻는다.
	//	std::string_view str_value = _context["char_encoding"sv];

	//	if (str_value.empty() == false)
	//	{
	//		// - 'windows_mbcs'인지 확인한다.
	//		if (str_value == "windows_mbcs")
	//		{
	//			// - char_encoding is 'eCHARACTER_SET::WINDOWS_MBCS'
	//			char_encoding = eCHARACTER_SET::WINDOWS_MBCS;

	//			// trace)
	//			LOG_INFO << "  + parameter['char_encoding']: 'windows_mbcs'"sv;
	//		}
	//		// - 'windows_unicode'인지 확인한다.
	//		else if (str_value == "windows_unicode")
	//		{
	//			// - char_encoding is 'eCHARACTER_SET::WINDOWS_UNICODE'
	//			char_encoding = eCHARACTER_SET::WINDOWS_UNICODE;

	//			// trace)
	//			LOG_INFO << "  + parameter['char_encoding']: 'windows_unicode'"sv;
	//		}
	//		// - 'utf-8'인지 확인한다.
	//		else if (str_value ==  "utf8")
	//		{
	//			// - char_encoding is 'eCHARACTER_SET::UTF8'
	//			char_encoding = eCHARACTER_SET::UTF8;

	//			// trace)
	//			LOG_INFO << "  + parameter['char_encoding']: 'utf8'"sv;
	//		}
	//		// - 'ucs-16'인지 확인한다.
	//		else if (str_value == "ucs16")
	//		{
	//			// - char_encoding is 'eCHARACTER_SET::UCS16'
	//			char_encoding = eCHARACTER_SET::UCS16;

	//			// trace)
	//			LOG_INFO << "  + parameter['char_encoding']: 'ucs16'"sv;
	//		}
	//		// - 'year_begin'로 설정한다.
	//		else if (str_value == "default")
	//		{
	//			// - char_encoding is 'default'
	//			char_encoding = DEFAULT_CONSOLE_CHARACTER_SET;

	//			// trace)
	//			LOG_INFO << "  + parameter['char_encoding']: 'default'"sv;
	//		}
	//	}
	//}

	//// check) WINDOWS_MBCS, WINDOWS_UNICODE, UTF-8만 지원한다.
	//THROW_IF(char_encoding != eCHARACTER_SET::WINDOWS_MBCS && char_encoding != eCHARACTER_SET::WINDOWS_UNICODE && char_encoding != eCHARACTER_SET::UTF8, throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 3) Log 정보를 reset한다.
	this->reset_log_count();

	// trace) Event Manager를 생성한다.
	LOG_PROGRESS << "@ <END> initializing LOGGER_SYSLOG["sv << name() << "]"sv;

	// return)
	return eRESULT::SUCCESS;
}

result_code logger_syslog::_process_destroying() noexcept
{
	// 1) Default Tracer로 설정되어 있다면 제거한다.
	reset_default_logger(this);

	// 2) detach from parent logger
	detach_from_parent();

	// 3) wait for the completion of the queued logs
	this->_process_wait_complete();

	try
	{
		// 4) on_destroying 함수를 호출한다.
		on_destroying();
	}
	catch(...)
	{
		// Log) 
		LOG_ERROR << "@ (excp) exception on 'on_destroying' function call ['"sv << name() << "']"sv;
	}

	// return)
	return eRESULT::SUCCESS;
}

void logger_syslog::process_log_batch(std::vector<object_ptr<LOG_RECORD>>& _vector_plog_record)
{
	// - the filter (once for the batch)
	const auto pfilter = this->_get_filter();

	// 1) output LOG_RECORDs
	for (auto& iter : _vector_plog_record)
	{
		// check)
		if (iter.empty())
			continue;

		// check) message must be not empty
		if (iter->buf_message.empty())
			continue;

		// - filtering
		if (pfilter != nullptr && pfilter->process_filtering(iter) == false)
			continue;

		// - log type
		auto log_type = iter->type & 0xffff;
		//bool log_not_continue = (uint32_t)(iter->type & log_type_t::CONTINUE_) == 0;

		// statistics) log count
		process_count_log(log_type);

		// - priority (log type에 맞춘다.)
		int priority = LOG_INFO;
		if (log_type == log_type_t::fatal)
			priority = LOG_CRIT;
		else if (log_type == log_type_t::error)
			priority = LOG_ERR;
		else if (log_type == log_type_t::warning)
			priority = LOG_WARNING;
		else if (log_type == log_type_t::debug)
			priority = LOG_DEBUG;

		// - get log count
		buffer_view buf_temp = iter->buf_message;

		// - get line
		auto count_line = buf_temp.extract<COUNT_T>();

		// check)
		CGASSERT_ERROR(count_line != 0);

		// check)
		if (count_line == 0)
			continue;

		//::openlog(const char* ident, int option, int facility);
		//::closelog(void);

		// - write first line
		{
			// - get string
			auto str_text = buf_temp.extract<std::string_view>();
			//pos = iter->get_log_message(buf_output, pos, m_char_encoding);

			// - execute 'syslog'
			::syslog(priority, "%s", std::string(str_text).c_str());
		}

		// - write other lines
		for (--count_line; count_line != 0; --count_line)
		{
			// - get string
			auto str_text = buf_temp.extract<std::string_view>();
			//pos = iter->get_log_message(buf_output, pos, m_char_encoding);

			// - execute 'syslog'
			::syslog(priority, "%s", std::string(str_text).c_str());
		}
	}
}

#endif
