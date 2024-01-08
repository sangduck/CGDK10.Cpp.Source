#include "pch.h"

using namespace CGDK;


log_filter::FILTER_INFO get_parameter_filter_info(const context& _context, bool _is_enable_log = true)
{
	// declare) 
	log_filter::FILTER_INFO	filter_info;

	filter_info.is_enable = true;	// bool

	// 2) level
	if (auto& context_find = _context["range_level_min"sv]; context_find.is_valid())
	{
		// - set value
		filter_info.range_level_min = log_level_t{ (int)context_find };
		filter_info.is_limit_level = true;

		// log)
		EXECUTE_IF(_is_enable_log, LOG_INFO << "  + parameter['range_level_min']: '"sv << filter_info.range_level_min << "'"sv);
	}
	if (auto& context_find = _context["range_level_max"sv]; context_find.is_valid())
	{
		// - set value
		filter_info.range_level_max = log_level_t{ (int)context_find };
		filter_info.is_limit_level = true;

		// log)
		EXECUTE_IF(_is_enable_log, LOG_INFO << "  + parameter['range_level_max']: '"sv << filter_info.range_level_max << "'"sv);
	}

	// 2) location
	if (auto& context_find = _context["range_origin_min"sv]; context_find.is_valid())
	{
		// - set value
		filter_info.range_origin_min = log_origin_t{ (uint64_t)context_find };
		filter_info.is_limit_origin = true;

		// log)
		EXECUTE_IF(_is_enable_log, LOG_INFO << "  + parameter['range_origin_min']: '"sv << filter_info.range_origin_min << "'"sv);
	}
	if (auto& context_find = _context["range_origin_max"sv]; context_find.is_valid())
	{
		// - set value
		filter_info.range_origin_max = log_origin_t{ (uint64_t)context_find };
		filter_info.is_limit_origin = true;

		// log)
		EXECUTE_IF(_is_enable_log, LOG_INFO << "  + parameter['range_origin_max']: '"sv << filter_info.range_origin_max << "'"sv);
	}

	// 3) source
	if (auto& context_find = _context["range_source_min"sv]; context_find.is_valid())
	{
		// - set value
		filter_info.range_source_min = log_source_t{ (uint64_t)context_find };
		filter_info.is_limit_source = true;

		// log)
		EXECUTE_IF(_is_enable_log, LOG_INFO << "  + parameter['range_source_min']: '"sv << filter_info.range_source_min << "'"sv);
	}
	if (auto& context_find = _context["range_source_max"sv]; context_find.is_valid())
	{
		// - set value
		filter_info.range_source_max = log_source_t{ (uint64_t)context_find };
		filter_info.is_limit_source = true;

		// log)
		EXECUTE_IF(_is_enable_log, LOG_INFO << "  + parameter['range_source_max']: '"sv << filter_info.range_source_max << "'"sv);
	}

	// 4) destination
	if (auto& context_find = _context["range_destination_min"sv]; context_find.is_valid())
	{
		// - set value
		filter_info.range_destination_min = log_destination_t{ (uint64_t)context_find };
		filter_info.is_limit_destination = true;

		// log)
		EXECUTE_IF(_is_enable_log, LOG_INFO << "  + parameter['range_destination_min']: '"sv << filter_info.range_destination_min << "'"sv);
	}
	if (auto& context_find = _context["range_destination_max"sv]; context_find.is_valid())
	{
		// - set value
		filter_info.range_destination_max = log_destination_t{ (uint64_t)context_find };
		filter_info.is_limit_destination = true;

		// log)
		EXECUTE_IF(_is_enable_log, LOG_INFO << "  + parameter['range_destination_max']: '"sv << filter_info.range_destination_max << "'"sv);
	}

	// return) 
	return filter_info;
}


result_code Nlogger::_process_initializing(context& _context)
{
	// declare)
	eRESULT result;

	try
	{
		// 1) ...
		result = Ninitializable::_process_initializing(_context);

		// 2) "filter"
		if (auto context_find = _context.find("filter"sv); context_find.is_valid())
		{
			// 3) Filter를 새로 생성한다.
			auto pfilter = make_object<log_filter>();

			if (auto context_filter_info = _context.find("info"sv); context_find.is_valid())
			{
				pfilter->set_filter_info(log_type_t::info, get_parameter_filter_info(context_filter_info));
			}
			else if (context_filter_info = _context.find("progress"sv); context_find.is_valid())
			{
				pfilter->set_filter_info(log_type_t::progress, get_parameter_filter_info(context_filter_info));
			}
			else if (context_filter_info = _context.find("debug"sv); context_find.is_valid())
			{
				pfilter->set_filter_info(log_type_t::debug, get_parameter_filter_info(context_filter_info));
			}
			else if (context_filter_info = _context.find("fatal"sv); context_find.is_valid())
			{
				pfilter->set_filter_info(log_type_t::fatal, get_parameter_filter_info(context_filter_info));
			}
			else if (context_filter_info = _context.find("error"sv); context_find.is_valid())
			{
				pfilter->set_filter_info(log_type_t::error, get_parameter_filter_info(context_filter_info));
			}
			else if (context_filter_info = _context.find("warning"sv); context_find.is_valid())
			{
				pfilter->set_filter_info(log_type_t::warning, get_parameter_filter_info(context_filter_info));
			}
			else if (context_filter_info = _context.find("user"sv); context_find.is_valid())
			{
				pfilter->set_filter_info(log_type_t::user, get_parameter_filter_info(context_filter_info));
			}
			else if (context_filter_info = _context.find("system"sv); context_find.is_valid())
			{
				pfilter->set_filter_info(log_type_t::system, get_parameter_filter_info(context_filter_info));
			}

			// - set filter object
			set_filter(pfilter);
		}
	}
	catch(...)
	{
		// log) 
		LOG_ERROR << "@ <TERMINATE> exception complete Nlogger['"sv << name() << "']"sv;

		// Reraise)
		throw;
	}

	// return)
	return result;
}
