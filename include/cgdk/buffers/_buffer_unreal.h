//*****************************************************************************
//*                                                                           *
//*                               CGDK::buffer                                *
//*                       ver 3.03 / release 2023.10.17                       *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangduck@cgcii.com           *
//*  Best for Game Developement and Optimized for Game Developement.          *
//*                                                                           *
//*                   (C) 2008 CGLabs All right reserved.                     *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************

#pragma once
//-----------------------------------------------------------------------------
//
// Includes
//
//-----------------------------------------------------------------------------
#if defined(UE_BUILD_DEBUG) || defined(UE_BUILD_DEVELOPMENT) || defined(UE_BUILD_TEST) || defined(UE_BUILD_SHIPPING)

// FString
class CORE_API FString;

// FStringView
template <typename CharType> 
class TStringViewImpl;


// array - TArray, FArchiveDiffMap, FArrayReader, ... , VectorND
template<typename InElementType, typename InAllocator>
class TArray;

// array - TStaticArray, TShaderResourceParameterArray
template<typename InElementType, uint32 NumElements, uint32 Alignment>
class TStaticArray;

// array - TSparseArray
template<typename InElementType, typename Allocator>
class TSparseArray;

// singly linked list - TList
template<class ElementType>
class TList;

// doubly linked list - TLinkedList, TIntrusiveLinkedList, TLinkedList
template<class ContainerType, class ElementType, template< class, class > class IteratorType>
class TLinkedListBase;
template <class ElementType>
class TIntrusiveLinkedList;

// doubly linked list - TDoubleLinkedList
template <class ElementType>
class TDoubleLinkedList;


// set - TSet, 	FTouchKeySet
template<typename InElementType, typename KeyFuncs, typename Allocator>
class TSet;

// map - TSortableMapBase, TMap, TMultiMap
template<typename KeyType, typename ValueType, typename SetAllocator, typename KeyFuncs>
class TMapBase;

// sorted map - TSortedMap, FAssetDataTagMap
template<typename KeyType, typename ValueType, typename ArrayAllocator, typename SortPredicate>
class TSortedMap;


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
template<class A> struct CGDK::is_object_ptr_type<TSharedPtr<A>> : public std::true_type {};
template<class A> struct CGDK::is_object_ptr_type<TSharedRef<A>> : public std::true_type {};

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
// 1) ue4 FString
template<class T, class F = void> struct is_ue4_FString : public std::false_type {};
template<class T> struct is_ue4_FString<T, std::enable_if_t<std::is_base_of_v<FString, T>>> : public std::true_type {};
template <class T> constexpr bool is_ue4_FString_v = is_ue4_FString<T>::value;

// 2) ue4 FStringView
template<class T, class F = void> struct is_ue4_FStringView : public std::false_type {};
template<class A> struct is_ue4_FStringView<TStringViewImpl<A>, void> : public std::true_type {};
template<>        struct is_ue4_FStringView<FStringView, void> : public std::true_type {};
template <class T> constexpr bool is_ue4_FStringView_v = is_ue4_FStringView<T>();

// 3) array - TArray, FArchiveDiffMap, FArrayReader, ... , VectorND
template<class T, class F = void> struct is_ue4_TArray : public std::false_type {};
template<typename A1, typename A2> struct is_ue4_TArray<TArray<A1,A2>, void> : public std::true_type {};
template <class T> constexpr bool is_ue4_TArray_v = is_ue4_TArray<T>::value;

// 4) array - TStaticArray, TShaderResourceParameterArray
template<class T, class F = void> struct is_ue4_TStaticArray : public std::false_type {};
template<typename A1, uint32_t A2, uint32_t A3> struct is_ue4_TStaticArray<TStaticArray<A1,A2,A3>, void> : public std::true_type {};
template <class T> constexpr bool is_ue4_TStaticArray_v = is_ue4_TStaticArray<T>::value;

// 5) array - TSparseArray
template<class T, class F = void> struct is_ue4_TSparseArray : public std::false_type {};
template<typename A1, typename A2> struct is_ue4_TSparseArray<TSparseArray<A1,A2>, void> : public std::true_type {};
template <class T> constexpr bool is_ue4_TSparseArray_v = is_ue4_TSparseArray<T>::value;

// 6) singly linked list - TList
template<class T, class F = void> struct is_ue4_TList : public std::false_type {};
template<typename A1> struct is_ue4_TList<TList<A1>, void> : public std::true_type {};
template <class T> constexpr bool is_ue4_TList_v = is_ue4_TList<T>::value;

// 7) doubly linked list - TLinkedList, TIntrusiveLinkedList
template<class T, class F = void> struct is_ue4_TLinkedListBase : public std::false_type {};
template<class A1, class A2, template< class, class > class A3> struct is_ue4_TLinkedListBase<TLinkedListBase<A1, A2, A3>, void> : public std::true_type {};
template<class A1> struct is_ue4_TLinkedListBase<TLinkedList<A1>, void> : public std::true_type {};
template<class A1> struct is_ue4_TLinkedListBase<TIntrusiveLinkedList<A1>, void> : public std::true_type {};
template <class T> constexpr bool is_ue4_TLinkedListBase_v = is_ue4_TLinkedListBase<T>::value;

// 8) set - TSet, 	FTouchKeySet
template<class T, class F = void> struct is_ue4_TSet : public std::false_type {};
template<typename A1, typename A2, typename A3> struct is_ue4_TSet<TSet<A1, A2, A3>, void> : public std::true_type {};
template<>			struct is_ue4_TSet<FTouchKeySet, void> : public std::true_type {};
template <class T> constexpr bool is_ue4_TSet_v = is_ue4_TSet<T>::value;

// 9) map - TSortableMapBase, TMap, TMultiMap
template<class T, class F = void> struct is_ue4_TMapBase : public std::false_type {};
template<typename A1, typename A2, typename A3, typename A4> struct is_ue4_TMapBase<TSortableMapBase<A1,A2,A3,A4>, void> : public std::true_type {};
template<typename A1, typename A2, typename A3, typename A4> struct is_ue4_TMapBase<TMap<A1, A2, A3, A4>, void> : public std::true_type {};
template<typename A1, typename A2, typename A3, typename A4> struct is_ue4_TMapBase<TMultiMap<A1, A2, A3, A4>, void> : public std::true_type {};
template <class T> constexpr bool is_ue4_TMapBase_v = is_ue4_TMapBase<T>::value;

// 10) sorted map - TSortedMap, FAssetDataTagMap
template<class T, class F = void> struct is_ue4_TSortedMap : public std::false_type {};
template<typename A1, typename A2, typename A3, typename A4> struct is_ue4_TSortedMap<TSortedMap<A1, A2, A3, A4>, void> : public std::true_type {};
template<>													 struct is_ue4_TSortedMap<FAssetDataTagMap, void> : public std::true_type {};
template <class T> constexpr bool is_ue4_TSortedMap_v = is_ue4_TSortedMap<T>::value;

// 11) element type - TList and TLinkedListBase have no ElementType typedef
template<class T, class F = void> struct ue4_element_type { using type = typename T::ElementType; };
template<class A1> struct ue4_element_type<TList<A1>, void> { using type = A1; };
template<class A1, class A2, template< class, class > class A3> struct ue4_element_type<TLinkedListBase<A1, A2, A3>, void> { using type = A2; };
template<class A1> struct ue4_element_type<TLinkedList<A1>, void> { using type = A1; };
template<class A1> struct ue4_element_type<TIntrusiveLinkedList<A1>, void> { using type = A1; };
template <class T> using ue4_element_type_t = typename ue4_element_type<T>::type;


//-----------------------------------------------------------------------------
//
// Includes
//
//-----------------------------------------------------------------------------
namespace CGDK
{



template<class B, class S, class T>
_buffer_view<typename B::element_t> _ue4_append_FStringView(S& _s, const TStringViewImpl<T>& _object)
{
	using RET_T = _buffer_view<typename B::element_t>;

	// 1) [목표_포인터]를 구한다.
	auto buf_dest = _s.data() + _s.size();

	// 2) get values
	auto length_string = _object.Len();	// strnlen(, _buffer_size)
	auto bytes_copy = length_string * sizeof(T);
	auto added_length = sizeof(COUNT_T) + sizeof(T) + bytes_copy;

	// check) upper bound
	_CGD_BUFFER_BOUND_CHECK((buf_dest + added_length) <= _s.get_upper_bound());

	// declare) 
	auto buf_now = buf_dest;

	// 3) [문자열_길이]를 저장한다. (NULL포함)
	*reinterpret_cast<COUNT_T*>(buf_now) = static_cast<COUNT_T>(length_string + 1);
	buf_now += sizeof(COUNT_T);

	// 4) copy string
	if (bytes_copy != 0)
	{
		memcpy(buf_now, _object.GetData(), bytes_copy);
		buf_now += bytes_copy;
	}

	// 5) NULL을 추가한다.
	*reinterpret_cast<T*>(buf_now) = 0;

	// 6) [버퍼_길이]를 업데이트한다.
	_s.add_size(added_length);

	// return)
	return _buffer_view<typename B::element_t>(buf_dest, added_length);
}

template<class T, class S>
inline T _ue4_extract_FStringView(S& _s)
{
	// 1) [데이터_크기]를 얻어온다.(COUNT_T[문자열_길이]+length(String))
	COUNT_T	str_length = _s.template _get_front_string_length<typename T::ElementType>();

	// check) length가 0보다 작으면 그냥 리턴한다.
	if (str_length == COUNT_T(-1))
	{
		_s += offset(sizeof(COUNT_T));
		return T();
	}

	// 2) 문자열의 제일 앞 포인터를 얻어둔다.
	auto str_string = _s.template data<typename T::ElementType>(sizeof(COUNT_T));

	// 2) sizeOfData 구하기
	auto size_data = static_cast<COUNT_T>(str_length * sizeof(typename T::ElementType) + sizeof(COUNT_T));

	// 3) [원본_버퍼_포인터]와 [원본_버퍼_길이]를 [데이터_크기]만큼 이동시킨다. 
	_s += offset(size_data);

	// return) [문자열_시작_포인터]을 리턴한다.
	return T(str_string, static_cast<int32>(str_length) - 1);
}
inline size_t _ue4_size_of_FStringView(const FStringView& _object)
{
	return sizeof(COUNT_T) + (_object.Len() + 1) * sizeof(TCHAR);
}

//-----------------------------------------------------------------------------
// 1) ptrs
//-----------------------------------------------------------------------------
// 2A) TSharedPtr<T>- Ibuffer_serializable
template<class B, class T>	class serializer_append<B, TSharedPtr<T>, std::enable_if_t<std::is_base_of_v<Ibuffer_serializable, T>>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const TSharedPtr<TX>& _data)
								{
									auto len_pre = _s.size();
									
									if(_data != nullptr)
										const_cast<TX*>(_data.Get())->serialize_out(_s);
									else
										_s.template _append<COUNT_T>(COUNT_T(0)-1);
									
									return type(_s.data() + len_pre, _s.size() - len_pre);
								}
							};
template<class B, class T>	class serializer_append<B, TSharedPtr<T>, std::enable_if_t<!std::is_base_of_v<Ibuffer_serializable, T>>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const TSharedPtr<TX>& _data)
								{
									auto len_pre = _s.size();
									
									if(_data != nullptr)
										_s.template append<TX>(*_data);
									else
										_s.template _append<COUNT_T>(COUNT_T(0)-1);
									
									return type(_s.data() + len_pre, _s.size() - len_pre);
								}
							};

template<class B, class T>	class serializer_extract<B, TSharedPtr<T>, std::enable_if_t<std::is_base_of_v<Ibuffer_serializable, T>>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = TSharedPtr<TX>;
								template<class S> constexpr static type _do_extract(S& _s) 
								{
									auto empty = _s.template front<COUNT_T>();

									if (empty == static_cast<COUNT_T>(-1))
									{
										_s.template extract<COUNT_T>();
										return {};
									}

									TSharedPtr<TX> t = MakeShared<TX>();
									t->serialize_in(_s);
									return t;
								}
								template<class D, class S> constexpr static void _do_extract(D& _dest, S& _s) 
								{
									_dest = MakeShared<TX>();
									_dest->serialize_in(_s);
								}
							};
template<class B, class T>	class serializer_extract<B, TSharedPtr<T>, std::enable_if_t<!std::is_base_of_v<Ibuffer_serializable, T>>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = TSharedPtr<TX>;
								template<class S> constexpr static type _do_extract(S& _s) 
								{
									auto empty = _s.template front<COUNT_T>();

									if (empty == static_cast<COUNT_T>(-1))
									{
										_s.template extract<COUNT_T>();
										return{};
									}

									return MakeShared<TX>(_s.template extract<TX>());
								}
								template<class D, class S> constexpr static void _do_extract(D& _dest, S& _s) 
								{
									auto empty = _s.template front<COUNT_T>();

									if (empty == static_cast<COUNT_T>(-1))
									{
										_dest = D{};
										_s.template extract<COUNT_T>();
										return;
									}

									_dest = MakeShared<TX>(_s.template extract<TX>());
								}
							};

template<class B, class T>	class serializer_peek<B, TSharedPtr<T>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = TSharedPtr<TX>;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset)
								{
									S tb =_s + static_cast<std::size_t>(_offset);
									return MakeShared<TX>(tb.template extract<TSharedPtr<T>>());
								}
								template<class D, class S> constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset)
								{
									S tb =_s + static_cast<std::size_t>(_offset);
									_dest = tb.template extract<TSharedPtr<T>>();
								}
							};
template<class T>			class serializer_size_of<TSharedPtr<T>>
							{	using TX = std::remove_const_t<T>;
								using type = TSharedPtr<TX>;
								public:
								constexpr static std::size_t  _get_append_size(const type& _object) 
								{
									if (_object == nullptr)
										return sizeof(COUNT_T);

									return _object->get_size_of();
								}
								template<class S> 
								constexpr static std::size_t  _get_extract_size(const S& _buffer, int64_t& _offset) noexcept { CGDK_ASSERT_ERROR; return 0;}
							};

// 2A) TSharedRef<T>- Ibuffer_serializable
template<class B, class T>	class serializer_append<B, TSharedRef<T>, std::enable_if_t<std::is_base_of_v<Ibuffer_serializable, T>>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const TSharedRef<T>& _data) { auto len_pre = _s.size(); if(_data != nullptr) {const_cast<TX*>(_data)->serialize_out(_s);} else { _s.template _append<COUNT_T>(COUNT_T(0)-1);} return type(_s.data() + len_pre, _s.size() - len_pre);}
							};
template<class B, class T>	class serializer_extract<B, TSharedRef<T>, std::enable_if_t<std::is_base_of_v<Ibuffer_serializable, T>>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = TSharedRef<TX>;
								template<class S> constexpr static type _do_extract(S& _s) { TSharedRef<TX> t = MakeShared<TX>(); t->serialize_in(_s); return t;}
								template<class D, class S> constexpr static void _do_extract(D& _dest, S& _s) { _dest = MakeShared<TX>(); _dest->serialize_in(_s);}
							};
template<class B, class T>	class serializer_peek<B, TSharedRef<T>, std::enable_if_t<std::is_base_of_v<Ibuffer_serializable, T>>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = TSharedRef<TX>;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset) { S tb =_s + static_cast<std::size_t>(_offset); type tx; tx->serialize_in(tb); return tx;}
								template<class D, class S> constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset) { S tb =_s + static_cast<std::size_t>(_offset); _dest->serialize_in(tb);}
							};
template<class T>			class serializer_size_of<TSharedRef<T>, std::enable_if_t<std::is_base_of_v<Ibuffer_serializable, T>>>
							{	using TX = std::remove_const_t<T>;
								using type = TSharedRef<TX>;
								public:
								constexpr static std::size_t  _get_append_size(const type& _object) { return _object->get_size_of(); }
								template<class S> 
								constexpr static std::size_t  _get_extract_size(const S& _buffer, int64_t& _offset) noexcept { CGDK_ASSERT_ERROR; return 0;}
							};

// 2C) TObjectPtr<T>- Ibuffer_serializable
template<class B, class T>	class serializer_append<B, TObjectPtr<T>, std::enable_if_t<std::is_base_of_v<Ibuffer_serializable, T>>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const TObjectPtr<T>& _data) { auto len_pre = _s.size(); if(_data != nullptr) {const_cast<TX*>(_data)->serialize_out(_s);} else { _s.template _append<COUNT_T>(COUNT_T(0)-1);} return type(_s.data() + len_pre, _s.size() - len_pre);}
							};
template<class B, class T>	class serializer_extract<B, TObjectPtr<T>, std::enable_if_t<std::is_base_of_v<Ibuffer_serializable, T>>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = TObjectPtr<TX>;
								template<class S> constexpr static type _do_extract(S& _s) { TObjectPtr<TX> t = NewObject<TX>(); t->serialize_in(_s); return t;}
								template<class D, class S> constexpr static void _do_extract(D& _dest, S& _s) { _dest = NewObject<TX>(); _dest->serialize_in(_s);}
							};
template<class B, class T>	class serializer_peek<B, TObjectPtr<T>, std::enable_if_t<std::is_base_of_v<Ibuffer_serializable, T>>>
							{	using TX = std::remove_const_t<T>;
								public:	using type = TObjectPtr<TX>;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset) { S tb =_s + static_cast<std::size_t>(_offset); type tx; tx->serialize_in(tb); return tx;}
								template<class D, class S> constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset) { S tb =_s + static_cast<std::size_t>(_offset); _dest->serialize_in(tb);}
							};
template<class T>			class serializer_size_of<TObjectPtr<T>, std::enable_if_t<std::is_base_of_v<Ibuffer_serializable, T>>>
							{	using TX = std::remove_const_t<T>;
								using type = TObjectPtr<TX>;
								public:
								constexpr static std::size_t  _get_append_size(const type& _object) { return _object->get_size_of(); }
								template<class S> 
								constexpr static std::size_t  _get_extract_size(const S& _buffer, int64_t& _offset) noexcept { CGDK_ASSERT_ERROR; return 0;}
							};


//-----------------------------------------------------------------------------
// 1) ue4 - FString
//-----------------------------------------------------------------------------
template<class B, class T> class serializer_append<B, T, std::enable_if_t<is_ue4_FString_v<T>>>
							{	public: using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const FString& _data)	{ return _ue4_append_FStringView<B, S, TCHAR>(_s, _data);}
								template<class S> constexpr static type _do_append(S& _s, FString&& _data)			{ return _ue4_append_FStringView<B, S, TCHAR>(_s, _data);}
							};
template<class B, class T> class serializer_extract<B, T, std::enable_if_t<is_ue4_FString_v<T>>>
							{	public: using type = std::remove_const_t<T>;
								template<class S> constexpr static type _do_extract(S& _s)							{ return _ue4_extract_FStringView<type>(_s); }
							};
template<class B, class T> class serializer_peek<B, T, std::enable_if_t<is_ue4_FString_v<T>>>
							{	public:	using type = std::remove_const_t<T>;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset)	{ S tb =_s + _offset; auto temp = _ue4_extract_FStringView<type>(tb); _offset = tb.data() - _s.data(); return temp;}
							};
template<class T>		    class serializer_size_of<T, std::enable_if_t<is_ue4_FString_v<T>>>
							{	public:
								constexpr static std::size_t  _get_append_size(const FString& _object)					{ return _ue4_size_of_FStringView(_object); }
							};

//-----------------------------------------------------------------------------
// 2) ue4 - FStringView
//-----------------------------------------------------------------------------
template<class B, class T> class serializer_append<B, T, std::enable_if_t<is_ue4_FStringView_v<T>>>
							{	public: using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const FStringView& _data){ return _ue4_append_FStringView<B>(_s, _data);}
								template<class S> constexpr static type _do_append(S& _s, FStringView&& _data)		{ return _ue4_append_FStringView<B>(_s, _data);}
							};
template<class B, class T> class serializer_extract<B, T, std::enable_if_t<is_ue4_FStringView_v<T>>>
							{	using TX = std::remove_const_t<T>;
								public: using type = TX;
								template<class S> constexpr static type _do_extract(S& _s)							{ return _ue4_extract_FStringView<type>(_s); }
							};
template<class B, class T> class serializer_peek<B, T, std::enable_if_t<is_ue4_FStringView_v<T>>>
							{	using TX = std::remove_const_t<T>;
								public: using type = TX;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset)	{ S tb =_s + _offset; auto temp = _ue4_extract_FStringView<type>(tb); _offset = tb.data() - _s.data(); return temp;}
							};
template<class T>		    class serializer_size_of<T, std::enable_if_t<is_ue4_FStringView_v<T>>>
							{	public:
								constexpr static std::size_t  _get_append_size(const FStringView& _object)				{ return _ue4_size_of_FStringView(_object); }
							};

//-----------------------------------------------------------------------------
// 3) ue4 - TArray
//-----------------------------------------------------------------------------
template <class S, class TELEMENT>
constexpr std::enable_if_t<!is_memcopyable<TELEMENT>::value, _buffer_view<typename S::element_t>>
_ue4_append_TArray(S& _s, const TArray<TELEMENT>& _object)
{
	using RET_T = _buffer_view<typename S::element_t>;

	// 1) get count
	auto count = _object.Num();

	// 1) [원본_버퍼_포인터]를 저장해 놓는다.
	auto len_old = _s.size();

	// 2) [데이터_갯수]를 써넣는다.
	_s.template _append_general<COUNT_T>(static_cast<COUNT_T>(count));

	// 3) [데이터]들을 추가한다.
	for (int32 i = 0; i < count; ++i)
		_s.template _append<TELEMENT>(_object[i]);

	// return)
	return RET_T(_s.data() + len_old, _s.size() - len_old);
}
template <class S, class TELEMENT>
constexpr std::enable_if_t<is_memcopyable<TELEMENT>::value, _buffer_view<typename S::element_t>>
_ue4_append_TArray(S& _s, const TArray<TELEMENT>& _object)
{
	using RET_T = _buffer_view<typename S::element_t>;

	// 1) get count
	auto count = _object.Num();

	// declare) [원본_버퍼_포인터]를 저장해 놓는다.
	auto len_old = _s.size();

	// 1) [데이터_갯수]를 써넣는다.
	_s.template _append_general<COUNT_T>(static_cast<COUNT_T>(count));

	// 2) [데이터]들을 통채로 복사한다.
	_s._append_bytes(count * sizeof(TELEMENT), _object.GetData());

	// return)
	return RET_T(_s.data() + len_old, _s.size() - len_old);
}

template<class S, class T, class TELEMENT>
constexpr std::enable_if_t<!is_memcopyable<TELEMENT>::value, T>
_ue4_extract_TArray(S& _s)
{
	// declare)
	T result{};

	// 1) [데이터_갯수]를 얻어낸다.
	auto size_data = _s.template _extract<COUNT_T>();

	// 2) [데이터_갯수]만큼 읽어들인다.
	if (size_data != COUNT_T(-1) && size_data > 0)
	{
		result.Reserve(size_data);
		for (COUNT_T i = 0; i < size_data; ++i)
			result.Add(_s.template _extract<TELEMENT>());
	}

	// return)
	return result;
}

template<class S, class T, class TELEMENT>
constexpr std::enable_if_t<is_memcopyable<TELEMENT>::value, T>
_ue4_extract_TArray(S& _s)
{
	// declare)
	T result{};

	// 1) [데이터_갯수]를 얻어낸다.
	auto size_data = _s.template _extract<COUNT_T>();

	// 2) memcpy
	if (size_data != COUNT_T(-1) && size_data > 0)
	{
		_CGD_BUFFER_BOUND_CHECK(size_data * sizeof(TELEMENT) <= _s.size());

		result.SetNumUninitialized(size_data);
		memcpy(result.GetData(), _s.data(), size_data * sizeof(TELEMENT));
		_s += offset(size_data * sizeof(TELEMENT));
	}

	// return)
	return result;
}

template<class T>
size_t _ue4_size_of_TArray(const T& _object)
{
	std::size_t size = sizeof(COUNT_T);
	
	for (const auto& iter : _object)
		size += get_size_of(iter);
	
	return size;
}

template<class B, class T>
							class serializer_append<B, T, std::enable_if_t<is_ue4_TArray_v<T>>>
							{	public: using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const T& _data)			{ return _ue4_append_TArray<S, typename T::ElementType>(_s, _data);}
								template<class S> constexpr static type _do_append(S& _s, T&& _data)				{ return _ue4_append_TArray<S, typename T::ElementType>(_s, _data);}
							};
template<class B, class T>
							class serializer_extract<B, T, std::enable_if_t<is_ue4_TArray_v<T>>>
							{	using TX = std::remove_const_t<T>;
								public: using type = TX;
								template<class S> constexpr static type _do_extract(S& _s)							{ return _ue4_extract_TArray<S, TX, typename TX::ElementType>(_s); }
							};
template<class B, class T>
							class serializer_peek<B, T, std::enable_if_t<is_ue4_TArray_v<T>>>
							{	using TX = std::remove_const_t<T>;
								public: using type = TX;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset)	{ S tb =_s + _offset; auto temp = _ue4_extract_TArray<S, TX, typename T::ElementType>(tb); _offset = tb.data() - _s.data(); return temp;}
							};
template<class T>
							class serializer_size_of<T, std::enable_if_t<is_ue4_TArray_v<T>>>
							{	public:
								constexpr static std::size_t  _get_append_size(const T& _object)							{ return _ue4_size_of_TArray(_object); }
							};

//-----------------------------------------------------------------------------
// 4) ue4 - TStaticArray (TStaticArray, TShaderResourceParameterArray)
//-----------------------------------------------------------------------------
template <class S, class A1, uint32 A2, uint32 A3>
constexpr std::enable_if_t<!is_memcopyable<A1>::value, _buffer_view<typename S::element_t>>
_ue4_append_TStaticArray(S& _s, const TStaticArray<A1, A2, A3>& _object)
{
	using RET_T = _buffer_view<typename S::element_t>;

	// 1) get count
	auto count = _object.Num();

	// 1) [원본_버퍼_포인터]를 저장해 놓는다.
	auto len_old = _s.size();

	// 2) [데이터_갯수]를 써넣는다.
	_s.template _append_general<COUNT_T>(static_cast<COUNT_T>(count));

	// 3) [데이터]들을 추가한다.
	for (uint32 i = 0; i < A2; ++i)
		_s.template _append<A1>(_object[i]);

	// return)
	return RET_T(_s.data() + len_old, _s.size() - len_old);
}
template <class S, class A1, uint32 A2, uint32 A3>
constexpr std::enable_if_t<is_memcopyable<A1>::value, _buffer_view<typename S::element_t>>
_ue4_append_TStaticArray(S& _s, const TStaticArray<A1, A2, A3>& _object)
{
	using RET_T = _buffer_view<typename S::element_t>;

	// 1) get count
	auto count = _object.Num();

	// declare) [원본_버퍼_포인터]를 저장해 놓는다.
	auto len_old = _s.size();

	// 2) [데이터_갯수]를 써넣는다.
	_s.template _append_general<COUNT_T>(static_cast<COUNT_T>(count));

	// 3) [데이터]들을 통채로 복사한다.
	_s._append_bytes(A2 * sizeof(A1), _object.GetData());

	// return)
	return RET_T(_s.data() + len_old, _s.size() - len_old);
}

template<class S, class T, class A1, uint32 A2, uint32 A3>
constexpr std::enable_if_t<!is_memcopyable<A1>::value, T>
_ue4_extract_TStaticArray(S& _s)
{
	// declare)
	T result{};

	// 1) [데이터_갯수]를 얻어낸다.
	auto size_data = _s.template _extract<COUNT_T>();

	// check) null(-1) 이거나 0 이면 빈 배열을 리턴한다.
	if (size_data == COUNT_T(-1) || size_data <= 0)
		return result;

	// check) [데이터_갯수]가 배열의 수보다 커서는 안됀다.
	if (static_cast<uint32>(size_data) > A2)
		throw std::length_error("overflow array size");

	// 2) [데이터_갯수]만큼 읽어들인다.
	for (COUNT_T i = 0; i < size_data; ++i)
		result[i] = _s.template _extract<A1>();

	// return)
	return result;
}

template<class S, class T, class A1, uint32 A2, uint32 A3>
constexpr std::enable_if_t<is_memcopyable<A1>::value, T>
_ue4_extract_TStaticArray(S& _s)
{
	// declare)
	T result{};

	// 1) [데이터_갯수]를 얻어낸다.
	auto size_data = _s.template _extract<COUNT_T>();

	// check) null(-1) 이거나 0 이면 빈 배열을 리턴한다.
	if (size_data == COUNT_T(-1) || size_data <= 0)
		return result;

	// check) [데이터_갯수]가 배열의 수보다 커서는 안됀다.
	if (static_cast<uint32>(size_data) > A2)
		throw std::length_error("overflow array size");

	// check) 읽을 데이터가 버퍼에 있어야 한다.
	_CGD_BUFFER_BOUND_CHECK(size_data * sizeof(A1) <= _s.size());

	// 2) memcpy 후 읽은 만큼 버퍼를 이동시킨다.
	memcpy(result.GetData(), _s.data(), size_data * sizeof(A1));
	_s += offset(size_data * sizeof(A1));

	// return)
	return result;
}

template<class B, class A1, uint32 A2, uint32 A3> class serializer_append<B, TStaticArray<A1, A2, A3>>
							{	public:
								using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const TStaticArray<A1, A2, A3>& _data)	{ return _ue4_append_TStaticArray<S, A1, A2, A3>(_s, _data);}
								template<class S> constexpr static type _do_append(S& _s, TStaticArray<A1, A2, A3>&& _data)		{ return _ue4_append_TStaticArray<S, A1, A2, A3>(_s, _data);}
							};
template<class B, class A1, uint32 A2, uint32 A3> class serializer_extract<B, TStaticArray<A1, A2, A3>>
							{	
								public: using type = TStaticArray<A1, A2, A3>;
								template<class S> constexpr static type _do_extract(S& _s)							{ return _ue4_extract_TStaticArray<S, TStaticArray<A1, A2, A3>, A1, A2, A3>(_s); }
							};
template<class B, class A1, uint32 A2, uint32 A3> class serializer_peek<B, TStaticArray<A1, A2, A3>>
							{	
								public: using type = TStaticArray<A1, A2, A3>;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset)	{ S tb =_s + _offset; auto temp = _ue4_extract_TStaticArray<S, TStaticArray<A1, A2, A3>, A1, A2, A3>(tb); _offset = tb.data() - _s.data(); return temp;}
							};
template<class A1, uint32 A2, uint32 A3> class serializer_size_of<TStaticArray<A1, A2, A3>>
							{	public:
								constexpr static std::size_t  _get_append_size(const TStaticArray<A1, A2, A3>& _object)	{ return _ue4_size_of_TArray(_object); }
							};

//-----------------------------------------------------------------------------
// 5) ue4 - TSparseArray
//    - 빈 슬롯(hole)이 있으므로 GetData()/[0, Num()) 인덱스 접근을 쓸 수 없다. 반복자로 할당된 원소만 순회한다.
//    - [데이터_갯수](Num()) + [데이터]... 로 TArray/std::vector 와 같은 형식이다. (인덱스는 보존하지 않는다.)
//-----------------------------------------------------------------------------
template<class S, class T>
_buffer_view<typename S::element_t> _ue4_append_TSparseArray(S& _s, const T& _object)
{
	using RET_T = _buffer_view<typename S::element_t>;

	// 1) [원본_버퍼_포인터]를 저장해 놓는다.
	auto len_old = _s.size();

	// 2) [데이터_갯수]를 써넣는다. (Num()은 할당된 원소의 수)
	_s.template _append_general<COUNT_T>(static_cast<COUNT_T>(_object.Num()));

	// 3) [데이터]들을 추가한다.
	for (const auto& iter : _object)
		_s.template _append<typename T::ElementType>(iter);

	// return)
	return RET_T(_s.data() + len_old, _s.size() - len_old);
}

template<class S, class T>
T _ue4_extract_TSparseArray(S& _s)
{
	// declare)
	T result{};

	// 1) [데이터_갯수]를 얻어낸다.
	auto size_data = _s.template _extract<COUNT_T>();

	// 2) [데이터_갯수]만큼 읽어들인다. (앞에서부터 채운다.)
	if (size_data != COUNT_T(-1) && size_data > 0)
	{
		result.Reserve(size_data);

		for (COUNT_T i = 0; i < size_data; ++i)
			result.Add(_s.template _extract<typename T::ElementType>());
	}

	// return)
	return result;
}

template<class B, class T> class serializer_append<B, T, std::enable_if_t<is_ue4_TSparseArray_v<T>>>
							{	public: using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const T& _data)			{ return _ue4_append_TSparseArray<S, T>(_s, _data);}
								template<class S> constexpr static type _do_append(S& _s, T&& _data)				{ return _ue4_append_TSparseArray<S, T>(_s, _data);}
							};
template<class B, class T> class serializer_extract<B, T, std::enable_if_t<is_ue4_TSparseArray_v<T>>>
							{	using TX = std::remove_const_t<T>;
								public: using type = TX;
								template<class S> constexpr static type _do_extract(S& _s)							{ return _ue4_extract_TSparseArray<S, TX>(_s); }
							};
template<class B, class T> class serializer_peek<B, T, std::enable_if_t<is_ue4_TSparseArray_v<T>>>
							{	using TX = std::remove_const_t<T>;
								public: using type = TX;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset)	{ S tb =_s + _offset; auto temp = _ue4_extract_TSparseArray<S, TX>(tb); _offset = tb.data() - _s.data(); return temp;}
							};
template<class T>		    class serializer_size_of<T, std::enable_if_t<is_ue4_TSparseArray_v<T>>>
							{	public:
								constexpr static std::size_t  _get_append_size(const T& _object)							{ return _ue4_size_of_TArray(_object); }
							};

//-----------------------------------------------------------------------------
// 6) ue4 - TList (singly linked list - TList)
//    - TList<T>는 { Element, Next } 노드 하나다. Num()/operator[]/반복자가 없으므로 Next 를 따라간다.
//    - [데이터_갯수](자신 포함 노드 수) + [데이터]... 로 std::list 와 같은 형식이다.
//    - 값으로 리턴하므로 빈 리스트는 표현할 수 없다. (읽을 때 [데이터_갯수]가 0 이하면 예외)
//    - extract 가 new 로 만든 두번째 이후의 노드들은 호출자가 delete 해야 한다.
//-----------------------------------------------------------------------------
template<class S, class T>
_buffer_view<typename S::element_t> _ue4_append_TList(S& _s, const T& _object)
{
	using RET_T = _buffer_view<typename S::element_t>;
	using TELEMENT = ue4_element_type_t<T>;

	// 1) [데이터_갯수]를 센다.
	COUNT_T count = 0;
	for (auto node = &_object; node != nullptr; node = node->Next)
		++count;

	// 2) [원본_버퍼_포인터]를 저장해 놓는다.
	auto len_old = _s.size();

	// 3) [데이터_갯수]를 써넣는다.
	_s.template _append_general<COUNT_T>(count);

	// 4) [데이터]들을 추가한다.
	for (auto node = &_object; node != nullptr; node = node->Next)
		_s.template _append<TELEMENT>(node->Element);

	// return)
	return RET_T(_s.data() + len_old, _s.size() - len_old);
}

template<class S, class T>
T _ue4_extract_TList(S& _s)
{
	using TELEMENT = ue4_element_type_t<T>;

	// 1) [데이터_갯수]를 얻어낸다.
	auto size_data = _s.template _extract<COUNT_T>();

	// check) TList 는 노드 자체이므로 빈 리스트(0 또는 null(-1))는 만들 수 없다.
	if (size_data <= 0)
		throw std::length_error("TList must have at least one element");

	// 2) 첫번째 [데이터]는 리턴할 노드에 담는다.
	T result(_s.template _extract<TELEMENT>());

	// 3) 나머지 [데이터]들은 노드를 new 로 만들어 Next 로 잇는다.
	try
	{
		auto tail = &result;

		for (COUNT_T i = 1; i < size_data; ++i)
		{
			tail->Next = new T(_s.template _extract<TELEMENT>());
			tail = tail->Next;
		}
	}
	catch (...)
	{
		// - 실패하면 만들었던 노드들을 지운다.
		for (auto node = result.Next; node != nullptr;)
		{
			auto next = node->Next;
			delete node;
			node = next;
		}
		throw;
	}

	// return)
	return result;
}

template<class T>
size_t _ue4_size_of_TList(const T& _object)
{
	std::size_t size = sizeof(COUNT_T);

	for (auto node = &_object; node != nullptr; node = node->Next)
		size += get_size_of(node->Element);

	return size;
}

template<class B, class T> class serializer_append<B, T, std::enable_if_t<is_ue4_TList_v<T>>>
							{	public: using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const T& _data)			{ return _ue4_append_TList<S, T>(_s, _data);}
								template<class S> constexpr static type _do_append(S& _s, T&& _data)				{ return _ue4_append_TList<S, T>(_s, _data);}
							};
template<class B, class T> class serializer_extract<B, T, std::enable_if_t<is_ue4_TList_v<T>>>
							{	using TX = std::remove_const_t<T>;
								public: using type = TX;
								template<class S> constexpr static type _do_extract(S& _s)							{ return _ue4_extract_TList<S, TX>(_s); }
							};
template<class B, class T> class serializer_peek<B, T, std::enable_if_t<is_ue4_TList_v<T>>>
							{	using TX = std::remove_const_t<T>;
								public: using type = TX;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset)	{ S tb =_s + _offset; auto temp = _ue4_extract_TList<S, TX>(tb); _offset = tb.data() - _s.data(); return temp;}
							};
template<class T>		    class serializer_size_of<T, std::enable_if_t<is_ue4_TList_v<T>>>
							{	public:
								constexpr static std::size_t  _get_append_size(const T& _object)							{ return _ue4_size_of_TList(_object); }
							};

//-----------------------------------------------------------------------------
// 7) ue4 - TLinkedListBase (doubly linked list - TLinkedList, TIntrusiveLinkedList)
//    - TLinkedList<T>도 노드 하나다. Num()/operator[]가 없으므로 TConstIterator 로 자신부터 GetNextLink()를 따라간다.
//    - [데이터_갯수](자신 포함 노드 수) + [데이터]... 로 std::list 와 같은 형식이다.
//    - extract 는 TLinkedList<T>만 된다. (TIntrusiveLinkedList 는 원소가 곧 노드라 값으로 만들 수 없다.)
//    - 값으로 리턴하므로 빈 리스트는 표현할 수 없다. (읽을 때 [데이터_갯수]가 0 이하면 예외)
//    - extract 가 new 로 만든 두번째 이후의 노드들은 호출자가 delete 해야 한다.
//    - 두번째 노드의 PrevLink 는 리턴된 노드의 NextLink 를 가리키므로 리턴값을 복사/이동하면 다시 이어야(LinkHead 등) 한다.
//-----------------------------------------------------------------------------
template<class A1, class A2, template< class, class > class A3>
inline A1* _ue4_TLinkedListBase_head(const TLinkedListBase<A1, A2, A3>& _object) noexcept
{
	return const_cast<A1*>(static_cast<const A1*>(&_object));
}

template<class S, class T>
_buffer_view<typename S::element_t> _ue4_append_TLinkedListBase(S& _s, const T& _object)
{
	using RET_T = _buffer_view<typename S::element_t>;
	using TELEMENT = ue4_element_type_t<T>;

	// 1) [데이터_갯수]를 센다.
	COUNT_T count = 0;
	for (typename T::TConstIterator iter(_ue4_TLinkedListBase_head(_object)); iter; ++iter)
		++count;

	// 2) [원본_버퍼_포인터]를 저장해 놓는다.
	auto len_old = _s.size();

	// 3) [데이터_갯수]를 써넣는다.
	_s.template _append_general<COUNT_T>(count);

	// 4) [데이터]들을 추가한다.
	for (typename T::TConstIterator iter(_ue4_TLinkedListBase_head(_object)); iter; ++iter)
		_s.template _append<TELEMENT>(*iter);

	// return)
	return RET_T(_s.data() + len_old, _s.size() - len_old);
}

template<class S, class T>
T _ue4_extract_TLinkedList(S& _s)
{
	using TELEMENT = ue4_element_type_t<T>;

	static_assert(std::is_same_v<T, TLinkedList<TELEMENT>>, "only TLinkedList<T> can be extracted (TIntrusiveLinkedList nodes can't be created)");

	// 1) [데이터_갯수]를 얻어낸다.
	auto size_data = _s.template _extract<COUNT_T>();

	// check) TLinkedList 는 노드 자체이므로 빈 리스트(0 또는 null(-1))는 만들 수 없다.
	if (size_data <= 0)
		throw std::length_error("TLinkedList must have at least one element");

	// 2) 첫번째 [데이터]는 리턴할 노드에 담는다.
	T result(_s.template _extract<TELEMENT>());

	// 3) 나머지 [데이터]들은 노드를 new 로 만들어 뒤에 잇는다.
	try
	{
		auto tail = &result;

		for (COUNT_T i = 1; i < size_data; ++i)
		{
			auto node = new T(_s.template _extract<TELEMENT>());
			node->LinkAfter(tail);
			tail = node;
		}
	}
	catch (...)
	{
		// - 실패하면 만들었던 노드들을 지운다.
		for (auto node = result.GetNextLink(); node != nullptr;)
		{
			auto next = node->GetNextLink();
			delete node;
			node = next;
		}
		throw;
	}

	// return)
	return result;
}

template<class T>
size_t _ue4_size_of_TLinkedListBase(const T& _object)
{
	std::size_t size = sizeof(COUNT_T);

	for (typename T::TConstIterator iter(_ue4_TLinkedListBase_head(_object)); iter; ++iter)
		size += get_size_of(*iter);

	return size;
}

template<class B, class T> class serializer_append<B, T, std::enable_if_t<is_ue4_TLinkedListBase_v<T>>>
							{	public: using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const T& _data)			{ return _ue4_append_TLinkedListBase<S, T>(_s, _data);}
								template<class S> constexpr static type _do_append(S& _s, T&& _data)				{ return _ue4_append_TLinkedListBase<S, T>(_s, _data);}
							};
template<class B, class T> class serializer_extract<B, T, std::enable_if_t<is_ue4_TLinkedListBase_v<T>>>
							{	using TX = std::remove_const_t<T>;
								public: using type = TX;
								template<class S> constexpr static type _do_extract(S& _s)							{ return _ue4_extract_TLinkedList<S, TX>(_s); }
							};
template<class B, class T> class serializer_peek<B, T, std::enable_if_t<is_ue4_TLinkedListBase_v<T>>>
							{	using TX = std::remove_const_t<T>;
								public: using type = TX;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset)	{ S tb =_s + _offset; auto temp = _ue4_extract_TLinkedList<S, TX>(tb); _offset = tb.data() - _s.data(); return temp;}
							};
template<class T>		    class serializer_size_of<T, std::enable_if_t<is_ue4_TLinkedListBase_v<T>>>
							{	public:
								constexpr static std::size_t  _get_append_size(const T& _object)							{ return _ue4_size_of_TLinkedListBase(_object); }
							};

//-----------------------------------------------------------------------------
// 8) ue4 - TSet (set - TSet, 	FTouchKeySet)
//-----------------------------------------------------------------------------

template<class S, class T>
_buffer_view<typename S::element_t> _ue4_append_TSet(S& _s, const T& _object)
{
	using RET_T = _buffer_view<typename S::element_t>;

	// 1) get number of data
	auto count = _object.Num();

	// 2) [원본_버퍼_포인터]를 저장해 놓는다.
	auto len_old = _s.size();

	// 3) [데이터_갯수]를 써넣는다.
	_s.template _append_general<COUNT_T>(static_cast<COUNT_T>(count));

	// 4) [데이터]들을 추가한다.
	for (const auto& iter : _object)
		_s.template _append<typename T::ElementType>(iter);

	// return)
	return RET_T(_s.data() + len_old, _s.size() - len_old);
}

template<class S, class T>
T _ue4_extract_TSet(S& _s)
{
	// declare)
	T result{};

	// 1) [데이터_갯수]를 얻어낸다.
	auto size_data = _s.template _extract<COUNT_T>();

	// 2) [데이터_갯수]만큼 읽어들인다.
	if (size_data != COUNT_T(-1) && size_data > 0)
	{
		// - reserve
		result.Reserve(size_data);

		// - add
		for (COUNT_T i = 0; i < size_data; ++i)
			result.Add(_s.template _extract<typename T::ElementType>());
	}

	// return)
	return result;
}



template<class B, class T> class serializer_append<B, T, std::enable_if_t<is_ue4_TSet_v<T>>>
							{	public: using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const T& _data)			{ return _ue4_append_TSet<S, T>(_s, _data);}
								template<class S> constexpr static type _do_append(S& _s, T&& _data)				{ return _ue4_append_TSet<S, T>(_s, _data);}
							};
template<class B, class T> class serializer_extract<B, T, std::enable_if_t<is_ue4_TSet_v<T>>>
							{	using TX = std::remove_const_t<T>;
								public: using type = TX;
								template<class S> constexpr static type _do_extract(S& _s)							{ return _ue4_extract_TSet<S, T>(_s); }
							};
template<class B, class T> class serializer_peek<B, T, std::enable_if_t<is_ue4_TSet_v<T>>>
							{	using TX = std::remove_const_t<T>;
								public: using type = TX;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset)	{ S tb =_s + _offset; auto temp = _ue4_extract_TSet<S, T>(tb); _offset = tb.data() - _s.data(); return temp;}
							};
template<class T>		    class serializer_size_of<T, std::enable_if_t<is_ue4_TSet_v<T>>>
							{	public:
								constexpr static std::size_t  _get_append_size(const T& _object)							{ return _ue4_size_of_TArray(_object); }
							};

//-----------------------------------------------------------------------------
// 9) ue4 - TMapBase (map - TSortableMapBase, TMap, TMultiMap)
//-----------------------------------------------------------------------------

template<class S, class T>
_buffer_view<typename S::element_t> _ue4_append_TMap(S& _s, const T& _object)
{
	using RET_T = _buffer_view<typename S::element_t>;

	// 1) [원본_버퍼_포인터]를 저장해 놓는다.
	auto len_old = _s.size();

	// 2) [데이터_갯수]를 써넣는다.
	_s.template _append_general<COUNT_T>(static_cast<COUNT_T>(_object.Num()));

	// 3) [데이터]들을 추가한다.
	for (auto& iter: _object)
	{
		_s._append(iter.Key);
		_s._append(iter.Value);
	}

	// return)
	return RET_T(_s.data() + len_old, _s.size() - len_old);
}

template<class S, class T>
T _ue4_extract_TMap(S& _s)
{
	// declare)
	T result{};

	// 1) [데이터_갯수]를 얻어낸다.
	auto size_data = _s.template _extract<COUNT_T>();

	// 2) [데이터_갯수]만큼 읽어들인다.
	if (size_data != COUNT_T(-1) && size_data > 0)
	{
		// - reserve
		result.Reserve(size_data);

		// - add
		for (auto i = 0; i < size_data; ++i)
		{
			auto key = _s.template _extract<typename T::KeyType>();
			auto value = _s.template _extract<typename T::ValueType>();

			result.Add(std::move(key), std::move(value));
		}
	}

	// return)
	return result;
}

template<class T>
size_t _ue4_size_of_TMap(const T& _object)
{
	std::size_t size = sizeof(COUNT_T);
	
	for (const auto& iter : _object)
	{
		size += get_size_of(iter.Key);
		size += get_size_of(iter.Value);
	}
	
	return size;
}

template<class B, class T> class serializer_append<B, T, std::enable_if_t<is_ue4_TMapBase_v<T>>>
							{	public: using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const T& _data)			{ return _ue4_append_TMap<S, T>(_s, _data);}
								template<class S> constexpr static type _do_append(S& _s, T&& _data)				{ return _ue4_append_TMap<S, T>(_s, _data);}
							};
template<class B, class T> class serializer_extract<B, T, std::enable_if_t<is_ue4_TMapBase_v<T>>>
							{	using TX = std::remove_const_t<T>;
								public: using type = TX;
								template<class S> constexpr static type _do_extract(S& _s)							{ return _ue4_extract_TMap<S, T>(_s); }
							};
template<class B, class T> class serializer_peek<B, T, std::enable_if_t<is_ue4_TMapBase_v<T>>>
							{	using TX = std::remove_const_t<T>;
								public: using type = TX;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset)	{ S tb =_s + _offset; auto temp = _ue4_extract_TMap<S, T>(tb); _offset = tb.data() - _s.data(); return temp;}
							};
template<class T>		    class serializer_size_of<T, std::enable_if_t<is_ue4_TMapBase_v<T>>>
							{	public:
								constexpr static std::size_t  _get_append_size(const T& _object)							{ return _ue4_size_of_TMap(_object); }
							};

//-----------------------------------------------------------------------------
// 10) ue4 - TSortedMap (sorted map - TSortedMap, FAssetDataTagMap)
//-----------------------------------------------------------------------------
template<class B, class T> class serializer_append<B, T, std::enable_if_t<is_ue4_TSortedMap_v<T>>>
							{	public: using type = _buffer_view<typename B::element_t>;
								template<class S> constexpr static type _do_append(S& _s, const T& _data)			{ return _ue4_append_TMap<S, T>(_s, _data);}
								template<class S> constexpr static type _do_append(S& _s, T&& _data)				{ return _ue4_append_TMap<S, T>(_s, _data);}
							};
template<class B, class T> class serializer_extract<B, T, std::enable_if_t<is_ue4_TSortedMap_v<T>>>
							{	using TX = std::remove_const_t<T>;
								public: using type = TX;
								template<class S> constexpr static type _do_extract(S& _s)							{ return _ue4_extract_TMap<S, T>(_s); }
							};
template<class B, class T> class serializer_peek<B, T, std::enable_if_t<is_ue4_TSortedMap_v<T>>>
							{	using TX = std::remove_const_t<T>;
								public: using type = TX;
								template<class S> constexpr static type _do_peek(const S& _s, int64_t& _offset)	{ S tb =_s + _offset; auto temp = _ue4_extract_TMap<S, T>(tb); _offset = tb.data() - _s.data(); return temp;}
							};
template<class T>		    class serializer_size_of<T, std::enable_if_t<is_ue4_TSortedMap_v<T>>>
							{	public:
								constexpr static std::size_t  _get_append_size(const T& _object)							{ return _ue4_size_of_TMap(_object); }
							};

//-----------------------------------------------------------------------------
// 11) get_type_hash_of<T>()
//     - same hash as the std/CGDK types with the same serialized format
//       FString, FStringView							== std::basic_string<TCHAR>
//       TArray, TSparseArray, TList, TLinkedList, TSet == std::vector<T>
//       TStaticArray<T,N>								== std::array<T,N>
//       TMap, TMultiMap, TSortedMap					== std::map<K,V>
//       TSharedPtr<T>, TSharedRef<T>, TObjectPtr<T>	== object_ptr<T>
//-----------------------------------------------------------------------------
template<class T>			class serializer_hash<T, std::enable_if_t<is_ue4_FString_v<T>>>
							{	public:
								template<class... V> [[nodiscard]] static constexpr uint64_t _get_hash() noexcept { return _buffer_hash::mix(_buffer_hash::FNV_OFFSET, _buffer_hash::KIND_STRING, sizeof(TCHAR)); }
							};
template<class T>			class serializer_hash<T, std::enable_if_t<is_ue4_FStringView_v<T>>>
							{	public:
								template<class... V> [[nodiscard]] static constexpr uint64_t _get_hash() noexcept { return _buffer_hash::mix(_buffer_hash::FNV_OFFSET, _buffer_hash::KIND_STRING, sizeof(typename T::ElementType)); }
							};
template<class T>			class serializer_hash<T, std::enable_if_t<is_ue4_TArray_v<T> || is_ue4_TSparseArray_v<T> || is_ue4_TList_v<T> || is_ue4_TLinkedListBase_v<T> || is_ue4_TSet_v<T>>>
							{	public:
								template<class... V> [[nodiscard]] static constexpr uint64_t _get_hash() noexcept { return _buffer_hash::mix(_buffer_hash::FNV_OFFSET, _buffer_hash::KIND_SEQUENCE, _get_type_hash_of<ue4_element_type_t<T>, V...>()); }
							};
template<class A1, uint32 A2, uint32 A3> class serializer_hash<TStaticArray<A1, A2, A3>>
							{	public:
								template<class... V> [[nodiscard]] static constexpr uint64_t _get_hash() noexcept { return _buffer_hash::mix(_buffer_hash::FNV_OFFSET, _buffer_hash::KIND_ARRAY, A2, _get_type_hash_of<A1, V...>()); }
							};
template<class T>			class serializer_hash<T, std::enable_if_t<is_ue4_TMapBase_v<T> || is_ue4_TSortedMap_v<T>>>
							{	public:
								template<class... V> [[nodiscard]] static constexpr uint64_t _get_hash() noexcept { return _buffer_hash::mix(_buffer_hash::FNV_OFFSET, _buffer_hash::KIND_MAP, _get_type_hash_of<typename T::KeyType, V...>(), _get_type_hash_of<typename T::ValueType, V...>()); }
							};
template<class T>			class serializer_hash<TSharedPtr<T>>
							{	public:
								template<class... V> [[nodiscard]] static constexpr uint64_t _get_hash() noexcept { return _buffer_hash::mix(_buffer_hash::FNV_OFFSET, _buffer_hash::KIND_NULLABLE, _get_type_hash_of<std::remove_cv_t<T>, V...>()); }
							};
template<class T>			class serializer_hash<TSharedRef<T>>
							{	public:
								template<class... V> [[nodiscard]] static constexpr uint64_t _get_hash() noexcept { return _buffer_hash::mix(_buffer_hash::FNV_OFFSET, _buffer_hash::KIND_NULLABLE, _get_type_hash_of<std::remove_cv_t<T>, V...>()); }
							};
template<class T>			class serializer_hash<TObjectPtr<T>>
							{	public:
								template<class... V> [[nodiscard]] static constexpr uint64_t _get_hash() noexcept { return _buffer_hash::mix(_buffer_hash::FNV_OFFSET, _buffer_hash::KIND_NULLABLE, _get_type_hash_of<std::remove_cv_t<T>, V...>()); }
							};
}

#endif
