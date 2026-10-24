 //*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                             Archive Classes                               *
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
// - std::basic_string<...> (context::operator T() must not be used for strings)
template <class>				struct _is_context_basic_string : std::false_type {};
template <class A, class B, class C>	struct _is_context_basic_string<std::basic_string<A, B, C>> : std::true_type {};
template <class>				struct _is_context_basic_string_view : std::false_type {};
template <class A, class B>		struct _is_context_basic_string_view<std::basic_string_view<A, B>> : std::true_type {};

//-----------------------------------------------------------------------------
//
// context
//
//
//-----------------------------------------------------------------------------
class context : public Ibuffer_serializable
{
public:
			enum class value_type_t : uint32_t
			{
				null_				 = 0,
				number_				 = 1,
				double_				 = 2,
				bool_				 = 3,
				chrono_duration_	 = 4,
				chrono_time_point_	 = 5,
				node_				 = 6,
				string_				 = 7,
				buffer_				 = 8,
				ptr_				 = 9,
			};

			struct value_t
			{
			public:
				value_t();
				value_t(const value_t& _copy);
				value_t(value_t&& _move) noexcept;
				~value_t();

				value_t& operator = (const value_t& _copy);
				value_t& operator = (value_t&& _move) noexcept;
				void clear() noexcept;
				void _free_value_object() noexcept;
			public:
				union
				{
					int64_t			value_int64;
					uint64_t		value_uint64{ 0 };
					double			value_double;
					std::string*	value_string;
					buffer_view		value_buffer;
					void*			value_ptr;
				};
				typename shared_buffer::object_ptr_t original;

				value_type_t	type{ value_type_t::null_ };
			};

			value_t				v;
			context*			pnode_parent {nullptr};
			using map_nodes_t = std::map<std::string, context, std::less<>>;
			map_nodes_t			map_nodes;

			using iterator = map_nodes_t::iterator;
			using const_iterator = map_nodes_t::const_iterator;
public:
			context() noexcept {}
			context(const context& _copy);
			context(context&& _move) noexcept;
			~context() noexcept { this->clear(); }

	[[nodiscard]] auto			begin() noexcept -> iterator { return this->map_nodes.begin(); }
	[[nodiscard]] auto			end() noexcept -> iterator { return this->map_nodes.end(); }
	[[nodiscard]] auto			begin() const noexcept -> const_iterator { return this->map_nodes.begin(); }
	[[nodiscard]] auto			end() const noexcept -> const_iterator { return this->map_nodes.end(); }
	[[nodiscard]] auto			size() const noexcept -> size_t { return this->map_nodes.size(); }
	[[nodiscard]] auto			type() const noexcept -> value_type_t { return v.type; }

	[[nodiscard]] bool			is_valid() const noexcept { return v.type != value_type_t::null_; }
	[[nodiscard]] bool			is_invalid() const noexcept { return !is_valid(); }

			void				from_json(std::string_view _data);
			shared_buffer		to_json() const;

			bool				erase(std::string_view _key) noexcept;
	[[nodiscard]] bool			has_key(std::string_view _key) const noexcept;
	[[nodiscard]] context&		find(std::string_view _key);
	[[nodiscard]] auto			find(std::string_view _key) const -> const context&;
	// - try_find : no node is created. nullptr if the key does not exist or the value is null
	[[nodiscard]] const context*	try_find(std::string_view _key) const { auto pnode = this->_find_node(_key); return (pnode != nullptr && pnode->is_valid()) ? pnode : nullptr; }
	[[nodiscard]] context*		try_find(std::string_view _key) { auto pnode = this->_find_node(_key); return (pnode != nullptr && pnode->is_valid()) ? pnode : nullptr; }
			template <class... TARGS>
	[[nodiscard]] auto			find(std::string_view _key_a, TARGS... _key_args) -> context& { return const_cast<context&>(this->_find(_key_a, _key_args...));}
			template <class... TARGS>
	[[nodiscard]] auto			find(std::string_view _key_a, TARGS... _key_args) const -> const context& { return this->_find(_key_a, _key_args...); }

			context&			insert(std::string_view _key, value_type_t _value_type = value_type_t::null_);
			context&			insert_bool(std::string_view _key, bool _value);
			context&			insert_string(std::string_view _key, std::string_view _value);
			context&			insert_integer(std::string_view _key, int64_t _value);
			context&			insert_double(std::string_view _key, double _value);
			void				append(const context& _context);
			void				append(context&& _context);

			template<class T>
	[[nodiscard]] T&			ref_value() const;
			void				clear() noexcept;
			void				swap(context& _rhs) noexcept;

			context&			operator [] (const char* _key) { return this->find(_key); }
			const context&		operator [] (const char* _key) const { return this->find(_key); }
			context&			operator [] (const std::string_view& _key) { return this->find(_key); }
			const context&		operator [] (const std::string_view& _key) const { return this->find(_key); }
			context&			operator [] (int _index);
			const context&		operator [] (int _index) const;
																					  
			context&			operator =	(bool _rhs);
			context&			operator =	(char _rhs);
			context&			operator =	(unsigned char _rhs);
			context&			operator =	(int16_t _rhs);
			context&			operator =	(uint16_t _rhs);
			context&			operator =	(int32_t _rhs);
			context&			operator =	(uint32_t _rhs);
			context&			operator =	(int64_t _rhs);
			context&			operator =	(uint64_t _rhs);
			context&			operator =	(float _rhs);
			context&			operator =	(double _rhs);
			context&			operator =	(const char* _rhs);
			context&			operator =	(const std::string_view& _rhs);
			context&			operator =	(const std::string& _rhs);
			context&			operator =	(std::string&& _rhs);
			context&			operator =	(const buffer_view& _rhs);
			context&			operator =	(const shared_buffer&  _rhs);
			context&			operator =	(shared_buffer&& _rhs);
			context&			operator =	(const chrono::tick::time_point& _rhs);
			context&			operator =	(const chrono::time::local::time_point& _rhs);
			context&			operator =	(const chrono::time::utc::time_point& _rhs);
			template <class A, class B>
			context&			operator =	(const std::chrono::duration<A,B>& _rhs);
			template <class A, class B=typename A::duration>
			context&			operator =	(const std::chrono::time_point<A,B>& _rhs);
			context&			operator =  (const context& _rhs);
			context&			operator =  (context&& _rhs) noexcept;

			template <class T>
			std::enable_if_t<!is_string_type<T>::value, context&>
								operator =  (const T* _rhs);
			template<class T>
			std::enable_if_t<!std::is_pointer_v<T>, context&>
								operator =	(const T& _rhs);

	[[nodiscard]]				operator bool() const;
	[[nodiscard]]				operator char() const;
	[[nodiscard]]				operator unsigned char() const;
	[[nodiscard]]				operator int16_t() const;
	[[nodiscard]]				operator uint16_t() const;
	[[nodiscard]]				operator int32_t() const;
	[[nodiscard]]				operator uint32_t() const;
	[[nodiscard]]				operator int64_t() const;
	[[nodiscard]]				operator uint64_t() const;
	[[nodiscard]]				operator float() const;
	[[nodiscard]]				operator double() const;
			template <class A, class B>
	[[nodiscard]]				operator std::chrono::duration<A, B>() const { if (this->v.type != value_type_t::chrono_duration_) throw std::exception(); return std::chrono::duration<A, B>(this->v.value_int64); }
			template <class A, class B = typename A::duration>
	[[nodiscard]]				operator std::chrono::time_point<A, B>() const { if (this->v.type != value_type_t::chrono_time_point_) throw std::exception(); return std::chrono::time_point<A, B>(B(this->v.value_int64)); }
			template<class A, class B, class C>
	[[nodiscard]]				operator std::basic_string<A, B, C>() const;
	[[nodiscard]]				operator std::string_view() const;				// - only for string value (others throw)
	[[nodiscard]]				operator buffer_view() const;
	[[nodiscard]]				operator shared_buffer() const;
			template <class T>
			requires (!_is_context_basic_string<std::remove_cvref_t<T>>::value && !_is_context_basic_string_view<std::remove_cvref_t<T>>::value)
	[[nodiscard]]				operator T() const;

	[[nodiscard]] bool			equal(const context& _rhs) const noexcept;
	[[nodiscard]] bool			greater(const context& _rhs) const noexcept;

	[[nodiscard]] bool			equal(std::string_view _rhs) const noexcept;
	[[nodiscard]] bool			greater(std::string_view _rhs) const noexcept;
	[[nodiscard]] bool			less(std::string_view _rhs) const noexcept;

	[[nodiscard]] bool			equal(int64_t _rhs) const noexcept;
	[[nodiscard]] bool			greater(int64_t _rhs) const noexcept;
	[[nodiscard]] bool			less(int64_t _rhs) const noexcept;

	[[nodiscard]] bool			equal(uint64_t _rhs) const noexcept;
	[[nodiscard]] bool			greater(uint64_t _rhs) const noexcept;
	[[nodiscard]] bool			less(uint64_t _rhs) const noexcept;

	[[nodiscard]] bool			equal(double _rhs) const noexcept;
	[[nodiscard]] bool			greater(double _rhs) const noexcept;
	[[nodiscard]] bool			less(double _rhs) const noexcept;

				void			_copy_context(const context& _rhs);
				void			_move_context(context&& _rhs);
				void			_validate_parent_nodes();
				void			_invalidate_parent_nodes() noexcept;
				void 			_free_child_nodes() noexcept;
			template <class... TARGS>
				const context&	_find(std::string_view _key_a, TARGS... _key_args) const;
				const context*	_find_node(std::string_view _key) const;
				context*		_find_node(std::string_view _key);
				context&		_insert_node(std::string_view _key);

	virtual	void				process_serialize_in(buffer_view& _buffer) override;
	virtual	void				process_serialize_out(buffer& _buffer) override;
	virtual std::size_t			process_size_of() const override;
};

template <class... TARGS>
const context& context::_find(std::string_view _key_a, TARGS... _key_args) const
{
	// check)
	THROW_IF(_key_a.size() >= 256, throwable::failure(eRESULT::KEY_LENGTH_ERROR, 0, "key length is too long(>=256)"sv));

	// 1) find key (case-insensitive)
	auto pnode = this->_find_node(_key_a);

	// check) return if find
	if (pnode != nullptr)
	{
		return *pnode;
	}

	// 3) find 
	return find(_key_args...);
}

template <class T>
 T& context::ref_value() const
{
	// check)
	if (this->v.value_string == nullptr)
		throw std::exception();

	// check)
	if (this->v.type != value_type_t::ptr_)
		throw std::exception();

	// 1) ...
	return *reinterpret_cast<T*>(this->v.value_ptr);
}

 template <class A, class B>
 context& context::operator = (const std::chrono::duration<A, B>& _rhs)
 {
	 // check)
	 THROW_IF(this->v.type == context::value_type_t::node_, throwable::failure(eRESULT::INVALID_VALUE, 0, "value is node_type."sv));

	 // 1) prepare value object
	 this->v._free_value_object();

	 // 2) set value
	 this->v.value_int64 = _rhs.count();
	 this->v.type = value_type_t::chrono_duration_;

	 // 3) validate child
	 this->_validate_parent_nodes();

	 // return)  
	 return *this;
 }

 template <class A, class B>
 context& context::operator = (const std::chrono::time_point<A, B>& _rhs)
 {
	 // check)
	 THROW_IF(this->v.type == context::value_type_t::node_, throwable::failure(eRESULT::INVALID_VALUE, 0, "value is node_type."sv));

	// 1) prepare value object
	 this->v._free_value_object();

	// 2) set value
	 this->v.value_int64 = _rhs.time_since_epoch().count();
	 this->v.type = value_type_t::chrono_time_point_;

	// 3) validate child
	this->_validate_parent_nodes();

	// return)  
	return *this;
 }

 template <class T>
 std::enable_if_t<!is_string_type<T>::value, context&>
 context::operator =  (const T* _rhs)
 {
	 // check)
	 THROW_IF(this->v.type == context::value_type_t::node_, throwable::failure(eRESULT::INVALID_VALUE, 0, "value is node_type."sv));

	 // 1) delete old value (string/buffer/...)
	 this->v._free_value_object();
	 this->_free_child_nodes();

	 // 2) Value
	 this->v.value_ptr = const_cast<T*>(_rhs);
	 this->v.type = value_type_t::ptr_;

	 // 3) validate parents nodes
	 this->_validate_parent_nodes();

	 // return)  
	 return *this;
 }

template<class T>
std::enable_if_t<!std::is_pointer_v<T>, context&>
context::operator = (const T& _rhs)
{
	// check)
	THROW_IF(this->v.type == context::value_type_t::node_, throwable::failure(eRESULT::INVALID_VALUE, 0, "value is node_type."sv));

	// 1) prepare value object
	this->v._free_value_object();

	// 2) alloc buffer
	auto size = get_size_of(_rhs);
	
	// 3) alloc buffer
	auto buf_temp = alloc_shared_buffer(size);
	
	// 4) serialize
	buf_temp.template append<T>(_rhs);
	
	// 5) set..
	this->v.value_buffer = buf_temp;
	this->v.original = buf_temp.move_out_source();
	this->v.type = value_type_t::buffer_;

	// 6) validate child
	this->_validate_parent_nodes();

	// return) 
	return *this;
}

template<class A, class B, class C>
context::operator std::basic_string<A, B, C>() const
{
	switch (this->v.type)
	{
	case	context::value_type_t::number_:
			return std::to_string(this->v.value_int64);

	case	context::value_type_t::double_:
			return std::to_string(this->v.value_double);

	case	context::value_type_t::string_:
			return *this->v.value_string;

	case	context::value_type_t::bool_:
			return (this->v.value_int64 != 0) ? "true"s : "false"s;

	case	context::value_type_t::chrono_duration_:
			return std::to_string(this->v.value_int64);

	case	context::value_type_t::chrono_time_point_:
			return std::to_string(this->v.value_int64);

	default:
		return {};
	};
} 

template <class T>
requires (!_is_context_basic_string<std::remove_cvref_t<T>>::value && !_is_context_basic_string_view<std::remove_cvref_t<T>>::value)
context::operator T() const
{
	using TX = std::remove_cvref_t<T>;

	if constexpr (std::is_base_of_v<chrono::tick::time_point, TX>)
	{
		if (this->v.type != value_type_t::chrono_time_point_)
			throw std::exception();

		return chrono::tick::time_point(chrono::tick::duration(this->v.value_int64));
	}
	else if constexpr (std::is_base_of_v<chrono::time::local::time_point, TX>)
	{
		if (this->v.type != value_type_t::chrono_time_point_)
			throw std::exception();

		return chrono::time::local::time_point(chrono::time::duration(this->v.value_int64));
	}
	else if constexpr (std::is_base_of_v<chrono::time::utc::time_point, TX>)
	{
		if (this->v.type != value_type_t::chrono_time_point_)
			throw std::exception();
		
		return chrono::time::utc::time_point(chrono::time::duration(this->v.value_int64));
	}
	else
	{
		// check)
		THROW_IF(this->v.type != context::value_type_t::buffer_, throwable::failure(eRESULT::INVALID_VALUE, 0, "value is not buffer type."sv));

		// 1) get _buffer
		auto buf_temp = this->v.value_buffer;

		// 2) deserialize and return 
		return buf_temp.template extract<TX>();
	}
}

// add
CGDK::context operator + (const CGDK::context& _lhs, const CGDK::context& _rhs);
CGDK::context operator + (CGDK::context&& _lhs, CGDK::context&& _rhs);

// compare with context
inline bool operator == (const CGDK::context& _lhs, const CGDK::context& _rhs) noexcept { return _lhs.equal(_rhs); }
inline bool operator != (const CGDK::context& _lhs, const CGDK::context& _rhs) noexcept { return !_lhs.equal(_rhs); }
inline bool operator >  (const CGDK::context& _lhs, const CGDK::context& _rhs) noexcept { return _lhs.greater(_rhs); }
inline bool operator >= (const CGDK::context& _lhs, const CGDK::context& _rhs) noexcept { return !(_rhs > _lhs); }
inline bool operator <  (const CGDK::context& _lhs, const CGDK::context& _rhs) noexcept { return _rhs > _lhs; }
inline bool operator <= (const CGDK::context& _lhs, const CGDK::context& _rhs) noexcept { return !(_lhs > _rhs); }

// compare with string
inline bool operator == (const CGDK::context& _lhs, std::string_view _rhs) noexcept { return _lhs.equal(_rhs); }
inline bool operator != (const CGDK::context& _lhs, std::string_view _rhs) noexcept { return !_lhs.equal(_rhs); }
inline bool operator >  (const CGDK::context& _lhs, std::string_view _rhs) noexcept { return _lhs.greater(_rhs); }
inline bool operator >= (const CGDK::context& _lhs, std::string_view _rhs) noexcept { return !_lhs.less(_rhs); }
inline bool operator <  (const CGDK::context& _lhs, std::string_view _rhs) noexcept { return _lhs.less(_rhs); }
inline bool operator <= (const CGDK::context& _lhs, std::string_view _rhs) noexcept { return !_lhs.greater(_rhs); }

inline bool operator == (std::string_view _lhs, const CGDK::context& _rhs) noexcept { return _rhs.equal(_lhs); }
inline bool operator != (std::string_view _lhs, const CGDK::context& _rhs) noexcept { return !_rhs.equal(_lhs); }
inline bool operator >  (std::string_view _lhs, const CGDK::context& _rhs) noexcept { return _rhs.less(_lhs); }
inline bool operator >= (std::string_view _lhs, const CGDK::context& _rhs) noexcept { return !_rhs.greater(_lhs); }
inline bool operator <  (std::string_view _lhs, const CGDK::context& _rhs) noexcept { return _rhs.greater(_lhs); }
inline bool operator <= (std::string_view _lhs, const CGDK::context& _rhs) noexcept { return !_rhs.less(_lhs); }

// compare with int
inline bool operator == (const CGDK::context& _lhs, int64_t _rhs) noexcept { return _lhs.equal(_rhs); }
inline bool operator != (const CGDK::context& _lhs, int64_t _rhs) noexcept { return !_lhs.equal(_rhs); }
inline bool operator >  (const CGDK::context& _lhs, int64_t _rhs) noexcept { return _lhs.greater(_rhs); }
inline bool operator <  (const CGDK::context& _lhs, int64_t _rhs) noexcept { return _lhs.less(_rhs); }
inline bool operator >= (const CGDK::context& _lhs, int64_t _rhs) noexcept { return !_lhs.less(_rhs); }
inline bool operator <= (const CGDK::context& _lhs, int64_t _rhs) noexcept { return !_lhs.greater(_rhs); }

inline bool operator == (int64_t _lhs, const CGDK::context& _rhs) noexcept { return _rhs.equal(_lhs); }
inline bool operator != (int64_t _lhs, const CGDK::context& _rhs) noexcept { return !_rhs.equal(_lhs); }
inline bool operator >  (int64_t _lhs, const CGDK::context& _rhs) noexcept { return _rhs.less(_lhs); }
inline bool operator >= (int64_t _lhs, const CGDK::context& _rhs) noexcept { return !_rhs.greater(_lhs); }
inline bool operator <  (int64_t _lhs, const CGDK::context& _rhs) noexcept { return _rhs.greater(_lhs); }
inline bool operator <= (int64_t _lhs, const CGDK::context& _rhs) noexcept { return !_rhs.less(_lhs); }

// compare with uint
inline bool operator == (const CGDK::context& _lhs, uint64_t _rhs) noexcept { return _lhs.equal(_rhs); }
inline bool operator != (const CGDK::context& _lhs, uint64_t _rhs) noexcept { return !_lhs.equal(_rhs); }
inline bool operator >  (const CGDK::context& _lhs, uint64_t _rhs) noexcept { return _lhs.greater(_rhs); }
inline bool operator <  (const CGDK::context& _lhs, uint64_t _rhs) noexcept { return _lhs.less(_rhs); }
inline bool operator >= (const CGDK::context& _lhs, uint64_t _rhs) noexcept { return !_lhs.less(_rhs); }
inline bool operator <= (const CGDK::context& _lhs, uint64_t _rhs) noexcept { return !_lhs.greater(_rhs); }

inline bool operator == (uint64_t _lhs, const CGDK::context& _rhs) noexcept { return _rhs.equal(_lhs); }
inline bool operator != (uint64_t _lhs, const CGDK::context& _rhs) noexcept { return !_rhs.equal(_lhs); }
inline bool operator >  (uint64_t _lhs, const CGDK::context& _rhs) noexcept { return _rhs.less(_lhs); }
inline bool operator >= (uint64_t _lhs, const CGDK::context& _rhs) noexcept { return !_rhs.greater(_lhs); }
inline bool operator <  (uint64_t _lhs, const CGDK::context& _rhs) noexcept { return _rhs.greater(_lhs); }
inline bool operator <= (uint64_t _lhs, const CGDK::context& _rhs) noexcept { return !_rhs.less(_lhs); }

// compare with double
inline bool operator == (const CGDK::context& _lhs, double _rhs) noexcept { return _lhs.equal(_rhs); }
inline bool operator != (const CGDK::context& _lhs, double _rhs) noexcept { return !_lhs.equal(_rhs); }
inline bool operator >  (const CGDK::context& _lhs, double _rhs) noexcept { return _lhs.greater(_rhs); }
inline bool operator <  (const CGDK::context& _lhs, double _rhs) noexcept { return _lhs.less(_rhs); }
inline bool operator >= (const CGDK::context& _lhs, double _rhs) noexcept { return !_lhs.less(_rhs); }
inline bool operator <= (const CGDK::context& _lhs, double _rhs) noexcept { return !_lhs.greater(_rhs); }

inline bool operator == (double _lhs, const CGDK::context& _rhs) noexcept { return _rhs.equal(_lhs); }
inline bool operator != (double _lhs, const CGDK::context& _rhs) noexcept { return !_rhs.equal(_lhs); }
inline bool operator >  (double _lhs, const CGDK::context& _rhs) noexcept { return _rhs.less(_lhs); }
inline bool operator >= (double _lhs, const CGDK::context& _rhs) noexcept { return !_rhs.greater(_lhs); }
inline bool operator <  (double _lhs, const CGDK::context& _rhs) noexcept { return _rhs.greater(_lhs); }
inline bool operator <= (double _lhs, const CGDK::context& _rhs) noexcept { return !_rhs.less(_lhs); }

}
