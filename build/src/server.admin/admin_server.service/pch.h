//*****************************************************************************
//*                                                                           *
//*                      Cho SangHyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                          Server Admin Host Agent                          *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
//*  Best for Game Developement and Optimized for Game Developement.          *
//*                                                                           *
//*                (c) 2003. Cho Sanghyun. All right reserved.                *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************

#pragma once
//----------------------------------------------------------------------------
// Definitions
//----------------------------------------------------------------------------
// 1) For gcc
#if (__GNUC__ < 11) || (__GNUC__==11 && __GNUC_MINOR__ < 2)
	#undef __STRICT_ANSI__
#endif

#if defined(_WIN32)
	#pragma message("[CGDK10] WINDOWS Platform")

	// 2) Prefix
	#include "cgdk/sdk10/prefix.h"

	// 3) Target Version
	#include <SDKDDKVer.h>

	// 4) for UWP
	#if defined(WINAPI_FAMILY)
		// - No GDI
		#define NOGDI

		// - WIN32 LEAN_AND_MEAN
		#ifndef WIN32_LEAN_AND_MEAN
			#define WIN32_LEAN_AND_MEAN
		#endif

		// - Windows header file
		#include <windows.h>
	#endif
#elif defined(__ANDROID__)
	//#pragma message ("[CGDK10] NDK Platform")

#elif defined(__APPLE__)
	//#pragma message ("[CGDK10] IOS Platform")

#elif defined(__linux__)
	//#pragma message ("[CGDK10] LINUX Platform")

#else
	#error "CGDK Not supported platform"
#endif

// Debug) 
#ifdef _USE_DEBUG_NEW_REDEFINITION
	#define new CGNEW_DEBUG
#endif

// 5) dll export
#if defined(_WIN32)
	#define __DLL_EXPORT __declspec(dllexport) 
#else
	#define __DLL_EXPORT
#endif


//----------------------------------------------------------------------------
// Definitions
//----------------------------------------------------------------------------
class socket_server;
class socket_host_agent;
class server_entity;

class Iobservable_websocket;
//class observable_websocket_server_overview_info;
//class observable_websocket_server_factory_info;
//class observable_websocket_server_executor_info;
//class observable_websocket_server_network_info;
//class observable_websocket_server_querier_info;
//class observable_websocket_server_crash_info;

//----------------------------------------------------------------------------
// Include
//----------------------------------------------------------------------------
// 1) CGNetServer Header File
#include "cgdk/sdk10/server.system.service.h"
#ifdef _USE_SVN_CONTROL
#include "cgdk/sdk10/server.admin.version_control.h"
#endif
#include "cgdk/sdk10/server.system.log.h"
#include "cgdk/sdk10/server.system.group.h"
#include "cgdk/sdk10/net.socket.http.h"

// using
using namespace CGDK;


//----------------------------------------------------------------------------
// license info
//----------------------------------------------------------------------------
// 1) svn version file
#if defined(_MSC_VER)
	#include "svn_version.h"
#else
	#ifndef _SVN_VERSION_H_
	#define _SVN_VERSION_H_

	#define SVN_LOCAL_MODIFICATIONS       0						// 1 if there are modifications to the local working copy, 0 otherwise
	#define SVN_COMMIT_REVISION           2310					// Highest committed revision number in the working copy
	#define SVN_COMMIT_REVISION_STRING    "2310"				// Highest committed revision number in the working copy
	#define SVN_UPDATE_REVISION_STRING    "2310"				// Highest committed revision number in the working copy
	#define SVN_TIME_NOW_STRING           "2018/07/13 08:54:17"	// Current system date &amp; time

	#endif
#endif

// 2) license files
#include "../../common/version.h"


//----------------------------------------------------------------------------
// library link
//----------------------------------------------------------------------------
// 4) lib files
#if defined(_WIN32)
	#pragma comment (lib,"Userenv.lib")
	#pragma comment (lib,"Wtsapi32.lib")
	//#pragma comment (lib,"msdasc.lib")
#endif


//----------------------------------------------------------------------------
// ...
//----------------------------------------------------------------------------
// 1) definitions
#include "definition_server.h"

// 2) for web
#include "socket_web.h"
#include "socket_websocket.h"
#include "group_websocket_immigration.h"

#include "Iobservable_websocket.h"
//#include "observable_websocket_system_info.h"
//#include "observable_websocket_host_info.h"
//#include "observable_websocket_server_overview_info.h"
//#include "observable_websocket_server_factory_info.h"
//#include "observable_websocket_server_executor_info.h"
//#include "observable_websocket_server_network_info.h"
//#include "observable_websocket_server_querier_info.h"
//#include "observable_websocket_server_crash_info.h"
//#include "observable_websocket_server_info.h"
#include "group_websocket.h"

// 3) server entity
#include "server_entity.h"

// 4) for control
#include "Isocket_host_agent.h"
#include "socket_host_agent.h"
#include "group_host_agent.h"
#include "group_host_agent_immigration.h"

// 5) for server
#include "socket_server.h"
#include "group_server.h"
#include "group_server_immigration.h"

// 6) service
#include "service.h"
