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

 	factory::Iobject
 
*/
//-----------------------------------------------------------------------------
template <class T>
class factory::Iobject : virtual public Ifactory, private non_copyable
{
public:
	Iobject(std::string_view _name, eFACTORY_TYPE _factory_type)
	{
		if (_name.empty() == false)
		{
			this->name(_name);
		}
		else
		{
		#if defined(_MSC_VER)
			__if_exists(T::_get_static_factory_name)
			{
				this->name(T::_get_static_factory_name());
			}
			__if_not_exists(T::_get_static_factory_name)
			{
		#endif
			// - set default name
			#ifdef __cpp_rtti
				this->name(_factory_name<T>::get_default_name());
			#else
				this->name(__FUNCTION__);
			#endif
	
		#if defined(_MSC_VER)
			}
		#endif
		}

		this->m_statistics.type = (_factory_type != eFACTORY_TYPE::DEFAULT) ? _factory_type : factory::_get_default_pool_factory_type<T>();
		this->m_statistics.object_type = _get_object_type<T>();

		// 주의) pool factory의 경우 여기서 process_register_factory()를 호출한다. (recursion이 없을 것이므로)
		this->process_register_factory();
	}

	virtual	~Iobject() noexcept
	{
		// check)
		CGASSERT_ERROR((this->m_statistics.now.alloc_create + this->m_statistics.now.alloc_stack + this->m_statistics.now.alloc_stack_tls) == (this->m_statistics.now.free_delete + this->m_statistics.now.free_stack + this->m_statistics.now.free_stack_tls));

		// check)
		CGASSERT_ERROR(this->m_statistics.now.in_using == 0);

		// 1) unregister from factory manager
		this->process_unregister_factory();
	}

protected:
	template <class P>
	std::enable_if_t<std::is_base_of_v<factory::_traits_has_pool, P>, void>
								process_destroy_object(T* _object) noexcept
	{
		// check)
		CGASSERT_ERROR(_object != nullptr);

		// 1) delete _object
		delete _object;
	}
	template <class P>
	std::enable_if_t<!std::is_base_of_v<factory::_traits_has_pool, P>, void>
								process_destroy_object(T* _object) noexcept
	{
		// check)
		CGASSERT_ERROR(_object != nullptr);

		// 1) delete _object
		delete _object;

		// 2) decrease reference count (m_ppool에 setting되어 있지 않았다면 자체적으로 감소시킨다.)
		this->release();
	}
	template <class P>
	std::enable_if_t<std::is_base_of_v<factory::_traits_has_on_alloc, P>, void>
								process_on_alloc(T* _object)
	{
		_object->process_on_pool_alloc();
	}
	template <class P>
	std::enable_if_t<!std::is_base_of_v<factory::_traits_has_on_alloc, P>, void>
								process_on_alloc(T* /*_object*/)
	{
	}
	template <class P>
	std::enable_if_t<std::is_base_of_v<factory::_traits_has_on_dealloc, P>, void>
								process_on_dealloc(T* _object) noexcept
	{
		_object->process_on_pool_dealloc();
	}
	template <class P>
	std::enable_if_t<!std::is_base_of_v<factory::_traits_has_on_dealloc, P>, void>
								process_on_dealloc(T* /*_object*/) noexcept
	{
	}

	[[nodiscard]] virtual T*	create_object(CGNEW_DEBUG_INFO_PARAMETERS) PURE;
	virtual	result_code			destroy() noexcept override { this->close(); return result_code(eRESULT::SUCCESS); }

};


}
