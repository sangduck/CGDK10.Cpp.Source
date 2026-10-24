//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                          network socket classes                           *
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

	// 3) Target version
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

// 5) dll export
#if defined(_WIN32)
	#define __DLL_EXPORT __declspec(dllexport) 
#else
	#define __DLL_EXPORT
#endif


//----------------------------------------------------------------------------
// include
//----------------------------------------------------------------------------
// 1) Main Include
#include "cgdk/sdk10/net.socket.mail.h"


//----------------------------------------------------------------------------
// license info
//----------------------------------------------------------------------------
// 1) definitions
#define		CGDK_INFO_CLASS				CInfo_network_socket_mail
#define		CGDK_COMPONENT_NAME			"CGDK10.net.socket.mail"

// 2) svn version file
#include "svn_version.h"

// 3) license files
#include "../../common/version.h"
#include "../../common/cgdk_info.h"
