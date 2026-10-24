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
//
// _make_object_raw<TREF>
//
//
//
//-----------------------------------------------------------------------------
template <class T, class... TARGS>
[[nodiscard]] object_ptr<T> _make_object_raw(CGNEW_DEBUG_INFO_PARAMETERS CGNEW_DEBUG_INFO_COMMA TARGS&&... _args)
{
	class __X : public T, public Idisposable::Ndelete
	{
	public:
		__X(TARGS&&... _nested_args) : T(std::forward<TARGS>(_nested_args)...), Idisposable::Ndelete() {}
		virtual ~__X() noexcept { __on_object_delete<T>(this); }
	};

#pragma push_macro("new")
#undef new
	#ifdef _ENABLE_DEBUG_NEW
		#if defined(_WIN32)
			auto pobject = new(_NORMAL_BLOCK, CGNEW_DEBUG_INFO_PARAMETERS_PASSING) __X(std::forward<TARGS>(_args)...);
		#else
			auto pobject = new __X(std::forward<TARGS>(_args)...);
		#endif
	#else
		auto pobject = new __X(std::forward<TARGS>(_args)...);
	#endif
#pragma pop_macro("new")

	// Debug)
	CGNEW_DEBUG_INFO_SET(pobject, _filename, _line);

	// return)
	return pobject;
}

// Case) 'Npoolable<T>' inherited
template <class T, class... TARGS>
[[nodiscard]] std::enable_if_t<std::is_base_of_v<Npoolable<T>, T>, object_ptr<T>>
_make_object(CGNEW_DEBUG_INFO_PARAMETERS CGNEW_DEBUG_INFO_COMMA TARGS&&... _args)
{
	return T::alloc(CGNEW_DEBUG_INFO_PARAMETERS_PASSING CGNEW_DEBUG_INFO_COMMA std::forward<TARGS>(_args)...);
}

// Case) No 'Npoolable<T>' inherited
template <class T, class... TARGS>
[[nodiscard]] std::enable_if_t<!std::is_base_of_v<Npoolable<T>, T>, object_ptr<T>>
_make_object(CGNEW_DEBUG_INFO_PARAMETERS CGNEW_DEBUG_INFO_COMMA TARGS&&... _args)
{
	// check) T must inherit class'Ireferenceable'
	static_assert(std::is_base_of_v<Ireferenceable, T>, "CGObj: T must inherit 'Ireferenceable' class");

	// return) 
	return _make_object_create<T, TARGS...>(CGNEW_DEBUG_INFO_PARAMETERS_PASSING CGNEW_DEBUG_INFO_COMMA std::forward<TARGS>(_args)...);
}

template <class T, class... TARGS>
[[nodiscard]] std::tuple<T*, TARGS...> _make_create_parameters(TARGS&&... _args)
{
	return std::tuple<T*, TARGS...>(nullptr, std::forward<TARGS>(_args)...);
}

template<class T, class P>
[[nodiscard]] object_ptr<T> _make_object_tuple_begin(CGNEW_DEBUG_INFO_PARAMETERS CGNEW_DEBUG_INFO_COMMA std::tuple<P>&& /*_tuple*/)
{
	return _make_object<T>(CGNEW_DEBUG_INFO_PARAMETERS_PASSING);
}

template<class T, class P, class... TARGS>
[[nodiscard]] object_ptr<T> _make_object_tuple(CGNEW_DEBUG_INFO_PARAMETERS CGNEW_DEBUG_INFO_COMMA std::tuple<P>&& _tuple, TARGS&&... _args)
{
	return _make_object<T>(CGNEW_DEBUG_INFO_PARAMETERS_PASSING CGNEW_DEBUG_INFO_COMMA std::forward<TARGS>(_args)..., std::forward<P>(std::get<0>(_tuple)));
}

template<class T, class P1, class P2, class... TREST, class... TARGS>
[[nodiscard]] object_ptr<T> _make_object_tuple(CGNEW_DEBUG_INFO_PARAMETERS CGNEW_DEBUG_INFO_COMMA std::tuple<P1, P2, TREST...>&& _tuple, TARGS&&... _args)
{
	return _make_object_tuple<T>(CGNEW_DEBUG_INFO_PARAMETERS_PASSING CGNEW_DEBUG_INFO_COMMA std::move((std::tuple<P2, TREST...>&)_tuple), std::forward<TARGS>(_args)..., std::forward<P1>(std::get<0>(_tuple)));
}

template<class T, class P>
[[nodiscard]] object_ptr<T> _make_object_tuple(CGNEW_DEBUG_INFO_PARAMETERS CGNEW_DEBUG_INFO_COMMA std::tuple<P>&& _tuple)
{
	return _make_object<T>(CGNEW_DEBUG_INFO_PARAMETERS_PASSING CGNEW_DEBUG_INFO_COMMA std::forward<P>(std::get<0>(_tuple)));
}

template<class T, class P1, class P2, class... TREST>
[[nodiscard]] object_ptr<T> _make_object_tuple(CGNEW_DEBUG_INFO_PARAMETERS CGNEW_DEBUG_INFO_COMMA std::tuple<P1, P2, TREST...>&& _tuple)
{
	return _make_object_tuple<T>(CGNEW_DEBUG_INFO_PARAMETERS_PASSING CGNEW_DEBUG_INFO_COMMA std::move((std::tuple<P2, TREST...>&)_tuple), std::forward<P1>(std::get<0>(_tuple)));
}

template<class T, class P1, class P2, class... TREST>
[[nodiscard]] object_ptr<T> _make_object_tuple_begin(CGNEW_DEBUG_INFO_PARAMETERS CGNEW_DEBUG_INFO_COMMA std::tuple<P1, P2, TREST...>&& _tuple)
{
	return _make_object_tuple<T>(CGNEW_DEBUG_INFO_PARAMETERS_PASSING CGNEW_DEBUG_INFO_COMMA std::move((std::tuple<P2, TREST...>&)_tuple));
}


}