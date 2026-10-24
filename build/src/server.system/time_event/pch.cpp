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
	// 1) event manager
	lock_object					time_event::entity_manager::m_lockable_create;
	own_ptr<time_event::entity_manager> time_event::entity_manager::m_pinstance;
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
	CGDK_INFO_CLASS	g_info_server_time_event;
}