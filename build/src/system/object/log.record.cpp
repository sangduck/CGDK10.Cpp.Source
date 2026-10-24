#include "pch.h"
#include "TLOG_RECORD.h"
#include <cstdlib>


namespace CGDK
{

namespace
{
	// - reads a line "[COUNT_T length (terminator included)][units][terminator]" at _p
	//   (false : broken buffer)
	bool _next_line(const char*& _p, const char* _end, std::size_t _unit, const void*& _text, std::size_t& _count) noexcept
	{
		if (static_cast<std::size_t>(_end - _p) < sizeof(COUNT_T))
			return false;

		COUNT_T length;
		memcpy(&length, _p, sizeof(COUNT_T));

		// - null string
		if (length <= 0)
		{
			_p += sizeof(COUNT_T);
			_text = _p;
			_count = 0;
			return true;
		}

		const auto bytes = static_cast<std::size_t>(length) * _unit;
		if (static_cast<std::size_t>(_end - _p) - sizeof(COUNT_T) < bytes)
			return false;

		_text = _p + sizeof(COUNT_T);
		_count = static_cast<std::size_t>(length) - 1;
		_p += sizeof(COUNT_T) + bytes;
		return true;
	}
}

size_t LOG_RECORD::get_buffer_size(buffer_view _buf_source, eCHARACTER_SET _character_set_src, eCHARACTER_SET _character_set_dst) noexcept
{
	// check)
	if (_buf_source.size() < sizeof(COUNT_T))
		return 32;

	// declare)
	const char* p = reinterpret_cast<const char*>(_buf_source.data());
	const char* end = p + _buf_source.size();
	const auto unit_src = __get_unit_size(_character_set_src);
	const auto unit_dst = __get_unit_size(_character_set_dst);
	const auto ratio = __get_convert_ratio(_character_set_dst, _character_set_src);

	// 1) line count
	COUNT_T count_line;
	memcpy(&count_line, p, sizeof(COUNT_T));
	p += sizeof(COUNT_T);

	// 2) every line : [COUNT_T][converted text][terminator] (no line is cut by __convert_text)
	std::size_t buffer_size = sizeof(COUNT_T);
	for (; count_line > 0; --count_line)
	{
		const void* text;
		std::size_t count;
		if (!_next_line(p, end, unit_src, text, count))
			break;

		buffer_size += sizeof(COUNT_T) + (count * ratio + 1) * unit_dst;
	}

	// return)
	return buffer_size;
}

void append_to_buffer(shared_buffer& _buf_dst, eCHARACTER_SET _character_set_dst, buffer_view _buf_src, eCHARACTER_SET _character_set_src)
{
	if (_character_set_dst == _character_set_src)
		_buf_dst.append(_buf_src.size(), _buf_src.data());
	else
		__convert_lines(_buf_dst, _character_set_dst, _buf_src, _character_set_src);
}

size_t LOG_RECORD::get_log_message(shared_buffer& _buf_dst, eCHARACTER_SET _character_set_dst, size_t _offset_src) const
{
	// declare)
	const char* begin = reinterpret_cast<const char*>(buf_message.data());
	const char* end = begin + buf_message.size();
	const char* p = begin + _offset_src;

	// 1) the line at _offset_src
	const void* text;
	std::size_t count;
	if (p > end || !_next_line(p, end, __get_unit_size(char_encoding), text, count))
		return buf_message.size();

	// 2) append the text (converted)
	const auto unit = __get_unit_size(_character_set_dst);
	const auto written = __convert_text(_buf_dst.data() + _buf_dst.size(), _buf_dst.get_remained_size() / unit, _character_set_dst, text, count, char_encoding);
	_buf_dst.add_size(written * unit);

	// return) offset of the next line
	return static_cast<size_t>(p - begin);
}
template<>
void LOG_RECORD::set_message<char>(const std::string_view _message, eCHARACTER_SET _character_set_dst) 
{
	// check) a record is not changed once it is traced
	CGASSERT_ERROR(this->is_traced() == false);

	// 1) get size of buffer
	const auto size = (_message.size() + 1) * sizeof(char) + sizeof(COUNT_T) + sizeof(COUNT_T);

	// 2) alloc buffer
	shared_buffer temp_buf_message = alloc_shared_buffer(size);

	// 3) write log line count
	temp_buf_message.append<COUNT_T>(1);

	// 4) write text
	temp_buf_message.append(_message);

	// 5) set shared_buffer
	{
		this->char_encoding = (_character_set_dst != eCHARACTER_SET::NONE && _character_set_dst != eCHARACTER_SET::UCS2) ? _character_set_dst : _DEFAULT_CHAR_CHARACTER_SET;
		this->buf_message = std::move(temp_buf_message);
	}
}

template<>
void LOG_RECORD::set_message<wchar_t>(const std::wstring_view _message, eCHARACTER_SET _character_set_dst) 
{
	// check) a record is not changed once it is traced
	CGASSERT_ERROR(this->is_traced() == false);

	// 1) get size of buffer
	const auto size = (_message.size() + 1) * sizeof(wchar_t) + sizeof(COUNT_T) + sizeof(COUNT_T);

	// 2) alloc buffer
	shared_buffer temp_buf_message = alloc_shared_buffer(size);

	// 2) write log line count
	temp_buf_message.append<COUNT_T>(1);

	// 3) write text
	temp_buf_message.append(_message);

	// 4) set shared_buffer
	{
		(void)_character_set_dst;
		this->char_encoding = eCHARACTER_SET::UCS2;	// - wchar_t text is always UCS2
		this->buf_message = std::move(temp_buf_message);
	}
}

size_t LOG_RECORD::line_count() const noexcept
{
	// check)
	if(buf_message.data() == nullptr)
		return 0;

	// check)
	if(buf_message.size() < sizeof(COUNT_T))
		return 0;

	// return) 
	return static_cast<size_t>(buf_message.front<COUNT_T>());
}


void LOG_RECORD::clear() noexcept
{
	// check) a record is not changed once it is traced
	CGASSERT_ERROR(this->is_traced() == false);

	this->buf_message.clear(); 
	this->m_sub_plog.clear();
}

object_ptr<LOG_RECORD> LOG_RECORD::clone(eCHARACTER_SET _character_set_dst) const
{
	// 1) 객체를 생성한다.
	auto plog_record = make_object<LOG_RECORD>();

	{
		// 2) 값을 복사한다.
		plog_record->type = this->type;
		plog_record->level = this->level;
		plog_record->result = this->result;
		plog_record->origin = this->origin;
		plog_record->attribute = this->attribute;
		plog_record->source = this->source;
		plog_record->destination = this->destination;
		plog_record->time_occurred = this->time_occurred;

		// declare)
		auto temp_char_encoding = (_character_set_dst != eCHARACTER_SET::NONE) ? _character_set_dst : this->char_encoding;

		// 3) Message를 변경한다.
		if (temp_char_encoding == this->char_encoding )
		{
			plog_record->char_encoding = this->char_encoding;
			plog_record->buf_message = this->buf_message;
		}
		else
		{
			// - convert
			shared_buffer temp_buf_message = alloc_shared_buffer(LOG_RECORD::get_buffer_size(this->buf_message, this->char_encoding, temp_char_encoding));
			__convert_lines(temp_buf_message, temp_char_encoding, this->buf_message, this->char_encoding);

			// - shrink to fit
			plog_record->char_encoding = temp_char_encoding;
			plog_record->buf_message = alloc_shared_buffer(temp_buf_message.size());
			plog_record->buf_message.append(temp_buf_message.size(), temp_buf_message.data());
		}

		// 4) Child도 동일하게 처리한다.
		for (auto& iter : this->m_sub_plog)
		{
			if (_character_set_dst == eCHARACTER_SET::NONE || iter->char_encoding == _character_set_dst)
			{
				plog_record->m_sub_plog.push_back(iter);
			}
			else
			{
				plog_record->m_sub_plog.push_back(iter->clone(_character_set_dst));
			}
		}
	}

	// return) 
	return plog_record;
}

object_ptr<LOG_RECORD> LOG_RECORD::_alloc()
{
	return TLOG_RECORD::_alloc();
}

void LOG_RECORD::process_serialize_in(CGDK::buffer_view& _buffer)
{
	this->type = _buffer.extract<log_type_t>();
	this->level = _buffer.extract<log_level_t>();
	this->result = _buffer.extract<result_code>();

	this->char_encoding = _buffer.extract<eCHARACTER_SET>();
	this->buf_message = _buffer.extract<shared_buffer>();

	this->origin = _buffer.extract<log_origin_t>();
	this->attribute = _buffer.extract<log_attribute_t>();
	this->source = _buffer.extract<log_source_t>();
	this->destination = _buffer.extract<log_destination_t>();
	this->time_occurred = _buffer.extract<chrono::time::utc::time_point>();

	this->m_sub_plog = _buffer.extract<std::vector<object_ptr<LOG_RECORD>>>();

}

void LOG_RECORD::process_serialize_out(CGDK::buffer& _buffer)
{
	_buffer.append<log_type_t>(this->type);
	_buffer.append<log_level_t>(this->level);
	_buffer.append<result_code>(this->result);

	_buffer.append<eCHARACTER_SET>(this->char_encoding);
	_buffer.append<shared_buffer>(this->buf_message);

	_buffer.append<log_origin_t>(this->origin);
	_buffer.append<log_attribute_t>(this->attribute);
	_buffer.append<log_source_t>(this->source);
	_buffer.append<log_destination_t>(this->destination);
	_buffer.append<chrono::time::utc::time_point>(this->time_occurred);

	_buffer.append<std::vector<object_ptr<LOG_RECORD>>>(this->m_sub_plog);
}

std::size_t LOG_RECORD::process_size_of() const
{
	// declare) 
	std::size_t size = 0;

	size += get_size_of(this->type);
	size += get_size_of(this->level);
	size += get_size_of(this->result);

	size += get_size_of(this->char_encoding);
	size += get_size_of(this->buf_message);

	size += get_size_of(this->origin);
	size += get_size_of(this->attribute);
	size += get_size_of(this->source);
	size += get_size_of(this->destination);
	size += get_size_of(this->time_occurred);

	size += get_size_of(this->m_sub_plog);

	// return)
	return size;
}

sMESSAGE_LOG::sMESSAGE_LOG(log_type_t _type, log_level_t _level, eCHARACTER_SET _character_set_src, const std::string_view _message) : sMESSAGE(eMESSAGE::SYSTEM::LOG_)
{
	// 1) allocate new LOG_RECORD object
	auto temp_plog_record = LOG_RECORD::_alloc();

	// check) no pool (the end of the process) : no record
	if (temp_plog_record.empty())
		return;

	// 2) set log info
	temp_plog_record->type = _type & 0xffff;
	temp_plog_record->level = _level;
	temp_plog_record->set_message(_message, _character_set_src);
	temp_plog_record->time_occurred = chrono::time::utc::now();

	// 3) Store LOG_RECORD object
	plog_record = std::move(temp_plog_record);
}

sMESSAGE_LOG::sMESSAGE_LOG(log_type_t _type, log_level_t _level, eCHARACTER_SET _character_set_src, const std::string_view _message, const chrono::time::utc::time_point& _time_occurred) : sMESSAGE(eMESSAGE::SYSTEM::LOG_)
{
	// 1) allocate new LOG_RECORD object
	auto temp_plog_record = LOG_RECORD::_alloc();

	// check) no pool (the end of the process) : no record
	if (temp_plog_record.empty())
		return;

	// 2) set log info
	temp_plog_record->type = _type & 0xffff;
	temp_plog_record->level = _level;
	temp_plog_record->set_message(_message, _character_set_src);
	temp_plog_record->time_occurred = _time_occurred;

	// 3) Store LOG_RECORD object
	plog_record = std::move(temp_plog_record);
}

}
