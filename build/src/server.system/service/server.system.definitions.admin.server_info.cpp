#include "pch.h"

using namespace CGDK;


void server_admin::sRANK::clear() noexcept
{
	this->group = 0;
	this->field = 0;
	this->role = 0;
	this->index = 0;
}

void server_admin::sSERVICE_SETTING::clear() noexcept
{
	this->nickname.clear();
	this->comment.clear();
	this->rank.clear();
	this->version.clear();
	this->parameter = { 0,0,0,0,0,0,0,0 };
	this->buf_configure.clear();
	this->buf_setting.clear();
	this->revision.url.clear();
	this->revision.revision = 0;
	this->revision.time = chrono::time::utc::time_point();

	this->file_execute.clear();
	this->str_path.clear();
	this->str_path_working.clear();
	this->buf_command_line.clear();
	this->buf_environment.clear();
}

bool server_admin::sSERVICE_SETTING::load(std::string_view _path)
{
	// check) 
	RETURN_IF(_path.empty(), false);

	wchar_t	tempPath[MAX_STRING_LENGTH]{};
	encode_string(tempPath, _path);

	// return) 
	return this->load(tempPath);
}

bool server_admin::sSERVICE_SETTING::load(std::wstring_view _path)
{
	// check) 
	RETURN_IF(_path.empty(), false);

#if defined(_WIN32)
	//// declare) 
	//_CHAR	strINIFileName[512]{};
	//_CHAR	strTemp[512]{};

	//// 1) Ini FileName을 설정한다.
	//__sprintf(strINIFileName, 512, _CGTEXT("%s\\%s"), _path, _CGTEXT(CGNET_INI_FILE_NAMETEXT));
	//
	//// 2) "SYSTEM"값을 설정한다.
	//GET_PRIVATE_PROFILE_STRING(_CGTEXT("SYSTEM"), _CGTEXT("sID"), _CGTEXT("-1"), strTemp, 256, strINIFileName);
	//id.key = __from_string<decltype(id.key)>(strTemp);
	//GET_PRIVATE_PROFILE_STRING(_CGTEXT("SYSTEM"), _CGTEXT("SYSTEM_CODE"), _CGTEXT("-1"), strTemp, 256, strINIFileName);
	//id.code = __from_string<decltype(id.key)>(strTemp);
	//GET_PRIVATE_PROFILE_STRING(_CGTEXT("SYSTEM"), _CGTEXT("SYSTEM_TYPE"), _CGTEXT("0"), strTemp, 256, strINIFileName);
	//id.type = (eSERVER_SYSTEM_TYPE)__from_string<int>(strTemp);

	//GET_PRIVATE_PROFILE_STRING(_CGTEXT("SYSTEM"), _CGTEXT("NAME"), _CGTEXT(""), strTemp, 256, strINIFileName);
	//__strcpy(strName, strTemp);
	//GET_PRIVATE_PROFILE_STRING(_CGTEXT("SYSTEM"), _CGTEXT("EXPLANATION"), _CGTEXT(""), strTemp, 256, strINIFileName);
	//__strcpy(strComment, strTemp);

	//// 3) "SERVER"값을 설정한다.
	//GET_PRIVATE_PROFILE_STRING(_CGTEXT("SERVER"), _CGTEXT("EXECUTE_FILE"), _CGTEXT(""), strTemp, 256, strINIFileName);
	//__strcpy(file_execute, strTemp);
	//GET_PRIVATE_PROFILE_STRING(_CGTEXT("SERVER"), _CGTEXT("PATH"), _CGTEXT(""), strTemp, 256, strINIFileName);
	//__strcpy(str_path, strTemp);
	//GET_PRIVATE_PROFILE_STRING(_CGTEXT("SERVER"), _CGTEXT("WORKING_PATH"), _CGTEXT(""), strTemp, 256, strINIFileName);
	//__strcpy(str_path_working, strTemp);

#elif defined(__linux__)

#else
#error	"[CGDK10] Not supported Android Platform"
#endif

	// return) 
	return true;
}

bool server_admin::sSERVICE_SETTING::save()
{
#if defined(_WIN32)
	//// declare) 
	//_CHAR	strINIFileName[256]	 = { _CHAR(NULL), };
	//_CHAR	strValue[256]		 = { _CHAR(NULL), };

	//// 1) Ini FileName을 설정한다.
	//__sprintf(strINIFileName, _CGTEXT("%s\\") _CGTEXT(CGNET_INI_FILE_NAMETEXT), str_path_working);

	//// 2) 기본 값을 설정한다.
	//__sprintf(strValue, _CGTEXT("%lld"), id.key); WRITE_PRIVATE_PROFILES_STRING(_CGTEXT("SYSTEM"), _CGTEXT("sID"), strValue, strINIFileName);
	//__sprintf(strValue, _CGTEXT("%lld"), id.code); WRITE_PRIVATE_PROFILES_STRING(_CGTEXT("SYSTEM"), _CGTEXT("SYSTEM_CODE"), strValue, strINIFileName);
	//__sprintf(strValue, _CGTEXT("%lld"),	(int64_t)id.eType);	WRITE_PRIVATE_PROFILES_STRING(_CGTEXT("SYSTEM"), _CGTEXT("SYSTEM_TYPE"), strValue, strINIFileName);
	//WRITE_PRIVATE_PROFILES_STRING(_CGTEXT("SYSTEM"), _CGTEXT("NAME"), strName, strINIFileName);

	//// 3) 
	//WRITE_PRIVATE_PROFILES_STRING(_CGTEXT("SERVER"), _CGTEXT("EXECUTE_FILE"), file_execute, strINIFileName);
	//WRITE_PRIVATE_PROFILES_STRING(_CGTEXT("SERVER"), _CGTEXT("WORKING"), str_path_working, strINIFileName);
	//WRITE_PRIVATE_PROFILES_STRING(_CGTEXT("SERVER"), _CGTEXT("WORKING_PATH"), str_path_working, strINIFileName);

#elif defined(__linux__)
	// 임시) 
	CGASSERT_ERROR(false);

#else
#error	"[CGDK10] Not supported Android Platform"
#endif

	// return) 
	return true;
}


void server_admin::sSERVICE_INSTANCE_INFO_BASIC::clear() noexcept
{
	this->host_iid = 0;

	this->revision.url.clear();
	this->revision.revision = -1;

	this->time_register = chrono::time::utc::time_point();
	this->time_update = chrono::time::utc::time_point();
}

void server_admin::sSERVICE_INSTANCE_INFO::clear() noexcept
{
	// 1) clear 
	this->sSERVICE_INSTANCE_INFO_BASIC::clear();

	// 2) clear 
	this->file_execute.clear();
	this->str_path.clear();
	this->str_path_working.clear();
	this->buf_command_line.clear();
	this->buf_environment.clear();
}

std::vector<object_ptr<LOG_RECORD>> server_admin::get_records_of_default_logger()
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

circular_list<object_ptr<LOG_RECORD>> server_admin::get_records_of_default_logger_circular_list()
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
