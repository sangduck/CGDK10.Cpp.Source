//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                           Server Admin Classes                            *
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
// api::manager_performance_query
//
//-----------------------------------------------------------------------------
class api::manager_performance_query :
// inherited classes)
	virtual public				Ireferenceable,
	public						factory::_traits_system
{
// constructor/destructor)
protected:
			manager_performance_query();
	virtual ~manager_performance_query() noexcept;

// publics)
public:
			bool				update_query();

	static	object_ptr<manager_performance_query> instance() { auto p = m_pinstance.get_acquire(); if (p == nullptr) { initialize_instance(); p = m_pinstance.get_acquire(); } return object_ptr<manager_performance_query>(p); }

// implementation)
private:
			bool				add_counter(performance_counter* _pcounter, std::string_view _query);
			bool				add_counter(performance_counter* _pcounter, std::wstring_view _query);
			bool				remove_counter(performance_counter* _pcounter) noexcept;

private:
			HQUERY				m_hquery;
	mutable	lock_object			m_lockable_counter;

private:
	static	void				initialize_instance();
	static	lock_object			m_lockable_create;
	static	own_ptr<manager_performance_query>	m_pinstance;

	friend	class performance_counter;
};


}