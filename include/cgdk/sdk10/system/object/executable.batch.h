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

 @class		NBatch
 @brief		한꺼번에 여러 개의 Executable 객체를 큐잉해 걸어주는 Executable이다. 
 @todo		
 @ref		CList<TEXECUTABLE>
 @details	\n
 여러 개의 Executable 객체를 큐잉해 실행해준다. 
 따라서 실행을 걸어주게 되면 큐잉된 Executable 객체들은 제거된다.

*///-----------------------------------------------------------------------------
class executable::Nbatch : virtual public Iexecutable
{
public:
	virtual ~Nbatch() noexcept;

public:
			void				queue_executable(object_ptr<Iexecutable>&& _pexecutable);
			void				clear() noexcept;
			bool				request_execute(Iexecutor* _pexecutor);
			// - the executables requested (request_execute) and not completed yet (counted down after each one ran)
			size_t				executing_count() const noexcept;

			Nbatch&				operator<<(object_ptr<Iexecutable>&& _rhs) { queue_executable(std::move(_rhs)); return *this;}

protected:
	virtual	void				process_complete_execution() PURE;
	virtual intptr_t			process_execute(intptr_t _return, size_t _param) override;
	virtual void				on_final_release() noexcept override;

			// - one requested execution ended (ran, threw, or the post failed). the last one calls process_complete_execution()
			void				_complete_one() noexcept;

	mutable	lock_object			m_lockable_list_executable;
			std::list<object_ptr<Iexecutable>> m_list_executable;
			std::atomic<size_t>	m_count_executing{ 0 };
};


}
