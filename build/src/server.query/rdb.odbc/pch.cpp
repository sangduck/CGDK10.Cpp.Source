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

bool CGDK::query::rdb::odbc::querier::m_s_enable_logging_warning_info = true;

#if defined(_MSC_VER)
	#pragma warning(default:4073)
#endif


//-----------------------------------------------------------------
// 2. CGDK Info
//-----------------------------------------------------------------
namespace CGDK
{
	// * CGDK Component Info
	CGDK_INFO_CLASS	g_info_server_query_odbc;
}