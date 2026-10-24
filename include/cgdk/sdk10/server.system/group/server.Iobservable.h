//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                         Socket template Classes                           *
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

 @class		array
 @brief		옵저버를 가질 수 있기 위한 인터페이스 클래스이다.
 @todo		
 @ref		
 @details	\n
 동작 형태는 그룹과 동일하다 다만 그룹과 다르게 배타적으로 동작하지 않는다.
 즉 옵저버는 동시에 여러 객체를 옵저빙할 수 있다.
       
*///-----------------------------------------------------------------------------
template <class _TOBSERVER>
class Iobservable :
	virtual public				Ireferenceable
{
public:
	using observer_t = _TOBSERVER;
	using observer_channel_t = typename observer_t::observer_channel_t;

public:
	virtual	bool				unsubscribe(observer_t* _pobserver, observer_channel_t _ocid) noexcept PURE;
	virtual	void				unsubscribe_all() noexcept PURE;

			void				lock_observer() noexcept { Ireferenceable::add_ref(); this->m_lockable_observable_enter.lock();}
			void				unlock_observer() noexcept { this->m_lockable_observable_enter.unlock(); Ireferenceable::release();}
			lock_object&		get_observer_lockable() noexcept { return this->m_lockable_observable;}
			const lock_object&	get_observer_lockable() const noexcept { return this->m_lockable_observable;}
								operator lock_object&() noexcept { return this->m_lockable_observable;}
								operator const lock_object& () const noexcept { return this->m_lockable_observable;}
protected:
	mutable	lock_object			m_lockable_observable_enter;
	mutable	lock_object			m_lockable_observable;
protected:
	static	void				_set_observer_data	(_TOBSERVER* _pobserver, void* _pdata) noexcept { _pobserver->m_pobserver_data=_pdata;}
	static	void				_reset_observer_data(_TOBSERVER* _pobserver) noexcept { _pobserver->m_pobserver_data=nullptr; }
	static	void*				_get_observer_data	(_TOBSERVER* _pobserver) noexcept { return _pobserver->m_pobserver_data;}
};


}