//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                              Utility Classes                              *
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
// Icryptable
//
//
//-----------------------------------------------------------------------------
class Icryptable : virtual public Ireferenceable
{
protected:
			Icryptable() noexcept {}
	virtual	~Icryptable() noexcept {}

public:
			// - a reference of the cryptor (taken under m_lockable_cryptor : the disconnect resets it on another thread)
			object_ptr<Icryptor> get_cryptor() const noexcept { std::lock_guard lock(this->m_lockable_cryptor); return this->m_pcryptor; }
			auto cryption_type() const noexcept { return this->m_cryption_type; }
public:
			void process_set_cryption_type(eCRYPTION_TYPE _type) noexcept { this->m_cryption_type= _type;}
			void process_set_cryptor(object_ptr<Icryptor>&& _pcryptor) noexcept { object_ptr<Icryptor> pcryptor_old; { std::lock_guard lock(this->m_lockable_cryptor); pcryptor_old = std::move(this->m_pcryptor); this->m_pcryptor = std::move(_pcryptor); } }
			void process_reset_cryptor() noexcept { object_ptr<Icryptor> pcryptor_old; { std::lock_guard lock(this->m_lockable_cryptor); pcryptor_old = std::move(this->m_pcryptor); } }
private:
			// - m_pcryptor is read by the senders and the receiver and reset by the disconnect : read and written under m_lockable_cryptor
			//   (the old cryptor is released out of it)
			object_ptr<Icryptor>	m_pcryptor;
			eCRYPTION_TYPE			m_cryption_type{ eCRYPTION_TYPE::NONE };
	mutable	std::mutex				m_lockable_cryptor;
};


}