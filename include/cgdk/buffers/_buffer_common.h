//*****************************************************************************
//*                                                                           *
//*                               CGDK::buffer                                *
//*                       ver 3.03 / release 2023.10.17                       *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  programmed by Cho SangHyun. sangduck@cgcii.com                           *
//*                                                                           *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************

#pragma once
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
// 1) C++ Standard Libraries and Common Includes
#include <cassert>
#include <stdexcept>
#include <type_traits>

// 2) memory
#include <memory>
#include <tuple>
#if !defined(_MSC_VER)
	#include <list>
#endif


//-----------------------------------------------------------------------------
//
// compile options
//
//  1) #define CGDK_NO_BOUND_CHECK
//     disable bound check, no assert, no throw exception on buffer overflow
// 
//  2) #define CGDK_DISABLE_ASSERT
//     enable bound check, no assert, throw exceptions on buffer overflow
// 
//  3) default
//     enable bound check, assert and then throw exceptions on buffer overflow
// 
//-----------------------------------------------------------------------------
//#if defined(NDEBUG) || !defined(_DEBUG)
//	#define CGDK_DISABLE_ASSERT
//#endif

#if defined(CGDK_NO_BOUND_CHECK)
	#define _CGD_BUFFER_BOUND_CHECK(condition)	
#elif defined(CGDK_DISABLE_ASSERT)
	#define _CGD_BUFFER_BOUND_CHECK(condition)	if((condition) == false) { throw std::overflow_error("CGDK::shared_buffer out of memory bounding");}
#else
	#define _CGD_BUFFER_BOUND_CHECK(condition)	if((condition) == false) { CGDK_ASSERT_ON_BOUND; throw std::overflow_error("CGDK::shared_buffer out of memory bounding");}
#endif


//-----------------------------------------------------------------------------
// preprocess
//-----------------------------------------------------------------------------
// 1) C++ Standard Template Libraries
namespace std
{
	template<class, size_t> class array;

	template<class,class> class vector;
	template<class,class> class deque;
#if defined(_MSC_VER)
	template<class,class> class list;
#endif
	template<class,class> class deque;

	template<class,class,class> class set;
	template<class,class,class> class multiset;

	template<class,class,class,class> class map;
	template<class,class,class,class> class multimap;

	template<class,class,class,class> class hash_map;
	template<class,class,class,class> class hash_multimap;

	template<class,class,class,class> class unordered_set;
	template<class,class,class,class> class unordered_multiset;

	template<class,class,class,class,class> class unordered_map;
	template<class,class,class,class,class> class unordered_multimap;

#if defined(__cpp_lib_span)
	template<class,size_t> class span;
#endif
}

// 2) CGDK classes
namespace CGDK
{
	template<class,std::size_t> class static_vector;
	template<class,class> class circular_list;
#if defined(__CGDK_STATIC_STRING)
	template<class, std::size_t> class basic_static_string;
#endif

	template<class> class object_ptr;
	template<class> class own_ptr;

	class Ibuffer_serializable;
	template<class=char> class _buffer_view;
	using buffer_view = _buffer_view<char>;
	using const_buffer_view = _buffer_view<const char>;
	template<class = char> class _basic_buffer;
	using buffer = _basic_buffer<char>;

	template<class> class _shared_buffer;
	using shared_buffer = _shared_buffer<buffer>;

	struct buffer_bound
	{
		const void* lower = nullptr;
		const void* upper = nullptr;
		constexpr void reset() noexcept { lower = upper = nullptr; }
	};
}

//-----------------------------------------------------------------------------
//
// configure
//
//-----------------------------------------------------------------------------
#ifndef	COUNT_T
	using COUNT_T = int32_t;
#endif

constexpr std::size_t	_XX_MAX_STRING_SIZE = 65536;
inline std::size_t _buffer_string_size_saturate(std::size_t _a) { return (_a < _XX_MAX_STRING_SIZE) ? _a : _XX_MAX_STRING_SIZE; }


//-----------------------------------------------------------------------------
//
// General Definitions
//
//-----------------------------------------------------------------------------
// 1) CGDK Assert
#if !defined(_DEBUG) && defined(NDEBUG)
	#define	CGDK_ASSERT(condition, ...)	
	#define	CGDK_ASSERT_ERROR
#else
	#define	CGDK_ASSERT(condition, ...)		if ((condition) == false) { assert(condition); __VA_ARGS__;}
	#define	CGDK_ASSERT_ERROR				/*assert(false);*/
#endif

// 2) ASSERT on bound check
#if defined(_CGDK_NO_ASSERT_ON_BOUND_CHECK) || defined(NDEBUG)
	#define	CGDK_ASSERT_ON_BOUND
#else
	#define	CGDK_ASSERT_ON_BOUND			assert(false)
#endif

constexpr char __hex_to_ascii(uint8_t _data) noexcept { return (_data < 10) ? (_data + '0') : (_data + ('a' - 10)); }


namespace CGDK
{
//-----------------------------------------------------------------------------
// definitions for append(<<) and Extract(>>)
//-----------------------------------------------------------------------------
// 1) Skip
class skip
{
public:
	constexpr skip(int64_t _amount) noexcept : amount(_amount) {}
public:
	int64_t	amount;
};

// 2) offset
class offset : public skip
{
public:
	constexpr offset() noexcept : skip(0) {}
	constexpr offset(const offset& _offset) noexcept : skip(_offset.amount) {}
	constexpr offset(int64_t _offset) noexcept : skip(_offset) {}

public:
	constexpr bool	operator==(const offset& _rhs) const noexcept { return this->amount == _rhs.amount; }
	constexpr bool	operator!=(const offset& _rhs) const noexcept { return this->amount != _rhs.amount; }
	constexpr bool	operator>=(const offset& _rhs) const noexcept { return this->amount >= _rhs.amount; }
	constexpr bool	operator> (const offset& _rhs) const noexcept { return this->amount >  _rhs.amount; }
	constexpr bool	operator<=(const offset& _rhs) const noexcept { return this->amount <= _rhs.amount; }
	constexpr bool	operator< (const offset& _rhs) const noexcept { return this->amount <  _rhs.amount; }

	constexpr bool	operator==(int64_t _rhs) const noexcept { return this->amount == _rhs; }
	constexpr bool	operator!=(int64_t _rhs) const noexcept { return this->amount != _rhs; }
	constexpr bool	operator>=(int64_t _rhs) const noexcept { return this->amount >= _rhs; }
	constexpr bool	operator> (int64_t _rhs) const noexcept { return this->amount > _rhs; }
	constexpr bool	operator<=(int64_t _rhs) const noexcept { return this->amount <= _rhs; }
	constexpr bool	operator< (int64_t _rhs) const noexcept { return this->amount < _rhs; }

	constexpr offset& operator+=(const offset& _rhs) noexcept { this->amount += _rhs.amount; return *this; }
	constexpr offset& operator+=(int64_t _rhs) noexcept { amount += _rhs; return *this; }
	constexpr offset& operator-=(const offset& _rhs) noexcept { this->amount -= _rhs.amount; return *this; }
	constexpr offset& operator-=(int64_t _rhs) noexcept { amount -= _rhs; return *this; }
	constexpr offset& operator*=(const offset& _rhs) noexcept { this->amount *= _rhs.amount; return *this; }
	constexpr offset& operator*=(int64_t _rhs) noexcept { amount *= _rhs; return *this; }
	constexpr offset& operator/=(const offset& _rhs) noexcept { this->amount /= _rhs.amount; return *this; }
	constexpr offset& operator/=(int64_t _rhs) noexcept { amount /= _rhs; return *this; }

	constexpr operator int64_t& () { return this->amount; }
	constexpr operator const int64_t& () const { return this->amount; }
};



// 3) size
class size : public skip
{
public:
	constexpr size(std::size_t _amount) noexcept : skip(_amount) {}
};

// 4)
template<class T>
concept iterator_type = std::is_pointer_v<T> || requires
{
	typename T::iterator_category;
	typename T::value_type;
	typename T::difference_type;
	typename T::pointer;
	typename T::reference;
};

template<class T>	struct is_iterator : std::bool_constant<iterator_type<T>> {};

template<class I, class TENABLE = void>
class _ITERATOR
{
public:
	_ITERATOR(const I& _iter_begin, const I& _iter_end) noexcept : iterFirst(_iter_begin), iterLast(_iter_end) {}
public:
	I		iterFirst;
	I		iterLast;
};

template<class I>
_ITERATOR<I,void> ITERATOR(const I& _iter_begin, const I& _iter_end) requires(is_iterator<I>::value)
{
	return _ITERATOR<I, void>(_iter_begin, _iter_end);
}


//-----------------------------------------------------------------------------
// text<T>
//-----------------------------------------------------------------------------
// 3) Web String
struct WEB {};

// 3) Web String
struct WEB_MODIFY {};

// 4) Text
template <class T>
struct text
{
public:
	using	value_type = T;

public:
	constexpr text() noexcept {}
	constexpr text(const text& _rhs) noexcept : p(_rhs.p) {}
	constexpr text(std::basic_string_view<T> _str) noexcept : p(_str) {}
	constexpr text(const std::basic_string<T>& _str) noexcept : p(_str) {}
#if defined(__CGDK_STATIC_STRING)
	template <std::size_t N>
	constexpr text(CGDK::static_string<N> _str) noexcept : p(_str) {}
#endif
	template <std::size_t N>
	constexpr text(const T(&_str)[N]) noexcept : p{ _str, N } {}
	constexpr text(const T* _str) noexcept : p(_str) {}

public:
	constexpr std::size_t	bytes() const noexcept { return p.size() * sizeof(T); }

public:
	constexpr text&	operator=(const text& _rhs) noexcept { p = _rhs.p; return *this; }
	constexpr text&	operator=(const std::basic_string_view<T> _rhs) noexcept { p = _rhs; return *this; }

	constexpr bool	operator==(const text& _rhs) const noexcept						{ return p == _rhs.p; }
	constexpr bool	operator==(const std::basic_string_view<T>& _rhs) const noexcept{ return p == _rhs.p; }
	constexpr bool	operator!=(const text& _rhs) const noexcept						{ return p != _rhs.p; }
	constexpr bool	operator!=(const std::basic_string_view<T>& _rhs) const noexcept{ return p != _rhs.p; }
	constexpr bool	operator>=(const text& _rhs) const noexcept						{ return p >= _rhs.p; }
	constexpr bool	operator>=(const std::basic_string_view<T>& _rhs) const noexcept{ return p >= _rhs.p; }
	constexpr bool	operator> (const text& _rhs) const noexcept						{ return p >  _rhs.p; }
	constexpr bool	operator> (const std::basic_string_view<T>& _rhs) const noexcept{ return p >  _rhs.p; }
	constexpr bool	operator<=(const text& _rhs) const noexcept						{ return p <= _rhs.p; }
	constexpr bool	operator<=(const std::basic_string_view<T>& _rhs) const noexcept{ return p <= _rhs.p; }
	constexpr bool	operator< (const text& _rhs) const noexcept						{ return p <  _rhs.p; }
	constexpr bool	operator< (const std::basic_string_view<T>& _rhs) const noexcept{ return p <  _rhs.p; }
	constexpr 		operator std::basic_string_view<T>() const noexcept				{ return p; }
public:
	std::basic_string_view<T>	p;
};

template <class T>
constexpr text<T> _text(std::basic_string_view<T> _str) noexcept
{
	return text<T>(_str);
}

template <class T>
constexpr text<T> _text(const T* _str) noexcept
{
	return text<T>(_str);
}


//using TEXT_CHAR = text<char>;
//using TEXT_WCHAR = text<wchar_t>;
//using TEXT_WEB = TEXT_CHAR;

inline text<char>		operator "" _text(const char* _text, std::size_t _size) noexcept { return text<char>(std::string_view(_text, _size));}
inline text<wchar_t>	operator "" _text(const wchar_t* _text, std::size_t _size) noexcept { return text<wchar_t>(std::wstring_view(_text, _size));}
inline text<char8_t> 	operator "" _text(const char8_t* _text, std::size_t _size) noexcept { return text<char8_t>(std::u8string_view(_text, _size)); }


//-----------------------------------------------------------------------------
// Ibuffer_serializable
//-----------------------------------------------------------------------------
#define	DEFINE_BUFFER_SERIALIZE(...) \
				virtual	void	process_serialize_in(CGDK::buffer_view& _buffer) override { _buffer._extract_multi(__VA_ARGS__); }\
				virtual	void	process_serialize_out(CGDK::buffer& _buffer) override { _buffer._append_multi(__VA_ARGS__); }\
				virtual std::size_t	process_size_of() const override { return CGDK::get_size_of(__VA_ARGS__);}\
				friend class CGDK::_buffer_serialize_access;\
				auto			process_get_type_hash_of() noexcept { return std::type_identity<decltype(CGDK::_buffer_serialize_types_of(__VA_ARGS__))>{}; }

// - for get_type_hash_of<T>() (member types of DEFINE_BUFFER_SERIALIZE)
template <class...> struct _type_list {};
template <class... A> _type_list<std::remove_cvref_t<A>...> _buffer_serialize_types_of(const A&...) noexcept;	// unevaluated only
class _buffer_serialize_access
{
public:
	template <class T> static constexpr bool has_types = requires (T& _t) { _t.process_get_type_hash_of(); };
	template <class T> using types = decltype(std::declval<T&>().process_get_type_hash_of());
};
template <class T, class FLAG = void> class serializer_hash;

// 1) serializable 
class Ibuffer_serializable
{
public:
			void		serialize_in(buffer_view& _buffer) { this->process_serialize_in(_buffer);}
			void		serialize_out(buffer& _buffer) { this->process_serialize_out(_buffer); }
			std::size_t	get_size_of() const { return this->process_size_of(); }
	template <class T>
	static	std::size_t	get_size_of(const T& _source);

protected:
	virtual	void		process_serialize_in(buffer_view& _buffer) = 0;
	virtual	void		process_serialize_out(buffer& _buffer) = 0;
	virtual std::size_t	process_size_of() const = 0;
};

#define ENABLE_STRUCT_SERIALIZABLE	public: using enable_struct_serialize = std::true_type;
#define DISABLE_STRUCT_SERIALIZABLE	public: using enable_struct_serialize = std::false_type;


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
// 1) is tuple
template<class T> struct is_tuple_type : public std::false_type {};
template<class... T> struct is_tuple_type<std::tuple<T...>> : public std::true_type {};
template<class T> struct is_tuple : public is_tuple_type<std::remove_cvref_t<T>> {};
template<class T> constexpr bool is_tuple_v = is_tuple<T>::value;

// 2) is text classes
template<class T> struct is_text_type : public std::false_type {};
template<class A> struct is_text_type<text<A>> : public std::true_type {};
template<class T> struct is_text : public is_text_type<std::remove_cvref_t<T>> {};
template<class T> constexpr bool is_text_v = is_text<T>::value;

// 3) is std::string_view
template<class T> struct is_std_string_view_type : public std::false_type {};
template<class A, class B> struct is_std_string_view_type<std::basic_string_view<A, B>> : public std::true_type {};
template <class T> struct is_std_string_view : public is_std_string_view_type<std::remove_cvref_t<T>> {};
template <class T> constexpr bool is_std_string_view_v = is_std_string_view<T>::value;

// 4) is std::string
template<class T> struct is_std_string_type : public std::false_type {};
template<class A, class B, class C> struct is_std_string_type<std::basic_string<A, B, C>> : public std::true_type {};
template <class T> struct is_std_string : public is_std_string_type<std::remove_cvref_t<T>> {};
template <class T> constexpr bool is_std_string_v = is_std_string<T>::value;

// 4) is std::static_string
#if defined(__CGDK_STATIC_STRING)
template<class T> struct is_static_string_type : public std::false_type {};
template<class A, std::size_t N> struct is_static_string_type<basic_static_string<A, N>> : public std::true_type {};
template<class T> struct is_static_string : public is_static_string_type<std::remove_cvref_t<T>> {};
template <class T> constexpr bool is_static_string_v = is_static_string<T>::value;
#endif

// 5) object_ptr<T>
template<class T> struct is_object_ptr_type : public std::false_type {};
template<class A> struct is_object_ptr_type<object_ptr<A>> : public std::true_type {};
template<class T> struct is_object_ptr : public is_object_ptr_type<std::remove_cvref_t<T>> {};
template <class T> constexpr bool is_object_ptr_v = is_object_ptr<T>::value;

// 6) own_ptr<T>
template<class T> struct is_own_ptr_type : public std::false_type {};
template<class A> struct is_own_ptr_type<own_ptr<A>> : public std::true_type {};
template<class T> struct is_own_ptr : public is_own_ptr_type<std::remove_cvref_t<T>> {};
template <class T> constexpr bool is_own_ptr_v = is_own_ptr<T>::value;

// 7) is struct serializable
template <class T, class ENABLE = void> struct is_struct_serializable_type : public std::false_type {};
template <class T> struct is_struct_serializable_type<T, std::void_t<typename std::remove_cvref_t<T>::enable_struct_serialize>>
	: public std::bool_constant<std::remove_cvref_t<T>::enable_struct_serialize::value
							 && !std::is_base_of_v<Ibuffer_serializable, std::remove_cvref_t<T>>> {};
template <class T> struct is_struct_serializable : public is_struct_serializable_type<std::remove_cvref_t<T>> {};
template <class T> constexpr bool is_struct_serializable_v = is_struct_serializable<T>::value;

// 8) memcpy_able
template <class T> struct is_memcopyable
	: public std::bool_constant<std::is_trivially_copyable_v<std::remove_cvref_t<T>>
							 && !std::is_pointer_v<std::remove_cvref_t<T>>
							 && !is_std_string_view_v<T>
							 && !is_std_string_v<T>
							 && !is_text_v<T>
							 && !is_struct_serializable_v<T>> {};
template <class T> struct is_memcopy_able : public is_memcopyable<T> {};
template <class T> constexpr bool is_memcopy_able_v = is_memcopyable<T>::value;

// 10) ...
template<class T> struct is_linear_container_tag : public std::false_type {};
template<class A, class B> struct is_linear_container_tag<std::list<A, B>> : public std::true_type {};
template<class A, class B> struct is_linear_container_tag<std::deque<A, B>> : public std::true_type {};
template<class A, class B> struct is_linear_container_tag<circular_list<A, B>> : public std::true_type {};

template<class T, class ENABLE = void> struct container_value_type {};
template<class T> struct container_value_type<T, std::void_t<typename std::remove_cvref_t<T>::value_type>>
{
	using type = typename std::remove_cvref_t<T>::value_type;
};

template<class T, class ENABLE = void> struct is_linear_container : public std::false_type {};
template<class T> struct is_linear_container<T, std::void_t<typename container_value_type<T>::type>>
	: public std::bool_constant<is_linear_container_tag<std::remove_cvref_t<T>>::value
							 && !is_object_ptr_v<typename container_value_type<T>::type>
							 && !is_own_ptr_v<typename container_value_type<T>::type>> {};

template<class T, class ENABLE = void> struct is_linear_object_ptr_container : public std::false_type {};
template<class T> struct is_linear_object_ptr_container<T, std::void_t<typename container_value_type<T>::type>>
	: public std::bool_constant<is_linear_container_tag<std::remove_cvref_t<T>>::value
							 && is_object_ptr_v<typename container_value_type<T>::type>> {};

template<class T, class ENABLE = void> struct is_linear_own_ptr_container : public std::false_type {};
template<class T> struct is_linear_own_ptr_container<T, std::void_t<typename container_value_type<T>::type>>
	: public std::bool_constant<is_linear_container_tag<std::remove_cvref_t<T>>::value
							 && is_own_ptr_v<typename container_value_type<T>::type>> {};

template<class T> struct is_buffer_type_base : public std::false_type {};
template<class A> struct is_buffer_type_base<_buffer_view<A>> : public std::true_type {};
template<class A> struct is_buffer_type_base<_basic_buffer<A>> : public std::true_type {};
template<class A> struct is_buffer_type_base<_shared_buffer<A>> : public std::true_type {};
template<class T> struct is_buffer_type : public is_buffer_type_base<std::remove_cvref_t<T>> {};
template <class T> constexpr bool is_buffer_type_v = is_buffer_type<T>::value;

template<class T>					struct is_linear_container_with_reserve_tag : public std::false_type {};
template<class A, class B>			struct is_linear_container_with_reserve_tag<std::vector<A, B>> : public std::true_type {};
template<class A, std::size_t B>	struct is_linear_container_with_reserve_tag<static_vector<A, B>> : public std::true_type {};

// linear container<shared_buffer>)
template<class T>					struct is_linear_container_with_buffer_tag
	: public std::bool_constant<is_linear_container_tag<T>::value || is_linear_container_with_reserve_tag<T>::value> {};

template<class T, class ENABLE = void> struct is_linear_container_with_buffer : public std::false_type {};
template<class T> struct is_linear_container_with_buffer<T, std::void_t<typename container_value_type<T>::type>>
	: public std::bool_constant<is_linear_container_with_buffer_tag<std::remove_cvref_t<T>>::value
							 && is_buffer_type_v<typename container_value_type<T>::type>> {};

// linear container<T>
template<class T, class ENABLE = void> struct is_linear_container_with_reserve : public std::false_type {};
template<class T> struct is_linear_container_with_reserve<T, std::void_t<typename container_value_type<T>::type>>
	: public std::bool_constant<is_linear_container_with_reserve_tag<std::remove_cvref_t<T>>::value
							 && !is_object_ptr_v<typename container_value_type<T>::type>
							 && !is_own_ptr_v<typename container_value_type<T>::type>> {};

// linear container<object_ptr<T>>
template<class T, class ENABLE = void> struct is_linear_object_ptr_container_with_reserve : public std::false_type {};
template<class T> struct is_linear_object_ptr_container_with_reserve<T, std::void_t<typename container_value_type<T>::type>>
	: public std::bool_constant<is_linear_container_with_reserve_tag<std::remove_cvref_t<T>>::value
							 && is_object_ptr_v<typename container_value_type<T>::type>> {};

// linear container<own_ptr<T>>
template<class T, class ENABLE = void> struct is_linear_own_ptr_container_with_reserve : public std::false_type {};
template<class T> struct is_linear_own_ptr_container_with_reserve<T, std::void_t<typename container_value_type<T>::type>>
	: public std::bool_constant<is_linear_container_with_reserve_tag<std::remove_cvref_t<T>>::value
							 && is_own_ptr_v<typename container_value_type<T>::type>> {};

// set container<T>)
template<class T> struct is_set_container_tag : public std::false_type {};
template<class A, class B, class C>	struct is_set_container_tag<std::set<A, B, C>> : public std::true_type {};
template<class A, class B, class C>	struct is_set_container_tag<std::multiset<A, B, C>> : public std::true_type {};

template<class T, class ENABLE = void> struct is_set_container : public std::false_type {};
template<class T> struct is_set_container<T, std::void_t<typename container_value_type<T>::type>>
	: public std::bool_constant<is_set_container_tag<std::remove_cvref_t<T>>::value
							 && !is_object_ptr_v<typename container_value_type<T>::type>
							 && !is_own_ptr_v<typename container_value_type<T>::type>> {};

// std::set<object_ptr<T>>, std::multiset<object_ptr<T>>, ...
template<class T, class ENABLE = void> struct is_set_object_ptr_container : public std::false_type {};
template<class T> struct is_set_object_ptr_container<T, std::void_t<typename container_value_type<T>::type>>
	: public std::bool_constant<is_set_container_tag<std::remove_cvref_t<T>>::value
							 && is_object_ptr_v<typename container_value_type<T>::type>> {};

// std::set<own_ptr<T>>, std::multiset<own_ptr<T>>, ...
template<class T, class ENABLE = void> struct is_set_own_ptr_container : public std::false_type {};
template<class T> struct is_set_own_ptr_container<T, std::void_t<typename container_value_type<T>::type>>
	: public std::bool_constant<is_set_container_tag<std::remove_cvref_t<T>>::value
							 && is_own_ptr_v<typename container_value_type<T>::type>> {};

// associated container<T>)
template<class T, class ENABLE = void> struct is_associative_container_tag : public std::false_type {};
template<class T> struct is_associative_container_tag<T, std::void_t<typename T::key_type>>
	: public std::bool_constant<!is_set_container_tag<T>::value> {};

template<class T, class ENABLE = void> struct has_mapped_type : public std::false_type {};
template<class T> struct has_mapped_type<T, std::void_t<typename std::remove_cvref_t<T>::mapped_type>> : public std::true_type {};

template<class T, class ENABLE = void> struct associative_container_serialized_type {};
template<class T> struct associative_container_serialized_type<T, std::enable_if_t<!has_mapped_type<T>::value, std::void_t<typename std::remove_cvref_t<T>::value_type>>>
{
	using type = typename std::remove_cvref_t<T>::value_type;
};
template<class T> struct associative_container_serialized_type<T, std::void_t<typename std::remove_cvref_t<T>::mapped_type>>
{
	using type = typename std::remove_cvref_t<T>::mapped_type;
};

template<class T, class ENABLE = void> struct is_associative_container : public std::false_type {};
template<class T> struct is_associative_container<T, std::void_t<typename associative_container_serialized_type<T>::type>>
	: public std::bool_constant<is_associative_container_tag<std::remove_cvref_t<T>>::value
							 && !is_object_ptr_v<typename associative_container_serialized_type<T>::type>
							 && !is_own_ptr_v<typename associative_container_serialized_type<T>::type>> {};

// associated container<object_ptr<T>>)
template<class T, class ENABLE = void> struct is_associative_object_ptr_container : public std::false_type {};
template<class T> struct is_associative_object_ptr_container<T, std::void_t<typename associative_container_serialized_type<T>::type>>
	: public std::bool_constant<is_associative_container_tag<std::remove_cvref_t<T>>::value
							 && is_object_ptr_v<typename associative_container_serialized_type<T>::type>> {};

// associated container<own_ptr<T>>)
template<class T, class ENABLE = void> struct is_associative_own_ptr_container : public std::false_type {};
template<class T> struct is_associative_own_ptr_container<T, std::void_t<typename associative_container_serialized_type<T>::type>>
	: public std::bool_constant<is_associative_container_tag<std::remove_cvref_t<T>>::value
							 && is_own_ptr_v<typename associative_container_serialized_type<T>::type>> {};


// 11) is basic_type
template <class T> struct is_basic_type
{
	static const bool value	 = /*std::is_trivially_copyable<T>::value*/
							(!std::is_class<T>::value || std::is_trivially_copyable<T>::value || std::is_union<T>::value)
							&& !is_std_string_view<T>::value
							&& !is_std_string<T>::value
							&& !std::is_base_of_v<Ibuffer_serializable, T>
							&& !is_struct_serializable_v<T>
							&& !is_linear_container<T>::value
							&& !is_linear_object_ptr_container<T>::value
							&& !is_linear_own_ptr_container<T>::value
							&& !is_linear_container_with_reserve<T>::value
							&& !is_linear_object_ptr_container_with_reserve<T>::value
							&& !is_linear_own_ptr_container_with_reserve<T>::value
							&& !is_set_container<T>::value
							&& !is_set_object_ptr_container<T>::value
							&& !is_set_own_ptr_container<T>::value
							&& !is_associative_container<T>::value
							&& !is_associative_object_ptr_container<T>::value
							&& !is_associative_own_ptr_container<T>::value
							&& !std::is_pointer<T>::value
							&& !is_buffer_type_v<T>
							&& !is_tuple_v<T>
							&& !is_text_v<T>;
};
template <class T> constexpr bool is_basic_type_v = is_basic_type<T>::value;

// 12) is general serializable
template <class T> struct is_general_serializable
{
#if defined(__CGDK_STATIC_STRING)
	static const bool value	 = is_basic_type_v<T>
							|| is_static_string_v<T>;
#else
	static const bool value = is_basic_type_v<T>;
#endif
};
template <class T> constexpr bool is_general_serializable_v = is_general_serializable<T>::value;



//-----------------------------------------------------------------------------
// string type
//-----------------------------------------------------------------------------
template<class T> struct is_string_type
	: public std::bool_constant<std::is_same_v<std::remove_cv_t<T>, char>
							 || std::is_same_v<std::remove_cv_t<T>, wchar_t>
							 || std::is_same_v<std::remove_cv_t<T>, char8_t>
> {};
template <class T> constexpr bool is_string_type_v = is_string_type<T>::value;

template<class T> struct is_string_pointer_type
	: public std::bool_constant<std::is_pointer_v<std::remove_cv_t<T>> && is_string_type_v<std::remove_pointer_t<std::remove_cv_t<T>>>> {};
template <class T> constexpr bool is_string_pointer_type_v = is_string_pointer_type<T>::value;


//-----------------------------------------------------------------------------
// base
//-----------------------------------------------------------------------------
template<class B, class T, class FLAG = void> class serializer_append
							{	using TX = std::remove_const_t<T>;
								public: using type = TX&;
								template<class S> constexpr static type _do_append(S& _s, const TX& _data) { return _s.template _append_general<TX> (_data); }
							};
template<class B, class T, class FLAG = void> class serializer_extract
							{	using T_BASE = std::remove_const_t<T>;
								using TX = typename _buffer_return<typename B::traits, T_BASE>::type;
								public: using type = TX&;
								public: using const_type = const TX&;
								template<class S> constexpr static type _do_extract(S& _s) { return _s.template _extract_general<TX>(); }
								template<class D, class S> constexpr static void _do_extract(D& _dest, S& _s) { _s.template _extract_general<T_BASE>(_dest); }
							};
template<class B, class T, class FLAG = void> class serializer_peek
							{	using T_BASE = std::remove_const_t<T>;
								using TX = typename _buffer_return<typename B::traits, T_BASE>::type;
								public: using type = TX&;
								public: using const_type = const TX&;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset) { return _s.template _front_general<TX> (_offset); }
								template<class D, class S> constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset) { return _s.template _front_general<T_BASE> (_dest, _offset); }
							};
template<class T, class FLAG = void> class serializer_size_of
							{	public:
								constexpr static std::size_t  _get_append_size(const T&) { return sizeof(T);}
								template<class S>
								constexpr static std::size_t  _get_extract_size(const S& _buffer, int64_t& _offset) { auto size = sizeof(T); if((_buffer.size() - _offset) < size) throw std::exception(); return size;}
							};

template<class B, class T> using APPD_t	 = serializer_append<B, std::remove_reference_t<std::remove_const_t<T>>>;
template<class B, class T> using APPD_tr = typename APPD_t<B, T>::type;


template<class B, class T> using EXTR_t	 = serializer_extract<B, std::remove_reference_t<std::remove_const_t<T>>>;
template<class B, class T> using EXTR_tr = typename EXTR_t<B, T>::type;

template<class B, class T> using PEEK_t	 = serializer_peek<B, std::remove_reference_t<std::remove_const_t<T>>>;
template<class B, class T> using PEEK_tr = typename PEEK_t<B, T>::type;

template<class T> using SIZE_OF_t = serializer_size_of<std::remove_reference_t<std::remove_const_t<T>>>;


//-----------------------------------------------------------------------------
//   get_size_of function
//-----------------------------------------------------------------------------
template<class TRETURN = std::size_t, class T>
constexpr TRETURN get_size_of(const T& _object)
{
	return static_cast<TRETURN>(SIZE_OF_t<T>::_get_append_size(_object));
}

template<class TRETURN = std::size_t, class TFIRST, class TSECOND, class... TREST>
constexpr TRETURN get_size_of(const TFIRST& _first, const TSECOND& _second, const TREST&... _rest)
{
	return static_cast<TRETURN>(SIZE_OF_t<TFIRST>::_get_append_size(_first) + get_size_of<TRETURN, TSECOND, TREST...>(_second, _rest...));
}

template <class T>
std::size_t Ibuffer_serializable::get_size_of(const T& _source)
{
	return CGDK::get_size_of(_source);
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

// class for append)
template<class B, class T,class E>	class serializer_append<B, _ITERATOR<T,E>>
							{	public:	using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const _ITERATOR<T,E>& _data) { return _s.template _append_iterator(_data.iterFirst, _data.iterLast);}
								template<class S> constexpr static type _do_append(S& _s, _ITERATOR<T,E>&& _data) { return _s.template _append_iterator(_data.iterFirst, _data.iterLast);}
							};
template<class B, class T>			class serializer_append<B, std::initializer_list<T>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const std::initializer_list<TX>& _data){ return _s.template _append_initializer_list(_data);;}
								template<class S> constexpr static type _do_append(S& _s, std::initializer_list<TX>&& _data) { return _s.template _append_initializer_list(_data);}
							};


//-----------------------------------------------------------------------------
// array
//-----------------------------------------------------------------------------
template<class B, class T, std::size_t N> class serializer_append<B, T[N], void>
							{	using TX = std::remove_const_t<T>;
								public:	using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const TX(&_data)[N]) { return _s.template _append_array<TX,N>((TX*)_data);}
								template<class S> constexpr static type _do_append(S& _s, TX(&&_data)[N]) { return _s.template _append_array<TX,N>((TX*)_data);}
							};

// serializer_extract - T[N] & !is_string_type_v<T> && !is_string_pointer_type_v<T>
template<class B, class T, std::size_t N> class serializer_extract<B, T[N], std::enable_if_t<!is_string_type_v<T> && !is_string_pointer_type_v<T>>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = std::array<TX, N>;
								template<class S> constexpr static type _do_extract(S& _s) { return _s.template _extract_array<TX,N>();}
								template<class D, class S> constexpr static void _do_extract(D& _dest, S& _s) { _s.template _extract_array<TX, N>(_dest); }
							};

template<class B, class T, std::size_t N> class serializer_peek<B, T[N], std::enable_if_t<!is_string_type_v<T> && !is_string_pointer_type_v<T>>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = std::array<TX, N>;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset) { S tb =_s + _offset; auto temp = tb.template _extract_array<TX,N>(); _offset = tb.data() - _s.data(); return temp;}
								template<class D, class S> constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset) { S tb =_s + _offset; tb.template _extract_array<D,TX,N>(_dest); _offset = tb.data() - _s.data();}
							};

// serializer_extract - T[N] & is_string_type_v<T>
template<class B, class T, std::size_t N> class serializer_extract<B, T[N], std::enable_if_t<is_string_type_v<T>>>
							{	using TX = std::remove_const_t<std::remove_pointer_t<T>>;
								public:	using type = std::basic_string_view<TX>;
								template<class S> constexpr static type _do_extract(S& _s) { return _s.template _extract_string_view<TX>();}
								template<class D, class S> constexpr static void _do_extract(D& _dest, S& _s) { _s.template _extract_string_view<TX>(_dest);}
							};
template<class B, class T, std::size_t N> class serializer_peek<B, T[N], std::enable_if_t<is_string_type_v<T>>>
							{	using TX = std::remove_const_t<std::remove_pointer_t<T>>;
								public:	using type = std::basic_string_view<TX>;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset) { return _s.template _front_string_view<TX>(); }
								template<class D, class S> constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset) { _s.template _front_string_view<TX>(_dest); }
							};

// serializer_extract - T[N] & is_string_pointer_type_v<T>
template<class B, class T, std::size_t N> class serializer_extract<B, T[N], std::enable_if_t<is_string_pointer_type_v<T>>>
							{	using TX = std::remove_const_t<std::remove_pointer_t<T>>;
								public:	using type = std::array<std::basic_string_view<TX>, N>;
								template<class S> constexpr static type _do_extract(S& _s) { return _s.template _extract_array<std::basic_string_view<TX>,N>();}
								template<class D, class S> constexpr static void _do_extract(D& _dest, S& _s) { _s.template _extract_array<D, std::basic_string_view<TX>,N>(_dest);}
							};
template<class B, class T, std::size_t N> class serializer_peek<B, T[N], std::enable_if_t<is_string_pointer_type_v<T>>>
							{	using TX = std::remove_const_t<std::remove_pointer_t<T>>;
								public:	using type = std::array<std::basic_string_view<TX>, N>;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset) { S tb =_s + static_cast<std::size_t>(_offset); auto temp = tb.template _extract_array<std::basic_string_view<TX>,N>(); _offset = tb.data() - _s.data(); return temp;}
								template<class D, class S> constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset) { S tb =_s + static_cast<std::size_t>(_offset); tb.template _extract_array<D,std::basic_string_view<TX>,N>(_dest); _offset = tb.data() - _s.data();}
							};

// serializer_size_of - T[N]
template<class T, std::size_t N> class serializer_size_of<T[N], std::enable_if_t<!is_memcopyable<T>::value>>
							{	using TX = std::remove_const_t<T>;
								public:
								constexpr static std::size_t  _get_append_size(const TX(&_object)[N]) { std::size_t size = sizeof(COUNT_T); for (const auto& iter : _object) size += get_size_of(iter); return size; }
								template<class S> 
								constexpr static std::size_t  _get_extract_size(const S& _buffer, int64_t& _offset) { std::size_t size = sizeof(COUNT_T); if ((_buffer.size() - _offset) < size) throw std::exception();  _offset += size; /*for (const auto& iter : _object) size += get_extract_size_of<T>(_buffer, _offset);*/ return size; }
							};

template<class T, std::size_t N> class serializer_size_of<T[N], std::enable_if_t<is_memcopyable<T>::value>>
							{	using TX = std::remove_const_t<T>;
								public:
								constexpr static std::size_t  _get_append_size(const TX(&)[N]) { return sizeof(COUNT_T) + sizeof(TX) * N; }
								template<class S> 
								constexpr static std::size_t  _get_extract_size(const S& _buffer, int64_t& _offset) { std::size_t size = sizeof(COUNT_T) + sizeof(TX) * N; if ((_buffer.size() - _offset) < size) throw std::exception();  _offset += size; return size; }
							};

// serializer_extract - T[] & !is_string_type_v<T> && !is_string_pointer_type_v<T>
template<class B, class T>	class serializer_extract<B, T[], std::enable_if_t<!is_string_type_v<T> && !is_string_pointer_type_v<T>>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = std::vector<TX>;
								template<class S> constexpr static type _do_extract(S& _s) { return _s.template _extract_container_list<type>(); }
								template<class D, class S> constexpr static void _do_extract(D& _dest, S& _s) { _s.template _extract_container_list<type>(_dest); }
							};
template<class B, class T>	class serializer_peek<B, T[], std::enable_if_t<!is_string_type_v<T> && !is_string_pointer_type_v<T>>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = std::vector<TX>;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset) { return _s.template _front_container_list<type>(_offset); }
								template<class D, class S> constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset) { _s.template _front_container_list<type>(_dest, _offset); }
							};

// 4) serializer_extract - T[] &  is_string_type_v<T>
template<class B, class T>	class serializer_extract<B, T[], std::enable_if_t<is_string_type_v<T>>>
							{	using TX = std::remove_const_t<std::remove_pointer_t<T>>;
								public:	using type = std::basic_string_view<TX>;
								template<class S> constexpr static type _do_extract(S& _s) { return _s.template _extract_string_view<TX>(); }
								template<class D, class S> constexpr static void _do_extract(D& _dest, S& _s) { _s.template _extract_string_view<TX>(_dest); }
};
template<class B, class T>	class serializer_peek<B, T[], std::enable_if_t<is_string_type_v<T>>>
							{	using TX = std::remove_const_t<std::remove_pointer_t<T>>;
								public:	using type = std::basic_string_view<TX>;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset) { return _s.template _front_string_view<TX>(_offset); }
								template<class D, class S> constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset) { _s.template _front_string_view<TX>(_dest, _offset); }
							};

// 4) serializer_extract - T[] &  is_string_pointer_type_v<T>
template<class B, class T>	class serializer_extract<B, T[], std::enable_if_t<is_string_pointer_type_v<T>>>
							{	using TX = std::remove_const_t<std::remove_pointer_t<T>>;
								public:	using type = std::vector<std::basic_string_view<TX>>;
								template<class S> constexpr static type _do_extract(S& _s) { return _s.template _extract_container_list<type>(); }
								template<class D, class S> constexpr static void _do_extract(D& _dest, S& _s) { _s.template _extract_container_list<type>(_dest); }
							};
template<class B, class T>	class serializer_peek<B, T[], std::enable_if_t<is_string_pointer_type_v<T>>>
							{	using TX = std::remove_const_t<std::remove_pointer_t<T>>;
								public:	using type = std::vector<std::basic_string_view<TX>>;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset) { return _s.template _front_container_list<type>(_offset); }
								template<class D, class S> constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset) { _s.template _front_container_list<type>(_dest, _offset); }
							};


//-----------------------------------------------------------------------------
// std::array<T,X>
//-----------------------------------------------------------------------------
// 1) T[N] - !is_string_pointer_type_v<T>
template<class B, class T, std::size_t N> class serializer_append<B, std::array<T,N>>
							{	public: using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const std::array<T,N>& _data) { return _s.template _append_array<T,N>(_data.data());}
								template<class S> constexpr static type _do_append(S& _s, std::array<T,N>&& _data) { return _s.template _append_array<T,N>(_data.data());}
							};
template<class B, class T, std::size_t N> class serializer_extract<B, std::array<T,N>>
							{	public: using type = std::array<T,N>;
								template<class S> constexpr static type _do_extract(S& _s) { return _s.template _extract_array<T, N>(); }
								template<class D, class S> constexpr static void _do_extract(D& _dest, S& _s) { _s.template _extract_array<T, N>(_dest); }
							};
template<class B, class T, std::size_t N> class serializer_peek<B, std::array<T,N>>
							{	public:	using type = std::array<T,N>;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset) { S tb =_s + _offset; auto temp = tb.template _extract_array<T,N>(); _offset = tb.data() - _s.data(); return temp;}
								template<class D, class S> constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset) { S tb =_s + _offset; tb.template _extract_array<T,N>(_dest); _offset = tb.data() - _s.data();}
							};
template<class T, std::size_t N> class serializer_size_of<std::array<T,N>>
							{	public:
								constexpr static std::size_t  _get_append_size(const std::array<T,N>& _object) { std::size_t size = sizeof(COUNT_T); for (const auto& iter : _object) size += get_size_of(iter); return size; }
								template<class S> 
								constexpr static std::size_t  _get_extract_size(const S& _buffer, int64_t& _offset) { std::size_t size = sizeof(COUNT_T); if ((_buffer.size() - _offset) < size) throw std::exception();  _offset += size; /*for (const auto& iter : _object) size += get_extract_size_of<T>(_buffer, _offset);*/ return size; }
							};


//-----------------------------------------------------------------------------
// buffer_view
//-----------------------------------------------------------------------------

template<class B>			class serializer_append<B, _buffer_view<typename B::element_t>>
							{	public:	using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const _buffer_view<typename B::element_t>& _data) { return _s._append_buffer(_data);}
							};
template<class B>			class serializer_append<B, _basic_buffer<typename B::element_t>>
							{	public:	using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const _basic_buffer<typename B::element_t>& _data) { return _s._append_buffer(_data);}
							};
template<class B, class T>	class serializer_append<B, _shared_buffer<T>>
							{	public:	using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const _shared_buffer<T>& _data) { return _s._append_buffer(_data);}
							};

template<class B>			class serializer_extract<B, _buffer_view<typename B::element_t>>
							{	public:	using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_extract(S& _s) { return _s._extract_buffer_view();}
								template<class D, class S> constexpr static void _do_extract(D& _dest, S& _s) { _s._extract_buffer_view(_dest);}
							};
template<class B, class T>	class serializer_peek<B, T, std::enable_if_t<std::is_base_of_v<_buffer_view<typename B::element_t>, T>>>
							{
								public:	using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset) { S tb =_s + static_cast<std::size_t>(_offset); auto temp = tb.template _extract_buffer<T>(); _offset = tb.data() - _s.data(); return temp;}
								template<class D, class S> constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset) { S tb =_s + static_cast<std::size_t>(_offset); tb.template _extract_buffer<T>(_dest); _offset = tb.data() - _s.data();}
							};
template<class T>	class serializer_size_of<T, std::enable_if_t<is_buffer_type_v<T>>>
							{
								public:
								constexpr static std::size_t  _get_append_size(const T& _object) { return (_object.data() != nullptr) ? (sizeof(std::size_t) + _object.size()) : sizeof(std::size_t); }
								template<class S> 
								constexpr static std::size_t  _get_extract_size(const S& _buffer, int64_t& _offset) noexcept { CGDK_ASSERT_ERROR; return 0;}
							};


//-----------------------------------------------------------------------------
// web
//-----------------------------------------------------------------------------
// 1) web
template<class B>			class serializer_extract<B, WEB>
							{	public:	using type = std::string_view;
								template<class S> constexpr static type _do_extract(S& _s) { return _s._extract_web();}
								template<class D, class S> constexpr static void _do_extract(D& _dest, S& _s) { _s._extract_web(_dest);}
							};
template<class B>			class serializer_peek<B, WEB>
							{	public:	using type = std::string_view;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset) { return _s._front_web(_offset);}
								template<class D, class S> constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset) { _s._front_web(_dest, _offset);}
							};
template<>					class serializer_size_of<WEB>
							{	public:
								constexpr static std::size_t _get_append_size(const WEB&) noexcept { CGDK_ASSERT_ERROR; return 0; }
								template<class S> 
								static constexpr std::size_t _get_extract_size(const S& _buffer, int64_t& _offset) noexcept { CGDK_ASSERT_ERROR; return 0;}
							};

// 2) web_modify
template<class B>			class serializer_extract<B, WEB_MODIFY>
							{	public:	using type = std::string_view;
								template<class S> constexpr static type _do_extract(S& _s) { return _s._extract_web_modify();}
								template<class D, class S> constexpr static void _do_extract(D& _dest, S& _s) { _s._extract_web_modify(_dest);}
							};
template<class B>			class serializer_peek<B, WEB_MODIFY>
							{	public:	using type = std::string_view;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset) { S tb =_s + static_cast<std::size_t>(_offset); return tb._extract_web_modify();}
								template<class D, class S> constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset) { S tb =_s + static_cast<std::size_t>(_offset); tb._extract_web_modify(_dest);}
							};
template<>					class serializer_size_of<WEB_MODIFY>
							{	public:
								constexpr static std::size_t _get_append_size(const WEB&) noexcept { CGDK_ASSERT_ERROR; return 0; }
								template<class S> 
								constexpr static std::size_t _get_extract_size(const S& _buffer, int64_t& _offset) noexcept { CGDK_ASSERT_ERROR; return 0;}
							};


//-----------------------------------------------------------------------------
// skip
//-----------------------------------------------------------------------------
template<class B>			class serializer_append<B, skip>
							{	public:	using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const skip& _data) { return _s._append_skip(_data.amount);}
								template<class S> constexpr static type _do_append(S& _s, skip&& _data) { return _s._append_skip(_data.amount);}
							};

template<>					class serializer_size_of<skip>
							{	public:
								constexpr static std::size_t  _get_append_size(const skip& _object) { return static_cast<std::size_t>(_object.amount); }
								template<class S> 
								constexpr static std::size_t  _get_extract_size(const S& _buffer, int64_t& _offset) noexcept { CGDK_ASSERT_ERROR; return 0;}
							};


//-----------------------------------------------------------------------------
// tuple
//-----------------------------------------------------------------------------
template<class B, class... T>	class serializer_append<B, std::tuple<T...>>
							{
								public:	using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const std::tuple<T...>& _data) { return _s.template _append_tuple<T...>(_data);}
								template<class S> constexpr static type _do_append(S& _s, std::tuple<T...>&& _data) { return _s.template _append_tuple<T...>(std::move(_data));}
							};
template<class B, class... T> class serializer_extract<B, std::tuple<T...>>
							{	public:	using type = std::tuple<T...>;
								template<class S> constexpr static type _do_extract(S& _s) { std::tuple<T...> temp; _s.template _extract_tuple<T...>(temp); return temp;}
								template<class D, class S> constexpr static void _do_extract(D& _dest, S& _s) { _s.template _extract_tuple<T...>(_dest);}
							};
template<class B, class... T> class serializer_peek<B, std::tuple<T...>>
							{	public:	using type = std::tuple<T...>;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset) { S tb = _s + static_cast<std::size_t>(_offset); std::tuple<T...> temp; tb.template _extract_tuple<T...>(temp); _offset = tb.data() - _s.data(); return temp;}
								template<class D, class S> constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset) { S tb = _s + static_cast<std::size_t>(_offset); tb.template _extract_tuple<T...>(_dest); _offset = tb.data() - _s.data();}
								constexpr static std::size_t  _get_extract_size(const buffer_view& _buffer, int64_t& _offset) noexcept { CGDK_ASSERT_ERROR; return 0;}
							};
template<class... T>		class serializer_size_of<std::tuple<T...>>
							{	public:
								template<class TLAST>
								constexpr static std::size_t  _get_append_size(const std::tuple<TLAST>& _tuple) { return get_size_of(std::get<0>(_tuple)); }
								template<class TFIRST, class TSECOND, class... TREST>
								constexpr static std::size_t  _get_append_size(const std::tuple<TFIRST, TSECOND, TREST...>& _tuple)	{ return get_size_of(std::get<0>(_tuple)) + _get_append_size((const std::tuple<TSECOND, TREST...>&)_tuple); }
								template<class S> 
								constexpr static std::size_t  _get_extract_size(const S& _buffer, int64_t& _offset) noexcept { CGDK_ASSERT_ERROR; return 0;}
							};


//-----------------------------------------------------------------------------
// Ibuffer_serializable
//-----------------------------------------------------------------------------
// 1) T- Ibuffer_serializable
template<class B, class T>	class serializer_append<B, T, std::enable_if_t<std::is_base_of_v<Ibuffer_serializable, T>>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const TX& _data) { auto len_old = _s.size(); const_cast<TX&>(_data).serialize_out(_s); return type(_s.data() + len_old, _s.size() - len_old);}
								template<class S> constexpr static type _do_append(S& _s, TX&& _data) { auto len_old = _s.size(); _data.serialize_out(_s); return type(_s.data() + len_old, _s.size() - len_old);}
							};
template<class B, class T>	class serializer_extract<B, T, std::enable_if_t<std::is_base_of_v<Ibuffer_serializable, T>>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = TX;
								template<class S> constexpr static type _do_extract(S& _s) { TX tx; tx.serialize_in(_s); return tx;}
								template<class D, class S> constexpr static void _do_extract(D& _dest, S& _s) { _dest.serialize_in(_s);}
							};
template<class B, class T>	class serializer_peek<B, T, std::enable_if_t<std::is_base_of_v<Ibuffer_serializable, T>>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = T;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset) { S tb =_s + static_cast<std::size_t>(_offset); type tx; tx.serialize_in(tb); _offset = tb.data() - _s.data(); return tx;}
								template<class D, class S> constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset) { S tb =_s + static_cast<std::size_t>(_offset); _dest.serialize_in(tb); _offset = tb.data() - _s.data();}
							};
template<class T>			class serializer_size_of<T, std::enable_if_t<std::is_base_of_v<Ibuffer_serializable, T>>>
							{	using TX = std::remove_const_t<T>;
								public:
								constexpr static std::size_t  _get_append_size(const TX& _object) { return _object.get_size_of(); }
								constexpr static std::size_t  _get_extract_size(const buffer_view& _buffer, int64_t& _offset) noexcept { CGDK_ASSERT_ERROR; return 0;}
							};

// null marker of the pointer serializers (COUNT_T(-1): every byte is 0xff)
template<class S> bool _is_null_serialized(const S& _s, std::size_t _offset = 0) noexcept
{
	if(static_cast<std::size_t>(_s.size()) < _offset + sizeof(COUNT_T))
		return false;

	const auto* p = reinterpret_cast<const unsigned char*>(_s.data()) + _offset;
	for(std::size_t i = 0; i < sizeof(COUNT_T); ++i)
		if(p[i] != 0xff) return false;

	return true;
}

// 2) T* - Ibuffer_serializable
template<class B, class T>	class serializer_append<B, T*, std::enable_if_t<std::is_base_of_v<Ibuffer_serializable, T>>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = _buffer_view<typename B::element_t>;
								template<class S>
								constexpr static type _do_append(S& _s, const TX* _data) { auto len_old = _s.size(); if(_data!=nullptr) {const_cast<std::decay_t<TX>*>(_data)->serialize_out(_s);} else { _s.template _append<COUNT_T>(COUNT_T(0)-1);} return type(_s.data() + len_old, _s.size() - len_old);}
							};
template<class T>			class serializer_size_of<T*, std::enable_if_t<std::is_base_of_v<Ibuffer_serializable, T>>>
							{	using TX = std::remove_const_t<T>;
								public:
								constexpr static std::size_t  _get_append_size(const TX* _object) { return (_object != nullptr) ? _object->get_size_of() : sizeof(COUNT_T); }
								template<class S> 
								constexpr static std::size_t  _get_extract_size(const S& _buffer, int64_t& _offset) noexcept { CGDK_ASSERT_ERROR; return 0; }
							};


#if defined(_CGDK) && !defined(_UNREAL)
// 2) object_ptr<T>- Ibuffer_serializable
template<class B, class T>	class serializer_append<B, object_ptr<T>, std::enable_if_t<std::is_base_of_v<Ibuffer_serializable, T>>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const TX* _data) { auto len_old = _s.size(); if(_data != nullptr) {const_cast<TX*>(_data)->serialize_out(_s);} else { _s.template _append<COUNT_T>(COUNT_T(0)-1);} return type(_s.data() + len_old, _s.size() - len_old);}
							};
template<class B, class T>	class serializer_extract<B, object_ptr<T>, std::enable_if_t<std::is_base_of_v<Ibuffer_serializable, T>>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = object_ptr<TX>;
								template<class S> constexpr static type _do_extract(S& _s) { if(_is_null_serialized(_s)) { _s.template _extract<COUNT_T>(); return type();} type t = make_object<TX>(); t->serialize_in(_s); return t;}
								template<class D, class S> constexpr static void _do_extract(D& _dest, S& _s) { if(_is_null_serialized(_s)) { _s.template _extract<COUNT_T>(); _dest = type(); return;} _dest = make_object<TX>(); _dest->serialize_in(_s);}
							};
template<class B, class T>	class serializer_peek<B, object_ptr<T>, std::enable_if_t<std::is_base_of_v<Ibuffer_serializable, T>>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = object_ptr<TX>;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset) { if(_is_null_serialized(_s, static_cast<std::size_t>(_offset))) return type(); S tb =_s + static_cast<std::size_t>(_offset); type tx = make_object<TX>(); tx->serialize_in(tb); return tx;}
								template<class D, class S> constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset) { if(_is_null_serialized(_s, static_cast<std::size_t>(_offset))) { _dest = type(); return;} S tb =_s + static_cast<std::size_t>(_offset); _dest = make_object<TX>(); _dest->serialize_in(tb);}
							};
template<class T>			class serializer_size_of<object_ptr<T>, std::enable_if_t<std::is_base_of_v<Ibuffer_serializable, T>>>
							{	using TX = std::remove_const_t<T>;
								public:
								constexpr static std::size_t  _get_append_size(const TX* _object) { return (_object != nullptr) ? _object->get_size_of() : sizeof(COUNT_T); }
								template<class S> 
								constexpr static std::size_t  _get_extract_size(const S& _buffer, int64_t& _offset) noexcept { CGDK_ASSERT_ERROR; return 0;}
							};

// 3) own_ptr<T>- Ibuffer_serializable
template<class B, class T>	class serializer_append<B, own_ptr<T>, std::enable_if_t<std::is_base_of_v<Ibuffer_serializable, T>>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const TX* _data) { auto len_old = _s.size(); if(_data != nullptr) {const_cast<TX*>(_data)->serialize_out(_s);} else { _s.template _append<COUNT_T>(COUNT_T(0)-1);} return type(_s.data() + len_old, _s.size() - len_old);}
							};
template<class B, class T>	class serializer_extract<B, own_ptr<T>, std::enable_if_t<std::is_base_of_v<Ibuffer_serializable, T>>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = own_ptr<TX>;
								template<class S> constexpr static type _do_extract(S& _s) { if(_is_null_serialized(_s)) { _s.template _extract<COUNT_T>(); return type();} type t = make_own<TX>(); t->serialize_in(_s); return t;}
								template<class D, class S> constexpr static void _do_extract(D& _dest, S& _s) { if(_is_null_serialized(_s)) { _s.template _extract<COUNT_T>(); _dest = type(); return;} _dest = make_own<TX>(); _dest->serialize_in(_s);}
							};
template<class B, class T>	class serializer_peek<B, own_ptr<T>, std::enable_if_t<std::is_base_of_v<Ibuffer_serializable, T>>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = own_ptr<TX>;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset) { if(_is_null_serialized(_s, static_cast<std::size_t>(_offset))) return type(); S tb =_s + static_cast<std::size_t>(_offset); type tx = make_own<TX>(); tx->serialize_in(tb); return tx;}
								template<class D, class S> constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset) { if(_is_null_serialized(_s, static_cast<std::size_t>(_offset))) { _dest = type(); return;} S tb =_s + static_cast<std::size_t>(_offset); _dest = make_own<TX>(); _dest->serialize_in(tb);}
							};
template<class T>			class serializer_size_of<own_ptr<T>, std::enable_if_t<std::is_base_of_v<Ibuffer_serializable, T>>>
							{	using TX = std::remove_const_t<T>;
								public:
								constexpr static std::size_t  _get_append_size(const TX* _object) { return (_object != nullptr) ? _object->get_size_of() : sizeof(COUNT_T); }
								template<class S> 
								constexpr static std::size_t  _get_extract_size(const S& _buffer, int64_t& _offset) noexcept { CGDK_ASSERT_ERROR; return 0; }
							};
#endif
#if defined(__cpp_lib_shared_ptr_arrays)
// 4) std::shared_ptr<T>- Ibuffer_serializable
template<class B, class T>	class serializer_append<B, std::shared_ptr<T>, std::enable_if_t<std::is_base_of_v<Ibuffer_serializable, T>>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const std::shared_ptr<T>& _data) { auto len_old = _s.size(); if(_data != nullptr) { _data->serialize_out(_s);} else { _s.template _append<COUNT_T>(COUNT_T(0)-1);} return type(_s.data() + len_old, _s.size() - len_old);}
							};
template<class B, class T>	class serializer_extract<B, std::shared_ptr<T>, std::enable_if_t<std::is_base_of_v<Ibuffer_serializable, T>>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = std::shared_ptr<TX>;
								template<class S> constexpr static type _do_extract(S& _s) { if(_is_null_serialized(_s)) { _s.template _extract<COUNT_T>(); return type();} type t = std::make_shared<TX>(); t->serialize_in(_s); return t;}
								template<class D, class S> constexpr static void _do_extract(D& _dest, S& _s) { if(_is_null_serialized(_s)) { _s.template _extract<COUNT_T>(); _dest = type(); return;} _dest = std::make_shared<TX>(); _dest->serialize_in(_s);}
							};
template<class B, class T>	class serializer_peek<B, std::shared_ptr<T>, std::enable_if_t<std::is_base_of_v<Ibuffer_serializable, T>>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = std::shared_ptr<TX>;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset) { if(_is_null_serialized(_s, static_cast<std::size_t>(_offset))) return type(); S tb =_s + static_cast<std::size_t>(_offset); type tx = std::make_shared<TX>(); tx->serialize_in(tb); return tx;}
								template<class D, class S> constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset) { if(_is_null_serialized(_s, static_cast<std::size_t>(_offset))) { _dest = type(); return;} S tb =_s + static_cast<std::size_t>(_offset); _dest = std::make_shared<TX>(); _dest->serialize_in(tb);}
							};
template<class T>			class serializer_size_of<std::shared_ptr<T>, std::enable_if_t<std::is_base_of_v<Ibuffer_serializable, T>>>
							{	using TX = std::remove_const_t<T>;
								public:
								constexpr static std::size_t  _get_append_size(const std::shared_ptr<T>& _object) { return (_object != nullptr) ? _object->get_size_of() : sizeof(COUNT_T); }
								template<class S> 
								constexpr static std::size_t  _get_extract_size(const S& _buffer, int64_t& _offset) noexcept { CGDK_ASSERT_ERROR; return 0;}
							};
#endif

// 5) std::unique_ptr<T>- Ibuffer_serializable
template<class B, class T>	class serializer_append<B, std::unique_ptr<T>, std::enable_if_t<std::is_base_of_v<Ibuffer_serializable, T>>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const std::unique_ptr<T>& _data) { auto len_old = _s.size(); if(_data != nullptr) { _data->serialize_out(_s);} else { _s.template _append<COUNT_T>(COUNT_T(0)-1);} return type(_s.data() + len_old, _s.size() - len_old);}
							};
template<class B, class T>	class serializer_extract<B, std::unique_ptr<T>, std::enable_if_t<std::is_base_of_v<Ibuffer_serializable, T>>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = std::unique_ptr<TX>;
								template<class S> constexpr static type _do_extract(S& _s) { if(_is_null_serialized(_s)) { _s.template _extract<COUNT_T>(); return type();} type t = std::make_unique<TX>(); t->serialize_in(_s); return t;}
								template<class D, class S> constexpr static void _do_extract(D& _dest, S& _s) { if(_is_null_serialized(_s)) { _s.template _extract<COUNT_T>(); _dest = type(); return;} _dest = std::make_unique<TX>(); _dest->serialize_in(_s);}
							};
template<class B, class T>	class serializer_peek<B, std::unique_ptr<T>, std::enable_if_t<std::is_base_of_v<Ibuffer_serializable, T>>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = std::unique_ptr<TX>;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset) { if(_is_null_serialized(_s, static_cast<std::size_t>(_offset))) return type(); S tb =_s + static_cast<std::size_t>(_offset); type tx = std::make_unique<TX>(); tx->serialize_in(tb); return tx;}
								template<class D, class S> constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset) { if(_is_null_serialized(_s, static_cast<std::size_t>(_offset))) { _dest = type(); return;} S tb =_s + static_cast<std::size_t>(_offset); _dest = std::make_unique<TX>(); _dest->serialize_in(tb);}
							};
template<class T>			class serializer_size_of<std::unique_ptr<T>, std::enable_if_t<std::is_base_of_v<Ibuffer_serializable, T>>>
							{	using TX = std::remove_const_t<T>;
								public:
								constexpr static std::size_t  _get_append_size(const std::unique_ptr<T>& _object) { return (_object != nullptr) ? _object->get_size_of() : sizeof(COUNT_T); }
								template<class S> 
								constexpr static std::size_t  _get_extract_size(const S& _buffer, int64_t& _offset) noexcept { CGDK_ASSERT_ERROR; return 0;}
							};

//-----------------------------------------------------------------------------
// protobuf
//-----------------------------------------------------------------------------
#if defined(GOOGLE_PROTOBUF_MESSAGE_H__)
// 1) T- protobuf
template<class B, class T>	class serializer_append<B, T, std::enable_if_t<std::is_base_of_v<::google::protobuf::Message, T>>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const TX& _data)					
								{
									auto object_size = _data.ByteSizeLong();
									auto dest_ptr = _s.get_back_ptr();
									auto remained = static_cast<int>(_s.capacity() - _s.size());

									auto result = const_cast<TX&>(_data).SerializePartialToArray(dest_ptr, remained);

									if (result == false)
										throw std::exception(/*_s.size<std::size_t>(), std::string("protobuf:fail to serialize(append)")*/);

									type result_return(dest_ptr, object_size);
									_s.add_size(object_size);

									return result_return;
								}
							};
template<class B, class T>	class serializer_extract<B, T, std::enable_if_t<std::is_base_of_v<::google::protobuf::Message, T>>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = TX;
								template<class S> constexpr static type _do_extract(S& _s)
								{
									TX dest;
									_do_extract(dest, _s);
									return dest;
								}
								template<class D, class S> constexpr static void _do_extract(D& _dest, S& _s)
								{
									auto result = _dest.ParseFromArray(_s.template data<void>(), _s.template size<int>());

									if (result == false)
										throw std::exception(/*_s.size<std::size_t>(), std::string("protobuf:fail to serialize(extract)")*/);

									_s += offset(_dest.ByteSizeLong());
								}
							};
template<class B, class T>	class serializer_peek<B, T, std::enable_if_t<std::is_base_of_v<::google::protobuf::Message, T>>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = T;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset)
								{
									type dest;
									_do_peek(dest, _s, _offset);
									return dest;
								}
								template<class D, class S> constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset)
								{
									S tb =_s + static_cast<std::size_t>(_offset);

									auto result = _dest.ParseFromArray(tb.template data<void>(), tb.template size<int>());

									if (result == false)
										throw std::exception(/*_s.size<std::size_t>(), std::string("protobuf:fail to serialize(peek)")*/);

									_offset += _dest.ByteSizeLong();
								}
							};
template<class T>			class serializer_size_of<T, std::enable_if_t<std::is_base_of_v<::google::protobuf::Message, T>>>
							{	using TX = std::remove_const_t<T>;
								public:
								constexpr static std::size_t  _get_append_size(const TX& _object) { return _object.ByteSizeLong(); }
								constexpr static std::size_t  _get_extract_size(const buffer_view& _buffer, int64_t& _offset) noexcept { CGDK_ASSERT_ERROR; return 0;}
							};

// 2) T* - protobuf
template<class B, class T>	class serializer_append<B, T*, std::enable_if_t<std::is_base_of_v<::google::protobuf::Message, T>>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = _buffer_view<typename B::element_t>;
								template<class S>
								constexpr static type _do_append(S& _s, const TX* _data)
								{
									auto len_old = _s.size();

									if(_data != nullptr)
									{
										auto object_size = _data->ByteSizeLong();
										auto dest_ptr = _s.get_back_ptr();
										auto remained = static_cast<int>(_s.capacity() - _s.size());

										auto result = const_cast<TX*>(_data)->SerializePartialToArray(dest_ptr, remained);

										if (result == false)
											throw std::exception(/*_s.size<std::size_t>(), std::string("protobuf:fail to serialize(append)")*/);

										type result_return(dest_ptr, object_size);
										_s.add_size(object_size);
									}
									else
									{
										_s.template _append<COUNT_T>(COUNT_T(0)-1);
									}

									return type(_s.data() + len_old, _s.size() - len_old);
								}
							};
template<class T>			class serializer_size_of<T*, std::enable_if_t<std::is_base_of_v<::google::protobuf::Message, T>>>
							{	using TX = std::remove_const_t<T>;
								public:
								constexpr static std::size_t  _get_append_size(const TX* _object) { return _object->ByteSizeLong(); }
								template<class S> 
								constexpr static std::size_t  _get_extract_size(const S& _buffer, int64_t& _offset) noexcept { CGDK_ASSERT_ERROR; return 0; }
							};

// 2) object_ptr<T>- protobuf
template<class B, class T>	class serializer_append<B, object_ptr<T>, std::enable_if_t<std::is_base_of_v<::google::protobuf::Message, T>>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const TX* _data)
								{
									auto len_old = _s.size();
									
									if(_data != nullptr)
									{
										auto object_size = _data->ByteSizeLong();
										auto dest_ptr = _s.get_back_ptr();
										auto remained = static_cast<int>(_s.capacity() - _s.size());

										auto result = const_cast<TX*>(_data)->SerializePartialToArray(dest_ptr, remained);

										if (result == false)
											throw std::exception(/*_s.size<std::size_t>(), std::string("protobuf:fail to serialize(append)")*/);

										type result_return(dest_ptr, object_size);
										_s.add_size(object_size);
									}
									else
									{
										_s.template _append<COUNT_T>(COUNT_T(0)-1);
									}
									
									return type(_s.data() + len_old, _s.size() - len_old);
								}
							};
template<class B, class T>	class serializer_extract<B, object_ptr<T>, std::enable_if_t<std::is_base_of_v<::google::protobuf::Message, T>>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = object_ptr<TX>;
								template<class S> constexpr static type _do_extract(S& _s)
								{
									object_ptr<TX> dest; 
									_do_extract(dest, _s);
									return dest;
								}
								template<class D, class S> constexpr static void _do_extract(D& _dest, S& _s)
								{
									_dest = make_object<TX>();

									auto result = _dest->ParseFromArray(_s.template data<void>(), _s.template size<int>());

									if (result == false)
										throw std::exception(/*_s.size<std::size_t>(), std::string("protobuf:fail to serialize(extract)")*/);

									_s += offset(_dest->ByteSizeLong());
								}
							};
template<class B, class T>	class serializer_peek<B, object_ptr<T>, std::enable_if_t<std::is_base_of_v<::google::protobuf::Message, T>>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = object_ptr<TX>;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset)
								{
									object_ptr<TX> dest;
									_do_peek(dest, _s, _offset);
									return dest;
								}
								template<class D, class S> constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset)
								{
									S tb =_s + static_cast<std::size_t>(_offset);

									_dest = make_object<TX>();

									auto result = _dest->ParseFromArray(tb.template data<void>(), tb.template size<int>());

									if (result == false)
										throw std::exception(/*tb.size<std::size_t>(), std::string("protobuf:fail to serialize(peek)")*/);

									_offset += _dest->ByteSizeLong();
								}
							};
template<class T>			class serializer_size_of<object_ptr<T>, std::enable_if_t<std::is_base_of_v<::google::protobuf::Message, T>>>
							{	using TX = std::remove_const_t<T>;
								public:
								constexpr static std::size_t  _get_append_size(const TX* _object) { return _object->ByteSizeLong(); }
								template<class S> 
								constexpr static std::size_t  _get_extract_size(const S& _buffer, int64_t& _offset) noexcept { CGDK_ASSERT_ERROR; return 0;}
							};

// 3) own_ptr<T>- protobuf
template<class B, class T>	class serializer_append<B, own_ptr<T>, std::enable_if_t<std::is_base_of_v<::google::protobuf::Message, T>>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const TX* _data)
								{
									auto len_old = _s.size();
									
									if(_data != nullptr)
									{
										auto object_size = _data->ByteSizeLong();
										auto dest_ptr = _s.get_back_ptr();
										auto remained = static_cast<int>(_s.capacity() - _s.size());

										auto result = const_cast<TX*>(_data)->SerializePartialToArray(dest_ptr, remained);

										if (result == false)
											throw std::exception(/*_s.size<std::size_t>(), std::string("protobuf:fail to serialize(append)")*/);

										type result_return(dest_ptr, object_size);
										_s.add_size(object_size);
									}
									else
									{
										_s.template _append<COUNT_T>(COUNT_T(0)-1);
									}
									
									return type(_s.data() + len_old, _s.size() - len_old);
								}
							};
template<class B, class T>	class serializer_extract<B, own_ptr<T>, std::enable_if_t<std::is_base_of_v<::google::protobuf::Message, T>>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = own_ptr<TX>;
								template<class S> constexpr static type _do_extract(S& _s)
								{
									own_ptr<TX> t;
									_do_extract(t, _s);

									return t;
								}
								template<class D, class S> constexpr static void _do_extract(D& _dest, S& _s)
								{
									_dest = make_own<TX>();
									
									auto result = _dest->ParseFromArray(_s.template data<void>(), _s.template size<int>());

									if (result == false)
										throw std::exception(/*_s.size<std::size_t>(), std::string("protobuf:fail to serialize(extract)")*/);

									_s += offset(_dest->ByteSizeLong());
								}
							};
template<class B, class T>	class serializer_peek<B, own_ptr<T>, std::enable_if_t<std::is_base_of_v<::google::protobuf::Message, T>>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = own_ptr<TX>;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset)
								{
									own_ptr<TX> dest;
									_do_peek(dest, _s, _offset);
									return dest;
								}
								template<class D, class S> constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset)
								{
									S tb =_s + static_cast<std::size_t>(_offset);

									_dest = make_own<TX>();

									auto result = _dest->ParseFromArray(tb.template data<void>(), tb.template size<int>());

									if (result == false)
										throw std::exception(/*tb.size<std::size_t>(), std::string("protobuf:fail to serialize(peek)")*/);

									_offset += _dest->ByteSizeLong();
								}
							};
template<class T>			class serializer_size_of<own_ptr<T>, std::enable_if_t<std::is_base_of_v<::google::protobuf::Message, T>>>
							{	using TX = std::remove_const_t<T>;
								public:
								constexpr static std::size_t  _get_append_size(const TX* _object)							
								{
									return _object->ByteSizeLong();
								}
								template<class S> 
								constexpr static std::size_t  _get_extract_size(const S& _buffer, int64_t& _offset) noexcept
								{
									CGDK_ASSERT_ERROR; return 0;
								}
							};
#endif


//-----------------------------------------------------------------------------
//
// for strings
//-----------------------------------------------------------------------------

// class for append)
template<class B, class T>	class serializer_append<B, T*, std::enable_if_t<is_string_type<T>::value>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const TX* _data) { return _s.template  _append_string<TX>(std::basic_string_view<TX>(_data));}
							};
template<class B, class T>	class serializer_append<B, text<T>, std::enable_if_t<is_string_type<T>::value>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const text<T>& _data) { return _s.template  _append_text<TX>(_data.p);}
								template<class S> constexpr static type _do_append(S& _s, text<T>&& _data) { return _s.template  _append_text<TX>(_data.p);}
								template<class S, class TY> constexpr static type _do_append(S& _s, text<TY>&& _data) { return _s.template  _append_text_encoding<TX,TY>(_data.p);}
							};
template<class B, class T>	class serializer_append<B, std::basic_string<T>, std::enable_if_t<is_string_type<T>::value>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const std::basic_string<TX>& _data) { return _s.template  _append_string<TX>(_data);}
								template<class S> constexpr static type _do_append(S& _s, std::basic_string<TX>&& _data) { return _s.template  _append_string<TX>(_data);}
							};
template<class B, class T>	class serializer_append<B, std::basic_string_view<T>, std::enable_if_t<is_string_type<T>::value>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const std::basic_string_view<TX>& _data) { return _s.template  _append_string<TX>(_data);}
								template<class S> constexpr static type _do_append(S& _s, std::basic_string_view<TX>&& _data) { return _s.template  _append_string<TX>(_data);}
							};

// class for extract)
template<class B, class T>	class serializer_extract<B, T*, std::enable_if_t<is_string_type<T>::value>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = std::basic_string_view<TX>;
								template<class S> constexpr static type _do_extract(S& _s) { return _s.template _extract_string_view<TX>();}
								template<class D, class S> constexpr static void _do_extract(D& _dest, S& _s) { _s.template _extract_string_view<TX>(_dest);}
							};
template<class B, class T>	class serializer_extract<B, text<T>, std::enable_if_t<is_string_type<T>::value>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = std::basic_string<TX>;
								template<class S> constexpr static type _do_extract(S& _s) { return _s.template _extract_text<TX>(static_cast<TX>(0));}
								template<class D, class S> constexpr static void _do_extract(D& _dest, S& _s) { _dest = _s.template _extract_text<TX>(static_cast<TX>(0));}
							};
template<class B, class T>	class serializer_extract<B, std::basic_string<T>, std::enable_if_t<is_string_type<T>::value>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = std::basic_string<TX>;
								template<class S> constexpr static type _do_extract(S& _s) { return _s.template _extract_string<TX>();}
								template<class D, class S> constexpr static void _do_extract(D& _dest, S& _s) { _s.template _extract_string<TX>(_dest);}
							};
template<class B, class T>	class serializer_extract<B, std::basic_string_view<T>, std::enable_if_t<is_string_type<T>::value>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = std::basic_string_view<TX>;
								template<class S> constexpr static type _do_extract(S& _s) { return _s.template _extract_string_view<TX>();}
								template<class D, class S> constexpr static void _do_extract(D& _dest, S& _s) { _s.template _extract_string_view<TX>(_dest);}
							};

// class for front)
template<class B, class T>	class serializer_peek<B, T*, std::enable_if_t<is_string_type<T>::value>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = std::basic_string_view<TX>;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset){ return _s.template _front_string_view<TX>(_offset);}
								template<class D, class S> constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset){ _s.template _front_string_view<TX>(_dest, _offset);}
							};
template<class B, class T>	class serializer_peek<B, text<T>, std::enable_if_t<is_string_type<T>::value>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = std::basic_string<TX>;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset)	{ return _s.template _front_text<TX>(_offset, 0);}
								template<class D, class S> constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset) { _s.template _front_text<TX>(_dest, _offset, 0);}
							};
template<class B, class T>	class serializer_peek<B, std::basic_string<T>, std::enable_if_t<is_string_type<T>::value>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = std::basic_string<TX>;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset) { return _s.template _front_string<TX>(_offset);}
								template<class D, class S> constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset) { _s.template _front_string<TX>(_dest, _offset);}
							};
template<class B, class T>	class serializer_peek<B, std::basic_string_view<T>, std::enable_if_t<is_string_type<T>::value>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = std::basic_string_view<TX>;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset) { return _s.template _front_string_view<TX>(_offset);}
								template<class D, class S> constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset) { _s.template _front_string_view<TX>(_dest, _offset);}
							};

// class for get_size_of)
template<class T>			class serializer_size_of<T*, std::enable_if_t<is_string_type<T>::value>>
							{	using TX = std::remove_const_t<T>;
								public:
								constexpr static std::size_t  _get_append_size(const std::basic_string_view<TX>& _object) { return sizeof(COUNT_T) + (_object.size() + 1) * sizeof(TX); }
								template<class S> 
								constexpr static std::size_t  _get_extract_size(const S& _buffer, int64_t& _offset) noexcept { CGDK_ASSERT_ERROR; return 0; }
};
template<class T>			class serializer_size_of<text<T>, std::enable_if_t<is_string_type<T>::value>>
							{	using TX = std::remove_const_t<T>;
								public:
								constexpr static std::size_t  _get_append_size(const text<TX>& _object) { return _object.bytes(); }
								template<class S> 
								constexpr static std::size_t  _get_extract_size(const S& _buffer, int64_t& _offset) noexcept { CGDK_ASSERT_ERROR; return 0;}
							};
template<class T>			class serializer_size_of<std::basic_string<T>, std::enable_if_t<is_string_type<T>::value>>
							{	using TX = std::remove_const_t<T>;
								public:
								constexpr static std::size_t  _get_append_size(const std::basic_string<TX>& _object) { return sizeof(COUNT_T) + (_object.size() + 1) * sizeof(TX); }
								template<class S> 
								constexpr static std::size_t  _get_extract_size(const S& _buffer, int64_t& _offset) noexcept { CGDK_ASSERT_ERROR; return 0; }
};
template<class T>			class serializer_size_of<std::basic_string_view<T>, std::enable_if_t<is_string_type<T>::value>>
							{	using TX = std::remove_const_t<T>;
								public:
								constexpr static std::size_t  _get_append_size(const std::basic_string_view<TX>& _object) { return sizeof(COUNT_T) + (_object.size() + 1) * sizeof(TX); }
								template<class S> 
								constexpr static std::size_t  _get_extract_size(const S& _buffer, int64_t& _offset) noexcept { CGDK_ASSERT_ERROR; return 0; }
							};


//-----------------------------------------------------------------------------
// linear container - push_back() only (list<T>, deque<T>, circular_list<T>, ...)
//-----------------------------------------------------------------------------
// linear container<T>)
template<class B, class T>	class serializer_append<B, T, std::enable_if_t<is_linear_container<T>::value>>
							{	public:	using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const T& _data) { return _s.template _append_container_list<T>(_data);}
								template<class S> constexpr static type _do_append(S& _s, T&& _data) { return _s.template _append_container_list<T>(_data);}
							};
template<class B, class T>	class serializer_extract<B, T, std::enable_if_t<is_linear_container<T>::value>>
							{	public:	using type = T;
								template<class S> constexpr static type _do_extract(S& _s) { return _s.template _extract_container_list<T>();}
								template<class D, class S> constexpr static void _do_extract(D& _dest, S& _s) { _s.template _extract_container_list<T>(_dest);}
							};
template<class B, class T>	class serializer_peek<B, T, std::enable_if_t<is_linear_container<T>::value>>
							{	public:	using type = T;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset)	{ return _s.template _front_container_list<T>(_offset);}
								template<class D, class S> constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset) { _s.template _front_container_list<T>(_dest, _offset);}
							};

template<typename T>		class serializer_size_of<T, std::enable_if_t<is_linear_container<T>::value>>
							{	public:
								constexpr static std::size_t  _get_append_size(const T& _object)
								{
									auto size = sizeof(COUNT_T);
								
									for (const auto& iter : _object)
									{
										size += get_size_of(iter);
									}

									// return )
									return	size;
								}
								template<class S> 
								constexpr static std::size_t  _get_extract_size(const S& _buffer, int64_t& _offset) noexcept
								{
									CGDK_ASSERT_ERROR;
									return 0;
								}
							};

// linear container<object_ptr<T>>)
template<class B, class T>	class serializer_append<B, T, std::enable_if_t<is_linear_object_ptr_container<T>::value>>
							{	public:	using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const T& _data) { return _s.template _append_container_object_ptr_list<T>(_data);}
								template<class S> constexpr static type _do_append(S& _s, T&& _data) { return _s.template _append_container_object_ptr_list<T>(_data);}
							};
template<class B, class T>	class serializer_extract<B, T, std::enable_if_t<is_linear_object_ptr_container<T>::value>>
							{	public:	using type = T;
								template<class S> constexpr static type _do_extract(S& _s) { return _s.template _extract_container_object_ptr_list<T>();}
								template<class D, class S> constexpr static void _do_extract(D& _dest, S& _s) { _s.template _extract_container_object_ptr_list<T>(_dest);}
							};
template<class B, class T>	class serializer_peek<B, T, std::enable_if_t<is_linear_object_ptr_container<T>::value>>
							{	public:	using type = T;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset){ return _s.template _front_container_object_ptr_list<T>(_offset);}
								template<class D, class S> constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset){ _s.template _front_container_object_ptr_list<T>(_dest, _offset);}
							};
template<class T>			class serializer_size_of<T, std::enable_if_t<is_linear_object_ptr_container<T>::value>>
							{	public:
								constexpr static std::size_t  _get_append_size(const T& _object)
								{
									auto size = sizeof(COUNT_T);
								
									for (const auto& iter : _object)
									{
										size += get_size_of(iter);
									}

									// return )
									return	size;
								}
								template<class S> 
								constexpr static std::size_t  _get_extract_size(const S& _buffer, int64_t& _offset) noexcept
								{
									CGDK_ASSERT_ERROR;
									return 0;
								}
							};

// linear container<own_ptr<T>>)
template<class B, class T>	class serializer_append<B, T, std::enable_if_t<is_linear_own_ptr_container<T>::value>>
							{	public:	using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const T& _data) { return _s.template _append_container_own_ptr_list<T>(_data);}
								template<class S> constexpr static type _do_append(S& _s, T&& _data) { return _s.template _append_container_own_ptr_list<T>(_data);}
							};
template<class B, class T>	class serializer_extract<B, T, std::enable_if_t<is_linear_own_ptr_container<T>::value>>
							{	public:	using type = T;
								template<class S> constexpr static type _do_extract(S& _s) { return _s.template _extract_container_own_ptr_list<T>();}
								template<class D, class S> constexpr static void _do_extract(D& _dest, S& _s) { _s.template _extract_container_own_ptr_list<T>(_dest);}
							};
template<class B, class T>	class serializer_peek<B, T, std::enable_if_t<is_linear_own_ptr_container<T>::value>>
							{	public:	using type = T;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset){ return _s.template _front_container_own_ptr_list<T>(_offset);}
								template<class D, class S> constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset){ _s.template _front_container_own_ptr_list<T>(_dest, _offset);}
							};
template<class T>			class serializer_size_of<T, std::enable_if_t<is_linear_own_ptr_container<T>::value>>
							{	public:
								constexpr static std::size_t  _get_append_size(const T& _object)
								{
									auto size = sizeof(COUNT_T);
								
									for (const auto& iter : _object)
									{
										size += get_size_of(iter);
									}

									// return )
									return	size;
								}
								template<class S> 
								constexpr static std::size_t  _get_extract_size(const S& _buffer, int64_t& _offset) noexcept
								{
									CGDK_ASSERT_ERROR;
									return 0;
								}
							};


//-----------------------------------------------------------------------------
// linear container<shared_buffer> - push_back() and reserve() - (vector<T>, ...)
//-----------------------------------------------------------------------------
// linear container<T>
template<class B, class T>	class serializer_append<B, T, std::enable_if_t<is_linear_container_with_reserve<T>::value>>
							{	public:	using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const T& _data) { return _s.template _append_container_array<T>(_data);}
								template<class S> constexpr static type _do_append(S& _s, T&& _data) { return _s.template _append_container_array<T>(_data);}
							};
template<class B, class T>	class serializer_extract<B, T, std::enable_if_t<is_linear_container_with_reserve<T>::value>>
							{	public:	using type = T;
								template<class S> constexpr static type _do_extract(S& _s) { return _s.template _extract_container_array<T>();}
								template<class D, class S> constexpr static void _do_extract(D& _dest, S& _s) { _s.template _extract_container_array<T>(_dest);}
							};
template<class B, class T>	class serializer_peek<B, T, std::enable_if_t<is_linear_container_with_reserve<T>::value>>
							{	public:	using type = T;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset){ return _s.template _front_container_array<T>(_offset);}
								template<class D, class S> constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset){ _s.template _front_container_array<T>(_dest, _offset);}
							};
template<class T>			class serializer_size_of<T, std::enable_if_t<is_linear_container_with_reserve<T>::value>>
							{	public:
								constexpr static std::size_t  _get_append_size(const T& _object)
								{
									auto size = sizeof(COUNT_T);
								
									for (const auto& iter : _object)
									{
										size += get_size_of(iter);
									}

									// return )
									return	size;
								}
								template<class S> 
								constexpr static std::size_t  _get_extract_size(const S& _buffer, int64_t& _offset) noexcept
								{
									CGDK_ASSERT_ERROR;
									return 0;
								}

							};

// linear container<object_ptr<T>>
template<class B, class T>	class serializer_append<B, T, std::enable_if_t<is_linear_object_ptr_container_with_reserve<T>::value>>
							{	public:	using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const T& _data) { return _s.template _append_container_array<T>(_data);}
								template<class S> constexpr static type _do_append(S& _s, T&& _data) { return _s.template _append_container_array<T>(_data);}
							};
template<class B, class T>	class serializer_extract<B, T, std::enable_if_t<is_linear_object_ptr_container_with_reserve<T>::value>>
							{	public:	using type = T;
								template<class S> constexpr static type _do_extract(S& _s) { return _s.template _extract_container_object_ptr_array<T>();}
								template<class D, class S> constexpr static void _do_extract(D& _dest, S& _s) { _s.template _extract_container_object_ptr_array<T>(_dest);}
							};
template<class B, class T>	class serializer_peek<B, T, std::enable_if_t<is_linear_object_ptr_container_with_reserve<T>::value>>
							{	public:	using type = T;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset){ return _s.template _front_object_ptr_vector<T>(_offset);}
								template<class D, class S> constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset){ _s.template _front_object_ptr_vector<T>(_dest, _offset);}
							};
template<class T>			class serializer_size_of<T, std::enable_if_t<is_linear_object_ptr_container_with_reserve<T>::value>>
							{	public:
								constexpr static std::size_t  _get_append_size(const T& _object)
								{
									auto size = sizeof(COUNT_T);

									for (const auto& iter : _object)
									{
										size += get_size_of(iter);
									}

									// return )
									return	size;
								}
								template<class S> 
								constexpr static std::size_t  _get_extract_size(const buffer_view& _buffer, int64_t& _offset) noexcept
								{
									CGDK_ASSERT_ERROR;
									return 0;
								}
							};

// linear container<own_ptr<T>>
template<class B, class T>	class serializer_append<B, T, std::enable_if_t<is_linear_own_ptr_container_with_reserve<T>::value>>
							{	public:	using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const T& _data) { return _s.template _append_container_array<T>(_s, _data);}
								template<class S> constexpr static type _do_append(S& _s, T&& _data) { return _s.template _append_container_array<T>(_s, _data);}
							};
template<class B, class T>	class serializer_extract<B, T, std::enable_if_t<is_linear_own_ptr_container_with_reserve<T>::value>>
							{	public:	using type = T;
								template<class S> constexpr static type _do_extract(S& _s) { return _s.template _extract_container_own_ptr_array<T>();}
								template<class D, class S> constexpr static void _do_extract(D& _dest, S& _s) { _s.template _extract_container_own_ptr_array<T>(_dest);}
							};
template<class B, class T>	class serializer_peek<B, T, std::enable_if_t<is_linear_own_ptr_container_with_reserve<T>::value>>
							{	public:	using type = T;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset){ return _s.template _front_own_ptr_vector<T>(_offset);}
								template<class D, class S> constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset) { _s.template _front_own_ptr_vector<T>(_dest, _offset);}
							};
template<class T>			class serializer_size_of<T, std::enable_if_t<is_linear_own_ptr_container_with_reserve<T>::value>>
							{	public:
								constexpr static std::size_t  _get_append_size(const T& _object)
								{
									auto size = sizeof(COUNT_T);
								
									for (const auto& iter : _object)
									{
										size += get_size_of(iter);
									}

									// return )
									return	size;
								}
								template<class S> 
								constexpr static std::size_t  _get_extract_size(const S& _buffer, int64_t& _offset) noexcept
								{ 
									CGDK_ASSERT_ERROR;
									return 0;
								}
							};


//-----------------------------------------------------------------------------
// std::span<T>
//-----------------------------------------------------------------------------
#if defined(__cpp_lib_span)
// std::span<T>)
template<class B, class T,size_t E>	class serializer_append<B, std::span<T,E>>
							{	public:	using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const std::span<T,E>& _data) { return _s.template _append_container_list<std::span<T,E>>(_data);}
								template<class S> constexpr static type _do_append(S& _s, std::span<T,E>&& _data) { return _s.template _append_container_list<std::span<T,E>>(_data);}
							};
template<class B, class T,size_t E>	class serializer_extract<B, std::span<T,E>, std::enable_if_t<is_memcopyable<T>::value>>
							{	public:	using type = std::span<T,E>;
								template<class S> constexpr static type _do_extract(S& _s) { return _s.template _extract_container_span<std::span<T,E>>();}
								template<class D, class S> constexpr static void _do_extract(D& _dest, S& _s) { _s.template _extract_container_span<std::span<T,E>>(_dest);}
							};
template<class B, class T,size_t E>	class serializer_extract<B, std::span<T,E>, std::enable_if_t<!is_memcopyable<T>::value>>
							{
							};
template<class B, class T,size_t E>	class serializer_peek<B, std::span<T,E>, std::enable_if_t<is_memcopyable<T>::value>>
							{	public:	using type = std::span<T,E>;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset)	{ return _s.template _front_container_array<std::span<T,E>>(_offset);}
								template<class D, class S> constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset) { _s.template _front_container_array<std::span<T,E>>(_dest, _offset);}
							};
template<class B, class T,size_t E>	class serializer_peek<B, std::span<T,E>, std::enable_if_t<!is_memcopyable<T>::value>>
							{
							};

template<class T,size_t E>	class serializer_size_of<std::span<T,E>>
							{	public:
								constexpr static std::size_t  _get_append_size(const std::span<T,E>& _object)
								{
									return sizeof(COUNT_T) + sizeof(T) * _object.size();
								}
								template<class S> 
								constexpr static std::size_t  _get_extract_size(const S& _buffer, int64_t& _offset) noexcept
								{
									CGDK_ASSERT_ERROR;
									return 0;
								}
							};
#endif
														
//-----------------------------------------------------------------------------
// associated container (set<T>, multiset<T>, ...)
//-----------------------------------------------------------------------------
// set container<T>)
// std::set<T>, std::multiset<T>, ...
template<class B, class T>	class serializer_append<B, T, std::enable_if_t<is_set_container<T>::value>>
							{	public:	using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const T& _data) { return _s.template _append_container_list<T>(_data);}
								template<class S> constexpr static type _do_append(S& _s, T&& _data) { return _s.template _append_container_list<T>(_data);}
							};
template<class B, class T>	class serializer_extract<B, T, std::enable_if_t<is_set_container<T>::value>>
							{	public:	using type = T;
								template<class S> constexpr static type _do_extract(S& _s) { return _s.template _extract_set<T>();}
								template<class D, class S> constexpr static void _do_extract(D& _dest, S& _s) { _s.template _extract_set<T>(_dest);}
							};
template<class B, class T>	class serializer_peek<B, T, std::enable_if_t<is_set_container<T>::value>>
							{	public:	using type = T;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset){ return _s.template _front_set<T>(_offset);}
								template<class D, class S> constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset){ _s.template _front_set<T>(_dest, _offset);}
							};
template<class T>			class serializer_size_of<T, std::enable_if_t<is_set_container<T>::value>>
							{	public:
								constexpr static std::size_t  _get_append_size(const T& _object)
								{
									auto size = sizeof(COUNT_T);

									for (const auto& iter : _object)
									{
										size += get_size_of(iter);
									}

									// return )
									return	size;
								}
								template<class S> 
								constexpr static std::size_t  _get_extract_size(const S& _buffer, int64_t& _offset) noexcept
								{
									CGDK_ASSERT_ERROR;
									return 0;
								}
							};

// std::set<object_ptr<T>>, std::multiset<object_ptr<T>>, ...
template<class B, class T>	class serializer_append<B, T, std::enable_if_t<is_set_object_ptr_container<T>::value>>
							{	public:	using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const T& _data) { return _s.template _append_container_list<T>(_data);}
								template<class S> constexpr static type _do_append(S& _s, T&& _data) { return _s.template _append_container_list<T>(_data);}
							};
template<class B, class T>	class serializer_extract<B, T, std::enable_if_t<is_set_object_ptr_container<T>::value>>
							{	public:	using type = T;
								template<class S> constexpr static type _do_extract(S& _s) { return _s.template _extract_object_ptr_set<T>();}
								template<class D, class S> constexpr static void _do_extract(D& _dest, S& _s) { _s.template _extract_object_ptr_set<T>(_dest);}
							};
template<class B, class T>	class serializer_peek<B, T, std::enable_if_t<is_set_object_ptr_container<T>::value>>
							{	public:	using type = T;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset){ return _s.template _front_object_ptr_set<T>(_offset);}
								template<class D, class S> constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset){ _s.template _front_object_ptr_set<T>(_dest, _offset);}
							};
template<class T>			class serializer_size_of<T, std::enable_if_t<is_set_object_ptr_container<T>::value>>
							{	public:
								constexpr static std::size_t  _get_append_size(const T& _object)
								{
									auto size = sizeof(COUNT_T);
								
									for (const auto& iter : _object)
									{
										size += get_size_of(iter);
									}

									// return )
									return	size;
								}
								template<class S> 
								constexpr static std::size_t  _get_extract_size(const S& _buffer, int64_t& _offset) noexcept
								{
									CGDK_ASSERT_ERROR;
									return 0;
								}
							};


// std::set<own_ptr<T>>, std::multiset<own_ptr<T>>, ...
template<class B, class T>	class serializer_append<B, T, std::enable_if_t<is_set_own_ptr_container<T>::value>>
							{	public:	using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const T& _data) { return _s.template _append_container_list<T>(_data);}
								template<class S> constexpr static type _do_append(S& _s, T&& _data) { return _s.template _append_container_list<T>(_data);}
							};
template<class B, class T>	class serializer_extract<B, T, std::enable_if_t<is_set_own_ptr_container<T>::value>>
							{	public:	using type = T;
								template<class S> constexpr static type _do_extract(S& _s) { return _s.template _extract_own_ptr_set<T>();}
								template<class D, class S> constexpr static void _do_extract(D& _dest, S& _s) { _s.template _extract_own_ptr_set<T>(_dest);}
							};
template<class B, class T>	class serializer_peek<B, T, std::enable_if_t<is_set_own_ptr_container<T>::value>>
							{	public:	using type = T;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset){ return _s.template _front_own_ptr_set<T>(_offset);}
								template<class D, class S> constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset){ _s.template _front_own_ptr_set<T>(_dest, _offset);}
							};
template<class T>			class serializer_size_of<T, std::enable_if_t<is_set_own_ptr_container<T>::value>>
							{	public:
								constexpr static std::size_t  _get_append_size(const T& _object)
								{
									auto size = sizeof(COUNT_T);
								
									for (const auto& iter : _object)
									{
										size += get_size_of(iter);
									}

									// return )
									return	size;
								}
								template<class S> 
								constexpr static std::size_t  _get_extract_size(const S& _buffer, int64_t& _offset) noexcept
								{
									CGDK_ASSERT_ERROR;
									return 0;
								}
							};


//-----------------------------------------------------------------------------
// associated container (std::map<T>, std::multimap<T>, ...)
//-----------------------------------------------------------------------------
// associated container<T>)
template<class B, class T>	class serializer_append<B, T, std::enable_if_t<is_associative_container<T>::value>>
							{	public:	using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const T& _data) { return _s.template _append_container_associative<T>(_data); }
								template<class S> constexpr static type _do_append(S& _s, T&& _data) { return _s.template _append_container_associative<T>(_data); }
							};
template<class B, class T>	class serializer_extract<B, T, std::enable_if_t<is_associative_container<T>::value>>
							{	public:	using type = T;
								template<class S> constexpr static type _do_extract(S& _s) { return _s.template _extract_object_ptr_container_associative<T>(); }
								template<class D, class S> constexpr static void _do_extract(D& _dest, S& _s) { _s.template _extract_object_ptr_container_associative<T>(_dest); }
							};
template<class B, class T>	class serializer_peek<B, T, std::enable_if_t<is_associative_container<T>::value>>
							{	public:	using type = T;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset){ return _s.template _front_container_associative<T>(_offset); }
								template<class D, class S> constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset){ _s.template _front_container_associative<T>(_dest, _offset); }
							};
template<class T>			class serializer_size_of<T, std::enable_if_t<is_associative_container<T>::value>>
							{	public:
								constexpr static std::size_t  _get_append_size(const T& _object)
								{
									auto size = sizeof(COUNT_T);
								
									for (const auto& iter : _object)
									{
										size += get_size_of(iter.first);
										size += get_size_of(iter.second);
									}

									// return )
									return	size;
								}
								template<class S> 
								constexpr static std::size_t  _get_extract_size(const S& _buffer, int64_t& _offset) noexcept
								{
									CGDK_ASSERT_ERROR;
									return 0;
								}
							};


// associated container<object_ptr<T>>)
template<class B, class T>		class serializer_append<B, T, std::enable_if_t<is_associative_object_ptr_container<T>::value>>
							{	public:	using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const T& _data) { return _s.template _append_container_associative<T>(_data);}
								template<class S> constexpr static type _do_append(S& _s, T&& _data) { return _s.template _append_container_associative<T>(_data);}
							};
template<class B, class T>		class serializer_extract<B, T, std::enable_if_t<is_associative_object_ptr_container<T>::value>>
							{	public:	using  type = T;
								template<class S> constexpr static type _do_extract(S& _s) { return _s.template _extract_object_ptr_container_associative<T>();}
								template<class D, class S> constexpr static void _do_extract(D& _dest, S& _s) { _s.template _extract_object_ptr_container_associative<T>(_dest);}
							};
template<class B, class T>		class serializer_peek<B, T, std::enable_if_t<is_associative_object_ptr_container<T>::value>>
							{	public:	using type = T;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset) { return _s.template _front_object_ptr_container_associative<T>(_offset);}
								template<class D, class S> constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset){ _s.template _front_object_ptr_container_associative<T>(_dest, _offset);}
							};
template<typename T>		class serializer_size_of<T, std::enable_if_t<is_associative_object_ptr_container<T>::value>>
							{	public:
								constexpr static std::size_t  _get_append_size(const T& _object)
								{
									auto size = sizeof(COUNT_T);
								
									for (const auto& iter : _object)
									{
										size += get_size_of(iter.first);
										size += get_size_of(iter.second);
									}

									// return )
									return	size;
								}
								template<class S> 
								constexpr static std::size_t  _get_extract_size(const S& _buffer, int64_t& _offset) noexcept
								{
									CGDK_ASSERT_ERROR;
									return 0;
								}
							};


// associated container<own_ptr<T>>)
template<class B, class T>	class serializer_append<B, T, std::enable_if_t<is_associative_own_ptr_container<T>::value>>
							{	public:	using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const T& _data) { return _s.template _append_container_associative<T>(_data);}
								template<class S> constexpr static type _do_append(S& _s, T&& _data) { return _s.template _append_container_associative<T>(_data);}
							};
template<class B, class T>	class serializer_extract<B, T, std::enable_if_t<is_associative_own_ptr_container<T>::value>>
							{	public:	using type = T;
								template<class S> constexpr static type _do_extract(S& _s) { return _s.template _extract_own_ptr_container_associative<T>();}
								template<class D, class S> constexpr static void _do_extract(D& _dest, S& _s) { _s.template _extract_own_ptr_container_associative<T>(_dest);}
							};
template<class B, class T>	class serializer_peek<B, T, std::enable_if_t<is_associative_own_ptr_container<T>::value>>
							{	public:	using type = T;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset) { return _s.template _front_own_ptr_container_associative<T>(_offset);}
								template<class D, class S> constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset) { _s.template _front_own_ptr_container_associative<T>(_dest, _offset);}
							};
template<typename T>		class serializer_size_of<T, std::enable_if_t<is_associative_own_ptr_container<T>::value>>
							{	public:
								constexpr static std::size_t  _get_append_size(const T& _object)
								{
									auto size = sizeof(COUNT_T);
								
									for (const auto& iter : _object)
									{
										size += get_size_of(iter.first);
										size += get_size_of(iter.second);
									}

									// return )
									return	size;
								}
								template<class S> 
								constexpr static std::size_t  _get_extract_size(const S& _buffer, int64_t& _offset) noexcept
								{
									CGDK_ASSERT_ERROR;
									return 0;
								}
							};


//-----------------------------------------------------------------------------
// aggregate structure (reflection)
//-----------------------------------------------------------------------------
template <std::size_t ISIZE>	constexpr std::size_t align_offset_pre     (std::size_t _source, std::size_t _add);
template <>						constexpr std::size_t align_offset_pre< 1> (std::size_t _source, std::size_t) { return _source; }
template <>						constexpr std::size_t align_offset_pre< 2> (std::size_t _source, std::size_t _add) { return (!(_source & 0x01) || _add <= 1) ? _source : _source + 1;}
template <>						constexpr std::size_t align_offset_pre< 4> (std::size_t _source, std::size_t _add) { auto remained = (_source & 0x03); return (remained == 0 || ( 4 - remained) >= _add) ? _source : ((_source & (~0x03)) +  4);}
template <>						constexpr std::size_t align_offset_pre< 8> (std::size_t _source, std::size_t _add) { auto remained = (_source & 0x07); return (remained == 0 || ( 8 - remained) >= _add) ? _source : ((_source & (~0x07)) +  8);}
template <>						constexpr std::size_t align_offset_pre<16> (std::size_t _source, std::size_t _add) { auto remained = (_source & 0x0f); return (remained == 0 || (16 - remained) >= _add) ? _source : ((_source & (~0x0f)) + 16);}
template <>						constexpr std::size_t align_offset_pre<32> (std::size_t _source, std::size_t _add) { auto remained = (_source & 0x1f); return (remained == 0 || (32 - remained) >= _add) ? _source : ((_source & (~0x1f)) + 32);}
template <>						constexpr std::size_t align_offset_pre<64> (std::size_t _source, std::size_t _add) { auto remained = (_source & 0x3f); return (remained == 0 || (64 - remained) >= _add) ? _source : ((_source & (~0x3f)) + 64);}

namespace reflection
{
	// - any construct
	struct any_constructor_lref { std::size_t ignore; template <class Type> /*constexpr*/ constexpr operator Type& () const noexcept; };
	struct any_constructor_rref { std::size_t ignore; template <class Type> /*constexpr*/ constexpr operator Type&& () const noexcept; };

	// - enable_is_constructible?
	template <class T, std::size_t... I, class /*ENABLE*/ = std::enable_if_t<std::is_copy_constructible<T>::value>>
	constexpr auto enable_if_constructible_helper(std::index_sequence<I...>) noexcept -> typename std::add_pointer_t<decltype(T{ any_constructor_lref{I}... })>;
	template <class T, std::size_t... I, class /*ENABLE*/ = std::enable_if_t<!std::is_copy_constructible<T>::value>>
	constexpr auto enable_if_constructible_helper(std::index_sequence<I...>) noexcept -> typename std::add_pointer_t<decltype(T{ any_constructor_rref{I}... })>;
	template <class T, std::size_t N, class /*ENABLE*/ = decltype(enable_if_constructible_helper<T>(std::make_index_sequence<N>())) >
	using enable_if_constructible_helper_t = std::size_t;

	template <class T, std::size_t BEGIN, std::size_t MIDDLE>
	constexpr std::size_t 	detect_fields_count(std::true_type, long) noexcept
	{
		return BEGIN;
	}
	template <class T, std::size_t BEGIN, std::size_t MIDDLE>
	constexpr std::size_t	detect_fields_count(std::false_type, int) noexcept;

	template <class T, std::size_t BEGIN, std::size_t MIDDLE>
	constexpr auto			detect_fields_count(std::false_type, long) noexcept -> enable_if_constructible_helper_t<T, MIDDLE>
	{
		constexpr std::size_t next_v = MIDDLE + (MIDDLE - BEGIN + 1) / 2;
		return detect_fields_count<T, MIDDLE, next_v>(std::integral_constant<bool, MIDDLE == next_v>{}, 1L);
	}
	template <class T, std::size_t BEGIN, std::size_t MIDDLE>
	constexpr std::size_t	detect_fields_count(std::false_type, int) noexcept
	{
		constexpr std::size_t next_v = BEGIN + (MIDDLE - BEGIN) / 2;
		return detect_fields_count<T, BEGIN, next_v>(std::integral_constant<bool, BEGIN == next_v>{}, 1L);
	}

	template <class T>
	constexpr std::size_t	get_fields_count() noexcept
	{
		using type = std::remove_const_t<T>;
		constexpr std::size_t max_fields_count = sizeof(type);
		constexpr std::size_t middle = max_fields_count / 2 + 1;
		return detect_fields_count<T, 0, middle>(std::false_type{}, 1L);
	}
}

template <class T>
constexpr std::size_t constexpr_field_count = std::integral_constant<std::size_t, reflection::get_fields_count<T>()>::value;


//-----------------------------------------------------------------------------
// functions
//-----------------------------------------------------------------------------
template <class T>
constexpr std::basic_string<T>	_float_to_string(double _value);
template <>
inline std::basic_string<char> _float_to_string<char>(double _value)
{
#if defined(_STRING_) || defined(_GLIBCXX_STRING)
	return std::to_string(_value);
#else
	static_assert(false, "<string> is not included. include <string>");
	CGDK_ASSERT(false);
	throw std::exception(reinterpret_cast<const char*>("<string> is not included. include <string>"));
#endif
}
template <>
inline std::basic_string<wchar_t> _float_to_string<wchar_t>(double _value)
{
#if defined(_STRING_) || defined(_GLIBCXX_STRING)
	return std::to_wstring(_value);
#else
	static_assert(false, "<string> is not included. include <string>");
	CGDK_ASSERT(false);
	throw std::exception(reinterpret_cast<const char*>("<string> is not included. include <string>"));
#endif
}



//-----------------------------------------------------------------------------
//   get_type_hash_of<T>()
//
//  - hash of the serialized format of T (not of a value)
//    ex) if (get_type_hash_of<foo>() != hash_received) -> the peer has a different foo
//  - 32 bits (default) or 64 bits
//    ex) get_type_hash_of<foo>(), get_type_hash_of<foo, uint64_t>()
//  - constexpr, and the same value on every compiler/platform for the same format
//    (wchar_t is 2 bytes on Windows and 4 bytes on Linux, so std::wstring differs)
//  - the hash follows the serialized format, not the C++ type
//    ex) std::vector<int>, std::list<int>, std::set<int>	-> same (COUNT + elements)
//        std::string, std::string_view, const char*		-> same
//        T*, object_ptr<T>, own_ptr<T>, std::shared_ptr<T> -> same (nullable T)
//  - Ibuffer_serializable : member types of DEFINE_BUFFER_SERIALIZE(...)
//    struct serializable  : member types (ENABLE_STRUCT_SERIALIZABLE)
//    memcpy-able class    : size (serialized as raw bytes)
//  - other types : specialize serializer_hash<T>
//    ex) template<> class CGDK::serializer_hash<foo>
//        { public: template<class... V> static constexpr uint64_t _get_hash() noexcept { return ...; } };
//-----------------------------------------------------------------------------
namespace _buffer_hash
{
	constexpr uint64_t FNV_OFFSET = 0xcbf29ce484222325ull;
	constexpr uint64_t FNV_PRIME = 0x00000100000001b3ull;
	constexpr std::size_t NPOS = static_cast<std::size_t>(-1);

	enum : uint64_t
	{
		KIND_BOOL = 1,
		KIND_CHAR,
		KIND_INT,
		KIND_UINT,
		KIND_FLOAT,
		KIND_ENUM,
		KIND_RAW,
		KIND_STRING,
		KIND_TEXT,
		KIND_BUFFER,
		KIND_TUPLE,
		KIND_ARRAY,
		KIND_SEQUENCE,
		KIND_MAP,
		KIND_STRUCT,
		KIND_OBJECT,
		KIND_NULLABLE,
		KIND_STATIC_STRING,
		KIND_RECURSION
	};

	// - FNV-1a (8 bytes, little endian)
	[[nodiscard]] constexpr uint64_t mix(uint64_t _hash, uint64_t _value) noexcept
	{
		for (int i = 0; i < 8; ++i)
		{
			_hash ^= (_value >> (i * 8)) & 0xff;
			_hash *= FNV_PRIME;
		}
		return _hash;
	}
	[[nodiscard]] constexpr uint64_t mix(uint64_t _hash, uint64_t _v1, uint64_t _v2) noexcept { return mix(mix(_hash, _v1), _v2); }
	[[nodiscard]] constexpr uint64_t mix(uint64_t _hash, uint64_t _v1, uint64_t _v2, uint64_t _v3) noexcept { return mix(mix(mix(_hash, _v1), _v2), _v3); }

	// - murmur3 fmix64
	[[nodiscard]] constexpr uint64_t finalize(uint64_t _hash) noexcept
	{
		_hash ^= _hash >> 33;
		_hash *= 0xff51afd7ed558ccdull;
		_hash ^= _hash >> 33;
		_hash *= 0xc4ceb9fe1a85ec53ull;
		_hash ^= _hash >> 33;
		return _hash;
	}

	// - index of T in V... (NPOS if not found)
	template <class T, class... V>
	[[nodiscard]] constexpr std::size_t index_of() noexcept
	{
		std::size_t index = 0;
		std::size_t found = NPOS;
		((found == NPOS && std::is_same_v<T, V> ? void(found = index) : void(), ++index), ...);
		return found;
	}

	template <class> constexpr bool dependent_false = false;

	template <class T> struct is_char : public std::bool_constant<std::is_same_v<T, char> || std::is_same_v<T, wchar_t> || std::is_same_v<T, char8_t> || std::is_same_v<T, char16_t> || std::is_same_v<T, char32_t>> {};

	template <class T> struct is_std_array : public std::false_type {};
	template <class T, std::size_t N> struct is_std_array<std::array<T, N>> : public std::true_type { using element_type = T; static constexpr std::size_t size = N; };

	// - T*, object_ptr<T>, own_ptr<T>, std::shared_ptr<T>, std::unique_ptr<T> (T : Ibuffer_serializable)
	template <class T> struct nullable_element { using type = void; };
	template <class T> struct nullable_element<T*> { using type = std::remove_cv_t<T>; };
	template <class T> struct nullable_element<object_ptr<T>> { using type = std::remove_cv_t<T>; };
	template <class T> struct nullable_element<own_ptr<T>> { using type = std::remove_cv_t<T>; };
	template <class T> struct nullable_element<std::shared_ptr<T>> { using type = std::remove_cv_t<T>; };
	template <class T, class D> struct nullable_element<std::unique_ptr<T, D>> { using type = std::remove_cv_t<T>; };
	template <class T> using nullable_element_t = typename nullable_element<T>::type;
	template <class T> constexpr bool is_nullable_serializable_v = std::is_base_of_v<Ibuffer_serializable, nullable_element_t<T>>;

	template <class T> struct is_span : public std::false_type {};
#if defined(__cpp_lib_span)
	template <class T, std::size_t E> struct is_span<std::span<T, E>> : public std::true_type {};
#endif
}

template <class T, class... V>
[[nodiscard]] constexpr uint64_t _get_type_hash_of() noexcept;

// - hash of the member type list
template <class... V, class... A>
[[nodiscard]] constexpr uint64_t _get_hash_list(uint64_t _hash, std::type_identity<_type_list<A...>>) noexcept
{
	((_hash = _buffer_hash::mix(_hash, _get_type_hash_of<std::remove_cvref_t<A>, V...>())), ...);
	return _buffer_hash::mix(_hash, sizeof...(A));
}

// - member types of the aggregate structure
//   1) T{ any_constructor_type<T, I>{}... } is tried in an unevaluated context (like detect_fields_count)
//   2) the conversion operator deduces the type U of the member I and its default template argument
//      instantiates member_type_recorder<T, I, U>, which defines the friend function loophole_member_type(member_tag<T, I>)
//   3) the return type of loophole_member_type(member_tag<T, I>) is the type of the member I
namespace reflection
{
#if defined(__GNUC__) && !defined(__clang__)
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-template-friend"
#endif
	template <class T, std::size_t INDEX>
	struct member_tag
	{
		friend constexpr auto loophole_member_type(member_tag<T, INDEX>) noexcept;
	};
#if defined(__GNUC__) && !defined(__clang__)
	#pragma GCC diagnostic pop
#endif

	template <class T, std::size_t INDEX, class U>
	struct member_type_recorder
	{
		friend constexpr auto loophole_member_type(member_tag<T, INDEX>) noexcept { return std::type_identity<U>{}; }
	};

	template <class T, std::size_t INDEX>
	struct any_constructor_type
	{
		std::size_t ignore;
		template <class U, std::size_t = sizeof(member_type_recorder<T, INDEX, std::remove_cvref_t<U>>)>
		constexpr operator U () const noexcept;
	};

	template <class T, class ISEQ> struct member_types_impl;
	template <class T, std::size_t... I>
	struct member_types_impl<T, std::index_sequence<I...>>
		: public std::type_identity<decltype(T{ any_constructor_type<T, I>{I}... })>	// records the member types (base class first)
	{
		using type = std::type_identity<_type_list<typename decltype(loophole_member_type(member_tag<T, I>{}))::type...>>;
	};

	template <class T>
	using member_types_t = typename member_types_impl<T, std::make_index_sequence<constexpr_field_count<T>>>::type;
}

// - hash rules
template <class T, class FLAG>
class serializer_hash
{
public:
	template <class... V>
	[[nodiscard]] static constexpr uint64_t _get_hash() noexcept
	{
		using namespace _buffer_hash;

		// 1) const char*, const wchar_t*, ...
		if constexpr (is_string_pointer_type_v<T>)
		{
			return mix(FNV_OFFSET, KIND_STRING, sizeof(std::remove_cv_t<std::remove_pointer_t<T>>));
		}
		// 2) T*, object_ptr<T>, own_ptr<T>, std::shared_ptr<T>, std::unique_ptr<T> (nullable)
		//    - checked before std::is_base_of_v<> (object_ptr<T> can be incomplete)
		else if constexpr (is_nullable_serializable_v<T>)
		{
			return mix(FNV_OFFSET, KIND_NULLABLE, _get_type_hash_of<nullable_element_t<T>, V...>());
		}
		// 3) Ibuffer_serializable (DEFINE_BUFFER_SERIALIZE)
		else if constexpr (std::is_base_of_v<Ibuffer_serializable, T>)
		{
			static_assert(_buffer_serialize_access::has_types<T>, "get_type_hash_of: Ibuffer_serializable needs DEFINE_BUFFER_SERIALIZE(...) or a specialization of serializer_hash<T>");
			return _get_hash_list<V...>(mix(FNV_OFFSET, KIND_OBJECT), _buffer_serialize_access::types<T>{});
		}
		// 4) std::basic_string<T>, std::basic_string_view<T>
		else if constexpr (is_std_string_v<T> || is_std_string_view_v<T>)
		{
			return mix(FNV_OFFSET, KIND_STRING, sizeof(typename T::value_type));
		}
		// 5) T[N] (char[N] is serialized as a string)
		else if constexpr (std::is_array_v<T>)
		{
			using element_t = std::remove_cv_t<std::remove_extent_t<T>>;
			if constexpr (is_string_type_v<element_t>)
				return mix(FNV_OFFSET, KIND_STRING, sizeof(element_t));
			else if constexpr (std::extent_v<T> != 0)
				return mix(FNV_OFFSET, KIND_ARRAY, std::extent_v<T>, _get_type_hash_of<element_t, V...>());
			else
				return mix(FNV_OFFSET, KIND_SEQUENCE, _get_type_hash_of<element_t, V...>());
		}
		// 6) std::array<T,N>
		else if constexpr (is_std_array<T>::value)
		{
			return mix(FNV_OFFSET, KIND_ARRAY, is_std_array<T>::size, _get_type_hash_of<typename is_std_array<T>::element_type, V...>());
		}
		// 7) text<T> (no length)
		else if constexpr (is_text_v<T>)
		{
			return mix(FNV_OFFSET, KIND_TEXT, sizeof(typename T::value_type));
		}
		// 8) buffer_view, buffer, shared_buffer
		else if constexpr (is_buffer_type_v<T>)
		{
			return mix(FNV_OFFSET, KIND_BUFFER);
		}
		// 9) std::tuple<T...>
		else if constexpr (is_tuple_v<T>)
		{
			return _get_hash_list<V...>(mix(FNV_OFFSET, KIND_TUPLE), []<class... A>(std::type_identity<std::tuple<A...>>) { return std::type_identity<_type_list<A...>>{}; }(std::type_identity<T>{}));
		}
	#if defined(__CGDK_STATIC_STRING)
		// 10) static_string<N>
		else if constexpr (is_static_string_v<T>)
		{
			return mix(FNV_OFFSET, KIND_STATIC_STRING, sizeof(typename T::value_type), sizeof(T));
		}
	#endif
		// 11) linear/set containers, std::span (COUNT + elements)
		else if constexpr (is_span<T>::value
						|| is_linear_container<T>::value || is_linear_object_ptr_container<T>::value || is_linear_own_ptr_container<T>::value
						|| is_linear_container_with_reserve<T>::value || is_linear_object_ptr_container_with_reserve<T>::value || is_linear_own_ptr_container_with_reserve<T>::value
						|| is_set_container<T>::value || is_set_object_ptr_container<T>::value || is_set_own_ptr_container<T>::value)
		{
			return mix(FNV_OFFSET, KIND_SEQUENCE, _get_type_hash_of<typename T::value_type, V...>());
		}
		// 12) associative containers (COUNT + key/value)
		else if constexpr (is_associative_container<T>::value || is_associative_object_ptr_container<T>::value || is_associative_own_ptr_container<T>::value)
		{
			if constexpr (has_mapped_type<T>::value)
				return mix(FNV_OFFSET, KIND_MAP, _get_type_hash_of<typename T::key_type, V...>(), _get_type_hash_of<typename T::mapped_type, V...>());
			else
				return mix(FNV_OFFSET, KIND_SEQUENCE, _get_type_hash_of<typename T::value_type, V...>());
		}
		// 13) struct serializable (ENABLE_STRUCT_SERIALIZABLE)
		else if constexpr (is_struct_serializable_v<T>)
		{
			return _get_hash_list<V...>(mix(FNV_OFFSET, KIND_STRUCT), reflection::member_types_t<T>{});
		}
		// 14) bool
		else if constexpr (std::is_same_v<T, bool>)
		{
			return mix(FNV_OFFSET, KIND_BOOL);
		}
		// 15) char, wchar_t, char8_t, char16_t, char32_t
		else if constexpr (is_char<T>::value)
		{
			return mix(FNV_OFFSET, KIND_CHAR, sizeof(T));
		}
		// 16) integer
		else if constexpr (std::is_integral_v<T>)
		{
			return mix(FNV_OFFSET, std::is_signed_v<T> ? KIND_INT : KIND_UINT, sizeof(T));
		}
		// 17) floating point
		else if constexpr (std::is_floating_point_v<T>)
		{
			return mix(FNV_OFFSET, KIND_FLOAT, sizeof(T));
		}
		// 18) enum
		else if constexpr (std::is_enum_v<T>)
		{
			return mix(FNV_OFFSET, KIND_ENUM, _get_type_hash_of<std::underlying_type_t<T>, V...>());
		}
		// 19) other memcpy-able types (raw bytes)
		else if constexpr (is_memcopyable<T>::value)
		{
			return mix(FNV_OFFSET, KIND_RAW, sizeof(T));
		}
		else
		{
			static_assert(dependent_false<T>, "get_type_hash_of: no hash rule for T. specialize CGDK::serializer_hash<T>");
			return 0;
		}
	}
};

// - hash of T (V... : types being hashed, for the recursive types)
template <class T, class... V>
[[nodiscard]] constexpr uint64_t _get_type_hash_of() noexcept
{
	using TX = std::remove_cvref_t<T>;

	constexpr auto index = _buffer_hash::index_of<TX, V...>();

	if constexpr (index != _buffer_hash::NPOS)
		return _buffer_hash::mix(_buffer_hash::FNV_OFFSET, _buffer_hash::KIND_RECURSION, index);
	else
		return serializer_hash<TX>::template _get_hash<V..., TX>();
}

template <class T, class TRETURN = uint32_t>
[[nodiscard]] constexpr TRETURN get_type_hash_of() noexcept
{
	static_assert(std::is_unsigned_v<TRETURN> && (sizeof(TRETURN) == 4 || sizeof(TRETURN) == 8), "get_type_hash_of: TRETURN must be uint32_t or uint64_t");

	constexpr uint64_t hash = _buffer_hash::finalize(_get_type_hash_of<T>());

	if constexpr (sizeof(TRETURN) == 4)
		return static_cast<TRETURN>(hash ^ (hash >> 32));
	else
		return static_cast<TRETURN>(hash);
}

}
