#include "pch.h"
#include <cwchar>
#include <cwctype>
#include <algorithm>

using namespace CGDK;


void server_system::sSERVICE::sSETTING::clear() noexcept
{
	this->name.clear();
	this->comment.clear();
	this->rank.clear();
	this->version.clear();
	this->revision.url.clear();
	this->revision.revision = 0;
	this->revision.time = chrono::time::utc::time_point();

	this->filename.clear();
	this->path.clear();
	this->working_path.clear();
	this->arguments.clear();
	this->environment.clear();

	this->context_start.clear();
}

std::string server_system::sSERVICE::sSETTING::get_arguments_string(std::string_view _execute) const
{
	// declare)
	std::string result;

	// 1) execute
	if (_execute.empty() == false)
		result = quote_argument(_execute);

	// 2) arguments
	for (const auto& iter : this->arguments)
	{
		if (result.empty() == false)
			result += ' ';

		result += quote_argument(iter);
	}

	// return)
	return result;
}

std::string server_system::sSERVICE::sSETTING::quote_argument(std::string_view _argument)
{
	// check) 공백/따옴표가 없으면 그대로 쓴다.
	if (_argument.empty() == false && _argument.find_first_of(" \t\"") == std::string_view::npos)
		return std::string(_argument);

	// declare)
	std::string result = "\"";
	std::size_t count_backslash = 0;

	for (auto ch : _argument)
	{
		if (ch == '\\')
		{
			++count_backslash;
			continue;
		}

		// - '"' 앞의 '\'는 두 배로, '"'는 '\"'로 한다. (CommandLineToArgv 규칙)
		if (ch == '"')
		{
			result.append(count_backslash * 2 + 1, '\\');
			result += '"';
		}
		else
		{
			result.append(count_backslash, '\\');
			result += ch;
		}
		count_backslash = 0;
	}

	// - 닫는 '"' 앞의 '\'는 두 배로 한다.
	result.append(count_backslash * 2, '\\');
	result += '"';

	// return)
	return result;
}

#if defined(_WIN32)
std::wstring server_system::sSERVICE::sSETTING::get_environment_block(const wchar_t* _pbase_block) const
{
	// declare)
	std::wstring block;

	// check) environment가 없으면 빈 문자열 (기본 환경을 그대로 쓴다.)
	RETURN_IF(this->environment.empty(), block);

	// - 'name=value'의 name (windows 환경 변수 이름은 대소문자를 구분하지 않는다. '='로 시작하는 숨은 변수(=C: 등)도 처리한다.)
	const auto get_name = [](std::wstring_view _item) -> std::wstring
	{
		auto pos = _item.find(L'=', 1);
		std::wstring name(_item.substr(0, pos));
		std::transform(name.begin(), name.end(), name.begin(), [](wchar_t _ch) { return static_cast<wchar_t>(::towupper(_ch)); });
		return name;
	};

	// 1) environment (UTF-8 -> UTF-16)
	std::vector<std::wstring> vector_environment;
	vector_environment.reserve(this->environment.size());
	for (const auto& iter : this->environment)
	{
		if (iter.empty() == false)
			vector_environment.push_back(utf8_to_utf16(iter));
	}

	// 2) 기본 환경 중 environment에 없는 이름만 남긴다.
	std::vector<std::wstring> vector_result;
	if (_pbase_block != nullptr)
	{
		for (const wchar_t* pos = _pbase_block; *pos != L'\0'; pos += std::wcslen(pos) + 1)
		{
			std::wstring_view item(pos);
			auto item_name = get_name(item);
			auto is_overwritten = std::any_of(vector_environment.begin(), vector_environment.end(), [&](const std::wstring& _env) { return get_name(_env) == item_name; });
			if (is_overwritten == false)
				vector_result.emplace_back(item);
		}
	}

	// 3) environment를 추가한다.
	for (auto& iter : vector_environment)
		vector_result.push_back(std::move(iter));

	// 4) 이름순으로 정렬한다. (windows 환경 블록은 이름순(대소문자 무시)이어야 한다.)
	std::stable_sort(vector_result.begin(), vector_result.end(), [&](const std::wstring& _lhs, const std::wstring& _rhs) { return get_name(_lhs) < get_name(_rhs); });

	// 5) 블록 ('name=value'마다 NULL, 블록 끝 NULL)
	for (const auto& iter : vector_result)
	{
		block += iter;
		block.push_back(L'\0');
	}
	block.push_back(L'\0');

	// return)
	return block;
}
#endif

//bool server_system::sSERVICE::sSETTING::set_configure_json_string_from_file(std::string_view _filename)
//{
//	// check)
//	RETURN_IF(_filename.empty(), false);
//
//	// 1) load json file
//	this->buf_configure = api::file::read(_filename);
//
//	// return)
//	return true;
//}

//bool server_system::sSERVICE::sSETTING::set_configure_json_string(std::string_view _json_string)
//{
//	// check)
//	RETURN_IF(_json_string.empty(), false);
//
//	// 1) alloc buffer
//	auto buf_json = alloc_shared_buffer(_json_string.size() + 1);
//
//	// 2) append data
//	buf_json.append(_json_string.size(), _json_string.data());
//	buf_json.append<char>(0);
//
//	// 3) set
//	this->buf_configure = std::move(buf_json);
//
//	// return)
//	return true;
//}


//bool server_system::sSERVICE::sSETTING::load(std::string_view _path)
//{
//	// check) 
//	RETURN_IF(_path.empty(), false);
//
//	wchar_t	tempPath[MAX_STRING_LENGTH]{};
//	encode_string(tempPath, _path);
//
//	// return) 
//	return this->load(tempPath);
//}
//
//bool server_system::sSERVICE::sSETTING::load(std::wstring_view _path)
//{
//	// check) 
//	RETURN_IF(_path.empty(), false);
//
//#if defined(_WIN32)
//	//// declare) 
//	//_CHAR	strINIFileName[512]{};
//	//_CHAR	strTemp[512]{};
//
//	//// 1) Ini FileName을 설정한다.
//	//std::snprintf(strINIFileName, 512, std::string_view(_CGTEXT("%s\\%s")).data(), _path, _CGTEXT(CGNET_INI_FILE_NAMETEXT));
//	//
//	//// 2) "SYSTEM"값을 설정한다.
//	//GET_PRIVATE_PROFILE_STRING(_CGTEXT("SYSTEM"), _CGTEXT("sID"), _CGTEXT("-1"), strTemp, 256, strINIFileName);
//	//id.key = __from_string<decltype(id.key)>(strTemp);
//	//GET_PRIVATE_PROFILE_STRING(_CGTEXT("SYSTEM"), _CGTEXT("SYSTEM_CODE"), _CGTEXT("-1"), strTemp, 256, strINIFileName);
//	//id.code = __from_string<decltype(id.key)>(strTemp);
//	//GET_PRIVATE_PROFILE_STRING(_CGTEXT("SYSTEM"), _CGTEXT("SYSTEM_TYPE"), _CGTEXT("0"), strTemp, 256, strINIFileName);
//	//id.type = (eSERVER_SYSTEM_TYPE)__from_string<int>(strTemp);
//
//	//GET_PRIVATE_PROFILE_STRING(_CGTEXT("SYSTEM"), _CGTEXT("NAME"), _CGTEXT(""), strTemp, 256, strINIFileName);
//	//([&](auto* dest__, std::size_t max__, auto&& source__) { auto source_view__ = std::basic_string_view(source__); auto count__ = (std::min)(source_view__.size(), max__ - 1); std::char_traits<std::remove_cv_t<std::remove_pointer_t<std::decay_t<decltype(dest__)>>>>::copy(dest__, source_view__.data(), count__); dest__[count__] = 0; return dest__; })(strName, std::size(strName), strTemp);
//	//GET_PRIVATE_PROFILE_STRING(_CGTEXT("SYSTEM"), _CGTEXT("EXPLANATION"), _CGTEXT(""), strTemp, 256, strINIFileName);
//	//([&](auto* dest__, std::size_t max__, auto&& source__) { auto source_view__ = std::basic_string_view(source__); auto count__ = (std::min)(source_view__.size(), max__ - 1); std::char_traits<std::remove_cv_t<std::remove_pointer_t<std::decay_t<decltype(dest__)>>>>::copy(dest__, source_view__.data(), count__); dest__[count__] = 0; return dest__; })(strComment, std::size(strComment), strTemp);
//
//	//// 3) "SERVER"값을 설정한다.
//	//GET_PRIVATE_PROFILE_STRING(_CGTEXT("SERVER"), _CGTEXT("EXECUTE_FILE"), _CGTEXT(""), strTemp, 256, strINIFileName);
//	//([&](auto* dest__, std::size_t max__, auto&& source__) { auto source_view__ = std::basic_string_view(source__); auto count__ = (std::min)(source_view__.size(), max__ - 1); std::char_traits<std::remove_cv_t<std::remove_pointer_t<std::decay_t<decltype(dest__)>>>>::copy(dest__, source_view__.data(), count__); dest__[count__] = 0; return dest__; })(filename, std::size(filename), strTemp);
//	//GET_PRIVATE_PROFILE_STRING(_CGTEXT("SERVER"), _CGTEXT("PATH"), _CGTEXT(""), strTemp, 256, strINIFileName);
//	//([&](auto* dest__, std::size_t max__, auto&& source__) { auto source_view__ = std::basic_string_view(source__); auto count__ = (std::min)(source_view__.size(), max__ - 1); std::char_traits<std::remove_cv_t<std::remove_pointer_t<std::decay_t<decltype(dest__)>>>>::copy(dest__, source_view__.data(), count__); dest__[count__] = 0; return dest__; })(path, std::size(path), strTemp);
//	//GET_PRIVATE_PROFILE_STRING(_CGTEXT("SERVER"), _CGTEXT("WORKING_PATH"), _CGTEXT(""), strTemp, 256, strINIFileName);
//	//([&](auto* dest__, std::size_t max__, auto&& source__) { auto source_view__ = std::basic_string_view(source__); auto count__ = (std::min)(source_view__.size(), max__ - 1); std::char_traits<std::remove_cv_t<std::remove_pointer_t<std::decay_t<decltype(dest__)>>>>::copy(dest__, source_view__.data(), count__); dest__[count__] = 0; return dest__; })(working_path, std::size(working_path), strTemp);
//
//#elif defined(__linux__)
//
//#else
//#error	"[CGDK10] Not supported Android Platform"
//#endif
//
//	// return) 
//	return true;
//}

//bool server_system::sSERVICE::sSETTING::save()
//{
//#if defined(_WIN32)
//	//// declare) 
//	//_CHAR	strINIFileName[256]	 = { _CHAR(NULL), };
//	//_CHAR	strValue[256]		 = { _CHAR(NULL), };
//
//	//// 1) Ini FileName을 설정한다.
//	//std::snprintf(strINIFileName, std::size(strINIFileName), std::string_view(_CGTEXT("%s\\") _CGTEXT(CGNET_INI_FILE_NAMETEXT)).data(), working_path);
//
//	//// 2) 기본 값을 설정한다.
//	//std::snprintf(strValue, std::size(strValue), std::string_view(_CGTEXT("%lld")).data(), id.key); WRITE_PRIVATE_PROFILES_STRING(_CGTEXT("SYSTEM"), _CGTEXT("sID"), strValue, strINIFileName);
//	//std::snprintf(strValue, std::size(strValue), std::string_view(_CGTEXT("%lld")).data(), id.code); WRITE_PRIVATE_PROFILES_STRING(_CGTEXT("SYSTEM"), _CGTEXT("SYSTEM_CODE"), strValue, strINIFileName);
//	//std::snprintf(strValue, std::size(strValue), std::string_view(_CGTEXT("%lld")).data(), (int64_t)id.eType);	WRITE_PRIVATE_PROFILES_STRING(_CGTEXT("SYSTEM"), _CGTEXT("SYSTEM_TYPE"), strValue, strINIFileName);
//	//WRITE_PRIVATE_PROFILES_STRING(_CGTEXT("SYSTEM"), _CGTEXT("NAME"), strName, strINIFileName);
//
//	//// 3) 
//	//WRITE_PRIVATE_PROFILES_STRING(_CGTEXT("SERVER"), _CGTEXT("EXECUTE_FILE"), filename, strINIFileName);
//	//WRITE_PRIVATE_PROFILES_STRING(_CGTEXT("SERVER"), _CGTEXT("WORKING"), working_path, strINIFileName);
//	//WRITE_PRIVATE_PROFILES_STRING(_CGTEXT("SERVER"), _CGTEXT("WORKING_PATH"), working_path, strINIFileName);
//
//#elif defined(__linux__)
//	// 임시) 
//	CGASSERT_ERROR(false);
//
//#else
//#error	"[CGDK10] Not supported Android Platform"
//#endif
//
//	// return) 
//	return true;
//}

//void server_system::sSERVICE_INSTANCE_INFO::clear() noexcept
//{
//	// 1) clear 
//	this->host_iid = 0;
//
//	this->revision.url.clear();
//	this->revision.revision = -1;
//
//	this->time_register = chrono::time::utc::time_point();
//	this->time_update = chrono::time::utc::time_point();
//
//	// 2) clear 
//	this->filename.clear();
//	this->path.clear();
//	this->working_path.clear();
//	this->arguments.clear();
//	this->environment.clear();
//}

std::vector<object_ptr<LOG_RECORD>> server_system::get_records_of_default_logger()
{
	// declare) 
	std::vector<object_ptr<LOG_RECORD>> vector_log_records;

	// 1) get default logger
	auto plogger = get_default_logger();

	// check)
	RETURN_IF(plogger.empty(), vector_log_records);

	// 2) get log_archive
	auto plog_archive = dynamic_pointer_cast<log_archive>(plogger);

	// check)
	RETURN_IF(plog_archive.empty(), vector_log_records);

	// 3) get log records
	vector_log_records = plog_archive->get_stored_log();

	// return)
	return vector_log_records;
}

circular_list<object_ptr<LOG_RECORD>> server_system::get_records_of_default_logger_circular_list()
{
	// declare) 
	circular_list<object_ptr<LOG_RECORD>> list_log_records;

	// 1) get default logger
	auto plogger = get_default_logger();

	// check)
	RETURN_IF(plogger.empty(), list_log_records);

	// 2) get log_archive
	auto plog_archive = dynamic_pointer_cast<log_archive>(plogger);

	// check)
	RETURN_IF(plog_archive.empty(), list_log_records);

	// 3) get log records
	auto vector_log_records = plog_archive->get_stored_log();

	for (auto& iter : vector_log_records)
	{
		list_log_records.push_back(iter);
	}

	// return)
	return list_log_records;
}
