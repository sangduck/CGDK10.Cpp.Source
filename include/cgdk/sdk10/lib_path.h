//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                         library path (windows)                            *
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
//-----------------------------------------------------------------------------
// CGDK_LIB_DIR_WINDOWS
//
//  - the libraries are built into 'lib/cgdk/sdk10/windows/<platform toolset>/'
//    (vcxproj: <OutDir>.../lib/cgdk/sdk10/windows/$(PlatformToolset)/</OutDir>)
//  - '#pragma comment(lib, CGDK_LIB_DIR_WINDOWS "CGDK10.xxx.lib")' links the library
//    built by the same toolset (v143(vs2022) and v145(vs2026) libraries are not mixed).
//  - the library directory of the application is 'lib' (as before).
//-----------------------------------------------------------------------------
#if defined(_MSC_VER) && !defined(CGDK_LIB_DIR_WINDOWS)
	#if _MSC_VER >= 1950
		#define	CGDK_MSVC_TOOLSET_NAME	"v145"
	#elif _MSC_VER >= 1930
		#define	CGDK_MSVC_TOOLSET_NAME	"v143"
	#elif _MSC_VER >= 1920
		#define	CGDK_MSVC_TOOLSET_NAME	"v142"
	#else
		#define	CGDK_MSVC_TOOLSET_NAME	"v141"
	#endif

	#define	CGDK_LIB_DIR_WINDOWS	"cgdk/sdk10/windows/" CGDK_MSVC_TOOLSET_NAME "/"
#endif
