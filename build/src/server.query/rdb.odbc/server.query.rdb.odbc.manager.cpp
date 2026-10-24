#include "pch.h"

namespace CGDK::query::rdb::odbc
{

// declare) 
lock_object g_lockable_vector_odbc_driver_info;
std::vector<ODBC_DRIVER_INFO> g_vector_odbc_driver_info;

void init_odbc_driver()
{
	// declare) 
	SQLHENV	handle_env;
	std::vector<ODBC_DRIVER_INFO> vector_driver_info;

	// 1) allocate an environment handle
	auto result = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &handle_env);

	// check) return if failed
	RETURN_IF(SQL_SUCCEEDED(result) == false);

	// 2) first try to odeb 3.8
	result = SQLSetEnvAttr(handle_env, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3_80, 0);

	// 3) we want odbc 3 support
	if (SQL_SUCCEEDED(result) == false)
	{
		// - odbc 3
		SQLSetEnvAttr(handle_env, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

		// check) return if failed
		RETURN_IF(SQL_SUCCEEDED(result) == false);
	}

	// 4) get list of odbc driver
	{
		// declare) 
		SQLCHAR		driver[256];
		SQLCHAR		attr[256];
		SQLSMALLINT	driver_ret;
		SQLSMALLINT	attr_ret;

		// 3) Call SQLDrivers to return ODBC Drivers
		for(SQLUSMALLINT direction = SQL_FETCH_FIRST; SQL_SUCCEEDED(result = SQLDrivers(handle_env, direction, driver, sizeof(driver), &driver_ret, attr, sizeof(attr), &attr_ret)); direction = SQL_FETCH_NEXT)
		{
			// declare) 
			ODBC_DRIVER_INFO	driver_info;

			// - set value
			driver_info.driver    = "{";
			driver_info.driver   += std::string_view(reinterpret_cast<char*>(driver), driver_ret);
			driver_info.driver 	 += "}";
			driver_info.attribute = std::string_view(reinterpret_cast<char*>(attr), attr_ret);

			// - push_back
			vector_driver_info.push_back(driver_info);
		}
	}

	// 4) set result
	g_vector_odbc_driver_info = std::move(vector_driver_info);

	// 5) free evn handle
	SQLFreeHandle(SQL_HANDLE_ENV, handle_env);
}

std::string_view find_odbc_driver(std::string_view _first)
{
	for (auto& iter : g_vector_odbc_driver_info)
	{
		// - to lower
		std::string str_iter_lower = __to_lower(iter.driver);

		// - find _first
		auto pos = str_iter_lower.find(_first, 0);

		if (pos < str_iter_lower.size())
		{
			// return) 
			return iter.driver;
		}
	}

	// return)
	return std::string_view();
}

std::string_view find_odbc_driver(std::string_view _first, std::string_view _second)
{
	for (auto& iter : g_vector_odbc_driver_info)
	{
		// - to lower
		std::string str_iter_lower = __to_lower(iter.driver);

		// - find _first
		auto pos = str_iter_lower.find(_first, 0);

		if (pos < str_iter_lower.size())
		{
			// - find _second
			pos = str_iter_lower.find(_second, pos);

			// check) 
			CONTINUE_IF(pos >= str_iter_lower.size())

			// return) 
			return iter.driver;
		}
	}

	// return)
	return std::string_view();
}

std::string_view find_odbc_driver(std::string_view _first, std::string_view _second, std::string_view _third)
{
	for (auto& iter : g_vector_odbc_driver_info)
	{
		// - to lower
		std::string str_iter_lower = __to_lower(iter.driver);

		// - find _first
		auto pos = str_iter_lower.find(_first, 0);

		if (pos < str_iter_lower.size())
		{
			// - find _second
			pos = str_iter_lower.find(_second, pos);

			// check) 
			CONTINUE_IF(pos >= str_iter_lower.size())

			// - find _third
			pos = str_iter_lower.find(_third, pos);

			// check) 
			CONTINUE_IF(pos >= str_iter_lower.size())

			// return) 
			return iter.driver;
		}
	}

	// return)
	return std::string_view();
}

std::string_view find_odbc_driver(std::string_view _first, std::string_view _second, std::string_view _third, std::string_view _forth)
{
	for (auto& iter : g_vector_odbc_driver_info)
	{
		// - to lower
		std::string str_iter_lower = __to_lower(iter.driver);

		// - find _first
		auto pos = str_iter_lower.find(_first, 0);

		if (pos < str_iter_lower.size())
		{
			// - find _second
			pos = str_iter_lower.find(_second, pos);

			// check) 
			CONTINUE_IF(pos >= str_iter_lower.size())

			// - find _third
			pos = str_iter_lower.find(_third, pos);

			// check) 
			CONTINUE_IF(pos >= str_iter_lower.size())

			// - find _forth
			pos = str_iter_lower.find(_forth, pos);

			// check) 
			CONTINUE_IF(pos >= str_iter_lower.size())

			// return) 
			return iter.driver;
		}
	}

	// return)
	return std::string_view();
}

std::string_view get_suitable_odbc_provider(eDB_PROVIDER _provider)
{
	// check) 
	SCOPED_LOCK(g_lockable_vector_odbc_driver_info)
	{
		if (g_vector_odbc_driver_info.empty())
		{
			init_odbc_driver();
		}
	}

	// declare) 
	std::string_view temp;

	switch (_provider & eDB_PROVIDER::MASK_)
	{
	case	eDB_PROVIDER::DSN:					break;
	case	eDB_PROVIDER::SQL_SERVER:			temp = find_odbc_driver("sql server"); break;
	case	eDB_PROVIDER::SQL_SERVER_18:		temp = find_odbc_driver("sql server"); break;
	case	eDB_PROVIDER::SQL_SERVER_NATIVE:	temp = find_odbc_driver("sql server native client"); break;
	case	eDB_PROVIDER::SQL_SERVER_ODBC:		temp = find_odbc_driver("odbc", "sql server"); break;
	case	eDB_PROVIDER::MYSQL:				temp = find_odbc_driver("mysql", "ansi"); break;
	case	eDB_PROVIDER::MYSQL_ANSI:			temp = find_odbc_driver("mysql", "ansi"); break;
	case	eDB_PROVIDER::MYSQL_UNICODE:		temp = find_odbc_driver("mysql", "unicode"); break;
	case	eDB_PROVIDER::MYSQL_3_5:			temp = find_odbc_driver("mysql", "3.5", "ansi"); break; 
	case	eDB_PROVIDER::MYSQL_5_1:			temp = find_odbc_driver("mysql", "5.1", "ansi"); break;
	case	eDB_PROVIDER::MYSQL_5_1_ANSI:		temp = find_odbc_driver("mysql", "5.1", "ansi"); break;
	case	eDB_PROVIDER::MYSQL_5_1_UNICODE:	temp = find_odbc_driver("mysql", "5.1", "unicode"); break;
	case	eDB_PROVIDER::MYSQL_5_2:			temp = find_odbc_driver("mysql", "5.2", "ansi"); break;
	case	eDB_PROVIDER::MYSQL_5_2_ANSI:		temp = find_odbc_driver("mysql", "5.2", "ansi"); break;
	case	eDB_PROVIDER::MYSQL_5_2_UNICODE:	temp = find_odbc_driver("mysql", "5.2", "unicode"); break;
	case	eDB_PROVIDER::MYSQL_5_3:			temp = find_odbc_driver("mysql", "5.3", "ansi"); break;
	case	eDB_PROVIDER::MYSQL_5_3_ANSI:		temp = find_odbc_driver("mysql", "5.3", "ansi"); break;
	case	eDB_PROVIDER::MYSQL_5_3_UNICODE:	temp = find_odbc_driver("mysql", "5.3", "unicode"); break;
	case	eDB_PROVIDER::MYSQL_8:				temp = find_odbc_driver("mysql", "8.0", "ansi"); break;
	case	eDB_PROVIDER::MYSQL_8_ANSI:			temp = find_odbc_driver("mysql", "8.0", "ansi"); break;
	case	eDB_PROVIDER::MYSQL_8_UNICODE:		temp = find_odbc_driver("mysql", "8.0", "unicode"); break;
	case	eDB_PROVIDER::MYSQL_9:				temp = find_odbc_driver("mysql", "9.0", "ansi"); break;
	case	eDB_PROVIDER::MYSQL_9_ANSI:			temp = find_odbc_driver("mysql", "9.0", "ansi"); break;
	case	eDB_PROVIDER::MYSQL_9_UNICODE:		temp = find_odbc_driver("mysql", "9.0", "unicode"); break;
	case	eDB_PROVIDER::MYSQL_9_7:			temp = find_odbc_driver("mysql", "9.7", "ansi"); break;
	case	eDB_PROVIDER::MYSQL_9_7_ANSI:		temp = find_odbc_driver("mysql", "9.7", "ansi"); break;
	case	eDB_PROVIDER::MYSQL_9_7_UNICODE:	temp = find_odbc_driver("mysql", "9.7", "unicode"); break;
	case	eDB_PROVIDER::MARIADB:				temp = find_odbc_driver("mariadb", "odbc"); break;
	case	eDB_PROVIDER::MARIADB_3_1:			temp = find_odbc_driver("mariadb", "odbc", "3.1"); break;
	case	eDB_PROVIDER::EXCEL:				temp = find_odbc_driver("microsoft excel driver"); break;
	case	eDB_PROVIDER::ACCESS:				temp = find_odbc_driver("microsoft access driver"); break;
	case	eDB_PROVIDER::ACCESS_DBASE:			temp = find_odbc_driver("microsoft access dBASE driver"); break;
	case	eDB_PROVIDER::ACCESS_TEXT:			temp = find_odbc_driver("microsoft access text driver"); break;

	default: break;
	}

	// return)
	return temp;
}

std::tuple<std::string_view, std::string_view, bool, int> get_suitable(std::string_view _provider)
{
	// declare) 
	std::string_view str_provider_suitable;
	std::string_view str_initial_option;
	bool is_access = false;
	int select_character_set = 0;

	// is _provider "dsn"
	if (CGDK::__stricmp(_provider, "dsn") == 0)
	{
		str_provider_suitable = "{SQLOLEDB}";
		str_initial_option = "";
	}
	// is _provider "sqlserver"
	else if (CGDK::__stricmp(_provider, "sqlserver") == 0 || CGDK::__stricmp(_provider, "sql_server") == 0)
	{
		str_provider_suitable = get_suitable_odbc_provider(eDB_PROVIDER::SQL_SERVER);	// "{SQL Server}";
		str_initial_option = "";
	}
	// is _provider "sqlserver_native"
	else if (CGDK::__stricmp(_provider, "sqlserver_native") == 0 || CGDK::__stricmp(_provider, "sql_server_native") == 0)
	{
		str_provider_suitable = get_suitable_odbc_provider(eDB_PROVIDER::SQL_SERVER_NATIVE);	// "{SQL Server}";
		str_initial_option = "";
	}
	// is _provider "sql_server_18"
	else if (CGDK::__stricmp(_provider, "sqlserver_18") == 0 || CGDK::__stricmp(_provider, "sql_server_18") == 0)
	{
		str_provider_suitable = "{SQL Server}"sv;
		str_initial_option = "";
	}
	// is _provider "sql_server_odbc"
	else if (CGDK::__stricmp(_provider, "sqlserver_odbc") == 0 || CGDK::__stricmp(_provider, "sql_server_odbc") == 0)
	{
		str_provider_suitable = get_suitable_odbc_provider(eDB_PROVIDER::SQL_SERVER_ODBC);	// "{ODBC Driver for SQL Server}";
		str_initial_option = "";
	}
	// is _provider "mysql"
	else if (CGDK::__stricmp(_provider, "mysql") == 0 || CGDK::__stricmp(_provider, "my_sql") == 0)
	{
		str_provider_suitable = get_suitable_odbc_provider(eDB_PROVIDER::MYSQL);	// "{MySQL ODBC Driver}";	
		str_initial_option = ";option=67108872;NO_SSPS=0"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
		select_character_set = 1;
	}
	// is _provider "mysql_ansi"
	else if (CGDK::__stricmp(_provider, "mysql_ansi") == 0 || CGDK::__stricmp(_provider, "my_sql_ansi") == 0)
	{
		str_provider_suitable = get_suitable_odbc_provider(eDB_PROVIDER::MYSQL_ANSI);	// "{MySQL ODBC ANSI Driver}";	
		str_initial_option = ";option=67108872;NO_SSPS=0"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
		select_character_set = 1;
	}
	// is _provider "mysql_unicode"
	else if (CGDK::__stricmp(_provider, "mysql_unicode") == 0 || CGDK::__stricmp(_provider, "my_sql_unicode") == 0)
	{
		str_provider_suitable = get_suitable_odbc_provider(eDB_PROVIDER::MYSQL_UNICODE);	// "{MySQL ODBC Driver}";	
		str_initial_option = ";option=67108872;NO_SSPS=0"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
		select_character_set = 2;
	}
	// is _provider "mysql_3_5"
	else if (CGDK::__stricmp(_provider, "mysql_3_5") == 0 || CGDK::__stricmp(_provider, "my_sql_3_5") == 0)
	{
		str_provider_suitable = "{MySQL ODBC 3.5 Driver}"sv;
		str_initial_option = ";option=67108872;NO_SSPS=0"; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
		select_character_set = 1;
	}
	// is _provider "mysql_5_1"
	else if (CGDK::__stricmp(_provider, "mysql_5_1") == 0 || CGDK::__stricmp(_provider, "my_sql_5_1") == 0)
	{
		str_provider_suitable = "{MySQL ODBC 5.1 Driver}"sv;
		str_initial_option = ";option=67108872;NO_SSPS=0"; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
		select_character_set = 1;
	}
	// is _provider "mysql_5_1_ansi"
	else if (CGDK::__stricmp(_provider, "mysql_5_1_ansi") == 0 || CGDK::__stricmp(_provider, "my_sql_5_1_ansi") == 0)
	{
		str_provider_suitable = "{MySQL ODBC 5.1 ANSI Driver}"sv;
		str_initial_option = ";option=67108872;NO_SSPS=0"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
		select_character_set = 1;
	}
	// is _provider "mysql_5_1_unicode"
	else if (CGDK::__stricmp(_provider, "mysql_5_1_unicode") == 0 || CGDK::__stricmp(_provider, "my_sql_5_1_unicode") == 0)
	{
		str_provider_suitable = "{MySQL ODBC 5.1 UNICODE Driver}"sv;
		str_initial_option = ";option=67108872;NO_SSPS=0"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
		select_character_set = 2;
	}
	// is _provider "mysql_5_2"
	else if (CGDK::__stricmp(_provider, "mysql_5_2") == 0 || CGDK::__stricmp(_provider, "my_sql_5_2") == 0)
	{
		str_provider_suitable = "{MySQL ODBC 5.2 Driver}"sv;
		str_initial_option = ";option=67108872;NO_SSPS=0"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
		select_character_set = 1;
	}
	// is _provider "mysql_5_2_ansi"
	else if (CGDK::__stricmp(_provider, "mysql_5_2_ansi") == 0 || CGDK::__stricmp(_provider, "my_sql_5_2_ansi") == 0)
	{
		str_provider_suitable = "{MySQL ODBC 5.2 ANSI Driver}"sv;
		str_initial_option = ";option=67108872;NO_SSPS=0"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
		select_character_set = 1;
	}
	// is _provider "mysql_5_2_unicode"
	else if (CGDK::__stricmp(_provider, "mysql_5_2_unicode") == 0 || CGDK::__stricmp(_provider, "my_sql_5_2_unicode") == 0)
	{
		str_provider_suitable = "{MySQL ODBC 5.2 UNICODE Driver}"sv;
		str_initial_option = ";option=67108872;NO_SSPS=0"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
		select_character_set = 2;
	}
	// is _provider "mysql_5_3"
	else if (CGDK::__stricmp(_provider, "mysql_5_3") == 0 || CGDK::__stricmp(_provider, "my_sql_5_3") == 0)
	{
		str_provider_suitable = "{MySQL ODBC 5.3 Driver}"sv;
		str_initial_option = ";option=67108872;NO_SSPS=0"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
		select_character_set = 1;
	}
	// is _provider "mysql_5_3_ansi"
	else if (CGDK::__stricmp(_provider, "mysql_5_3_ansi") == 0 || CGDK::__stricmp(_provider, "my_sql_5_3_ansi") == 0)
	{
		str_provider_suitable = "{MySQL ODBC 5.3 ANSI Driver}"sv;
		str_initial_option = ";option=67108872;NO_SSPS=0"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
		select_character_set = 1;
	}
	// is _provider "mysql_5_3_unicode"
	else if (CGDK::__stricmp(_provider, "mysql_5_3_unicode") == 0 || CGDK::__stricmp(_provider, "my_sql_5_3_unicode") == 0)
	{
		str_provider_suitable = "{MySQL ODBC 5.3 UNICODE Driver}"sv;
		str_initial_option = ";option=67108872;NO_SSPS=0"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
		select_character_set = 2;
	}
	// is _provider "mysql_8"
	else if (CGDK::__stricmp(_provider, "mysql_8") == 0 || CGDK::__stricmp(_provider, "my_sql_8") == 0)
	{
		str_provider_suitable = "{MySQL ODBC 8.0 Driver}"sv;
		str_initial_option = ";option=67108872;NO_SSPS=0"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
		select_character_set = 1;
	}
	// is _provider "mysql_8_ansi"
	else if (CGDK::__stricmp(_provider, "mysql_8_ansi") == 0 || CGDK::__stricmp(_provider, "my_sql_8_ansi") == 0)
	{
		str_provider_suitable = "{MySQL ODBC 8.0 ANSI Driver}"sv;
		str_initial_option = ";option=67108872;NO_SSPS=0"; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
		select_character_set = 1;
	}
	// is _provider "mysql_8_unicode"
	else if (CGDK::__stricmp(_provider, "mysql_8_unicode") == 0 || CGDK::__stricmp(_provider, "my_sql_8_unicode") == 0)
	{
		str_provider_suitable = "{MySQL ODBC 8.0 UNICODE Driver}"sv;
		str_initial_option = ";option=67108872;NO_SSPS=0"; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
		select_character_set = 2;
	}
	// is _provider "mysql_9"
	else if (CGDK::__stricmp(_provider, "mysql_9") == 0 || CGDK::__stricmp(_provider, "my_sql_9") == 0)
	{
		str_provider_suitable = "{MySQL ODBC 9.0 Driver}"sv;
		str_initial_option = ";option=67108872;NO_SSPS=0"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 9
		select_character_set = 1;
	}
	// is _provider "mysql_9_ansi"
	else if (CGDK::__stricmp(_provider, "mysql_9_ansi") == 0 || CGDK::__stricmp(_provider, "my_sql_9_ansi") == 0)
	{
		str_provider_suitable = "{MySQL ODBC 9.0 ANSI Driver}"sv;
		str_initial_option = ";option=67108872;NO_SSPS=0"; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 9
		select_character_set = 1;
	}
	// is _provider "mysql_9_unicode"
	else if (CGDK::__stricmp(_provider, "mysql_9_unicode") == 0 || CGDK::__stricmp(_provider, "my_sql_9_unicode") == 0)
	{
		str_provider_suitable = "{MySQL ODBC 9.0 UNICODE Driver}"sv;
		str_initial_option = ";option=67108872;NO_SSPS=0"; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 9
		select_character_set = 2;
	}
	// is _provider "mysql_9.7"
	else if (CGDK::__stricmp(_provider, "mysql_9_7") == 0 || CGDK::__stricmp(_provider, "my_sql_9_7") == 0)
	{
		str_provider_suitable = "{MySQL ODBC 9.7 Driver}"sv;
		str_initial_option = ";option=67108872;NO_SSPS=0"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 9.7
		select_character_set = 1;
	}
	// is _provider "mysql_9_7_ansi"
	else if (CGDK::__stricmp(_provider, "mysql_9_7_ansi") == 0 || CGDK::__stricmp(_provider, "my_sql_9_7_ansi") == 0)
	{
		str_provider_suitable = "{MySQL ODBC 9.7 ANSI Driver}"sv;
		str_initial_option = ";option=67108872;NO_SSPS=0"; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 9.7
		select_character_set = 1;
	}
	// is _provider "mysql_9_7_unicode"
	else if (CGDK::__stricmp(_provider, "mysql_9_7_unicode") == 0 || CGDK::__stricmp(_provider, "my_sql_9_7_unicode") == 0)
	{
		str_provider_suitable = "{MySQL ODBC 9.7 UNICODE Driver}"sv;
		str_initial_option = ";option=67108872;NO_SSPS=0"; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 9.7
		select_character_set = 2;
	}
	// is _provider "mariadb"
	else if (CGDK::__stricmp(_provider, "mariadb") == 0 || CGDK::__stricmp(_provider, "maria_db") == 0)
	{
		str_provider_suitable = get_suitable_odbc_provider(eDB_PROVIDER::MARIADB);
		str_initial_option = ""sv;
	}
	// is _provider "mariadb_3_1"
	else if (CGDK::__stricmp(_provider, "mariadb_3_1") == 0 || CGDK::__stricmp(_provider, "maria_db_3_1") == 0)
	{
		str_provider_suitable = get_suitable_odbc_provider(eDB_PROVIDER::MARIADB_3_1);
		str_initial_option = ""sv;
	}
	// is _provider "SQLOLEDB.1"
	else if (CGDK::__stricmp(_provider, "SQLOLEDB.1") == 0 || CGDK::__stricmp(_provider, "SQLOLEDB") == 0)
	{
		str_provider_suitable = "{SQLOLEDB}"sv;
		str_initial_option = "";
	}
	// is _provider "MSDASQL.1"
	else if (CGDK::__stricmp(_provider, "MSDASQL.1") == 0 || CGDK::__stricmp(_provider, "MSDASQL") == 0)
	{
	}
	// is _provider "excel"
	else if (CGDK::__stricmp(_provider, "excel") == 0)
	{
		str_provider_suitable = get_suitable_odbc_provider(eDB_PROVIDER::EXCEL);
		str_initial_option = ""sv;
		//str_initial_option	 = "FIRSTROWHASNAMES=1;READONLY=FALSE"sv;
		is_access = true;
	}
	// is _provider "access"
	else if (CGDK::__stricmp(_provider, "access") == 0)
	{
		str_provider_suitable = get_suitable_odbc_provider(eDB_PROVIDER::ACCESS);
		str_initial_option = ""sv;
		//str_initial_option	 = "FIRSTROWHASNAMES=1;READONLY=FALSE"sv;
		is_access = true;
	}
	// is _provider "access_dbase"
	else if (CGDK::__stricmp(_provider, "access_dbase") == 0)
	{
		str_provider_suitable = get_suitable_odbc_provider(eDB_PROVIDER::ACCESS_DBASE);
		str_initial_option = ""sv;
		//str_initial_option	 = "FIRSTROWHASNAMES=1;READONLY=FALSE"sv;
		is_access = true;
	}
	// is _provider "access_text"
	else if (CGDK::__stricmp(_provider, "access_text") == 0)
	{
		str_provider_suitable = get_suitable_odbc_provider(eDB_PROVIDER::ACCESS_TEXT);
		str_initial_option = ""sv;
		//str_initial_option	 = "FIRSTROWHASNAMES=1;READONLY=FALSE"sv;
		is_access = true;
	}
	else
	{
	}

	return { str_provider_suitable, str_initial_option, is_access, select_character_set };
}

void get_connection_string(querier* _pquerier, shared_buffer& _str_initial, shared_buffer& _buf_output, const querier::START_PARAMETER& _parameter, std::string_view _str_provider, bool _is_access)
{
	// declare) 
	std::string_view str_provider_suitable;
	std::string_view str_initial_option;
	int select_character_set = 0;

	// - try to get key "provider"
	if (_str_provider.empty() == false)
	{
		// - to lower case
		auto _provider = __to_lower(std::string_view(_str_provider));

		// - get initialization string
		std::tie(str_provider_suitable, str_initial_option, _is_access, select_character_set) = get_suitable(_provider);

		// check) str_value is nullptr?
		if (str_provider_suitable.empty())
		{
			// trace) 
			LOG_FATAL << "@ failed QUERIER_RDB_ODBC[" << _pquerier->name() << "]: cann't find odbc driver for provider '"sv << _str_provider << "'"sv;

			SCOPED_LOCK(g_lockable_vector_odbc_driver_info)
			{
				if (g_vector_odbc_driver_info.empty() == false)
				{
					LOG_FATAL << log_continue_t() << "  * installed odbc driver list"sv;
					for (auto& iter : g_vector_odbc_driver_info)
					{
						LOG_FATAL << log_continue_t() << "    - "sv << iter.driver;
					}
				}
				else
				{
					LOG_FATAL << log_continue_t() << "  no odbc driver installed"sv;
				}
			}

			// throw)
			throw throwable::failure_ex(eRESULT::INVALID_ARGUMENT);
		}

		// - append driver string
		_str_initial.append_text<char>("Driver="sv);
		_str_initial.append_text<char>(str_provider_suitable);
		_str_initial.append_text<char>(";");

		_buf_output.append_text<char>("Driver="sv);
		_buf_output.append_text<char>(str_provider_suitable);
		_buf_output.append_text<char>(";"sv);
	}
	else
	{
		_str_initial.append_text<char>("Provider=MSDASQL.1;"sv);
		_buf_output.append_text<char>("Provider=MSDASQL.1;"sv);
	}

	// - try to get key "Server" or "data_source"
	if (_parameter.data_source.empty() == false)
	{
		if (_is_access == false)
		{
			_str_initial.append_text<char>("Server="sv);
			_str_initial.append_text<char>(_parameter.data_source);
			_str_initial.append_text<char>(";"sv);

			_buf_output.append_text<char>("Server="sv);
			_buf_output.append_text<char>(_parameter.data_source);
			_buf_output.append_text<char>(";"sv);
		}
		else
		{
			_str_initial.append_text<char>("Extended Properties=\"Excel 12.0\";READONLY=0;"sv);
			_buf_output.append_text<char>("Extended Properties=\"Excel 12.0\";READONLY=0;"sv);

			_str_initial.append_text<char>("DBQ="sv);
			_str_initial.append_text<char>(_parameter.data_source);
			_str_initial.append_text<char>(";"sv);

			_buf_output.append_text<char>("DBQ="sv);
			_buf_output.append_text<char>(_parameter.data_source);
			_buf_output.append_text<char>(";"sv);
		}
	}

	// - try to get key "Uid"
	if (_parameter.user_id.empty() == false)
	{
		_str_initial.append_text<char>("Uid="sv);
		_str_initial.append_text<char>(_parameter.user_id);
		_str_initial.append_text<char>(";"sv);

		_buf_output.append_text<char>("Uid="sv);
		_buf_output.append_text<char>(_parameter.user_id);
		_buf_output.append_text<char>(";"sv);
	}

	// - try to get key "Pwd"
	if (_parameter.password.empty() == false)
	{
		_str_initial.append_text<char>("Pwd="sv);
		_str_initial.append_text<char>(_parameter.password);
		_str_initial.append_text<char>(";"sv);

		_buf_output.append_text<char>("Pwd="sv);
		_buf_output.append_text<char>("*"sv);
		_buf_output.append_text<char>(";"sv);
	}

	// - try to get key "initial_catalog"
	if (_parameter.initial_catalog.empty() == false)
	{
		_str_initial.append_text<char>("Database="sv);
		_str_initial.append_text<char>(_parameter.initial_catalog);
		_str_initial.append_text<char>(";"sv);

		_buf_output.append_text<char>("Database="sv);
		_buf_output.append_text<char>(_parameter.initial_catalog);
		_buf_output.append_text<char>(";"sv);
	}

	// - append char_set
	if (_parameter.character_set.empty() == false)
	{
	}

	if (_parameter.character_set.empty() == false)
	{
		_str_initial.append_text<char>("charset="sv);
		_str_initial.append_text<char>(_parameter.character_set);
		_str_initial.append_text<char>(";");

		_buf_output.append_text<char>("charset="sv);
		_buf_output.append_text<char>(_parameter.character_set);
		_buf_output.append_text<char>(";"sv);

		// trace)
		LOG_INFO << "  + parameter['character_set']: "sv << _parameter.character_set;
	}
	else
	{
		switch (select_character_set)
		{
		// - default
		case	0:
				break;

		// - set 'ANSI'
		case	1:
				break;

		// - set 'UNICODE'
		case	2:
				{
					LOG_WARNING << "  (!) unicode ODBC driver is selected but character_set is not assigned unicode set('utf8','euckr', etc)"sv;
					LOG_WARNING << "      this can cause error on getting string. set 'character_set' parameter as unicode set "sv;
				}
				break;

		// - auto selected 'UNICODE' 
		case	3:
				{
					// - set as unicode
					_str_initial.append_text<char>("charset=utf8;"sv);
					_buf_output.append_text<char>("charset=utf8;"sv);

					// trace)
					LOG_INFO << "  - character_set => 'utf8' (UNICODE driver)"sv << _parameter.character_set;
				}
				break;
		}
	}

	// - append initial options
	if (str_initial_option.empty() == false)
	{
		_str_initial.append_text<char>(str_initial_option);
		_buf_output.append_text<char>(str_initial_option);
	}

	// - append options
	if (_parameter.options.empty() == false)
	{
		_str_initial.append_text<char>(_parameter.options);
		_buf_output.append_text<char>(_parameter.options);

		// trace)
		LOG_INFO << "  + parameter['option']: "sv << _parameter.options;
	}

	// - append other connection string
	//if(provider == eDB_PROVIDER::OLEDB)
	//{
	//	_str_initial.append_text("Integrated Security=SSPI;;Persist Security Info=false;Use Procedure for prepare=1;Auto Translate=True;Packet Size=8192;Use Encryption for Data=False;Tag with column collation when possible=False");
	//	_buf_output.append_text ("Integrated Security=SSPI;;Persist Security Info=false;Use Procedure for prepare=1;Auto Translate=True;Packet Size=8192;Use Encryption for Data=False;Tag with column collation when possible=False");
	//}
	//else if (provider == eDB_PROVIDER::ODBC)
	//{
	//	// mysql
	//	_str_initial.append_text("Option=3;");
	//	_buf_output.append_text("Option=3;");
	//}
	//else
	//{
	//	CGASSERT_ERROR(false);
	//	throw	throwable::failure(eRESULT::INVALID_ARGUMENT);
	//}

	// - 마지막에 NULL을 붙여준다.
	_str_initial.append<char>(0);
	_buf_output.append<char>(0);

	// trace)
	LOG_INFO << "  + parameter['connection_string']: '"sv << _buf_output.data<char>() << "'"sv;
}


}