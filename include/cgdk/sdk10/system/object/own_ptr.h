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

 @class		own_ptr

*/
//-----------------------------------------------------------------------------
template <class TREF>
class own_ptr
{
// definitions)
public:
			using value_type = typename _CGTRAITS_PTR<TREF>::value_type;
			using _mytype = own_ptr<TREF>;

			//! @brief owning set : add_ref() is performed (for a newly created object)
			class  OWN_SET
			{
			public:
			constexpr explicit OWN_SET(value_type* _ptr) noexcept : ptr(_ptr) {}
			public:
				value_type*	ptr;
			};

private:
		#ifdef _USE_CHECK_NULLPTR
			void CHECK_NULLPTR() const
			{
				CGASSERT_ERROR(this->m_ref != nullptr);

				if (this->m_ref == nullptr)
					throw std::invalid_argument("own_ptr: dereferenced nullptr");
			}
		#else
			void CHECK_NULLPTR() const
			{
				CGASSERT_ERROR(this->m_ref != nullptr);
			}
		#endif

// constructor/destructor)
public:
	constexpr own_ptr() noexcept : m_ref(nullptr) {}
	constexpr own_ptr(std::nullptr_t) noexcept : m_ref(nullptr) {}
	constexpr own_ptr(own_ptr&& _ref) noexcept : m_ref(_ref.m_ref) { _ref.m_ref = nullptr; }
			template <class T>
	constexpr own_ptr(own_ptr<T>&& _ref) noexcept : m_ref(_ref.detach()) {}
	constexpr explicit own_ptr(OWN_SET _set) noexcept : m_ref(_set.ptr) { if (m_ref != nullptr) static_cast<Ireferenceable*>(m_ref)->add_ref(); }
	constexpr own_ptr(value_type* _ref, adopt_ref_t) noexcept : m_ref(_ref) {}
	constexpr ~own_ptr() noexcept;

	// - own_ptr can not be copied. (use std::move)
	constexpr own_ptr(const own_ptr&) noexcept = delete;
			template <class T>
	constexpr own_ptr(const own_ptr<T>&) noexcept = delete;
			template <class T>
	constexpr own_ptr(const object_ptr<T>&) noexcept = delete;
			template <class T>
	constexpr own_ptr(T*) noexcept = delete;

// public)
public:
	// 1) basic operating functions
	constexpr void				destroy() noexcept;
	constexpr void				reset() noexcept { this->_reset_own(); }
	constexpr void				reset(std::nullptr_t) noexcept { this->_reset_own(); }
			template <class T>
	constexpr void				force_assign(T* _ref) noexcept { this->_reset_own(_ref); }
	constexpr void				swap(own_ptr& _ref) noexcept { value_type* temp = m_ref; m_ref = _ref.m_ref; _ref.m_ref = temp; }
			template <class T>
	constexpr void				move(own_ptr<T>&& _rhs) noexcept;
	//! @brief give up the ownership without destroy()/release() and return it (the caller owns the reference)
	constexpr value_type*		detach() noexcept { value_type* p = m_ref; m_ref = nullptr; return p; }

	[[nodiscard]] constexpr value_type* get() const noexcept { return m_ref; }
	[[nodiscard]] constexpr value_type** get_ptr() noexcept { return &m_ref; }
	[[nodiscard]] constexpr value_type* const* get_ptr() const noexcept { return &m_ref; }
	[[nodiscard]] constexpr auto get_object_ptr() const noexcept { return object_ptr<value_type>(m_ref); }
			template <class T>
	[[nodiscard]] constexpr auto get_object_ptr() noexcept { return object_ptr<T>(m_ref); }

	[[nodiscard]] constexpr bool exist() const noexcept { return m_ref != nullptr; }
	[[nodiscard]] constexpr bool empty() const noexcept { return m_ref == nullptr; }

	// - singleton (double-checked locking) : read with get_acquire() outside the lock,
	//   publish with exchange_release() inside the lock. (release(store) / acquire(load) pair :
	//   no other thread can see the pointer before the object is fully constructed. a plain mov on x86)
	[[nodiscard]] value_type*	get_acquire() const noexcept { return std::atomic_ref<value_type*>(const_cast<value_type*&>(m_ref)).load(std::memory_order_acquire); }
	//! @brief publish _ref and return the previous owner (its reference is handed over, not re-counted)
				_mytype			exchange_release(_mytype&& _ref) noexcept { value_type* pnew = _ref.detach(); return _mytype(std::atomic_ref<value_type*>(m_ref).exchange(pnew, std::memory_order_acq_rel), adopt_ref); }

			template <class T>
	constexpr own_ptr<T>		force_casting_move() noexcept { return own_ptr<T>(reinterpret_cast<typename own_ptr<T>::value_type*>(this->detach()), adopt_ref); }

	// 2) operator- assignment-Enabled
	constexpr _mytype&			operator=(own_ptr&& _rhs) noexcept { this->move(std::move(_rhs)); return *this; }
			template <class T>
	constexpr _mytype&			operator=(own_ptr<T>&& _rhs) noexcept { this->move(std::move(_rhs)); return *this; }
	constexpr _mytype&			operator=(std::nullptr_t) noexcept { this->_reset_own(); return *this; }

	// 4) operator-
	[[nodiscard]] constexpr value_type* operator->() const { CHECK_NULLPTR(); return m_ref; }
	[[nodiscard]] constexpr value_type&	operator*() const { CHECK_NULLPTR(); return *m_ref; }
	[[nodiscard]] constexpr value_type& operator[](size_t _index) const { CHECK_NULLPTR(); return m_ref[_index]; }

	// 5) operator- conversion
	[[nodiscard]] constexpr		operator object_ptr<value_type>() const noexcept { return object_ptr<value_type>(m_ref); }
	[[nodiscard]] constexpr 	operator value_type*() const noexcept { return m_ref; }
	[[nodiscard]] constexpr		operator bool() const noexcept { return m_ref != nullptr; }

	// 6) operator- Assignment-Disabled
	constexpr _mytype&			operator=(const own_ptr&) noexcept = delete;
			template <class T>
	constexpr _mytype&			operator=(const own_ptr<T>&) noexcept = delete;
			template <class T>
	constexpr _mytype&			operator=(object_ptr<T>&&) noexcept = delete;
			template <class T>
	constexpr _mytype&			operator=(const object_ptr<T>&) noexcept = delete;
			template <class T>
	constexpr _mytype&			operator=(T*) noexcept = delete;

// implementation)
public:
	constexpr void				_clear() noexcept { m_ref = nullptr; }
private:
	static constexpr void		_destroy_own(value_type* _ref) noexcept;

	constexpr void				_reset_own() noexcept;
	constexpr void				_reset_own(value_type* _ref) noexcept;

// members)
private:
			value_type*			m_ref;

			template <class> friend class object_ptr;
			template <class> friend class own_ptr;
};

//-----------------------------------------------------------------------------
// comparison (C++20 : "!=", "<", "<=", ">", ">=" and the reversed forms are synthesized.
//             own_ptr vs object_ptr is served by the reversed form of object_ptr.h)
//-----------------------------------------------------------------------------
template <class TONE, class TOTHER>	[[nodiscard]] constexpr bool operator==(const own_ptr<TONE>& _lhs, const own_ptr<TOTHER>& _rhs) noexcept { return _lhs.get() == _rhs.get(); }
template <class TONE, class TOTHER>	[[nodiscard]] constexpr std::strong_ordering operator<=>(const own_ptr<TONE>& _lhs, const own_ptr<TOTHER>& _rhs) noexcept { return std::compare_three_way{}(_lhs.get(), _rhs.get()); }

template <class TONE, class TPTR>	[[nodiscard]] constexpr bool operator==(const own_ptr<TONE>& _lhs, TPTR* _rhs) noexcept { return _lhs.get() == _rhs; }
template <class TONE, class TPTR>	[[nodiscard]] constexpr std::strong_ordering operator<=>(const own_ptr<TONE>& _lhs, TPTR* _rhs) noexcept { return std::compare_three_way{}(_lhs.get(), _rhs); }

template <class TONE>				[[nodiscard]] constexpr bool operator==(const own_ptr<TONE>& _lhs, std::nullptr_t) noexcept { return _lhs.get() == nullptr; }

//-----------------------------------------------------------------------------
// swap
//-----------------------------------------------------------------------------
template <class T>
constexpr void swap(own_ptr<T>& _lhs, own_ptr<T>& _rhs) noexcept
{
	_lhs.swap(_rhs);
}

//-----------------------------------------------------------------------------
// implementation
//-----------------------------------------------------------------------------
template <class TREF>
constexpr own_ptr<TREF>::~own_ptr() noexcept
{
	// 1) get m_ref
	value_type*	p = m_ref;

	// check) nothing to do if empty
	if (p == nullptr)
		return;

	// 2) destroy (m_ref is kept alive during destroy() : the object may still be looked up through this owner)
	this->_destroy_own(p);

	// 3) release (re-read : destroy() may have reset/reassigned this owner re-entrantly)
	p = m_ref;

	if (p != nullptr)
	{
		m_ref = nullptr;

		static_cast<Ireferenceable*>(p)->release();
	}
}

template <class TREF>
constexpr void own_ptr<TREF>::destroy() noexcept
{
	// 1) get m_ref
	value_type* p = m_ref;

	// check) nothing to do if empty
	if (p == nullptr)
		return;

	// 2) keep the object alive during destroy() (destroy() may reset this owner re-entrantly)
	object_ptr<value_type> guard(p);

	// 3) destroy
	this->_destroy_own(p);
}

template <class TREF>
template <class T>
constexpr void own_ptr<TREF>::move(own_ptr<T>&& _rhs) noexcept
{
	// 1) take the ownership from _rhs first (self-move safe)
	value_type* pnew = _rhs.detach();

	// 2) old object
	value_type* pold = m_ref;

	// 3) destroy old object (if it is not the same object)
	if (pold != nullptr && pold != pnew)
		this->_destroy_own(pold);

	// 4) set new
	m_ref = pnew;

	// 5) release old object
	if (pold != nullptr)
		static_cast<Ireferenceable*>(pold)->release();
}

template <class TREF>
constexpr void own_ptr<TREF>::_destroy_own(value_type* _ref) noexcept
{
	// 1) call destroy() or stop()
	if constexpr (std::is_base_of_v<object::Idestroyable, value_type>)
	{
		static_cast<object::Idestroyable*>(_ref)->destroy();
	}
	else if constexpr (std::is_base_of_v<object::Istartable, value_type>)
	{
		static_cast<object::Istartable*>(_ref)->stop();
	}
	else
	{
		if (auto* pdestroyable = dynamic_cast<object::Idestroyable*>(_ref); pdestroyable != nullptr)
		{
			pdestroyable->destroy();
		}
		else if (auto* pstartable = dynamic_cast<object::Istartable*>(_ref); pstartable != nullptr)
		{
			pstartable->stop();
		}
	}

	// 2) detach messageables
	if constexpr (std::is_base_of_v<Imessage_transmitter, value_type>)
	{
		static_cast<Imessage_transmitter*>(_ref)->reset_message_transmitter();
	}
	else
	{
		if (auto* ptransmitter = dynamic_cast<Imessage_transmitter*>(_ref); ptransmitter != nullptr)
		{
			ptransmitter->reset_message_transmitter();
		}
	}

	// 3) Ischedulable
	// 4) Igroupable
	// 5) Iobservable
}

template <class TREF>
constexpr void own_ptr<TREF>::_reset_own() noexcept
{
	// 1) get old
	value_type* ref_old = m_ref;

	// check) nothing to do if empty
	if (ref_old == nullptr)
		return;

	// 2) clear
	m_ref = nullptr;

	// 3) destroy
	this->_destroy_own(ref_old);

	// 4) release
	static_cast<Ireferenceable*>(ref_old)->release();
}

template <class TREF>
constexpr void own_ptr<TREF>::_reset_own(value_type* _ref) noexcept
{
	// 1) get old
	value_type* ref_old = m_ref;

	// check) same object
	if (_ref == ref_old)
		return;

	// 2) clear
	m_ref = nullptr;

	// 3) destroy old
	if (ref_old != nullptr)
		this->_destroy_own(ref_old);

	// 4) set new
	m_ref = _ref;

	// 5) add_ref new
	if (_ref != nullptr)
		static_cast<Ireferenceable*>(_ref)->add_ref();

	// 6) release old
	if (ref_old != nullptr)
		static_cast<Ireferenceable*>(ref_old)->release();
}

//-----------------------------------------------------------------------------
// object_ptr <- own_ptr
//-----------------------------------------------------------------------------
template <class TREF>
template <class T>
constexpr typename object_ptr<TREF>::_mytype& object_ptr<TREF>::operator = (own_ptr<T>&& _rhs) noexcept
{
	// 1) copy (keep a reference)
	this->_reset(_rhs.get());

	// 2) destroy the owned object
	_rhs.reset();

	// return)
	return *this;
}

template <class TREF>
constexpr typename object_ptr<TREF>::_mytype& object_ptr<TREF>::operator = (const own_ptr<TREF>& _rhs) noexcept
{
	this->_reset(_rhs.get());

	return *this;
}

template <class TREF>
template <class T>
constexpr typename object_ptr<TREF>::_mytype& object_ptr<TREF>::operator = (const own_ptr<T>& _rhs) noexcept
{
	this->_reset(_rhs.get());

	return *this;
}

//-----------------------------------------------------------------------------
// any_ptr <- own_ptr
//-----------------------------------------------------------------------------
template <class T>
inline std::enable_if_t<std::is_base_of_v<Ireferenceable, T>, any_ptr&>
any_ptr::operator = (own_ptr<T>&& _rhs) noexcept
{
	// 1) copy (keep a reference)
	this->_reset(_rhs.get());

	// 2) destroy the owned object
	_rhs.reset();

	return *this;
}

template <class T>
inline std::enable_if_t<std::is_base_of_v<Ireferenceable, T>, any_ptr&>
any_ptr::operator = (const own_ptr<T>& _rhs) noexcept
{
	this->_reset(_rhs.get());

	return *this;
}


}
