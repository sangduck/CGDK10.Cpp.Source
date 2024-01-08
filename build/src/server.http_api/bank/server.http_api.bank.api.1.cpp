#include "pch.h"


namespace CGDK
{

template<>
std::string_view to_string<char>(eOAUTH_SCOPE _scope) noexcept
{
	// declare) 
	std::string_view	str_result;

	switch (_scope)
	{
	case		eOAUTH_SCOPE::LOGIN_:					str_result = "login"sv; break;
	case		eOAUTH_SCOPE::INQUIRY_:					str_result = "inquiry"sv; break;
	case		eOAUTH_SCOPE::TRANSFER_:				str_result = "transfer"sv; break;
	case		eOAUTH_SCOPE::LOGIN_INQUIRY_:			str_result = "login+inquiry"sv; break;
	case		eOAUTH_SCOPE::LOGIN_TRNASFER_:			str_result = "login+transfer"sv; break;
	case		eOAUTH_SCOPE::INQUIRY_TRNASFER_:		str_result = "inquiry+transfer"sv; break;
	case		eOAUTH_SCOPE::LOGIN_INQUIRY_TRNASFER_:	str_result = "login+inquiry+transfer"sv; break;
	case		eOAUTH_SCOPE::OOB_:						str_result = "oob"sv; break;
	default:	CGASSERT_ERROR(false);					str_result = ""sv; break;
	}

	// Return)
	return	str_result;
}
template<>
std::wstring_view to_string<wchar_t>(eOAUTH_SCOPE _scope) noexcept
{
	// declare) 
	std::wstring_view	str_result;

	switch (_scope)
	{
	case		eOAUTH_SCOPE::LOGIN_:					str_result = L"login"sv; break;
	case		eOAUTH_SCOPE::INQUIRY_:					str_result = L"inquiry"sv; break;
	case		eOAUTH_SCOPE::TRANSFER_:				str_result = L"transfer"sv; break;
	case		eOAUTH_SCOPE::LOGIN_INQUIRY_:			str_result = L"login+inquiry"sv; break;
	case		eOAUTH_SCOPE::LOGIN_TRNASFER_:			str_result = L"login+transfer"sv; break;
	case		eOAUTH_SCOPE::INQUIRY_TRNASFER_:		str_result = L"inquiry+transfer"sv; break;
	case		eOAUTH_SCOPE::LOGIN_INQUIRY_TRNASFER_:	str_result = L"login+inquiry+transfer"sv; break;
	case		eOAUTH_SCOPE::OOB_:						str_result = L"oob"sv; break;
	default:	CGASSERT_ERROR(false);					str_result = L""sv; break;
	}

	// Return)
	return	str_result;
}

template<>
std::string_view to_string<char>(eOAUTH_YESNO _value) noexcept
{
	std::string_view str_return;

	switch (_value)
	{
	case		eOAUTH_YESNO::YES_:							str_return = "Y"sv; break;
	case		eOAUTH_YESNO::NO_:							str_return = "N"sv; break;
	default:	CGASSERT_ERROR(false);
	}

	// return) 
	return	str_return;
}
template<>
std::wstring_view to_string<wchar_t>(eOAUTH_YESNO _value) noexcept
{
	std::wstring_view str_return;

	switch (_value)
	{
	case		eOAUTH_YESNO::YES_:							str_return = L"Y"sv; break;
	case		eOAUTH_YESNO::NO_:							str_return = L"N"sv; break;
	default:	CGASSERT_ERROR(false);
	}

	// return) 
	return	str_return;
}

template<>
std::string_view to_string<char>(eOAUTH_SORT_ORDER _value) noexcept
{
	std::string_view str_return;

	switch(_value)
	{
	case		eOAUTH_SORT_ORDER::ASCENDING_:				str_return = "A"sv; break;
	case		eOAUTH_SORT_ORDER::DESCENDING_:				str_return = "D"sv; break;
	default:	CGASSERT_ERROR(false);
	}

	// return) 
	return	str_return;
}
template<>
std::wstring_view to_string<wchar_t>(eOAUTH_SORT_ORDER _value) noexcept
{
	std::wstring_view str_return;

	switch (_value)
	{
	case		eOAUTH_SORT_ORDER::ASCENDING_:				str_return = L"A"sv; break;
	case		eOAUTH_SORT_ORDER::DESCENDING_:				str_return = L"D"sv; break;
	default:	CGASSERT_ERROR(false);
	}

	// return) 
	return	str_return;
}

template<>
std::string_view to_string<char>(eOAUTH_ORG_REQ_GUBUN _value) noexcept
{
	std::string_view str_return;

	switch (_value)
	{
	case		eOAUTH_ORG_REQ_GUBUN::FINTECH_NUM_:			str_return = "1"sv; break;
	case		eOAUTH_ORG_REQ_GUBUN::ACCOUNT_NUM_:			str_return = "2"sv; break;
	default:	CGASSERT_ERROR(false);
	}

	// return) 
	return	str_return;
}
template<>
std::wstring_view to_string<wchar_t>(eOAUTH_ORG_REQ_GUBUN _value) noexcept
{
	std::wstring_view str_return;

	switch (_value)
	{
	case		eOAUTH_ORG_REQ_GUBUN::FINTECH_NUM_:			str_return = L"1"sv; break;
	case		eOAUTH_ORG_REQ_GUBUN::ACCOUNT_NUM_:			str_return = L"2"sv; break;
	default:	CGASSERT_ERROR(false);
	}

	// return) 
	return	str_return;
}

template<>
std::string_view to_string<char>(eOAUTH_INQUIRY_TYPE _value) noexcept
{
	std::string_view str_return;

	switch(_value)
	{
	case		eOAUTH_INQUIRY_TYPE::ALL_:					str_return = "A"sv; break;
	case		eOAUTH_INQUIRY_TYPE::IN_:					str_return = "I"sv; break;
	case		eOAUTH_INQUIRY_TYPE::OUT_:					str_return = "O"sv; break;
	default:	CGASSERT_ERROR(false);
	}

	// return) 
	return	str_return;
}
template<>
std::wstring_view to_string<wchar_t>(eOAUTH_INQUIRY_TYPE _value) noexcept
{
	std::wstring_view str_return;

	switch (_value)
	{
	case		eOAUTH_INQUIRY_TYPE::ALL_:					str_return = L"A"sv; break;
	case		eOAUTH_INQUIRY_TYPE::IN_:					str_return = L"I"sv; break;
	case		eOAUTH_INQUIRY_TYPE::OUT_:					str_return = L"O"sv; break;
	default:	CGASSERT_ERROR(false);
	}

	// return) 
	return	str_return;
}

template<>
std::string_view to_string<char>(eOAUTH_CHECK_TYPE _value) noexcept
{
	std::string_view str_return;

	switch(_value)
	{
	case		eOAUTH_CHECK_TYPE::OUT_:					str_return = "O"sv; break;
	case		eOAUTH_CHECK_TYPE::IN_:						str_return = "I"sv; break;
	default:	CGASSERT_ERROR(false);
	}

	// return) 
	return	str_return;
}
template<>
std::wstring_view to_string<wchar_t>(eOAUTH_CHECK_TYPE _value) noexcept
{
	std::wstring_view str_return;

	switch(_value)
	{
	case		eOAUTH_CHECK_TYPE::OUT_:					str_return = L"O"sv; break;
	case		eOAUTH_CHECK_TYPE::IN_:						str_return = L"I"sv; break;
	default:	CGASSERT_ERROR(false);
	}

	// return) 
	return	str_return;
}

template<>
std::string_view to_string<char>(eOAUTH_GENDER _value) noexcept
{
	std::string_view str_return;

	switch(_value)
	{
	case		eOAUTH_GENDER::MALE_:						str_return = "M"sv; break;
	case		eOAUTH_GENDER::FEMALE_:						str_return = "F"sv; break;
	default:	CGASSERT_ERROR(false);
	}

	// return) 
	return	str_return;
}
template<>
std::wstring_view to_string<wchar_t>(eOAUTH_GENDER _value) noexcept
{
	std::wstring_view str_return;

	switch(_value)
	{
	case		eOAUTH_GENDER::MALE_:				str_return = L"M"sv; break;
	case		eOAUTH_GENDER::FEMALE_:				str_return = L"F"sv; break;
	default:	CGASSERT_ERROR(false);
	}

	// return) 
	return	str_return;
}

template<>
std::string_view to_string<char>(eOAUTH_TOKEN_TYPE _value) noexcept
{
	std::string_view str_return;

	switch (_value)
	{
	case		eOAUTH_TOKEN_TYPE::BEARER_:			str_return = "Bearer"sv; break;
	default:	CGASSERT_ERROR(false);
	}

	// return) 
	return	str_return;
}
template<>
std::wstring_view to_string<wchar_t>(eOAUTH_TOKEN_TYPE _value) noexcept
{
	std::wstring_view str_return;

	switch (_value)
	{
	case		eOAUTH_TOKEN_TYPE::BEARER_:			str_return = L"Bearer"sv; break;
	default:	CGASSERT_ERROR(false);
	}

	// return) 
	return	str_return;
}

template<>
std::string_view to_string<char>(eOAUTH_ACCOUNT_TYPE _value) noexcept
{
	std::string_view str_return;

	switch (_value)
	{
	case		eOAUTH_ACCOUNT_TYPE::_1_:			str_return = "1"sv; break;
	case		eOAUTH_ACCOUNT_TYPE::_2_:			str_return = "2"sv; break;
	case		eOAUTH_ACCOUNT_TYPE::_6_:			str_return = "6"sv; break;
	default:	CGASSERT_ERROR(false);
	}

	// return) 
	return	str_return;
}
template<>
std::wstring_view to_string<wchar_t>(eOAUTH_ACCOUNT_TYPE _value) noexcept
{
	std::wstring_view str_return;

	switch (_value)
	{
	case		eOAUTH_ACCOUNT_TYPE::_1_:			str_return = L"1"sv; break;
	case		eOAUTH_ACCOUNT_TYPE::_2_:			str_return = L"2"sv; break;
	case		eOAUTH_ACCOUNT_TYPE::_6_:			str_return = L"6"sv; break;
	default:	CGASSERT_ERROR(false);
	}

	// return) 
	return	str_return;
}

template<>
std::string_view to_string<char>(eOAUTH_INOUT_TYPE _value) noexcept
{
	std::string_view str_return;

	switch (_value)
	{
	case		eOAUTH_INOUT_TYPE::IN_:				str_return = "Inbound"sv; break;
	case		eOAUTH_INOUT_TYPE::OUT_:			str_return = "Outbound"sv; break;
	case		eOAUTH_INOUT_TYPE::NOT_DEFINED_:	str_return = "Undefined"sv; break;
	default:	CGASSERT_ERROR(false);
	}

	// return) 
	return	str_return;
}
template<>
std::wstring_view to_string<wchar_t>(eOAUTH_INOUT_TYPE _value) noexcept
{
	std::wstring_view str_return;

	switch (_value)
	{
	case		eOAUTH_INOUT_TYPE::IN_:				str_return = L"Inbound"sv; break;
	case		eOAUTH_INOUT_TYPE::OUT_:			str_return = L"Outbound"sv; break;
	case		eOAUTH_INOUT_TYPE::NOT_DEFINED_:	str_return = L"Undefined"sv; break;
	default:	CGASSERT_ERROR(false);
	}

	// return) 
	return	str_return;
}

template<>
eOAUTH_ACCOUNT_TYPE get_enum_OAUTH_ACCOUNT_TYPE<char>(std::string_view _string) noexcept
{
	// check)
	CGASSERT_ERROR(_string.empty() == false);

	// check)
	RETURN_IF(_string.empty(), eOAUTH_ACCOUNT_TYPE::NOT_DEFINED_);

	// declare)
	eOAUTH_ACCOUNT_TYPE	result;

	// - "access_token"
	switch (_string.at(0))
	{
		// Case 거래가능
	case	'1':
		result = eOAUTH_ACCOUNT_TYPE::_1_;
		break;

		// Case 장애
	case	'2':
		result = eOAUTH_ACCOUNT_TYPE::_2_;
		break;

		// Case 개시이전
	case	'6':
		result = eOAUTH_ACCOUNT_TYPE::_6_;
		break;

	default:
		result = eOAUTH_ACCOUNT_TYPE::NOT_DEFINED_;
		break;
	}

	// Return)
	return	result;
}
template<>
eOAUTH_ACCOUNT_TYPE get_enum_OAUTH_ACCOUNT_TYPE<wchar_t>(std::wstring_view _string) noexcept
{
	// check)
	CGASSERT_ERROR(_string.empty() == false);

	// check)
	RETURN_IF(_string.empty(), eOAUTH_ACCOUNT_TYPE::NOT_DEFINED_);

	// declare)
	eOAUTH_ACCOUNT_TYPE	result;

	// - "access_token"
	switch (_string.at(0))
	{
		// Case 거래가능
	case	'1':
		result = eOAUTH_ACCOUNT_TYPE::_1_;
		break;

		// Case 장애
	case	'2':
		result = eOAUTH_ACCOUNT_TYPE::_2_;
		break;

		// Case 개시이전
	case	'6':
		result = eOAUTH_ACCOUNT_TYPE::_6_;
		break;

	default:
		result = eOAUTH_ACCOUNT_TYPE::NOT_DEFINED_;
		break;
	}

	// Return)
	return	result;
}

template<>
eOAUTH_SCOPE get_enum_OAUTH_SCOPE<char>(std::string_view _string) noexcept
{
	// check)
	CGASSERT_ERROR(_string.empty() == false);

	// check)
	RETURN_IF(_string.empty(), eOAUTH_SCOPE::NOT_DEFINED_);

	// declare)
	eOAUTH_SCOPE result;

	// - "access_token"
	if (_string == "login"sv)
	{
		result = eOAUTH_SCOPE::LOGIN_;
	}
	// - "token_type"
	else if (_string == "inquiry"sv)
	{
		result = eOAUTH_SCOPE::INQUIRY_;
	}
	// - "expires_in"
	else if (_string == "transfer"sv)
	{
		result = eOAUTH_SCOPE::TRANSFER_;
	}
	// - "scope"
	else if (_string == "login inquiry"sv)
	{
		result = eOAUTH_SCOPE::LOGIN_INQUIRY_;
	}
	// - "user_seq_no"
	else if (_string == "login transfer"sv)
	{
		result = eOAUTH_SCOPE::LOGIN_TRNASFER_;
	}
	// - "inquiry transfer"
	else if (_string == "inquiry transfer"sv)
	{
		result = eOAUTH_SCOPE::INQUIRY_TRNASFER_;
	}
	else if (_string == "login inquiry transfer"sv)
	{
		result = eOAUTH_SCOPE::LOGIN_INQUIRY_TRNASFER_;
	}
	else if (_string == "oob"sv)
	{
		result = eOAUTH_SCOPE::OOB_;
	}
	else
	{
		result = eOAUTH_SCOPE::NOT_DEFINED_;
	}

	// Return)
	return	result;
}
template<>
eOAUTH_SCOPE get_enum_OAUTH_SCOPE<wchar_t>(std::wstring_view _string) noexcept
{
	// check)
	CGASSERT_ERROR(_string.empty() == false);

	// check)
	RETURN_IF(_string.empty(), eOAUTH_SCOPE::NOT_DEFINED_);

	// declare)
	eOAUTH_SCOPE result;

	// - "access_token"
	if (_string == L"login"sv)
	{
		result = eOAUTH_SCOPE::LOGIN_;
	}
	// - "token_type"
	else if (_string == L"inquiry"sv)
	{
		result = eOAUTH_SCOPE::INQUIRY_;
	}
	// - "expires_in"
	else if (_string == L"transfer"sv)
	{
		result = eOAUTH_SCOPE::TRANSFER_;
	}
	// - "scope"
	else if (_string == L"login inquiry"sv)
	{
		result = eOAUTH_SCOPE::LOGIN_INQUIRY_;
	}
	// - "user_seq_no"
	else if (_string == L"login transfer"sv)
	{
		result = eOAUTH_SCOPE::LOGIN_TRNASFER_;
	}
	// - "inquiry transfer"
	else if (_string == L"inquiry transfer"sv)
	{
		result = eOAUTH_SCOPE::INQUIRY_TRNASFER_;
	}
	else if (_string == L"login inquiry transfer"sv)
	{
		result = eOAUTH_SCOPE::LOGIN_INQUIRY_TRNASFER_;
	}
	else if (_string == L"oob"sv)
	{
		result = eOAUTH_SCOPE::OOB_;
	}
	else
	{
		result = eOAUTH_SCOPE::NOT_DEFINED_;
	}

	// Return)
	return	result;
}

template<>
eOAUTH_BANK_STATUS get_enum_OAUTH_BANK_STATUS<char>(std::string_view _string) noexcept
{
	// check)
	CGASSERT_ERROR(_string.empty() == false);

	// check)
	RETURN_IF(_string.empty(), eOAUTH_BANK_STATUS::NOT_DEFINED_);

	// declare)
	eOAUTH_BANK_STATUS result;

	// - "access_token" 
	switch (_string.at(0))
	{
		// Case 거래가능
	case	'Y':
		result = eOAUTH_BANK_STATUS::OPEN_;
		break;

		// Case 장애
	case	'D':
		result = eOAUTH_BANK_STATUS::DISORDER_;
		break;

		// Case 개시이전
	case	'L':
		result = eOAUTH_BANK_STATUS::LAUNCH_;
		break;

		// Case 종료처리중
	case	'E':
		result = eOAUTH_BANK_STATUS::END;
		break;

	default:
		result = eOAUTH_BANK_STATUS::NOT_DEFINED_;
		break;
	}

	// Return)
	return	result;
}
template<>
eOAUTH_BANK_STATUS get_enum_OAUTH_BANK_STATUS<wchar_t>(std::wstring_view _string) noexcept
{
	// check)
	CGASSERT_ERROR(_string.empty() == false);

	// check)
	RETURN_IF(_string.empty(), eOAUTH_BANK_STATUS::NOT_DEFINED_);

	// declare)
	eOAUTH_BANK_STATUS result;

	// - "access_token" 
	switch (_string.at(0))
	{
		// Case 거래가능
	case	'Y':
		result = eOAUTH_BANK_STATUS::OPEN_;
		break;

		// Case 장애
	case	'D':
		result = eOAUTH_BANK_STATUS::DISORDER_;
		break;

		// Case 개시이전
	case	'L':
		result = eOAUTH_BANK_STATUS::LAUNCH_;
		break;

		// Case 종료처리중
	case	'E':
		result = eOAUTH_BANK_STATUS::END;
		break;

	default:
		result = eOAUTH_BANK_STATUS::NOT_DEFINED_;
		break;
	}

	// Return)
	return	result;
}

template<>
eOAUTH_YESNO get_enum_OAUTH_YESNO<char>(std::string_view _string) noexcept
{
	// check)
	CGASSERT_ERROR(_string.empty() == false);

	// check)
	RETURN_IF(_string.empty(), eOAUTH_YESNO::NOT_DEFINED_);

	// declare)
	eOAUTH_YESNO result;

	// - "access_token"
	switch (_string.at(0))
	{
		// Case Yes
	case	'Y':
		result = eOAUTH_YESNO::YES_;
		break;

		// Case No
	case	'N':
		result = eOAUTH_YESNO::NO_;
		break;

	default:
		result = eOAUTH_YESNO::NOT_DEFINED_;
		break;
	}

	// Return)
	return	result;
}
template<>
eOAUTH_YESNO get_enum_OAUTH_YESNO<wchar_t>(std::wstring_view _string) noexcept
{
	// check)
	CGASSERT_ERROR(_string.empty() == false);

	// check)
	RETURN_IF(_string.empty(), eOAUTH_YESNO::NOT_DEFINED_);

	// declare)
	eOAUTH_YESNO result;

	// - "access_token"
	switch (_string.at(0))
	{
		// Case Yes
	case	'Y':
		result = eOAUTH_YESNO::YES_;
		break;

		// Case No
	case	'N':
		result = eOAUTH_YESNO::NO_;
		break;

	default:
		result = eOAUTH_YESNO::NOT_DEFINED_;
		break;
	}

	// Return)
	return	result;
}

template<>
eOAUTH_BANK_CODE get_enum_OAUTH_BANK_CODE<char>(std::string_view _string) noexcept
{
	// check)
	CGASSERT_ERROR(_string.empty() == false);

	// check)
	RETURN_IF(_string.empty(), 0);

	// 1) Value를 얻음.
	int	value = __from_string<int>(_string);

	// check)값이 유효한지 검사.


	// Return)
	return	static_cast<eOAUTH_BANK_CODE>(value);
}
template<>
eOAUTH_BANK_CODE get_enum_OAUTH_BANK_CODE<wchar_t>(std::wstring_view _string) noexcept
{
	// check)
	CGASSERT_ERROR(_string.empty() == false);

	// check)
	RETURN_IF(_string.empty(), 0);

	// 1) Value를 얻음.
	int	value = __from_string<int>(_string);

	// check)값이 유효한지 검사.


	// Return)
	return	static_cast<eOAUTH_BANK_CODE>(value);
}

template<>
eOAUTH_TOKEN_TYPE get_enum_OAUTH_TOKEN_TYPE<char>(std::string_view _string) noexcept
{
	// check)
	CGASSERT_ERROR(_string.empty() == false);

	// check)
	RETURN_IF(_string.empty(), eOAUTH_TOKEN_TYPE::NOT_DEFINED_);

	// declare)
	eOAUTH_TOKEN_TYPE result;

	// - "access_token"
	if (_string == "Bearer"sv)
	{
		result = eOAUTH_TOKEN_TYPE::BEARER_;
	}
	else
	{
		result = eOAUTH_TOKEN_TYPE::NOT_DEFINED_;
	}

	// Return)
	return	result;
}
template<>
eOAUTH_TOKEN_TYPE get_enum_OAUTH_TOKEN_TYPE<wchar_t>(std::wstring_view _string) noexcept
{
	// check)
	CGASSERT_ERROR(_string.empty() == false);

	// check)
	RETURN_IF(_string.empty(), eOAUTH_TOKEN_TYPE::NOT_DEFINED_);

	// declare)
	eOAUTH_TOKEN_TYPE result;

	// - "access_token"
	if (_string == L"Bearer"sv)
	{
		result = eOAUTH_TOKEN_TYPE::BEARER_;
	}
	else
	{
		result = eOAUTH_TOKEN_TYPE::NOT_DEFINED_;
	}

	// Return)
	return	result;
}

template<>
eOAUTH_GENDER get_enum_OAUTH_GENDER<char>(std::string_view _string) noexcept
{
	// check)
	CGASSERT_ERROR(_string.empty() == false);

	// check) 
	RETURN_IF(_string.empty(), eOAUTH_GENDER::NOT_DEFINED_);

	// declare)
	eOAUTH_GENDER result;

	// - "access_token"
	switch (_string.at(0))
	{
		// Case Yes
	case	'M':
		result = eOAUTH_GENDER::MALE_;
		break;

		// Case No
	case	'F':
		result = eOAUTH_GENDER::FEMALE_;
		break;

	default:
		result = eOAUTH_GENDER::NOT_DEFINED_;
		break;
	}

	// return) 
	return	result;
}
template<>
eOAUTH_GENDER get_enum_OAUTH_GENDER<wchar_t>(std::wstring_view _string) noexcept
{
	// check)
	CGASSERT_ERROR(_string.empty() == false);

	// check) 
	RETURN_IF(_string.empty(), eOAUTH_GENDER::NOT_DEFINED_);

	// declare)
	eOAUTH_GENDER result;

	// - "access_token"
	switch (_string.at(0))
	{
		// Case Yes
	case	'M':
			result = eOAUTH_GENDER::MALE_;
			break;

		// Case No
	case	'F':
			result = eOAUTH_GENDER::FEMALE_;
			break;

	default:
			result = eOAUTH_GENDER::NOT_DEFINED_;
			break;
	}

	// return) 
	return	result;
}

chrono::date_time from_string_date_time_millsec(std::string_view _string)
{
	// declare) 
	chrono::date_time temp;

	// check)
	RETURN_IF(_string.empty(), temp);

	// check)
	RETURN_IF(_string.at(0) == 0, temp);

	// 1) Year
	temp.t_year = ((_string.at(0) - '0') * 1000 + (_string.at(1) - '0') * 100 + (_string.at(2) - '0') * 10 + (_string.at(3) - '0'));

	// 2) Month
	temp.t_month = ((_string.at(4) - '0') * 10 + (_string.at(5) - '0'));

	// 3) Day
	temp.t_day = ((_string.at(6) - '0') * 10 + (_string.at(7) - '0'));

	// 4) Hour
	temp.t_hour = ((_string.at(8) - '0') * 10 + (_string.at(9) - '0'));

	// 5) Minute
	temp.t_minute = ((_string.at(10) - '0') * 10 + (_string.at(11) - '0'));

	// 6) Second
	temp.t_second = ((_string.at(12) - '0') * 10 + (_string.at(13) - '0'));

	// 7) millisecond
	temp.t_fraction = ((_string.at(14) - '0') * 100 + (_string.at(15) - '0') * 10 + (_string.at(16) - '0')) * 10000;

	// return) 
	return temp;
}
chrono::date_time from_string_date_time(std::string_view _string)
{
	// declare) 
	chrono::date_time temp;

	// check)
	RETURN_IF(_string.empty(), temp);

	// check)
	RETURN_IF(_string.at(0) == 0, temp);

	// 1) Year
	temp.t_year = ((_string.at(0) - '0') * 1000 + (_string.at(1) - '0') * 100 + (_string.at(2) - '0') * 10 + (_string.at(3) - '0'));

	// 2) Month
	temp.t_month = ((_string.at(4) - '0') * 10 + (_string.at(5) - '0'));

	// 3) Day
	temp.t_day = ((_string.at(6) - '0') * 10 + (_string.at(7) - '0'));

	// 4) Hour
	temp.t_hour = ((_string.at(8) - '0') * 10 + (_string.at(9) - '0'));

	// 5) Minute
	temp.t_minute = ((_string.at(10) - '0') * 10 + (_string.at(11) - '0'));

	// 6) Second
	temp.t_second = ((_string.at(12) - '0') * 10 + (_string.at(13) - '0'));

	// 7) millisecond
	temp.t_fraction = 0;

	// return) 
	return temp;
}
chrono::date_time from_string_date(std::string_view _string)
{
	// check)
	CGASSERT_ERROR(_string.empty() == false);

	// check)
	CGASSERT_ERROR(_string.size() >= 8);

	// declare) 
	chrono::date_time temp;

	// check)
	RETURN_IF(_string.empty(), temp);

	// check)
	RETURN_IF(_string.at(0) == 0, temp);

	// 1) Year
	temp.t_year = ((_string.at(0) - '0') * 1000 + (_string.at(1) - '0') * 100 + (_string.at(2) - '0') * 10 + (_string.at(3) - '0'));

	// 2) Month
	temp.t_month = ((_string.at(4) - '0') * 10 + (_string.at(5) - '0'));

	// 3) Day
	temp.t_day = ((_string.at(6) - '0') * 10 + (_string.at(7) - '0'));

	// 4) Hour
	temp.t_hour = 0;

	// 5) Minute
	temp.t_minute = 0;

	// 6) Second
	temp.t_second = 0;

	// 7) millisecond
	temp.t_fraction = 0;

	// return) 
	return temp;
}
chrono::date_time from_string_time(std::string_view _string)
{
	// check)
	CGASSERT_ERROR(_string.empty() == false);

	// check)
	CGASSERT_ERROR(_string.size() >= 6);

	// declare) 
	chrono::date_time temp;

	// check)
	RETURN_IF(_string.empty(), temp);

	// check)
	RETURN_IF(_string[0] == 0, temp);

	// 1) Year
	temp.t_year = 0;

	// 2) Month
	temp.t_month = 0;

	// 3) Day
	temp.t_day = 0;

	// 4) Hour
	temp.t_hour = ((_string.at(0) - '0') * 10 + (_string.at(1) - '0'));

	// 5) Minute
	temp.t_minute = ((_string.at(2) - '0') * 10 + (_string.at(3) - '0'));

	// 6) Second
	temp.t_second = ((_string.at(4) - '0') * 10 + (_string.at(5) - '0'));

	// 7) millisecond
	temp.t_fraction = 0;

	// return) 
	return	temp;
}


}