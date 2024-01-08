#include "pch.h"

using namespace CGDK;

namespace CGDK
{

template<> std::string_view to_string<char>(eSOCKET_STATE _state) noexcept
{
	// declare)
	std::string_view result;

	switch(_state)
	{
	case	eSOCKET_STATE::CLOSED:
			result = "CLOSED"sv;
			break;

	case	eSOCKET_STATE::BINDED:
			result  = "BINDED"sv;
			break;

	case	eSOCKET_STATE::SYN:
			result = "SYN"sv;
			break;

	case	eSOCKET_STATE::SEND_DISCONNECTED:
			result = "SND_DIS"sv;
			break;

	case	eSOCKET_STATE::FIN_WAIT_1:
			result = "FIN_WAIT_1"sv;
			break;

	case	eSOCKET_STATE::FIN_WAIT_2:
			result = "FIN_WAIT_2"sv;
			break;

	case	eSOCKET_STATE::TIME_WAIT:
			result = "TIME_WAIT"sv;
			break;

	case	eSOCKET_STATE::CLOSE_WAIT:
			result = "CLOSE_WAIT"sv;
			break;

	case	eSOCKET_STATE::LAST_ACK:
			result = "LAST_ACK"sv;
			break;

	case	eSOCKET_STATE::ESTABLISHED:
			result = "ESTABLISHED"sv;
			break;

	case	eSOCKET_STATE::LISTEN:
			result = "LISTEN"sv;
			break;

	case	eSOCKET_STATE::CERTIFIED:
			result = "CERTIFIED"sv;
			break;

	case	eSOCKET_STATE::LOGINED:
			result = "LOGINED"sv;
			break;
	default:
			result = ""sv;
			break;
	};

	// return)
	return result;
}

template<> std::wstring_view to_string<wchar_t>(eSOCKET_STATE _state) noexcept
{
	// declare)
	std::wstring_view result;

	switch(_state)
	{
	case	eSOCKET_STATE::CLOSED:
			result = L"CLOSED"sv;
			break;

	case	eSOCKET_STATE::BINDED:
			result  = L"BINDED"sv;
			break;

	case	eSOCKET_STATE::SYN:
			result = L"SYN"sv;
			break;

	case	eSOCKET_STATE::SEND_DISCONNECTED:
			result = L"SND_DIS"sv;
			break;

	case	eSOCKET_STATE::FIN_WAIT_1:
			result = L"FIN_WAIT_1"sv;
			break;

	case	eSOCKET_STATE::FIN_WAIT_2:
			result = L"FIN_WAIT_2"sv;
			break;

	case	eSOCKET_STATE::TIME_WAIT:
			result = L"TIME_WAIT"sv;
			break;

	case	eSOCKET_STATE::CLOSE_WAIT:
			result = L"CLOSE_WAIT"sv;
			break;

	case	eSOCKET_STATE::LAST_ACK:
			result = L"LAST_ACK"sv;
			break;

	case	eSOCKET_STATE::ESTABLISHED:
			result = L"ESTABLISHED"sv;
			break;

	case	eSOCKET_STATE::LISTEN:
			result = L"LISTEN"sv;
			break;

	case	eSOCKET_STATE::CERTIFIED:
			result = L"CERTIFIED"sv;
			break;

	case	eSOCKET_STATE::LOGINED:
			result = L"LOGINED"sv;
			break;
	default:
			result = L""sv;
			break;
	};

	// return)
	return result;
}


}