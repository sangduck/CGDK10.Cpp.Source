#include "pch.h"

namespace CGDK::query::rdb::oledb
{

// definitions)
struct OLEDB_PROVIDER_INFO
{
	std::string name;
	std::string description;
};

struct ODBC_DRIVER_INFO
{
	std::string driver;
	std::string attribute;
};

// declare) 
lockable<std::vector<OLEDB_PROVIDER_INFO>> g_vector_oledb_provider_info;
lockable<std::vector<ODBC_DRIVER_INFO>> g_vector_odbc_driver_info;

void init_oledb_provider()
{
	// declare) 
	std::vector<OLEDB_PROVIDER_INFO> vector_oledb_provider_info;

	// declare) 
	CEnumerator	oProviders;

	// 1) open
	HRESULT hr = oProviders.Open();

	// check) 
	RETURN_IF(SUCCEEDED(hr) == false);

	// 2) 
	USES_CONVERSION;

	// 3) get all proviser
	while (oProviders.MoveNext() == S_OK)
	{
		// declare) 
		OLEDB_PROVIDER_INFO	temp;
		char temp_string[256];

		// - set values
		temp.name = encode_string(temp_string, oProviders.m_szName);
		temp.description = encode_string(temp_string, oProviders.m_szDescription);

		// - push_back
		vector_oledb_provider_info.push_back(temp);
	}

	// 4) close provider
	oProviders.Close();

	// 5) 
	g_vector_oledb_provider_info = std::move(vector_oledb_provider_info);
}

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
	result	 = SQLSetEnvAttr(handle_env, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3_80, 0);

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
			ODBC_DRIVER_INFO driver_info;

			// - set value
			driver_info.driver	  = "{";
			driver_info.driver	 += std::string_view(reinterpret_cast<char*>(driver), driver_ret);
			driver_info.driver	 += "}";
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



std::string_view find_oledb_provider(std::string_view _first)
{
	for (auto& iter : g_vector_oledb_provider_info)
	{
		// - to lower
		std::string str_iter_lower = __to_lower<char>(iter.name);

		// - find _first
		auto pos = str_iter_lower.find(_first, 0);

		if (pos < str_iter_lower.size())
		{
			// return) 
			return iter.name;
		}
	}

	// return)
	return std::string_view();
}

std::string_view find_odbc_driver(std::string_view _first)
{
	for (auto& iter : g_vector_odbc_driver_info)
	{
		// - to lower
		std::string str_iter_lower = __to_lower<char>(iter.driver);

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
		std::string str_iter_lower = __to_lower<char>(iter.driver);

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

std::string_view find_odbc_driver(std::string_view _first, std::string_view _second, std::string_view _thrid)
{
	for (auto& iter : g_vector_odbc_driver_info)
	{
		// - to lower
		std::string str_iter_lower = __to_lower<char>(iter.driver);

		// - find _first
		auto pos = str_iter_lower.find(_first, 0);

		if (pos < str_iter_lower.size())
		{
			// - find _second
			pos = str_iter_lower.find(_second, pos);

			// check) 
			CONTINUE_IF(pos >= str_iter_lower.size())

			// - find _third
			pos = str_iter_lower.find(_thrid, pos);

			// check) 
			CONTINUE_IF(pos >= str_iter_lower.size())

			// return) 
			return iter.driver;
		}
	}

	// return)
	return std::string_view();
}

std::string_view find_odbc_driver(std::string_view _first, std::string_view _second, std::string_view _thrid, std::string_view _forth)
{
	for (auto& iter : g_vector_odbc_driver_info)
	{
		// - to lower
		std::string str_iter_lower = __to_lower<char>(iter.driver);

		// - find _first
		auto pos = str_iter_lower.find(_first, 0);

		if (pos < str_iter_lower.size())
		{
			// - find _second
			pos = str_iter_lower.find(_second, pos);

			// check) 
			CONTINUE_IF(pos >= str_iter_lower.size())

			// - find _third
			pos = str_iter_lower.find(_thrid, pos);

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

std::string_view get_suitable_oledb_provider(eDB_PROVIDER _provider)
{
	// check) 
	scoped_lock(g_vector_oledb_provider_info)
	{
		if (g_vector_oledb_provider_info.empty())
		{
			init_oledb_provider();
		}
	}

	// declare) 
	std::string_view temp;

	switch (_provider & eDB_PROVIDER::MASK_)
	{
	case	eDB_PROVIDER::DSN:					temp = find_oledb_provider("msdasql"); break;
	case	eDB_PROVIDER::SQL_SERVER:			temp = find_oledb_provider("msoledbsql"); if(temp.empty()) temp = find_oledb_provider("sqloledb");  break;
	case	eDB_PROVIDER::SQL_SERVER_18:		temp = find_oledb_provider("msoledbsql"); if (temp.empty()) temp = find_oledb_provider("sqloledb");  break;
	case	eDB_PROVIDER::SQL_SERVER_NATIVE:	temp = find_oledb_provider("sqlncli"); break;
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
	case	eDB_PROVIDER::EXCEL:				temp = find_oledb_provider("microsoft.ace.oledb"); break;
	case	eDB_PROVIDER::ACCESS:				temp = find_oledb_provider("microsoft.ace.oledb"); break;
	case	eDB_PROVIDER::ACCESS_DBASE:			temp = find_oledb_provider("microsoft.ace.oledb"); break;
	case	eDB_PROVIDER::ACCESS_TEXT:			temp = find_oledb_provider("microsoft.ace.oledb"); break;

	default:
			break;
	}

	// return)
	return	temp;
}

std::string_view get_suitable_odbc_driver(eDB_PROVIDER _provider)
{
	// check) 
	scoped_lock(g_vector_odbc_driver_info)
	{
		if (g_vector_odbc_driver_info.empty())
		{
			init_odbc_driver();
		}
	}

	// declare) 
	std::string_view temp;

	// 주의) 비교를 위한 문자열은 반드시 소문자로 비교해야한다.
	//       (case in-sensitive한 비교를 위해 소문자끼리 비교를 하기 때문이다.)

	switch (_provider & eDB_PROVIDER::MASK_)
	{
	case	eDB_PROVIDER::DSN:					break;
	case	eDB_PROVIDER::SQL_SERVER:			temp = find_odbc_driver("sql server"); break;
	case	eDB_PROVIDER::SQL_SERVER_18:		break;
	case	eDB_PROVIDER::SQL_SERVER_NATIVE:	temp = find_odbc_driver("sql server native client"); break;
	case	eDB_PROVIDER::SQL_SERVER_ODBC:		temp = find_odbc_driver("odbc", "sql server"); break;
	case	eDB_PROVIDER::MYSQL:				temp = find_odbc_driver("mysql", "ansi", "driver"); break;
	case	eDB_PROVIDER::MYSQL_ANSI:			temp = find_odbc_driver("mysql", "ansi", "driver"); break;
	case	eDB_PROVIDER::MYSQL_UNICODE:		temp = find_odbc_driver("mysql", "unicode", "driver"); break;
	case	eDB_PROVIDER::MYSQL_3_5:			temp = find_odbc_driver("mysql", "3.5", "ansi", "driver"); break; 
	case	eDB_PROVIDER::MYSQL_5_1:			temp = find_odbc_driver("mysql", "5.1", "ansi", "driver"); break;
	case	eDB_PROVIDER::MYSQL_5_1_ANSI:		temp = find_odbc_driver("mysql", "5.1", "ansi", "driver"); break;
	case	eDB_PROVIDER::MYSQL_5_1_UNICODE:	temp = find_odbc_driver("mysql", "5.1", "unicode", "driver"); break;
	case	eDB_PROVIDER::MYSQL_5_2:			temp = find_odbc_driver("mysql", "5.2", "ansi", "driver"); break;
	case	eDB_PROVIDER::MYSQL_5_2_ANSI:		temp = find_odbc_driver("mysql", "5.2", "ansi", "driver"); break;
	case	eDB_PROVIDER::MYSQL_5_2_UNICODE:	temp = find_odbc_driver("mysql", "5.2", "unicode", "driver"); break;
	case	eDB_PROVIDER::MYSQL_5_3:			temp = find_odbc_driver("mysql", "5.3", "ansi", "driver"); break;
	case	eDB_PROVIDER::MYSQL_5_3_ANSI:		temp = find_odbc_driver("mysql", "5.3", "ansi", "driver"); break;
	case	eDB_PROVIDER::MYSQL_5_3_UNICODE:	temp = find_odbc_driver("mysql", "5.3", "unicode", "driver"); break;
	case	eDB_PROVIDER::MYSQL_8:				temp = find_odbc_driver("mysql", "8.0", "ansi", "driver"); break;
	case	eDB_PROVIDER::MYSQL_8_ANSI:			temp = find_odbc_driver("mysql", "8.0", "ansi", "driver"); break;
	case	eDB_PROVIDER::MYSQL_8_UNICODE:		temp = find_odbc_driver("mysql", "8.0", "unicode", "driver"); break;
	case	eDB_PROVIDER::MARIADB:				temp = find_odbc_driver("mariadb", "odbc"); break;
	case	eDB_PROVIDER::MARIADB_3_1:			temp = find_odbc_driver("mariadb", "odbc", "3.1"); break;
	case	eDB_PROVIDER::EXCEL:				temp = find_odbc_driver("microsoft excel driver"); break;
	case	eDB_PROVIDER::ACCESS:				temp = find_odbc_driver("microsoft access driver"); break;
	case	eDB_PROVIDER::ACCESS_DBASE:			temp = find_odbc_driver("microsoft access dBASE driver"); break;
	case	eDB_PROVIDER::ACCESS_TEXT:			temp = find_odbc_driver("microsoft access text driver"); break;

	default:
			break;
	}

	// return)
	return	temp;
}



}