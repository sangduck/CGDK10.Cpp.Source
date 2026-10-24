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

   executor::io_context_threads
 

*///-----------------------------------------------------------------------------
class executor::io_context_threads :
	public						executor::io_context,
	public						api::threads,
	virtual public				object::Inameable,
	virtual public				Iexecutor,
	virtual public				executor::Iio_context_threads
{
public:
			struct START_PARAMETER
			{
				int				thread_count;
				int				cpu_affinity = -1;		// - the CPU the threads are pinned to (-1 : not pinned). context : "cpu_affinity"
				uint32_t		busy_poll_usecs = 0;	// - busy poll of epoll_wait() of this io_context (linux epoll, kernel 6.9+. 0 : the default of executor::set_default_busy_poll()). context : "busy_poll"
				uint16_t		busy_poll_budget = 0;	// - packets per poll (0 : the kernel default). context : "busy_poll_budget"
				bool			prefer_busy_poll = false;	// - prefer busy poll over the NIC interrupt. context : "prefer_busy_poll"
				eTHREAD_PRIORITY thread_priority = eTHREAD_PRIORITY::DEFAULT;	// - the priority of the threads (DEFAULT : not changed). context : "thread_priority"
			};

			io_context_threads();
	virtual ~io_context_threads() noexcept;

			result_code			start() { context _context; return start(_context); }
			result_code			start(const START_PARAMETER& _parameter) { context temp; temp = buffer_view((char*)&_parameter, sizeof(START_PARAMETER)); return start(temp); }
			result_code			start(const context& _context) { context context_copy(_context); return start(context_copy); }
	virtual	result_code			start(context& _context) override;
	virtual	result_code			stop() noexcept override;
	virtual	void				request_stop() noexcept override;
	[[nodiscard]] virtual std::size_t existing_thread_count() const noexcept override { return this->get_existing_thread_count(); }

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
			std::mutex			m_cs_threads_extra;
			std::condition_variable m_cv_threads_extra;
			std::atomic<int>	m_thread_index{ 0 };
			int					m_cpu_affinity{ -1 };		// - the CPU the threads are pinned to (-1 : not pinned)
			eTHREAD_PRIORITY	m_thread_priority{ eTHREAD_PRIORITY::DEFAULT };	// - the priority of the threads (applied by each thread before its first execution)
			std::atomic<bool>	m_is_thread_priority_warned{ false };
			void				_apply_thread_priority() noexcept;
};


}