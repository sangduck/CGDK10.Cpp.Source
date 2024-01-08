#include "pch.h"
#include <iostream>


//-----------------------------------------------------------------
// 1. Init_seg(lib)
//-----------------------------------------------------------------
// Static) 
#if defined(_MSC_VER)
	#pragma warning(disable:4073)
	#pragma init_seg(lib)
#endif

#if defined(_MSC_VER)
	#pragma warning(default:4073)
#endif

//-----------------------------------------------------------------
// 2. CGDK Info
//-----------------------------------------------------------------
#define	_FREE_RELEASE_MAX_ACCEPT_SOCKET		128
#define	_INDIE_RELEASE_MAX_ACCEPT_SOCKET	1024

namespace CGDK
{
	// * CGDK Component Info
	CGDK_INFO_CLASS	g_info_net_socket_http;

	void CGDK_INFO_CLASS::process_license_result(const sCOMPONENT_LICENSE& _info)
	{
		// 1) get now time
		auto time_now = chrono::time::utc::now();

		// 2) ...
		switch (_info.license_type)
		{
		case	CGDK::eLICENSE_TYPE::FREE:
				{
					// - 한번만 실행되도록 한다.
					if (get_license_alert())
					{
					#if defined(WINAPI_FAMILY)
					#else
					#endif
					}
				}
				break;

		case	CGDK::eLICENSE_TYPE::INDIE:
				{
				}
				break;

		case	CGDK::eLICENSE_TYPE::LIMITED_PROJECT:
				if (time_now > _info.component_info.time_expire)
				{
					// - Aleart는 1번만 하도록 한다.
					if (get_license_alert())
					{
					#if defined(WINAPI_FAMILY)
						MessageBox(nullptr, TEXT("CGCII license expired (update license or contact us[sangduck@cgcii.co.kr])"), TEXT("Error"), MB_OK);
					#else
						std::cout << "CGCII license expired (update license or contact us[sangduck@cgcii.co.kr])" << '\n';
					#endif
					}

					// - License기간이 Margin을 초과했을 경우 강제로 종료시킨다.
					if (time_now > (_info.component_info.time_expire + chrono::days(7)))
					{
						//ExitProcess(0);
						std::abort();
					}
				}
				break;

		case	CGDK::eLICENSE_TYPE::LIMITED_COMPPANY:
				if (time_now > _info.component_info.time_expire)
				{
					// - Aleart는 1번만 하도록 한다.
					if (get_license_alert())
					{
					#if defined(WINAPI_FAMILY)
						MessageBox(nullptr, TEXT("CGCII license expired (update license or contact us[sangduck@cgcii.co.kr])"), TEXT("Error"), MB_OK);
					#else
						std::cout << "CGCII license expired (update license or contact us[sangduck@cgcii.co.kr])" << '\n';
					#endif
					}

					// - License기간이 Margin을 초과했을 경우 강제로 종료시킨다.
					if (time_now > (_info.component_info.time_expire + chrono::days(7)))
					{
						//ExitProcess(0);
						std::abort();
					}
				}
				break;

		case	CGDK::eLICENSE_TYPE::UNLIMITED:
				break;

		case	CGDK::eLICENSE_TYPE::DEVELOPER:
				break;

		default:
				std::abort();
				break;
		}
	}
}