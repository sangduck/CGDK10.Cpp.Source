#include "pch.h"

namespace CGDK
{

std::string_view to_string(eSCHEDULABLE_TYPE _executor_type) noexcept
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

std::string_view to_string(eSCHEDULE_OVERRUN _overrun) noexcept
{
	std::string_view result;

	switch (_overrun)
	{
	case	eSCHEDULE_OVERRUN::CATCH_UP:	result = "catch_up";		break;
	case	eSCHEDULE_OVERRUN::SKIP:		result = "skip";			break;
	default:								result = "";				break;
	}

	return result;
}


}