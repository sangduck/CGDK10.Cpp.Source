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

namespace CGDK
{
//-----------------------------------------------------------------------------
//
// definitions
//
//-----------------------------------------------------------------------------
enum class eSCHEDULABLE_TYPE : int32_t
{
	DEFAULT				 = 0,	// default
	SYSTEM				 = 1,	// system scheduler
	USER				 = 2,	// user scheduler
	MAX
};

[[nodiscard]] std::string_view to_string(eSCHEDULABLE_TYPE _character_set) noexcept;

//-----------------------------------------------------------------------------
// eSCHEDULE_OVERRUN
//  - policy when the next execution time has already passed
//    (the previous execution took longer than the interval, or the execution was delayed)
//
//  CATCH_UP : execute the missed executions immediately (the execution count is guaranteed)
//             ex) interval 100ms, execution delayed 350ms -> executed 3 times immediately, then 100ms interval
//  SKIP     : skip the missed executions and execute at the next interval (the interval grid is kept)
//             ex) interval 100ms, execution delayed 350ms -> next execution at 400ms
//-----------------------------------------------------------------------------
enum class eSCHEDULE_OVERRUN : int32_t
{
	CATCH_UP			 = 0,
	SKIP				 = 1,
	DEFAULT				 = CATCH_UP
};

[[nodiscard]] std::string_view to_string(eSCHEDULE_OVERRUN _overrun) noexcept;

namespace statistics
{
	struct schedulable
	{
		ENABLE_STRUCT_SERIALIZABLE

		uint64_t				id = 0;
		eSCHEDULABLE_TYPE		type = eSCHEDULABLE_TYPE::USER;
		std::atomic<uint64_t>	count_execute = 0;
	};
}

}