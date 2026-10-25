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

 @class		executor::queue_list
 @brief		IOCP를 사용한 스레드풀을 구성한 executor이다.
 @todo		
 @ref		
 @details	\n
 @remark	\n
 IOCP에 Socket을 붙이거나 IO를 Post하는 용도로 주로 사용된다.
 이 IOCP Completion Port는 상속을 하여 Thread용으로 사용되기도 하지만 주로 .
 Client IOCP에 많이 사용된다. MainLoop와 동일한 Thread에서 사용하기 위해
 Execut()함수를 TimeOut을 0으로 절어줘서 사용한다.

*///-----------------------------------------------------------------------------
class executor::queue_list_threads :
	public						executor::queue_list,
	public						api::threads,
	virtual public				object::Inameable,
	virtual public				Iexecutor
{
public:
			struct START_PARAMETER
			{
				int				thread_count;
			};

			queue_list_threads();
	virtual ~queue_list_threads() noexcept;

			result_code			start() { context _context; return start(_context); }
			result_code			start(const START_PARAMETER& _parameter) { context temp; temp = buffer_view((char*)&_parameter, sizeof(START_PARAMETER)); return start(temp); }
			result_code			start(const context& _context) { context context_copy(_context); return start(context_copy); }
	virtual	result_code			start(context& _context) override;
	virtual	result_code			stop() noexcept override;

protected:
			uint32_t			process_thread_run();
			uint32_t			process_thread_run_extra();
	virtual	bool				process_control_thread_blocked() noexcept;
	virtual	bool				process_control_thread_unblocked() noexcept;
	virtual	void				process_begin_thread_object(api::thread* _pthread) noexcept override;
	virtual	void				process_end_thread_object(api::thread* _pthread) noexcept override;

protected:
			void				_is_extra_thread_run() noexcept;
			void				_process_stop_extra_thread() noexcept;

			std::atomic<bool>	m_is_active{ false };		// - stop()이 쓰고 thread들이 loop에서 읽는다. (atomic : 경합/최적화로 loop를 못 빠져나오지 않도록)
			bool				m_enable_auto_expand{ false };

			int					m_count_threads_blocked{ 0 };
			int					m_count_threads_extra_threshold{ 0 };
			int					m_count_threads_extra_active{ 0 };
			int					m_count_threads_extra_exist{ 0 };
			int					m_count_threads_extra_max{ 0 };
			int					m_count_threads_extra_wake{ 0 };	// wakes of waiting extra threads not taken yet (counted as active when notified : a second thread_blocked() spawned nothing in the window)
			std::mutex			m_lockable_threads_extra;
			std::condition_variable m_cv_threads_extra;
			std::atomic<int>	m_thread_index{ 0 };
};


}