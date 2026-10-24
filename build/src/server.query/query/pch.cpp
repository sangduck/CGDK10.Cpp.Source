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
	// 1) statistics
	query::statistics::unit_query	__ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL3 + 0)	query::statistics::Nquerier::m_statistics_querier_total;
	int								g_statistics_db_time = 0;

	// 2) DBQuerier Manager
	lock_object						__ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL3 + 1)	query::manager_querier::m_lockable_create;
	own_ptr<query::manager_querier>	__ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL3 + 1)	query::manager_querier::m_pinstance;
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
	CGDK_INFO_CLASS g_info_ServerQueryClasses;
}
