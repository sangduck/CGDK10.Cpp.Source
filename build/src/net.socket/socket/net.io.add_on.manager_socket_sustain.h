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

namespace CGDK
{
//-----------------------------------------------------------------------------
//
// net::io::add_on::manager_socket_sustain
//
//-----------------------------------------------------------------------------
class net::io::add_on::manager_socket_sustain :
	public						schedulable::Iexecutable,
	public						Nmessage_transmitter,
	virtual public				CGDK::Imessageable,
	virtual public				object::Idestroyable,
	private						non_copyable
{
public:
			bool				add_socket( Nsocket_sustain* _pSocket);
			bool				remove_socket( Nsocket_sustain* _pSocket) noexcept;

	static	auto				instance() { auto p = m_pinstance.get_acquire(); if (p == nullptr) { initialize_instance(); p = m_pinstance.get_acquire(); } return object_ptr<manager_socket_sustain>(p);}
	static	void				initialize_instance();
	static	void				set_instance(own_ptr<manager_socket_sustain>&& _pinstance) noexcept;
	static	void				reset_instance() noexcept;
	static	void				detach_instance() noexcept { m_pinstance.detach(); }

protected:
	virtual	intptr_t			process_execute( intptr_t _result, size_t _param) override;
	virtual	result_code			process_message(sMESSAGE& _msg) override;

private:
	mutable	lock_object			m_lockable_list_socket;
			circular_list<Nsocket_sustain*> m_list_socket;
	static own_ptr<manager_socket_sustain> m_pinstance;
	static std::mutex			m_lockable_create;

	virtual	result_code			destroy() noexcept override;
};


}
