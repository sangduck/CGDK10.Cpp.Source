#include "pch.h"

#if defined(_WIN32)
	#pragma comment(lib, "advapi32.lib")
#elif defined(__linux__)
	#include <errno.h>

	// - <syslog.h>의 LOG_INFO/LOG_WARNING 등은 CGDK의 log 매크로와 이름이 같으므로 include하지 않고 필요한 것만 선언한다.
	extern "C" void openlog(const char* _ident, int _option, int _facility);
	extern "C" void syslog(int _priority, const char* _format, ...);
	extern "C" void closelog(void);

	namespace
	{
		constexpr int SYSLOG_OPTION_PID		= 0x01;		// LOG_PID
		constexpr int SYSLOG_OPTION_NDELAY	= 0x08;		// LOG_NDELAY
		constexpr int SYSLOG_FACILITY_DAEMON	= (3 << 3);	// LOG_DAEMON
		constexpr int SYSLOG_PRIORITY_CRIT	= 2;		// LOG_CRIT
		constexpr int SYSLOG_PRIORITY_ERR		= 3;		// LOG_ERR
		constexpr int SYSLOG_PRIORITY_WARNING	= 4;		// LOG_WARNING
		constexpr int SYSLOG_PRIORITY_INFO	= 6;		// LOG_INFO
		constexpr int SYSLOG_PRIORITY_DEBUG	= 7;		// LOG_DEBUG
	}
#endif


using namespace CGDK;

namespace
{
	// - 기본 source 이름 (실행 파일 이름. 확장자 제외)
	std::string _get_default_source()
	{
	#if defined(_WIN32)
		char path[MAX_PATH * 4]{};
		auto size = ::GetModuleFileNameA(nullptr, path, static_cast<DWORD>(std::size(path)));
		std::string name(path, size);
		auto pos = name.find_last_of("\\/");
		if (pos != std::string::npos)
			name = name.substr(pos + 1);
		auto pos_ext = name.find_last_of('.');
		if (pos_ext != std::string::npos)
			name.resize(pos_ext);
		return name;
	#elif defined(__linux__)
		return std::string(program_invocation_short_name);
	#else
		return std::string("cgdk");
	#endif
	}

	// - 기록할 log type인가? (기본: fatal/error/warning)
	bool _is_writable(log_type_t _log_type, bool _all_types) noexcept
	{
		return _all_types || _log_type == log_type_t::fatal || _log_type == log_type_t::error || _log_type == log_type_t::warning;
	}
}

logger_system::logger_system()
{
}

logger_system::~logger_system() noexcept
{
	this->destroy();
}

result_code logger_system::initialize(const INITIALIZE_PARAMETER& _parameter)
{
	// declare)
	context context_setting;

	// 1) parameter -> context
	if (_parameter.source.empty() == false)
		context_setting["source"sv] = std::string(_parameter.source);

	context_setting["all_types"sv] = _parameter.all_types;

	// 2) initialize
	return this->object::Ninitializable::initialize(context_setting);
}

result_code logger_system::_process_initializing(context& _context)
{
	// trace)
	LOG_PROGRESS << "@ <BEGIN> initializing LOGGER_SYSTEM["sv << name() << "]"sv;

	// 1) on_initializing 함수를 호출한다.
	this->on_initializing(_context);

	// 2) source (없으면 실행 파일 이름)
	{
		auto& context_source = _context.find("source"sv);
		this->m_source = context_source.is_valid() ? context_source.operator std::string() : std::string();

		if (this->m_source.empty())
			this->m_source = _get_default_source();

		// trace)
		LOG_INFO << "  + parameter['source']: '"sv << this->m_source << "'"sv;
	}

	// 3) all_types
	{
		auto& context_all_types = _context.find("all_types"sv);
		this->m_all_types = context_all_types.is_valid() ? (bool)context_all_types : false;

		// trace)
		LOG_INFO << "  + parameter['all_types']: "sv << (this->m_all_types ? "true"sv : "false"sv);
	}

	// 4) open
#if defined(_WIN32)
	this->m_handle_event_source = ::RegisterEventSourceA(nullptr, this->m_source.c_str());

	// check)
	if (this->m_handle_event_source == nullptr)
	{
		LOG_ERROR << "(err ) logger_system: fail to RegisterEventSource '"sv << this->m_source << "' ("sv << ::GetLastError() << ")"sv;
	}
#elif defined(__linux__)
	// - m_source는 closelog까지 유지된다. (openlog는 ident 포인터를 그대로 사용한다.)
	::openlog(this->m_source.c_str(), SYSLOG_OPTION_PID | SYSLOG_OPTION_NDELAY, SYSLOG_FACILITY_DAEMON);
#endif

	// 5) Log 정보를 reset한다.
	this->reset_log_count();

	// trace)
	LOG_PROGRESS << "@ <END> initializing LOGGER_SYSTEM["sv << name() << "]"sv;

	// return)
	return eRESULT::SUCCESS;
}

result_code logger_system::_process_destroying() noexcept
{
	// 1) Default Tracer로 설정되어 있다면 제거한다.
	reset_default_logger(this);

	// 2) detach from parent logger
	detach_from_parent();

	// 3) wait for the completion of the queued logs (written before the close)
	this->_process_wait_complete();

	try
	{
		// 4) on_destroying 함수를 호출한다.
		on_destroying();
	}
	catch (...)
	{
		// Log)
		LOG_ERROR << "@ (excp) exception on 'on_destroying' function call ['"sv << name() << "']"sv;
	}

	// 5) close
#if defined(_WIN32)
	if (this->m_handle_event_source != nullptr)
	{
		::DeregisterEventSource(this->m_handle_event_source);
		this->m_handle_event_source = nullptr;
	}
#elif defined(__linux__)
	::closelog();
#endif

	// return)
	return eRESULT::SUCCESS;
}

void logger_system::process_log_batch(std::vector<object_ptr<LOG_RECORD>>& _vector_plog_record)
{
	// - the filter (once for the batch)
	const auto pfilter = this->_get_filter();

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

		// check) 기록할 log type만
		if (_is_writable(log_type, this->m_all_types) == false)
			continue;

		// statistics) log count
		process_count_log(log_type);

		// - lines -> text (줄은 '\n'으로 잇는다.)
		buffer_view buf_temp = iter->buf_message;
		auto count_line = buf_temp.extract<COUNT_T>();

		// check)
		if (count_line == 0)
			continue;

		std::string text;
		for (; count_line != 0; --count_line)
		{
			auto str_line = buf_temp.extract<std::string_view>();

			if (text.empty() == false)
				text += '\n';

			text += str_line;
		}

	#if defined(_WIN32)
		// - UTF-8이 아니면 ANSI 코드 페이지로 보고 UTF-8로 바꾼다.
		if (iter->char_encoding != eCHARACTER_SET::UTF8)
		{
			auto length = ::MultiByteToWideChar(CP_ACP, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
			std::wstring wtext(static_cast<std::size_t>(length), L'\0');
			::MultiByteToWideChar(CP_ACP, 0, text.data(), static_cast<int>(text.size()), wtext.data(), length);

			auto size = ::WideCharToMultiByte(CP_UTF8, 0, wtext.data(), length, nullptr, 0, nullptr, nullptr);
			std::string utf8(static_cast<std::size_t>(size), '\0');
			::WideCharToMultiByte(CP_UTF8, 0, wtext.data(), length, utf8.data(), size, nullptr, nullptr);
			text = std::move(utf8);
		}
	#endif

		// - write
		this->process_write(log_type, text);
	}
}

void logger_system::process_write(log_type_t _log_type, std::string_view _text) noexcept
{
#if defined(_WIN32)
	// check)
	RETURN_IF(this->m_handle_event_source == nullptr);

	// 1) event type
	WORD event_type = EVENTLOG_INFORMATION_TYPE;
	if (_log_type == log_type_t::fatal || _log_type == log_type_t::error)
		event_type = EVENTLOG_ERROR_TYPE;
	else if (_log_type == log_type_t::warning)
		event_type = EVENTLOG_WARNING_TYPE;

	// 2) UTF-8 -> UTF-16
	auto length = ::MultiByteToWideChar(CP_UTF8, 0, _text.data(), static_cast<int>(_text.size()), nullptr, 0);
	std::wstring wtext(static_cast<std::size_t>(length), L'\0');
	::MultiByteToWideChar(CP_UTF8, 0, _text.data(), static_cast<int>(_text.size()), wtext.data(), length);

	// 3) report (event id 1: EventCreate.exe의 메시지 '%1')
	LPCWSTR strings[1] = { wtext.c_str() };
	::ReportEventW(this->m_handle_event_source, event_type, 0, 1, nullptr, 1, 0, strings, nullptr);

#elif defined(__linux__)
	// 1) priority
	int priority = SYSLOG_PRIORITY_INFO;
	if (_log_type == log_type_t::fatal)
		priority = SYSLOG_PRIORITY_CRIT;
	else if (_log_type == log_type_t::error)
		priority = SYSLOG_PRIORITY_ERR;
	else if (_log_type == log_type_t::warning)
		priority = SYSLOG_PRIORITY_WARNING;
	else if (_log_type == log_type_t::debug)
		priority = SYSLOG_PRIORITY_DEBUG;

	// 2) syslog (NULL로 끝나도록 std::string으로 만든다.)
	std::string text(_text);
	::syslog(priority, "%s", text.c_str());
#else
	(void)_log_type;
	(void)_text;
#endif
}

#if defined(_WIN32)
bool logger_system::register_event_source(std::string_view _source) noexcept
{
	// declare) HKLM\SYSTEM\CurrentControlSet\Services\EventLog\Application\<source>
	std::string key = "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\" + std::string(_source);

	// 1) create key
	HKEY hkey = nullptr;
	auto result = ::RegCreateKeyExA(HKEY_LOCAL_MACHINE, key.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, nullptr, &hkey, nullptr);

	// check)
	RETURN_IF(result != ERROR_SUCCESS, false);

	// 2) EventMessageFile (EventCreate.exe는 event id 1~1000의 메시지가 '%1'이다.)
	const char message_file[] = "%SystemRoot%\\System32\\EventCreate.exe";
	result = ::RegSetValueExA(hkey, "EventMessageFile", 0, REG_EXPAND_SZ, reinterpret_cast<const BYTE*>(message_file), static_cast<DWORD>(sizeof(message_file)));

	// 3) TypesSupported (error/warning/information)
	DWORD types_supported = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE;
	if (result == ERROR_SUCCESS)
		result = ::RegSetValueExA(hkey, "TypesSupported", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&types_supported), sizeof(types_supported));

	::RegCloseKey(hkey);

	// return)
	return result == ERROR_SUCCESS;
}

bool logger_system::unregister_event_source(std::string_view _source) noexcept
{
	// declare)
	std::string key = "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\" + std::string(_source);

	// 1) delete key
	auto result = ::RegDeleteKeyA(HKEY_LOCAL_MACHINE, key.c_str());

	// return)
	return result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND;
}
#endif
