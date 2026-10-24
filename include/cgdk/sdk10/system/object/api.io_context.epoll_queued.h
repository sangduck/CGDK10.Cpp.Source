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

 @class		io_context_epoll_queued
 

*///-----------------------------------------------------------------------------
class api::io_context_epoll_queued : public io_context_Nepoll
{
// constructor/destructor)
public:
			io_context_epoll_queued();
	virtual	~io_context_epoll_queued() noexcept;

public:
			bool				create(int _hint_size = 4096) noexcept;
			CGHANDLE			dispose() noexcept;
	virtual	result_code			destroy() noexcept override;

			bool				post_executable(uint32_t _bytes_transferred, uintptr_t _completion_key, Iexecutable* _pexecutable) noexcept;
			bool				post_executable(uint32_t _bytes_transferred, uintptr_t _completion_key, object_ptr<Iexecutable>&& _executable) noexcept;

			void				pop_executable(RESULT& _result_event, chrono::tick::duration _ticks_wait) noexcept;
			[[nodiscard]] size_t queued_count() const noexcept { std::lock_guard cs(this->m_lockable_complete); return this->m_queue_executable.size(); }

// implementation)
private:
			void				_process_popup_epoll_event();
	static	void				_fn_process_popup_epoll_event(api::io_context_epoll_queued* _pobject);

			using CONTAINER_EXECUTABLE_QUEUE = CGDK::circular_list<RESULT>;

			CONTAINER_EXECUTABLE_QUEUE	m_queue_executable;
			int					m_count_wait;
	mutable	std::mutex			m_lockable_complete;
			std::condition_variable	m_cv_complete;
			object_ptr<api::threads> m_pthread;
};

}

#endif