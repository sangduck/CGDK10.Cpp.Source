#include "pch.h"
#include "TLOG_RECORD.h"
#include <cstdlib>


namespace CGDK
{

void LOG_RECORD::append_text_date(shared_buffer& _buf_dst, chrono::time::utc::time_point _time_occurred, eCHARACTER_SET _character_set)
{
	// check) the date text is written in char (UCS2 isn't supported)
	CGASSERT_ERROR(_character_set == eCHARACTER_SET::MBCS || _character_set == eCHARACTER_SET::UTF8);

	// check)
	if(_character_set != eCHARACTER_SET::MBCS && _character_set != eCHARACTER_SET::UTF8)
		return;

	// 1) "[yyyy-mm-dd hh:mm:ss] " (cached on this thread : the logs of the same second share the text)
	struct sDATE_CACHE
	{
		int64_t		second = (std::numeric_limits<int64_t>::min)();
		char		text[23] = "[0000-00-00 00:00:00] ";
	};
	thread_local sDATE_CACHE t_cache;

	const int64_t second = std::chrono::floor<std::chrono::seconds>(_time_occurred.time_since_epoch()).count();
	if (second != t_cache.second)
	{
		const auto time_temp = chrono::to_date_time(_time_occurred);

		auto put = [](char* _p, unsigned _value, int _digits) noexcept
		{
			for (int i = _digits - 1; i >= 0; --i)
			{
				_p[i] = static_cast<char>('0' + _value % 10);
				_value /= 10;
			}
		};
		put(t_cache.text +  1, static_cast<unsigned>(time_temp.t_year) % 10000, 4);
		put(t_cache.text +  6, time_temp.t_month, 2);
		put(t_cache.text +  9, time_temp.t_day, 2);
		put(t_cache.text + 12, time_temp.t_hour, 2);
		put(t_cache.text + 15, time_temp.t_minute, 2);
		put(t_cache.text + 18, time_temp.t_second, 2);
		t_cache.second = second;
	}

	// 2) append
	_buf_dst.append_text(std::string_view(t_cache.text, sizeof(t_cache.text) - 1));
}
void append_to_buffer(shared_buffer& _buf_dst, eCHARACTER_SET _character_set_dst, buffer_view _buf_src, eCHARACTER_SET _character_set_src);

namespace
{
	// - appends a line converted to the record's character set
	void _add_message(LOG_RECORD& _record, const void* _text, std::size_t _count, eCHARACTER_SET _character_set_src)
	{
		// check) a record is not changed once it is traced
		CGASSERT_ERROR(_record.is_traced() == false);

		// declare)
		const auto character_set_dst = _record.char_encoding;
		const bool exist = (_record.buf_message.size() >= sizeof(COUNT_T));

		// 1) alloc buffer : current lines + a converted line
		const auto size_line = sizeof(COUNT_T) + (_count * __get_convert_ratio(character_set_dst, _character_set_src) + 1) * __get_unit_size(character_set_dst);
		shared_buffer buf_target = alloc_shared_buffer((exist ? _record.buf_message.size() : sizeof(COUNT_T)) + size_line);

		// 2) copy current lines
		if (exist)
			buf_target.append(_record.buf_message.size(), _record.buf_message.data());
		else
			buf_target.append<COUNT_T>(0);

		// 3) add a line
		++buf_target.front<COUNT_T&>();
		__append_line(buf_target, character_set_dst, _text, _count, _character_set_src);

		// 4) store
		_record.buf_message = std::move(buf_target);
	}
}

template<>
void LOG_RECORD::add_message<char>(const std::string_view _message, eCHARACTER_SET _character_set_src)
{
	// - char text is MBCS or UTF8
	_add_message(*this, _message.data(), _message.size(), (_character_set_src == eCHARACTER_SET::UTF8) ? eCHARACTER_SET::UTF8 : eCHARACTER_SET::MBCS);
}

template<>
void LOG_RECORD::add_message<wchar_t>(const std::wstring_view _message, eCHARACTER_SET /*_character_set_src*/)
{
	// - wchar_t text is always UCS2
	_add_message(*this, _message.data(), _message.size(), eCHARACTER_SET::UCS2);
}
void LOG_RECORD::add_sub_log(object_ptr<LOG_RECORD>&& _sub_log) 
{
	// check) a record is not changed once it is traced
	CGASSERT_ERROR(this->is_traced() == false);

	this->m_sub_plog.push_back(std::move(_sub_log));
}

void LOG_RECORD::remove_front_sub_log() noexcept
{
	// check) a record is not changed once it is traced
	CGASSERT_ERROR(this->is_traced() == false);

	// check)
	if (this->m_sub_plog.empty())
		return;

	this->m_sub_plog.erase(this->m_sub_plog.begin());
}

void LOG_RECORD::stream_out(std::ostream& _stream_out, eCHARACTER_SET _character_set_dst) const
{
	// 1) log type
	bool log_not_continue = (uint32_t)(type & log_type_t::continue_) == 0;

	// 2) alloc shared_buffer for log text
	auto buf_text_date = alloc_shared_buffer(64);
	auto buf_text_log = alloc_shared_buffer(LOG_RECORD::get_buffer_size(this->buf_message, this->char_encoding, _character_set_dst) + 64);

	// 3) 먼저 시간을 출력한다.
	if (log_not_continue)
		LOG_RECORD::append_text_date(buf_text_date, this->time_occurred, _character_set_dst);

	// 4) 메시지를 작성한다.
	append_to_buffer(buf_text_log, _character_set_dst, buf_message, char_encoding);

	// 5) CR을 붙인다.
	buf_text_log.append("\n");
	buf_text_log.append<char>(0);

	// 6) 출력한다.
	{
		// - shared_buffer
		shared_buffer temp_src = buf_text_log;

		// - line수를 읽어들인다.
		auto line_count = temp_src.extract<COUNT_T>();

		for (auto i = 0; i < line_count; ++i)
		{
			// - 날짜와 시간을 출력한다.
			if (log_not_continue && i == 0)
			{
				// - 출력한다.
				_stream_out << std::string_view(buf_text_date.data<const char>(), buf_text_date.size());
			}
			else
			{
				// - 빈칸을 출력한다.
				//           "[0000-00-00 00:00:00] "
				_stream_out << "                      ";
			}

			// - text를 읽어들인다.
			std::string_view temp_text = temp_src.extract<std::string_view>();

			_stream_out << temp_text;

			// - CR을 붙인다.
			_stream_out << '\n';
		}

		_stream_out.flush();
	}
}


}
