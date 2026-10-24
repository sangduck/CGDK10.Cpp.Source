//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                              Utility Classes                              *
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
// cpu feature (runtime detection)
//
//  - CGDK_SIMD_AVX2        : defined when AVX2 code can be compiled (x64)
//  - CGDK_TARGET_AVX2      : put on a function using AVX2 intrinsics
//                            (gcc/clang compile it for AVX2 without -mavx2)
//  - cpu::is_avx2_supported() : AVX2 instructions + OS saves YMM registers
//
//  * call an AVX2 function only after checking cpu::is_avx2_supported()
//-----------------------------------------------------------------------------
#if defined(_M_X64) || defined(__x86_64__)
	#define CGDK_SIMD_AVX2
	#include <immintrin.h>
	#if defined(_MSC_VER)
		#include <intrin.h>
	#endif
	#if defined(__GNUC__) || defined(__clang__)
		#define CGDK_TARGET_AVX2	__attribute__((target("avx2")))
	#else
		#define CGDK_TARGET_AVX2
	#endif
#endif

namespace CGDK
{
namespace cpu
{
#if defined(CGDK_SIMD_AVX2)
	[[nodiscard]] inline bool is_avx2_supported() noexcept
	{
		static const bool s_supported = []() noexcept
		{
		#if defined(_MSC_VER)
			int info[4]{};
			__cpuid(info, 0);
			if (info[0] < 7)
				return false;

			// - OSXSAVE(27), AVX(28)
			__cpuid(info, 1);
			if ((info[2] & (1 << 27)) == 0 || (info[2] & (1 << 28)) == 0)
				return false;

			// - XMM/YMM state enabled by OS
			if ((_xgetbv(0) & 0x6) != 0x6)
				return false;

			// - AVX2 (leaf 7, ebx bit 5)
			__cpuidex(info, 7, 0);
			return (info[1] & (1 << 5)) != 0;
		#else
			__builtin_cpu_init();
			return __builtin_cpu_supports("avx2") != 0;
		#endif
		}();
		return s_supported;
	}
#else
	[[nodiscard]] constexpr bool is_avx2_supported() noexcept { return false; }
#endif
}
}
