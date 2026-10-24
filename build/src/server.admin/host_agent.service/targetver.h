#pragma once

// Including SDKDDKVer.h defines the highest available Windows platform.

// If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
// set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.

#if _MSC_VER>=1600
	#include <SDKDDKVer.h>
#else
	#ifndef _WIN32_WINNT
		#define _WIN32_WINNT 0x0603
	#endif
#endif
