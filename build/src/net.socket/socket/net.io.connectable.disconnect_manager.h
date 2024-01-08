//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                          network socket classes                           *
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

//-----------------------------------------------------------------------------
//
// net::io::connectable::manager_disconnect
//
// 1. net::io::connectable::manager_disconnect은~
//    1) connect Interface를 가진 Interface Class이다.
//    2) 단순히 connect()를 수행하기 위한 Interface들로만 되어 있다.
//
//
//-----------------------------------------------------------------------------
#if defined(_WINSOCK2API_)
class CGDK::net::io::connectable::manager_disconnect :
	public						schedulable::Iexecutable,
	public						object::Nnameable,
	virtual public				object::Idestroyable
{
protected:
			manager_disconnect();
	virtual	~manager_disconnect() noexcept;

public:
	static 	auto				instance() { if (m_pinstance.empty()) { initialize_instance(); } return object_ptr<manager_disconnect>(m_pinstance); }
	static	void				initialize_instance();
	static	void				set_instance(own_ptr<manager_disconnect>&& _pinstance);
	static	void				reset_instance() noexcept;
	static	void				detach_instance() noexcept { m_pinstance.detach(); }

			void				join( net::io::Iconnectable* _pbinder);
			void				leave( net::io::Iconnectable* _pbinder) noexcept;

protected:
	virtual	intptr_t			process_execute( intptr_t _result, std::size_t _param) override;

private:
			lockable<circular_list<net::io::Iconnectable*>>	m_list_connectable;

	static std::mutex			m_lockable_create;
	static own_ptr<manager_disconnect> m_pinstance;

private:
	virtual	result_code			destroy() noexcept override;
};
#endif
