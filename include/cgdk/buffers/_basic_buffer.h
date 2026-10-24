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

namespace CGDK
{

template <class ELEM_T>
class _basic_buffer : public _buffer_view<char>
{
// traits) 
public:
			using base_t = _buffer_view<char>;
			using self_t = _basic_buffer<ELEM_T>;
			using traits = typename base_t::traits;
			using element_t = typename base_t::element_t;
			using size_type = typename base_t::size_type;

			template<class T> using peek_tr = PEEK_tr<self_t, T>;
			template<class T> using extr_tr = EXTR_tr<self_t, T>;
			template<class T> using appd_tr = APPD_tr<self_t, T>;

// constructor/destructor) 
public:
	constexpr _basic_buffer() noexcept {}
	constexpr _basic_buffer(const _basic_buffer& _buffer) noexcept : base_t(_buffer), bound(_buffer.get_bound()) {}
	constexpr _basic_buffer(_basic_buffer&& _buffer) noexcept : base_t(_buffer), bound(_buffer.get_bound()) { _buffer = _basic_buffer(); }
	template <class T>
	constexpr _basic_buffer(_buffer_view<T> _buffer) noexcept : base_t(_buffer), bound{ _buffer.get_front_ptr(), _buffer.get_back_ptr() } {}
	template <class T>
	constexpr _basic_buffer(_buffer_view<T> _buffer, const buffer_bound& _bound) : base_t(_buffer), bound(_bound) { _CGD_BUFFER_BOUND_CHECK(_buffer.get_front_ptr() >= _bound.lower && _buffer.get_back_ptr() <= _bound.upper); }
	template <class T>
	constexpr _basic_buffer(_buffer_view<T> _buffer, buffer_bound&& _bound) : base_t(_buffer), bound(std::move(_bound)) { _CGD_BUFFER_BOUND_CHECK(_buffer.get_front_ptr() >= this->get_lower_bound() && _buffer.get_back_ptr() <= this->get_back_ptr<T>()); }

	template <class T, size_t N>
	constexpr _basic_buffer(T (&_buffer)[N]) : base_t(_buffer, 0), bound{reinterpret_cast<element_t*>(_buffer), reinterpret_cast<element_t*>(_buffer + N)} {}
	template <class T, size_t N>
	constexpr _basic_buffer(std::array<T, N>& _buffer) : base_t(_buffer.data(), 0), bound{ reinterpret_cast<element_t*>(_buffer.data()), reinterpret_cast<element_t*>(_buffer.data() + _buffer.size()) } {}
	template <class T>
	constexpr _basic_buffer(std::basic_string_view<T> _string) : base_t(_string.data(), 0), bound{ reinterpret_cast<element_t*>(_string.data()), reinterpret_cast<element_t*>(_string.data() + _string.size()) } {}

public:
	// 1) capacity/clear/copy/clone
	constexpr void				set_size(size_type _new_size) { _CGD_BUFFER_BOUND_CHECK(_new_size <= this->_get_upper_distance_from_front()); this->size_ = _new_size; }
	constexpr void				add_size(size_type _size) { _CGD_BUFFER_BOUND_CHECK(_size <= this->_get_upper_distance_from_end()); this->size_ += _size; }
	constexpr void				sub_size(size_type _size) { _CGD_BUFFER_BOUND_CHECK(_size <= this->_get_lower_distance_from_end()); this->size_ -= _size; }
	constexpr void				inc_size() { _CGD_BUFFER_BOUND_CHECK(1 <= this->_get_upper_distance_from_end()); ++this->size_; }
	constexpr void				dec_size() { _CGD_BUFFER_BOUND_CHECK(1 <= this->size_); --this->size_; }

	// 2) data/
	constexpr element_t*		data() const noexcept { return this->data_; }
	constexpr element_t*		data(int64_t _offset) const { _CGD_BUFFER_BOUND_CHECK((this->data_ + _offset) <= this->get_upper_bound()); return this->data_ + _offset; } // unsafe
	template <class T = element_t>
	constexpr auto				data() const noexcept { return reinterpret_cast<_buffer_return_t<traits, T>*>(this->data_); }
	template <class T = element_t>
	constexpr auto				data(int64_t _offset) const { _CGD_BUFFER_BOUND_CHECK(_offset <= static_cast<int64_t>(this->_get_upper_distance_from_end())); return reinterpret_cast<_buffer_return_t<traits, T>*>(this->data_ + _offset); } // unsafe
	constexpr void				set_data(element_t* _data) { _CGD_BUFFER_BOUND_CHECK(_data >= this->get_lower_bound() && (_data + this->size_) <= this->get_upper_bound()); this->data_ = _data; } // unsafe
	constexpr void				add_data(size_type _size) { _CGD_BUFFER_BOUND_CHECK(_size <= this->_get_upper_distance_from_end()); this->data_ += _size; } // unsafe
	constexpr void				sub_data(size_type _size) { _CGD_BUFFER_BOUND_CHECK(_size <= this->_get_lower_distance_from_front()); this->data_ -= _size; } // unsafe

	constexpr std::size_t		capacity() const noexcept { return (this->bound.upper != nullptr) ? (reinterpret_cast<const element_t*>(this->bound.upper) - this->data_) : 0;}
	constexpr std::size_t		remained_size() const noexcept { return this->get_remained_size();}
	constexpr void				clear() noexcept { this->base_t::clear(); this->bound.reset();}
	constexpr self_t			remained() const noexcept { return self_t(base_t{ this->get_back_ptr(), 0 }, this->bound); }
	constexpr self_t			remained(skip _skip) const noexcept { return self_t(base_t{ this->get_back_ptr() + _skip.amount, 0 }, this->bound); }
			template <class T>
			void				copy_from(const _buffer_view<T>& _source) { if (this->data_ + _source.size() > this->get_upper_bound()) throw std::length_error("buffer overflow. out of upper bound 'copy(_source)'"); memcpy(data(), _source.data(), _source.size()); size_ = _source.size();}
	constexpr void				swap(_basic_buffer& _rhs) noexcept { this->base_t::swap(_rhs); auto p = _rhs.get_bound(); _rhs._set_bound(bound); bound = p; }
	constexpr void				swap(base_t& _rhs) { if (_rhs.data() < this->get_lower_bound() || (_rhs.data() + _rhs.size()) > this->get_upper_bound()) { assert(false); throw std::out_of_range("_rhs data_ is must in bound of this shared_buffer"); } this->base_t::swap(_rhs); }
			self_t				split_head(const base_t& _source)
			{
				_CGD_BUFFER_BOUND_CHECK(size_ >= _source.size());
				char* p = this->data_;
				memcpy(p, _source.data(), _source.size());
				this->data_+= _source.size();
				this->size_-=_source.size();
				return self_t{ buffer_view{p, _source.size()}, bound };
			}
			self_t				split_tail(const base_t& _source)
			{
				_CGD_BUFFER_BOUND_CHECK(size_ >= _source.size());
				char* p = this->data_;
				memcpy(p + this->size_ - _source.size(), _source.data(), _source.size());
				this->size_ -= _source.size();
				return self_t{ buffer_view{p + this->size_, _source.size()}, bound };
			}

	// 4) append
			template <std::size_t N>
	constexpr auto&				append() { return this->_append_skip(N);}

			template <class T>																					  
	constexpr T&				append() requires(is_basic_type_v<T>) { return this->_append_emplace<std::remove_reference_t<std::remove_const_t<T>>>();}
	template <class T>
	constexpr appd_tr<T>		append() requires(!is_basic_type_v<T>) { return this->append<T>(T{}); }
			template <class T>																					  
	constexpr appd_tr<T>		append(const T& _data) { return APPD_t<self_t,T>::_do_append(*this, _data); }
			template <class T>																					  
	constexpr appd_tr<T>		append(T&& _data) { return APPD_t<self_t,T>::_do_append(*this, std::forward<T>(_data)); }
			template <class T>																					  
	constexpr auto				append(const T* _data, std::size_t _count) { return this->_append_array(_data, _count);}
			template <class T> 																					  
	constexpr base_t			append(const std::initializer_list<T>& _List) { return this->_append_initializer_list<T>(_List);}
			template<class I> 																					  
	constexpr base_t			append(I _first, I _last) requires(is_iterator<I>::value) { return _append_iterator(_first, _last);}
	constexpr auto				append(std::size_t _size, const void* _buffer) { return this->_append_bytes(_size, _buffer); }

			template <class T = char>
	constexpr base_t			append_text(const std::basic_string_view<T> _text) requires(is_string_type<T>::value) { return this->_append_text<T>(_text);}
			template <class T = char>
	constexpr base_t			append_text(const std::basic_string<T>& _text) requires(is_string_type<T>::value) { return this->_append_text<T>(std::basic_string_view<T>(_text));}

			template <class T = char, std::size_t N>
	constexpr base_t			append_text(const T(&_text)[N]) requires(is_string_type<T>::value) { return this->_append_text<T>(_text);}
			template <class T = char, class TNUM>
	constexpr base_t			append_text(TNUM _value) requires(std::is_integral_v<TNUM>) { return this->_append_text_integer<T>(_value);}
			template <class T = char, class TNUM>
	constexpr base_t			append_text(TNUM _value) requires(std::is_floating_point_v<TNUM>) { return this->_append_text_float<T>(_value);}
			template <class T = char>
			base_t				append_text_hex(const char* _data, std::size_t _size) { return this->_append_text_binary<T>(_data, _size); }

	// 5) extract/subtract
			template <class T>
	constexpr extr_tr<T>		extract() requires(!std::is_reference_v<T>) { return this->base_t::template extract<T>();}
			template <class T>
	constexpr void				extract_to(T& _dest) requires(!std::is_reference_v<T>) { this->base_t::template extract_to<T>(_dest);}
			template <class T>
	constexpr T				extract() requires(std::is_reference_v<T>) { return this->base_t::template extract<T>();}
			template <class T>
	constexpr void				extract_to(T& _dest) requires(std::is_reference_v<T>) { this->base_t::template extract_to<T>(_dest);}
			template <std::size_t N>
	constexpr auto				extract() { return self_t(this->base_t::extract(CGDK::size(N)), bound);}
	constexpr auto				extract(CGDK::size _length) { return self_t(this->base_t::extract(_length), bound);}

	// 6) reference
			template <class T = char>
	constexpr void				set_front_ptr(T* _pos) { _CGD_BUFFER_BOUND_CHECK(_pos >= this->get_lower_bound() && _pos <= this->get_back_ptr<T>()); auto temp_offset = reinterpret_cast<element_t*>(_pos) - this->data_; this->data_ = reinterpret_cast<element_t*>(_pos); this->size_ -= temp_offset;  }
			template <class T = char>
	constexpr void				set_back_ptr(T* _pos) { _CGD_BUFFER_BOUND_CHECK(_pos >= this->get_front_ptr<T>() && _pos <= this->get_upper_bound()); this->size_ = reinterpret_cast<element_t*>(_pos) - this->data_; }
	constexpr const buffer_bound& get_bound() const noexcept { return this->bound;}
	constexpr std::size_t		get_remained_size() const noexcept { return this->_get_upper_distance_from_end();}

	constexpr std::size_t		_get_upper_distance_from_front() const noexcept { return static_cast<const char*>(this->bound.upper) - this->data_; }
	constexpr std::size_t		_get_lower_distance_from_front() const noexcept { return this->data_ - static_cast<const char*>(this->bound.lower); }
	constexpr std::size_t		_get_upper_distance_from_end() const noexcept { return static_cast<const char*>(this->bound.upper) - this->data_ - this->size_; }
	constexpr std::size_t		_get_lower_distance_from_end() const noexcept { return this->data_ + this->size_ - static_cast<const char*>(this->bound.lower); }

	// 7) operator overloading																					  
			// [operator] +/-		
	constexpr self_t			operator ~ () const { auto data_value = this->data_ + this->size_; return self_t{ base_t { data_value, static_cast<size_type>(static_cast<const element_t*>(this->bound.upper) - data_value) }, this->bound }; }
	constexpr self_t			operator + (offset _rhs) const { return self_t{ this->base_t::operator+(_rhs), bound };}
	constexpr self_t			operator - (offset _rhs) const
			{
				_CGD_BUFFER_BOUND_CHECK(_rhs.amount <= this->_get_lower_distance_from_front());

				return self_t(base_t{ this->data_ - _rhs.amount, this->size_ + _rhs.amount }, bound);
			}
	constexpr self_t			operator+(CGDK::size _rhs) const
			{
				_CGD_BUFFER_BOUND_CHECK(_rhs.amount <= static_cast<int64_t>(this->_get_upper_distance_from_end()));

				return self_t(base_t{ this->data_, this->size_ + _rhs.amount }, bound);
			}
	constexpr self_t			operator-(CGDK::size _rhs) const { return self_t{ this->base_t::operator-(_rhs), bound }; }
			// [operator] +=/-=
	constexpr self_t&			operator+=(offset _rhs) { this->base_t::operator+=(_rhs); return *this; }
	constexpr self_t&			operator-=(offset _rhs)
			{
				_CGD_BUFFER_BOUND_CHECK(_rhs.amount <= this->_get_lower_distance_from_front());

				this->data_ -= _rhs.amount;
				this->size_ += _rhs.amount;
				
				return *this;
			}
	constexpr self_t&			operator+=(CGDK::size _rhs)
			{
				_CGD_BUFFER_BOUND_CHECK( _rhs.amount <= this->_get_upper_distance_from_end());

				this->size_ += _rhs.amount;

				return *this;
			}
	constexpr self_t&			operator-=(CGDK::size _rhs) { this->base_t::operator-=(_rhs); return *this; }
	constexpr self_t&			operator+=(const base_t& _rhs) { this->_append_bytes(_rhs.size(), _rhs.data()); return *this;}
	constexpr self_t&			operator+=(base_t&& _rhs) { this->_append_bytes(_rhs.size(), _rhs.data()); _rhs = base_t(); return *this;}
			template <class T>
	constexpr self_t&			operator+=(const T& _rhs) requires(is_linear_container_with_buffer<T>::value) { for (auto& iter : _rhs) { if (iter.empty()) continue; this->_append_bytes(iter.size(), iter.data()); } return *this; }
			template <class T>
	constexpr self_t&			operator+=(T&& _rhs) requires(is_linear_container_with_buffer<T>::value) { for (auto& iter : _rhs) { if (iter.empty()) continue; this->_append_bytes(iter.size(), iter.data()); } _rhs = T(); return *this; }
			template <class T, std::size_t N>
	constexpr self_t&			operator+=(const std::array<T, N>& _rhs) requires(std::is_base_of_v<base_t, T>) { for(auto& iter: _rhs) { if (iter.empty()) continue; this->_append_bytes(iter.size(), iter.data());} return *this; }
			template <class T, std::size_t N>
	constexpr self_t&			operator+=(std::array<T, N>&& _rhs) requires(std::is_base_of_v<base_t, T>) { for(auto& iter: _rhs) { if (iter.empty()) continue; this->_append_bytes(iter.size(), iter.data());} _rhs=std::array<T,N>();return *this; }
			template <class T, std::size_t N>
	constexpr self_t&			operator+=(const base_t(&_rhs)[N] ) requires(std::is_base_of_v<base_t, T>) { const T* iter = _rhs; const T* iter_end = _rhs + N; for(;iter != iter_end; ++iter) { if (iter->empty()) continue; this->_append_bytes(iter->size_, iter->data_); } return *this;	}
	constexpr self_t&			operator+=(const self_t& _rhs) { this->_append_bytes(_rhs.size(), _rhs.data()); return *this;}
	constexpr self_t&			operator+=(self_t&& _rhs) { this->_append_bytes(_rhs.size(), _rhs.data()); _rhs=self_t(); return *this;}
			// [operator] =
	constexpr self_t&			operator= (const base_t& _rhs) noexcept { _check_bound(_rhs); this->base_t::operator=(_rhs); return *this; }
	constexpr self_t&			operator= (base_t&& _rhs) noexcept { _check_bound(_rhs); this->base_t::operator=(std::move(_rhs)); return *this; }
	constexpr self_t&			operator= (const self_t& _rhs) noexcept { this->base_t::operator=(_rhs); bound = _rhs.bound; return *this; }
	constexpr self_t&			operator= (self_t&& _rhs) noexcept { this->base_t::operator=(std::move(_rhs)); bound = _rhs.bound; _rhs.clear(); return *this; }

			// [operator] ^=
			template<class T>
	constexpr self_t&			operator^=(const _buffer_view<T>& _rhs) { this->base_t::operator=(_rhs); return *this;}
			template<class T>
	constexpr self_t&			operator^=(_buffer_view<T>&& _rhs) { this->base_t::operator=(std::move(_rhs)); return *this;}
			template<class T>
	constexpr self_t&			operator^=(const buffer_base<T>& _rhs) { this->base_t::operator=(_rhs); return *this;}
			template<class T>
	constexpr self_t&			operator^=(buffer_base<T>&& _rhs) { this->base_t::operator=(std::move(_rhs)); return *this;}
	constexpr self_t&			operator^=(std::size_t _rhs) { this->base_t::set_size(_rhs); return *this;}
			// [operator >> - extract
			template <class T>
			self_t&				operator>>(T& _rhs) { _rhs = _extract<T>(); return *this;}
			// [operator] << - append																			  
			template <class T>																					  
			self_t&				operator<<(const T& _rhs) { APPD_t<self_t, T>::_do_append(*this, _rhs); return *this;}

public:
	// append) 
	template <class T>
	constexpr appd_tr<T>		_append(const T& _data)
	{
		return APPD_t<self_t,T>::_do_append(*this, _data);
	}
	template <class T>
	constexpr appd_tr<T>		_append(const object_ptr<std::remove_const_t<T>>& _data)
	{
		return APPD_t<self_t,T>::_do_append(*this, *_data);
	}
	template <class T>
	constexpr base_t			_append_initializer_list(const std::initializer_list<T>& _list)
	{
		auto p = this->data_;
		for(auto& iter:_list)
			_append<T>(iter);
		return base_t(p, this->data_ - p);
	}
	constexpr base_t			_append_skip(std::size_t _length)
	{
		auto p = this->data_ + this->size_;

		_CGD_BUFFER_BOUND_CHECK((p + _length) <= this->get_upper_bound());

		this->size_ += _length;

		return base_t(p, _length);
	}
	template <class T>
	constexpr T&				_append_emplace()
	{
		_CGD_BUFFER_BOUND_CHECK(sizeof(T) <= this->_get_upper_distance_from_end());

		auto p = reinterpret_cast<T*>(this->data_ + this->size_);

	#pragma push_macro("new")
	#undef new
		new (p) T;
	#pragma pop_macro("new")

		this->size_ += sizeof(T);

		return *p;
	}
	template <class T>
	constexpr T&				_append_general(const T& _data)
	{
		auto p = reinterpret_cast<T*>(this->data_ + this->size_);
	
		_CGD_BUFFER_BOUND_CHECK(this->data_ != nullptr && ((p + 1) <= this->get_upper_bound()));

	#pragma push_macro("new")
	#undef new
		new (p) T(_data);
	#pragma pop_macro("new")
	
		this->size_ += sizeof(T);
	
		return *p;
	}
	template <class T> 
	constexpr base_t			_append_array(const T* _data, std::size_t _count) requires(!is_memcopyable<T>::value)
	{
		CGDK_ASSERT(_data != nullptr && _count != 0, throw std::invalid_argument("_data is nullptr or _count is 0 [1]"));

		_CGD_BUFFER_BOUND_CHECK(sizeof(T) * _count <= this->_get_upper_distance_from_end());

		auto len_old = this->size_;

		this->_append_general<COUNT_T>(static_cast<COUNT_T>(_count));

		for(std::size_t i=0; i<_count; ++i)
			this->_append<T>(_data[i]);

		return base_t(this->data_ + len_old, this->size_ - len_old);
	}
	template <class T>
	constexpr base_t			_append_array(const T* _data, std::size_t _count) requires(is_memcopyable<T>::value)
	{
		CGDK_ASSERT(_data != nullptr && _count != 0, throw std::invalid_argument("_data is nullptr or _count is 0 [1]"));
	
		this->_append_general<COUNT_T>(static_cast<COUNT_T>(_count));

		return this->_append_bytes(_count * sizeof(T), _data);
	}
	template <class T, std::size_t N>
	constexpr base_t			_append_array(const T* _data) requires(!is_memcopyable<T>::value)
	{
		return this->_append_array<T>(_data, N);
	}
	template <class T, std::size_t N>
	constexpr base_t			_append_array(const T* _data) requires(is_memcopyable<T>::value)
	{
		static_assert(N != 0, "size of array '_data' is 0");

		this->_append_general<COUNT_T>(static_cast<COUNT_T>(N));

		return this->_append_bytes(N * sizeof(T), _data);
	}
	template <class T>
	constexpr base_t			_append_string(std::basic_string_view<T> _string) requires(is_string_type<T>::value)
	{
		const auto buf_dest = this->data_ + this->size_;

		const auto length_string = _string.size();	// strnlen(, _buffer_size)
		const auto bytes_copy = length_string * sizeof(T);
		const auto added_length = sizeof(COUNT_T) + sizeof(T) + bytes_copy;

		_CGD_BUFFER_BOUND_CHECK((buf_dest + added_length) <= this->get_upper_bound());

		auto buf_now = buf_dest;

		*reinterpret_cast<COUNT_T*>(buf_now) = static_cast<COUNT_T>(length_string + 1);
		buf_now += sizeof(COUNT_T);

		if (bytes_copy != 0)
		{
			memcpy(buf_now, _string.data(), bytes_copy);
			buf_now += bytes_copy;
		}

		*reinterpret_cast<T*>(buf_now) = 0;

		this->size_ += added_length;

		return base_t(buf_dest, added_length);
	}
	template <class T, std::size_t N>
	constexpr base_t			_append_string(const T(&_string)[N]) requires(is_string_type<T>::value)
	{
		const auto length_string = std::char_traits<T>::length(_string) + 1;
		const auto bytes_copy = length_string * sizeof(T);
		const auto added_length = sizeof(COUNT_T) + bytes_copy;

		auto buf_dest = this->data_ + this->size_;

		_CGD_BUFFER_BOUND_CHECK(added_length <= reinterpret_cast<const char*>(this->get_upper_bound()) - buf_dest);

		auto buf_now = buf_dest;

		*reinterpret_cast<COUNT_T*>(buf_now) = static_cast<COUNT_T>(length_string);
		buf_now += sizeof(COUNT_T);

		if (bytes_copy != 0)
		{
			memcpy(buf_now, _string, bytes_copy);
			buf_now += bytes_copy;
		}

		this->size_ += added_length;

		return base_t(buf_dest, added_length);
	}
	template <class T>
	constexpr base_t			_append_text(std::basic_string_view<T> _string) requires(is_string_type<T>::value)
	{
		auto size_string = _string.size() * sizeof(T);

		_CGD_BUFFER_BOUND_CHECK(size_string <= this->_get_upper_distance_from_end());

		auto buf_pos = this->data_ + this->size_;

		memcpy(buf_pos, _string.data(), size_string);

		this->size_ += size_string;

		return base_t(buf_pos, size_string);
	}
	template <class T, class TY>
	constexpr base_t			_append_text_encoding(std::basic_string_view<TY> _string) requires(is_string_type<T>::value)
	{
		auto size_string = _string.size() * sizeof(T);

		_CGD_BUFFER_BOUND_CHECK(size_string <= this->_get_upper_distance_from_end());

		auto buf_pos = this->data_ + this->size_;

		if (size_string != 0)
			encode_string(reinterpret_cast<T*>(buf_pos), _string);

		this->size_ += size_string;

		return base_t(buf_pos, size_string);
	}

	template <class T, std::size_t N>
	constexpr base_t			_append_text(const T(&_text)[N])
	{
		const auto length_string = std::char_traits<T>::length(_text);
		const auto size_string = length_string * sizeof(T);

		_CGD_BUFFER_BOUND_CHECK(size_string <= this->_get_upper_distance_from_end());

		auto buf_dest = this->data_ + this->size_;

		memcpy(buf_dest, _text, size_string);

		this->size_ += size_string;

		return base_t(buf_dest, size_string);
	}
	template <class T, std::size_t N, class TY>
	constexpr base_t			_append_text_encoding(const TY(&_text)[N])
	{
		const auto length_string = std::char_traits<T>::length(_text);
		const auto size_string = length_string * sizeof(T);

		_CGD_BUFFER_BOUND_CHECK(size_string <= this->_get_upper_distance_from_end());

		const auto buf_dest = this->data_ + this->size_;

		if (size_string != 0)
			encode_string(reinterpret_cast<T*>(buf_dest), _text);

		this->size_ += size_string;

		return base_t(buf_dest, size_string);
	}
	constexpr base_t			_append_bytes(std::size_t _size, const void* _buffer)
	{
		CGDK_ASSERT(_buffer != nullptr || (_buffer == nullptr && _size == 0), throw std::invalid_argument("_buffer is nullptr [1]"));

		_CGD_BUFFER_BOUND_CHECK(_size <= this->_get_upper_distance_from_end());

		const auto buf_dest = this->data_ + this->size_;
	
		if(_size == 0) return base_t(buf_dest, 0);
	
		if(_buffer != nullptr && _buffer != buf_dest)
			memcpy(buf_dest, _buffer, _size);
	
		this->size_ += _size;
	
		return base_t(buf_dest, _size);
	}
	constexpr self_t			_append_buffer(const base_t& _buffer)
	{
		_CGD_BUFFER_BOUND_CHECK((sizeof(base_t::size_type) + _buffer.size()) <= this->_get_upper_distance_from_end());

		CGDK_ASSERT(_buffer.data() != nullptr || _buffer.size() == 0, throw std::invalid_argument("_data is nullptr [1]"));

		auto len_old = this->size_;

		if(_buffer.data() != nullptr && _buffer.size() != 0)
		{
			this->_append_general<base_t::size_type>(_buffer.size());
			this->_append_bytes(_buffer.size(), _buffer.data());
		}
		else
		{
			this->_append_general<base_t::size_type>(0);
		}

		// return) 
		return self_t{ base_t{this->data_ + len_old, this->size_ - len_old} };
	}
	template<class T>
	constexpr base_t			_append_tuple(const std::tuple<T>& _tuple)
	{
		auto len_old = this->size_;
		this->_append<std::decay_t<T>>(std::get<0>(_tuple));
		return base_t(this->data_ + len_old, this->size_ - len_old);
	}
	template<class TFIRST, class TSECOND, class... TREST>
	constexpr base_t			_append_tuple(const std::tuple<TFIRST, TSECOND, TREST...>& _tuple)
	{ 
		auto len_old = this->size_;
		this->_append<std::decay_t<TFIRST>>(std::get<0>(_tuple));
		this->_append_tuple<TSECOND, TREST...>((const std::tuple<TSECOND, TREST...>&)_tuple);
		return base_t(this->data_ + len_old, this->size_ - len_old);
	}
	template<class T>
	constexpr void				_append_multi_(T&& _tuple)
	{
		this->_append<std::decay_t<T>>(std::forward<T>(_tuple));
	}
	template<class TFIRST, class TSECOND, class... TREST>
	constexpr void				_append_multi_(TFIRST&& _first, TSECOND&& _second, TREST&&... _rest)
	{ 
		this->_append<std::decay_t<TFIRST>>(_first);
		this->_append_multi_<TSECOND, TREST...>(std::forward<TSECOND>(_second), std::forward<TREST>(_rest)...);
	}
	template<class... T>
	constexpr base_t			_append_multi(T&&... _multi)
	{ 
		auto len_old = size_;
		this->_append_multi_<T...>(std::forward<T>(_multi)...);
		return base_t(data_ + len_old, size_ - len_old);
	}

	template <class T>
	constexpr auto				_append_text_integer(char _value) { return this->_append_text_integer<T>(static_cast<int64_t>(_value));}
	template <class T>
	constexpr auto				_append_text_integer(int8_t _value) { return this->_append_text_integer<T>(static_cast<int64_t>(_value));}
	template <class T>
	constexpr auto				_append_text_integer(uint8_t _value) { return this->_append_text_integer<T>(static_cast<uint64_t>(_value));}
	template <class T>
	constexpr auto				_append_text_integer(int16_t _value) { return this->_append_text_integer<T>(static_cast<int64_t>(_value));}
	template <class T>
	constexpr auto				_append_text_integer(uint16_t _value) { return this->_append_text_integer<T>(static_cast<uint64_t>(_value));}
	template <class T>
	constexpr auto				_append_text_integer(int32_t _value) { return this->_append_text_integer<T>(static_cast<int64_t>(_value));}
	template <class T>
	constexpr auto				_append_text_integer(uint32_t _value) { return this->_append_text_integer<T>(static_cast<uint64_t>(_value));}
#if defined(_MSC_VER)
	template <class T>
	constexpr auto				_append_text_integer(long _value) { return this->_append_text_integer<T>(static_cast<int64_t>(_value));}
	template <class T>
	constexpr auto				_append_text_integer(unsigned long _value) { return this->_append_text_integer<T>(static_cast<uint64_t>(_value));}
#else
	template <class T>
	constexpr auto				_append_text_integer(long long _value) { return this->_append_text_integer<T>(static_cast<int64_t>(_value));}
	template <class T>
	constexpr auto				_append_text_integer(unsigned long long _value) { return this->_append_text_integer<T>(static_cast<uint64_t>(_value));}
#endif
	template <class T>
	constexpr base_t			_append_text_integer(uint64_t _value)
	{
		T temp_buf[32]{};
		auto pos_now = temp_buf;

		if (_value != 0)
		{
			while (_value)
			{
				*(pos_now++) = _value % 10 + '0';
				_value /= 10;
			}
		}
		else
		{
			*(pos_now++) = '0';
		}

		auto buf_dest = this->data_ + this->size_;

		base_t result(buf_dest, (pos_now - temp_buf) * sizeof(T));

		_CGD_BUFFER_BOUND_CHECK(result.size<int64_t>() <= reinterpret_cast<const char*>(this->get_upper_bound()) - buf_dest);

		for (--pos_now; pos_now >= temp_buf; ++buf_dest, --pos_now)
			*buf_dest = *pos_now;

		this->size_ += result.size() * sizeof(T);

		return result;
	}
	template <class T>
	constexpr base_t			_append_text_integer(int64_t _value)
	{
		T temp_buf[32]{};
		auto pos_now = temp_buf;

		if (_value > 0)
		{
			do
			{
				*(pos_now++) = _value % 10 + '0';
				_value /= 10;
			} while (_value);
		}
		else if (_value == 0)
		{
			*(pos_now++) = '0';
		}
		else
		{
			_value = ~_value + 1;

			do
			{
				*(pos_now++) = _value % 10 + '0';
				_value /= 10;
			} while (_value);

			*(pos_now++) = '-';
		}

		// 2) copy
		auto buf_dest = this->data_ + this->size_;

		base_t result(buf_dest, (pos_now - temp_buf) * sizeof(T));

		_CGD_BUFFER_BOUND_CHECK(result.size<int64_t>() <= reinterpret_cast<const char*>(this->get_upper_bound()) - buf_dest);

		for (--pos_now; pos_now >= temp_buf; ++buf_dest, --pos_now)
			*buf_dest = *pos_now;

		this->size_ += result.size() * sizeof(T);

		return result;
	}
	template <class T>
	constexpr base_t			_append_text_float(double _value)
	{
		auto temp = _float_to_string<T>(_value);
		return _append_text(static_cast<std::basic_string_view<T>>(temp));
	}

	template <class T = char>
	constexpr base_t			_append_text_binary(const char* _buffer, std::size_t _length)
	{
		CGDK_ASSERT(_buffer != nullptr || (_buffer == nullptr && _length == 0), throw std::invalid_argument("_buffer is nullptr [1]"));

		auto size_added = _length * sizeof(T) * 2;

		_CGD_BUFFER_BOUND_CHECK(size_added <= this->_get_upper_distance_from_end());

		auto buf_dest = reinterpret_cast<T*>(this->data_ + this->size_);
	
		if(_length == 0)
			return base_t(buf_dest, 0);
	
		auto buf_src = reinterpret_cast<const uint8_t*>(_buffer);
		auto buf_src_end = buf_src + _length;

		while(buf_src != buf_src_end)
		{
			auto temp = *buf_src++;
			*buf_dest++ = __hex_to_ascii(temp >> 4);
			*buf_dest++ = __hex_to_ascii(temp & 0x0f);
		}
	
		this->size_ += size_added;
	
		return base_t(buf_dest, size_added);
	}

	template<class T>
	constexpr base_t			_append_container_associative(const T& _container)
	{
		auto l = size_;

		_append_general<COUNT_T>(static_cast<COUNT_T>(_container.size()));

		for (auto iter = _container.begin(); iter != _container.end(); ++iter)
		{
			this->_append<typename T::key_type>(iter->first);
			this->_append<typename T::mapped_type>(iter->second);
		}

		return base_t(this->data_ + l, this->size_ - l);
	}

	template<class I>
	constexpr base_t			_append_iterator(I _first, I _last) requires(is_iterator<I>::value)
	{
		auto l = this->size_;

		auto buf_count = _append_skip(sizeof(COUNT_T));

		COUNT_T iter_count = 0;

		for (; _first != _last; ++_first, ++iter_count)
			_append<typename I::value_type>(*_first);

		buf_count.template front<COUNT_T>() = iter_count;

		return base_t(this->data_ + l, this->size_ - l);
	}
	template <class T>
	constexpr base_t			_append_container_array(const T& _container) requires(!is_memcopyable<typename T::value_type>::value)
	{
		auto l = this->size_;

		this->_append_general<COUNT_T>(static_cast<COUNT_T>(_container.size()));

		auto iter = _container.begin();
		auto iterEnd = _container.end();

		for (; iter != iterEnd; ++iter)
			this->_append<typename T::value_type>(*iter);

		return base_t(this->data_ + l, this->size_ - l);
	}
	template <class T>
	constexpr base_t			_append_container_array(const T& _container) requires(is_memcopyable<typename T::value_type>::value)
	{
		auto l = this->size_;

		auto count = _container.size();

		this->_append_general<COUNT_T>(static_cast<COUNT_T>(count));

		if (count != 0)
			this->_append_bytes(count * sizeof(typename T::value_type), &_container.front());

		return base_t(this->data_ + l, this->size_ - l);
	}
	template <class T>
	constexpr base_t			_append_container_own_ptr_list(const T& _container)
	{
		auto l = this->size_;

		this->_append_general<COUNT_T>(static_cast<COUNT_T>(_container.size()));

		auto iter = _container.begin();
		auto iterEnd = _container.end();

		for (; iter != iterEnd; ++iter)
		{
			if(iter->empty()==false)
				this->_append<typename T::value_type>(*iter);
			else
				this->_append<COUNT_T>(COUNT_T(0)-1);
		}

		// return)
		return base_t(data_ + l, size_ - l);
	}
	template <class T>
	constexpr base_t			_append_container_object_ptr_list(const T& _container)
	{
		auto l = this->size_;

		this->_append_general<COUNT_T>(static_cast<COUNT_T>(_container.size()));

		auto iter = _container.begin();
		auto iterEnd = _container.end();

		for (; iter != iterEnd; ++iter)
		{
			if(iter->empty()==false)
				this->_append<typename T::value_type>(*iter);
			else
				this->_append<COUNT_T>(COUNT_T(0)-1);
		}

		return base_t(this->data_ + l, this->size_ - l);
	}
	template <class T>
	constexpr base_t			_append_container_list(const T& _container)
	{
		auto l = this->size_;

		this->_append_general<COUNT_T>(static_cast<COUNT_T>(_container.size()));

		auto iter = _container.begin();
		auto iterEnd = _container.end();

		for(; iter!=iterEnd; ++iter)
			this->_append<typename T::value_type>(*iter);

		return base_t(this->data_ + l, this->size_ - l);
	}

public:
	// 10) begin/end)
	constexpr self_t			_begin(int _offset) const noexcept { return self_t(this->base_t::_begin(_offset), bound);}
	constexpr self_t			_end(int _offset) const noexcept { return self_t(this->base_t::_end(_offset), bound);}

	// 11) append/extract
	constexpr self_t			_extract_buffer();

	constexpr bool				_is_buffer_overflow() const
								{
									if(this->get_upper_bound() == nullptr)
										return false;

									if(this->data_ > this->get_lower_bound())
										return false;

									if((this->data_ + this->size_) <= this->get_upper_bound())
										return false;

									return true;
								}

	constexpr const void*		get_lower_bound() const noexcept { return bound.lower;}
	constexpr const void*		get_upper_bound() const noexcept { return bound.upper;}
protected:
	constexpr void				_set_bound(const buffer_bound&  _bound) { bound = _bound;}

	constexpr void				_check_bound(const _buffer_view<char>& _rhs) const
								{
									if(_rhs.empty())
										return;

									CGDK_ASSERT(_rhs.get_front_ptr() >= this->get_lower_bound());

									if(_rhs.get_front_ptr() < this->get_lower_bound())
										throw std::length_error("buffer overflow. out of lower bound'");									

									CGDK_ASSERT(_rhs.get_back_ptr() <= this->get_upper_bound());

									if (_rhs.get_back_ptr() > this->get_upper_bound())
										throw std::length_error("buffer overflow. out of upper bound'");
								}
	// 11) source
			buffer_bound		bound;
};

template <class U>
constexpr CGDK::_basic_buffer<U> operator ^ (const CGDK::_basic_buffer<U>& _lhs, std::size_t _size)
{
	return CGDK::_basic_buffer<U>{ ((const CGDK::_buffer_view<U>&)_lhs) ^ _size, _lhs.get_bound() };
}

template <class U>
constexpr CGDK::_basic_buffer<U> operator ^ (const CGDK::_basic_buffer<U>& _lhs, CGDK::_buffer_view<U> _buffer)
{
	return CGDK::_basic_buffer<U>{ _buffer, _lhs.get_bound() };
}


template <class T, class V, std::size_t N>
constexpr _basic_buffer<T> to_basic_buffer(V(&_memory)[N])
{
	return _basic_buffer<T>{ typename _basic_buffer<T>::base_t{ _memory, 0 }, buffer_bound{ _memory, _memory + N }};
}

template <class T, class V, std::size_t N>
constexpr _basic_buffer<T> to_basic_buffer(T(&_memory)[N], std::size_t _size)
{
	return _basic_buffer<T>{ typename _basic_buffer<T>::base_t{ _memory, _size }, buffer_bound{ _memory, _memory + N }};
}

template <class T, class V>
constexpr _basic_buffer<T> to_basic_buffer(std::basic_string_view<V> _string)
{
	return _basic_buffer<T>{ typename _basic_buffer<T>::base_t{ reinterpret_cast<T*>(const_cast<V*>(_string.data())), (_string.size() * sizeof(V)) / sizeof(T) }};
}


}

#include "cgdk/buffers/_basic_buffer.inl"
