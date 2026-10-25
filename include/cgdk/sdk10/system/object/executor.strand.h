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

 @class		executable::strand
 

*///-----------------------------------------------------------------------------
class executor::strand :
	virtual public				Iexecutor,
	virtual public				Iexecutable,
	public						object::Ninitializable_startable,
	public						object::Nnameable,
	private						non_copyable

{
public:
			strand() noexcept;
	virtual ~strand() noexcept;

public:
			bool				post_at(const object_ptr<Iexecutable>& _pexecutable, chrono::tick::time_point _tick_execute) { return this->process_post_at(object_ptr<Iexecutable>(_pexecutable), _tick_execute); }
			bool				post_at(object_ptr<Iexecutable>&& _pexecutable, chrono::tick::time_point _tick_execute) { return this->process_post_at(std::move(_pexecutable), _tick_execute); }

	[[nodiscard]] auto			executor() const noexcept { std::lock_guard lock(this->m_lockable_pexecutor); return this->m_pexecutor;}
			void				executor(const object_ptr<Iexecutor>& _pexecutor) noexcept { this->executor(object_ptr<Iexecutor>(_pexecutor));}
			void				executor(object_ptr<Iexecutor>&& _pexecutor) noexcept { std::lock_guard lock(this->m_lockable_pexecutor); this->m_pexecutor = std::move(_pexecutor);}
	[[nodiscard]] bool			has_executor() const noexcept { std::lock_guard lock(this->m_lockable_pexecutor); return this->m_pexecutor.exist();}

protected:
			struct EXECUTABLE
			{
				object_ptr<Iexecutable> pexecutable;
				intptr_t result = 0;
				size_t param = 0;
			};
			class executor_post_at;

	mutable	lock_object			m_lockable_pexecutor;
			object_ptr<Iexecutor> m_pexecutor;

	mutable	std::mutex			m_cs_queue_executable;		// not recursive : nothing is called back while it is held (the executables are released outside)
			std::vector<EXECUTABLE> m_queue_executable;
			std::multimap<chrono::tick::time_point, EXECUTABLE> m_queue_executable_at;	// post_at : by the time (the same time : in the order of the posts)
			std::multiset<chrono::tick::time_point> m_set_tick_wake;	// the times of the wakes pending in the scheduler (one is posted only when none is earlier)
			bool				m_is_processing = false;	// posted to the executor or executing (one execution at a time)
			bool				m_is_executing = false;		// a thread is executing the queue now (false : only posted, not started yet)

	virtual result_code			_process_initialize(context& _context);
	virtual result_code			_process_destroying() noexcept;

	virtual	bool				process_post(object_ptr<Iexecutable>&& _pexecutable) override;
	virtual	bool				process_post_at(object_ptr<Iexecutable>&& _pexecutable, chrono::tick::time_point _tick_execute);
	virtual	bool				process_execute(chrono::tick::duration _ticks_wait, intptr_t _option) override;
	virtual	intptr_t			process_execute(intptr_t _result, size_t _param) override;
			void				process_execute_post_at(chrono::tick::time_point _tick_wake);
			auto				process_get_executor() -> object_ptr<Iexecutor>;
			void				process_post_next();
			bool				_post_to_executor() noexcept;
			bool				_post_wake(chrono::tick::time_point _tick_wake) noexcept;
};


}