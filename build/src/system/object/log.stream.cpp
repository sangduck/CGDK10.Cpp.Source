#include "pch.h"
#include <charconv>
#include <format>
#include "TLOG_RECORD.h"


namespace CGDK
{

//-----------------------------------------------------------------------------
// _log_stream
//
//  - buf_message : [COUNT_T line count] { [COUNT_T length (terminator included)][text][0] } ...
//    pos is the offset of the current line, its text is extended in place
//  - the buffer is allocated at the first write with the size needed (256 at least)
//    and grows up to MAX_TRACE_STRING_LENGTH (a longer text is cut like before)
//-----------------------------------------------------------------------------
namespace
{
	// - initial size of buf_message
	constexpr std::size_t LOG_STREAM_INITIAL_BUFFER = 256;

	// - header of an empty message : [line count 1][length 1][0]
	constexpr std::size_t LOG_STREAM_HEADER = sizeof(COUNT_T) + sizeof(COUNT_T) + sizeof(char);

	// - makes room for _bytes after the end (grows up to MAX_TRACE_STRING_LENGTH)
	// return) remained bytes after the end
	std::size_t _reserve(LOG_RECORD& _record, std::size_t _bytes)
	{
		auto& buf = _record.buf_message;

		// - no header yet : header + _bytes at once
		//   (a pooled record keeps its empty buffer : reused when it is large enough)
		if (buf.empty())
		{
			auto size = LOG_STREAM_HEADER + _bytes;
			if (size < LOG_STREAM_INITIAL_BUFFER)
				size = LOG_STREAM_INITIAL_BUFFER;
			if (size > MAX_TRACE_STRING_LENGTH)
				size = MAX_TRACE_STRING_LENGTH;

			if (buf.data() == nullptr || buf.get_remained_size() < size)
				buf = alloc_shared_buffer(size);

			buf.append<COUNT_T>(1);
			buf.append<COUNT_T>(1);
			buf.append<char>(0);
		}

		const auto remained = buf.get_remained_size();

		// check) enough
		if (remained >= _bytes)
			return remained;

		// check) max size
		const auto capacity = buf.size() + remained;
		if (capacity >= MAX_TRACE_STRING_LENGTH)
			return remained;

		// 1) new capacity (x2 for small buffers, x1.5 for large buffers to reduce memory waste)
		constexpr std::size_t SIZE_THRESHOLD = 1024 * 1024;  // 1MB threshold
		auto new_capacity = (capacity != 0) ? capacity : LOG_STREAM_INITIAL_BUFFER;

		// Use x2 growth for small buffers, x1.5 for large buffers
		while (new_capacity < buf.size() + _bytes)
		{
			if (new_capacity < SIZE_THRESHOLD)
				new_capacity *= 2;
			else
				new_capacity = (new_capacity * 3) / 2;  // 1.5x growth
		}

		if (new_capacity > MAX_TRACE_STRING_LENGTH)
			new_capacity = MAX_TRACE_STRING_LENGTH;

		// 2) copy
		auto buf_new = alloc_shared_buffer(new_capacity);
		buf_new.append(buf.size(), buf.data());
		buf = std::move(buf_new);

		// return)
		return buf.get_remained_size();
	}

	// - the terminator of the current line (a text is written from here)
	//   text + new terminator can use 'remained + 1' bytes (the old terminator is reused)
	char* _get_tail(LOG_RECORD& _record, int64_t _pos) noexcept
	{
		auto& buf = _record.buf_message;
		return buf.data() + _pos + sizeof(COUNT_T) + buf.front<COUNT_T>(_pos) - sizeof(char);
	}

	// - _written bytes were written at the tail (+ terminator)
	void _commit(LOG_RECORD& _record, int64_t _pos, std::size_t _written) noexcept
	{
		auto& buf = _record.buf_message;
		buf.front<COUNT_T&>(_pos) += static_cast<COUNT_T>(_written);
		buf.add_size(_written);
	}

}

template <class T>
void _log_stream::process_stream_in(T _rhs)
{
	// check)
	RETURN_IF(plogger == nullptr);

	// check)
	RETURN_IF(plog_record.empty());

	// 1) room for a number (shortest double : 24 chars)
	const auto remained = _reserve(*plog_record, 32);
	auto dest_ptr = _get_tail(*plog_record, pos);

	// 2) write (std::to_chars)
	std::to_chars_result result;
	if constexpr (std::is_integral_v<T>)
	{
		if constexpr (std::is_unsigned_v<T>)
			result = std::to_chars(dest_ptr, dest_ptr + remained, static_cast<uint64_t>(_rhs));
		else
			result = std::to_chars(dest_ptr, dest_ptr + remained, static_cast<int64_t>(_rhs));
	}
	else
	{
		result = std::to_chars(dest_ptr, dest_ptr + remained, _rhs);
	}

	// check)
	RETURN_IF(result.ec != std::errc{});

	// 3) terminator + size
	*result.ptr = 0;
	_commit(*plog_record, pos, static_cast<std::size_t>(result.ptr - dest_ptr));
}

std::pair<char*, std::size_t> _log_stream::process_format_prepare(std::size_t _size)
{
#if !defined(_DEBUG) || defined(NDEBUG)
	// check)
	RETURN_IF(plogger == nullptr, std::pair<char*, std::size_t>(nullptr, 0));
#endif

	// check)
	RETURN_IF(plog_record.empty(), std::pair<char*, std::size_t>(nullptr, 0));

	// 1) room (text + terminator : 'remained + 1' bytes from the tail)
	const auto remained = _reserve(*plog_record, _size);

	// return)
	return { _get_tail(*plog_record, pos), remained };
}

void _log_stream::process_format_commit(char* _dest, std::size_t _written) noexcept
{
	_dest[_written] = 0;
	_commit(*plog_record, pos, _written);
}

_log_stream::_log_stream()
{
	try
	{
		// 1) log record (the message buffer is allocated at the first write)
		auto temp_log_record = LOG_RECORD::_alloc();

		// check) no pool (the end of the process) : the log is dropped (plog_record stays empty)
		if (temp_log_record.empty())
			return;

		temp_log_record->char_encoding = _DEFAULT_CHAR_CHARACTER_SET;

		// 3) set
		plog_record = std::move(temp_log_record);
		pos = sizeof(COUNT_T);
	}
	catch (...)
	{
	}
}

_log_stream::_log_stream(Ilogger* _plogger,  log_type_t _type) : _log_stream()
{
	// check)
	RETURN_IF(plog_record.empty());

	plogger = (_plogger != nullptr) ? _plogger : _get_default_logger_cached();

	plog_record->type = _type;
}

_log_stream::_log_stream(log_type_t _type) : _log_stream()
{
	// check)
	RETURN_IF(plog_record.empty());

	plogger = _get_default_logger_cached();

	plog_record->type = _type;
}

_log_stream& _log_stream::operator << (char _rhs)
{
	// - a char is written as a character (not as a number)
	return operator<<(std::string_view(&_rhs, 1));
}

_log_stream& _log_stream::operator << (int8_t _rhs)
{
	process_stream_in<int8_t>(_rhs); return *this;
}

_log_stream& _log_stream::operator << (uint8_t _rhs)
{
	process_stream_in<uint8_t>(_rhs); return *this;
}

_log_stream& _log_stream::operator << (int16_t _rhs)
{
	process_stream_in<int16_t>(_rhs); return *this;
}

_log_stream& _log_stream::operator << (uint16_t _rhs)
{
	process_stream_in<uint16_t>(_rhs); return *this;
}

_log_stream& _log_stream::operator << (int32_t _rhs)
{
	process_stream_in<int32_t>(_rhs); return *this;
}

_log_stream& _log_stream::operator << (uint32_t _rhs)
{
	process_stream_in<uint32_t>(_rhs); return *this;
}

_log_stream& _log_stream::operator << (int64_t _rhs)
{
	process_stream_in<int64_t>(_rhs); return *this;
}

_log_stream& _log_stream::operator << (uint64_t _rhs)
{
	process_stream_in<uint64_t>(_rhs); return *this;
}

#if defined(_MSC_VER)
_log_stream& _log_stream::operator << (long _rhs)
{
	process_stream_in<long>(_rhs); return *this;
}
_log_stream& _log_stream::operator << (unsigned long _rhs)
{
	process_stream_in<unsigned long>(_rhs); return *this;
}
#else
_log_stream& _log_stream::operator << (long long _rhs)
{
	process_stream_in<long long>(_rhs); return *this;
}

_log_stream& _log_stream::operator << (unsigned long long _rhs)
{
	process_stream_in<unsigned long long>(_rhs); return *this;
}
#endif
_log_stream& _log_stream::operator << (float _rhs)
{
	process_stream_in<float>(_rhs); return *this;
}

_log_stream& _log_stream::operator << (double _rhs)
{
	process_stream_in<double>(_rhs); return *this;
}

_log_stream& _log_stream::operator << (const char* _rhs)
{
	return (_rhs != nullptr) ? operator<<(std::string_view(_rhs)) : *this;
}

_log_stream& _log_stream::operator << (const std::string& _rhs)
{
	return operator<<(std::string_view(_rhs));
}

_log_stream& _log_stream::operator << (std::string&& _rhs)
{
	return operator<<(std::string_view(_rhs));
}

_log_stream& _log_stream::operator << (std::string_view _rhs)
{
	// check)
	RETURN_IF(_rhs.empty(), *this);

#if !defined(_DEBUG) || defined(NDEBUG)
	// check)
	RETURN_IF(plogger == nullptr, *this);
#endif

	// check)
	RETURN_IF(plog_record.empty(), *this);

	// 1) room
	const auto remained = _reserve(*plog_record, _rhs.size());
	auto dest_ptr = _get_tail(*plog_record, pos);

	// 2) copy (cut to fit) + terminator
	const auto length = (_rhs.size() < remained) ? _rhs.size() : remained;
	memcpy(dest_ptr, _rhs.data(), length);
	dest_ptr[length] = 0;

	// 3) size
	_commit(*plog_record, pos, length);

	// return)
	return *this;
}

_log_stream& _log_stream::operator << (void(*_pfunction)(_log_stream&))
{
	_pfunction(*this); return *this;
}

_log_stream& _log_stream::operator << (log_continue_t /*_rhs*/)
{
	// check) 
	CGASSERT_ERROR(plog_record != nullptr);

	// check)
	if (plog_record == nullptr)
		return *this;

	plog_record->type = (plog_record->type | log_type_t::continue_);

	return *this;
}
_log_stream& _log_stream::operator << (log_level_t _rhs)
{
	// check) 
	CGASSERT_ERROR(plog_record != nullptr);

	// check)
	if (plog_record == nullptr)
		return *this;

	plog_record->level = _rhs;

	return *this;
}

_log_stream& _log_stream::operator << (log_attribute_t _rhs)
{
	// check) 
	CGASSERT_ERROR(plog_record != nullptr);

	// check)
	if (plog_record == nullptr)
		return *this;

	plog_record->attribute = _rhs;

	return *this;
}

_log_stream& _log_stream::operator << (log_origin_t _rhs)
{
	// check)
	CGASSERT_ERROR(plog_record != nullptr);

	// check)
	if (plog_record == nullptr)
		return *this;

	plog_record->origin = _rhs;

	return *this;
}

_log_stream& _log_stream::operator << (log_source_t _rhs)
{
	// check)
	CGASSERT_ERROR(plog_record != nullptr);

	// check)
	if (plog_record == nullptr)
		return *this;

	plog_record->source = _rhs;

	return *this;
}

_log_stream& _log_stream::operator << (Ilog_source* _rhs)
{
	// check)
	CGASSERT_ERROR(plog_record != nullptr);

	// check)
	if (plog_record == nullptr)
		return *this;

	plog_record->plog_source = _rhs;

	return *this;
}

_log_stream& _log_stream::operator << (log_destination_t _rhs)
{
	// check)
	CGASSERT_ERROR(plog_record != nullptr);

	// check)
	if (plog_record == nullptr)
		return *this;

	plog_record->destination = _rhs;

	return *this;
}

_log_stream& _log_stream::operator << (const wchar_t* _rhs)
{
	return (_rhs != nullptr) ? operator<<(std::wstring_view(_rhs)) : *this;
}

_log_stream& _log_stream::operator << (const std::wstring& _rhs)
{
	return operator<<(std::wstring_view(_rhs));
}

_log_stream& _log_stream::operator << (std::wstring&& _rhs)
{
	return operator<<(std::wstring_view(_rhs));
}

_log_stream& _log_stream::operator << (std::wstring_view _rhs)
{
	// check)
	if(_rhs.empty())
		return *this;

#if !defined(_DEBUG) || defined(NDEBUG)
	// check)
	if(plogger == nullptr)
		return *this;
#endif

	// check)
	if(plog_record.empty())
		return *this;

	// 1) room (a wchar_t can be converted to 4 bytes at most)
	const auto remained = _reserve(*plog_record, _rhs.size() * __get_convert_ratio(plog_record->char_encoding, eCHARACTER_SET::UCS2));
	auto dest_ptr = _get_tail(*plog_record, pos);

	// 2) convert to the character set of the record (cut to fit, terminator included)
	const auto length = __convert_text(dest_ptr, remained + 1, plog_record->char_encoding, _rhs.data(), _rhs.size(), eCHARACTER_SET::UCS2);

	// 3) size
	_commit(*plog_record, pos, length);

	// return)
	return *this;
}

void log_endl(_log_stream& _log_stream)
{
#if !defined(_DEBUG) || defined(NDEBUG)
	// check)
	if(_log_stream.plogger == nullptr)
		return;
#endif

	// 1) get log record
	auto plog_record = _log_stream.plog_record.get();

	// check)
	if(plog_record == nullptr)
		return;

	// check) room for a new line
	if(_reserve(*plog_record, sizeof(COUNT_T) + sizeof(char)) < sizeof(COUNT_T) + sizeof(char))
		return;

	// 2) increase line count
	++plog_record->buf_message.front<COUNT_T&>();

	// 3) new line
	_log_stream.pos = plog_record->buf_message.size();

	// 4) length = 1 (terminator only)
	//    - the time is set once at the end of the log (_log_end)
	plog_record->buf_message.append<COUNT_T>(1);
	plog_record->buf_message.append<char>(0);
}

void CGOUTPUT_DEBUG_STRING(LOG_RECORD* _plog_record);

_log_stream& _log_end::operator = (_log_stream& _log)
{
	// 1) get log_record
	auto plog_record = std::move(_log.plog_record);

	// 2) output debug Message
#if defined(_DEBUG) && !defined(NDEBUG)
	CGOUTPUT_DEBUG_STRING(plog_record);
#endif

	// 3) get log_target
	auto plogger = std::exchange(_log.plogger, nullptr);

	// check) return if _log.plogger is nullptr
	if(plogger == nullptr)
		return _log;

	// check) nothing was written : an empty message
	if(plog_record.empty())
		return _log;

	// 4) reserve for the empty message (the message is allocated at the first write)
	if (plog_record->buf_message.empty())
		_reserve(*plog_record, 0);

	// 5) set time
	plog_record->time_occurred = chrono::time::utc::now();

	// 6) trace
	plogger->trace(std::move(plog_record));

	// return)
	return _log;
}


}
