//*****************************************************************************
//*                                                                           *
//*                      Cho SangHyun's Game Classes II                       *
//*                       Ver 9.0 / release 2017.01.03                        *
//*                                                                           *
//*                           Server Event Classes                            *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
//*  Best for Game Developement and Optimized for Game Developement.          *
//*                                                                           *
//*                 ¨Ï 2016 Cho Sanghyun All right reserved.                  *
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
#ifdef _USE_DEBUG_MAKE_OBJECT
	#define new CGMAKE_OBJECT_DEBUG
#endif


// ----------------------------------------------------------------------------
// Include
// ----------------------------------------------------------------------------
// 1) Main Include
#include "cgdk/sdk10/server.service.doss.h"

// definitions) 
#define		CGDK_INFO_CLASS				CInfo_server_doss
#define		CGDK_COMPONENT_NAME			"CGDK10.server.doss"

// 2) svn version file
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

// 3) license files
#include "../../common/version.h"
#include "../../license/license_info/license_info.h"
