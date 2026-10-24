//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                              memory Classes                               *
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
// CGCII ASSERT definitions
//-----------------------------------------------------------------------------
// 1) assert
#include <cassert>


// ----------------------------------------------------------------------------
//
// Assert
//
// ----------------------------------------------------------------------------
#ifndef NDEBUG
	#define	_USE_CGASSERT
#endif

// 1) ASSERT
#ifdef _USE_CGASSERT
	#if defined(_WIN32)
		#define CGDK_DEBUG_BREAK()	do { if (::IsDebuggerPresent()) __debugbreak(); } while (0)
	#else
		#include <csignal>
		#include <cstdlib>
		#include <fstream>
		#include <string>

		inline bool cgdk_is_debugger_present() noexcept
		{
			std::ifstream status("/proc/self/status");
			for (std::string line; std::getline(status, line); )
				if (line.rfind("TracerPid:", 0) == 0)
					return std::atoi(line.c_str() + 10) != 0;
			return false;
		}

		#define CGDK_DEBUG_BREAK()	do { if (cgdk_is_debugger_present()) ::raise(SIGTRAP); } while (0)
	#endif

	#define CGASSERT(equation, value)              if(!(equation)) { CGDK_DEBUG_BREAK(); return value; }
	#define CGASSERT_TRACE(equation, value, state) if(!(equation)) { state; CGDK_DEBUG_BREAK(); return value; }
	#define CGASSERT_ERROR(equation)               if(!(equation)) { CGDK_DEBUG_BREAK(); }
	#define CGASSERT_FATAL(equation)			   assert(equation);
	#define CGASSERT_ERROR_TRACE(equation, state)  if(!(equation)) { state; CGDK_DEBUG_BREAK(); }
	#define CGASSERT_ERROR_CRITICAL                CGDK_DEBUG_BREAK();
#else
	#define	CGASSERT(equation, value)				if(!(equation))	{ return value;}
	#define	CGASSERT_TRACE(equation, value, state)	if(!(equation))	{ state; return value;}
	#define	CGASSERT_ERROR(equation)
	#define CGASSERT_FATAL(equation)
	#define	CGASSERT_ERROR_TRACE(equation, state)	if(!(equation))	{ state;}
	#define	CGASSERT_ERROR_CRITICAL
#endif
