#include "pch.h"

namespace CGDK
{

template <>
std::string_view to_string<char>(eSCHEDULABLE_TYPE _executor_type) noexcept
{
	std::string_view result;

	switch (_executor_type)
	{
	case	eSCHEDULABLE_TYPE::DEFAULT:	result = "default";			break;
	case	eSCHEDULABLE_TYPE::SYSTEM:	result = "system";			break;
	case	eSCHEDULABLE_TYPE::USER:	result = "user";			break;
	default:							result = "";				break;
	}

	return result;
}

template <>
std::wstring_view to_string<wchar_t>(eSCHEDULABLE_TYPE _executor_type) noexcept
{
	std::wstring_view result;

	switch (_executor_type)
	{
	case	eSCHEDULABLE_TYPE::DEFAULT:	result = L"default";		break;
	case	eSCHEDULABLE_TYPE::SYSTEM:	result = L"system";			break;
	case	eSCHEDULABLE_TYPE::USER:	result = L"user";			break;
	default:							result = L"";				break;
	}

	return result;
}


}