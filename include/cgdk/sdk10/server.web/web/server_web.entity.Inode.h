//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 9.0 / Release 2017.01.03                        *
//*                                                                           *
//*                             Web Server Classes                            *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
//*  Best for Game Developement and Optimized for Game Developement.          *
//*                                                                           *
//*                 (c) 2016 Cho Sanghyun All right reserved.                 *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************

#pragma once

namespace CGDK
{
//-----------------------------------------------------------------------------
//
// CGDK::server::web::entity::Ifolder
//
//
// ----------------------------------------------------------------------------
class server::web::entity::Inode :
	virtual public				server::web::Ientity
{
public:
			Inode() noexcept;
	virtual ~Inode() noexcept;

public:
			bool				attach_child			(Ientity* _pchild);
			void				detach_child			(Ientity* _pchild) noexcept;
			void				detach_child_all		() noexcept;

			object_ptr<Ientity>	find_child_entity		(std::string_view _source) const noexcept;
protected:
	virtual	result_code			process_update			(context& _context) override;

	mutable	lock_object			m_lockable_map_child;
			std::map<std::string, object_ptr<server::web::Ientity>, std::less<>> m_map_child;
};


}