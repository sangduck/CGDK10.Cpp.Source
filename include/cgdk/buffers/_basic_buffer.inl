namespace CGDK
{

template<class B, class T>
class serializer_extract<B, _basic_buffer<T>>
{
public:
	using type = _basic_buffer<T>;
	template<class S>
	constexpr static type _do_extract(S& _s)
	{
		return S::template _extract_basic_buffer<T>(_s);
	}
	template<class D, class S>
	constexpr static void _do_extract(D& _dest, S& _s)
	{
		S::template _extract_basic_buffer<T>(_dest, _s);
	}
};

template<class B, class T>
class serializer_extract<B, _shared_buffer<T>>
{	using TX = _shared_buffer<T>;
public:
	using type = TX;
	template<class S>
	constexpr static type _do_extract(S& _s)
	{
		return _s.template _extract_shared_buffer<T>();
	}
	template<class D, class S>
	constexpr static void _do_extract(D& _dest, S& _s)
	{
		_s.template _extract_shared_buffer<T>(_dest);
	}
};

template<class B, class T>
class serializer_peek<B, _basic_buffer<T>>
{
public:
	using type = _basic_buffer<T>;
	template<class S>
	constexpr static type _do_peek(const S& _s, int64_t& _offset)
	{
		S temp_buffer = _s + static_cast<std::size_t>(_offset);

		auto temp = S::_extract_buffer(temp_buffer);

		_offset = (int64_t)(temp_buffer.data() - _s.data());

		return temp;
	}
	template<class D, class S>
	constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset)
	{
		S temp_buffer = _s + static_cast<std::size_t>(_offset);

		_dest = S::_extract_buffer(temp_buffer);

		_offset = (int64_t)(temp_buffer.data() - _s.data());
	}
};

template<class B, class T>
class serializer_size_of<B, _basic_buffer<T>>
{
public:
	constexpr static std::size_t  _get_append_size(const _basic_buffer<T>& _object)
	{
		return sizeof(std::size_t) + _object.size();
	}
	template<class S>
	constexpr static std::size_t  _get_extract_size(const S& _buffer, int64_t& _offset)
	{
		CGDK_ASSERT(false);
		return 0;
	}
};

template<class B, class T>
class serializer_peek<B, _buffer_view<T>>
{
public:
	using type = _buffer_view<T>;
	template<class S>
	constexpr static type _do_peek(const S& _s, int64_t& _offset)
	{
		S temp_buffer = _s + static_cast<std::size_t>(_offset);

		auto temp = temp_buffer._extract_buffer();

		_offset = (int64_t)(temp_buffer.data() - _s.data());

		return temp;
	}
	template<class D, class S>
	constexpr static void _do_peek(D& _dest, const S& _s, int64_t& _offset)
	{
		S temp_buffer = _s + static_cast<std::size_t>(_offset);

		_dest = temp_buffer._extract_buffer();

		_offset = (int64_t)(temp_buffer.data() - _s.data());
	}
};

template<class B, class T>
class serializer_size_of<B, _buffer_view<T>>
{
public:
	constexpr static std::size_t  _get_append_size(const _buffer_view<B>& _object)
	{
		return sizeof(std::size_t) + _object.size();
	}
	template<class S>
	constexpr static std::size_t  _get_extract_size(const S& _buffer, int64_t& _offset)
	{
		CGDK_ASSERT(false);
		return 0;
	}
};

template <class T>
bool validate_message(const T* _buffers, std::size_t _count)
{

	std::size_t	iTotalLength = 0;

	for (std::size_t i = 0; i < _count; ++i)
		iTotalLength += _buffers[i].size();

	std::size_t	idx_buffer = 0;
	const_buffer_view temp_buf = _buffers[idx_buffer];

	while (iTotalLength != 0)
	{
		CGDK_ASSERT(idx_buffer <= _count);
		if (idx_buffer > _count) return false;

		int32_t	message_size = *temp_buf.data<int32_t>();

		CGDK_ASSERT(message_size != 0);
		if (message_size == 0) return false;

		CGDK_ASSERT(message_size <= static_cast<int32_t>(iTotalLength));
		if (message_size > static_cast<int32_t>(iTotalLength)) return false;

		iTotalLength -= message_size;

		CGDK_ASSERT(iTotalLength >= 0);
		if (iTotalLength < 0) return false;

		while (message_size >= temp_buf.size<int32_t>())
		{
			message_size -= temp_buf.size<int32_t>();

			++idx_buffer;

			if (message_size == 0)
				break;

			CGDK_ASSERT(idx_buffer < _count);
			if (idx_buffer >= _count) return false;

			temp_buf = _buffers[idx_buffer];
		}

		temp_buf.add_data(message_size);
		temp_buf.sub_size(message_size);
	}

	return true;
}

template <std::size_t OFFSET, std::size_t IALIGN>
struct any_constructor_append
{
	const char* source;
	std::size_t& offset;
	buffer& buf_dest;

	template <class Type>
	constexpr operator Type () const
	{
		offset = align_offset_pre<IALIGN>(offset, sizeof(Type));

		buf_dest.append<std::decay_t<Type>>(*reinterpret_cast<const Type*>(source + offset));

		offset += sizeof(Type);

		return Type();
	}
};

template <class T, std::size_t... I>
constexpr void append_member_impl(const char* _source, buffer& _buf_dest, std::index_sequence<I...>)
{
	std::size_t offset = 0;
	T{ any_constructor_append<I, alignof(T)>{_source, offset, _buf_dest}... };
}

template <class D, class T, std::size_t... I>
constexpr void append_member_impl(D& _dest, const char* _source, buffer& _buf_dest, std::index_sequence<I...>)
{
	std::size_t offset = 0;
	_dest.~D();
	new (&_dest) T{ any_constructor_append<I, alignof(T)>{_source, offset, _buf_dest}... };
}

template<class B, class T>
class serializer_append<B, T, std::enable_if_t<is_struct_serializable<T>::value>>
{
public:
	using type = _buffer_view<typename B::element_t>;
	template<class S>
	constexpr static type _do_append(S& _s, const T& _data)
	{
		static_assert(std::is_default_constructible<T>::value, "T must be default constructable");

		static_assert(!std::is_base_of_v<Ibuffer_serializable, T>, "'Ibuffer_serializable' attribute is priorier than 'struct_serializable' attribute");

		static_assert(is_struct_serializable_v<T>, "T must have 'struct_serializable'");

		static_assert(std::is_default_constructible_v<T>, "'is_default_constructible<T>' class must have default constructor (remove constructor)");

		auto len_old = _s.size();
		append_member_impl<T>(reinterpret_cast<const char*>(&_data), _s, std::make_index_sequence<constexpr_field_count<T>>());

		return _buffer_view<typename B::element_t>(_s.data() + len_old, _s.size() - len_old);
	}
	template<class D, class S>
	constexpr static void _do_append(D& _dest, S& _s, const T& _data)
	{
		static_assert(std::is_default_constructible<T>::value, "T must be default constructable");

		static_assert(!std::is_base_of_v<Ibuffer_serializable, T>, "'Ibuffer_serializable' attribute is priorier than 'struct_serializable' attribute");

		static_assert(is_struct_serializable_v<T>, "T must have 'struct_serializable'");

		static_assert(std::is_default_constructible_v<T>, "'is_default_constructible<T>' class must have default constructor (remove constructor)");

		append_member_impl<T>(_dest, reinterpret_cast<const char*>(&_data), _s, std::make_index_sequence<constexpr_field_count<T>>());
	}
};

template <class ELEM_T>
constexpr _buffer_view<ELEM_T> _buffer_view<ELEM_T>::_extract_buffer_view()
{
	_buffer_view<ELEM_T> dest;
	_extract_buffer_view(dest);
	return dest;
}

template <class ELEM_T>
constexpr void _buffer_view<ELEM_T>::_extract_buffer_view(_buffer_view<ELEM_T>& _dest)
{
	_CGD_BUFFER_BOUND_CHECK(this->size_ >= sizeof(size_type));

	auto length = *reinterpret_cast<size_type*>(this->data_);

	const auto bytes_extract = length + sizeof(size_type);

	_CGD_BUFFER_BOUND_CHECK(this->size_ >= bytes_extract);

	if (length == 0)
	{
		_dest = _basic_buffer<ELEM_T>();

		this->data_ += sizeof(size_type);
		this->size_ -= sizeof(size_type);

		return;
	}

	_dest = _buffer_view<ELEM_T>(this->data_ + sizeof(size_type), length);

	this->data_ += bytes_extract;
	this->size_ -= bytes_extract;
}

template <class ELEM_T>
template <class BUFFER_T>
constexpr _basic_buffer<BUFFER_T> _buffer_view<ELEM_T>::_extract_basic_buffer()
{
	_basic_buffer<BUFFER_T> buf_temp;

	_CGD_BUFFER_BOUND_CHECK(this->size_ >= sizeof(size_type));

	auto length = *reinterpret_cast<size_type*>(this->data_);

	if (length == 0)
	{
		this->data_ += sizeof(size_type);
		this->size_ -= sizeof(size_type);

		return buf_temp;
	}

	const auto bytes_extract = length + sizeof(size_type);

	_CGD_BUFFER_BOUND_CHECK(this->size_ >= bytes_extract);

	const auto ptr_data = this->data_ + sizeof(size_type);

	buf_temp = _basic_buffer<BUFFER_T>({ ptr_data , length }, buffer_bound{ ptr_data, this->data_ + bytes_extract });

	this->data_ += bytes_extract;
	this->size_ -= bytes_extract;

	return buf_temp;
}

}
