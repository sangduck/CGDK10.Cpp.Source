//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                        network socket kafka classes                       *
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
#define	CGDK_NET_SOCKET_KAFKA


//----------------------------------------------------------------------------
// prototypes
//----------------------------------------------------------------------------
namespace CGDK
{
	namespace net
	{
		namespace io
		{
			namespace kafka
			{
				// interfaces
				class Iproducible;
				class Iproducer;
				class Iconsumable;
				class Imessageable;

				// components
				class Nclient;
				class Nproducer;
				class Nconsumer;
			}
		}

		namespace kafka
		{
			class producer;
			class consumer;
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
#include <shared_mutex>
#include <stop_token>

// 2) librdkafka (vcpkg 'librdkafka' on windows, 'librdkafka-dev' on linux)
#if defined(_MSC_VER)
	#pragma warning(push)
	#pragma warning(disable: 4100 4127 4251 4996)
#endif
#include <librdkafka/rdkafkacpp.h>
#if defined(_MSC_VER)
	#pragma warning(pop)
#endif

// 3) definitions
#include "cgdk/sdk10/net.socket/socket.kafka/net.definitions.kafka.h"


//----------------------------------------------------------------------------
// Component Classes
//----------------------------------------------------------------------------
// 1) interfaces
#include "cgdk/sdk10/net.socket/socket.kafka/net.io.kafka.Iproducible.h"
#include "cgdk/sdk10/net.socket/socket.kafka/net.io.kafka.Iproducer.h"
#include "cgdk/sdk10/net.socket/socket.kafka/net.io.kafka.Iconsumable.h"
#include "cgdk/sdk10/net.socket/socket.kafka/net.io.kafka.Imessageable.h"

// 2) components
#include "cgdk/sdk10/net.socket/socket.kafka/net.io.kafka.Nclient.h"
#include "cgdk/sdk10/net.socket/socket.kafka/net.io.kafka.Nproducer.h"
#include "cgdk/sdk10/net.socket/socket.kafka/net.io.kafka.Nconsumer.h"


//----------------------------------------------------------------------------
// templates
//----------------------------------------------------------------------------
// 1) producer & consumer
#include "cgdk/sdk10/net.socket/socket.kafka/net.kafka.producer.h"
#include "cgdk/sdk10/net.socket/socket.kafka/net.kafka.consumer.h"


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

	// 1) librdkafka from vcpkg ('CGDK10.Cpp/vcpkg.json'. the path is relative to the CGDK lib directory 'CGDK10.Cpp/lib')
	//    the dlls (rdkafka.dll, rdkafka++.dll, ...) are in 'CGDK10.Cpp/vcpkg_installed/x64-windows/bin' ('debug/bin' for debug)
	#ifdef NDEBUG
		#pragma comment(lib, "../vcpkg_installed/x64-windows/lib/rdkafka++.lib")
		#pragma comment(lib, "../vcpkg_installed/x64-windows/lib/rdkafka.lib")
	#else
		#pragma comment(lib, "../vcpkg_installed/x64-windows/debug/lib/rdkafka++.lib")
		#pragma comment(lib, "../vcpkg_installed/x64-windows/debug/lib/rdkafka.lib")
	#endif


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
				#pragma comment(lib, CGDK_LIB_DIR_WINDOWS "CGDK10.net.socket.kafka_Windows10.0_x64_Release.lib")
			#else
				#pragma comment(lib, CGDK_LIB_DIR_WINDOWS "CGDK10.net.socket.kafka_Windows10.0_x64_Debug.lib")
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
	// link 'rdkafka++' and 'rdkafka'
#else
	#error "[CGDK10] Not supported Platform"
#endif
