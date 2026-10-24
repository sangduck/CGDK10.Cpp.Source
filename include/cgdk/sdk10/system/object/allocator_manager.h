//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                              object classes                               *
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
/**

 @class		allocator_manager
 @brief		
 @todo		
 @ref		
 @details	\n

*///-----------------------------------------------------------------------------
class allocator_manager
{
public:
	class allocator : virtual public Ireferenceable
	{
	public:
		virtual object_ptr<Ireferenceable> make_object_ptr() PURE;
		virtual object_ptr<Ireferenceable> make_own_ptr() PURE;
	};


public:
	allocator_manager();
	virtual ~allocator_manager() noexcept;

			auto				get_allocator(const std::string& _name) const -> object_ptr<allocator>;
			void				register_allocator(std::string&& _name, own_ptr<allocator>&& _allocator);
			void				unregister_allocator(std::string&& _name) noexcept;

private:
	mutable	lock_object			m_lockable_map_allocator;
			std::map<std::string, own_ptr<allocator>> m_map_allocator;
};


}