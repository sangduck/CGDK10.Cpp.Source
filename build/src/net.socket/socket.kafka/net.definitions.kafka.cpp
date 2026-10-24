#include "pch.h"

using namespace CGDK;


//-----------------------------------------------------------------------------
// helpers
//-----------------------------------------------------------------------------
// 1) value of the context as a string (bool: "true"/"false")
static std::string _to_parameter_string(const context& _context)
{
	if (_context.type() == context::value_type_t::bool_)
		return static_cast<bool>(_context) ? "true"s : "false"s;

	std::string str_value = _context;

	return str_value;
}

// 2) "true"/"enable" or "false"/"disable" (0: not assigned, 1: false, 2: true)
static int _to_parameter_flag(const context& _context)
{
	// check)
	RETURN_IF(_context.is_valid() == false, 0);

	// 1) get string
	auto str_find = _to_parameter_string(_context);

	if (CGDK::__stricmp(str_find, "true") == 0
	 || CGDK::__stricmp(str_find, "enable") == 0)
		return 2;

	if (CGDK::__stricmp(str_find, "false") == 0
	 || CGDK::__stricmp(str_find, "disable") == 0)
		return 1;

	// return)
	return 0;
}

// 3) milliseconds (number or string)
static bool _to_parameter_duration(const context& _context, chrono::tick::duration& _duration)
{
	// check)
	RETURN_IF(_context.is_valid() == false, false);

	// 1) duration
	if (_context.type() == context::value_type_t::chrono_duration_)
	{
		_duration = static_cast<chrono::tick::duration>(_context);
		return true;
	}

	// 2) milliseconds
	auto str_find = _to_parameter_string(_context);

	// check)
	RETURN_IF(str_find.empty(), false);

	try
	{
		_duration = std::chrono::milliseconds(std::stoll(str_find));
	}
	catch (...)
	{
		return false;
	}

	// return)
	return true;
}

// 4) split "a,b, c" => { "a", "b", "c" }
static void _append_parameter_list(std::vector<std::string>& _list, std::string_view _str)
{
	while (_str.empty() == false)
	{
		// - find ','
		auto pos = _str.find(',');
		auto item = _str.substr(0, pos);

		// - trim
		while (item.empty() == false && std::isspace(static_cast<unsigned char>(item.front()))) item.remove_prefix(1);
		while (item.empty() == false && std::isspace(static_cast<unsigned char>(item.back()))) item.remove_suffix(1);

		// - append
		if (item.empty() == false)
			_list.emplace_back(item);

		// check)
		if (pos == std::string_view::npos)
			break;

		// - next
		_str.remove_prefix(pos + 1);
	}
}

// 5) the value of the property for log (hides secrets)
static std::string_view _to_log_value(std::string_view _name, std::string_view _value)
{
	if (_name.find("password") != std::string_view::npos
	 || _name.find("secret") != std::string_view::npos
	 || _name.find("sasl.oauthbearer.config") != std::string_view::npos)
		return "****"sv;

	return _value;
}


//-----------------------------------------------------------------------------
// get_parameter_kafka_client
//-----------------------------------------------------------------------------
void CGDK::get_parameter_kafka_client(const context& _context, sKAFKA_CLIENT_PARAMETER& _parameter, bool _enable_progress_log)
{
	// 1) "bootstrap_servers"
	if (const auto& context_find = _context.find("bootstrap_servers", "bootstrap servers", "bootstrap.servers", "brokers", "broker", "servers", "server"); context_find.is_valid())
	{
		_parameter.bootstrap_servers = _to_parameter_string(context_find);

		// log)
		EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['bootstrap_servers']: '"sv << _parameter.bootstrap_servers << "'"sv);
	}

	// 2) "client_id"
	if (const auto& context_find = _context.find("client_id", "client id", "client.id"); context_find.is_valid())
	{
		_parameter.client_id = _to_parameter_string(context_find);

		// log)
		EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['client_id']: '"sv << _parameter.client_id << "'"sv);
	}

	// 3) "poll_interval"
	if (const auto& context_find = _context.find("poll_interval", "poll interval"); context_find.is_valid())
	{
		if (chrono::tick::duration duration; _to_parameter_duration(context_find, duration) && duration > chrono::tick::duration::zero())
		{
			_parameter.poll_interval = duration;

			// log)
			EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['poll_interval']: "sv << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() << "ms"sv);
		}
		else
		{
			// log)
			LOG_ERROR << "  + parameter['poll_interval']: '"sv << _to_parameter_string(context_find) << "' INVALID poll interval. default interval applied"sv;
		}
	}

	// 4) "properties" (any librdkafka property)
	if (const auto& context_find = _context.find("properties"); context_find.is_valid())
	{
		for (const auto& iter : context_find)
		{
			// check)
			if (iter.second.is_valid() == false || iter.second.type() == context::value_type_t::node_)
				continue;

			// - append
			_parameter.properties.emplace_back(iter.first, _to_parameter_string(iter.second));

			// log)
			EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['properties']['"sv << iter.first << "']: '"sv << _to_log_value(iter.first, _parameter.properties.back().second) << "'"sv);
		}
	}

	// 5) "progress_log"
	if (auto flag = _to_parameter_flag(_context.find("progress_log", "progress log", "enable_progress_log", "enable progress log")); flag != 0)
	{
		_parameter.enable_progress_log = (flag == 2);
	}
}


//-----------------------------------------------------------------------------
// get_parameter_kafka_producer
//-----------------------------------------------------------------------------
sKAFKA_PRODUCER_PARAMETER CGDK::get_parameter_kafka_producer(const context& _context, bool _enable_progress_log)
{
	// declare)
	sKAFKA_PRODUCER_PARAMETER parameter;

	// 1) common
	get_parameter_kafka_client(_context, parameter, _enable_progress_log);

	// 2) "topic"
	if (const auto& context_find = _context.find("topic", "default_topic", "default topic"); context_find.is_valid())
	{
		parameter.topic = _to_parameter_string(context_find);

		// log)
		EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['topic']: '"sv << parameter.topic << "'"sv);
	}

	// 3) "flush_timeout"
	if (const auto& context_find = _context.find("flush_timeout", "flush timeout"); context_find.is_valid())
	{
		if (chrono::tick::duration duration; _to_parameter_duration(context_find, duration))
		{
			parameter.flush_timeout = duration;

			// log)
			EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['flush_timeout']: "sv << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() << "ms"sv);
		}
		else
		{
			// log)
			LOG_ERROR << "  + parameter['flush_timeout']: '"sv << _to_parameter_string(context_find) << "' INVALID flush timeout. default timeout applied"sv;
		}
	}

	// 4) "block_on_queue_full"
	if (auto flag = _to_parameter_flag(_context.find("block_on_queue_full", "block on queue full")); flag != 0)
	{
		parameter.block_on_queue_full = (flag == 2);

		// log)
		EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['block_on_queue_full']: "sv << (parameter.block_on_queue_full ? "'enable'"sv : "'disable'"sv));
	}

	// return)
	return parameter;
}


//-----------------------------------------------------------------------------
// get_parameter_kafka_consumer
//-----------------------------------------------------------------------------
sKAFKA_CONSUMER_PARAMETER CGDK::get_parameter_kafka_consumer(const context& _context, bool _enable_progress_log)
{
	// declare)
	sKAFKA_CONSUMER_PARAMETER parameter;

	// 1) common
	get_parameter_kafka_client(_context, parameter, _enable_progress_log);

	// 2) "group_id"
	if (const auto& context_find = _context.find("group_id", "group id", "group.id"); context_find.is_valid())
	{
		parameter.group_id = _to_parameter_string(context_find);

		// log)
		EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['group_id']: '"sv << parameter.group_id << "'"sv);
	}

	// 3) "topics" ("a,b" or ["a", "b"])
	if (const auto& context_find = _context.find("topics", "topic"); context_find.is_valid())
	{
		if (context_find.type() == context::value_type_t::node_)
		{
			for (const auto& iter : context_find)
			{
				if (iter.second.is_valid())
					_append_parameter_list(parameter.topics, _to_parameter_string(iter.second));
			}
		}
		else
		{
			_append_parameter_list(parameter.topics, _to_parameter_string(context_find));
		}

		// log)
		if (_enable_progress_log)
		{
			for (const auto& iter : parameter.topics)
				LOG_INFO << "  + parameter['topics']: '"sv << iter << "'"sv;
		}
	}

	// 4) "auto_offset_reset"
	if (const auto& context_find = _context.find("auto_offset_reset", "auto offset reset", "auto.offset.reset"); context_find.is_valid())
	{
		parameter.auto_offset_reset = _to_parameter_string(context_find);

		// log)
		EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['auto_offset_reset']: '"sv << parameter.auto_offset_reset << "'"sv);
	}

	// return)
	return parameter;
}
