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
			auto get_cryptor() const noexcept { return this->m_pcryptor; }
			auto cryption_type() const noexcept { return this->m_cryption_type; }
public:
			void process_set_cryption_type(eCRYPTION_TYPE _type) noexcept { this->m_cryption_type= _type;}
			auto process_set_cryptor(object_ptr<Icryptor>&& _pcryptor) noexcept { this->m_pcryptor = std::move(_pcryptor); }
			void process_reset_cryptor() noexcept { this->m_pcryptor.reset(); }
private:
			object_ptr<Icryptor>	m_pcryptor;
			eCRYPTION_TYPE			m_cryption_type{ eCRYPTION_TYPE::NONE };
};


}