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

#if defined(__ANDROID__) || defined(__linux__)

namespace CGDK
{
//-----------------------------------------------------------------------------
/**

 @class		io_context_epoll_direct
 

*///-----------------------------------------------------------------------------
class api::io_context_epoll_direct : public io_context_Nepoll
{
// constructor/destructor)
public:
			io_context_epoll_direct();
	virtual	~io_context_epoll_direct() noexcept;

public:
			bool				create(int _hint_size = 4096) noexcept;
			CGHANDLE			dispose() noexcept;
	virtual	result_code			destroy() noexcept override;

			bool				post_executable(uint32_t _bytes_transferred, uintptr_t _completion_key, Iexecutable* _pexecutable) noexcept;
			bool				post_executable(uint32_t _bytes_transferred, uintptr_t _completion_key, object_ptr<Iexecutable>&& _executable) noexcept;

			void				pop_executable(RESULT& _result_event, chrono::tick::duration _ticks_wait) noexcept;
			[[nodiscard]] size_t queued_count() const noexcept;

	// - batch : one epoll_wait() returns up to MAX_EVENT_BATCH events.
	//   pop_events() fills _events and returns the count (0 : timeout or failure)
	//   get_result() finds the executable of an event (add_ref, invalid if the handle was closed)
	//   every popped event must be passed to get_result() and executed (oneshot events are not reported again)
	static	constexpr int		MAX_EVENT_BATCH = 32;
			int					pop_events(epoll_event* _events, int _max_events, chrono::tick::duration _ticks_wait) noexcept;
			void				get_result(const epoll_event& _event, RESULT& _result) const noexcept;

// implementation)
private:
			class executable_post;
			friend class executable_post;
			using CONTAINER_EXECUTABLE_QUEUE = CGDK::circular_list<RESULT>;

			own_ptr<executable_post> m_pexecutable_post;
};

}

#endif