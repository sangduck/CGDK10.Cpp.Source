#include "pch.h"
#include "cgdk/sdk10/server.log.slack.h"
#include <charconv>
#include <condition_variable>
#include <cstdlib>
#include <deque>

using namespace CGDK;


//-----------------------------------------------------------------------------
// helpers
//-----------------------------------------------------------------------------
namespace
{
	constexpr std::string_view SLACK_API_HOST = "slack.com";
	constexpr std::string_view SLACK_API_PATH = "/api/chat.postMessage";

	// - the max segments (attachments) of a message
	constexpr size_t MAX_ATTACHMENTS = 20;

	// - sgr style
	constexpr uint32_t STYLE_BOLD = 1;
	constexpr uint32_t STYLE_ITALIC = 2;
	constexpr uint32_t STYLE_STRIKE = 4;

	// - environment variable (the secrets are not written in the setting file)
	std::string _get_environment(std::string_view _name)
	{
		// check)
		if (_name.empty())
			return std::string();

		const std::string name(_name);
	#if defined(_WIN32)
		char* pvalue = nullptr;
		size_t length = 0;
		if (_dupenv_s(&pvalue, &length, name.c_str()) != 0 || pvalue == nullptr)
			return std::string();
		std::string result(pvalue);
		free(pvalue);
		return result;
	#else
		const char* pvalue = std::getenv(name.c_str());
		return (pvalue != nullptr) ? std::string(pvalue) : std::string();
	#endif
	}

	// - a value of the setting : "key" or "key_env" (the name of an environment variable)
	std::string _get_setting(context& _context, std::string_view _key)
	{
		if (auto& context_value = _context.find(_key); context_value.is_valid())
		{
			std::string value = context_value;
			if (value.empty() == false)
				return value;
		}

		const std::string key_env = std::string(_key) + "_env";
		if (auto& context_env = _context.find(std::string_view(key_env)); context_env.is_valid())
		{
			std::string name = context_env;
			return _get_environment(name);
		}

		return std::string();
	}

	// - a bool of the setting ("true"/"false", "1"/"0", "yes"/"no")
	bool _get_bool(context& _context, std::string_view _key, bool _default)
	{
		auto& context_value = _context.find(_key);
		if (context_value.is_valid() == false)
			return _default;

		std::string text = context_value;
		auto value = __to_lower(std::string_view(text));
		if (value == "true" || value == "1" || value == "yes")
			return true;
		if (value == "false" || value == "0" || value == "no")
			return false;
		return _default;
	}

	// - a number of the setting
	template <class T>
	bool _get_number(context& _context, std::string_view _key, T& _value)
	{
		auto& context_value = _context.find(_key);
		if (context_value.is_valid() == false)
			return false;

		std::string text = context_value;
		T value{};
		const auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
		if (ec != std::errc())
			return false;

		_value = value;
		return true;
	}

	// - log type by name
	bool _get_log_type(std::string_view _name, log_type_t& _type) noexcept
	{
		auto name = __to_lower(_name);
		if (name == "info")							{ _type = log_type_t::info; return true; }
		if (name == "progress")						{ _type = log_type_t::progress; return true; }
		if (name == "debug")						{ _type = log_type_t::debug; return true; }
		if (name == "fatal" || name == "exception")	{ _type = log_type_t::fatal; return true; }
		if (name == "error")						{ _type = log_type_t::error; return true; }
		if (name == "warning")						{ _type = log_type_t::warning; return true; }
		if (name == "user")							{ _type = log_type_t::user; return true; }
		if (name == "system")						{ _type = log_type_t::system; return true; }
		return false;
	}

	// - "fatal,error" -> mask
	uint32_t _parse_types(std::string_view _text) noexcept
	{
		uint32_t mask = 0;
		while (_text.empty() == false)
		{
			const auto pos = _text.find(',');
			auto item = _text.substr(0, pos);
			_text = (pos == std::string_view::npos) ? std::string_view() : _text.substr(pos + 1);

			while (item.empty() == false && (item.front() == ' ' || item.front() == '\t')) item.remove_prefix(1);
			while (item.empty() == false && (item.back() == ' ' || item.back() == '\t')) item.remove_suffix(1);

			log_type_t type;
			if (_get_log_type(item, type))
				mask |= (1u << static_cast<uint32_t>(type));
		}
		return mask;
	}

	constexpr uint32_t DEFAULT_MASK_TYPES = (1u << static_cast<uint32_t>(log_type_t::fatal)) | (1u << static_cast<uint32_t>(log_type_t::error));

	// - json string (escaped)
	void _append_json_string(std::string& _out, std::string_view _text)
	{
		_out += '"';
		for (const char ch : _text)
		{
			switch (ch)
			{
			case '"':	_out += "\\\""; break;
			case '\\':	_out += "\\\\"; break;
			case '\n':	_out += "\\n"; break;
			case '\r':	_out += "\\r"; break;
			case '\t':	_out += "\\t"; break;
			default:
				if (static_cast<unsigned char>(ch) < 0x20)
				{
					char temp[8];
					std::snprintf(temp, sizeof(temp), "\\u%04x", static_cast<unsigned>(static_cast<unsigned char>(ch)));
					_out += temp;
				}
				else
				{
					_out += ch;
				}
				break;
			}
		}
		_out += '"';
	}

	// - a value of a json string in a response ("key":"value")
	std::string _find_json_string(std::string_view _json, std::string_view _key)
	{
		const std::string token = "\"" + std::string(_key) + "\":\"";
		const auto pos = _json.find(token);
		if (pos == std::string_view::npos)
			return std::string();

		const auto begin = pos + token.size();
		const auto end = _json.find('"', begin);
		if (end == std::string_view::npos)
			return std::string();

		return std::string(_json.substr(begin, end - begin));
	}

	// - the end of a part of _text not longer than _max (at a line, or at a utf-8 character)
	size_t _find_split(std::string_view _text, size_t _max) noexcept
	{
		if (_text.size() <= _max)
			return _text.size();

		// 1) the last line in it
		const auto pos = _text.rfind('\n', _max);
		if (pos != std::string_view::npos && pos > 0)
			return pos + 1;

		// 2) a long line : at a utf-8 character
		size_t end = _max;
		while (end > 0 && (static_cast<unsigned char>(_text[end]) & 0xc0) == 0x80)
			--end;
		return (end > 0) ? end : _max;
	}

	// - slack text : '&', '<', '>' are escaped (slack requires it)
	void _append_slack_escaped(std::string& _out, std::string_view _text)
	{
		for (const char ch : _text)
		{
			switch (ch)
			{
			case '&':	_out += "&amp;"; break;
			case '<':	_out += "&lt;"; break;
			case '>':	_out += "&gt;"; break;
			default:	_out += ch; break;
			}
		}
	}

	// - a run of a style : the marks are put around the text without the spaces at both ends (slack needs it)
	void _append_styled_run(std::string& _out, std::string_view _text, uint32_t _style)
	{
		// check) no style
		if (_style == 0)
		{
			_append_slack_escaped(_out, _text);
			return;
		}

		size_t begin = 0;
		size_t end = _text.size();
		while (begin < end && (_text[begin] == ' ' || _text[begin] == '\t'))
			++begin;
		while (end > begin && (_text[end - 1] == ' ' || _text[end - 1] == '\t'))
			--end;

		_append_slack_escaped(_out, _text.substr(0, begin));
		if (begin < end)
		{
			if (_style & STYLE_BOLD)	_out += '*';
			if (_style & STYLE_ITALIC)	_out += '_';
			if (_style & STYLE_STRIKE)	_out += '~';
			_append_slack_escaped(_out, _text.substr(begin, end - begin));
			if (_style & STYLE_STRIKE)	_out += '~';
			if (_style & STYLE_ITALIC)	_out += '_';
			if (_style & STYLE_BOLD)	_out += '*';
		}
		_append_slack_escaped(_out, _text.substr(end));
	}

	// - sgr parameters ("1;31") -> style (the colors are ignored : slack can't show the color of a text)
	uint32_t _apply_sgr(std::string_view _params, uint32_t _style) noexcept
	{
		size_t pos = 0;
		bool is_end = false;

		auto next = [&](int& _value) -> bool
		{
			if (is_end)
				return false;

			auto end = _params.find(';', pos);
			if (end == std::string_view::npos)
			{
				end = _params.size();
				is_end = true;
			}

			const auto token = _params.substr(pos, end - pos);
			pos = end + 1;

			// - an empty parameter is 0, a sub parameter (':') is ignored
			_value = 0;
			if (token.find(':') != std::string_view::npos)
			{
				_value = -1;
				return true;
			}
			std::from_chars(token.data(), token.data() + token.size(), _value);
			return true;
		};

		int code = 0;
		while (next(code))
		{
			switch (code)
			{
			case 0:		_style = 0; break;
			case 1:		_style |= STYLE_BOLD; break;
			case 3:		_style |= STYLE_ITALIC; break;
			case 9:		_style |= STYLE_STRIKE; break;
			case 21:
			case 22:	_style &= ~STYLE_BOLD; break;
			case 23:	_style &= ~STYLE_ITALIC; break;
			case 29:	_style &= ~STYLE_STRIKE; break;

			// - extended color (38;5;n / 38;2;r;g;b) : skipped
			case 38:
			case 48:
			case 58:
				{
					int mode = 0;
					if (next(mode) == false)
						break;

					const int count_skip = (mode == 5) ? 1 : ((mode == 2) ? 3 : 0);
					for (int i = 0; i < count_skip; ++i)
					{
						int value = 0;
						if (next(value) == false)
							break;
					}
				}
				break;

			// - colors, underline, ... : not in slack
			default:
				break;
			}
		}

		return _style;
	}

	// - a line : the escape sequences are removed (sgr -> slack marks when _is_format)
	void _append_line(std::string& _out, std::string_view _line, uint32_t& _style, bool _is_format)
	{
		std::string run;
		run.reserve(_line.size());

		size_t i = 0;
		const size_t size = _line.size();
		while (i < size)
		{
			const char ch = _line[i];

			// 1) a character (the control characters except tab are removed)
			if (ch != '\x1b')
			{
				if (static_cast<unsigned char>(ch) >= 0x20 || ch == '\t')
					run += ch;
				++i;
				continue;
			}

			// 2) CSI (ESC '[' ... final byte)
			if (i + 1 < size && _line[i + 1] == '[')
			{
				size_t j = i + 2;
				while (j < size && (static_cast<unsigned char>(_line[j]) < 0x40 || static_cast<unsigned char>(_line[j]) > 0x7e))
					++j;

				if (j < size && _line[j] == 'm' && _is_format)
				{
					const auto style = _apply_sgr(_line.substr(i + 2, j - (i + 2)), _style);
					if (style != _style)
					{
						_append_styled_run(_out, run, _style);
						run.clear();
						_style = style;
					}
				}

				i = (j < size) ? j + 1 : size;
			}
			// 3) OSC (ESC ']' ... BEL or ESC '\')
			else if (i + 1 < size && _line[i + 1] == ']')
			{
				size_t j = i + 2;
				while (j < size && _line[j] != '\x07' && !(_line[j] == '\x1b' && j + 1 < size && _line[j + 1] == '\\'))
					++j;

				i = (j >= size) ? size : ((_line[j] == '\x07') ? j + 1 : j + 2);
			}
			// 4) the others (ESC + a character)
			else
			{
				i = (i + 2 < size) ? i + 2 : size;
			}
		}

		_append_styled_run(_out, run, _style);
	}

	// - the color bar of a log type (as logger_console)
	std::string_view _get_type_color(log_type_t _type, bool _is_high) noexcept
	{
		switch (_type)
		{
		case log_type_t::info:		return _is_high ? "#d0d0d0"sv : "#a0a0a0"sv;
		case log_type_t::progress:	return _is_high ? "#23d18b"sv : "#0dbc79"sv;
		case log_type_t::debug:		return "#666666"sv;
		case log_type_t::fatal:		return "#cd3131"sv;
		case log_type_t::error:		return _is_high ? "#f14c4c"sv : "#cd3131"sv;
		case log_type_t::warning:	return _is_high ? "#d670d6"sv : "#bc3fbc"sv;
		case log_type_t::user:		return _is_high ? "#f5f543"sv : "#e5e510"sv;
		case log_type_t::system:	return _is_high ? "#3b8eea"sv : "#2472c8"sv;
		default:					return "#a0a0a0"sv;
		}
	}
}


//-----------------------------------------------------------------------------
// logger_slack
//-----------------------------------------------------------------------------
logger_slack::logger_slack()
{
}

logger_slack::~logger_slack() noexcept
{
	this->destroy();
}

void logger_slack::append_slack_text(std::string& _out, std::string_view _text, eANSI _ansi, uint32_t& _style)
{
	// 1) raw : as it is ('&', '<', '>' escaped only)
	if (_ansi == eANSI::raw)
	{
		_append_slack_escaped(_out, _text);
		return;
	}

	// 2) format / strip : line by line (the style is carried to the next line)
	const bool is_format = (_ansi == eANSI::format);
	while (_text.empty() == false)
	{
		const auto pos = _text.find('\n');
		const auto line = _text.substr(0, pos);
		if (line.empty() == false && line.back() == '\r')
			_append_line(_out, line.substr(0, line.size() - 1), _style, is_format);
		else
			_append_line(_out, line, _style, is_format);

		if (pos == std::string_view::npos)
			break;

		_out += '\n';
		_text.remove_prefix(pos + 1);
	}
}

result_code logger_slack::initialize(const INITIALIZE_PARAMETER& _initialize_param)
{
	// declare)
	context context_setting;

	// 1) settings
	if (_initialize_param.token.empty() == false)
		context_setting["token"sv] = _initialize_param.token;
	if (_initialize_param.channel.empty() == false)
		context_setting["channel"sv] = _initialize_param.channel;
	if (_initialize_param.webhook_url.empty() == false)
		context_setting["webhook_url"sv] = _initialize_param.webhook_url;
	if (_initialize_param.title.empty() == false)
		context_setting["title"sv] = _initialize_param.title;
	if (_initialize_param.types.empty() == false)
		context_setting["types"sv] = _initialize_param.types;
	context_setting["show_time"sv] = _initialize_param.is_show_time ? "true"sv : "false"sv;
	context_setting["ansi"sv] = (_initialize_param.ansi == eANSI::raw) ? "raw"sv : ((_initialize_param.ansi == eANSI::strip) ? "strip"sv : "format"sv);
	context_setting["max_message_length"sv] = std::to_string(_initialize_param.max_message_length);

	// return)
	return this->Ninitializable::initialize(context_setting);
}

result_code logger_slack::initialize(const std::string_view _token, const std::string_view _channel, const std::string_view _title)
{
	INITIALIZE_PARAMETER param;
	param.token = _token;
	param.channel = _channel;
	param.title = _title;
	return this->initialize(param);
}

result_code logger_slack::_process_initializing(context& _context)
{
	// trace)
	LOG_PROGRESS << "@ <BEGIN> initializing LOGGER_SLACK["sv << this->name() << "]"sv;

	// 1) Nlogger ("filter")
	auto result = this->Nlogger::_process_initializing(_context);

	// 2) bot token or webhook
	auto token = _get_setting(_context, "token"sv);
	auto webhook_url = _get_setting(_context, "webhook_url"sv);
	auto channel = _get_setting(_context, "channel"sv);

	if (token.empty() == false)
	{
		// check)
		THROW_IF(channel.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT), LOG_ERROR << "@ (err) logger_slack : 'channel' is needed with 'token'"sv);

		this->m_host = SLACK_API_HOST;
		this->m_path = SLACK_API_PATH;
		this->m_token = std::move(token);
		this->m_channel = std::move(channel);

		// trace)
		LOG_INFO << "  + mode: bot token (chat.postMessage), channel: '"sv << this->m_channel << "'"sv;
	}
	else if (webhook_url.empty() == false)
	{
		// - "https://host/path"
		constexpr std::string_view scheme = "https://";
		std::string_view url = webhook_url;
		THROW_IF(url.substr(0, scheme.size()) != scheme, throwable::failure(eRESULT::INVALID_ARGUMENT), LOG_ERROR << "@ (err) logger_slack : 'webhook_url' must be https"sv);
		url.remove_prefix(scheme.size());

		const auto pos = url.find('/');
		THROW_IF(pos == std::string_view::npos || pos == 0, throwable::failure(eRESULT::INVALID_ARGUMENT), LOG_ERROR << "@ (err) logger_slack : invalid 'webhook_url'"sv);

		this->m_host = std::string(url.substr(0, pos));
		this->m_path = std::string(url.substr(pos));
		this->m_token.clear();
		this->m_channel.clear();

		// trace) (the path of a webhook is a secret : not logged)
		LOG_INFO << "  + mode: incoming webhook, host: '"sv << this->m_host << "'"sv;
	}
	else
	{
		THROW_IF(true, throwable::failure(eRESULT::INVALID_ARGUMENT), LOG_ERROR << "@ (err) logger_slack : 'token' (+'channel') or 'webhook_url' is needed"sv);
	}

	// 3) types
	this->m_mask_types = DEFAULT_MASK_TYPES;
	if (auto& context_types = _context.find("types"sv); context_types.is_valid())
	{
		uint32_t mask = 0;
		if (context_types.type() == context::value_type_t::node_)
		{
			for (auto& iter : context_types)
			{
				std::string name = iter.second;
				mask |= _parse_types(name);
			}
		}
		else
		{
			std::string text = context_types;
			mask = _parse_types(text);
		}

		if (mask != 0)
			this->m_mask_types = mask;
	}

	// 4) title
	if (auto& context_title = _context.find("title"sv); context_title.is_valid())
		this->m_title = context_title.operator std::string();
	else
		this->m_title = std::string(this->name());

	// 5) show_time
	this->m_is_show_time = _get_bool(_context, "show_time"sv, true);

	// 6) ansi ("format" / "raw" / "strip")
	this->m_ansi = eANSI::format;
	if (auto& context_ansi = _context.find("ansi"sv); context_ansi.is_valid())
	{
		std::string text = context_ansi;
		auto value = __to_lower(std::string_view(text));
		if (value == "raw")
			this->m_ansi = eANSI::raw;
		else if (value == "strip" || value == "false")
			this->m_ansi = eANSI::strip;
	}

	// 7) max_message_length
	if (size_t length = 0; _get_number(_context, "max_message_length"sv, length) && length != 0)
		this->m_max_message_length = std::clamp<size_t>(length, 500, 39000);

	// 8) max_queue
	if (size_t count = 0; _get_number(_context, "max_queue"sv, count) && count != 0)
		this->m_max_queue = count;

	// 9) timeout (ms)
	if (int64_t ms = 0; _get_number(_context, "timeout"sv, ms) && ms > 0)
		this->m_timeout = std::chrono::milliseconds(ms);

	// 10) reset
	this->reset_log_count();
	{
		std::lock_guard lock(this->m_lockable_send);
		this->m_is_send_failed = false;
	}

	// trace)
	LOG_PROGRESS << "@ <END> initializing LOGGER_SLACK["sv << this->name() << "]"sv;

	// return)
	return result;
}

result_code logger_slack::_process_destroying() noexcept
{
	// 1) reset the default logger if it is this
	reset_default_logger(this);

	// 2) detach from parent logger
	this->detach_from_parent();

	// 3) on_destroying
	try
	{
		this->on_destroying();
	}
	catch (...)
	{
		// log)
		LOG_ERROR << "@ (excp) exception on 'on_destroying' function call ['"sv << this->name() << "']"sv;
	}

	// 4) wait for the completion of the queued logs (the messages are queued)
	this->_process_wait_complete();

	// 5) wait for the messages sent (up to 3 times of the timeout : the rest are dropped)
	{
		const auto time_limit = std::chrono::steady_clock::now() + this->m_timeout * 3;

		std::unique_lock lock(this->m_lockable_send);
		while (this->m_ppost_sending || this->m_queue_post.empty() == false)
		{
			// - the limit
			if (std::chrono::steady_clock::now() >= time_limit)
			{
				const size_t count = this->m_queue_post.size() + (this->m_ppost_sending ? 1 : 0);
				this->m_count_dropped.fetch_add(count, std::memory_order_relaxed);
				this->m_queue_post.clear();
				this->m_ppost_sending.reset();
				++this->m_id_request;	// - a late response is ignored
				break;
			}

			this->m_cv_send.wait_for(lock, std::chrono::milliseconds(100));

			// - a request timed out
			lock.unlock();
			this->_process_check_timeout();
			lock.lock();
		}
	}

	// return)
	return eRESULT::SUCCESS;
}

bool logger_slack::_is_sent(const LOG_RECORD& _record) const noexcept
{
	// check) message must be not empty
	if (_record.buf_message.empty())
		return false;

	// 1) types
	const auto type = static_cast<uint32_t>(_record.type) & 0xffff;
	if (type >= 32 || (this->m_mask_types & (1u << type)) == 0)
		return false;

	// 2) filter
	if (auto pfilter = this->_get_filter(); pfilter != nullptr)
	{
		if (pfilter->process_filtering(&_record) == false)
			return false;
	}

	// return)
	return true;
}

std::string_view logger_slack::_get_color(const LOG_RECORD& _record) const noexcept
{
	const auto type = static_cast<log_type_t>(static_cast<uint32_t>(_record.type) & 0xffff);
	return _get_type_color(type, !(_record.level < log_level_t::higher()));
}

void logger_slack::_append_record(std::string& _out, const LOG_RECORD& _record, size_t _depth) const
{
	const std::string indent(_depth * 4, ' ');

	// 1) date ("[yyyy-mm-dd hh:mm:ss] ")
	auto buf_date = alloc_shared_buffer(64);
	if (this->m_is_show_time)
		LOG_RECORD::append_text_date(buf_date, _record.time_occurred, eCHARACTER_SET::UTF8);

	// 2) type
	const auto type = static_cast<log_type_t>(static_cast<uint32_t>(_record.type) & 0xffff);

	// 3) lines (converted to utf-8, the sgr style is carried to the next line)
	uint32_t style = 0;
	auto buf_line = alloc_shared_buffer(LOG_RECORD::get_buffer_size(_record.buf_message, _record.char_encoding, eCHARACTER_SET::UTF8) + 64);
	auto count_line = _record.line_count();
	size_t pos = sizeof(COUNT_T);
	for (size_t i = 0; i < count_line; ++i)
	{
		buf_line.set_size(0);
		pos = _record.get_log_message(buf_line, eCHARACTER_SET::UTF8, pos);

		_out += indent;
		if (i == 0)
		{
			_out.append(buf_date.data(), buf_date.size());
			_out += '[';
			_out += ::to_string(type);
			_out += "] ";
		}
		else
		{
			_out += "    ";
		}
		append_slack_text(_out, std::string_view(buf_line.data(), buf_line.size()), this->m_ansi, style);
		_out += '\n';
	}
}

void logger_slack::_append_sub_records(std::string& _out, const LOG_RECORD& _record, size_t _depth) const
{
	for (auto& iter : _record.sub_plog())
	{
		if (iter.empty())
			continue;

		this->_append_record(_out, *iter, _depth);
		this->_append_sub_records(_out, *iter, _depth + 1);
	}
}

void logger_slack::process_log_batch(std::vector<object_ptr<LOG_RECORD>>& _vector_plog_record)
{
	// check)
	if (this->m_host.empty())
		return;

	// 1) a request timed out
	this->_process_check_timeout();

	// - a thread is possible with the bot token only (a webhook returns no ts)
	const bool is_thread = (this->m_token.empty() == false);

	// declare) the logs without a thread are sent together
	std::vector<sSEGMENT> vector_pending;

	for (auto& iter : _vector_plog_record)
	{
		// check)
		if (iter.empty() || this->_is_sent(*iter) == false)
			continue;

		// statistics)
		this->process_count_log(iter->type);

		// 2) a log without sub logs (or a webhook : the sub logs are appended)
		if (iter->sub_plog().empty() || is_thread == false)
		{
			sSEGMENT segment{ this->_get_color(*iter), std::string() };
			this->_append_record(segment.text, *iter, 0);
			this->_append_sub_records(segment.text, *iter, 1);
			vector_pending.push_back(std::move(segment));
			continue;
		}

		// 3) a log with sub logs : the message, then the sub logs in its thread (the ts is set when the message is sent)
		if (vector_pending.empty() == false)
		{
			this->_queue_segments(std::move(vector_pending), nullptr, nullptr);
			vector_pending.clear();
		}

		auto pts = std::make_shared<std::string>();
		{
			std::vector<sSEGMENT> vector_main;
			sSEGMENT segment{ this->_get_color(*iter), std::string() };
			this->_append_record(segment.text, *iter, 0);
			vector_main.push_back(std::move(segment));
			this->_queue_segments(std::move(vector_main), nullptr, pts);
		}
		{
			std::vector<sSEGMENT> vector_sub;
			for (auto& iter_sub : iter->sub_plog())
			{
				if (iter_sub.empty())
					continue;

				sSEGMENT segment{ this->_get_color(*iter_sub), std::string() };
				this->_append_record(segment.text, *iter_sub, 0);
				this->_append_sub_records(segment.text, *iter_sub, 1);
				vector_sub.push_back(std::move(segment));
			}
			this->_queue_segments(std::move(vector_sub), pts, nullptr);
		}
	}

	// 4) the rest
	if (vector_pending.empty() == false)
		this->_queue_segments(std::move(vector_pending), nullptr, nullptr);
}

void logger_slack::_queue_segments(std::vector<sSEGMENT>&& _vector_segment, const std::shared_ptr<std::string>& _pthread_ts, const std::shared_ptr<std::string>& _pts_result)
{
	// check)
	if (_vector_segment.empty())
		return;

	// - the size of a message (the title is at the first line of every message)
	const size_t size_title = this->m_title.empty() ? 0 : this->m_title.size() + 1;
	const size_t max_text = (this->m_max_message_length > size_title + 100) ? this->m_max_message_length - size_title : 100;
	const size_t max_segments = (this->m_ansi == eANSI::format) ? MAX_ATTACHMENTS : SIZE_MAX;

	// 1) the segments -> the posts (a long segment is split : the same color)
	std::vector<std::shared_ptr<sPOST>> vector_post;
	auto ppost = std::make_shared<sPOST>();
	size_t size_now = 0;

	auto fn_flush = [&]()
	{
		if (ppost->vector_segment.empty())
			return;

		ppost->pthread_ts = _pthread_ts;
		if (vector_post.empty())
			ppost->pts_result = _pts_result;	// - the ts of the first message
		vector_post.push_back(std::move(ppost));
		ppost = std::make_shared<sPOST>();
		size_now = 0;
	};

	for (auto& iter : _vector_segment)
	{
		std::string_view text = iter.text;
		while (text.empty() == false)
		{
			const auto size = _find_split(text, max_text);
			const auto part = text.substr(0, size);
			text.remove_prefix(size);

			if (size_now + part.size() > max_text || ppost->vector_segment.size() >= max_segments)
				fn_flush();

			ppost->vector_segment.push_back(sSEGMENT{ iter.color, std::string(part) });
			size_now += part.size();
		}
	}
	fn_flush();

	// 2) queued (more than m_max_queue : dropped)
	{
		std::lock_guard lock(this->m_lockable_send);
		for (auto& iter : vector_post)
		{
			if (this->m_queue_post.size() >= this->m_max_queue)
			{
				this->m_count_dropped.fetch_add(1, std::memory_order_relaxed);
				continue;
			}
			this->m_queue_post.push_back(std::move(iter));
		}
	}

	// 3) send
	this->_process_send_next();
}

void logger_slack::_process_send_next()
{
	for (;;)
	{
		// 1) the next message (none if a message is in flight)
		std::shared_ptr<sPOST> ppost;
		uint64_t id = 0;
		{
			std::lock_guard lock(this->m_lockable_send);

			if (this->m_ppost_sending)
				return;

			if (this->m_queue_post.empty())
			{
				this->m_cv_send.notify_all();
				return;
			}

			ppost = std::move(this->m_queue_post.front());
			this->m_queue_post.pop_front();

			this->m_ppost_sending = ppost;
			id = ++this->m_id_request;
			this->m_time_request = std::chrono::steady_clock::now();
		}

		// 2) request (the response comes on the io thread)
		if (this->_process_request(ppost, id))
			return;

		// 3) fail to request : the next message
		this->_process_on_response(id, false, std::string());
	}
}

bool logger_slack::_process_request(const std::shared_ptr<sPOST>& _ppost, uint64_t _id)
{
	try
	{
		// 1) body (json)
		const auto body = this->_make_body(*_ppost);

		// 2) headers
		const std::string header_authorization = this->m_token.empty() ? std::string() : ("Authorization: Bearer " + this->m_token);
		std::vector<std::string_view> vector_header{ "Content-Type: application/json; charset=utf-8"sv, "Connection: close"sv };
		if (header_authorization.empty() == false)
			vector_header.push_back(header_authorization);
		const std::vector<std::string_view> vector_accept{ "application/json"sv };

		// 3) request (https POST)
		auto prequest = make_object<net::http_request>();
		prequest->set_request(eHTTP_METHOD::POST_, this->m_host, 443, "HTTP/1.1"sv, this->m_path, std::string_view(), vector_header, to_buffer_view(std::string_view(body)), vector_accept, true);

		// - the response (on the io thread) : the next message is requested
		prequest->set_function([pthis = object_ptr<logger_slack>(this), _id](sMESSAGE_HTTP_RESPONSE& _msg)
		{
			// check)
			if (_msg.is_completed == false)
				return;

			std::string body_response;
			for (auto& iter : _msg.contents)
				body_response.append(iter.data(), iter.size());

			pthis->_process_on_response(_id, _msg.status_code == eHTTP_STATUS_CODE::OK_, body_response);
			pthis->_process_send_next();
		});

		// return)
		return prequest->process_request() == eRESULT::SUCCESS;
	}
	catch (...)
	{
		return false;
	}
}

void logger_slack::_process_on_response(uint64_t _id, bool _is_success, const std::string& _body)
{
	// - http 200 (and "ok":true for chat.postMessage)
	bool result = _is_success;
	if (result && this->m_token.empty() == false)
		result = (_body.find("\"ok\":true") != std::string::npos);

	bool is_log_error = false;
	{
		std::lock_guard lock(this->m_lockable_send);

		// check) not the message in flight (timed out already)
		if (this->m_ppost_sending == nullptr || _id != this->m_id_request)
			return;

		// - the ts of the message (the sub logs are sent to its thread : set before the next message)
		if (result && this->m_ppost_sending->pts_result)
			*this->m_ppost_sending->pts_result = _find_json_string(_body, "ts"sv);

		this->m_ppost_sending.reset();

		// statistics)
		if (result)
		{
			this->m_count_sent.fetch_add(1, std::memory_order_relaxed);
			this->m_is_send_failed = false;
		}
		else
		{
			this->m_count_failed.fetch_add(1, std::memory_order_relaxed);
			is_log_error = (this->m_is_send_failed == false);
			this->m_is_send_failed = true;
		}

		this->m_cv_send.notify_all();
	}

	// log) only when it fails first (the error log may come back to this logger)
	if (is_log_error)
	{
		LOG_ERROR << "@ (err) logger_slack['"sv << this->name() << "'] fail to send (success: "sv << (_is_success ? "true"sv : "false"sv) << ", error: '"sv << _find_json_string(_body, "error"sv) << "')"sv;
	}
}

void logger_slack::_process_check_timeout()
{
	uint64_t id = 0;
	{
		std::lock_guard lock(this->m_lockable_send);

		// check) no message in flight or not timed out
		if (this->m_ppost_sending == nullptr || std::chrono::steady_clock::now() - this->m_time_request < this->m_timeout)
			return;

		id = this->m_id_request;
	}

	// - failed (a late response is ignored) and the next message
	this->_process_on_response(id, false, std::string());
	this->_process_send_next();
}

std::string logger_slack::_make_body(const sPOST& _post) const
{
	// declare)
	std::string body;
	size_t size_reserve = this->m_title.size() + 256;
	for (auto& iter : _post.vector_segment)
		size_reserve += iter.text.size() + 64;
	body.reserve(size_reserve);

	// - the title (escaped : the logs are escaped by _append_record)
	std::string title;
	{
		uint32_t style = 0;
		append_slack_text(title, this->m_title, eANSI::strip, style);
	}

	body += '{';

	// 1) channel (bot token)
	if (this->m_channel.empty() == false)
	{
		body += "\"channel\":";
		_append_json_string(body, this->m_channel);
		body += ',';
	}

	// 2) thread (the sub logs)
	if (_post.pthread_ts && _post.pthread_ts->empty() == false)
	{
		body += "\"thread_ts\":";
		_append_json_string(body, *_post.pthread_ts);
		body += ',';
	}

	// 3) "format" : the title and the logs as the attachments (the color bar of each log)
	if (this->m_ansi == eANSI::format)
	{
		body += "\"text\":";
		_append_json_string(body, title);
		body += ",\"attachments\":[";

		bool is_first = true;
		for (auto& iter : _post.vector_segment)
		{
			std::string_view text = iter.text;
			while (text.empty() == false && text.back() == '\n')
				text.remove_suffix(1);

			if (is_first == false)
				body += ',';
			is_first = false;

			body += "{\"color\":";
			_append_json_string(body, iter.color);
			body += ",\"text\":";
			_append_json_string(body, text);
			body += ",\"mrkdwn_in\":[\"text\"]}";
		}
		body += ']';
	}
	// 4) "raw" / "strip" : a text
	else
	{
		std::string text;
		text.reserve(size_reserve);
		if (title.empty() == false)
		{
			text += title;
			text += '\n';
		}
		for (auto& iter : _post.vector_segment)
			text += iter.text;

		body += "\"text\":";
		_append_json_string(body, text);
	}

	body += '}';

	// return)
	return body;
}
