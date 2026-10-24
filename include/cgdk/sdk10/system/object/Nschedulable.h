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
/**

 @class		Nschedulable
 @brief		
 @todo		
 @ref		
 @details	\n

*///-----------------------------------------------------------------------------
class Nschedulable :
		virtual public			Ischedulable
{
public:
	virtual	~Nschedulable() noexcept {}

// public)
public:
	[[nodiscard]] auto			execute_interval() const noexcept { return this->m_tick_diff_execute; }
			void				execute_interval(chrono::tick::duration _tick_interval);

	//! @brief policy when the next execution time has already passed (see eSCHEDULE_OVERRUN)
	[[nodiscard]] auto			overrun_policy() const noexcept { return this->m_overrun_policy; }
			void				overrun_policy(eSCHEDULE_OVERRUN _overrun_policy) noexcept { this->m_overrun_policy = _overrun_policy; }

// framework)
protected:
	//! @brief advance 'next tick' by the interval (applying the overrun policy)
			void				process_advance_next_tick() noexcept;

			chrono::tick::duration	m_tick_diff_execute = 1s;
			eSCHEDULE_OVERRUN	m_overrun_policy = eSCHEDULE_OVERRUN::DEFAULT;
};


}