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

 @class		_executable_callable
 @brief		the executable of make_executable(callable)
 @details	\n
 the callable is kept as it is (no std::function : no heap allocation for its captures)
 and destroyed when the executable is released (a pooled executable does not keep
 the captures (object_ptr etc.) alive until it is reused)

*///-----------------------------------------------------------------------------
template <class TCALLABLE>
class _executable_callable : public Iexecutable, public factory::_traits_system
{
public:
	template <class TFUNCTION>
			void				set_function(TFUNCTION&& _function) { this->m_callable.emplace(std::forward<TFUNCTION>(_function)); }

private:
			std::optional<TCALLABLE> m_callable;

	virtual	intptr_t			process_execute(intptr_t, size_t) override
	{
		if constexpr (std::is_void_v<std::invoke_result_t<TCALLABLE&>>)
		{
			(*this->m_callable)();
			return intptr_t(0);
		}
		else
		{
			return static_cast<intptr_t>((*this->m_callable)());
		}
	}
	virtual	void				on_final_release() noexcept override { this->m_callable.reset(); }
};

//-----------------------------------------------------------------------------
/**

 @class		nake_executable functions

*///-----------------------------------------------------------------------------
template <class TFUNCTION>
static	std::enable_if_t<!std::is_base_of_v<Iexecutable, std::remove_pointer_t<std::decay_t<TFUNCTION>>> && !std::is_function<std::remove_pointer_t<std::decay_t<TFUNCTION>>>::value, object_ptr<Iexecutable>>
make_executable(TFUNCTION&& _function)
{
	// declaration)
	using executable_function = _executable_callable<std::decay_t<TFUNCTION>>;

	// 1) alloc executable
	auto pexecutable = alloc_object<executable_function>();

	// 2) set '_function' to pexecutable
	pexecutable->set_function(std::forward<TFUNCTION>(_function));

	// return)
	return pexecutable;
}

template <class TFUNCTION>
static	std::enable_if_t<!std::is_base_of_v<Iexecutable, std::remove_pointer_t<std::decay_t<TFUNCTION>>>&& std::is_function<std::remove_pointer_t<std::decay_t<TFUNCTION>>>::value, object_ptr<Iexecutable>>
make_executable(TFUNCTION&& _function)
{
	// declaration)
	using executable_function = _executable_callable<std::decay_t<TFUNCTION>>;

	// 1) alloc executable
	auto pexecutable = alloc_object<executable_function>();

	// 2) set '_function' to pexecutable
	pexecutable->set_function(std::forward<TFUNCTION>(_function));

	// return)
	return pexecutable;
}

template <class T, class TFUNCTION, class... TARGS>
static	std::enable_if_t<std::is_base_of_v<Ireferenceable, std::remove_pointer_t<std::decay_t<T>>>&& std::is_member_function_pointer_v<TFUNCTION>, object_ptr<Iexecutable>>
make_executable(T* _this, TFUNCTION _function, TARGS&&... _args)
{
	object_ptr<T> hold_this(_this);
	return make_executable([=]()
		{
			// - call function
			(hold_this->*_function)(std::forward<TARGS>(_args)...);
		});
}
template <class T, class TFUNCTION, class... TARGS>
static	std::enable_if_t<std::is_base_of_v<Ireferenceable, std::remove_pointer_t<std::decay_t<T>>>&& std::is_member_function_pointer_v<TFUNCTION>, object_ptr<Iexecutable>>
make_executable(TFUNCTION _function, TARGS&&... _args)
{
	// 2) post
	return make_executable([=]()
		{
			// - call function
			_function(std::forward<TARGS>(_args)...);
		});
}


}
