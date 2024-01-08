#include "pch.h"
#include "TLOG_RECORD.h"


namespace CGDK
{

_log_stream::_log_stream()
{
	// definitions) 
	const size_t _BUFFER_LENGTH = MAX_TRACE_STRING_LENGTH;

	try
	{
		// 1) shared_buffer를 생성한다.
		shared_buffer temp_message_buffer = alloc_shared_buffer(_BUFFER_LENGTH);
		temp_message_buffer.append<COUNT_T>(1);
		temp_message_buffer.append<COUNT_T>(1);
		temp_message_buffer.append<char>(0);

		// 2) 생성한다.
		auto temp_log_record = LOG_RECORD::_alloc();

		// 3) 값을 설정한다
		temp_log_record->buf_message = std::move(temp_message_buffer);

		// 4) 설정...
		plog_record	 = std::move(temp_log_record);
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

	plogger = (_plogger != nullptr) ? _plogger : get_default_logger().get();

	plog_record->type = _type;
}

_log_stream::_log_stream(log_type_t _type) : _log_stream()
{
	// check)
	RETURN_IF(plog_record.empty());

	plogger = get_default_logger();

	plog_record->type = _type;
}

_log_stream& _log_stream::operator << (const std::string_view _rhs)
{
	// check) return if _rhs is empty
	RETURN_IF(_rhs.empty(), *this);

#if !defined(_DEBUG) || defined(NDEBUG)
	// check) return if plog_record is nullptr
	RETURN_IF(plogger.empty(), *this);
#endif

	// check) return if plog_record is nullptr
	RETURN_IF(plog_record.empty(), *this);

	// 1) 설정한다.
	auto dest_strlen = plog_record->buf_message.front<COUNT_T>(pos);
	auto dest_remained = plog_record->buf_message.get_remained_size();
	auto dest_ptr = plog_record->buf_message.data() + pos + dest_strlen - sizeof(char) + sizeof(COUNT_T);

	auto source_ptr = _rhs.data();
	auto source_length = _rhs.size();

	// check) 
	RETURN_IF(dest_remained == 0, *this)

	// check) 
	if (source_length > dest_remained)
	{
		source_length = dest_remained;
	}

	// 2) ...
	memcpy(dest_ptr, source_ptr, source_length + 1);

	// 3) 크기 써넣기
	plog_record->buf_message.front<COUNT_T&>(pos) = static_cast<COUNT_T>(dest_strlen + source_length);
	plog_record->buf_message.add_size(source_length);

	// return) 
	return *this;
}

_log_stream& _log_stream::operator << (const std::wstring_view _rhs)
{
	// check) return if _rhs is empty
	RETURN_IF(_rhs.empty(), *this);

#if !defined(_DEBUG) || defined(NDEBUG)
	// check) 
	RETURN_IF(plogger.empty(), *this);
#endif

	// check) return if plog_record is nullptr
	RETURN_IF(plog_record.empty(), *this);

	// 1) 설정한다.
	auto dest_strlen = plog_record->buf_message.front<COUNT_T>(pos);
	auto dest_remained = plog_record->buf_message.get_remained_size();
	auto dest_ptr = plog_record->buf_message.data() + pos + dest_strlen - sizeof(char) + sizeof(COUNT_T);

	// 2) ...
	auto temp_result = encode_string(dest_ptr, _rhs, dest_remained);

	// 3) 크기 써넣기
	plog_record->buf_message.front<COUNT_T&>(pos) = static_cast<COUNT_T>(dest_strlen + temp_result.size());
	plog_record->buf_message.add_size(temp_result.size());

	// return) 
	return *this;
}

void log_endl(_log_stream& _log_stream)
{
#if !defined(_DEBUG) || defined(NDEBUG)
	// check) 
	RETURN_IF(_log_stream.plogger.empty());
#endif

	// 1) get log record
	auto plog_record = _log_stream.plog_record.get();

	// check) 
	RETURN_IF(plog_record == nullptr);

	// 1) increase message_line
	++plog_record->buf_message.front<COUNT_T&>();

	// 2) add new line
	_log_stream.pos = plog_record->buf_message.size();

	// 3) set time
	plog_record->time_occure = chrono::time::utc::now();

	// 4) setline count = 1 and add NULL to string 
	plog_record->buf_message.append<COUNT_T>(1);
	plog_record->buf_message.append<char>(0);
}

void CGOUTPUT_DEBUG_STRING(LOG_RECORD* _plog_record);

void _log_end::operator = (_log_stream& _log)
{
	// 1) get log_record
	auto plog_record = std::move(_log.plog_record);

	// 2) output debug Message
#if defined(_DEBUG) && !defined(NDEBUG)
	CGOUTPUT_DEBUG_STRING(plog_record);
#endif

	// 3) get log_target
	auto plogger = std::move(_log.plogger);

	// check) return if _log.plogger is nullptr
	RETURN_IF(plogger.empty());

	// 4) set time
	plog_record->time_occure = chrono::time::utc::now();

	// 5) Messageable이 존재하면 trace Dispatch
	plogger->trace(plog_record);
}


}