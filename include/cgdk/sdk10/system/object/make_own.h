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
// _make_own<TREF>
//
//
//
//-----------------------------------------------------------------------------
template <class T>
class _make_own_raw_object : public T, public Idisposable::Ndelete
{
public:
    template <class... TARGS>
    explicit _make_own_raw_object(TARGS&&... _args) : T(std::forward<TARGS>(_args)...), Idisposable::Ndelete() {}
    virtual ~_make_own_raw_object() {}
};

template <class T, class... TARGS>
[[nodiscard]] own_ptr<T> _make_own_raw(CGNEW_DEBUG_INFO_PARAMETERS CGNEW_DEBUG_INFO_COMMA TARGS&&... _args)
{

#pragma push_macro("new")
#undef new
	#ifdef _ENABLE_DEBUG_NEW
		#if defined(_WIN32)
			auto pobject = new(_NORMAL_BLOCK, CGNEW_DEBUG_INFO_PARAMETERS_PASSING) _make_own_raw_object<T>(std::forward<TARGS>(_args)...);
		#else
			auto pobject = new _make_own_raw_object<T>(std::forward<TARGS>(_args)...);
		#endif
	#else
		auto pobject = new _make_own_raw_object<T>(std::forward<TARGS>(_args)...);
	#endif
#pragma pop_macro("new")

	// Debug)
	CGNEW_DEBUG_INFO_SET(pobject, _filename, _line);

	// return)
	return own_ptr<T>(typename own_ptr<T>::OWN_SET(pobject));
}

// Case) 'Npoolable<T>' inherited
template <class T, class... TARGS>
[[nodiscard]] std::enable_if_t<std::is_base_of_v<Npoolable<T>, T>, own_ptr<T>>
_make_own(CGNEW_DEBUG_INFO_PARAMETERS CGNEW_DEBUG_INFO_COMMA TARGS&&... _args)
{
	return own_ptr<T>(typename own_ptr<T>::OWN_SET(T::alloc(CGNEW_DEBUG_INFO_PARAMETERS_PASSING CGNEW_DEBUG_INFO_COMMA std::forward<TARGS>(_args)...)));
}

// Case) No 'Npoolable<T>' inherited
template <class T, class... TARGS>
[[nodiscard]] std::enable_if_t<!std::is_base_of_v<Npoolable<T>, T>, own_ptr<T>>
_make_own(CGNEW_DEBUG_INFO_PARAMETERS CGNEW_DEBUG_INFO_COMMA TARGS&&... _args)
{
	// check) T must inherit class'Ireferenceable'
	static_assert(std::is_base_of_v<Ireferenceable, T>, "CGObj: T must inherit 'Ireferenceable' class");

	auto pobject = _make_object_create<T, TARGS...>(CGNEW_DEBUG_INFO_PARAMETERS_PASSING CGNEW_DEBUG_INFO_COMMA std::forward<TARGS>(_args)...);

	// return) 
	return own_ptr<T>(typename own_ptr<T>::OWN_SET(pobject));
}

template <class T, class... TARGS>
[[nodiscard]] std::tuple<T*, TARGS...> _make_own_parameters(TARGS&&... _args)
{
	return std::tuple<T*, TARGS...>(nullptr, std::forward<TARGS>(_args)...);
}

template<class T, class P>
[[nodiscard]] own_ptr<T> _make_own_tuple_begin(CGNEW_DEBUG_INFO_PARAMETERS CGNEW_DEBUG_INFO_COMMA std::tuple<P>&& /*_tuple*/)
{
	return	_make_own<T>(CGNEW_DEBUG_INFO_PARAMETERS_PASSING);
}

template<class T, class P, class... TARGS>
[[nodiscard]] own_ptr<T> _make_own_tuple(CGNEW_DEBUG_INFO_PARAMETERS CGNEW_DEBUG_INFO_COMMA std::tuple<P>&& _tuple, TARGS&&... _args)
{
	return	_make_own<T>(CGNEW_DEBUG_INFO_PARAMETERS_PASSING CGNEW_DEBUG_INFO_COMMA std::forward<TARGS>(_args)..., std::forward<P>(std::get<0>(_tuple)));
}

template<class T, class P1, class P2, class... TREST, class... TARGS>
[[nodiscard]] own_ptr<T> _make_own_tuple(CGNEW_DEBUG_INFO_PARAMETERS CGNEW_DEBUG_INFO_COMMA std::tuple<P1, P2, TREST...>&& _tuple, TARGS&&... _args)
{
	return	_make_own_tuple<T>(CGNEW_DEBUG_INFO_PARAMETERS_PASSING CGNEW_DEBUG_INFO_COMMA std::move((std::tuple<P2, TREST...>&)_tuple), std::forward<TARGS>(_args)..., std::forward<P1>(std::get<0>(_tuple)));
}

template<class T, class P>
[[nodiscard]] own_ptr<T> _make_own_tuple(CGNEW_DEBUG_INFO_PARAMETERS CGNEW_DEBUG_INFO_COMMA std::tuple<P>&& _tuple)
{
	return	_make_own<T>(CGNEW_DEBUG_INFO_PARAMETERS_PASSING CGNEW_DEBUG_INFO_COMMA std::forward<P>(std::get<0>(_tuple)));
}

template<class T, class P1, class P2, class... TREST>
[[nodiscard]] own_ptr<T> _make_own_tuple(CGNEW_DEBUG_INFO_PARAMETERS CGNEW_DEBUG_INFO_COMMA std::tuple<P1, P2, TREST...>&& _tuple)
{
	return	_make_own_tuple<T>(CGNEW_DEBUG_INFO_PARAMETERS_PASSING CGNEW_DEBUG_INFO_COMMA std::move((std::tuple<P2, TREST...>&)_tuple), std::forward<P1>(std::get<0>(_tuple)));
}

template<class T, class P1, class P2, class... TREST>
[[nodiscard]] own_ptr<T> _make_own_tuple_begin(CGNEW_DEBUG_INFO_PARAMETERS CGNEW_DEBUG_INFO_COMMA std::tuple<P1, P2, TREST...>&& _tuple)
{
	return	_make_own_tuple<T>(CGNEW_DEBUG_INFO_PARAMETERS_PASSING CGNEW_DEBUG_INFO_COMMA std::move((std::tuple<P2, TREST...>&)_tuple));
}


}