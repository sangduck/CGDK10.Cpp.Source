//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                          Utinity.Crypto Classes                           *
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

#include "cgdk/sdk10/lib_path.h"
//----------------------------------------------------------------------------
//
// definitions
//
//----------------------------------------------------------------------------
// 1) 
#define	CGDK_SYSTEM_UTIL_CRYPT


//----------------------------------------------------------------------------
// prototypes
//----------------------------------------------------------------------------
namespace CGDK
{
	namespace crypt
	{
		class ssl_manager;
		class ssl_context;
		class ssl;
		class ssl2;
		class ssl3;
	}

	class Issl_cryptable;
}

using SSL_METHOD = struct ssl_method_st;
using SSL = struct ssl_st;
using SSL_CTX = struct ssl_ctx_st;
using BIO = struct bio_st;


//----------------------------------------------------------------------------
//
// head files
//
//----------------------------------------------------------------------------
// 1) CGDK base header files
#include "cgdk/sdk10/system.object.h"

// 2) crypto
#include "cgdk/sdk10/system/util.crypt/util.crypt.pn_code.h"
#include "cgdk/sdk10/system/util.crypt/util.crypt.rsa.h"
#include "cgdk/sdk10/system/util.crypt/util.crypt.bcrypt.h"
#include "cgdk/sdk10/system/util.crypt/util.crypt.sha.h"
#include "cgdk/sdk10/system/util.crypt/util.crypt.sha_buffer_serializable.h"
#include "cgdk/sdk10/system/util.crypt/util.crypt.aes.h"
#include "cgdk/sdk10/system/util.crypt/util.crypt.hmac.h"

// 3) open ssl
#include "cgdk/sdk10/system/util.crypt/util.crypt.ssl.context.h"
#include "cgdk/sdk10/system/util.crypt/util.crypt.ssl.h"
#include "cgdk/sdk10/system/util.crypt/util.crypt.ssl.manager.h"


//----------------------------------------------------------------------------
//
// libraries
//
//----------------------------------------------------------------------------
#if defined(_MSC_VER)

	#if !defined(_MSC_PLATFORM_TOOLSET)
		#if _MSC_VER>=_MSC_VER_2022
			#define	_MSC_PLATFORM_TOOLSET	143
		#elif _MSC_VER>=_MSC_VER_2019
			#define	_MSC_PLATFORM_TOOLSET	142
		#elif _MSC_VER>=_MSC_VER_2017
			#define	_MSC_PLATFORM_TOOLSET	141
		#elif _MSC_VER>=_MSC_VER_2015
			#define	_MSC_PLATFORM_TOOLSET	140
		#elif _MSC_VER>=_MSC_VER_2013
			#define	_MSC_PLATFORM_TOOLSET	120
		#elif _MSC_VER>=_MSC_VER_2012
			#define	_MSC_PLATFORM_TOOLSET	110
		#elif _MSC_VER>=_MSC_VER_2010
			#define	_MSC_PLATFORM_TOOLSET	100
		#elif _MSC_VER>=_MSC_VER_2008
			#define	_MSC_PLATFORM_TOOLSET	90
		#else
			#define	_MSC_PLATFORM_TOOLSET	80
		#endif
	#endif

	#if !defined(_LIB) || defined(_CONSOLE)

	// - OpenSSL from vcpkg ('CGDK10.Cpp/vcpkg.json'. the path is relative to the CGDK lib directory 'CGDK10.Cpp/lib')
	//   the dlls (libcrypto-3-x64.dll, libssl-3-x64.dll) are in 'CGDK10.Cpp/vcpkg_installed/x64-windows/bin'
	#pragma comment(lib, "../vcpkg_installed/x64-windows/lib/libcrypto.lib")
	#pragma comment(lib, "../vcpkg_installed/x64-windows/lib/libssl.lib")

	//----------------------------------------------------------------------------
	// * Windows
	//----------------------------------------------------------------------------
	#if defined(WDK_NTDDI_VERSION)

		// check) 
		#if !defined(_M_X64)
			#error "[CGDK10] No WIN32-x86. Please use x64"
		#endif

		// check) 
		#ifndef _DLL 
			#error "[CGDK10] No Multi-Thread(/MT) or Multi-Thread Debug(/MTd). use Multi-thread DLL(/MD) or Multi-thread DLL Debug(/MDd)"
		#endif

		// WinSDK 10
		#if WDK_NTDDI_VERSION >= NTDDI_WIN10
			#ifdef NDEBUG
				#pragma comment(lib, CGDK_LIB_DIR_WINDOWS "CGDK10.system.util.crypt_Windows10.0_x64_Release.lib")
			#else
				#pragma comment(lib, CGDK_LIB_DIR_WINDOWS "CGDK10.system.util.crypt_Windows10.0_x64_Debug.lib")
			#endif
		// Error - No supported
		#else
			#error "[CGDK10] Not supported Windows SDK version (Need Windows SDK 10.0 or higher)"
		#endif
	#endif

	#endif

#elif defined(__ANDROID__)
#elif defined(__APPLE__)
#elif defined(__linux__)
#else
		#error "[CGDK10] Not supported platform"
#endif

