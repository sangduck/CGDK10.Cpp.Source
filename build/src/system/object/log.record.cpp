#include "pch.h"
#include "TLOG_RECORD.h"
#include <cstdlib>


namespace CGDK
{

void LOG_RECORD::set_message(const std::string_view _message, eCHARACTER_SET _character_set)
{
	// 1) get size of buffer
	const auto size = (_message.size() + 1) * sizeof(char) + sizeof(COUNT_T) + sizeof(COUNT_T);

	// 2) alloc buffer
	shared_buffer temp_buf_message = alloc_shared_buffer(size);

	// 3) write log line count
	temp_buf_message.append<COUNT_T>(1);

	// 4) write text
	temp_buf_message.append(_message);

	// 5) set shared_buffer
	char_encoding = (_character_set != eCHARACTER_SET::NONE) ? _character_set : _DEFAULT_CHAR_CHARACTER_SET;
	buf_message = std::move(temp_buf_message);
}

void LOG_RECORD::set_message(const std::wstring_view _message, eCHARACTER_SET _character_set)
{
	// 1) get size of buffer
	const auto size = (_message.size() + 1) * sizeof(wchar_t) + sizeof(COUNT_T) + sizeof(COUNT_T);

	// 2) alloc buffer
	shared_buffer temp_buf_message = alloc_shared_buffer(size);

	// 2) write log line count
	temp_buf_message.append<COUNT_T>(1);

	// 3) write text
	temp_buf_message.append(_message);

	// 4) set shared_buffer
	char_encoding = (_character_set != eCHARACTER_SET::NONE) ? _character_set : _DEFAULT_WCHAR_CHARACTER_SET;
	buf_message = std::move(temp_buf_message);
}

void LOG_RECORD::set_message(const std::u16string_view _message, eCHARACTER_SET _character_set)
{
	// 1) get size of buffer
	const auto size = (_message.size() + 1) * sizeof(char16_t) + sizeof(COUNT_T) + sizeof(COUNT_T);

	// 2) alloc buffer
	shared_buffer temp_buf_message = alloc_shared_buffer(size);

	// 2) write log line count
	temp_buf_message.append<COUNT_T>(1);

	// 3) write text
	temp_buf_message.append(_message);

	// 4) set shared_buffer
	char_encoding = (_character_set != eCHARACTER_SET::NONE) ?  _character_set : _DEFAULT_CHAR16_CHARACTER_SET;
	buf_message = std::move(temp_buf_message);
}

void LOG_RECORD::add_message(const std::string_view _message, eCHARACTER_SET _character_set)
{
	// 1) get size of buffer
	const auto target_buffer_size = buf_message.size() + _message.size() * 2;

	// 2) expend shared_buffer
	shared_buffer buf_target = alloc_shared_buffer(target_buffer_size);

	// 3) copy for append
	if (buf_message.size() != 0)
	{
		buf_target.append(buf_message.size(), buf_message.data());
	}
	else
	{
		buf_target.append<COUNT_T>(0);
	}

	// 4) increase line count
	++buf_target.front<COUNT_T&>();

	// 5) append string
	switch(char_encoding)
	{
	case	eCHARACTER_SET::MBCS:
			switch (_character_set)
			{
			case	eCHARACTER_SET::MBCS:
					{
						buf_target.append(_message);
					}
					break;

			case	eCHARACTER_SET::UTF8:
					{
						// - get size
						const auto temp_buffer_size = _message.size() + 64;

						// - alloc temporary shared_buffer
						shared_buffer temp_buffer = alloc_shared_buffer(temp_buffer_size);

						// - 
						std::u8string_view temp_message{ reinterpret_cast<const char8_t*>(_message.data()), temp_message.size() };

						// - target
						wchar_t* temp_dst = reinterpret_cast<wchar_t*>(temp_buffer.data());

						// declare) temporary shared_buffer for converting
						auto str_temp = encode_string(temp_dst, temp_message, temp_buffer_size);

						// - ...
						auto& str_len = buf_target.append<COUNT_T>();

						// - unicode를 mbcs로 변환한다.
						auto len_added = encode_string(buf_target.data() + buf_target.size(), str_temp, buf_target.get_remained_size()).size();
						str_len = static_cast<COUNT_T>(len_added + 1);
						buf_target.add_size(len_added);
						buf_target.append<char8_t>(0);
					}
					break;

			default: 
					CGASSERT_ERROR(false);
					throw throwable::failure(eRESULT::INVALID_ARGUMENT);
			}
			break;

	case	eCHARACTER_SET::UCS2:
			switch (_character_set)
			{
			case	eCHARACTER_SET::MBCS:
					{
						auto& str_len = buf_target.append<COUNT_T>();
						auto len_added = encode_string(reinterpret_cast<wchar_t*>(buf_target.data() + buf_target.size()), _message).size();
						str_len = static_cast<COUNT_T>(len_added + 1);
						buf_target.add_size(len_added * sizeof(wchar_t));
						buf_target.append<wchar_t>(0);
			}
					break;

			case	eCHARACTER_SET::UTF8:
					{
						// - 
						std::u8string_view temp_message{ reinterpret_cast<const char8_t*>(_message.data()), temp_message.size() };

						auto& str_len = buf_target.append<COUNT_T>();
						auto len_added = encode_string(reinterpret_cast<wchar_t*>(buf_target.data() + buf_target.size()), temp_message, buf_target.get_remained_size()).size();
						str_len = static_cast<COUNT_T>(len_added + 1);
						buf_target.add_size(len_added * sizeof(wchar_t));
						buf_target.append<wchar_t>(0);
			}
					break;

			default:
					CGASSERT_ERROR(false);
					throw throwable::failure(eRESULT::INVALID_ARGUMENT);
			}
			break;

	case	eCHARACTER_SET::UTF8:
			switch (_character_set)
			{
			case	eCHARACTER_SET::MBCS:
					{
						// - get size
						const auto temp_buffer_size = _message.size() + 64;

						// - alloc temporary shared_buffer
						shared_buffer temp_buffer = alloc_shared_buffer(temp_buffer_size);

						// - target
						auto* temp_dst = temp_buffer.data<wchar_t>();

						// declare) temporary shared_buffer for converting
						auto str_temp = encode_string(temp_dst, _message, temp_buffer_size);

						// - ...
						auto& str_len = buf_target.append<COUNT_T>();

						// - unicode를 mbcs로 변환한다.
						auto len_added = encode_string(buf_target.data<char8_t>() + buf_target.size(), str_temp, buf_target.get_remained_size()).size();
						str_len = static_cast<COUNT_T>(len_added + 1);
						buf_target.add_size(len_added + sizeof(char8_t));
						buf_target.append<char8_t>(0);
			}
					break;

			case	eCHARACTER_SET::UTF8:
					{
						buf_target.append(_message);
					}
					break;

			default:
					CGASSERT_ERROR(false);
					throw throwable::failure(eRESULT::INVALID_ARGUMENT);
			}
			break;

	default:
			CGASSERT_ERROR(false);
			break;
	}

	// 6) store result
	buf_message = buf_target;
}

void LOG_RECORD::add_message(const std::wstring_view _message, eCHARACTER_SET _character_set)
{
	// 1) get size of buffer
	const auto size = buf_message.size() + _message.size() * 2;

	// 2) expend shared_buffer
	shared_buffer buf_target = alloc_shared_buffer(size);

	// 3) copy for append
	if (buf_message.size() != 0)
	{
		buf_target.append(buf_message.size(), buf_message.data());
	}
	else
	{
		buf_target.append<COUNT_T>(0);
	}

	// 4) increase line count
	++buf_target.front<COUNT_T&>();

	// 5) append string
	switch(char_encoding)
	{
	case	eCHARACTER_SET::MBCS:
			switch (_character_set)
			{
			case	eCHARACTER_SET::UCS2:
					{
						auto& str_len = buf_target.append<COUNT_T>();
						auto len_added = encode_string(buf_target.data() + buf_target.size(), _message, buf_target.get_remained_size()).size();
						str_len = static_cast<COUNT_T>(len_added + 1);
						buf_target.add_size(len_added * sizeof(char));
						buf_target.append<char>(0);
			}
					break;

			default:	CGASSERT_ERROR(false);	throw throwable::failure(eRESULT::INVALID_ARGUMENT);
			}
			break;

	case	eCHARACTER_SET::UCS2:
			switch (_character_set)
			{
			case	eCHARACTER_SET::UCS2:
					{
						buf_target.append(_message);
					}
					break;

			default:	CGASSERT_ERROR(false);	throw throwable::failure(eRESULT::INVALID_ARGUMENT);
			}
			break;

	case	eCHARACTER_SET::UTF8:
			switch (_character_set)
			{
			case	eCHARACTER_SET::UCS2:
					{
						auto& str_len = buf_target.append<COUNT_T>();
						auto len_added = encode_string(buf_target.data<char8_t>() + buf_target.size(), _message, buf_target.get_remained_size()).size();
						str_len = static_cast<COUNT_T>(len_added + 1);
						buf_target.add_size(len_added * sizeof(char8_t));
						buf_target.append<char8_t>(0);
			}
					break;

			default:	CGASSERT_ERROR(false);	throw throwable::failure(eRESULT::INVALID_ARGUMENT);
			}
			break;

	default:
			CGASSERT_ERROR(false);
			break;
	}

	// 6) store result
	buf_message = buf_target;
}
void LOG_RECORD::add_message(const std::u16string_view /*_message*/, eCHARACTER_SET /*_character_set*/)
{
	CGASSERT_ERROR(false);
}

size_t LOG_RECORD::get_line_count() const noexcept
{
	// check)
	RETURN_IF(buf_message.data() == nullptr, 0)

	// check)
	RETURN_IF(buf_message.size() < sizeof(COUNT_T), 0)

	// return) 
	return static_cast<size_t>(buf_message.front<COUNT_T>());
}

void LOG_RECORD::stream_out(std::ostream& _stream_out, eCHARACTER_SET _character_set) const
{
	// 1) log type
	bool log_not_continue = (uint32_t)(type & log_type_t::continue_) == 0;

	// 2) alloc shared_buffer for log text
	auto buf_text_date = alloc_shared_buffer(64);
	auto buf_text_log = alloc_shared_buffer(process_buffer_size(_character_set) + 64);

	// 3) 먼저 시간을 출력한다.
	if (log_not_continue)
	{
		// - Text를 먼저 append한다.
		process_append_text_date(buf_text_date, _character_set);
	}

	// 4) 메시지를 작성한다.
	process_append_to_buffer(buf_text_log, _character_set);

	// 5) CR을 붙인다.
	buf_text_log.append("\n");
	buf_text_log.append<char>(0);

	// 6) 출력한다.
	{
		// - shared_buffer
		shared_buffer temp_src = buf_text_log;

		// - line수를 읽어들인다.
		auto line_count = temp_src.extract<COUNT_T>();

		for(auto i=0; i< line_count; ++i)
		{
			// - 날짜와 시간을 출력한다.
			if (log_not_continue && i==0)
			{
				// - 출력한다.
				_stream_out << buf_text_date.data<const char>();
			}
			else
			{
				// - 빈칸을 출력한다.
				//           "[0000-00-00 00:00:00] "
				_stream_out << "                      ";
			}

			// - text를 읽어들인다.
			std::string_view temp_text = temp_src.extract<std::string_view>();

			_stream_out << temp_text.data();

			// - CR을 붙인다.
			_stream_out << std::endl;
		}
	}
}

object_ptr<LOG_RECORD> LOG_RECORD::clone(eCHARACTER_SET _character_set) const
{
	// 1) 객체를 생성한다.
	auto plog_record = make_object<LOG_RECORD>();

	// 2) 값을 복사한다.
	plog_record->type = type;
	plog_record->level = level;
	plog_record->result = result;
	plog_record->origin = origin;
	plog_record->attribute = attribute;
	plog_record->source = source;
	plog_record->destination = destination;
	plog_record->time_occure = time_occure;

	// declare)
	auto temp_char_encoding = (_character_set != eCHARACTER_SET::NONE) ? _character_set : char_encoding;

	// 3) Message를 변경한다.
	if (temp_char_encoding == char_encoding )
	{
		plog_record->char_encoding = char_encoding;
		plog_record->buf_message = buf_message;
	}
	else
	{
		// declare) 
		shared_buffer temp_buf_message = alloc_shared_buffer(buf_message.size() * 4 + 64); ;;

		// - 변환해 넣는다.
		switch (temp_char_encoding)
		{
		case	eCHARACTER_SET::MBCS:
				{
					switch (char_encoding)
					{
					case	eCHARACTER_SET::UCS2:
							__convert_unicode_to_mbcs(temp_buf_message, buf_message);
							break;

					case	eCHARACTER_SET::UTF8:
							__convert_utf8_to_mbcs(temp_buf_message, buf_message);
							break;

					default:
							CGASSERT_ERROR(false);
							break;
					}
				}
				break;

		case	eCHARACTER_SET::UCS2:
				{
					switch (char_encoding)
					{
					case	eCHARACTER_SET::MBCS:
							__convert_mbcs_to_unicode(temp_buf_message, buf_message);
							break;

					case	eCHARACTER_SET::UTF8:
							__convert_utf8_to_unicode(temp_buf_message, buf_message);
							break;

					case	 eCHARACTER_SET::UCS2:
							CGASSERT_ERROR(false);
							break;

					default:
							CGASSERT_ERROR(false);
							break;
					}
				}
				break;

		case	eCHARACTER_SET::UTF8:
				{
					switch (char_encoding)
					{
					case	 eCHARACTER_SET::MBCS:
							__convert_mbcs_to_utf8(temp_buf_message, buf_message);
							break;

					case	eCHARACTER_SET::UCS2:
							__convert_unicode_to_utf8(temp_buf_message, buf_message);
							break;

					default:
							CGASSERT_ERROR(false);
							break;
					}
				}
				break;

		default:
				CGASSERT_ERROR(false);
				break;
		}

		// - shrink to fit
		plog_record->char_encoding = char_encoding;
		plog_record->buf_message = alloc_shared_buffer(temp_buf_message.size());
		plog_record->buf_message.append(temp_buf_message.size(), temp_buf_message.data());
	}

	// 4) Child도 동일하게 처리한다.
	for (auto& iter : sub_plog)
	{
		if (iter->char_encoding == _character_set)
		{
			plog_record->sub_plog.push_back(iter);
		}
		else
		{
			plog_record->sub_plog.push_back(iter->clone(_character_set));
		}
	}

	// return) 
	return	plog_record;
}

size_t LOG_RECORD::process_buffer_size(eCHARACTER_SET _character_set) const noexcept
{
	// check)
	RETURN_IF(buf_message.size() < sizeof(COUNT_T), 32);

	// 1) copy shared_buffer
	buffer_view buf_temp = buf_message;

	// 2) read line count
	auto count_line = static_cast<size_t>(buf_temp.extract<COUNT_T>());

	// declare)
	auto buffer_size = sizeof(COUNT_T);


	for(; count_line>0; --count_line)
	{
		// - get string lenbght (include NULL)
		auto message_length = static_cast<size_t>(buf_temp.extract<COUNT_T>());

		// - string extract
		switch (char_encoding)
		{
		case	 eCHARACTER_SET::MBCS:
		case	eCHARACTER_SET::UTF8:
				buf_temp += offset(message_length);
				break;
		case	eCHARACTER_SET::UCS2:
				buf_temp += offset(message_length * sizeof(wchar_t));
				break;
		default:
				break;
		}

		// - message length
		buffer_size += sizeof(COUNT_T);

		switch (_character_set)
		{
		case	 eCHARACTER_SET::MBCS:
				buffer_size	+= message_length + sizeof(char);
				break;

		case	eCHARACTER_SET::UTF8:
				buffer_size	+= message_length * 4 + sizeof(char);
				break;

		case	eCHARACTER_SET::UCS2:
				buffer_size	+= (message_length + 1) * sizeof(wchar_t);
				break;

		default:
				CGASSERT_ERROR(false);	
				buffer_size	+= message_length * 4 + 1;	// 정의되지 않았을 때는 최대 크기로
				break;
		}
	}

	// return)
	return buffer_size;
}


void LOG_RECORD::process_append_text_date(shared_buffer& _buf_dst, eCHARACTER_SET _character_set) const
{
	switch (_character_set)
	{
	case	 eCHARACTER_SET::MBCS:
	case	eCHARACTER_SET::UTF8:
			{
				auto time_temp = chrono::to_date_time(time_occure);

				_buf_dst.append_text("[{:4d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}] ",
					time_temp.t_year,
					time_temp.t_month,
					time_temp.t_day,
					time_temp.t_hour,
					time_temp.t_minute,
					time_temp.t_second
				);
			}
			break;

	case	eCHARACTER_SET::UCS2:
			{
				auto time_temp = chrono::to_date_time(time_occure);

				_buf_dst.append_text<wchar_t>(L"[{:4d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}] ",
					time_temp.t_year,
					time_temp.t_month,
					time_temp.t_day,
					time_temp.t_hour,
					time_temp.t_minute,
					time_temp.t_second
				);
			}
			break;

	default: CGASSERT_ERROR(false); break; 
	}
}

void LOG_RECORD::process_append_to_buffer(shared_buffer& _buf_dst, eCHARACTER_SET _character_set) const
{
	switch (_character_set)
	{
	case	eCHARACTER_SET::MBCS:	process_append_to_buffer_mbcs	(_buf_dst); break;
	case	eCHARACTER_SET::UCS2:	process_append_to_buffer_windows_unicode(_buf_dst); break;
	case	eCHARACTER_SET::UTF8:	process_append_to_buffer_utf8	(_buf_dst); break;
	default:						CGASSERT_ERROR(false); break;
	}
}

void LOG_RECORD::process_append_to_buffer_mbcs(shared_buffer& _buf_dst) const
{
	switch(char_encoding)
	{
	case	 eCHARACTER_SET::MBCS:
			_buf_dst.append(buf_message.size(), buf_message.data());
			break;

	case	eCHARACTER_SET::UCS2:
			__convert_unicode_to_mbcs(_buf_dst, buf_message);
			break;

	case	eCHARACTER_SET::UTF8:
			__convert_utf8_to_mbcs(_buf_dst, buf_message);
			break;

	default:
			CGASSERT_ERROR(false);
			break;
	}
}

void LOG_RECORD::process_append_to_buffer_windows_unicode(shared_buffer& _buf_dst) const
{
	switch(char_encoding)
	{
	case	 eCHARACTER_SET::MBCS:
			__convert_mbcs_to_unicode(_buf_dst, buf_message);
			break;

	case	eCHARACTER_SET::UCS2:
			_buf_dst.append(buf_message.size(), buf_message.data());
			break;

	case	eCHARACTER_SET::UTF8:
			__convert_utf8_to_unicode(_buf_dst, buf_message);
			break;

	default:
			CGASSERT_ERROR(false);
			break;
	}
}

void LOG_RECORD::process_append_to_buffer_utf8(shared_buffer& _buf_dst) const
{
	switch(char_encoding)
	{
	case	 eCHARACTER_SET::MBCS:
			__convert_mbcs_to_utf8(_buf_dst, buf_message);
			break;

	case	eCHARACTER_SET::UCS2:
			__convert_unicode_to_utf8(_buf_dst, buf_message);
			break;

	case	eCHARACTER_SET::UTF8:
			_buf_dst.append(buf_message.size(), buf_message.data());
			break;

	default:
			CGASSERT_ERROR(false);
			break;
	}
}

size_t LOG_RECORD::process_get_message(shared_buffer& _buf_dst, size_t _pos, eCHARACTER_SET _character_set) const
{
	//declare) 
	size_t size_result;

	switch (char_encoding)
	{
	case	eCHARACTER_SET::MBCS:	size_result = process_get_message_from_mbcs (_buf_dst, _pos, _character_set); break;
	case	eCHARACTER_SET::UCS2:	size_result = process_get_message_from_windows_unicode (_buf_dst, _pos, _character_set); break;
	case	eCHARACTER_SET::UTF8:	size_result = process_get_message_from_utf8 (_buf_dst, _pos, _character_set); break;
	default:						size_result = 0; CGASSERT_ERROR(false);	break;
	}

	// return)
	return size_result;
}

size_t LOG_RECORD::process_get_message_from_mbcs(shared_buffer& _buf_dst, size_t _pos, eCHARACTER_SET _character_set) const
{
	//declare) 
	shared_buffer buf_src = buf_message + offset(_pos);
	shared_buffer buf_dst = _buf_dst;

	// 2) get source string
	std::string_view str_src = buf_src.extract<std::string_view>();

	// 3) append string
	switch (_character_set)
	{
	case	eCHARACTER_SET::MBCS:
			buf_dst.append_text(str_src);
			break;

	case	eCHARACTER_SET::UCS2:
			buf_dst.add_size(encode_string(reinterpret_cast<wchar_t*>(buf_dst.data() + buf_dst.size()), str_src, buf_dst.get_remained_size()).size() * sizeof(wchar_t));
			break;

	case	eCHARACTER_SET::UTF8:
			{
				// declare) 
				const size_t size_buf = buf_message.size() * 3 + 64;

				// - alloc temporary shared_buffer
				shared_buffer temp_buffer = alloc_shared_buffer(size_buf);

				// declare) temporary shared_buffer for converting
				auto str_temp = encode_string(reinterpret_cast<wchar_t*>(temp_buffer.data()), str_src, size_buf/sizeof(wchar_t));

				// - convert
				auto str_result = encode_string(buf_dst.data<char8_t>() + buf_dst.size(), str_temp, buf_dst.get_remained_size());

				// - add size
				buf_dst.add_size(str_result.size());
			}
			break;

	default:
			CGASSERT_ERROR(false);
			break;
	}

	// 4) set size_result
	_buf_dst = buf_dst;

	// return)
	return buf_src.data() - buf_message.data();
}

size_t LOG_RECORD::process_get_message_from_windows_unicode(shared_buffer& _buf_dst, size_t _pos, eCHARACTER_SET _character_set) const
{
	// 1) get shared_buffer
	auto buf_src = buf_message + offset(_pos);
	shared_buffer buf_dst = _buf_dst;

	// 2) get source string
	auto str_src = buf_src.extract<std::wstring_view>();

	// 3) append string
	switch (_character_set)
	{
	case	eCHARACTER_SET::MBCS:
			buf_dst.add_size(encode_string(buf_dst.data() + buf_dst.size(), str_src, buf_dst.get_remained_size()).size());
			break;

	case	eCHARACTER_SET::UCS2:
			buf_dst.append_text(str_src);
			break;

	case	eCHARACTER_SET::UTF8:
			buf_dst.add_size(encode_string(buf_dst.data<char8_t>() + buf_dst.size(), str_src).size());
			break;

	default:
		CGASSERT_ERROR(false);
		break;
	}

	// 4) set size_result
	_buf_dst = buf_dst;

	// return)
	return buf_src.data() - buf_message.data();
}

size_t LOG_RECORD::process_get_message_from_utf8(shared_buffer& _buf_dst, size_t _pos, eCHARACTER_SET _character_set) const
{
	// 1) get shared_buffer
	auto buf_src = buf_message + offset(_pos);
	auto buf_dst = _buf_dst;

	// 2) get source string
	auto str_src = buf_src.extract<std::u8string_view>();

	// 3) append string
	switch (_character_set)
	{
	case	eCHARACTER_SET::MBCS:
			{
				// - alloc temporary shared_buffer
				auto temp_buffer = alloc_shared_buffer(buf_message.size() + 64);

				// - target
				auto temp_dst = temp_buffer.data<wchar_t>();

				// declare) temporary shared_buffer for converting
				auto str_temp = encode_string(temp_dst, str_src);

				// - unicode를 mbcs로 변환한다.
				buf_dst.add_size(encode_string(buf_dst.data() + buf_dst.size(), str_temp, buf_dst.get_remained_size()).size());
			}
			break;

	case	eCHARACTER_SET::UCS2:
			buf_dst.add_size(encode_string(reinterpret_cast<wchar_t*>(buf_dst.data() + buf_dst.size()), str_src).size() * sizeof(wchar_t));
			break;

	case	eCHARACTER_SET::UTF8:
			buf_dst.append_text(str_src);
			break;

	default:
			CGASSERT_ERROR(false);
			break;
	}

	// 4) set size_result
	_buf_dst = buf_dst;

	// return)
	return buf_src.data() - buf_message.data();
}

size_t LOG_RECORD::process_get_message_from_ucs16(shared_buffer& /*_buf_dst*/, size_t /*_pos*/, eCHARACTER_SET /*_character_set*/) const
{
	// error)
	CGASSERT_ERROR(false);

	// return)
	return 0;
}

object_ptr<LOG_RECORD> LOG_RECORD::_alloc()
{
	return TLOG_RECORD::_alloc();
}

sMESSAGE_LOG::sMESSAGE_LOG(log_type_t _type, log_level_t _level, eCHARACTER_SET _character_set, const std::string_view _message) : sMESSAGE(eMESSAGE::SYSTEM::LOG_)
{
	try
	{
		// 1) allocate new LOG_RECORD object
		auto temp_plog_record = LOG_RECORD::_alloc();

		// 2) set log info
		temp_plog_record->type = _type & 0xffff;
		temp_plog_record->level = _level;
		temp_plog_record->set_message(_message, _character_set);
		temp_plog_record->time_occure = chrono::time::utc::now();

		// 3) Store LOG_RECORD object
		plog_record = std::move(temp_plog_record);
	}
	catch (...)
	{
	}
}

sMESSAGE_LOG::sMESSAGE_LOG(log_type_t _type, log_level_t _level, eCHARACTER_SET _character_set, const std::string_view _message, const chrono::time::utc::time_point& _time_occure) : sMESSAGE(eMESSAGE::SYSTEM::LOG_)
{
	try
	{
		// 1) allocate new LOG_RECORD object
		auto temp_plog_record = LOG_RECORD::_alloc();

		// 2) set log info
		temp_plog_record->type = _type & 0xffff;
		temp_plog_record->level = _level;
		temp_plog_record->set_message(_message, _character_set);
		temp_plog_record->time_occure = _time_occure;

		// 3) Store LOG_RECORD object
		plog_record = std::move(temp_plog_record);
	}
	catch (...)
	{
	}
}


}