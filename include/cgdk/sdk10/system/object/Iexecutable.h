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

 @class		Iexecutable
 

*///-----------------------------------------------------------------------------
class Iexecutable :
	public						EXECUTABLE_NODE,
	virtual public				Ireferenceable
{
public:
	virtual	~Iexecutable() noexcept {}

public:
	virtual	intptr_t			process_execute(intptr_t _result, size_t _param) PURE;

public:
	// position in the heap of executor::scheduler (SIZE_MAX : not pending)
	// - written under the lock of the executor::scheduler holding it. another scheduler may read it under its own lock (atomic, relaxed :
	//   a position read is checked against the heap before it is used)
	// - is_shared : it was posted while pending (twice, or to another scheduler) : the position may belong to another entry or
	//   scheduler, cancel() searches the heap when the position does not match (kept : the other entries are not known)
	// - not copied (a copy is not pending)
	class scheduler_position
	{
	public:
		scheduler_position() noexcept {}
		scheduler_position(const scheduler_position&) noexcept {}
		scheduler_position& operator=(const scheduler_position&) noexcept { return *this; }
	public:
		std::atomic<size_t>		index{ SIZE_MAX };
		std::atomic<bool>		is_shared{ false };
	};
			scheduler_position	m_scheduler_position;
};


}