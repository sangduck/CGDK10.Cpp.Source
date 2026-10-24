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

//-----------------------------------------------------------------------------
/**

 @class		OBJ
 
*/
//-----------------------------------------------------------------------------
template<class T, class FLAG = void>
class CGDK::OBJ :
	public	Nreferenceable_no_act,
	public	T
{
public:
	OBJ() noexcept : Nreferenceable_no_act(), T() {}
	OBJ(const T& _copy) noexcept : Nreferenceable_no_act(), T(_copy) {}
	OBJ(T&& _move) noexcept : Nreferenceable_no_act(), T(std::move(_move)) {}
	template <class P, class... TARGS>
	OBJ(P&& _first, TARGS&&... _args) : Nreferenceable_no_act(), T(std::forward<P>(_first), std::forward<TARGS>(_args)...) {}
	OBJ(const OBJ<T,FLAG>& _copy) : Nreferenceable_no_act(), T(static_cast<T&>(_copy)) {}
	OBJ(OBJ<T, FLAG>&& _move) : Nreferenceable_no_act(), T(std::move(static_cast<T&>(_move))) {}
	virtual ~OBJ() noexcept { _process_on_final_release();}

public:
	template<class NTYPE>
	OBJ<T>&				operator=(const NTYPE& _rhs) { T::operator=(_rhs); return *this; }
	template<class NTYPE>
	OBJ<T>&				operator=(NTYPE&& _rhs) { T::operator=(std::forward<NTYPE>(_rhs)); return *this; }
	OBJ<T>&				operator=(const OBJ<T, FLAG>& _rhs) { T::operator=(static_cast<const T&>(_rhs)); return *this; }
	OBJ<T>&				operator=(OBJ<T, FLAG>&& _rhs) { T::operator=(std::move(static_cast<T&>(_rhs))); return *this; }

	template<class NTYPE>
	bool				operator==(const NTYPE& _rhs) const noexcept { return T::operator==(_rhs); }
	template<class NTYPE>
	bool				operator!=(const NTYPE& _rhs) const noexcept { return T::operator!=(_rhs); }
	template<class NTYPE>
	bool				operator>=(const NTYPE& _rhs) const noexcept { return T::operator>=(_rhs); }
	template<class NTYPE>
	bool				operator> (const NTYPE& _rhs) const noexcept { return T::operator> (_rhs); }
	template<class NTYPE>
	bool				operator<=(const NTYPE& _rhs) const noexcept { return T::operator<=(_rhs); }
	template<class NTYPE>
	bool				operator< (const NTYPE& _rhs) const noexcept { return T::operator< (_rhs); }
};

template<class T>
class CGDK::OBJ<T, std::enable_if_t<std::is_base_of_v<CGDK::object::Idestroyable, T>>> :
	public	Nreferenceable_no_act,
	public	T
{
public:
	OBJ() : Nreferenceable_no_act(), T() {}
	OBJ(const T& _copy) : Nreferenceable_no_act(), T(_copy) {}
	template <class P, class... TARGS>
	OBJ(P&& _first, TARGS&&... _args) : Nreferenceable_no_act(), T(std::forward<P>(_first), std::forward<TARGS>(_args)...) {}
	virtual ~OBJ() { T::destroy(); _process_on_final_release(); }

public:
	template<class NTYPE>
	OBJ<T>&				operator=(const NTYPE& _rhs) { T::operator=(_rhs); return *this; }
	template<class NTYPE>
	OBJ<T>&				operator=(NTYPE&& _rhs) { T::operator=(std::forward<NTYPE>(_rhs)); return *this; }

	template<class NTYPE>
	bool				operator==(const NTYPE& _rhs) const noexcept { return T::operator==(_rhs); }
	template<class NTYPE>
	bool				operator!=(const NTYPE& _rhs) const noexcept { return T::operator!=(_rhs); }
	template<class NTYPE>
	bool				operator>=(const NTYPE& _rhs) const noexcept { return T::operator>=(_rhs); }
	template<class NTYPE>
	bool				operator> (const NTYPE& _rhs) const noexcept { return T::operator> (_rhs); }
	template<class NTYPE>
	bool				operator<=(const NTYPE& _rhs) const noexcept { return T::operator<=(_rhs); }
	template<class NTYPE>
	bool				operator< (const NTYPE& _rhs) const noexcept { return T::operator< (_rhs); }
};
