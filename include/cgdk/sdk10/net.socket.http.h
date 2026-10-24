//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                        network socket http classes                        *
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
			namespace connect_requestable
			{
				class Nhttp;
			}

			namespace sender
			{
				class Nhttp;
			}

			class Nhttp_server;
			class Nhttp_client;
		}

		namespace socket
		{
			template <class, class> class tcp_client_crypt;
			template <class, class> class tcp_server_crypt;

			class http_client;
			class http_server;
		}

		class Ihttp_request;
		class http_request;
	}
}


//----------------------------------------------------------------------------
//
// head files
//
//----------------------------------------------------------------------------
// 1) object system
#include "cgdk/sdk10/net.socket.h"
#include "cgdk/sdk10/net.socket.crypt.h"

// 2) definitions
#include "cgdk/sdk10/net.socket/socket.http/net.definitions.http.h"

// 3) http
#include "cgdk/sdk10/net.socket/socket.http/MSG_HTTP.h"
#include "cgdk/sdk10/net.socket/socket.http/MSG_HTTP_REQUEST.h"
#include "cgdk/sdk10/net.socket/socket.http/MSG_HTTP_RESPONSE.h"
#include "cgdk/sdk10/net.socket/socket.http/MSG_HTTP_WEBSOCKET.h"
#include "cgdk/sdk10/net.socket/socket.http/WEBSOCKET_MESSAGE.h"
#include "cgdk/sdk10/net.socket/socket.http/net.Ihttp_request.h"
#include "cgdk/sdk10/net.socket/socket.http/net.http_request.h"


//----------------------------------------------------------------------------
// Component Classes
//----------------------------------------------------------------------------
// 1) connect_requestable
#include "cgdk/sdk10/net.socket/socket.http/net.io.connect_requestable.http.h"

// 2) sender
#include "cgdk/sdk10/net.socket/socket.http/net.io.sender.http.h"

// 3) http
#include "cgdk/sdk10/net.socket/socket.http/net.io.Nhttp_server.h"
#include "cgdk/sdk10/net.socket/socket.http/net.io.Nhttp_client.h"


//----------------------------------------------------------------------------
// templates
//----------------------------------------------------------------------------
// 1) acceptor & connector
#include "cgdk/sdk10/net.socket/socket.http/net.acceptor.http.h"
#include "cgdk/sdk10/net.socket/socket.http/net.connector.http.h"

// 2) socket - http server/ http client
#include "cgdk/sdk10/net.socket/socket.http/net.socket.http_client.h"
#include "cgdk/sdk10/net.socket/socket.http/net.socket.http_server.h"


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
				#pragma comment(lib, CGDK_LIB_DIR_WINDOWS "CGDK10.net.socket.http_Windows10.0_x64_Release.lib")
			#else
				#pragma comment(lib, CGDK_LIB_DIR_WINDOWS "CGDK10.net.socket.http_Windows10.0_x64_Debug.lib")
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

