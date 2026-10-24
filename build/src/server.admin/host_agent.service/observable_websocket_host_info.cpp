#include "pch.h"
#include <charconv>

void _append_key_value(shared_buffer& _buf_target, std::string_view _str_key, std::string_view _str_value);
void _append_key_value(shared_buffer& _buf_target, std::string_view _str_key, std::size_t _value);

// FOREGROUND
#define CONSOLE_FORE_COLOR_BLACK			"black"sv		//  0: black		#000000 rgb(0,0,0)
#define CONSOLE_FORE_COLOR_RED				"red"sv			//  4:red			#FF0000 rgb(255,0,0)
#define CONSOLE_FORE_COLOR_GREENN			"green"sv		//  2:green			#008000 rgb(0,128,0)
#define CONSOLE_FORE_COLOR_YELLOW			"yellow"sv		//  6:yellow		#FFFF00 rgb(255,255,0)
#define CONSOLE_FORE_COLOR_BLUE				"blue"sv		//  1: blue			#0000FF rgb(0,0,255)
#define CONSOLE_FORE_COLOR_MAGENTA			"magenta"sv		//  5:magenta		#FF00FF rgb(255,0,255)
#define CONSOLE_FORE_COLOR_CYAN				"cyan"sv		//  3:cyan			#00FFFF rgb(0,255,255)
#define CONSOLE_FORE_COLOR_LIGHT_GRAY		"lightgray"sv	//  7:light gray	#D3D3D3 rgb(211,211,211)
#define CONSOLE_FORE_COLOR_DARK_GRAY		"darkgray"sv	//  8:dark gray		#A9A9A9 rgb(169,169,169)
#define CONSOLE_FORE_COLOR_LIGHT_RED		"indianred"sv	// 12:light red		#CD5C5C rgb(205,92,92)
#define CONSOLE_FORE_COLOR_LIGHT_GREEN		"lightgreen"sv	// 10:light green	#90EE90 rgb(144,238,144) 
#define CONSOLE_FORE_COLOR_LIGHT_YELLOW		"lightyellow"sv	// 14:light yellow	#FFFFE0	rgb(255,255,224)
#define CONSOLE_FORE_COLOR_LIGHT_BLUE		"lightblue"sv	//  9:light blue	#ADD8E6 rgb(173,216,230)
#define CONSOLE_FORE_COLOR_LIGHT_MAGENTA	"violet"sv		// 13:light magenta	#EE82EE	rgb(238,130,238)
#define CONSOLE_FORE_COLOR_LIGHT_CYAN		"lightcyan"sv	// 11:light cyan	#E0FFFF rgb(224,255,255)
#define CONSOLE_FORE_COLOR_WHITE			"white"sv		// 15:white			#FFFFFF rgb(255,255,255)

#define CONSOLE_FORE_COLOR_DARK_RED			"darkred"sv		// dark red			#8B0000	rgb(139,0,0)
#define CONSOLE_FORE_COLOR_GOLD				"gold"sv		// gold				#FFD700 rgb(255,215,0)

const std::string_view get_web_text_color(log_type_t _log_type, int /*_log_level*/)
{
	// declare)
	std::string_view text_color;

	// - select text color
	switch (_log_type)
	{
	case	log_type_t::info:
			text_color = CONSOLE_FORE_COLOR_BLACK;
			break;

	case	log_type_t::progress:
			text_color = CONSOLE_FORE_COLOR_GREENN;
			break;

	case	log_type_t::debug:
			text_color = CONSOLE_FORE_COLOR_LIGHT_GRAY;
			break;

	case	log_type_t::fatal:
			text_color = CONSOLE_FORE_COLOR_RED;
			break;

	case	log_type_t::error:
			text_color = CONSOLE_FORE_COLOR_RED;
			break;

	case	log_type_t::warning:
			text_color = CONSOLE_FORE_COLOR_LIGHT_RED;
			break;

	case	log_type_t::user:
			text_color = CONSOLE_FORE_COLOR_GOLD;
			break;

	case	log_type_t::system:
			text_color = CONSOLE_FORE_COLOR_BLUE;
			break;

	default:
			text_color = CONSOLE_FORE_COLOR_BLACK;
			break;
	}

	// return)
	return text_color;
}

void _append_log_info(shared_buffer& _buf_out, const LOG_RECORD* _plog_record)
{
	// check) message must be not empty
	RETURN_IF(_plog_record->buf_message.empty());

	// 1) log type
	auto log_type = _plog_record->type & 0xffff;
	auto log_level = _plog_record->level;
	bool log_not_continue = (uint32_t)(_plog_record->type & log_type_t::continue_) == 0;

	size_t pos = sizeof(COUNT_T);
	size_t count_line = _plog_record->line_count();

	// check)
	CGASSERT_ERROR(count_line != 0);

	// check) 
	RETURN_IF(count_line == 0);

	// 2) write first line
	{
		if (log_not_continue)
		{
			// - style
			_buf_out.append_text("<span style='color: silver;font-size:12px;'>"sv);

			// - output data-time
			LOG_RECORD::append_text_date(_buf_out, _plog_record->time_occurred, eCHARACTER_SET::UTF8);

			// - stype
			_buf_out.append_text("</span>"sv);
		}
		else
		{
			// - output space
			_buf_out.append_text("<pre style='display:inline;font-size:12px;'>                     </pre>"sv);
		}

		// - style
		_buf_out.append_text<char>("<span>"sv);
		_buf_out.append_text<char>("<pre style='display:inline;font-size:12px;color:"sv);
		_buf_out.append_text<char>(get_web_text_color(log_type, log_level));
		if (log_level >= log_level_t::higher() || log_type == log_type_t::fatal) _buf_out.append_text<char>(";font-weight:bold"sv);
		_buf_out.append_text<char>(";'>"sv);

		// - 메시지를 작성한다.
		pos = _plog_record->get_log_message(_buf_out, eCHARACTER_SET::UTF8, pos);

		// - stype
		_buf_out.append_text("</pre></span>");
	}

	// 3) 출력한다.
	for (--count_line; count_line != 0; --count_line)
	{
		// - newline + style(color)
		_buf_out.append_text("<br>"sv);

		// - output space
		_buf_out.append_text("<pre style='display:inline;font-size:12px;'>                     </pre>"sv);

		// - style
		_buf_out.append_text<char>("<span>"sv);
		_buf_out.append_text<char>("<pre style='display:inline;font-size:12px;color:"sv);
		_buf_out.append_text<char>(get_web_text_color(log_type, log_level));
		if (log_level >= log_level_t::higher() || log_type == log_type_t::fatal) _buf_out.append_text<char>(";font-weight:bold"sv);
		_buf_out.append_text<char>(";'>"sv);

		// - get next message
		pos = _plog_record->get_log_message(_buf_out, eCHARACTER_SET::UTF8, pos);

		// - stype
		_buf_out.append_text("</pre></span>"sv);
	}
}
//----------------------------------------------------------------------------
// instance 
//----------------------------------------------------------------------------
#define MAX_CANVAS_WITH_C 64

void _append_service_instance_status_setting(buffer& _buf_dest, const server_system::instance_service* _pinstance_service)
{
	const auto& setting = _pinstance_service->m_instance.info.setting;

	// 2) "Name"
	_buf_dest.append_text<char>("<td><span style='font-size:15px;font-weight:bold'><i class='fa fa-server'></i> "sv);
	_buf_dest.append_text<char>(setting.name);
	_buf_dest.append_text<char>("</span></td>"sv);

	// 3) TYPE/UID
	_buf_dest.append_text<char>("<td>"sv);
	_buf_dest.append_text<char>(setting.rank.role);
	_buf_dest.append_text<char>("<br><span style='font-size:11px'>"sv);
	_buf_dest.append_text<char>(setting.iid);
	_buf_dest.append_text<char>("</span></td>"sv);

	// 4) REVISION/VERSION
	_buf_dest.append_text<char>("<td>"sv);
	_buf_dest.append_text<char>("<i class='xi-git-symbol'></i>"sv);
	_buf_dest.append_text<char>(setting.revision.revision);
	_buf_dest.append_text<char>(" <i class='fa fa-caret-down'></i><br><span style='font-size:11px'>"sv);
	if (_pinstance_service->m_instance.info.status.state != eOBJECT_STATE::NONE)
	{
		_buf_dest.append_text<char>(CGDK::stream_string(setting.version.major, ".", setting.version.minor, ".", setting.version.build));
	}
	_buf_dest.append_text<char>("</span></td>"sv);
}
void _append_service_instance_status_status_open(buffer& _buf_dest, const server_system::instance_service* _pinstance_service)
{
	// 1) get instance
	auto id_instance = reinterpret_cast<uint64_t>(static_cast<const server_system::Iinstance*>(_pinstance_service));

	// 2) get info
	const auto& status = _pinstance_service->m_instance.info.status;

	// 3) STATUS
	_buf_dest.append_text<char>(CGDK::stream_string("<td id='hi_s", id_instance, "'>"));
	_buf_dest.append_text<char>(to_string(status.state));
	_buf_dest.append_text<char>("</td>"sv);

	// 4) PROCESSOR
	_buf_dest.append_text<char>("<td>");
	_buf_dest.append_text<char>(CGDK::stream_string("<canvas class='table_server_list_canvas' width='128px' height='100' id='hi_cv1", id_instance, "'>"));
	_buf_dest.append_text<char>("</canvas>"sv);

	// 5) MEMORY
	_buf_dest.append_text<char>("<td>");
	_buf_dest.append_text<char>(CGDK::stream_string("<canvas class='table_server_list_canvas' width='128px' height='100' id='hi_cv2", id_instance, "'>"));
	_buf_dest.append_text<char>("</canvas>"sv);

	// 6) SESSION
	_buf_dest.append_text<char>("<td>"sv);
	_buf_dest.append_text<char>(CGDK::stream_string("<canvas class='table_server_list_canvas' width='128px' height='100' id='hi_cv3", id_instance, "'>"));
	_buf_dest.append_text<char>("</canvas>"sv);
	_buf_dest.append_text<char>("</td>"sv);

	// 7) NETWORK
	_buf_dest.append_text<char>("<td>"sv);
	_buf_dest.append_text<char>(CGDK::stream_string("<canvas class='table_server_list_canvas' width='128px' height='100' id='hi_cv4", id_instance, "'>"));
	_buf_dest.append_text<char>("</canvas>"sv);
	_buf_dest.append_text<char>("</td>"sv);
}
void _append_service_instance_status_status_close(buffer& _buf_dest, const server_system::instance_service* _pinstance_service)
{
	// 5) get instance
	auto id_instance = reinterpret_cast<uint64_t>(static_cast<const server_system::Iinstance*>(_pinstance_service));

	// 6) STATUS
	_buf_dest.append_text<char>(CGDK::stream_string("<td id='hi_s", id_instance, "'>"));
	_buf_dest.append_text<char>("</td>"sv);

	// 7) PROCESSOR
	_buf_dest.append_text<char>("<td>");
	_buf_dest.append_text<char>("</td>"sv);

	// 8) MEMORY
	_buf_dest.append_text<char>("<td>");
	_buf_dest.append_text<char>("</td>"sv);

	// 9) SESSION
	_buf_dest.append_text<char>("<td>"sv);
	_buf_dest.append_text<char>("</td>"sv);

	// 10) NETWORK
	_buf_dest.append_text<char>("<td>"sv);
	_buf_dest.append_text<char>("</td>"sv);
}
void _append_service_instance_status_info(buffer& _buf_dest, const server_system::instance_service* _pinstance_service)
{
	// - get info
	const auto& setting = _pinstance_service->m_instance.info.setting;

	// - PATHW
	_buf_dest.append_text<char>("<td>"sv);
	_buf_dest.append_text<char>(setting.path);
	_buf_dest.append_text<char>("</td>"sv);
}
void _append_service_instance_status_close(buffer& _buf_dest, const server_system::instance_service* _pinstance_service)
{
	// 1) setting
	_append_service_instance_status_setting(_buf_dest, _pinstance_service);

	// 2) status
	_append_service_instance_status_status_close(_buf_dest, _pinstance_service);

	// 3) info
	_append_service_instance_status_info(_buf_dest, _pinstance_service);
}
void _append_service_instance_status_open(buffer& _buf_dest, const server_system::instance_service* _pinstance_service)
{
	// 1) setting
	_append_service_instance_status_setting(_buf_dest, _pinstance_service);

	// 2) status
	_append_service_instance_status_status_open(_buf_dest, _pinstance_service);

	// 3) info
	_append_service_instance_status_info(_buf_dest, _pinstance_service);
}

shared_buffer _make_webmessage_service_instance_add(const server_system::Iinstance* _pinstance)
{
	// 1) buffer
	auto buf_result = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE * 4);

	// 2) append message head
	buf_result.append_text<char>(MESSAGE_WEB_SERVER_LIST); // "SELI"

	// 3) append message 
	{
		// - temporary buffer
		auto buf_text = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE * 4);

		// - instance service
		auto pinstance_service = dynamic_cast<const server_system::instance_service*>(_pinstance);

		// - append tr data
		if (pinstance_service->m_instance.info.status.state != eOBJECT_STATE::NONE)
			_append_service_instance_status_open(buf_text, pinstance_service);
		else
			_append_service_instance_status_close(buf_text, pinstance_service);

		// - append command (A:Add, D:delete, U:update)
		buf_result.append<char>('A');

		// - tr element id
		auto tr_element_id = CGDK::stream_string("tr", reinterpret_cast<uint64_t>(_pinstance));

		// - append data
		::_append_key_value(buf_result, tr_element_id, buf_text);
	}

	// 4) send message
	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_result.to_string<char>()));
}
shared_buffer _make_webmessage_service_instance_remove(const server_system::Iinstance* _pinstance)
{
	// 1) get instance 
	object_ptr<service> pservice = ::service::instance();

	// 2) buffer
	auto buf_result = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);
	auto buf_temp = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

	// 3) append message head
	buf_result.append_text<char>(MESSAGE_WEB_SERVER_LIST); // "SELI"

	// 4) append message 
	{
		// - append command (A:Add, D:delete, U:update)
		buf_result.append<char>('D');

		// - tr node name
		auto tr_element_id = CGDK::stream_string("tr", reinterpret_cast<uint64_t>(_pinstance));

		// - append tr_element_id
		buf_result.append_text<char>(tr_element_id);
		buf_result.append<char>('\n');
	}

	// 5) send message
	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_result.to_string<char>()));
}
shared_buffer _make_webmessage_service_instance_update_open(server_system::Iinstance* _pinstance, uint64_t /*_flag*/)
{
	// 1) buffer
	auto buf_result = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

	// 2) append message head
	buf_result.append_text<char>(MESSAGE_WEB_SERVER_LIST); // "SELI"

	// 3) append message 
	{
		// - temporary buffer
		auto buf_text = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

		// - append tr data
		_append_service_instance_status_open(buf_text, dynamic_cast<server_system::instance_service*>(_pinstance));

		// - append command (A:Add, D:delete, U:update)
		buf_result.append<char>('U');

		// - tr element id
		auto tr_element_id = CGDK::stream_string("tr", reinterpret_cast<uint64_t>(_pinstance));

		// - append data
		::_append_key_value(buf_result, tr_element_id, buf_text);
	}

	// 4) send message
	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_result.to_string<char>()));
}
shared_buffer _make_webmessage_service_instance_state(server_system::Iinstance* _pinstance, eOBJECT_STATE _instance_state, uint64_t /*_flag*/)
{
	// 1) buffer
	auto buf_result = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

	// 2) append message head
	buf_result.append_text<char>(MESSAGE_WEB_SERVER_LIST); // "SELI"

	// 3) append message 
	{
		// - append command (A:Add, D:delete, U:update)
		buf_result.append<char>('U');

		// - tr element id
		auto tr_element_id = CGDK::stream_string("hi_s", reinterpret_cast<uint64_t>(_pinstance));

		// - append data
		::_append_key_value(buf_result, tr_element_id, to_string(_instance_state));
	}

	// 4) send message
	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_result.to_string<char>()));
}
shared_buffer _make_webmessage_service_instance_update_update(server_system::Iinstance* _pinstance, uint64_t /*_flag*/)
{
	// 1) buffer
	auto buf_result = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

	// 2) append message head
	buf_result.append_text<char>(MESSAGE_WEB_SERVER_LIST); // "SELI"

	// 3) append message 
	{
		// - temporary buffer
		auto buf_text = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

		// - append tr data
		_append_service_instance_status_open(buf_text, dynamic_cast<server_system::instance_service*>(_pinstance));

		// - append command (A:Add, D:delete, U:update)
		buf_result.append<char>('U');

		// - tr element id
		auto tr_element_id = CGDK::stream_string("tr", reinterpret_cast<uint64_t>(_pinstance));

		// - append data
		::_append_key_value(buf_result, tr_element_id, buf_text);
	}

	// 4) send message
	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_result.to_string<char>()));
}
shared_buffer _make_webmessage_service_instance_update_close(server_system::Iinstance* _pinstance, uint64_t /*_flag*/)
{
	// 1) get instance 
	object_ptr<service> pservice = ::service::instance();

	// 2) buffer
	auto buf_result = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);
	auto buf_temp = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

	// 3) append message head
	buf_result.append_text<char>(MESSAGE_WEB_SERVER_LIST); // "SELI"

	// 3) append message 
	{
		// - temporary buffer
		buffer buf_text = buf_temp;

		// - append tr data
		_append_service_instance_status_close(buf_text, dynamic_cast<server_system::instance_service*>(_pinstance));

		// - append command (A:Add, D:delete, U:update)
		buf_result.append<char>('U');

		// - tr element id
		auto tr_element_id = CGDK::stream_string("tr", reinterpret_cast<uint64_t>(_pinstance));

		// - append data
		::_append_key_value(buf_result, tr_element_id, buf_text);
	}

	// 4) send message
	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_result.to_string<char>()));
}
shared_buffer _make_webmessage_service_instance_cavas_process(server_system::Iinstance* _pinstance, std::string_view _head, uint64_t value, uint64_t /*_flag*/)
{
	// 2) buffer
	auto buf_result = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

	// 3) append message head
	buf_result.append_text<char>(MESSAGE_WEB_SERVER_LIST); // "SELI"

	// 4) append message 
	{
		// - append command (A:Add, D:delete, U:update, V:canvas process)
		buf_result.append<char>('V');

		// - tr element id
		auto tr_element_id = CGDK::stream_string(_head, reinterpret_cast<uint64_t>(_pinstance));

		// - append element_id
		buf_result.append_text<char>(tr_element_id);
		buf_result.append<char>('\n');

		// - append count
		buf_result.append_text<char>(1);
		buf_result.append<char>('\n');

		// - append value
		buf_result.append_text<char>(value);
		buf_result.append<char>('\n');
	}

	// 5) send message
	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_result.to_string<char>()));
}
shared_buffer _make_webmessage_service_instance_cavas_process(server_system::Iinstance* _pinstance, std::string_view _head, std::vector<uint64_t>& _vector_values, uint64_t /*_flag*/)
{
	// 2) buffer
	auto buf_result = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

	// 3) append message head
	buf_result.append_text<char>(MESSAGE_WEB_SERVER_LIST); // "SELI"

	// 4) append message 
	{
		// - append command (A:Add, D:delete, U:update, V:canvas process)
		buf_result.append<char>('V');

		// - tr element id
		auto tr_element_id = CGDK::stream_string(_head, reinterpret_cast<uint64_t>(_pinstance));

		// - append element_id
		buf_result.append_text<char>(tr_element_id);
		buf_result.append<char>('\n');

		// - append values
		{
			// - append value
			buf_result.append_text<char>(_vector_values.size());
			buf_result.append<char>('\n');

			// - append count
			for (auto& iter : _vector_values)
			{
				buf_result.append_text<char>(iter);
				buf_result.append<char>('\n');
			}
		}
	}

	// 5) send message
	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_result.to_string<char>()));
}
shared_buffer _make_web_message_log(std::string_view _command, const std::vector<object_ptr<LOG_RECORD>>& _vector_log_record);
shared_buffer _make_web_message_log_count(std::array<size_t, MAX_LOG_TYPE>& _array_log_count_last)
{
	// declare) 
	size_t log_count_total = 0;
	std::array<size_t, MAX_LOG_TYPE> array_log_count;

	// 1) get logger
	auto plogger = service::get_logger();

	// 2) get loc count
	SCOPED_LOCK(plogger->get_count_lock())
	{
		log_count_total = plogger->get_log_count();
		array_log_count = plogger->_log_count();
		_array_log_count_last = array_log_count;
	}

	// 2) alloc buffer
	auto buf_text = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

	// 3) append message head
	buf_text.append_text<char>(MESSAGE_WEB_GENERAL_HOST_INFO); // "GNRL"

	// 4) append log_counts
	::_append_key_value(buf_text, "log_count_total"sv, log_count_total);
	::_append_key_value(buf_text, "log_count_info"sv, array_log_count[static_cast<int>(log_type_t::info)]);		 // 0
	::_append_key_value(buf_text, "log_count_progress"sv, array_log_count[static_cast<int>(log_type_t::progress)]);// 1
	::_append_key_value(buf_text, "log_count_debug"sv, array_log_count[static_cast<int>(log_type_t::debug)]);		 // 2
	::_append_key_value(buf_text, "log_count_fatal"sv, array_log_count[static_cast<int>(log_type_t::fatal)]);		 // 3
	::_append_key_value(buf_text, "log_count_error"sv, array_log_count[static_cast<int>(log_type_t::error)]);		 // 4
	::_append_key_value(buf_text, "log_count_warning"sv, array_log_count[static_cast<int>(log_type_t::warning)]);	 // 5
	::_append_key_value(buf_text, "log_count_user"sv, array_log_count[static_cast<int>(log_type_t::user)]);		 // 6
	::_append_key_value(buf_text, "log_count_system"sv, array_log_count[static_cast<int>(log_type_t::system)]);	 // 7

	// return) make shared buffer and return
	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_text.to_string<char>()));;
}

void _append_event_object(buffer& _buf_dest, const server_system::time_event_snap& _time_event)
{
	// 1) "Name"
	_buf_dest.append_text<char>("<td><span style='font-size:15px;font-weight:bold'><i class='xi-alarm-o'></i> "sv);
	_buf_dest.append_text<char>(_time_event.updatable.entity_setting.name);
	_buf_dest.append_text<char>("</span></td>"sv);

	// 2) TYPE (ONCE/ITERATION/...)
	_buf_dest.append_text<char>("<td>"sv);
	_buf_dest.append_text<char>(to_string(static_cast<time_event::eTYPE>(_time_event.updatable.entity_setting.type)));
	_buf_dest.append_text<char>("</td>"sv);

	// 3) STATUS
	_buf_dest.append_text<char>("<td>"sv);
	_buf_dest.append_text<char>(to_string(_time_event.updatable.entity_status.state));
	_buf_dest.append_text<char>("</td>"sv);

	// 4) SETTER
	_buf_dest.append_text<char>("<td>"sv);
	_buf_dest.append_text<char>(to_string(_time_event.updatable.entity_setting.setter));
	_buf_dest.append_text<char>("</td>"sv);

	// 5) TIME.SET
	_buf_dest.append_text<char>("<td>"sv);
	_buf_dest.append_text<char>(to_string(_time_event.updatable.entity_status.time_setup, "{:04}-{:02}-{:02} {:02}:{:02}:{:02}"sv));
	_buf_dest.append_text<char>("</td>"sv);

	// 6) TIME.NEXT
	_buf_dest.append_text<char>("<td>"sv);
	_buf_dest.append_text<char>(to_string(_time_event.updatable.entity_status.time_next, "{:04}-{:02}-{:02} {:02}:{:02}:{:02}"sv));
	_buf_dest.append_text<char>("</td>"sv);

	// 7) TIME.LAST.TRY
	_buf_dest.append_text<char>("<td>"sv);
	if (_time_event.updatable.entity_status.count_try != 0)
	{
		_buf_dest.append_text<char>(to_string(_time_event.updatable.entity_status.time_last_try, "{:04}-{:02}-{:02} {:02}:{:02}:{:02}"sv));
	}
	_buf_dest.append_text<char>("</td>"sv);

	// 8) TRY
	_buf_dest.append_text<char>("<td>"sv);
	if (_time_event.updatable.entity_status.count_try != 0)
	{
		_buf_dest.append_text<char>(to_string(_time_event.updatable.entity_status.count_try));
	}
	_buf_dest.append_text<char>("</td>"sv);

	// 9) TIME.LAST_SUCCESS
	_buf_dest.append_text<char>("<td>"sv);
	if (_time_event.updatable.entity_status.count_succeeded != 0)
	{
		_buf_dest.append_text<char>(to_string(_time_event.updatable.entity_status.time_last_succeeded, "{:04}-{:02}-{:02} {:02}:{:02}:{:02}"sv));
	}
	_buf_dest.append_text<char>("</td>"sv);

	// 10) SUCCESS
	_buf_dest.append_text<char>("<td>"sv);
	if (_time_event.updatable.entity_status.count_succeeded != 0)
	{
		_buf_dest.append_text<char>(to_string(_time_event.updatable.entity_status.count_succeeded));
	}
	_buf_dest.append_text<char>("</td>"sv);

	// 11) FAILED
	_buf_dest.append_text<char>("<td>"sv);
	if (_time_event.updatable.entity_status.count_failed != 0)
	{
		_buf_dest.append_text<char>(to_string(_time_event.updatable.entity_status.count_failed));
	}
	_buf_dest.append_text<char>("</td>"sv);
}
shared_buffer _make_web_message_event_list()
{
	// 1) buffer
	auto buf_result = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

	// 2) append message head
	buf_result.append_text<char>("EVLI");

	// 3) get time_event amanger
	auto ptime_event_entity_manager = time_event::entity_manager::instance();

	// 4) append message 
	SCOPED_LOCK(ptime_event_entity_manager->get_lockable())
	{
		// - 공통적으로 사용할 temporary buffer
		auto buf_temp = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

		for (const auto& iter : *ptime_event_entity_manager)
		{
			// - temporary buffer set!
			buffer buf_text = buf_temp;

			// - tr element id
			auto tr_element_id = CGDK::stream_string("tr", reinterpret_cast<uint64_t>(iter.get()));

			// - append tr data
			_append_event_object(buf_text, server_system::get_snap(iter));

			// - append command
			buf_result.append<char>('A');

			// - append element id
			buf_result.append_text<char>(tr_element_id);
			buf_result.append<char>('\n');

			// - append length
			buf_result.append_text<char>(buf_text.size());
			buf_result.append<char>('\n');

			// - append data
			buf_result.append(buf_text.size(), buf_text.data());
		}
	}

	// 5) websocket message로 만들어 전송한다.
	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_result.to_string<char>()));
}
shared_buffer _make_webmessage_host_info_event_add(const time_event::Ientity* _pentity, const server_system::time_event_snap& _time_event)
{
	// 2) buffer
	auto buf_result = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

	// 3) append message head
	buf_result.append_text<char>("EVLI"sv);

	// 4) get time_event amanger
	auto ptime_event_entity_manager = time_event::entity_manager::instance();

	// 5) append message 
	{
		// - temporary buffer
		auto buf_text = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

		// - append tr data
		_append_event_object(buf_text, _time_event);

		// - append command
		buf_result.append<char>('A');

		// - tr element id
		auto tr_element_id = CGDK::stream_string("tr", reinterpret_cast<uint64_t>(_pentity));

		// - append data
		::_append_key_value(buf_result, tr_element_id, buf_text);
	}

	// 5) send message
	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_result.to_string<char>()));
}
shared_buffer _make_webmessage_host_info_event_update(const time_event::Ientity* _pentity, const server_system::time_event_snap& _entity_snap, uint64_t /*_flag*/)
{
	// 1) buffer
	auto buf_result = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

	// 2) append message head
	buf_result.append_text<char>("EVLI"sv);

	// 3) get time_event amanger
	auto ptime_event_entity_manager = time_event::entity_manager::instance();

	// 4) append message 
	{
		// - temporary buffer
		auto buf_text = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);;

		// - append tr data
		_append_event_object(buf_text, _entity_snap);

		// - append command
		buf_result.append<char>('U');

		// - tr element id
		auto tr_element_id = CGDK::stream_string("tr", reinterpret_cast<uint64_t>(_pentity));

		// - append data
		::_append_key_value(buf_result, tr_element_id, buf_text);
	}

	// 5) send message
	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_result.to_string<char>()));
}
shared_buffer _make_webmessage_host_info_event_remove(const time_event::Ientity* _pentity)
{
	// 1) buffer
	auto buf_result = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

	// 2) append message head
	buf_result.append_text<char>("EVLI"sv);

	// 3) get time_event amanger
	auto ptime_event_entity_manager = time_event::entity_manager::instance();

	// 4) append message 
	{
		// - append command (A:Add, D:delete, U:update)
		buf_result.append<char>('D');

		// - tr element id
		auto tr_element_id = CGDK::stream_string("tr", reinterpret_cast<uint64_t>(_pentity));

		// - append tr_element_id
		buf_result.append_text<char>(tr_element_id);
		buf_result.append<char>('\n');
	}

	// 5) send message
	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_result.to_string<char>()));
}

void process_service_instance_update(std::vector<shared_buffer>& _vector_send, server_system::Iinstance* _pinstance, uint64_t _flag)
{
	//-----------------------------------------------------------------
	// 설명) service instance 정보 update 처리 
	// 
	//   1) service_instance의 기본 정보를 update한다.
	//   2) 현재 service_list를 전송한다.
	//   3) 현재 time_event_list를 전송한다.
	//   4) 현재 log 정보를 전송한다.
	// 
	//-----------------------------------------------------------------
	// 1) canvas 정보 update
	// - casting
	auto pinstnace_service = dynamic_cast<server_system::instance_service*>(_pinstance);

	// - canvas 정보 update 
	if (pinstnace_service->m_instance.info.status.state != eOBJECT_STATE::NONE)
	{
		// - 
		if (pinstnace_service->m_state_applied == eOBJECT_STATE::NONE)
			_vector_send.push_back(_make_webmessage_service_instance_update_open(_pinstance, _flag));
		else
			_vector_send.push_back(_make_webmessage_service_instance_state(_pinstance, pinstnace_service->m_instance.info.status.state, _flag));

		if (pinstnace_service->m_vector_process_status.empty() == false)
		{
			const auto& process_status = pinstnace_service->m_vector_process_status.back();
			_vector_send.push_back(_make_webmessage_service_instance_cavas_process(_pinstance, "hi_cv1"sv, process_status.usage_process, _flag));
			_vector_send.push_back(_make_webmessage_service_instance_cavas_process(_pinstance, "hi_cv2"sv, process_status.bytes_memory_usage, _flag));
			_vector_send.push_back(_make_webmessage_service_instance_cavas_process(_pinstance, "hi_cv3"sv, process_status.handle_count, _flag));
			_vector_send.push_back(_make_webmessage_service_instance_cavas_process(_pinstance, "hi_cv4"sv, 0, _flag));
		}
	}
	else
	{
		_vector_send.push_back(_make_webmessage_service_instance_update_close(pinstnace_service, _flag));
	}

	// - update 상태 저장(이것ㅇs NONE -> OPEND 상태로 변경이 일어난 상태에서만 canvas 정보 등의 추가 정보를 보내주기 위해...)
	pinstnace_service->m_state_applied = pinstnace_service->m_instance.info.status.state;
}

object_ptr<server_system::Iinstance> find_instance(uint64_t _id)
{
	// declare)
	object_ptr<server_system::Iinstance> pselected;

	// 1) get instance 
	object_ptr<service> pservice = ::service::instance();

	// 4) append message 
	SCOPED_LOCK(::service::pmanager_instance->m_lockable_instance)
	{
		for (const auto& iter : ::service::pmanager_instance->m_map_instance)
		{
			if (reinterpret_cast<uint64_t>(iter.second.get()) == _id)
			{
				pselected = iter.second.get_object_ptr();
				break;
			}
		}
	}

	// return)
	return pselected;
}

object_ptr<time_event::Ientity> find_time_event_entity(uint64_t _id)
{
	// declare)
	object_ptr<time_event::Ientity> ptime_event_entity;

	// 1) get instance 
	auto ptime_event_entity_manager = time_event::entity_manager::instance();

	// 4) append message 
	SCOPED_LOCK(ptime_event_entity_manager->get_lockable())
	{
		for (const auto& iter : *ptime_event_entity_manager)
		{
			if (iter->entity_status().id == _id)
			{
				ptime_event_entity = iter;
				break;
			}
		}
	}

	// return)
	return ptime_event_entity;
}

observable_websocket_host_info::observable_websocket_host_info() :
	m_is_count_log_count_updating(false),
	m_array_log_count_last{ 0,0,0,0,0,0,0,0 }
{
	this->m_webpage_str_target = "webpage_host_info.htm"sv;
}

observable_websocket_host_info::~observable_websocket_host_info()
{
}

void observable_websocket_host_info::notify_to_web_controls_INSTANCE_ENTER(server_system::Iinstance* _pinstance)
{
	// declare) 
	shared_buffer buf_send;

	// 1) get
	SCOPED_LOCK(::service::pmanager_instance->m_lockable_instance)
	{
		buf_send = _make_webmessage_service_instance_add(_pinstance);
	}

	// 2) send
	this->send(buf_send);
}
void observable_websocket_host_info::notify_to_web_controls_INSTANCE_LEAVE(server_system::Iinstance* _pinstance)
{
	// declare) 
	shared_buffer buf_send;

	// 1) get
	SCOPED_LOCK(::service::pmanager_instance->m_lockable_instance)
	{
		buf_send = _make_webmessage_service_instance_remove(_pinstance);
	}

	// return)
	this->send(buf_send);
}

void observable_websocket_host_info::notify_to_web_controls_INSTANCE_STATUS_UPDATE(server_system::Iinstance* _pinstance, uint64_t _flag)
{
	// declare) 
	std::vector<shared_buffer> vector_send;

	// 1) get
	SCOPED_LOCK(::service::pmanager_instance->m_lockable_instance)
	{
		process_service_instance_update(vector_send, _pinstance, _flag);
	}

	// 2) send
	for (const auto& iter : vector_send)
		this->send(iter);
}

void observable_websocket_host_info::notify_to_web_controls_INSTANCE_STATUS_UPDATE()
{
	// declare) 
	std::vector<shared_buffer> vector_send;

	SCOPED_LOCK(::service::pmanager_instance->m_lockable_instance)
	{
		// 1) get
		for (auto& iter : ::service::pmanager_instance->m_map_instance)
		{
			process_service_instance_update(vector_send, iter.second, 0xffffffff);
		}
	}

	// 2) ...
	//vector_send.push_back(make_webmessage_server_update());
	//vector_send.push_back(_make_webmessage_host_info_server_update(_pinstance, 0xffffffff));

	// 3) send
	for (const auto& iter : vector_send)
		this->send(iter);
}

void observable_websocket_host_info::on_start(context& /*_context*/)
{
	// log) 
	LOG_INFO << " - start OBSERVABLE['"sv << name() << "']"sv;

	// 2) register to default logger
	service::get_logger()->register_log_targetable(this);
}

void observable_websocket_host_info::on_stop()
{
	// log) 
	LOG_INFO << " - stop OBSERVABLE['"sv << name() << "']"sv;

	// 1) unregister form default logger
	service::get_logger()->unregister_log_targetable(this);
}

void observable_websocket_host_info::process_web_service_instance_list(net::io::Isender* _psender)
{
	// 1)
	std::vector<shared_buffer> vector_buf_send;

	// 2) get instance 
	object_ptr<server_system::instance_manager> pmanager_instance = ::service::pmanager_instance;

	// 3) buffer
	auto buf_result = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);
	auto buf_temp = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

	// 4) append message head
	buf_result.append_text<char>(MESSAGE_WEB_SERVER_LIST);

	// 5) append message 
	SCOPED_LOCK(pmanager_instance->m_lockable_instance)
	{
		for (const auto& iter : pmanager_instance->m_map_instance)
		{
			// - temporary buffer
			buffer buf_text = buf_temp;

			// check) 
			CONTINUE_IF(iter.second->get_instance_type() != ::eINSTANCE_TYPE::SERVICE);

			// - get instance service
			auto pinstance_service = dynamic_pointer_cast<server_system::instance_service>(iter.second);

			// check)
			CGASSERT_ERROR(pinstance_service != nullptr);

			// - append tr data
			if (pinstance_service->m_instance.info.status.state != eOBJECT_STATE::NONE)
				_append_service_instance_status_open(buf_text, pinstance_service);
			else
				_append_service_instance_status_close(buf_text, pinstance_service);

			// - append command
			buf_result.append<char>('A');

			// - tr node name
			auto tr_element_id = CGDK::stream_string("tr", reinterpret_cast<uint64_t>(iter.second.get()));
			
			// - append element id
			buf_result.append_text<char>(tr_element_id);
			buf_result.append<char>('\n');

			// - append length
			buf_result.append_text<char>(buf_text.size());
			buf_result.append<char>('\n');

			// - append data
			buf_result.append(buf_text.size(), buf_text.data());

			if (pinstance_service->m_instance.info.status.state != eOBJECT_STATE::NONE && pinstance_service->m_vector_process_status.empty() == false)
			{
				// - get count
				std::vector<uint64_t> vector_data_1;
				std::vector<uint64_t> vector_data_2;
				std::vector<uint64_t> vector_data_3;
				std::vector<uint64_t> vector_data_4;
				{
					auto count = pinstance_service->m_vector_process_status.size();
					auto iter_data = pinstance_service->m_vector_process_status.begin();
					if (count > MAX_CANVAS_WITH_C)
					{
						iter_data += (count - MAX_CANVAS_WITH_C);
						count = MAX_CANVAS_WITH_C;
					}
				}

				for (auto& iter_data : pinstance_service->m_vector_process_status)
				{
					vector_data_1.push_back(iter_data.usage_process);
					vector_data_2.push_back(iter_data.bytes_memory_usage);
					vector_data_3.push_back(iter_data.size_traffic);
					vector_data_4.push_back(iter_data.handle_count);
				}

				// - canvas process
				vector_buf_send.push_back(_make_webmessage_service_instance_cavas_process(pinstance_service.get(), "hi_cv1"sv, vector_data_1, 0xffffffff));
				vector_buf_send.push_back(_make_webmessage_service_instance_cavas_process(pinstance_service.get(), "hi_cv2"sv, vector_data_2, 0xffffffff));
				vector_buf_send.push_back(_make_webmessage_service_instance_cavas_process(pinstance_service.get(), "hi_cv3"sv, vector_data_3, 0xffffffff));
				vector_buf_send.push_back(_make_webmessage_service_instance_cavas_process(pinstance_service.get(), "hi_cv4"sv, vector_data_4, 0xffffffff));
			}
		}
	}

	// 6) send message
	send(make_shared_buffer(sWEBSOCKET_MESSAGE(buf_result.to_string<char>())));

	// 7) canvas info update 
	for (auto& iter : vector_buf_send)
	{
		_psender->send(iter);
	}
}

void observable_websocket_host_info::on_observer_registered(socket_websocket* _pobserver, observer_channel_t /*_ocid*/, const observer_data_t& /*_observer_data*/)
{
	//-----------------------------------------------------------------
	// 설명) register 
	// 
	//  처음 observer에 등록되게 되면
	// 
	//   1) 먼저 message mediator로 등록한다.
	//   2) 현재 service_list를 전송한다.
	//   3) 현재 time_event_list를 전송한다.
	//   4) 현재 log 정보를 전송한다.
	// 
	//-----------------------------------------------------------------
	// 1) register _pmenber
	_pobserver->register_messageable(this);

	// 2) send server list/event list
	this->process_web_service_instance_list(_pobserver);
	_pobserver->send(_make_web_message_event_list());

	// 3) get logger
	auto plogger_archive = dynamic_pointer_cast<logger_hub_archive>(service::get_logger());
	_pobserver->send(_make_web_message_log_count(this->m_array_log_count_last));
	_pobserver->send(_make_web_message_log("LOGS"sv, plogger_archive->get_stored_log()));
}

uintptr_t observable_websocket_host_info::on_observer_unregistering(socket_websocket* _pobserver, observer_channel_t /*_ocid*/)
{
	// 1) unregister _pmenber
	_pobserver->unregister_messageable(this);

	// return) 
	return 0;
}

result_code observable_websocket_host_info::on_message(sMESSAGE_NETWORK& _msg)
{
	// 1) websocket message
	auto& msg = static_cast<sMESSAGE_HTTP_WEBSOCKET&>(_msg);

	// 2) get message head
	auto message_dest = msg.buf_message.data<char>()[0];

	// 3) process message by dest
	switch(message_dest)
	{
	// case) message - time_event 
	case	'E':
			return on_message_time_event(msg);
			break;

	// case) message - sever
	case	'S':
			return on_message_server_list(msg);
			break;

	default:
			break;
	}

	// return) 
	return eRESULT::BYPASS;
}

result_code observable_websocket_host_info::on_message_time_event(sMESSAGE_HTTP_WEBSOCKET& _msg)
{
	// 1) get message type
	auto message_type = _msg.buf_message.extract_text<char>(size(4));

	// 2) get target id
	const auto str_element_id = (_msg.buf_message + offset(2)).to_string<char>();
	uint64_t element_id = 0;
	std::from_chars(str_element_id.data(), str_element_id.data() + str_element_id.size(), element_id);

	// 3) find time event object
	object_ptr<time_event::Ientity> ptime_event_entity = find_time_event_entity(element_id);

	// check)
	if (ptime_event_entity.empty())
	{
		// return)
		return eRESULT::DONE;
	}

	// 4) time_event add
	if (message_type == "EVAD"sv)
	{
		//send(make_shared_buffer(sWEBSOCKET_MESSAGE(buf_binary)));

		// return)
		return eRESULT::DONE;
	}
	// 5) time_event remove
	else if (message_type == "EVRV"sv)
	{
		ptime_event_entity->destroy();

		// return)
		return eRESULT::DONE;
	}
	// 6) time_event start
	else if (message_type == "EVST"sv)
	{
		ptime_event_entity->start();

		// return)
		return eRESULT::DONE;
	}
	// 7) time_event pause
	else if (message_type == "EVPU"sv)
	{
		if (ptime_event_entity->get_object_state() == eOBJECT_STATE::PAUSED
			|| ptime_event_entity->get_object_state() == eOBJECT_STATE::PAUSE_PENDING)
		{
			ptime_event_entity->resume();
		}
		else if (ptime_event_entity->get_object_state() == eOBJECT_STATE::START_PENDING
			|| ptime_event_entity->get_object_state() == eOBJECT_STATE::RUNNING)
		{
			ptime_event_entity->pause();
		}

		// return)
		return eRESULT::DONE;
	}
	// 8) time_event stop
	else if (message_type == "EVSP"sv)
	{
		ptime_event_entity->stop();

		// return)
		return eRESULT::DONE;
	}

	// return)
	return eRESULT::BYPASS;
}

result_code observable_websocket_host_info::on_message_server_list(sMESSAGE_HTTP_WEBSOCKET& _msg)
{
	// 1) get message type
	auto message_type = _msg.buf_message.extract_text<char>(size(4));

	// 2) get target id
	const auto str_element_id = (_msg.buf_message + offset(2)).to_string<char>();
	uint64_t element_id = 0;
	std::from_chars(str_element_id.data(), str_element_id.data() + str_element_id.size(), element_id);

	// 3) find object
	object_ptr<server_system::Iinstance> pinstance = find_instance(element_id);

	// 4) 
	auto pinstance_service = dynamic_pointer_cast<server_system::instance_service>(pinstance);

	// check)
	RETURN_IF(pinstance_service.empty(), eRESULT::DONE);

	// check)
	RETURN_IF(element_id == 0, 1);

	// 4) server add
	if (message_type == "SVAD"sv)
	{
		// return)
		return eRESULT::DONE;
	}
	// 5) server remove
	else if (message_type == "SVRV"sv)
	{
		// return)
		return eRESULT::DONE;
	}
	// 6) server update
	else if (message_type == "SVUD"sv)
	{
		// - 
		pinstance_service->request_service_update();

		// return)
		return eRESULT::DONE;
	}
	// 7) server initialize
	else if (message_type == "SVIN"sv)
	{
		// - 
		pinstance_service->request_service_initialize();

		// return)
		return eRESULT::DONE;
	}
	// 8) server start
	else if (message_type == "SVST"sv)
	{
		std::string str_start;
		pinstance_service->request_service_start(str_start);

		// return)
		return eRESULT::DONE;
	}
	// 9) server stop
	else if (message_type == "SVSP"sv)
	{
		pinstance_service->request_service_stop();

		// return)
		return eRESULT::DONE;
	}
	// 10) server destroy
	else if (message_type == "SVDT"sv)
	{
		pinstance_service->request_service_destroy();

		// return)
		return eRESULT::DONE;
	}
	// 11) server kill
	else if (message_type == "SVKI"sv)
	{
		pinstance_service->request_service_kill();

		// return)
		return eRESULT::DONE;
	}

	// return)
	return eRESULT::BYPASS;
}

void observable_websocket_host_info::add_time_event(time_event::Ientity* _pentity)
{
	this->send(_make_webmessage_host_info_event_add(_pentity, server_system::get_snap(_pentity)));
}

void observable_websocket_host_info::remove_time_event(time_event::Ientity* _pentity)
{
	this->send(_make_webmessage_host_info_event_remove(_pentity));
}

void observable_websocket_host_info::execute_time_event(time_event::Ientity* _pentity)
{
	this->send(_make_webmessage_host_info_event_update(_pentity, server_system::get_snap(_pentity), 0));
}

void observable_websocket_host_info::update_time_event(time_event::Ientity* _pentity)
{
	this->send(_make_webmessage_host_info_event_update(_pentity, server_system::get_snap(_pentity), 0));
}

void observable_websocket_host_info::add_log_records(const std::vector<object_ptr<LOG_RECORD>>& _vector_log_record)
{
	this->process_send_logs(_vector_log_record);
}

void observable_websocket_host_info::process_log(object_ptr<LOG_RECORD>&& _plog_record)
{
	// check)
	RETURN_IF(_plog_record.empty());

	this->process_send_logs({ std::move(_plog_record) });
}

void observable_websocket_host_info::process_send_logs(const std::vector<object_ptr<LOG_RECORD>>& _vector_log_record)
{
	// 1) send log message
	this->send(_make_web_message_log("LOGS"sv, _vector_log_record));

	// 2) send updating log count message
	SCOPED_LOCK(service::get_logger()->get_count_lock())
	{
		// check)
		RETURN_IF(this->m_is_count_log_count_updating == true);

		// - flag set true
		this->m_is_count_log_count_updating = true;
	}

	// 3) request
	system_executor::post(make_executable(this, &observable_websocket_host_info::process_send_webmessage_log_count_update));
}

void observable_websocket_host_info::process_send_webmessage_log_count_update()
{
	// declare) 
	size_t log_count_total = 0;
	std::array<size_t, MAX_LOG_TYPE> array_log_count;

	// 1) get logger
	auto plogger = service::get_logger();

	// 1) get loc count
	SCOPED_LOCK(plogger->get_count_lock())
	{
		// check)
		CGASSERT_ERROR(this->m_is_count_log_count_updating == true);

		// - check) 
		log_count_total = plogger->get_log_count();
		array_log_count = plogger->_log_count();
	}

	// 2) alloc buffer
	auto buf_text = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

	// 3) append message head
	buf_text.append_text<char>(MESSAGE_WEB_GENERAL_HOST_INFO); // "GNRL"

	// 3) append log_count.total
	::_append_key_value(buf_text, "log_count_total"sv, log_count_total);

	// 3) append log_count.total
	if(m_array_log_count_last.at(0) != array_log_count.at(0))
		::_append_key_value(buf_text, "log_count_info"sv, array_log_count[static_cast<int>(log_type_t::info)]);			// 0

	if (m_array_log_count_last.at(1) != array_log_count.at(1))
		::_append_key_value(buf_text, "log_count_progress"sv, array_log_count[static_cast<int>(log_type_t::progress)]);	// 1

	if (m_array_log_count_last.at(2) != array_log_count.at(2))
		::_append_key_value(buf_text, "log_count_debug"sv, array_log_count[static_cast<int>(log_type_t::debug)]);		// 2

	if (m_array_log_count_last.at(3) != array_log_count.at(3))
		::_append_key_value(buf_text, "log_count_fatal"sv, array_log_count[static_cast<int>(log_type_t::fatal)]);		// 3

	if (m_array_log_count_last.at(4) != array_log_count.at(4))
		::_append_key_value(buf_text, "log_count_error"sv, array_log_count[static_cast<int>(log_type_t::error)]);		// 4

	if (m_array_log_count_last.at(5) != array_log_count.at(5))
		::_append_key_value(buf_text, "log_count_warning"sv, array_log_count[static_cast<int>(log_type_t::warning)]);	// 5

	if (m_array_log_count_last.at(6) != array_log_count.at(6))
		::_append_key_value(buf_text, "log_count_user"sv, array_log_count[static_cast<int>(log_type_t::user)]);			// 6

	if (m_array_log_count_last.at(7) != array_log_count.at(7))
		::_append_key_value(buf_text, "log_count_system"sv, array_log_count[static_cast<int>(log_type_t::system)]);		// 7

	// 4) send
	this->send(make_shared_buffer(sWEBSOCKET_MESSAGE(buf_text.to_string<char>())));

	// 5) re-request or stop
	SCOPED_LOCK(plogger->get_count_lock())
	{
		// - store last log count
		this->m_array_log_count_last = array_log_count;

		// - stop if no-log posted
		if (log_count_total == plogger->get_log_count())
		{
			this->m_is_count_log_count_updating = false;

			// return)
			return;
		}
	}

	// 6) request
	system_executor::post(make_executable(this, &observable_websocket_host_info::process_send_webmessage_log_count_update));
}

