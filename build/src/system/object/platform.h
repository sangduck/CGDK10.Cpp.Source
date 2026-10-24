//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                              object classes                               *
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

// 6) Thread (Linux or Mobile)
#if defined(__ANDROID__)
	#include <signal.h>
	#include <poll.h>
	#include <pthread.h>
	#include <sched.h>
	#include <unistd.h>

	#include <linux/input.h>
	#include <sys/epoll.h>

#elif defined(__linux__)
	#include <pthread.h>
	#include <unistd.h>
	//#include <linux/input.h>

	// - epoll
	#include <sys/epoll.h>

	//// - aio
	#include <signal.h>
	#include <aio.h>
	//#include <linux/aio_abi.h>

#elif defined (__APPLE__)
	#include <poll.h>
	#include <pthread.h>
	#include <sched.h>
	#include <unistd.h>

	#include <linux/input.h>
#endif


namespace CGDK
{
	//class CGDK_INFO_CLASS
	//{
	//public:
	//	CGDK_INFO_CLASS();
	//};

	namespace api
	{
	#if defined(__ANDROID__) || defined(__linux__)
		class io_context_Nepoll;
		class io_context_epoll_queued;
		class io_context_epoll_direct;
		class io_context_uring;
		class io_context_uring_cq;
		class io_context_linux;
		//using io_context = io_context_epoll_queued;
		//using io_context = io_context_epoll_direct;
		using io_context = io_context_linux;		// - epoll or io_uring (executor::default_io_backend())
	#elif defined(_IO_APISET_H_)
		class io_context_Niocp;
		class io_context_iocp_queued;
		class io_context_iocp_direct;
		//using io_context = io_context_iocp_queued;
		using io_context = io_context_iocp_direct;
	#else
	#error "Unsupported API"
	#endif
	}
}

#if defined(_WIN32) // defined(_IO_APISET_H_)
	#include "cgdk/sdk10/system/object/api.io_context.Niocp.h"
	#include "cgdk/sdk10/system/object/api.io_context.iocp_queued.h"
	#include "cgdk/sdk10/system/object/api.io_context.iocp_direct.h"
#elif defined(__linux__) // 
	#include "cgdk/sdk10/system/object/api.io_context.Nepoll.h"
	#include "cgdk/sdk10/system/object/api.io_context.epoll_queued.h"
	#include "cgdk/sdk10/system/object/api.io_context.epoll_direct.h"
	#include "cgdk/sdk10/system/object/api.io_context.uring_cq.h"
	#include "cgdk/sdk10/system/object/api.io_context.uring.h"
#endif

#include "cgdk/sdk10/system/object/executor.io_context.h"
#include "cgdk/sdk10/system/object/executor.io_context_threads.h"
