#include "pch.h"
#include <charconv>

using namespace CGDK;


logger_file::logger_file():	Nlogger_async()
{
}

logger_file::~logger_file() noexcept
{
	this->destroy();
}

result_code logger_file::initialize(const INITIALIZE_PARAMETER& _initialize_param)
{
	// check)
	CGASSERT_ERROR(_initialize_param.char_code != eCHARACTER_SET::NONE && _initialize_param.char_code <= eCHARACTER_SET::MAX_)

	// check)
	THROW_IF(_initialize_param.char_code == eCHARACTER_SET::NONE || _initialize_param.char_code > eCHARACTER_SET::MAX_, throwable::failure(eRESULT::INVALID_ARGUMENT));

	// declare)
	context	context_setting;

	// 1) 값을 설정한다.
	context_setting["filename"sv] = _initialize_param.filename;
	context_setting["path"sv] = _initialize_param.path;
	context_setting["backup_path"sv] = _initialize_param.backup_path;

	// 2) Origin type을 써넣는다.
	context_setting["origin_time_type"sv] = to_string(_initialize_param.origin_time_type);

	// 3) Recreation Option
	if(_initialize_param.recreation_interval != chrono::time::duration::zero())
	{
		context_setting["recreation"sv] = true;
		context_setting["interval"sv] = std::chrono::duration_cast<chrono::minutes>(_initialize_param.recreation_interval).count();
	}

	// 4) Recreation limit
	if(_initialize_param.limit_bytes != 0)
		context_setting["limit_bytes"sv] = std::to_string(_initialize_param.limit_bytes);
	if(_initialize_param.limit_lines != 0)
		context_setting["limit_lines"sv] = std::to_string(_initialize_param.limit_lines);

	// 5) Origin time
	if(_initialize_param.begin_time != (chrono::time::utc::time_point::min)())
		context_setting["origin_time"sv] = to_string(_initialize_param.begin_time);

	// 6) character type
	context_setting["char_encoding"sv] = to_string(_initialize_param.char_code);
	context_setting["eol_type"sv] = to_string(_initialize_param.eol_type);

	// 7) instant open
	context_setting["instant_open"sv] = (_initialize_param.is_instant_open) ? "true"sv : "false"sv;
	context_setting["instant_close_delay"sv] = std::to_string(_initialize_param.instant_close_delay.count());

	// return) 
	return this->Ninitializable::initialize(context_setting);
}

result_code logger_file::_process_initializing(context& _context)
{
	// trace) 
	LOG_PROGRESS << "@ <BEGIN> initializing LOGGER_FILE["sv << name() << "]"sv;

	// 1) on_initializing 함수를 호출한다.
	this->on_initializing(_context);

	// 2) 현재 시간을 구한다.
	auto time_now = chrono::time::utc::now();

	// 3) get parameters
	auto enabled = _process_get_parameter_enabled(_context);
	auto path = _process_get_parameter_path(_context);
	auto path_backup = _process_get_parameter_path_backup(_context);
	auto filename = _process_get_parameter_filename(_context);
	auto duration_recreation = _process_get_parameter_interval(_context);
	auto origin_time_type = _process_get_parameter_origin_time_type(_context);
	auto time_begin = _process_get_parameter_origin_time(_context, time_now, origin_time_type, filename);
	auto recreation_limit_bytes = _process_get_parameter_recreation_bytes_limit(_context);
	auto recreation_limit_lines = _process_get_parameter_recreation_lines_limit(_context);
	auto is_create_new = _process_get_parameter_create_new(_context);
	auto char_encoding = _process_get_parameter_char_encoding(_context);
	//auto filter = _process_get_parameter_filter(_context);
	auto is_instant_open = _process_get_parameter_is_instant_open(_context);
	auto instant_close_delay = _process_get_parameter_instant_close_delay(_context);

	if (enabled)
	{
		// check) MBCS, WINDOWS_UNICODE, UTF8만 지원한다.
		THROW_IF(char_encoding != eCHARACTER_SET::MBCS && char_encoding != eCHARACTER_SET::UCS2 && char_encoding != eCHARACTER_SET::UTF8, throwable::failure(eRESULT::INVALID_ARGUMENT));

		// 4) 현재 시간을 구한다.
		switch (origin_time_type)
		{
		case	eORIGIN_TIME_TYPE::NOW:
				break;

		case	eORIGIN_TIME_TYPE::MINUTE_BEGIN:
				{
					const auto ticks_per_minute = std::chrono::duration_cast<chrono::time::duration>(std::chrono::minutes(1)).count();
					auto time_epoch = time_begin.time_since_epoch().count();
					time_epoch -= (time_epoch % ticks_per_minute);

					time_begin = chrono::time::utc::time_point(chrono::time::duration(time_epoch));
				}
				break;

		case	eORIGIN_TIME_TYPE::HOUR_BEGIN:
				{
					const auto ticks_per_hour = std::chrono::duration_cast<chrono::time::duration>(std::chrono::hours(1)).count();
					auto time_epoch = time_begin.time_since_epoch().count();
					time_epoch -= (time_epoch % ticks_per_hour);

					time_begin = chrono::time::utc::time_point(chrono::time::duration(time_epoch));
				}
				break;

		case	eORIGIN_TIME_TYPE::DAY_BEGIN:
				{
					const auto ticks_per_day = std::chrono::duration_cast<chrono::time::duration>(std::chrono::hours(1)).count() * 24;
					auto time_epoch = time_begin.time_since_epoch().count();
					time_epoch -= (time_epoch % ticks_per_day);

					time_begin = chrono::time::utc::time_point(chrono::time::duration(time_epoch));
				}
				break;

		case	eORIGIN_TIME_TYPE::WEEK_BEGIN:
				{
					const auto ticks_per_day = std::chrono::duration_cast<chrono::time::duration>(std::chrono::hours(1)).count() * 24;

					auto time_epoch = time_begin.time_since_epoch().count();
					auto temp_tm = chrono::to_tm(time_begin);

					time_epoch -= (time_epoch % ticks_per_day);
					time_epoch -= ticks_per_day * temp_tm.tm_wday;

					time_begin = chrono::time::utc::time_point(chrono::time::duration(time_epoch));
				}
				break;

		case	eORIGIN_TIME_TYPE::MONTH_BEGIN:
				{
					auto temp_dtf = chrono::to_date_time(time_begin);

					temp_dtf.t_fraction = 0;
					temp_dtf.t_second = 0;
					temp_dtf.t_minute = 0;
					temp_dtf.t_hour = 0;
					temp_dtf.t_day = 1;

					time_begin = chrono::from_date_time<chrono::time::utc::time_point>(temp_dtf);
				}
				break;

		case	eORIGIN_TIME_TYPE::YEAR_BEGIN:
				{
					auto temp_dtf = chrono::to_date_time(time_begin);

					temp_dtf.t_fraction = 0;
					temp_dtf.t_second = 0;
					temp_dtf.t_minute = 0;
					temp_dtf.t_hour = 0;
					temp_dtf.t_day = 1;
					temp_dtf.t_month = 1;

					time_begin = chrono::from_date_time<chrono::time::utc::time_point>(temp_dtf);
				}
				break;

		default:
				break;
		}

		// 5) 계산된 시간을 현시점으로 말아버린다.
		while ((time_begin + duration_recreation) < time_now)
		{
			time_begin += duration_recreation;
		}

		// declare)
		auto temp_tm = chrono::to_tm(time_begin);

		// declare)
		std::string temp_file_name;

		try
		{
			// 13) File이름을 설정한다.
			temp_file_name = std::vformat(filename, std::make_format_args(
				temp_tm.tm_year,
				temp_tm.tm_mon,
				temp_tm.tm_mday,
				temp_tm.tm_hour,
				temp_tm.tm_min,
				temp_tm.tm_sec));
		}
		catch (std::format_error& _e)
		{
			throw throwable::failure(eRESULT::FORMAT_ERROR, 0, std::string(_e.what()));
		}

		// 14) get fullpath
		auto str_full_path = api::file::make_full_path(path, temp_file_name);

		// 16) open the file
		LOG_INFO << "  - try opening file '"sv << str_full_path << "'"sv;
		api::file::create_parent_directories(str_full_path);
		{
			// lock)
			std::lock_guard lock(this->m_lockable_file);

			// - close the file opened before
			this->_process_close_file();

			// - open (create new : only this time, it is appended when it is opened again)
			this->m_full_path_now = str_full_path;
			this->m_file_operation = (is_create_new) ? eFILE_OPERATION::WRITE : eFILE_OPERATION::APPEND;
			THROW_IF(this->_process_open_file() == false, throwable::failure(eRESULT::FAIL_TO_OPEN), LOG_ERROR << "@ (err) fail to open log file '"sv << str_full_path << "'"sv);

			// - instant open : closed until the first write
			if (is_instant_open)
				this->_process_close_file();

			// - enabled
			this->m_is_file_enabled = true;
		}

		// 19) File을 setting한다.
		SCOPED_LOCK(this->m_lockable_file)
		{
			// - 기타 값을 설정한다.
			this->m_path = path;
			this->m_is_instant_open = is_instant_open;
			this->m_instant_close_delay = instant_close_delay;
			this->m_path_backup = path_backup;
			this->m_filename = filename;
			this->m_recreation_last = time_begin;
			this->m_recreation_duration = duration_recreation;
			this->m_recreation_bytes_limit = recreation_limit_bytes;
			this->m_recreation_lines_limit = recreation_limit_lines;
			this->m_char_encoding = char_encoding;
		}

		// Log A) 최종 설정된 사항들을 출력한다.()
		LOG_INFO << "  - path: "sv << path;
		LOG_INFO << "  - backup path: "sv << path_backup;
		LOG_INFO << "  - file_name: "sv << filename;
		LOG_INFO << "  - begin_time: "sv << to_string(time_begin);
		LOG_INFO << "  - origin: "sv << to_string(origin_time_type);

		// Log B) origin_time이 SPECIFIED이면 origina_time을 출력해준다.
		if (origin_time_type == eORIGIN_TIME_TYPE::SPECIFIED)
		{
			LOG_INFO << "  - origin_time: "sv << to_string(time_begin);
		}

		if (duration_recreation.count() != 0)
		{
			LOG_INFO << "  - recreation by time: enabled"sv;
			LOG_INFO << "  - interval: "sv << std::chrono::duration_cast<chrono::minutes>(duration_recreation).count() << " minutes"sv;
		}
		else
		{
			LOG_INFO << "  - recreation by time: disabled"sv;
		}

		LOG_INFO << "  - char_encoding: "sv << to_string(char_encoding);
	}
	else
	{
		// log) 
		LOG_INFO << log_level_t::important() << "  ! log file disabled "sv;

		SCOPED_LOCK(this->m_lockable_file)
		{
			this->m_is_file_enabled = false;

			// - 기타 값을 설정한다.
			this->m_path = path;
			this->m_is_instant_open = is_instant_open;
			this->m_instant_close_delay = instant_close_delay;
			this->m_path_backup = path_backup;
			this->m_filename = filename;
			this->m_recreation_last = time_begin;
			this->m_recreation_duration = duration_recreation;
			this->m_recreation_bytes_limit = recreation_limit_bytes;
			this->m_recreation_lines_limit = recreation_limit_lines;
			this->m_char_encoding = char_encoding;

		}
	}

	// log) reset log count
	this->reset_log_count();

	// trace) Event Manager를 생성한다.
	LOG_PROGRESS << "@ <END> initializing LOGGER_FILE["sv << name() << "]"sv;

	// return)
	return eRESULT::SUCCESS;
}

result_code logger_file::_process_destroying() noexcept
{
	// 1) Default Tracer로 설정되어 있다면 제거한다.
	reset_default_logger(this);

	// 2) detach from parent logger
	this->detach_from_parent();

	try
	{
		// 3) on_destroying 함수를 호출한다.
		this->on_destroying();
	}
	catch (...)
	{
		// Log) 
		LOG_ERROR << "@ (excp) Exception on 'on_destroying' function call ['"sv << name() << "']"sv;
	}

	// 3) wait for the completion of the queued logs
	this->_process_wait_complete();

	// 4) close the file
	{
		// lock)
		std::lock_guard lock(this->m_lockable_file);

		this->_process_close_file();
		this->m_is_file_enabled = false;
	}

	// return)
	return eRESULT::SUCCESS;
}

void logger_file::_process_recreate_file()
{
	// - called with m_lockable_file locked

	// check)
	if (this->m_is_file_enabled == false)
		return;

	// 2) 값을 얻어 놓는다.
	auto recreation_last = this->m_recreation_last;
	auto recreation_duration = this->m_recreation_duration;
	auto time_now = chrono::time::utc::now();

	// 3) 시간을 읽는다.
	while ((recreation_last + recreation_duration) < time_now)
		recreation_last += recreation_duration;

	// 4) get time
	auto temp_tm = chrono::to_tm(recreation_last);

	// 5) filename for backup
	std::string filename_backup = std::vformat(this->m_filename, std::make_format_args(
		temp_tm.tm_year,
		temp_tm.tm_mon,
		temp_tm.tm_mday,
		temp_tm.tm_hour,
		temp_tm.tm_min,
		temp_tm.tm_sec));

	// 6) backup file
	auto str_full_path = api::file::make_full_path(this->m_path, filename_backup);

	// 7) backup file path
	auto str_full_path_backup = (!this->m_path_backup.empty())
		? api::file::make_full_path(this->m_path_backup, filename_backup)
		: str_full_path;

	// 8) close the file
	this->_process_close_file();

	// 9) move file to backup path
	if (!this->m_path_backup.empty() && str_full_path != str_full_path_backup)
	{
		try
		{
			api::file::create_parent_directories(str_full_path_backup);

			// - a backup with the same name exists (the file name has no time) : a new name ("name_1.ext", ...)
			std::filesystem::path path_backup = str_full_path_backup;
			for (int i = 1; std::filesystem::exists(path_backup) && i < 100000; ++i)
			{
				const std::filesystem::path path_origin = str_full_path_backup;
				path_backup = path_origin.parent_path() / (path_origin.stem().string() + "_" + std::to_string(i) + path_origin.extension().string());
			}

			std::filesystem::rename(str_full_path, path_backup);
			LOG_INFO << "  - backup file moved from '"sv << str_full_path << "' to '"sv << path_backup.string() << "'"sv;
		}
		catch (const std::exception& _e)
		{
			LOG_WARNING << "  (!) fail to move backup file: "sv << _e.what();
		}
		catch (...)
		{
			LOG_WARNING << "  (!) fail to move backup file: unknown error"sv;
		}
	}

	// 10) get filename for new log file
	std::string filename = std::vformat(this->m_filename, std::make_format_args(
		temp_tm.tm_year,
		temp_tm.tm_mon,
		temp_tm.tm_mday,
		temp_tm.tm_hour,
		temp_tm.tm_min,
		temp_tm.tm_sec));

	// 11) get fullpath
	auto str_full_path_new = api::file::make_full_path(this->m_path, filename);

	// log)
	LOG_INFO << "  - try recreation file '"sv << str_full_path_new << "'"sv;

	// 12) file restart
	api::file::create_parent_directories(str_full_path_new);
	this->m_full_path_now = str_full_path_new;
	this->m_file_operation = eFILE_OPERATION::APPEND;
	if (this->m_is_instant_open == false)
		this->_process_open_file();

	// 13) Reset
	this->m_recreation_last = recreation_last;
	this->m_recreation_bytes_now = 0;
	this->m_recreation_lines_now = 0;
}

void logger_file::_process_append_EOL(shared_buffer& _buf_target, eEOL_TYPE _EOL_type)
{
	// 설명) End Of Line 추가.
	switch (_EOL_type)
	{
	case	eEOL_TYPE::NONE:
			break;

	case	eEOL_TYPE::CRLF:
			_buf_target.append<uint16_t>(uint16_t('\r') | (uint16_t('\n') << 8));
			break;

	case	eEOL_TYPE::LF:
			_buf_target.append<char>('\n');
			break;

	case	eEOL_TYPE::CR:
			_buf_target.append<char>('\r');
			break;

	case	eEOL_TYPE::NULL_:
			_buf_target.append<char>(0);
			break;

	case	eEOL_TYPE::CR_NULL:
			_buf_target.append<char>('\r');
			_buf_target.append<char>(0);
			break;

	default:
			CGASSERT_ERROR(false);
			_buf_target.append<char>(0);
			break;
	}
}
bool logger_file::_process_get_parameter_enabled(context& _context)
{
	// declare) 
	bool enabled = true;

	// - 'disable'을 얻는다.
	auto& context_value = _context["disable"];

	if (context_value.is_valid())
	{
		// - get string
		std::string str_value = context_value;

		// - to lower case
		auto value = __to_lower(std::string_view(str_value));

		// - 'true'인가를 검사한다.
		if (value == "true")
		{
			// - enable_reaction을 true로 설정한다.
			enabled = true;

			// trace) 
			LOG_INFO << "  + parameter['disable']: 'true'"sv;
		}
		// - 'enable'인가를 검사한다.
		else if (value == "enable")
		{
			// - enable_reaction을 true로 설정한다.
			enabled = true;

			// trace) 
			LOG_INFO << "  + parameter['disable']: 'enable'=('true')"sv;
		}
		// - '1'인가를 검사한다.
		else if (value == "1")
		{
			// - enable_reaction을 true로 설정한다.
			enabled = true;

			// trace) 
			LOG_INFO << "  + parameter['disable']: 1(='true')"sv;
		}
		// - 'false'인가를 검사한다.
		else if (value == "false")
		{
			// - enable_reaction을 false로 설정한다.
			enabled = false;

			// trace) 
			LOG_INFO << "  + parameter['disable']: 'false'"sv;
		}
		// - 'disable'인가를 검사한다.
		else if (value == "disable")
		{
			// - enable_reaction을 false로 설정한다.
			enabled = false;

			// trace) 
			LOG_INFO << "  + parameter['disable']: disable(='false')"sv;
		}
		// - '0'인가를 검사한다.
		else if (value == "0")
		{
			// - enable_reaction을 false로 설정한다.
			enabled = false;

			// trace) 
			LOG_INFO << "  + parameter['disable']: 0(='false')"sv;
		}
	}

	// return)
	return enabled;
}

std::string logger_file::_process_get_parameter_path(context& _context)
{
	// declare)
	std::string path = "./"s;

	// 1) 'path'을 얻는다.
	std::string str_value = _context["path"];

	if (str_value.empty() == false)
	{
		path = std::move(str_value);
	}

	// return)
	return path;
}

std::string logger_file::_process_get_parameter_path_backup(context& _context)
{
	return _context["backup_path"];
}

std::string logger_file::_process_get_parameter_filename(context& _context)
{
	// declare)
	std::string filename;

	// - 'filename'을 얻는다.
	auto& context_value = _context["filename"];

	if (context_value.is_valid())
	{
		// - get string
		std::string str_value = context_value;

		// - %가 6개 이하여야 하며 %d만 허용됨.
		int countP = 0;
		auto iter = str_value.data();
		auto iter_end = str_value.data() + str_value.size();

		for (; iter != iter_end; ++iter)
		{
			if (*iter == '{')
			{
				++countP;
			}
		}

		// check) 
		THROW_IF(countP > 6, throwable::failure(eRESULT::FAIL), LOG_ERROR << "  [Err] parameter count of Filenames is less then 6"sv);

		// - 이름을 저장한다.
		filename = std::move(str_value);
	}

	// check)
	if (filename.empty())
	{
		// - set filename
		filename = "log@{:04}{:02}{:02}_{:02}{:02}{:02}.log"sv;

		// log)
		LOG_WARNING << "  (!) 'filename' is empty (set filename as '"sv << filename << "')"sv;
	}


	// return)
	return filename;
}

chrono::time::duration logger_file::_process_get_parameter_interval(context & _context)
{
	// declare) 
	chrono::time::duration duration_recreation = chrono::hours(24);

	// - 'Interval'을 얻는다.
	auto& context_value = _context.find("interval");

	if (context_value.is_valid())
	{
		// - get string
		std::string str_value = context_value;

		// - Interval을 얻는다.
		int interval = 0;
		std::from_chars(str_value.data(), str_value.data() + str_value.size(), interval);

		// - 최하 1분
		SATURATE_MIN(interval, 1);

		// - Duration을 설정한다.
		duration_recreation = chrono::minutes(interval);
	}

	// return)
	return duration_recreation;
}

eORIGIN_TIME_TYPE logger_file::_process_get_parameter_origin_time_type(context& _context)
{
	// declare) 
	eORIGIN_TIME_TYPE origin_time_type = eORIGIN_TIME_TYPE::NOW;

	// - 'Interval'을 얻는다.
	auto& context_value = _context["origin_time_type"];

	if (context_value.is_valid())
	{
		// - get string
		std::string str_value = context_value;

		// - to lower case
		auto value = __to_lower(std::string_view(str_value));

		// - 'now'로 설정한다.
		if (value == "now"sv)
		{
			// - Origin is 'eORIGIN_TIME_TYPE::NOW'
			origin_time_type = eORIGIN_TIME_TYPE::NOW;

			// trace)
			LOG_INFO << "  + parameter['origin']: 'now'"sv;
		}
		// - 'minute_begin'로 설정한다.
		else if (value == "minute_begin"sv)
		{
			// - Origin is 'eORIGIN_TIME_TYPE::MINUTE_BEGIN'
			origin_time_type = eORIGIN_TIME_TYPE::MINUTE_BEGIN;

			// trace)
			LOG_INFO << "  + parameter['origin']: 'minute_begin'"sv;
		}
		// - 'hour_begin'로 설정한다.
		else if (value == "hour_begin"sv)
		{
			// - Origin is 'eORIGIN_TIME_TYPE::HOUR_BEGIN'
			origin_time_type = eORIGIN_TIME_TYPE::HOUR_BEGIN;

			// trace)
			LOG_INFO << "  + parameter['origin']: 'hour_begin'"sv;
		}
		// - 'day_begin'로 설정한다.
		else if (value == "day_begin"sv)
		{
			// - Origin is 'eORIGIN_TIME_TYPE::DAY_BEGIN'
			origin_time_type = eORIGIN_TIME_TYPE::DAY_BEGIN;

			// trace)
			LOG_INFO << "  + parameter['origin']: 'day_begin'"sv;
		}
		// - 'week_begin'로 설정한다.
		else if (value == "week_begin"sv)
		{
			// - Origin is 'eORIGIN_TIME_TYPE::WEEK_BEGIN'
			origin_time_type = eORIGIN_TIME_TYPE::WEEK_BEGIN;

			// trace)
			LOG_INFO << "  + parameter['origin']: 'week_begin'"sv;
		}
		// - 'month_begin'로 설정한다.
		else if (value == "month_begin"sv)
		{
			// - Origin is 'eORIGIN_TIME_TYPE::MONTH_BEGIN'
			origin_time_type = eORIGIN_TIME_TYPE::MONTH_BEGIN;

			// trace)
			LOG_INFO << "  + parameter['origin']: 'month_begin'"sv;
		}
		// - 'year_begin'로 설정한다.
		else if (value == "year_begin"sv)
		{
			// - Origin is 'eORIGIN_TIME_TYPE::YEAR_BEGIN'
			origin_time_type = eORIGIN_TIME_TYPE::YEAR_BEGIN;

			// trace)
			LOG_INFO << "  + parameter['origin']: 'year_begin'"sv;
		}
		// - 'default'로 설정한다.
		else if (value == "default"sv)
		{
			// - Origin is 'eORIGIN_TIME_TYPE::DEFAULT'
			origin_time_type = eORIGIN_TIME_TYPE::DEFAULT;

			// trace)
			LOG_INFO << "  + parameter['origin']: 'default'"sv;
		}
		// - 'default'로 설정한다.
		else if (value == "specified"sv)
		{
			// - Origin is 'eORIGIN_TIME_TYPE::DEFAULT'
			origin_time_type = eORIGIN_TIME_TYPE::SPECIFIED;

			// trace)
			LOG_INFO << "  + parameter['origin']: 'specified'"sv;
		}
		else
		{
			// - Origin is 'eORIGIN_TIME_TYPE::DEFAULT'
			origin_time_type = eORIGIN_TIME_TYPE::DEFAULT;
		}
	}

	// return)
	return origin_time_type;
}

chrono::time::utc::time_point logger_file::_process_get_parameter_origin_time(context& _context, chrono::time::utc::time_point _time_now, eORIGIN_TIME_TYPE _origin_time_type, std::string _filename)
{
	// declare) 
	chrono::time::utc::time_point time_begin = _time_now;

	// 1) 'original_time'을 얻는다.
	auto& context_value = _context["origin_time"];

	if (context_value.is_valid())
	{
		// - get string
		std::string str_value = context_value;

		// - convert to chrono::time::utc::time_point
		time_begin = from_string<chrono::time::utc::time_point>(str_value);

		// check) Origin_type이 SPECIFIED가 아닐 경우 워닝으르 출력한다.
		if (_origin_time_type != eORIGIN_TIME_TYPE::SPECIFIED)
			LOG_WARNING << "  (!) 'origin_time' is ignored (origin_time_type is not 'specified') filename='"sv << _filename << "'";
	}

	// return)
	return time_begin;
}

//bool logger_file::_process_get_parameter_enable_recreation(context& _context)
//{
//	// declare)
//	bool enable_recreation = false;
//
//	// - 'recreation'을 얻는다.
//	auto& context_value = _context["recreation"];
//
//	if (context_value.is_valid())
//	{
//		// - get string
//		std::string str_value = context_value;
//
//		// - to lower case
//		auto value = __to_lower(std::string_view(str_value));
//
//		// - 'true'인가를 검사한다.
//		if (value == "true"sv)
//		{
//			// - enable_reaction을 true로 설정한다.
//			enable_recreation = true;
//
//			// trace) 
//			LOG_INFO << "  + parameter['recreation']: 'true'"sv;
//		}
//		// - 'enable'인가를 검사한다.
//		else if (value == "enable"sv)
//		{
//			// - enable_reaction을 true로 설정한다.
//			enable_recreation = true;
//
//			// trace) 
//			LOG_INFO << "  + parameter['recreation']: 'enable'=('true')"sv;
//		}
//		// - '1'인가를 검사한다.
//		else if (value == "1"sv)
//		{
//			// - enable_reaction을 true로 설정한다.
//			enable_recreation = true;
//
//			// trace) 
//			LOG_INFO << "  + parameter['recreation']: 1(='true')"sv;
//		}
//		// - 'false'인가를 검사한다.
//		else if (value == "false"sv)
//		{
//			// - enable_reaction을 false로 설정한다.
//			enable_recreation = false;
//
//			// trace) 
//			LOG_INFO << "  + parameter['recreation']: 'false'"sv;
//		}
//		// - 'disable'인가를 검사한다.
//		else if (value == "disable"sv)
//		{
//			// - enable_reaction을 false로 설정한다.
//			enable_recreation = false;
//
//			// trace) 
//			LOG_INFO << "  + parameter['recreation']: disable(='false')"sv;
//		}
//		// - '0'인가를 검사한다.
//		else if (value == "0"sv)
//		{
//			// - enable_reaction을 false로 설정한다.
//			enable_recreation = false;
//
//			// trace) 
//			LOG_INFO << "  + parameter['recreation']: disable(='false')"sv;
//		}
//	}
//	else
//	{
//		// trace) 
//		LOG_INFO << "  + parameter['recreation']: default(disable)"sv;
//	}
//
//	// return)
//	return enable_recreation;
//}

uint64_t logger_file::_process_get_parameter_recreation_bytes_limit(context& _context)
{
	// declare) 
	uint64_t recreation_limit_bytes{ 0 };

	// - 'Interval'을 얻는다.
	auto& context_value = _context.find("recreation_bytes_limit", "bytes_limit", "limit_bytes");

	if (context_value.is_valid())
	{
		// - get string
		std::string str_value = context_value;

		// - Interval을 얻는다.
		int64_t temp_value = 0;
		std::from_chars(str_value.data(), str_value.data() + str_value.size(), temp_value);

		// - 음수 방지: 최소값을 0으로 설정
		SATURATE_MIN(temp_value, 0);

		// - 값 걸정
		recreation_limit_bytes = static_cast<uint64_t>(temp_value);
	}

	// return)
	return recreation_limit_bytes;
}

uint64_t logger_file::_process_get_parameter_recreation_lines_limit(context& _context)
{
	// declare) 
	uint64_t recreation_limit_lines{ 0 };

	// - 'Interval'을 얻는다.
	auto& context_value = _context.find("recreation_limit_lines", "lines_limit", "limit_lines");

	if (context_value.is_valid())
	{
		// - get string
		std::string str_value = context_value;

		// - Interval을 얻는다.
		int64_t temp_value = 0;
		std::from_chars(str_value.data(), str_value.data() + str_value.size(), temp_value);

		// - 음수 방지: 최소값을 0으로 설정
		SATURATE_MIN(temp_value, 0);

		// - 값 걸정
		recreation_limit_lines = static_cast<uint64_t>(temp_value);
	}

	// return)
	return recreation_limit_lines;
}

bool logger_file::_process_get_parameter_create_new(context& _context)
{
	// declare) 
	bool is_create_new = false;

	// - 'recreation'을 얻는다.
	auto& context_value = _context["create_new"];

	if (context_value.is_valid())
	{
		// - get string
		std::string str_value = context_value;

		// - to lower case
		auto value = __to_lower(std::string_view(str_value));

		// - 'true'인가를 검사한다.
		if (value == "true")
		{
			// - is_create_new을 true로 설정한다.
			is_create_new = true;

			// trace) 
			LOG_INFO << "  + parameter['create_new']: 'true'"sv;
		}
		// - 'enable'인가를 검사한다.
		else if (value == "enable"sv)
		{
			// - is_create_new을 true로 설정한다.
			is_create_new = true;

			// trace) 
			LOG_INFO << "  + parameter['create_new']: 'enable'=('true')"sv;
		}
		// - '1'인가를 검사한다.
		else if (value == "1"sv)
		{
			// - is_create_new을 true로 설정한다.
			is_create_new = true;

			// trace) 
			LOG_INFO << "  + parameter['create_new']: 1(='true')"sv;
		}
		// - 'false'인가를 검사한다.
		else if (value == "false"sv)
		{
			// - is_create_new을 false로 설정한다.
			is_create_new = false;

			// trace) 
			LOG_INFO << "  + parameter['create_new']: 'false'"sv;
		}
		// - 'disable'인가를 검사한다.
		else if (value == "disable"sv)
		{
			// - is_create_new을 false로 설정한다.
			is_create_new = false;

			// trace) 
			LOG_INFO << "  + parameter['create_new']: disable(='false')"sv;
		}
		// - '0'인가를 검사한다.
		else if (value == "0"sv)
		{
			// - is_create_new을 false로 설정한다.
			is_create_new = false;

			// trace) 
			LOG_INFO << "  + parameter['create_new']: 0(='false')"sv;
		}
	}

	// return)
	return is_create_new;
}
eCHARACTER_SET logger_file::_process_get_parameter_char_encoding(context& _context)
{
	eCHARACTER_SET char_encoding = _DEFAULT_CHAR_CHARACTER_SET;

	// - character_set type을 읽어들인다.
	auto& context_value = _context.find("character_set"sv, "character set"sv, "  "sv, "char_set"sv, "char set"sv, "charset"sv);

	if (context_value.is_valid())
	{
		// - get string
		std::string str_value = context_value;

		// - to lower case
		str_value = __to_lower(std::string_view(str_value));

		// - 'mbcs'인지 확인한다.
		if (str_value == "mbcs"sv || str_value == "euckr"sv)
		{
			// - char_encoding is 'eCHARACTER_SET::MBCS'
			char_encoding = eCHARACTER_SET::MBCS;

			// trace)
			LOG_INFO << "  + parameter['char_encoding']: 'euckr(mbcs)'"sv;
		}
		// - 'windows_unicode'인지 확인한다.
		else if (str_value == "windows_unicode"sv || str_value == "ucs2"sv)
		{
			// - char_encoding is 'eCHARACTER_SET::WINDOWS_UNICODE'
			char_encoding = eCHARACTER_SET::UCS2;

			// trace)
			LOG_INFO << "  + parameter['character_set']: 'ucs2(windows_unicode)'"sv;
		}
		// - 'utf-8'인지 확인한다.
		else if (str_value == "utf8"sv)
		{
			// - char_encoding is 'eCHARACTER_SET::UTF8'
			char_encoding = eCHARACTER_SET::UTF8;

			// trace)
			LOG_INFO << "  + parameter['char_encoding']: 'utf8'"sv;
		}
		// - 'default'로 설정한다.
		else if (str_value == "default"sv)
		{
			// - char_encoding is 'default'
			char_encoding = _DEFAULT_CHAR_CHARACTER_SET;

			// trace)
			LOG_INFO << "  + parameter['character_set']: 'default'"sv;
		}
	}

	// return)
	return char_encoding;
}
int logger_file::_process_get_parameter_filter(context& _context)
{
	// declare)
	int filter = 0;

	// - 'filter'을 얻는다.
	auto str_filter = _context["filter"sv];

	if (str_filter.is_valid())
	{

	}
	else
	{
		// trace)
		LOG_INFO << "  + parameter['filter']: none'"sv;
	}

	// return)
	return filter;
}
bool logger_file::_process_get_parameter_is_instant_open(context& _context)
{
	// declare)
	bool is_instant_open = false;

	// - "instant_open"키에 대응하는 값을 얻는다.
	auto& context_instant_open = _context.find("instant_open"sv, "instant open"sv);

	// - "instant_open"키에 대응하는 값을 얻었다면...
	if (context_instant_open.is_valid())
	{
		// - get string
		std::string str_value = context_instant_open;

		// - to lower case
		auto value = __to_lower(std::string_view(str_value));

		if (value == "true")
		{
			is_instant_open = true;

			// trace)
			LOG_INFO << "  + parameter['instant_open']: true"sv;
		}
		else if (value == "false")
		{
			is_instant_open = false;

			// trace)
			LOG_INFO << "  + parameter['instant_open']: false"sv;
		}
		else if (value == "1")
		{
			is_instant_open = true;

			// trace)
			LOG_INFO << "  + parameter['instant_open']: true"sv;
		}
		else if (value == "0")
		{
			is_instant_open = false;

			// trace)
			LOG_INFO << "  + parameter['instant_open']: false"sv;
		}
		else if (value == "default")
		{
			is_instant_open = false;

			// trace)
			LOG_INFO << "  + parameter['instant_open']: default(false)"sv;
		}
	}
	else
	{
		// trace)
		LOG_INFO << "  + parameter['instant_open']: default(false)"sv;
	}

	// return)
	return is_instant_open;
}

std::chrono::milliseconds logger_file::_process_get_parameter_instant_close_delay(context& _context)
{
	// declare)
	std::chrono::milliseconds delay{ 100 };

	// - "instant_close_delay" (ms)
	auto& context_value = _context.find("instant_close_delay"sv, "instant close delay"sv);

	if (context_value.is_valid())
	{
		// - get string
		std::string str_value = context_value;

		// - value (0 or more)
		int64_t temp_value = 0;
		std::from_chars(str_value.data(), str_value.data() + str_value.size(), temp_value);
		SATURATE_MIN(temp_value, 0);
		delay = std::chrono::milliseconds(temp_value);

		// trace)
		LOG_INFO << "  + parameter['instant_close_delay']: "sv << temp_value << "ms"sv;
	}
	else
	{
		// trace)
		LOG_INFO << "  + parameter['instant_close_delay']: default(100ms)"sv;
	}

	// return)
	return delay;
}

namespace
{
	// - size of the text buffer (the logs are appended until it is full)
	constexpr std::size_t LOG_FILE_TEXT_BUFFER = 1024 * 1024;
}

void logger_file::process_log_batch(std::vector<object_ptr<LOG_RECORD>>& _vector_plog_record)
{
	// declare)
	bool result = true;

	try
	{
		// check)
		if(this->m_is_file_enabled == false)
			return;

		// 1) size of the text of a log (upper bound : converted message + date or indent + EOL for every line)
		auto size_of = [this](const LOG_RECORD& _record) noexcept
		{
			return _record.buf_message.size() * __get_convert_ratio(this->m_char_encoding, _record.char_encoding) * __get_unit_size(this->m_char_encoding) + (_record.line_count() + 1) * 32;
		};

		// Pre-check recreation limits once per batch (avoid repeated atomic loads in loop)
		const auto bytes_now = this->m_recreation_bytes_now.load(std::memory_order_relaxed);
		const auto lines_now = this->m_recreation_lines_now.load(std::memory_order_relaxed);
		const bool has_bytes_limit = (this->m_recreation_bytes_limit != 0);
		const bool has_lines_limit = (this->m_recreation_lines_limit != 0);

		// 2) append the text of every log
		for (auto& iter : _vector_plog_record)
		{
			// check) message must be not empty
			if(iter.empty() || iter->buf_message.empty())
				continue;

			// - filtering
			if (auto pfilter = this->_get_filter(); pfilter != nullptr)
				if(pfilter->process_filtering(iter) == false)
					continue;

			// statistics) log count
			this->process_count_log(iter->type);

			// - size of the text
			const auto size_text = size_of(*iter);

			// - write the text first (optimized: use pre-cached limit checks)
			//   (no room, or this log reaches the recreation limit)
			if (this->m_buf_text.size() != 0
			 && (this->m_buf_text.get_remained_size() < size_text
			  || (has_lines_limit && lines_now + this->m_lines_text + 1 >= this->m_recreation_lines_limit)
			  || (has_bytes_limit && bytes_now + this->m_buf_text.size() + size_text >= this->m_recreation_bytes_limit)))
			{
				if (this->_process_write_text() == false)
					result = false;
			}

			// - no room : the text is written first (the buffer is reused), a larger buffer for a large log
			if (this->m_buf_text.get_remained_size() < size_text)
			{
				if (this->_process_write_text() == false)
					result = false;

				if (this->m_buf_text.get_remained_size() < size_text)
					this->m_buf_text = alloc_shared_buffer((std::max)(size_text, LOG_FILE_TEXT_BUFFER));
			}

			// - append log text
			this->process_log_write(this->m_buf_text, iter);
			++this->m_lines_text;
		}

		// 3) write
		if (this->_process_write_text() == false)
			result = false;
	}
	catch (std::exception& _e)
	{
		// - reset the text
		this->m_buf_text = shared_buffer();
		this->m_lines_text = 0;

		// log)
		LOG_ERROR << "@ (excp) fail to writing log to file("sv << _e.what() << "), logger: '"sv << this->name() << "' filename:'["sv << this->m_path << this->m_filename << "'";
		return;
	}
	catch (...)
	{
		// - reset the text
		this->m_buf_text = shared_buffer();
		this->m_lines_text = 0;

		// log)
		LOG_ERROR << "@ (excp) fail to writing by unknowned exception on writing log to file, logger:'"sv << this->name() << "' filename:'"sv << this->m_path << this->m_filename << "'";
		return;
	}

	// check) the error is logged only when the writing fails first
	//        (the error log comes back to this logger : not for every batch while failing)
	if (result == false)
	{
		if (this->m_is_write_failed == false)
		{
			this->m_is_write_failed = true;
			LOG_ERROR << "@ (excp) fail to writing log to file, logger: '"sv << this->name() << "' filename: '"sv << this->m_path << this->m_filename << "'";
		}
	}
	else
	{
		this->m_is_write_failed = false;
	}
}

bool logger_file::_process_write_text()
{
	// - called on the execution

	// check)
	if (this->m_buf_text.size() == 0)
		return true;

	// declare)
	const auto lines = std::exchange(this->m_lines_text, 0);
	bool result = false;

	// Fast-path check: disabled loggers skip lock entirely
	if (this->m_is_file_enabled == false)
	{
		this->m_buf_text.set_size(0);
		return true;
	}

	// Fast-path: check recreation limits with atomic reads (no lock)
	auto is_recreate{ false };
	const auto size_write = this->m_buf_text.size();

	// check) BY_TIME
	if (this->m_recreation_duration.count() != 0 && this->m_recreation_last + this->m_recreation_duration < chrono::time::utc::now())
		is_recreate = true;
	// check) BY_BYTES (atomic read)
	else if (this->m_recreation_bytes_limit != 0 && this->m_recreation_bytes_now.load(std::memory_order_relaxed) + size_write >= this->m_recreation_bytes_limit)
		is_recreate = true;
	// check) BY_LINES (atomic read)
	else if (this->m_recreation_lines_limit != 0 && this->m_recreation_lines_now.load(std::memory_order_relaxed) + lines >= this->m_recreation_lines_limit)
		is_recreate = true;

	{
		// lock) - minimize critical section: only protect file I/O and state updates
		std::lock_guard lock(this->m_lockable_file);

		// Re-check enabled (may have changed during lock contention)
		if (this->m_is_file_enabled == false)
		{
			this->m_buf_text.set_size(0);
			return true;
		}

		// 1) recreate the file if needed (recheck with lock to avoid race)
		if (is_recreate)
			this->_process_recreate_file();

		// 2) open (instant open, or failed before)
		if (this->m_handle_file == INVALID_FILE_HANDLE)
			this->_process_open_file();

		// 3) write the whole text at once
		//    (synchronous : on the execution of this logger, the callers never wait)
		if (this->m_handle_file != INVALID_FILE_HANDLE)
		{
			result = (api::file::write(this->m_handle_file, size_write, this->m_buf_text.data()) == size_write);

			// Update counts with atomic operations to allow lock-free reads
			this->m_recreation_bytes_now.fetch_add(size_write, std::memory_order_relaxed);
			this->m_recreation_lines_now.fetch_add(lines, std::memory_order_relaxed);
		}

		// 4) instant open : closed when no write for 'instant_close_delay' (now if no delay)
		if (this->m_is_instant_open)
		{
			if (this->m_instant_close_delay.count() <= 0)
			{
				this->_process_close_file();
			}
			else
			{
				this->m_tick_last_write = chrono::tick::now();
				if (this->m_is_close_scheduled == false)
					this->_process_post_check_close(std::chrono::duration_cast<chrono::tick::duration>(this->m_instant_close_delay));
			}
		}
	}

	// 5) reuse the buffer (outside lock)
	this->m_buf_text.set_size(0);

	// return)
	return result;
}

bool logger_file::_process_open_file()
{
	// - called with m_lockable_file locked

	// check) opened already
	if (this->m_handle_file != INVALID_FILE_HANDLE)
		return true;

	// 1) open
	this->m_handle_file = api::file::open(this->m_full_path_now, this->m_file_operation, eFILE_LOCATION::DEFAULT);

	// check)
	if (this->m_handle_file == INVALID_FILE_HANDLE)
		return false;

	// 2) linux : no buffering of stdio (a write goes to the kernel at once : the text is buffered by this logger already)
#if defined(__linux__) && !defined(__ANDROID__)
	std::setvbuf(this->m_handle_file, nullptr, _IONBF, 0);
#endif

	// 3) the next opening appends (create new : only once)
	this->m_file_operation = eFILE_OPERATION::APPEND;

	// return)
	return true;
}

void logger_file::_process_close_file() noexcept
{
	// - called with m_lockable_file locked

	// check)
	if (this->m_handle_file == INVALID_FILE_HANDLE)
		return;

	// 1) close
	api::file::close(this->m_handle_file);
	this->m_handle_file = INVALID_FILE_HANDLE;
}

void logger_file::_process_post_check_close(chrono::tick::duration _delay)
{
	// - called with m_lockable_file locked
	try
	{
		object_ptr<logger_file> pthis(this);
		this->m_is_close_scheduled = system_executor::post(make_executable([pthis]() { pthis->_process_check_close(); }), _delay);
	}
	catch (...)
	{
		this->m_is_close_scheduled = false;
	}

	// check) fail to schedule : closes now
	if (this->m_is_close_scheduled == false)
		this->_process_close_file();
}

void logger_file::_process_check_close()
{
	// lock)
	std::lock_guard lock(this->m_lockable_file);

	// 1) reset
	this->m_is_close_scheduled = false;

	// check) closed already or not instant open
	if (this->m_is_instant_open == false || this->m_handle_file == INVALID_FILE_HANDLE)
		return;

	// check) written recently : checks again later
	const auto delay = std::chrono::duration_cast<chrono::tick::duration>(this->m_instant_close_delay);
	const auto elapsed = chrono::tick::now() - this->m_tick_last_write;
	if (elapsed < delay)
	{
		this->_process_post_check_close(delay - elapsed);
		return;
	}

	// 2) close
	this->_process_close_file();
}
void logger_file::process_log_write(shared_buffer& _buf_output, const object_ptr<LOG_RECORD>& _log)
{
	// 1) 
	size_t pos = sizeof(COUNT_T);
	size_t count_line = _log->line_count();

	// check)
	CGASSERT_ERROR(count_line != 0);

	// check) 
	if(count_line == 0)
		return;

	// 2) log type
	bool log_not_continue = (uint32_t)(_log->type & log_type_t::continue_) == 0;

	// 3) 날짜를 출력한다.
	if (log_not_continue)
		LOG_RECORD::append_text_date(_buf_output, _log->time_occurred, this->m_char_encoding);

	// 4) 메시지를 작성한다.
	pos = _log->get_log_message(_buf_output, this->m_char_encoding, pos);

	// 5) 터미널 문자를 붙인다.
	_process_append_EOL(_buf_output, this->m_EOL_type);

	// 6) 출력한다.
	for (--count_line; count_line != 0; --count_line)
	{
		// - 빈칸을 출력한다.
		//           "[0000/00/00 00:00:00] "
		_buf_output.append_text("                      ");

		// - get next message
		pos = _log->get_log_message(_buf_output, this->m_char_encoding, pos);

		// - 터미널 문자를 붙인다.
		this->_process_append_EOL(_buf_output, this->m_EOL_type);
	}
}