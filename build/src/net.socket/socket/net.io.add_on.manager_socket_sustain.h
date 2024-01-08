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
// inherited classes)
	public						schedulable::Iexecutable,
	public						Nmessage_transmitter,
	virtual public				Imessageable,
	virtual public				object::Idestroyable
{
// publics) 
public:
	// 1) add_socket/remove_socket
			bool				add_socket( Nsocket_sustain* _pSocket);
			bool				remove_socket( Nsocket_sustain* _pSocket) noexcept;

	// 2) Init Instance/run_executor
	static	auto				instance() { if(m_pinstance.empty()){ initialize_instance();} return object_ptr<manager_socket_sustain>(m_pinstance);}
	static	void				initialize_instance();
	static	void				set_instance(own_ptr<manager_socket_sustain>&& _pinstance) noexcept;
	static	void				reset_instance() noexcept;
	static	void				detach_instance() noexcept { m_pinstance.detach(); }

// implementation) 
protected:
	virtual	intptr_t			process_execute( intptr_t _result, size_t _param) override;

// Messages)
protected:
	// 1) process_message Function
	virtual	result_code			process_message(sMESSAGE& _msg) override;

// implementation) 
private:
	// 1) socket List
			lockable<circular_list<Nsocket_sustain*>> m_list_socket;

	// 2) Instance
	static own_ptr<manager_socket_sustain> m_pinstance;
	static std::mutex			m_lockable_create;

private:
	// 3) destroy
	virtual	result_code			destroy() noexcept override;
};


}
