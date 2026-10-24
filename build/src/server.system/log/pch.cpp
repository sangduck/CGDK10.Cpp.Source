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
namespace CGDK
{
	// * CGDK Component Info
	CGDK_INFO_CLASS	g_info_server_log;
}