#include "pch.h"

using namespace CGDK;


log_filter::log_filter()
{
}


log_filter::~log_filter() noexcept
{
}

void log_filter::enable_log(log_type_t _log_type, bool _bEnable)
{
	// check) 
	THROW_IF(_log_type<log_type_t::info || _log_type>=log_type_t::max_, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "log type is invalid"sv));

	// 1) enable함.
	m_filter_info[static_cast<int>(_log_type)].is_enable = _bEnable;
}

void log_filter::disable_log(log_type_t _log_type)
{
	// check) 
	THROW_IF(_log_type<log_type_t::info || _log_type>=log_type_t::max_, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "log type is invalid"sv));

	m_filter_info[static_cast<int>(_log_type)].is_enable = false;
}


void log_filter::enable_filtering_by_level_range(log_type_t _log_type, bool _bEnable)
{
	// check) 
	THROW_IF(_log_type<log_type_t::info || _log_type>= log_type_t::max_, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "log type is invalid"sv));

	// 1) enable함.
	m_filter_info[static_cast<int>(_log_type)].is_limit_level = _bEnable;
}

void log_filter::disable_filtering_by_level_range(log_type_t _log_type)
{
	// check) 
	THROW_IF(_log_type<log_type_t::info || _log_type>= log_type_t::max_, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "log type is invalid"sv));

	// 1) enable함.
	m_filter_info[static_cast<int>(_log_type)].is_limit_level = false;
}

void log_filter::set_filtering_range_by_level(log_type_t _log_type, log_level_t _min, log_level_t _max)
{
	// check) 
	THROW_IF(_log_type<log_type_t::info || _log_type>= log_type_t::max_, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "log type is invalid"sv));

	m_filter_info[static_cast<int>(_log_type)].range_level_min = log_level_t{ _min };
	m_filter_info[static_cast<int>(_log_type)].range_level_max = log_level_t{ _max };
}


void log_filter::enable_filtering_by_source_range(log_type_t _log_type, bool _bEnable)
{
	// check) 
	THROW_IF(_log_type<log_type_t::info || _log_type>= log_type_t::max_, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "log type is invalid"sv));

	// 1) enable함.
	m_filter_info[static_cast<int>(_log_type)].is_limit_source = _bEnable;
}

void log_filter::disable_filtering_by_source_range(log_type_t _log_type)
{
	// check) 
	THROW_IF(_log_type<log_type_t::info || _log_type>= log_type_t::max_, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "log type is invalid"sv));

	// 1) enable함.
	m_filter_info[static_cast<int>(_log_type)].is_limit_source = false;
}

void log_filter::set_filtering_range_by_source(log_type_t _log_type, log_source_t _min, log_source_t _max)
{
	// check) 
	THROW_IF(_log_type<log_type_t::info || _log_type>= log_type_t::max_, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "log type is invalid"sv));

	m_filter_info[static_cast<int>(_log_type)].range_source_min = _min;
	m_filter_info[static_cast<int>(_log_type)].range_source_max = _max;
}


void log_filter::enable_filtering_by_destination_range(log_type_t _log_type, bool _bEnable)
{
	// check) 
	THROW_IF(_log_type<log_type_t::info || _log_type>= log_type_t::max_, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "log type is invalid"sv));

	// 1) enable함.
	m_filter_info[static_cast<int>(_log_type)].is_limit_destination	 = _bEnable;
}

void log_filter::disable_filtering_by_destination_range(log_type_t _log_type)
{
	// check) 
	THROW_IF(_log_type<log_type_t::info || _log_type>= log_type_t::max_, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "log type is invalid"sv));

	// 1) enable함.
	m_filter_info[static_cast<int>(_log_type)].is_limit_destination	 = false;
}

void log_filter::set_filtering_range_by_destination(log_type_t _log_type, log_destination_t _min, log_destination_t _max)
{
	// check) 
	THROW_IF(_log_type<log_type_t::info || _log_type>= log_type_t::max_, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "log type is invalid"sv));

	m_filter_info[static_cast<int>(_log_type)].range_destination_min = _min;
	m_filter_info[static_cast<int>(_log_type)].range_destination_max = _max;
}


void log_filter::enable_filtering_by_origin_range(log_type_t _log_type, bool _bEnable)
{
	// check) 
	THROW_IF(_log_type<log_type_t::info || _log_type>= log_type_t::max_, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "log type is invalid"sv));

	// 1) enable함.
	m_filter_info[static_cast<int>(_log_type)].is_limit_origin = _bEnable;
}

void log_filter::disable_filtering_by_origin_range(log_type_t _log_type)
{
	// check) 
	THROW_IF(_log_type<log_type_t::info || _log_type>= log_type_t::max_, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "log type is invalid"sv));

	// 1) enable함.
	m_filter_info[static_cast<int>(_log_type)].is_limit_origin = false;
}

void log_filter::set_filtering_range_by_origin(log_type_t _log_type, log_origin_t _min, log_origin_t _max)
{
	// check) 
	THROW_IF(_log_type<log_type_t::info || _log_type>= log_type_t::max_, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "log type is invalid"sv));

	m_filter_info[static_cast<int>(_log_type)].range_origin_min = _min;
	m_filter_info[static_cast<int>(_log_type)].range_origin_max = _max;
}

void log_filter::set_filter_info(log_type_t _log_type, const log_filter::FILTER_INFO& _filter_info)
{
	// check) 
	THROW_IF(_log_type<log_type_t::info || _log_type>= log_type_t::max_, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "log type is invalid"sv));

	// 1) 값을 설정한다.
	m_filter_info[static_cast<int>(_log_type)] = _filter_info;
}

const log_filter::FILTER_INFO& log_filter::get_filter_info(log_type_t _log_type) const
{
	// check) 
	THROW_IF(_log_type<log_type_t::info || _log_type>= log_type_t::max_, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "log type is invalid"sv));

	// return) 
	return m_filter_info[static_cast<int>(_log_type)];
}

bool log_filter::process_filtering(const LOG_RECORD* _plog_record)
{
	// 1) Log type을 얻는다.
	log_type_t log_type = _plog_record->type & 0xffff;

	// check) type이 범위안의 값인지 검사한다.
	RETURN_IF(log_type < log_type_t::info || log_type >= log_type_t::max_, false);

	// 2) 
	auto& filter_info = m_filter_info[static_cast<int>(log_type)];

	// 2) 허가된 type인지 검사한다.
	RETURN_IF(filter_info.is_enable == false, false);

	// 3) Level검사를 한다.
	RETURN_IF( filter_info.is_limit_level && (_plog_record->level < filter_info.range_level_min || _plog_record->level > filter_info.range_level_max), false);

	// return) 
	return true;
}
