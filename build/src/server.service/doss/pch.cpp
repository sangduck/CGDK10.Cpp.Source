#include "pch.h"
#include <iostream>


//-----------------------------------------------------------------
// 1. Init_seg(lib)
//-----------------------------------------------------------------
// static) 
#if defined(_MSC_VER)
	#pragma warning(disable:4073)
	#pragma init_seg(lib)
#endif

namespace CGDK
{
	// 1) DBRepository Manager
	lock_object							__ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL4 + 0)	doss::manager_repository::m_lockable_create;
	own_ptr<doss::manager_repository>	__ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL4 + 0)	doss::manager_repository::m_pinstance;
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
	CGDK_INFO_CLASS	g_info_server_doss;
}
