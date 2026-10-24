//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                       network socket crypt classes                        *
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
#define	CGDK_NET_SOCKET_HTTP


//----------------------------------------------------------------------------
// prototypes
//----------------------------------------------------------------------------
namespace CGDK
{
	namespace net
	{
		namespace io
		{
			namespace socket
			{
				class Nsocket_ssl;
			}

			namespace receivable
			{
				class Nstream_crypt;
			}

			namespace sendable
			{
				class Ntcp_crypt;
				class Ntcp_crypt_async_gather_buffered;
			}

			namespace connect_requestable
			{
				class Ntcp_ssl;
				class Ntcp_reconnect_ssl;
			}

			namespace sender
			{
			}

			class Nsocket_ssl;
			class Ntcp_crypt;
		}

		namespace socket
		{
			template <class, class> class tcp_crypt;
			template <class, class> class tcp_ssl;
		}
	}
}


//----------------------------------------------------------------------------
//
// head files
//
//----------------------------------------------------------------------------
// 1) object system
#include "cgdk/sdk10/net.socket.h"
#include "cgdk/sdk10/system.util.crypt.h"

// 2) definitions
#include "cgdk/sdk10/net.socket/socket.crypt/net.definitions.crypt.h"


//----------------------------------------------------------------------------
// Component Classes
//----------------------------------------------------------------------------
// 1) connect_requestable
#include "cgdk/sdk10/net.socket/socket.crypt/net.io.connect_requestable.tcp_ssl.h"
#include "cgdk/sdk10/net.socket/socket.crypt/net.io.connect_requestable.tcp_reconnect_ssl.h"

// 2) acceptor
#include "cgdk/sdk10/net.socket/socket.crypt/net.io.connective.acceptor_ssl.h"

// 3) receivable
#include "cgdk/sdk10/net.socket/socket.crypt/net.io.receivable.stream_crypt.h"

// 4) sendable
#include "cgdk/sdk10/net.socket/socket.crypt/net.io.sendable.tcp_crypt.h"
#include "cgdk/sdk10/net.socket/socket.crypt/net.io.sendable.tcp_crypt.async_gather_buffered.h"

// 5) compositions
#include "cgdk/sdk10/net.socket/socket.crypt/net.io.socket_ssl.h"
#include "cgdk/sdk10/net.socket/socket.crypt/net.io.tcp_crypt.h"


//----------------------------------------------------------------------------
// templates
//----------------------------------------------------------------------------
// 1) socket - tcp
#include "cgdk/sdk10/net.socket/socket.crypt/net.socket.tcp_crypt.h"
#include "cgdk/sdk10/net.socket/socket.crypt/net.socket.tcp_ssl.h"



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

	// 1) Default socket 
	#pragma comment(lib,"ws2_32.lib")


	//----------------------------------------------------------------------------
	// * WinSDK 10
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
				#pragma comment(lib, CGDK_LIB_DIR_WINDOWS "CGDK10.net.socket.crypt_Windows10.0_x64_Release.lib")
			#else
				#pragma comment(lib, CGDK_LIB_DIR_WINDOWS "CGDK10.net.socket.crypt_Windows10.0_x64_Debug.lib")
			#endif
		// Error - No supported
		#else
			#error "[CGDK10] Not supported Windows SDK version (Need Windows SDK 10.0 or higher)"
		#endif

	#else
		#error "[CGDK10] Not supported platform toolset (require visual studio 2019 or higher)"
	#endif

	#endif

#elif defined(__ANDROID__)
#elif defined(__APPLE__)
#elif defined(__linux__)
#else
	#error "[CGDK10] Not supported Platform"
#endif

