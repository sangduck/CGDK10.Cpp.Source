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
	CGDK_INFO_CLASS	g_info_server_service;
}
