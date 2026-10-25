//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                               server.query                                *
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
#include <atomic>

namespace CGDK
{
//-----------------------------------------------------------------------------
//
// Iserver_query
//
//
//-----------------------------------------------------------------------------
class Iserver_query : 
	virtual public				Ireferenceable
{
protected:
	virtual ~Iserver_query() noexcept {}

public:
			void				set_requestor(Iserver_queryrequestor* _pR) noexcept { this->m_prequestor = _pR;}
			void				reset_requestor() noexcept { this->m_prequestor.reset();}
	[[nodiscard]] auto			get_requestor() const noexcept { return this->m_prequestor;}
	[[nodiscard]] eOBJECT_STATE	get_object_state() const noexcept { return this->m_state_query.load(std::memory_order_acquire);}
public:
	virtual	void				process_execute_query() PURE;
	virtual	void				process_reset_session() noexcept PURE;

protected:
			object_ptr<Iserver_queryrequestor>	m_prequestor;
			std::atomic<eOBJECT_STATE> m_state_query{eOBJECT_STATE::NONE};
};


}