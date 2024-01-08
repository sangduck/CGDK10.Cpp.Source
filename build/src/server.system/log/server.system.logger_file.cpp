#include "pch.h"

using namespace CGDK;


logger_file::logger_file():	Nlogger()
{
}

logger_file::logger_file(const std::string_view _name) :
	Nlogger(_name)
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

	// 2) Origin type을 써넣는다.
	context_setting["origin_time_type"sv] = to_string<_CHAR>(_initialize_param.origin_time_type);

	// 3) Recreation Option
	if(_initialize_param.recreation_interval != chrono::time::duration::zero())
	{
		context_setting["recreation"sv] = true;
		context_setting["interval"sv] = std::chrono::duration_cast<chrono::minutes>(_initialize_param.recreation_interval).count();
	}

	// 4) character type
	context_setting["char_encoding"sv] = to_string<_CHAR>(_initialize_param.char_code);
	context_setting["eol_type"sv] = to_string<_CHAR>(_initialize_param.eol_type);

	// return) 
	return this->Ninitializable::initialize(context_setting);
}

//result_code logger_file::initialize(const std::string_view _filename, chrono::time::duration _recreation_interval, eORIGIN_TIME_TYPE _begin_type, eCHARACTER_SET _char_code, eEOL_TYPE _EOL_type)
//{
//	// check)
//	CGASSERT_ERROR(_char_code != eCHARACTER_SET::NONE && _char_code <= eCHARACTER_SET::MAX_)
//
//	// check)
//	THROW_IF(_char_code == eCHARACTER_SET::NONE || _char_code > eCHARACTER_SET::MAX_, throwable::failure(eRESULT::INVALID_ARGUMENT));
//
//	// declare)
//	context	context_setting;
//
//	// 1) 값을 설정한다.
//	context_setting["filename"sv] = _filename;
//
//	// 2) Origin type을 써넣는다.
//	context_setting["origin_time_type"sv] = to_string<_CHAR>(_begin_type);
//
//	// 3) Recreation Option
//	if(_recreation_interval != chrono::time::duration::zero())
//	{
//		context_setting["recreation"sv] = true;
//		context_setting["interval"sv] = std::chrono::duration_cast<chrono::minutes>(_recreation_interval).count();
//	}
//
//	// 4) character type
//	context_setting["char_encoding"sv] = to_string<_CHAR>(_char_code);
//	context_setting["eol_type"sv] = to_string<_CHAR>(_EOL_type);
//
//	// return) 
//	return	Ninitializable::initialize(context_setting);
//}
//
//result_code logger_file::initialize(const std::string_view _filename, chrono::time::duration _recreation_interval, const chrono::time::utc::time_point& _origin_time, eCHARACTER_SET _char_code, eEOL_TYPE _EOL_type)
//{
//	// check)
//	CGASSERT_ERROR(_char_code != eCHARACTER_SET::NONE && _char_code <= eCHARACTER_SET::MAX_)
//
//	// check)
//	THROW_IF(_char_code == eCHARACTER_SET::NONE || _char_code > eCHARACTER_SET::MAX_, throwable::failure(eRESULT::INVALID_ARGUMENT));
//
//	// declare)
//	context context_setting;
//
//	// 1) 값을 설정한다.
//	context_setting["Filename"sv] = _filename;
//
//	// 2) Origin type을 써넣는다.
//	context_setting["origin_time_type"sv] = "specified";
//
//	// 3) Recreation Option
//	if(_recreation_interval != chrono::time::duration::zero())
//	{
//		context_setting["recreation"sv] = true;
//		context_setting["interval"sv] = std::chrono::duration_cast<chrono::minutes>(_recreation_interval).count();
//	}
//
//	// 4) Origin시간을 설정한다.
//	if(_origin_time != chrono::time::utc::time_point(chrono::time::duration::zero()))
//	{
//		context_setting["origin_time"sv] = to_string<char>(_origin_time);
//	}
//
//	// 4) character type
//	context_setting["char_encoding"sv] = to_string<_CHAR>(_char_code);
//	context_setting["eol_type"sv] = to_string<_CHAR>(_EOL_type);
//
//	// return) 
//	return Ninitializable::initialize(context_setting);
//}

result_code logger_file::_process_initializing(context& _context)
{
	// trace) 
	LOG_PROGRESS << "@ <BEGIN> initializing LOGGER_FILE["sv << name() << "]"sv;

	// 1) on_initializing 함수를 호출한다.
	this->on_initializing(_context);

	// declare) 
	std::string					path = "./"s;
	std::string					filename;
	bool						enable_recreation = false;
	eORIGIN_TIME_TYPE			origin_time_type = eORIGIN_TIME_TYPE::DEFAULT;
	chrono::time::utc::time_point time_begin;
	chrono::time::duration	duration_recreation = chrono::hours(24);
	eCHARACTER_SET				char_encoding = _DEFAULT_CHAR_CHARACTER_SET;
	bool						is_create_new = false;
	bool						enabled = true;
	bool						is_instant_open = true;

	// 2) 현재 시간을 구한다.
	auto time_now = chrono::time::utc::now();

	// 3) ...
	time_begin = time_now;

	// 4) 'disable'
	{
		// - 'disable'을 얻는다.
		std::string str_value = _context["disable"sv];

		if (str_value.empty() == false)
		{
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
	}

	// 5) path를 읽는다.
	{
		// - 'path'을 얻는다.
		std::string str_value = _context["path"sv];

		if (str_value.empty() == false)
		{
			path = str_value;
		}
	}

	// 6) file name를 읽는다.
	{
		// - 'filename'을 얻는다.
		std::string str_value = _context["filename"sv];

		if (str_value.empty() == false)
		{
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
			filename = str_value;
		}
	}

	// 6) interval
	{
		// - 'Interval'을 얻는다.
		std::string str_value = _context["interval"sv];

		if (str_value.empty() == false)
		{
			// - Interval을 얻는다.
			auto interval = __from_string<int>(str_value.data());

			// - 최하 1분
			SATURATE_MIN(interval, 1);

			// - Duration을 설정한다.
			duration_recreation = chrono::minutes(interval);
		}
	}

	// 7) origin_time_type
	{
		// - 'Interval'을 얻는다.
		std::string str_value = _context["origin_time_type"sv];

		if (str_value.empty() == false)
		{
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
	}

	// 8) Origin_time
	{
		// - 'original_time'을 얻는다.
		std::string str_value = _context["origin_time"sv];

		if (str_value.empty() == false)
		{
			// - convert to chrono::time::utc::time_point
			time_begin = from_string<chrono::time::utc::time_point>(str_value);

			// check) Origin_type이 SPECIFIED가 아닐 경우 워닝으르 출력한다.
			if (origin_time_type != eORIGIN_TIME_TYPE::SPECIFIED)
			{
				LOG_WARNING << "  (!) 'origin_time' is ignored (origin_time_type is not 'specified')"sv << filename;
			}
		}
	}

	// 9) recreation 여부를 읽는다.
	{
		// - 'recreation'을 얻는다.
		std::string str_value = _context["recreation"sv];

		if (str_value.empty() == false)
		{
			// - to lower case
			auto value = __to_lower(std::string_view(str_value));

			// - 'true'인가를 검사한다.
			if (value == "true"sv)
			{
				// - enable_reaction을 true로 설정한다.
				enable_recreation = true;

				// trace) 
				LOG_INFO << "  + parameter['recreation']: 'true'"sv;
			}
			// - 'enable'인가를 검사한다.
			else if (value == "enable"sv)
			{
				// - enable_reaction을 true로 설정한다.
				enable_recreation = true;

				// trace) 
				LOG_INFO << "  + parameter['recreation']: 'enable'=('true')"sv;
			}
			// - '1'인가를 검사한다.
			else if (value == "1"sv)
			{
				// - enable_reaction을 true로 설정한다.
				enable_recreation = true;

				// trace) 
				LOG_INFO << "  + parameter['recreation']: 1(='true')"sv;
			}
			// - 'false'인가를 검사한다.
			else if (value == "false"sv)
			{
				// - enable_reaction을 false로 설정한다.
				enable_recreation = false;

				// trace) 
				LOG_INFO << "  + parameter['recreation']: 'false'"sv;
			}
			// - 'disable'인가를 검사한다.
			else if (value == "disable"sv)
			{
				// - enable_reaction을 false로 설정한다.
				enable_recreation = false;

				// trace) 
				LOG_INFO << "  + parameter['recreation']: disable(='false')"sv;
			}
			// - '0'인가를 검사한다.
			else if (value == "0"sv)
			{
				// - enable_reaction을 false로 설정한다.
				enable_recreation = false;

				// trace) 
				LOG_INFO << "  + parameter['recreation']: disable(='false')"sv;
			}
		}
		else
		{
			// trace) 
			LOG_INFO << "  + parameter['recreation']: default(disable)"sv;
		}
	}

	// 10) create new 여부를 읽는다.
	{
		// - 'recreation'을 얻는다.
		std::string str_value = _context["create_new"sv];

		if (str_value.empty() == false)
		{
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
	}

	// 11) Character code을 읽어들인다.
	{
		// - character_set type을 읽어들인다.
		auto context_value = _context.find("character_set"sv, "character set"sv, "  "sv, "char_set"sv, "char set"sv, "charset"sv);

		// - to lower case
		auto str_value = __to_lower(std::string_view(static_cast<std::string>(context_value)));

		if (str_value.empty() == false)
		{
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
	}

	// 12) Filter를 읽어들인다.
	{
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
	}

	// 13) instant open
	{
		// declare) 
		std::string str_instant_open;

		// - "instant_open"키에 대응하는 값을 얻는다.
		str_instant_open = (std::string)_context.find("instant_open"sv, "instant open"sv);

		// - "instant_open"키에 대응하는 값을 얻었다면...
		if (str_instant_open.empty() == false)
		{
			// - to lower case
			auto value = __to_lower(std::string_view(str_instant_open));

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
				is_instant_open = true;

				// trace)
				LOG_INFO << "  + parameter['instant_open']: default(true)"sv;
			}
		}
		else
		{
			// trace)
			LOG_INFO << "  + parameter['instant_open']: default(true)"sv;
		}
	}

	if (enabled)
	{
		// check) MBCS, WINDOWS_UNICODE, UTF8만 지원한다.
		THROW_IF(char_encoding != eCHARACTER_SET::MBCS && char_encoding != eCHARACTER_SET::UCS2 && char_encoding != eCHARACTER_SET::UTF8, throwable::failure(eRESULT::INVALID_ARGUMENT));

		// declare) 현재 시간을 구한다.
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

		// 12) 계산된 시간을 현시점으로 말아버린다.
		while ((time_begin + duration_recreation) < time_now)
		{
			time_begin += duration_recreation;
		}

		// declare)
		auto temp_tm = chrono::to_tm(time_begin);

		// check)
		if (filename.empty())
		{
			// - set filename
			filename = "log@{:04}{:02}{:02}_{:02}{:02}{:02}.log"sv;

			// log) 
			LOG_WARNING << "  (!) 'filemane' is empty (set filename as '"sv << filename << "')"sv;
		}

		// declare)
		std::string temp_file_name;

		try
		{
			// 13) File이름을 설정한다.
#if defined(__GNUC__) || defined(FMT_FORMAT_H_)
			temp_file_name = NFMT::format(filename,
				temp_tm.tm_year,
				temp_tm.tm_mon,
				temp_tm.tm_mday,
				temp_tm.tm_hour,
				temp_tm.tm_min,
				temp_tm.tm_sec);
#else
			temp_file_name = std::vformat(filename, std::make_format_args(
				temp_tm.tm_year,
				temp_tm.tm_mon,
				temp_tm.tm_mday,
				temp_tm.tm_hour,
				temp_tm.tm_min,
				temp_tm.tm_sec));
#endif
		}
		catch (NFMT::format_error& _e)
		{
			throw throwable::failure(eRESULT::FORMAT_ERROR, 0, std::string(_e.what()));
		}

		// 14) Recreation Next를 설정한다.
		if (enable_recreation == true)
		{
			this->m_recreation_next_time = time_begin + duration_recreation;
		}

		// 15) get fullpath
		auto str_full_path = api::file::make_full_path(path, temp_file_name);

		// 16) Initailization처리...
		auto pfile = make_object<archive::file>();

		// log) 
		LOG_INFO << "  - try opening file '"sv << str_full_path << "'"sv;

		// declaree)
		struct START_PARAMETER
		{
		};

		//  declare)
		archive::file::START_PARAMETER temp_param;

		// 17) start parameter를 설정한다.
		temp_param.filename = str_full_path;
		temp_param.file_operation = (is_create_new) ? eFILE_OPERATION::WRITE_ASYNC : eFILE_OPERATION::APPEND_ASYNC;
		temp_param.file_location = eFILE_LOCATION::DEFAULT;
		temp_param.is_create_new = false;
		temp_param.is_instant_open = is_instant_open;

		// 18) File을 open한다.
		pfile->start(temp_param);

		// 19) File을 setting한다.
		scoped_unique_lock(this->m_lockable_pfile)
		{
			// - 파일 객체를 옮긴다.
			m_pfile = std::move(pfile);

			// - 기타 값을 설정한다.
			this->m_path = path;
			this->m_filename = filename;
			this->m_enable_recreation = enable_recreation;
			this->m_recreation_duration = duration_recreation;
			this->m_char_encoding = char_encoding;
		}

		// Log A) 최종 설정된 사항들을 출력한다.()
		LOG_INFO << "  - path: "sv << path;
		LOG_INFO << "  - file_name: "sv << filename;
		LOG_INFO << "  - begin_time: "sv << to_string<_CHAR>(time_begin);
		LOG_INFO << "  - origin: "sv << to_string<_CHAR>(origin_time_type);

		// Log B) origin_time이 SPECIFIED이면 origina_time을 출력해준다.
		if (origin_time_type == eORIGIN_TIME_TYPE::SPECIFIED)
		{
			LOG_INFO << "  - origin_time: "sv << filename;
		}
		if (enable_recreation)
		{
			LOG_INFO << "  - enable_recreation: enabled"sv;
			LOG_INFO << "  - interval: "sv << std::chrono::duration_cast<chrono::minutes>(m_recreation_duration).count() << " minutes"sv;
		}
		else
		{
			LOG_INFO << "  - enable_recreation: disabled"sv;
		}

		LOG_INFO << "  - char_encoding: "sv << filename;
	}
	else
	{
		// log) 
		LOG_INFO << log_level_t::important() << "  ! log file disabled "sv;

		scoped_unique_lock(this->m_lockable_pfile)
		{
			// - 파일 객체를 옮긴다.
			this->m_pfile.reset();

			// - 기타 값을 설정한다.
			this->m_path = path;
			this->m_filename = filename;
			this->m_enable_recreation = enable_recreation;
			this->m_recreation_duration = duration_recreation;
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

	// 3) destroy처리....
	object_ptr<archive::file> pfile;

	scoped_unique_lock(this->m_lockable_pfile)
	{
		pfile = std::move(this->m_pfile);
	}

	// 4) File을 close한다.
	if (pfile.exist())
	{
		pfile->destroy();
	}

	// return)
	return eRESULT::SUCCESS;
}

void logger_file::process_log(const std::vector<object_ptr<LOG_RECORD>>& _vector_log_record)
{
	// check) _vector_log_record must be not null
	RETURN_IF(_vector_log_record.empty(), );

	// check) 
	RETURN_IF(this->get_object_state() < eOBJECT_STATE::STOPPED || this->get_object_state() > eOBJECT_STATE::RUNNING);

	// check) 
	RETURN_IF(this->m_pfile.empty());

	// 1) calculate memory size
	size_t size_buffer = 0;
	for (auto& iter : _vector_log_record)
	{
		// check)
		CONTINUE_IF(iter.empty());

		// - add size
		size_buffer += iter->buf_message.size() * 4 + 128;
		size_buffer += iter->get_line_count() * 128;
	}

	// 2) alloc shared_buffer for log text
	auto buf_text = alloc_shared_buffer(size_buffer);

	// 3) append log text
	for (auto& iter : _vector_log_record)
	{
		// check)
		CONTINUE_IF(iter.empty());

		// check) message must be not empty
		CONTINUE_IF(iter->buf_message.empty());

		// 1) filtering
		if (this->m_plog_filter.exist())
		{
			CONTINUE_IF(this->m_plog_filter->process_filtering(iter) == false);
		}

		// 2) log type
		bool log_not_continue = (uint32_t)(iter->type & log_type_t::continue_) == 0;

		// statistics) log count
		process_count_log(iter->type);

		// - ...
		size_t pos = sizeof(COUNT_T);
		size_t count_line = iter->get_line_count();

		// check)
		CGASSERT_ERROR(count_line != 0);

		// check) 
		CONTINUE_IF(count_line == 0);

		// 5) write first line
		{
			if (log_not_continue)
			{
				// - 날짜를 출력한다.
				iter->process_append_text_date(buf_text, m_char_encoding);
			}

			// - 메시지를 작성한다.
			pos = iter->process_get_message(buf_text, pos, m_char_encoding);

			// - 터미널 문자를 붙인다.
			_process_append_EOL(buf_text, m_EOL_type);
		}

		// 6) 출력한다.
		for (--count_line; count_line != 0; --count_line)
		{
			// - 빈칸을 출력한다.
			//           "[0000/00/00 00:00:00] "
			buf_text.append_text("                      ");

			// - get next message
			pos = iter->process_get_message(buf_text, pos, m_char_encoding);

			// - 터미널 문자를 붙인다.
			_process_append_EOL(buf_text, m_EOL_type);
		}
	}

	// check) buf_text.size_가 0이면 안됀다.
	CGASSERT_ERROR(buf_text.size() != 0);

	// check) buf_text.size_가 0이면 안됀다.
	RETURN_IF(buf_text.size() == 0);

	// decalre)
	object_ptr<archive::file> pfile;

	// 4) pfile 얻어 오기...
	scoped_unique_lock(this->m_lockable_pfile)
	{
		// check)
		RETURN_IF(this->m_pfile.empty());

		// check)
		if (this->m_enable_recreation == true)
		{
			auto time_now = chrono::time::utc::now();

			if (time_now > this->m_recreation_next_time)
			{
				// - 재성성한다.
				this->_process_recreate_file();
			}
		}

		pfile = m_pfile;
	}

	// 5) write to file
	auto result = pfile->write(buf_text);

	// check) 
	RETURN_IF(result == false);
}

void logger_file::_process_recreate_file()
{
	// 1) pfile 얻어 오기...
	object_ptr<archive::file> pfile = m_pfile;

	// check) File이 없으면 그냥 끝낸다.
	RETURN_IF(pfile.empty());

	// 2) 값을 얻어 놓는다.
	auto recreation_next_time = m_recreation_next_time;
	auto time_now = chrono::time::utc::now();

	// 3) 시간을 감는다.
	while ((recreation_next_time + m_recreation_duration) < time_now)
	{
		recreation_next_time += m_recreation_duration;
	}

	// 4) get time
	auto temp_tm = chrono::to_tm(recreation_next_time);

	// 5) File이름을 재설정한다.
#if defined(__GNUC__) || defined(FMT_FORMAT_H_)
	std::string filename = NFMT::format(m_filename,
		temp_tm.tm_year,
		temp_tm.tm_mon,
		temp_tm.tm_mday,
		temp_tm.tm_hour,
		temp_tm.tm_min,
		temp_tm.tm_sec);
#else
	std::string filename = std::vformat(m_filename, std::make_format_args(
		temp_tm.tm_year,
		temp_tm.tm_mon,
		temp_tm.tm_mday,
		temp_tm.tm_hour,
		temp_tm.tm_min,
		temp_tm.tm_sec));
#endif

	// 6) 먼저 close한다.
	pfile->stop();

	// 7) get fullpath
	auto str_full_path = api::file::make_full_path(this->m_path, filename);

	// log) 
	LOG_INFO << "  - try recreation file '"sv << str_full_path << "'"sv;

	// 8) 다시 시작한다.
	pfile->start({ str_full_path, eFILE_OPERATION::APPEND_ASYNC });

	// 9) 재생성 시간을 변경한다.
	this->m_recreation_next_time = recreation_next_time + this->m_recreation_duration;
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
			_buf_target.append('\n');
			_buf_target.append<char>(0);
			break;

	default:
			CGASSERT_ERROR(false);
			_buf_target.append<char>(0);
			break;
	}
}