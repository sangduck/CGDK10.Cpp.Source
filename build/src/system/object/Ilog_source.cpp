#include "pch.h"

namespace CGDK
{

object_ptr<LOG_RECORD> Ilog_source::process_store_log(log_type_t _log_type, log_level_t _log_level, eCHARACTER_SET _char_code, std::string_view _log_message) noexcept
{
	// declare)
	object_ptr<LOG_RECORD> plog_record;

	try
	{
		// 1) allocate new LOG_RECORD object
		plog_record = make_object<LOG_RECORD>();

		// 2) set log info
		plog_record->type = _log_type;
		plog_record->level = _log_level;
		plog_record->set_message(_log_message, _char_code);
		plog_record->time_occure = chrono::time::utc::now();
	}
	catch (...)
	{
		plog_record.reset();
	}

	std::unique_ptr<LOG_RECORD> x;


	// return) 
	return plog_record;
}


}