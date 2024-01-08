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

namespace CGDK
{
	lockable<>								__ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL4 + 1)	api::manager_perfornamce_query::m_lockable_create;
	own_ptr<api::manager_perfornamce_query>	__ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL4 + 2)	api::manager_perfornamce_query::m_pinstance;

	lockable<>								__ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL4 + 3)	g_lockable_pservice_global;
	object_ptr<server_system::Nservice>		__ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL4 + 5)	g_pservice_global;
}

#if defined(_MSC_VER)
	#pragma warning(default:4073)
#endif


//-----------------------------------------------------------------
// 2. CGDK Info
//-----------------------------------------------------------------
namespace CGDK
{
	// * CGDK Component Info
	CGDK_INFO_CLASS	g_info_server_system_service;

	void CGDK_INFO_CLASS::process_license_result(const sCOMPONENT_LICENSE& _info)
	{
		// 1) get now time
		auto time_now = chrono::time::utc::now();

		// 2) ...
		switch (_info.license_type)
		{
		case	CGDK::eLICENSE_TYPE::FREE:
				{
					if (get_license_alert())
					{
					#if defined(WINAPI_FAMILY)
					#else
					#endif
					}
				}
				break;

		case	CGDK::eLICENSE_TYPE::INDIE:
				break;

		case	CGDK::eLICENSE_TYPE::LIMITED_PROJECT:
				if (time_now > _info.component_info.time_expire)
				{
					if (get_license_alert())
					{
						// - Message Box
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
