#include "pch.h"


namespace CGDK
{

template<class T_CHAR>
std::basic_string_view<T_CHAR> _get_provider_string(eDB_PROVIDER _provider);

template<>
std::string_view _get_provider_string<char>(eDB_PROVIDER _provider)
{
	// declare) 
	std::string_view	str_provider;

	switch (_provider)
	{
	case	eDB_PROVIDER::NONE:					str_provider = "none"; break;
	case	eDB_PROVIDER::DSN:					str_provider = "dsn"; break;
	case	eDB_PROVIDER::SQL_SERVER:			str_provider = "sql_server"; break;
	case	eDB_PROVIDER::SQL_SERVER_NATIVE:	str_provider = "sql_server_native"; break;
	case	eDB_PROVIDER::SQL_SERVER_18:		str_provider = "sql_server_18"; break;
	case	eDB_PROVIDER::SQL_SERVER_ODBC:		str_provider = "sql_server_odbc"; break;
	case	eDB_PROVIDER::MYSQL:				str_provider = "mysql"; break;
	case	eDB_PROVIDER::MYSQL_ANSI:			str_provider = "mysql_ansi"; break;
	case	eDB_PROVIDER::MYSQL_UNICODE:		str_provider = "mysql_unicode"; break;
	case	eDB_PROVIDER::MYSQL_3_5:			str_provider = "mysql_3_5"; break;
	case	eDB_PROVIDER::MYSQL_5_1:			str_provider = "mysql_5_1"; break;
	case	eDB_PROVIDER::MYSQL_5_1_ANSI:		str_provider = "mysql_5_1_ansi"; break;
	case	eDB_PROVIDER::MYSQL_5_1_UNICODE:	str_provider = "mysql_5_1_unicode"; break;
	case	eDB_PROVIDER::MYSQL_5_2:			str_provider = "mysql_5_2"; break;
	case	eDB_PROVIDER::MYSQL_5_2_ANSI:		str_provider = "mysql_5_2_ansi"; break;
	case	eDB_PROVIDER::MYSQL_5_2_UNICODE:	str_provider = "mysql_5_2_unicode"; break;
	case	eDB_PROVIDER::MYSQL_5_3:			str_provider = "mysql_5_3"; break;
	case	eDB_PROVIDER::MYSQL_5_3_ANSI:		str_provider = "mysql_5_3_ansi"; break;
	case	eDB_PROVIDER::MYSQL_5_3_UNICODE:	str_provider = "mysql_5_3_unicode"; break;
	case	eDB_PROVIDER::MYSQL_8:				str_provider = "mysql_8"; break;
	case	eDB_PROVIDER::MYSQL_8_ANSI:			str_provider = "mysql_8_ansi"; break;
	case	eDB_PROVIDER::MYSQL_8_UNICODE:		str_provider = "mysql_8_unicode"; break;
	case	eDB_PROVIDER::MYSQL_9:				str_provider = "mysql_9"; break;
	case	eDB_PROVIDER::MYSQL_9_ANSI:			str_provider = "mysql_9_ansi"; break;
	case	eDB_PROVIDER::MYSQL_9_UNICODE:		str_provider = "mysql_9_unicode"; break;
	case	eDB_PROVIDER::MYSQL_9_7:			str_provider = "mysql_9_7"; break;
	case	eDB_PROVIDER::MYSQL_9_7_ANSI:		str_provider = "mysql_9_7_ansi"; break;
	case	eDB_PROVIDER::MYSQL_9_7_UNICODE:	str_provider = "mysql_9_7_unicode"; break;
	case	eDB_PROVIDER::MARIADB:				str_provider = "mariadb"; break;
	case	eDB_PROVIDER::MARIADB_3_1:			str_provider = "mariadb_3_1"; break;
	case	eDB_PROVIDER::EXCEL:				str_provider = "excel"; break;
	case	eDB_PROVIDER::ACCESS:				str_provider = "access"; break;
	case	eDB_PROVIDER::ACCESS_DBASE:			str_provider = "access_dbase"; break;
	case	eDB_PROVIDER::ACCESS_TEXT:			str_provider = "access_text"; break;

	case	eDB_PROVIDER::EXCEL_CREATE_:		str_provider = "excel_create"; break;

	default:									break;
	}

	// return)
	return	str_provider;
}

template<>
std::wstring_view _get_provider_string<wchar_t>(eDB_PROVIDER _provider)
{
	// declare) 
	std::wstring_view	str_provider;

	switch (_provider)
	{
	case	eDB_PROVIDER::NONE:					str_provider = L"none"; break;
	case	eDB_PROVIDER::DSN:					str_provider = L"dsn"; break;
	case	eDB_PROVIDER::SQL_SERVER:			str_provider = L"sql_server"; break;
	case	eDB_PROVIDER::SQL_SERVER_NATIVE:	str_provider = L"sql_server_native"; break;
	case	eDB_PROVIDER::SQL_SERVER_18:		str_provider = L"sql_server_18"; break;
	case	eDB_PROVIDER::SQL_SERVER_ODBC:		str_provider = L"odbc driver 17 for sql server"; break;
	case	eDB_PROVIDER::MYSQL:				str_provider = L"mysql"; break;
	case	eDB_PROVIDER::MYSQL_ANSI:			str_provider = L"mysql_ansi"; break;
	case	eDB_PROVIDER::MYSQL_UNICODE:		str_provider = L"mysql_unicode"; break;
	case	eDB_PROVIDER::MYSQL_3_5:			str_provider = L"mysql_3_5"; break;
	case	eDB_PROVIDER::MYSQL_5_1:			str_provider = L"mysql_5_1"; break;
	case	eDB_PROVIDER::MYSQL_5_1_ANSI:		str_provider = L"mysql_5_1_ansi"; break;
	case	eDB_PROVIDER::MYSQL_5_1_UNICODE:	str_provider = L"mysql_5_1_unicode"; break;
	case	eDB_PROVIDER::MYSQL_5_2:			str_provider = L"mysql_5_2"; break;
	case	eDB_PROVIDER::MYSQL_5_2_ANSI:		str_provider = L"mysql_5_2_ansi"; break;
	case	eDB_PROVIDER::MYSQL_5_2_UNICODE:	str_provider = L"mysql_5_2_unicode"; break;
	case	eDB_PROVIDER::MYSQL_5_3:			str_provider = L"mysql_5_3"; break;
	case	eDB_PROVIDER::MYSQL_5_3_ANSI:		str_provider = L"mysql_5_3_ansi"; break;
	case	eDB_PROVIDER::MYSQL_5_3_UNICODE:	str_provider = L"mysql_5_3_unicode"; break;
	case	eDB_PROVIDER::MYSQL_8:				str_provider = L"mysql_8"; break;
	case	eDB_PROVIDER::MYSQL_8_ANSI:			str_provider = L"mysql_8_ansi"; break;
	case	eDB_PROVIDER::MYSQL_8_UNICODE:		str_provider = L"mysql_8_unicode"; break;
	case	eDB_PROVIDER::MYSQL_9:				str_provider = L"mysql_9"; break;
	case	eDB_PROVIDER::MYSQL_9_ANSI:			str_provider = L"mysql_9_ansi"; break;
	case	eDB_PROVIDER::MYSQL_9_UNICODE:		str_provider = L"mysql_9_unicode"; break;
	case	eDB_PROVIDER::MYSQL_9_7:			str_provider = L"mysql_9_7"; break;
	case	eDB_PROVIDER::MYSQL_9_7_ANSI:		str_provider = L"mysql_9_7_ansi"; break;
	case	eDB_PROVIDER::MYSQL_9_7_UNICODE:	str_provider = L"mysql_9_7_unicode"; break;
	case	eDB_PROVIDER::MARIADB:				str_provider = L"mariadb"; break;
	case	eDB_PROVIDER::MARIADB_3_1:			str_provider = L"mariadb_3_1"; break;
	case	eDB_PROVIDER::EXCEL:				str_provider = L"excel"; break;
	case	eDB_PROVIDER::ACCESS:				str_provider = L"access"; break;
	case	eDB_PROVIDER::ACCESS_DBASE:			str_provider = L"access_dbase"; break;
	case	eDB_PROVIDER::ACCESS_TEXT:			str_provider = L"access_text"; break;

	case	eDB_PROVIDER::EXCEL_CREATE_:		str_provider = L"excel_create"; break;

	default:									break;
	}

	// return)
	return	str_provider;
}


}
