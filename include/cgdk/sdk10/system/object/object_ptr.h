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
	template <class TREF> class own_ptr;

//-----------------------------------------------------------------------------
/**

 @brief		adopt_ref : take over a reference that is already counted
			(no add_ref() is performed, the pointer must own one reference)

*/
//-----------------------------------------------------------------------------
struct adopt_ref_t { explicit constexpr adopt_ref_t() noexcept = default; };
inline constexpr adopt_ref_t adopt_ref {};

//-----------------------------------------------------------------------------
/**

 @class		object_ptr

*/
//-----------------------------------------------------------------------------
template <class TREF>
class object_ptr
{
// definitions)
public:
			using value_type = typename _CGTRAITS_PTR<TREF>::value_type;
			using _mytype = object_ptr<TREF>;

private:
		#ifdef _USE_CHECK_NULLPTR
			void CHECK_NULLPTR() const
			{
				CGASSERT_ERROR(this->m_ref != nullptr);

				if (this->m_ref == nullptr)
					throw std::invalid_argument("object_ptr: dereferenced nullptr");
			}
		#else
			void CHECK_NULLPTR() const
			{
				CGASSERT_ERROR(this->m_ref != nullptr);
			}
		#endif

// constructor/destructor)
public:
	constexpr object_ptr() noexcept : m_ref(nullptr) {}
	constexpr object_ptr(std::nullptr_t) noexcept : m_ref(nullptr) {}
			template <class T>
	constexpr object_ptr(T* _ref) noexcept : m_ref(_ref) { if (m_ref != nullptr) m_ref->add_ref(); }
	constexpr object_ptr(value_type* _ref, adopt_ref_t) noexcept : m_ref(_ref) {}
	constexpr object_ptr(const object_ptr& _ref) noexcept : m_ref(_ref.m_ref) { if (m_ref != nullptr) m_ref->add_ref(); }
			template <class T>
	constexpr object_ptr(const object_ptr<T>& _ref) noexcept : m_ref(_ref.get()) { if (m_ref != nullptr) m_ref->add_ref(); }
	constexpr object_ptr(object_ptr&& _ref) noexcept : m_ref(_ref.m_ref) { _ref.m_ref = nullptr; }
			template <class T>
	constexpr object_ptr(object_ptr<T>&& _ref) noexcept : m_ref(_ref.detach()) {}
			template <class T>
	constexpr object_ptr(own_ptr<T>&&) noexcept = delete;
			template <class T>
	constexpr object_ptr(const own_ptr<T>& _ref) noexcept : m_ref(_ref.get()) { if (m_ref != nullptr) m_ref->add_ref(); }
	constexpr ~object_ptr() noexcept { if (m_ref != nullptr) m_ref->release(); }

// public)
public:
	// 1) basic operating functions
	constexpr void				reset() noexcept { this->_reset(); }
	constexpr void				reset(std::nullptr_t) noexcept { this->_reset(); }
			template <class T>
	constexpr void				reset(T* _ref) noexcept { this->_reset(_ref); }
			template <class T>
	constexpr void				reset(const object_ptr<T>& _ref) noexcept { this->_reset(_ref.get()); }
			template <class T>
	constexpr void				reset(object_ptr<T>&& _ref) noexcept { this->_adopt(_ref.detach()); }
	constexpr void				swap(object_ptr& _ref) noexcept { value_type* temp = m_ref; m_ref = _ref.m_ref; _ref.m_ref = temp; }
	//! @brief give up the reference without release() and return it (the caller owns the reference)
	constexpr value_type*		detach() noexcept { value_type* p = m_ref; m_ref = nullptr; return p; }

			template <class T>
	constexpr void				move(object_ptr<T>& _ref) noexcept { this->_adopt(_ref.detach()); }
			template <class T>
	constexpr void				move(object_ptr<T>&& _ref) noexcept { this->_adopt(_ref.detach()); }

	[[nodiscard]] constexpr value_type* get() const noexcept { return m_ref; }
	[[nodiscard]] constexpr value_type** get_ptr() noexcept { return &m_ref; }
	[[nodiscard]] constexpr value_type* const* get_ptr() const noexcept { return &m_ref; }

	[[nodiscard]] constexpr bool exist() const noexcept { return m_ref != nullptr; }
	[[nodiscard]] constexpr bool empty() const noexcept { return m_ref == nullptr; }

	// 2) operator- Assignment-A
	constexpr _mytype&			operator=(std::nullptr_t) noexcept { this->_reset(); return *this; }
	constexpr _mytype&			operator=(value_type* _rhs) noexcept { this->_reset(_rhs); return *this; }
			template <class T>
	constexpr _mytype&			operator=(T* _rhs) noexcept { this->_reset(_rhs); return *this; }
	constexpr _mytype&			operator=(const _mytype& _rhs) noexcept { this->_reset(_rhs.m_ref); return *this; }
	constexpr _mytype&			operator=(_mytype&& _rhs) noexcept { this->_adopt(_rhs.detach()); return *this; }
	constexpr _mytype&			operator=(const own_ptr<TREF>&) noexcept;
	constexpr _mytype&			operator=(own_ptr<TREF>&&) = delete;

	// 3) operator- Assignment-B
			template <class T>
	constexpr _mytype&			operator=(const object_ptr<T>& _rhs) noexcept { this->_reset(_rhs.get()); return *this; }
			template <class T>
	constexpr _mytype&			operator=(object_ptr<T>&& _rhs) noexcept { this->_adopt(_rhs.detach()); return *this; }
			template <class T>
	constexpr _mytype&			operator=(const own_ptr<T>& _rhs) noexcept;
			template <class T>
	constexpr _mytype&			operator=(own_ptr<T>&& _rhs) noexcept;

	// 4) operator-
	[[nodiscard]] constexpr value_type* operator->() const { CHECK_NULLPTR(); return m_ref; }
	[[nodiscard]] constexpr value_type&	operator*() const { CHECK_NULLPTR(); return *m_ref; }
	[[nodiscard]] constexpr value_type& operator[](size_t _index) const { CHECK_NULLPTR(); return m_ref[_index]; }

	// 5) operator- Conversion
	[[nodiscard]] constexpr		operator value_type*() const noexcept { return m_ref; }
	[[nodiscard]] constexpr		operator bool() const noexcept { return m_ref != nullptr; }

// implementation)
public:
	constexpr void				_clear() noexcept { m_ref = nullptr; }
private:
	constexpr void				_reset() noexcept
			{
				value_type* ref_old = m_ref;

				if (ref_old == nullptr)
					return;

				m_ref = nullptr;

				ref_old->release();
			}
	constexpr void				_reset(value_type* _ref) noexcept
			{
				value_type* ref_old = m_ref;

				if (_ref == ref_old)
					return;

				m_ref = _ref;

				if (_ref != nullptr)
					_ref->add_ref();

				if (ref_old != nullptr)
					ref_old->release();
			}
	// - take over an already counted reference (the source must be detached beforehand -> self-move safe)
	constexpr void				_adopt(value_type* _ref) noexcept
			{
				value_type* ref_old = m_ref;

				m_ref = _ref;

				if (ref_old != nullptr)
					ref_old->release();
			}

// members)
private:
			value_type*			m_ref;

			template <class> friend class object_ptr;
			template <class> friend class own_ptr;
};

//-----------------------------------------------------------------------------
// comparison (C++20 : "!=", "<", "<=", ">", ">=" and the reversed forms are synthesized)
//-----------------------------------------------------------------------------
template <class TONE, class TOTHER>	[[nodiscard]] constexpr bool operator==(const object_ptr<TONE>& _lhs, const object_ptr<TOTHER>& _rhs) noexcept { return _lhs.get() == _rhs.get(); }
template <class TONE, class TOTHER>	[[nodiscard]] constexpr std::strong_ordering operator<=>(const object_ptr<TONE>& _lhs, const object_ptr<TOTHER>& _rhs) noexcept { return std::compare_three_way{}(_lhs.get(), _rhs.get()); }

template <class TONE, class TOTHER>	[[nodiscard]] constexpr bool operator==(const object_ptr<TONE>& _lhs, const own_ptr<TOTHER>& _rhs) noexcept { return _lhs.get() == _rhs.get(); }
template <class TONE, class TOTHER>	[[nodiscard]] constexpr std::strong_ordering operator<=>(const object_ptr<TONE>& _lhs, const own_ptr<TOTHER>& _rhs) noexcept { return std::compare_three_way{}(_lhs.get(), _rhs.get()); }

template <class TONE, class TPTR>	[[nodiscard]] constexpr bool operator==(const object_ptr<TONE>& _lhs, TPTR* _rhs) noexcept { return _lhs.get() == _rhs; }
template <class TONE, class TPTR>	[[nodiscard]] constexpr std::strong_ordering operator<=>(const object_ptr<TONE>& _lhs, TPTR* _rhs) noexcept { return std::compare_three_way{}(_lhs.get(), _rhs); }

template <class TONE>				[[nodiscard]] constexpr bool operator==(const object_ptr<TONE>& _lhs, std::nullptr_t) noexcept { return _lhs.get() == nullptr; }

//-----------------------------------------------------------------------------
// swap
//-----------------------------------------------------------------------------
template <class T>
constexpr void swap(object_ptr<T>& _lhs, object_ptr<T>& _rhs) noexcept
{
	_lhs.swap(_rhs);
}

template <class T>
constexpr void swap_object(object_ptr<T>& _lhs, object_ptr<T>& _rhs) noexcept
{
	_lhs.swap(_rhs);
}

//-----------------------------------------------------------------------------
// pointer casting
//  - rvalue object_ptr : the reference is moved into the result (no add_ref/release)
//  - rvalue own_ptr    : deleted (an owning pointer can not be moved into a non-owning one)
//-----------------------------------------------------------------------------
template <class TTARGET, class TSOURCE>
[[nodiscard]] constexpr object_ptr<TTARGET> static_pointer_cast(const object_ptr<TSOURCE>& _source) noexcept
{
	return object_ptr<TTARGET>(static_cast<typename object_ptr<TTARGET>::value_type*>(_source.get()));
}

template <class TTARGET, class TSOURCE>
[[nodiscard]] constexpr object_ptr<TTARGET> static_pointer_cast(object_ptr<TSOURCE>&& _source) noexcept
{
	return object_ptr<TTARGET>(static_cast<typename object_ptr<TTARGET>::value_type*>(_source.detach()), adopt_ref);
}

template <class TTARGET, class TSOURCE>
[[nodiscard]] constexpr object_ptr<TTARGET> static_pointer_cast(const own_ptr<TSOURCE>& _source) noexcept
{
	return object_ptr<TTARGET>(static_cast<typename object_ptr<TTARGET>::value_type*>(_source.get()));
}

template <class TTARGET, class TSOURCE>
object_ptr<TTARGET> static_pointer_cast(own_ptr<TSOURCE>&&) = delete;

template <class TTARGET, class TSOURCE>
[[nodiscard]] constexpr object_ptr<TTARGET> const_pointer_cast(const object_ptr<TSOURCE>& _source) noexcept
{
	return object_ptr<TTARGET>(const_cast<typename object_ptr<TTARGET>::value_type*>(_source.get()));
}

template <class TTARGET, class TSOURCE>
[[nodiscard]] constexpr object_ptr<TTARGET> const_pointer_cast(object_ptr<TSOURCE>&& _source) noexcept
{
	return object_ptr<TTARGET>(const_cast<typename object_ptr<TTARGET>::value_type*>(_source.detach()), adopt_ref);
}

template <class TTARGET, class TSOURCE>
[[nodiscard]] constexpr object_ptr<TTARGET> const_pointer_cast(const own_ptr<TSOURCE>& _source) noexcept
{
	return object_ptr<TTARGET>(const_cast<typename object_ptr<TTARGET>::value_type*>(_source.get()));
}

template <class TTARGET, class TSOURCE>
object_ptr<TTARGET> const_pointer_cast(own_ptr<TSOURCE>&&) = delete;

template <class TTARGET, class TSOURCE>
[[nodiscard]] constexpr object_ptr<TTARGET> dynamic_pointer_cast(const object_ptr<TSOURCE>& _source) noexcept
{
	return object_ptr<TTARGET>(dynamic_cast<typename object_ptr<TTARGET>::value_type*>(_source.get()));
}

template <class TTARGET, class TSOURCE>
[[nodiscard]] constexpr object_ptr<TTARGET> dynamic_pointer_cast(object_ptr<TSOURCE>&& _source) noexcept
{
	// 1) cast
	auto* p = dynamic_cast<typename object_ptr<TTARGET>::value_type*>(_source.get());

	// check) failed : the source keeps its reference
	if (p == nullptr)
		return object_ptr<TTARGET>();

	// 2) move the reference into the result
	_source.detach();

	// return)
	return object_ptr<TTARGET>(p, adopt_ref);
}

template <class TTARGET, class TSOURCE>
[[nodiscard]] constexpr object_ptr<TTARGET> dynamic_pointer_cast(const own_ptr<TSOURCE>& _source) noexcept
{
	return object_ptr<TTARGET>(dynamic_cast<typename object_ptr<TTARGET>::value_type*>(_source.get()));
}

template <class TTARGET, class TSOURCE>
object_ptr<TTARGET> dynamic_pointer_cast(own_ptr<TSOURCE>&&) = delete;

//-----------------------------------------------------------------------------
// hash
//-----------------------------------------------------------------------------
template <class TTYPE>
struct hash_object_ptr
{
	[[nodiscard]] size_t operator()(const object_ptr<TTYPE>& _rhs) const noexcept
	{
		return reinterpret_cast<size_t>(_rhs.get());
	}
};

template <class TTYPE>
struct hash_own_ptr
{
	[[nodiscard]] size_t operator()(const own_ptr<TTYPE>& _rhs) const noexcept
	{
		return reinterpret_cast<size_t>(_rhs.get());
	}
};

}

namespace std
{
	template<class TKEY>
	struct hash<CGDK::object_ptr<TKEY>>
	{
		using argument_type = TKEY*;
		using result_type = size_t;

		[[nodiscard]] size_t operator()(const CGDK::object_ptr<TKEY>& _keyval) const noexcept
		{
			return reinterpret_cast<size_t>(_keyval.get());
		}
	};

	template<class TKEY>
	struct hash<CGDK::own_ptr<TKEY>>
	{
		using argument_type = TKEY*;
		using result_type = size_t;

		[[nodiscard]] size_t operator()(const CGDK::own_ptr<TKEY>& _keyval) const noexcept
		{
			return reinterpret_cast<size_t>(_keyval.get());
		}
	};
}
