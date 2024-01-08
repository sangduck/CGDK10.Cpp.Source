#include "pch.h"

using namespace CGDK;

// FOREGROUND
#define CONSOLE_COLOR_RST					"\x1b[0m"sv			// reset
#define CONSOLE_FORE_COLOR_DEFAULT			"\x1b[39m"sv		// 
#define CONSOLE_FORE_COLOR_BLACK			"\x1b[30m"sv		//  0: black
#define CONSOLE_FORE_COLOR_RED				"\x1b[31m"sv		//  4:red
#define CONSOLE_FORE_COLOR_GREENN			"\x1b[32m"sv		//  2:green
#define CONSOLE_FORE_COLOR_YELLOW			"\x1b[33m"sv		//  6:yellow
#define CONSOLE_FORE_COLOR_BLUE				"\x1b[34m"sv		//  1: blue
#define CONSOLE_FORE_COLOR_MAGENTA			"\x1b[35m"sv		//  5:magenta
#define CONSOLE_FORE_COLOR_CYAN				"\x1b[36m"sv		//  3:cyan
#define CONSOLE_FORE_COLOR_LIGHT_GRAY		"\x1b[37m"sv		//  7:light gray
#define CONSOLE_FORE_COLOR_DARK_GRAY		"\x1b[90m"sv		//  8:dark gray
#define CONSOLE_FORE_COLOR_LIGHT_RED		"\x1b[91m"sv		// 12:light red
#define CONSOLE_FORE_COLOR_LIGHT_GREEN		"\x1b[92m"sv		// 10:light green
#define CONSOLE_FORE_COLOR_LIGHT_YELLOW		"\x1b[93m"sv		// 14:light yellow
#define CONSOLE_FORE_COLOR_LIGHT_BLUE		"\x1b[94m"sv		//  9:light blue
#define CONSOLE_FORE_COLOR_LIGHT_MAGENTA	"\x1b[95m"sv		// 13:light magenta
#define CONSOLE_FORE_COLOR_LIGHT_CYAN		"\x1b[96m"sv		// 11:light cyan
#define CONSOLE_FORE_COLOR_WHITE			"\x1b[97m"sv		// 15:white
#define CONSOLE_BOLD						"\x1b[1m"sv			// bold
#define CONSOLE_UNDL						"\x1b[4m"sv			// under line

// BACKGROUND
#define CONSOLE_BACK_COLOR_RED				"\x1b[41m"sv		// red
#define CONSOLE_BACK_COLOR_GREEN			"\x1b[42m"sv		// green
#define CONSOLE_BACK_COLOR_BLUE				"\x1b[44m"sv		// blue
#define CONSOLE_BACK_COLOR_DEFAULT			"\x1b[49m"sv		// default


logger_console::logger_console()
{
}

logger_console::logger_console(const std::string_view _name) :
	Nlogger(_name)
{
}

logger_console::~logger_console() noexcept
{
	this->destroy();
}

result_code logger_console::_process_initializing(context& _context)
{
	// trace) 
	LOG_PROGRESS << "@ <BEGIN> initializing LOGGER_CONSOLE["sv << name() << "]"sv;

	// 1) on_initializing 함수를 호출한다.
	this->on_initializing(_context);

	// declare) 
	eCHARACTER_SET char_encoding = DEFAULT_CONSOLE_CHARACTER_SET;

	// 2) character_set type을 읽어들인다.
	{
		// - 'Interval'을 얻는다.
		std::string str_value = _context.find("character_set", "character set", "characterset", "char_set", "char set", "charset");

		if (str_value.empty() == false)
		{
			// - 'mbcs'인지 확인한다.
			if (str_value == "mbcs" || str_value == "euckr")
			{
				// - char_encoding is 'eCHARACTER_SET::MBCS'
				char_encoding = eCHARACTER_SET::MBCS;

				// trace)
				LOG_INFO << "  + parameter['char_encoding']: 'euckr(mbcs)'"sv;
			}
			// - 'ucs2'인지 확인한다.
			else if (str_value == "windows_unicode" || str_value == "ucs2")
			{
				// - char_encoding is 'eCHARACTER_SET::WINDOWS_UNICODE'
				char_encoding = eCHARACTER_SET::UCS2;

				// trace)
				LOG_INFO << "  + parameter['character_set']: 'ucs2(windows_unicode)'"sv;
			}
			// - 'utf-8'인지 확인한다.
			else if (str_value ==  "utf8")
			{
				// - char_encoding is 'eCHARACTER_SET::UTF8'
				char_encoding = eCHARACTER_SET::UTF8;

				// trace)
				LOG_INFO << "  + parameter['char_encoding']: 'utf8'"sv;
			}
			// - 'default'로 설정한다.
			else if (str_value == "default")
			{
				// - char_encoding is 'default'
				char_encoding = DEFAULT_CONSOLE_CHARACTER_SET;

				// trace)
				LOG_INFO << "  + parameter['character_set']: 'default'"sv;
			}
		}
	}

	// check) MBCS, UCS2, UTF-8만 지원한다.
	THROW_IF(char_encoding != eCHARACTER_SET::MBCS && char_encoding != eCHARACTER_SET::UCS2 && char_encoding != eCHARACTER_SET::UTF8, throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 2) get console handle
#if defined(_WIN32)
	this->m_console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
#endif

	// 3) Log 정보를 reset한다.
	this->reset_log_count();

	// 4) set character encoding
	this->m_char_encoding = char_encoding;

	// trace) Event Manager를 생성한다.
	LOG_PROGRESS << "@ <END> initializing LOGGER_CONSOLE["sv << name() << "]"sv;

	// return)
	return eRESULT::SUCCESS;
}

result_code logger_console::_process_destroying() noexcept
{
	// 1) Default Tracer로 설정되어 있다면 제거한다.
	reset_default_logger(this);

	// 2) detach from parent logger
	this->detach_from_parent();

	try
	{
		// 2) on_destroying 함수를 호출한다.
		this->on_destroying();
	}
	catch(...)
	{
		// Log) 
		LOG_ERROR << "@ (excp) exception on 'on_destroying' function call ['"sv << name() << "']"sv;
	}

	// return)
	return eRESULT::SUCCESS;
}

std::string_view get_text_color(log_type_t _log_type, log_level_t _log_level)
{
	// declare)
	std::string_view text_color;

	// - select text color
	switch (_log_type)
	{
	case	log_type_t::info:
			text_color = (_log_level < log_level_t::higher()) ? CONSOLE_FORE_COLOR_LIGHT_GRAY : CONSOLE_FORE_COLOR_WHITE;
			break;

	case	log_type_t::progress:
			text_color = (_log_level < log_level_t::higher()) ? CONSOLE_FORE_COLOR_GREENN : CONSOLE_FORE_COLOR_LIGHT_GREEN;
			break;

	case	log_type_t::debug:
			text_color = CONSOLE_FORE_COLOR_DARK_GRAY;
			break;

	case	log_type_t::fatal:
			text_color = CONSOLE_FORE_COLOR_RED;
			break;

	case	log_type_t::error:
			text_color = (_log_level < log_level_t::higher()) ? CONSOLE_FORE_COLOR_RED : CONSOLE_FORE_COLOR_LIGHT_RED;
			break;

	case	log_type_t::warning:
			text_color = (_log_level < log_level_t::higher()) ? CONSOLE_FORE_COLOR_MAGENTA : CONSOLE_FORE_COLOR_LIGHT_MAGENTA;
			break;

	case	log_type_t::user:
			text_color = (_log_level < log_level_t::higher()) ? CONSOLE_FORE_COLOR_YELLOW : CONSOLE_FORE_COLOR_LIGHT_YELLOW;
			break;

	case	log_type_t::system:
			text_color = (_log_level < log_level_t::higher()) ? CONSOLE_FORE_COLOR_BLUE : CONSOLE_FORE_COLOR_LIGHT_BLUE;
			break;

	default:
			text_color = CONSOLE_FORE_COLOR_LIGHT_GRAY;
			break;
	}

	// return)
	return text_color;
}

void logger_console::process_log(const std::vector<object_ptr<LOG_RECORD>>& _vector_log_record)
{
	// check) _plog_record must be not null
	RETURN_IF(_vector_log_record.empty(),);

	// check) 
	RETURN_IF(this->get_object_state() < eOBJECT_STATE::STOPPED || this->get_object_state() > eOBJECT_STATE::RUNNING);

	// 1) calculate buffer size
	size_t size_buffer = 0;
	for (auto& iter: _vector_log_record)
	{
		// - add size
		size_buffer += iter->process_buffer_size(this->m_char_encoding) * 4 + 128;
		size_buffer += iter->get_line_count() * 128;
	}

	// check)
	CGASSERT_ERROR(size_buffer < 65536);

	// check)
	RETURN_IF(size_buffer > 65536, );

	// 2) alloc shared_buffer for log text
	auto buf_output = alloc_shared_buffer(size_buffer);

	// 3) output LOG_RECORD
	for (const auto& iter: _vector_log_record)
	{
		// check)
		CONTINUE_IF(iter.empty());

		// check) message must be not empty
		CONTINUE_IF(iter->buf_message.empty());

		// - filtering
		if (m_plog_filter.exist())
		{
			CONTINUE_IF(m_plog_filter->process_filtering(iter) == false);
		}

		// - log type
		auto log_type = iter->type & 0xffff;
		bool log_not_continue = (uint32_t)(iter->type & log_type_t::continue_) == 0;

		// - get text color
		auto text_color = get_text_color(log_type, iter->level);

		// statistics) log count
		this->process_count_log(log_type);

		// - get line count
		size_t pos = sizeof(COUNT_T);
		auto count_line = iter->get_line_count();

		// check)
		CGASSERT_ERROR(count_line != 0);

		// check) 
		CONTINUE_IF(count_line == 0);

		// - write first line
		{
			if (log_not_continue)
			{
				buf_output.append_text(CONSOLE_FORE_COLOR_DARK_GRAY);
				iter->process_append_text_date(buf_output, m_char_encoding);
			}
			else
			{
				//                     "[0000-00-00 00:00:00] "
				buf_output.append_text("                      "sv);
			}

			// - 메시지를 작성한다.
			buf_output.append_text(text_color);
			pos = iter->process_get_message(buf_output, pos, m_char_encoding);
			buf_output.append_text(CONSOLE_COLOR_RST);
			buf_output.append('\n');
		}

		// - write other lines
		for (--count_line; count_line != 0; --count_line)
		{
			// - append space
			buf_output.append_text(CONSOLE_FORE_COLOR_DARK_GRAY);
			//                     "[0000-00-00 00:00:00] "
			buf_output.append_text("                      "sv);

			// - get next message
			buf_output.append_text(text_color);
			pos = iter->process_get_message(buf_output, pos, m_char_encoding);
			buf_output.append_text(CONSOLE_COLOR_RST);
			buf_output.append('\n');
		}
	}

	// 4) 로그 메시지를 출력한다.
#if defined(_WIN32)
	DWORD number_written = 0;
	WriteConsoleA(m_console_handle, buf_output.data(), buf_output.size<DWORD>(), &number_written, nullptr);
#else
	fwrite(buf_output.data(), 1, buf_output.size<int>(), stdout);
#endif
}
