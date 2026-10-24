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


static eCHARACTER_SET detect_console_character_set() noexcept
{
#if defined(_WIN32)
	const UINT cp = GetConsoleOutputCP();
	if (cp == CP_UTF8) return eCHARACTER_SET::UTF8;
	if (cp == 1200)    return eCHARACTER_SET::UCS2;   // UTF-16 LE
	return eCHARACTER_SET::MBCS;
#else
	for (const char* env_name : {"LC_ALL", "LC_CTYPE", "LANG"})
	{
		const char* val = getenv(env_name);
		if (val == nullptr || val[0] == '\0')
			continue;
		std::string_view sv(val);
		auto dot = sv.rfind('.');
		std::string_view codeset = (dot != std::string_view::npos) ? sv.substr(dot + 1) : sv;
		if (codeset == "UTF-8" || codeset == "utf-8" || codeset == "UTF8" || codeset == "utf8")
			return eCHARACTER_SET::UTF8;
		if (!codeset.empty())
			return eCHARACTER_SET::MBCS;
	}
	return eCHARACTER_SET::UTF8;  // modern Linux default
#endif
}

logger_console::logger_console()
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
	eCHARACTER_SET char_encoding = detect_console_character_set();

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
			else if (str_value == "utf8")
			{
				// - char_encoding is 'eCHARACTER_SET::UTF8'
				char_encoding = eCHARACTER_SET::UTF8;

				// trace)
				LOG_INFO << "  + parameter['char_encoding']: 'utf8'"sv;
			}
			// - 'auto' / 'detect' / 'default': 런타임 감지
			else if (str_value == "auto" || str_value == "detect" || str_value == "default")
			{
				char_encoding = detect_console_character_set();

				// trace)
				LOG_INFO << "  + parameter['character_set']: 'auto' -> detected: '"sv
				         << (char_encoding == eCHARACTER_SET::UTF8 ? "utf8"sv : char_encoding == eCHARACTER_SET::UCS2 ? "ucs2"sv : "mbcs"sv)
				         << "'"sv;
			}
		}
	}

	// - log detected default encoding (config 미지정 시)
	LOG_INFO << "  + character_set: '"sv
	         << (char_encoding == eCHARACTER_SET::UTF8 ? "utf8"sv : char_encoding == eCHARACTER_SET::UCS2 ? "ucs2"sv : "mbcs"sv)
	         << "'"sv;

	// check) MBCS, UCS2, UTF-8만 지원한다.
	THROW_IF(char_encoding != eCHARACTER_SET::MBCS && char_encoding != eCHARACTER_SET::UCS2 && char_encoding != eCHARACTER_SET::UTF8, throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 2) get console handle
#if defined(_WIN32)
	this->m_console_handle = GetStdHandle(STD_OUTPUT_HANDLE);

	// - a console (WriteConsole) or redirected to a pipe or a file (WriteFile : WriteConsole fails on them)
	{
		DWORD mode = 0;
		this->m_is_console = (this->m_console_handle != INVALID_HANDLE_VALUE && this->m_console_handle != nullptr && GetConsoleMode(this->m_console_handle, &mode) != FALSE);
	}
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
	auto pholder = reset_default_logger(this);

	// 2) detach from parent logger
	this->detach_from_parent();

	// - wait for the completion of the queued logs
	this->_process_wait_complete();

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

namespace
{
	// - size of the output buffer (the logs are appended until it is full, written when it is full or at the end of a batch)
	constexpr std::size_t LOG_CONSOLE_OUTPUT_BUFFER = 1024 * 1024;
}

void logger_console::process_log_batch(std::vector<object_ptr<LOG_RECORD>>& _vector_plog_record)
{
	// 1) append the text of every log
	for (auto& iter : _vector_plog_record)
		this->_process_append_log(iter);

	// 2) write at once
	this->_process_write_output();
}

void logger_console::_process_write_output()
{
	// declare)
	auto& buf_output = this->m_buf_output;

	// check)
	RETURN_IF(buf_output.size() == 0);

	// 1) write
#if defined(_WIN32)
	DWORD number_written = 0;
	if (this->m_is_console == false)
		WriteFile(m_console_handle, buf_output.data(), buf_output.size<DWORD>(), &number_written, nullptr);
	else if (this->m_char_encoding == eCHARACTER_SET::UCS2)
		WriteConsoleW(m_console_handle, buf_output.data(), buf_output.size<DWORD>() / sizeof(wchar_t), &number_written, nullptr);
	else
		WriteConsoleA(m_console_handle, buf_output.data(), buf_output.size<DWORD>(), &number_written, nullptr);
#else
	fwrite(buf_output.data(), 1, buf_output.size<int>(), stdout);
	fflush(stdout);
#endif

	// 2) reuse the buffer
	buf_output.set_size(0);
}

void logger_console::_process_append_log(const object_ptr<LOG_RECORD>& _plog_record)
{
	// definitions) 
	constexpr size_t MAX_LOG_RECORD_SIZE = 65536 * 8;

	// check) _plog_record must be not null
	if(_plog_record.empty())
		return;

	// check) 
	//   (the state is not checked : the queued logs are written while destroying)

	// declare)
	auto char_encoding = this->m_char_encoding;

	// 1) calculate buffer size
	size_t size_buffer = 0;
	if (auto& iter = _plog_record; true)
	{
		size_buffer += LOG_RECORD::get_buffer_size(iter->buf_message, iter->char_encoding, char_encoding) + 128;
		size_buffer += iter->line_count() * 128;
	}

	// check)
	CGASSERT_ERROR(size_buffer < MAX_LOG_RECORD_SIZE);

	// check)
	if(size_buffer > MAX_LOG_RECORD_SIZE)
		return;

	try
	{
		// 2) alloc shared_buffer for log text
		if (this->m_buf_output.get_remained_size() < size_buffer)
		{
			// - write the output first
			this->_process_write_output();

			// - a new buffer when it is the first or too small for this log (reused : the console is written synchronously)
			if (this->m_buf_output.get_remained_size() < size_buffer)
				this->m_buf_output = alloc_shared_buffer((std::max)(size_buffer, LOG_CONSOLE_OUTPUT_BUFFER));
		}
		auto& buf_output = this->m_buf_output;

		// 3) output LOG_RECORD
		if (auto& iter = _plog_record; true)
		{
			// check)
			if(iter.empty())
				return;

			// check) message must be not empty
			if(iter->buf_message.empty())
				return;

			try
			{
				// - filtering
				if (auto pfilter = this->_get_filter(); pfilter != nullptr)
					if(pfilter->process_filtering(iter) == false)
						return;

				// - log type
				auto log_type = iter->type & 0xffff;
				bool log_not_continue = (uint32_t)(iter->type & log_type_t::continue_) == 0;

				// - get text color
				auto text_color = get_text_color(log_type, iter->level);

				// statistics) log count
				this->process_count_log(log_type);

				// - get line count
				size_t pos = sizeof(COUNT_T);
				auto count_line = iter->line_count();

				// check)
				CGASSERT_ERROR(count_line != 0);

				// check) 
				if(count_line == 0)
					return;

				// - write first line
				{
					if (log_not_continue)
					{
						buf_output.append_text(CONSOLE_FORE_COLOR_DARK_GRAY);
						LOG_RECORD::append_text_date(buf_output, iter->time_occurred, char_encoding);
					}
					else
					{
						//                     "[0000-00-00 00:00:00] "
						buf_output.append_text("                      "sv);
					}

					// - 메시지를 작성한다.
					buf_output.append_text(text_color);
					pos = iter->get_log_message(buf_output, char_encoding, pos);
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
					pos = iter->get_log_message(buf_output, char_encoding, pos);
					buf_output.append_text(CONSOLE_COLOR_RST);
					buf_output.append('\n');
				}
			}
			catch (...)
			{
				// Log) 
				LOG_ERROR << "@ (excp) fail to writing log to buffer, logger: '"sv << this->name() << "'"sv;
			}
		}

	}
	catch (std::exception& _e)
	{
		// log) 
		LOG_ERROR << "@ (excp) fail to writing log to console("sv << _e.what() << "), logger: '"sv << this->name() << "'"sv;
	}
	catch (...)
	{
		// log) 
		LOG_ERROR << "@ (excp) fail to writing by unknowned exception on writing log to console, logger: '"sv << this->name() << "'"sv;
	}
}
