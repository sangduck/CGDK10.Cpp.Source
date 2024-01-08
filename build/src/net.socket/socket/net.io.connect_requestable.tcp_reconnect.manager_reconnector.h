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
// net::io::connect_requestable::Ntcp_reconnect (자동재접속 기능을 가진 TCP 연결자)
//
//-----------------------------------------------------------------------------
class CGDK::net::io::connect_requestable::Ntcp_reconnect::manager_reconnector :
	public						Nnameable,
	public						schedulable::Iexecutable,
	virtual public				Idestroyable
{
public:
			manager_reconnector();
			~manager_reconnector();

public:
			bool				register_reconnector(net::io::connect_requestable::Ntcp_reconnect* _preconnector);
			bool				unregister_reconnector(net::io::connect_requestable::Ntcp_reconnect* _preconnector) noexcept;

public:
	virtual	intptr_t			process_execute( intptr_t _result, size_t _param) override;

public:
			lockable<std::set<object_ptr<net::io::connect_requestable::Ntcp_reconnect>>> m_set_reconnect;

public:
	// 1) Init Instance/run_executor
	static 	auto				instance() { if (m_pinstance.empty()) { initialize_instance();} return object_ptr<manager_reconnector>(m_pinstance); }
	static	void				initialize_instance();
	static	void				set_instance(own_ptr<manager_reconnector>&& _pinstance) noexcept;
	static	void				reset_instance() noexcept;
	static	void				detach_instance() noexcept { m_pinstance.detach(); }

	// 2) Instance
	static std::mutex			m_lockable_create;
	static own_ptr<manager_reconnector> m_pinstance;

private:
	// 3) destroy
	virtual	result_code			destroy() noexcept override;
};
