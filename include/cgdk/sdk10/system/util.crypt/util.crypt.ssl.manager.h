//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                          Utinity.Crypto Classes                           *
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
// CGDK::crypt::ssl_manager
//
//
//-----------------------------------------------------------------------------
namespace crypt
{

class ssl_manager :
	public						object::Nnameable,
	virtual public				object::Idestroyable
{
public:
			ssl_manager() noexcept;
	virtual ~ssl_manager() noexcept;

	static	auto				instance() { auto p = m_s_pinstance.get_acquire(); if (p == nullptr) { initialize_instance(); p = m_s_pinstance.get_acquire(); } return object_ptr<ssl_manager>(p);}
	static	void				initialize_instance();
	static	void				set_instance(own_ptr<ssl_manager>&& _pinstance) noexcept;
	static	void				reset_instance() noexcept;

	[[nodiscard]] const SSL_METHOD*	get_ssl_method(int _type) const noexcept;

private:
	virtual result_code			initialize();
	virtual	result_code			destroy() noexcept override;

	mutable	lock_object			m_lockable_vector_ssl_method;
			std::vector<const SSL_METHOD*> m_vector_ssl_method;

	static	std::mutex			m_s_lockable_create;
	static	own_ptr<ssl_manager> m_s_pinstance;
	static	void				_set_instance(own_ptr<ssl_manager>&& _instance);

};


}
}