#include "pch.h"
#include <charconv>

namespace CGDK
{

//-----------------------------------------------------------------------------
// key of context is case-insensitive (stored in lower case)
//  - _lower_key does not allocate when the key is already lower case or short (< 256)
//-----------------------------------------------------------------------------
namespace
{
	class _lower_key
	{
	public:
		explicit _lower_key(std::string_view _key)
		{
			// check) already lower case
			auto iter = std::find_if(_key.begin(), _key.end(), [](char _ch) { return _ch >= 'A' && _ch <= 'Z'; });
			if (iter == _key.end())
			{
				this->m_view = _key;
				return;
			}

			// 1) to lower
			char* pdest = this->m_buffer;
			if (_key.size() > sizeof(this->m_buffer))
			{
				this->m_string.resize(_key.size());
				pdest = this->m_string.data();
			}
			for (std::size_t i = 0; i < _key.size(); ++i)
				pdest[i] = (_key[i] >= 'A' && _key[i] <= 'Z') ? static_cast<char>(_key[i] - 'A' + 'a') : _key[i];

			this->m_view = std::string_view(pdest, _key.size());
		}
		_lower_key(const _lower_key&) = delete;
		_lower_key& operator=(const _lower_key&) = delete;

		[[nodiscard]] std::string_view view() const noexcept { return this->m_view; }

	private:
		std::string_view	m_view;
		char				m_buffer[256]{};
		std::string			m_string;
	};
}

context::value_t::value_t() : value_buffer(0,0)
{
}

context::value_t::value_t(const value_t& _copy)
{
	this->operator = (_copy);
}

context::value_t::value_t(value_t&& _move) noexcept
{
	this->operator = (std::move(_move));
}

context::value_t::~value_t()
{
	this->clear();
}

context::value_t& context::value_t::operator = (const value_t& _copy)
{
	// check) self assignment
	if (this == &_copy)
		return *this;

	// 1) free existing value object
	this->_free_value_object();

	// 2) copy value type
	this->type = _copy.type;

	// 3) copy value
	switch (this->type)
	{
	//case	value_type_t::null_:
	//		break;
	case	value_type_t::number_:
			this->value_uint64 = _copy.value_uint64;
			break;

	case	value_type_t::double_:
			this->value_double = _copy.value_double;
			break;

	case	value_type_t::string_:
			{
				this->value_string = new std::string(*_copy.value_string);
			}
			break;

	case	value_type_t::bool_:
			this->value_uint64 = _copy.value_uint64;
			break;

	case	value_type_t::node_:
			{

			}
			break;

	case	value_type_t::buffer_:
			{
				if (_copy.original.exist())
				{
					// - copy only the range of value_buffer
					//   (value_buffer can be a part of original. copying from original->data() was wrong data and too much copy)
					const auto size_copy = _copy.value_buffer.size();
					auto buf_copy = alloc_shared_buffer(size_copy);
					if (size_copy != 0)
						memcpy(buf_copy.data(), _copy.value_buffer.data(), size_copy);
					buf_copy.set_size(size_copy);
					this->value_buffer = buffer_view(buf_copy.data(), size_copy);
					this->original = buf_copy.move_out_source();
				}
				else
				{
					this->value_buffer = _copy.value_buffer;
				}
			}
			break;

	case	value_type_t::chrono_duration_:
			this->value_uint64 = _copy.value_uint64;
			break;

	case	value_type_t::chrono_time_point_:
			this->value_uint64 = _copy.value_uint64;
			break;

	case	value_type_t::ptr_:
			this->value_buffer = _copy.value_buffer;
			break;

	default:
			this->value_buffer = _copy.value_buffer;
			break;
	}


	// return) 
	return *this;
}

context::value_t& context::value_t::operator = (value_t&& _move) noexcept
{
	// check) self assignment
	if (this == &_move)
		return *this;

	// 1) free existing value object
	this->_free_value_object();

	// 2) move value
	this->value_buffer = _move.value_buffer;
	_move.value_buffer = buffer_view{ 0,0 };

	// 3) move original
	this->original = std::move(_move.original);

	// 4) move type
	this->type = _move.type;
	_move.type = value_type_t::null_;

	// return) 
	return *this;
}

void context::value_t::clear() noexcept
{
	// check)
	if(this->type == context::value_type_t::null_)
		return;

	// 1) free existing value object
	this->_free_value_object();

	// 2) clear value
	this->type = value_type_t::null_;
}

void context::value_t::_free_value_object() noexcept
{
	switch(this->type)
	{
	case	value_type_t::null_:
			break;
	case	value_type_t::number_:
			this->value_uint64 = 0;
			break;
	case	value_type_t::double_:
			this->value_uint64 = 0;
			break;
	case	value_type_t::string_:
			{
				CGASSERT_ERROR(this->value_string != nullptr)
				if (this->value_string != nullptr)
					delete this->value_string;
			}
			break;
	case	value_type_t::bool_:
			this->value_uint64 = 0;
			break;
	case	value_type_t::node_:
			break;
	case	value_type_t::buffer_:
			this->value_buffer = buffer_view{ 0,0 };
			break;
	case	value_type_t::chrono_duration_:
			this->value_uint64 = 0;
			break;
	case	value_type_t::chrono_time_point_:
			this->value_uint64 = 0;
			break;
	case	value_type_t::ptr_:
			this->value_buffer = buffer_view{ 0,0 };
			break;

	default:
			break;
	}

	// 2) reset original
	this->original.reset();
}


context::context(const context& _copy)
{
	this->operator = (_copy);
}

context::context(context&& _move) noexcept
{
	this->operator = (std::move(_move));
}

void context::clear() noexcept
{
	// 1) reset value
	this->v.clear();

	// 2) clear child nodes
	this->_free_child_nodes();

	// 3) invalidate parents nodes
	this->_invalidate_parent_nodes();
}

void context::swap(context& _rhs) noexcept
{
	// check)
	if (this == &_rhs)
		return;

	// 1) swap value and child nodes (pnode_parent is not swapped. it is the position of the node in the tree)
	SWAP(this->v.value_buffer, _rhs.v.value_buffer);
	SWAP(this->v.type, _rhs.v.type);
	SWAP(this->v.original, _rhs.v.original);
	this->map_nodes.swap(_rhs.map_nodes);

	// 2) reset parent of child nodes
	for (auto& iter : this->map_nodes)
		iter.second.pnode_parent = this;
	for (auto& iter : _rhs.map_nodes)
		iter.second.pnode_parent = &_rhs;
}

context::operator bool() const
{
	switch (this->v.type)
	{
	case	context::value_type_t::bool_:
	case	context::value_type_t::number_:
			return this->v.value_int64 != 0;

	case	context::value_type_t::double_:
			return this->v.value_double != 0.0;

	case	context::value_type_t::string_:
			{
				if (this->v.value_string == nullptr)
					throw std::exception();

				if (CGDK::__stricmp(this->v.value_string->c_str(), "true") == 0)
					return true;

				return *this->v.value_string == "1"sv;
			}

	default:
			return false;
	}
}

context::operator std::string_view() const
{
	// check) only string value can be viewed (others have no storage of text)
	THROW_IF(this->v.type != context::value_type_t::string_ || this->v.value_string == nullptr, throwable::failure(eRESULT::INVALID_VALUE, 0, "value is not string type."sv));

	// return)
	return *this->v.value_string;
}

template <class T>
static T __get_value_from_value(const context::value_t& _value)
{
	switch (_value.type)
	{
	case	context::value_type_t::number_:
			return static_cast<T>(_value.value_int64);

	case	context::value_type_t::double_:
			return static_cast<T>(_value.value_double);

	case	context::value_type_t::string_:
			{
				if (_value.value_string == nullptr)
					throw std::exception();

				// - std::from_chars (char / bool : parsed as a number)
				using parse_t = std::conditional_t<std::is_same_v<T, char> || std::is_same_v<T, bool>, int64_t, T>;
				parse_t parsed{};
				std::from_chars(_value.value_string->data(), _value.value_string->data() + _value.value_string->size(), parsed);
				return static_cast<T>(parsed);
			}

	case	context::value_type_t::bool_:
			return static_cast<T>(_value.value_int64 != 0);

	case	context::value_type_t::chrono_duration_:
			return static_cast<T>(_value.value_int64);

	case	context::value_type_t::chrono_time_point_:
			return static_cast<T>(_value.value_int64);

	default:
			throw throwable::failure(eRESULT::INVALID_VALUE, 0, "undefined value."sv);
	};
}

context::operator char() const 
{
	return __get_value_from_value<char>(this->v);
}

context::operator unsigned char() const
{
	return __get_value_from_value<unsigned char>(this->v);
}

context::operator int16_t() const
{
	return __get_value_from_value<int16_t>(this->v);
}

context::operator uint16_t() const
{
	return __get_value_from_value<uint16_t>(this->v);
}

context::operator int32_t() const
{
	return __get_value_from_value<int32_t>(this->v);
}

context::operator uint32_t() const
{
	return __get_value_from_value<uint32_t>(this->v);
}

context::operator int64_t() const
{
	return __get_value_from_value<int64_t>(this->v);
}

context::operator uint64_t() const
{
	return __get_value_from_value<uint64_t>(this->v);
}

context::operator float() const
{
	return __get_value_from_value<float>(this->v);
}

context::operator double() const
{
	return __get_value_from_value<double>(this->v);
}

context::operator buffer_view() const
{
	return (this->v.type == context::value_type_t::buffer_) ? this->v.value_buffer : buffer_view();
}

context::operator shared_buffer() const
{
	// check) parsing에 실패했으면 false를 리턴한다.
	THROW_IF(this->v.type != context::value_type_t::buffer_, throwable::failure(eRESULT::FAIL, 0, "fail to paring json string"sv));

	// 1) set buffer ptr/size
	shared_buffer temp_buffer(this->v.value_buffer);

	// 2) set source buffer
	temp_buffer.set_source(this->v.original);

	// return)
	return temp_buffer;
}

void context::_validate_parent_nodes()
{
	if (this->v.type == context::value_type_t::null_)
		this->v.type = context::value_type_t::node_;

	if (pnode_parent != nullptr)
		pnode_parent->_validate_parent_nodes();
}

void context::_invalidate_parent_nodes() noexcept
{
	// check)
	CGASSERT_ERROR(this->v.type == context::value_type_t::node_ || this->v.type == context::value_type_t::null_);

	// check) is child nodes empty?
	for(const auto& iter: this->map_nodes)
	{
		// check)
		if(iter.second.v.type != context::value_type_t::null_)
			return;
	}

	// 1) reeset node type
	this->v.type = context::value_type_t::null_;

	// 2) invalidate parent node
	if (this->pnode_parent != nullptr)
	{
		this->pnode_parent->_invalidate_parent_nodes();
	}
}

void context::_free_child_nodes() noexcept
{
	// 1) reset parent node
	for (auto& iter : this->map_nodes)
	{
		iter.second.pnode_parent = nullptr;
	}

	// 2) clear
	this->map_nodes.clear();
}

context* context::_find_node(std::string_view _key)
{
	// 1) find (case-insensitive. no allocation)
	_lower_key key(_key);
	auto iter_find = this->map_nodes.find(key.view());

	// return)
	return (iter_find != this->map_nodes.end()) ? &iter_find->second : nullptr;
}

const context* context::_find_node(std::string_view _key) const
{
	// 1) find (case-insensitive. no allocation)
	_lower_key key(_key);
	auto iter_find = this->map_nodes.find(key.view());

	// return)
	return (iter_find != this->map_nodes.end()) ? &iter_find->second : nullptr;
}

context& context::_insert_node(std::string_view _key)
{
	// 1) find (case-insensitive)
	_lower_key key(_key);
	auto iter_find = this->map_nodes.find(key.view());

	// check) return if exist
	if (iter_find != this->map_nodes.end())
		return iter_find->second;

	// 2) add new node
	auto& node_new = this->map_nodes.try_emplace(std::string(key.view())).first->second;
	node_new.pnode_parent = this;

	// return)
	return node_new;
}

context& context::insert(const std::string_view _key, context::value_type_t _value_type)
{
	// 1) get node (added if not exist)
	auto& node_new = this->_insert_node(_key);

	// 2) delete old data
	node_new.v._free_value_object();

	// 3) set value
	node_new.v.value_string = nullptr;
	node_new.v.type = _value_type;
	node_new.pnode_parent = this;

	// 4) 실존 node로 설정한다.
	node_new._validate_parent_nodes();

	// return) 
	return node_new;
}

context& context::insert_string(const std::string_view _key, const std::string_view _value)
{
	// 1) get node (added if not exist)
	auto& node_new = this->_insert_node(_key);

	// 2) delete old data
	node_new.v._free_value_object();

	// 3) set value
	node_new.v.value_string = new std::string(_value);
	node_new.v.type = context::value_type_t::string_;
	node_new.pnode_parent = this;

	// 4) 실존 node로 설정한다.
	node_new._validate_parent_nodes();

	// return) 
	return node_new;
}

context& context::insert_bool(const std::string_view _key, bool _value)
{
	// 1) get node (added if not exist)
	auto& node_new = this->_insert_node(_key);

	// 2) delete old data
	node_new.v._free_value_object();

	// 3) set value
	node_new.v.value_int64 = (_value) ? 1 : 0;
	node_new.v.type = context::value_type_t::bool_;
	node_new.pnode_parent = this;

	// 4) 실존 node로 설정한다.
	node_new._validate_parent_nodes();

	// return) 
	return node_new;
}

context& context::insert_integer(const std::string_view _key, int64_t _value)
{
	// 1) get node (added if not exist)
	auto& node_new = this->_insert_node(_key);

	// 2) delete old data
	node_new.v._free_value_object();

	// 3) set value
	node_new.v.value_int64 = _value;
	node_new.v.type = context::value_type_t::number_;
	node_new.pnode_parent = this;

	// 4) 실존 node로 설정한다.
	node_new._validate_parent_nodes();

	// return) 
	return node_new;
}

context& context::insert_double(const std::string_view _key, double _value)
{
	// 1) get node (added if not exist)
	auto& node_new = this->_insert_node(_key);

	// 2) delete old data
	node_new.v._free_value_object();

	// 3) set value
	node_new.v.value_double = _value;
	node_new.v.type = context::value_type_t::double_;
	node_new.pnode_parent = this;

	// 4) 실존 node로 설정한다.
	node_new._validate_parent_nodes();

	// return) 
	return node_new;
}

bool context::erase(const std::string_view _key) noexcept
{
	// check)
	if(_key.empty())
		return false;

	// 1) find (case-insensitive. _key.data() is not null-terminated)
	_lower_key key(_key);
	auto iter_find = this->map_nodes.find(key.view());

	// check)
	if (iter_find == this->map_nodes.end())
		return false;

	// 2) erase
	iter_find->second.pnode_parent = nullptr;
	this->map_nodes.erase(iter_find);

	// 3) node가 비었으면 invalidate한다.
	if (this->v.type == context::value_type_t::node_)
		this->_invalidate_parent_nodes();

	// return) 
	return true;
}

bool context::has_key(const std::string_view _key) const noexcept
{
	// check)
	if(_key.empty())
		return false;

	// 1) 해당 key를 찾는다.
	auto pnode = this->_find_node(_key);

	// return) 
	return pnode != nullptr && pnode->is_valid();
}

context& context::find(const std::string_view _key)
{
	// - 없으면 신규 추가해서 리턴한다.
	return this->_insert_node(_key);
}

const context& context::find(const std::string_view _key) const
{
	// 1) find
	auto pnode = this->_find_node(_key);

	// check) 없으면 empty context object를 리턴한다.
	if (pnode == nullptr)
	{
		static const context empty_context;
		return empty_context;
	}

	// return) 
	return *pnode;
}

context& context::operator [] (int _index)
{
	// 1) to string
	char str_temp[32]{0};
	*std::to_chars(str_temp, str_temp + (sizeof(str_temp)/sizeof(str_temp[0])) - 1, static_cast<int64_t>(_index)).ptr = 0;

	// check) 
	if(v.type == context::value_type_t::null_)
		v.type = context::value_type_t::node_;
	else if(v.type != context::value_type_t::node_)
		throw throwable::failure(eRESULT::INVALID_ARGUMENT);

	// return) 
	return find(str_temp);
}

const context& context::operator [] (int _index) const
{
	// 1) _index를 문자열로 변환한다.
	char str_temp[32]{0};
	*std::to_chars(str_temp, str_temp + (sizeof(str_temp)/sizeof(str_temp[0])) - 1, static_cast<int64_t>(_index)).ptr = 0;

	// check) value(not node) can't be indexed (null: empty context is returned)
	THROW_IF(v.type != context::value_type_t::null_ && v.type != context::value_type_t::node_, throwable::failure(eRESULT::INVALID_ARGUMENT));

	// return) 
	return find(str_temp);
}

context& context::operator = (const char* _rhs)
{
	// check)
	THROW_IF(this->v.type == context::value_type_t::node_, throwable::failure(eRESULT::INVALID_VALUE, 0, "value is node_type."sv));

	// 1) prepare value object
	this->v._free_value_object();
	this->_free_child_nodes();

	// 2) set value
	this->v.value_string = (_rhs != nullptr) ? new std::string(_rhs) : new std::string();
	this->v.type = context::value_type_t::string_;

	// 3) validate hierarchy
	this->_validate_parent_nodes();

	// return)  
	return *this;
}

context& context::operator = (const std::string_view& _rhs)
{
	// check)
	THROW_IF(this->v.type == context::value_type_t::node_, throwable::failure(eRESULT::INVALID_VALUE, 0, "value is node_type."sv));

	// 1) free exist value object
	this->v._free_value_object();
	this->_free_child_nodes();

	// 2) set value
	v.value_string = new std::string(_rhs);
	v.type = context::value_type_t::string_;

	// 3) validate parents nodes
	_validate_parent_nodes();

	// return)  
	return *this;
}

context& context::operator = (const std::string& _rhs)
{
	// check)
	THROW_IF(this->v.type == context::value_type_t::node_, throwable::failure(eRESULT::INVALID_VALUE, 0, "value is node_type."sv));

	// 1) free exist value object
	this->v._free_value_object();
	this->_free_child_nodes();

	// 2) set value
	this->v.value_string = new std::string(_rhs);
	this->v.type = context::value_type_t::string_;

	// 3) validate parents nodes
	this->_validate_parent_nodes();

	// return)  
	return *this;
}

context& context::operator = (std::string&& _rhs)
{
	// check)
	THROW_IF(this->v.type == context::value_type_t::node_, throwable::failure(eRESULT::INVALID_VALUE, 0, "value is node_type."sv));

	// 1) free exist value object
	this->v._free_value_object();
	this->_free_child_nodes();

	// 2) set value
	this->v.value_string = new std::string(std::move(_rhs));
	this->v.type = context::value_type_t::string_;

	// 3) validate parents nodes
	this->_validate_parent_nodes();

	// return)  
	return *this;
}

context& context::operator = (bool _rhs)
{
	// check)
	THROW_IF(this->v.type == context::value_type_t::node_, throwable::failure(eRESULT::INVALID_VALUE, 0, "value is node_type."sv));

	// 1) free existing value object
	this->v._free_value_object();
	this->_free_child_nodes();

	// 2) set value
	this->v.value_uint64 = (_rhs) ? 1 : 0;
	this->v.type = context::value_type_t::bool_;

	// 3) validate parents nodes
	this->_validate_parent_nodes();

	// return)  
	return *this;
}

context& context::operator = (char _rhs)
{
	// check)
	THROW_IF(this->v.type == context::value_type_t::node_, throwable::failure(eRESULT::INVALID_VALUE, 0, "value is node_type."sv));

	// 1) free existing value object
	this->v._free_value_object();
	this->_free_child_nodes();

	// 2) set value
	this->v.value_uint64 = static_cast<uint64_t>(_rhs);
	this->v.type = context::value_type_t::number_;

	// 3) validate parents nodes
	this->_validate_parent_nodes();

	// return)  
	return *this;
}

context& context::operator = (unsigned char _rhs)
{
	// check)
	THROW_IF(this->v.type == context::value_type_t::node_, throwable::failure(eRESULT::INVALID_VALUE, 0, "value is node_type."sv));

	// 1) free existing value object
	this->v._free_value_object();
	this->_free_child_nodes();

	// 2) set value
	this->v.value_uint64 = static_cast<int64_t>(_rhs);
	this->v.type = context::value_type_t::number_;

	// 3) validate parents nodes
	this->_validate_parent_nodes();

	// return)  
	return *this;
}

context& context::operator = (int16_t _rhs)
{
	// check)
	THROW_IF(this->v.type == context::value_type_t::node_, throwable::failure(eRESULT::INVALID_VALUE, 0, "value is node_type."sv));

	// 1) free existing value object
	this->v._free_value_object();
	this->_free_child_nodes();

	// 2) set value
	this->v.value_int64 = static_cast<int64_t>(_rhs);
	this->v.type = context::value_type_t::number_;

	// 3) validate parents nodes
	this->_validate_parent_nodes();

	// return)  
	return *this;
}

context& context::operator = (uint16_t _rhs)
{
	// check)
	THROW_IF(this->v.type == context::value_type_t::node_, throwable::failure(eRESULT::INVALID_VALUE, 0, "value is node_type."sv));

	// 1) free existing value object
	this->v._free_value_object();
	this->_free_child_nodes();

	// 2) set value
	this->v.value_uint64 = static_cast<int64_t>(_rhs);
	this->v.type = context::value_type_t::number_;

	// 3) validate parents nodes
	this->_validate_parent_nodes();

	// return)  
	return *this;
}

context& context::operator = (int32_t _rhs)
{
	// check)
	THROW_IF(this->v.type == context::value_type_t::node_, throwable::failure(eRESULT::INVALID_VALUE, 0, "value is node_type."sv));

	// 1) free existing value object
	this->v._free_value_object();
	this->_free_child_nodes();

	// 2) set value
	this->v.value_int64 = static_cast<int64_t>(_rhs);
	this->v.type = context::value_type_t::number_;

	// 3) validate parents nodes
	this->_validate_parent_nodes();

	// return)  
	return *this;
}

context& context::operator = (uint32_t _rhs)
{
	// check)
	THROW_IF(this->v.type == context::value_type_t::node_, throwable::failure(eRESULT::INVALID_VALUE, 0, "value is node_type."sv));

	// 1) free existing value object
	this->v._free_value_object();
	this->_free_child_nodes();

	// 2) set value
	this->v.value_uint64 = static_cast<int64_t>(_rhs);
	this->v.type = context::value_type_t::number_;

	// 3) validate parents nodes
	this->_validate_parent_nodes();

	// return)  
	return *this;
}

context& context::operator = (int64_t _rhs)
{
	// check)
	THROW_IF(this->v.type == context::value_type_t::node_, throwable::failure(eRESULT::INVALID_VALUE, 0, "value is node_type."sv));

	// 1) free existing value object
	this->v._free_value_object();
	this->_free_child_nodes();

	// 2) set value
	this->v.value_int64 = static_cast<int64_t>(_rhs);
	this->v.type = context::value_type_t::number_;

	// 3) validate parents nodes
	this->_validate_parent_nodes();

	// return)  
	return *this;
}

context& context::operator = (uint64_t _rhs)
{
	// check)
	THROW_IF(this->v.type == context::value_type_t::node_, throwable::failure(eRESULT::INVALID_VALUE, 0, "value is node_type."sv));

	// 1) free existing value object
	this->v._free_value_object();
	this->_free_child_nodes();

	// 2) set value
	this->v.value_uint64 = static_cast<int64_t>(_rhs);
	this->v.type = context::value_type_t::number_;

	// 3) validate parents nodes
	this->_validate_parent_nodes();

	// return)  
	return *this;
}

context& context::operator = (float _rhs)
{
	// check)
	THROW_IF(this->v.type == context::value_type_t::node_, throwable::failure(eRESULT::INVALID_VALUE, 0, "value is node_type."sv));

	// 1) free existing value object
	this->v._free_value_object();
	this->_free_child_nodes();

	// 2) set value
	this->v.value_double = _rhs;
	this->v.type = context::value_type_t::double_;

	// 3) validate parents nodes
	this->_validate_parent_nodes();

	// return)  
	return *this;
}

context& context::operator = (double _rhs)
{
	// check)
	THROW_IF(this->v.type == context::value_type_t::node_, throwable::failure(eRESULT::INVALID_VALUE, 0, "value is node_type."sv));

	// 1) free existing value object
	this->v._free_value_object();
	this->_free_child_nodes();

	// 2) set value
	this->v.value_double = _rhs;
	this->v.type = context::value_type_t::double_;

	// 3) validate parents nodes
	this->_validate_parent_nodes();

	// return)  
	return *this;
}

context& context::operator = (const buffer_view& _rhs)
{
	// check)
	THROW_IF(this->v.type == context::value_type_t::node_, throwable::failure(eRESULT::INVALID_VALUE, 0, "value is node_type."sv));

	// 1) free existing value object
	this->v._free_value_object();
	this->_free_child_nodes();

	// 2) set value
	this->v.value_buffer = _rhs;
	this->v.type = context::value_type_t::buffer_;
	
	// 3) validate parents nodes
	this->_validate_parent_nodes();

	// return)  
	return *this;
}

context& context::operator = (const shared_buffer& _rhs)
{
	// check)
	THROW_IF(this->v.type == context::value_type_t::node_, throwable::failure(eRESULT::INVALID_VALUE, 0, "value is node_type."sv));

	// 1) free existing value object
	this->v._free_value_object();
	this->_free_child_nodes();

	// 2) set value
	this->v.value_buffer = _rhs;
	this->v.original = _rhs.get_source();
	this->v.type = context::value_type_t::buffer_;

	// 3) validate parents nodes
	this->_validate_parent_nodes();

	// return)  
	return *this;
}

context& context::operator = (shared_buffer&& _rhs)
{
	// check)
	THROW_IF(this->v.type == context::value_type_t::node_, throwable::failure(eRESULT::INVALID_VALUE, 0, "value is node_type."sv));

	// 1) free existing value object
	this->v._free_value_object();
	this->_free_child_nodes();

	//// 2) set value
	this->v.value_buffer = _rhs;
	this->v.original = _rhs.move_out_source();
	this->v.type = context::value_type_t::buffer_;

	// 3) validate parents nodes
	this->_validate_parent_nodes();

	// return)  
	return *this;
}

context& context::operator = (const chrono::tick::time_point& _rhs)
{
	// check)
	THROW_IF(this->v.type == context::value_type_t::node_, throwable::failure(eRESULT::INVALID_VALUE, 0, "value is node_type."sv));

	// 1) prepare value object
	this->v._free_value_object();
	this->_free_child_nodes();

	// 2) set value
	this->v.value_uint64 = _rhs.time_since_epoch().count();
	this->v.type = value_type_t::chrono_time_point_;

	// 3) validate parents nodes
	this->_validate_parent_nodes();

	// return)  
	return *this;
}

context& context::operator = (const chrono::time::local::time_point& _rhs)
{
	// check)
	THROW_IF(this->v.type == context::value_type_t::node_, throwable::failure(eRESULT::INVALID_VALUE, 0, "value is node_type."sv));

	// 1) prepare value object
	this->v._free_value_object();
	this->_free_child_nodes();

	// 2) set value
	this->v.value_uint64 = _rhs.time_since_epoch().count();
	this->v.type = value_type_t::chrono_time_point_;

	// 3) validate parents nodes
	this->_validate_parent_nodes();

	// return)  
	return *this;
}

context& context::operator = (const chrono::time::utc::time_point& _rhs)
{
	// check)
	THROW_IF(this->v.type == context::value_type_t::node_, throwable::failure(eRESULT::INVALID_VALUE, 0, "value is node_type."sv));

	// 1) prepare value object
	this->v._free_value_object();
	this->_free_child_nodes();

	// 2) set value
	this->v.value_uint64 = _rhs.time_since_epoch().count();
	this->v.type = value_type_t::chrono_time_point_;

	// 3) validate parents nodes
	this->_validate_parent_nodes();

	// return)  
	return *this;
}

context& context::operator = (const context& _rhs)
{
	// check) self assignment
	if (this == &_rhs)
		return *this;

	// 1) set (old value cleared)
	this->v = _rhs.v;

	// 2) clear child nodes
	this->_free_child_nodes();

	// declare)
	map_nodes_t temp_map_nodes;

	// 3) copy child nodes
	for(auto& iter : _rhs.map_nodes)
	{
		// skip invalid nodes
		if(iter.second.is_invalid())
			continue;

		// - add new node
		auto temp = temp_map_nodes.try_emplace(iter.first, iter.second);

		// - set parent node
		temp.first->second.pnode_parent = this; 
	}

	// 4) move child nodes
	this->map_nodes = std::move(temp_map_nodes);

	// 5) validate parents nodes
	if (this->v.type != context::value_type_t::null_)
	{
		this->_validate_parent_nodes();
	}

	// return)  
	return *this;
}

context& context::operator = (context&& _rhs) noexcept
{
	// check) self assignment
	if (this == &_rhs)
		return *this;

	// 1) clear child nodes
	this->_free_child_nodes();

	// 2) value move
	this->v = std::move(_rhs.v);

	// 3) move child nodes
	this->map_nodes = std::move(_rhs.map_nodes);

	// 4) set parent node for child nodes
	for (auto& iter : this->map_nodes)
	{
		iter.second.pnode_parent = this;
	}

	// 5) validate parents nodes
	if (v.type != context::value_type_t::null_)
	{
		this->_validate_parent_nodes();
	}

	// return)  
	return *this;
}

void context::process_serialize_in(buffer_view& _buffer)
{
	// function) 
	constexpr auto f_extract_map_context = [](buffer_view& _buffer)
		{
			// declare)
			map_nodes_t temp_map_nodes;

			// - get 'COUNT_T' (copy. 'auto&' was a reference into the buffer and '--count' changed the source buffer)
			COUNT_T count = _buffer.extract<COUNT_T>();

			// - append map node
			for (; count>0; --count)
			{
				auto first = _buffer.extract<std::string>();
				auto second = _buffer.extract<context>();

				temp_map_nodes.insert_or_assign(std::move(first), std::move(second));
			}

			// return)
			return temp_map_nodes;
		};

	// declare) 
	value_t temp_value;
	map_nodes_t temp_map_nodes;

	// 1) get value type
	temp_value.type = static_cast<context::value_type_t>(_buffer.extract<int>());

	// 2) process.. 
	switch (temp_value.type)
	{
	case	context::value_type_t::null_:
			{
			}
			break;

	case	context::value_type_t::number_:
			{
				temp_value.value_int64 = _buffer.extract<uint64_t>();
			}
			break;

	case	context::value_type_t::double_:
			{
				temp_value.value_double = _buffer.extract<double>();
			}
			break;

	case	context::value_type_t::string_:
			{
				temp_value.value_string = new std::string();
				*temp_value.value_string = _buffer.extract<std::string>();
			}
			break;

	case	context::value_type_t::bool_:
			{
				temp_value.value_int64 = _buffer.extract<char>();
			}
			break;

	case	context::value_type_t::node_:
			{
				temp_map_nodes = f_extract_map_context(_buffer);
			}
			break;

	case	context::value_type_t::buffer_:
			{
				auto buf_temp = _buffer.extract<shared_buffer>();
				temp_value.value_buffer = buf_temp;
				temp_value.original = buf_temp.move_out_source();
			}
			break;

	case	context::value_type_t::chrono_duration_:
			{
				temp_value.value_int64 = _buffer.extract<int64_t>();
			}
			break;

	case	context::value_type_t::chrono_time_point_:
			{
				temp_value.value_int64 = _buffer.extract<int64_t>();
			}
			break;

	case	context::value_type_t::ptr_:
			{
			}
			break;
	};

	// 3) set parent node as this
	for (auto iter = temp_map_nodes.begin(); iter != temp_map_nodes.end();)
	{
		if (iter->second.is_valid())
		{
			iter->second.pnode_parent = this;
			++iter;
		}
		else
		{
			iter = temp_map_nodes.erase(iter);
		}
	}

	// 4) move
	this->v = std::move(temp_value);
	this->_free_child_nodes();
	this->map_nodes = std::move(temp_map_nodes);

	// 5) parent of child nodes (address of this)
	for (auto& iter : this->map_nodes)
		iter.second.pnode_parent = this;
}

void context::process_serialize_out(buffer& _buffer)
{
	// function) 
	constexpr auto f_append_map_context = [](buffer & _buffer, const map_nodes_t& _map_nodes)
	{
		// - append 'COUNT_T'
		auto& count_pos = _buffer.append <COUNT_T>(0);

		// declare)
		int count = 0;

		// - append map node
		for (auto iter = _map_nodes.begin(); iter != _map_nodes.end(); ++iter)
		{
			CONTINUE_IF(iter->second.is_invalid());

			_buffer.append(iter->first);
			_buffer.append(iter->second);

			++count;
		}

		// - write count
		count_pos = static_cast<COUNT_T>(count);
	};

	// 1) append context_type
	_buffer.append<int>(static_cast<int>(this->v.type));

	// check) 
	if(this->is_invalid())
		return;

	switch (this->v.type)
	{
	case	context::value_type_t::null_:
			{
			}
			break;

	case	context::value_type_t::number_:
			{
				_buffer.append<uint64_t>(this->v.value_int64);
			}
			break;

	case	context::value_type_t::double_:
			{
				_buffer.append<double>(this->v.value_double);
			}
			break;

	case	context::value_type_t::string_:
			{
				_buffer.append<std::string>(*this->v.value_string);
			}
			break;

	case	context::value_type_t::bool_:
			{
				_buffer.append<char>(static_cast<char>(this->v.value_int64));
			}
			break;

	case	context::value_type_t::node_:
			{
				f_append_map_context(_buffer, this->map_nodes);
			}
			break;

	case	context::value_type_t::buffer_:
			{
				_buffer.append(this->v.value_buffer);
			}
			break;

	case	context::value_type_t::chrono_duration_:
			{
				_buffer.append<uint64_t>(this->v.value_uint64);
			}
			break;

	case	context::value_type_t::chrono_time_point_:
			{
				_buffer.append<uint64_t>(this->v.value_uint64);
			}
			break;

	case	context::value_type_t::ptr_:
			{
			}
			break;
	};
}

std::size_t context::process_size_of() const
{
	// function) 
	constexpr auto f_get_size_of_map_context = [](const map_nodes_t& _map_nodes)
		{
			// declare)
			std::size_t size = sizeof(COUNT_T);

			// - append map node
			for (auto iter = _map_nodes.begin(); iter != _map_nodes.end(); ++iter)
			{
				CONTINUE_IF(iter->second.is_invalid());

				size += get_size_of(iter->first);
				size += get_size_of(iter->second);
			}

			// return) 
			return size;
		};

	// 1) add size of value type
	std::size_t size = sizeof(int);

	// 2) add size by value type
	switch (this->v.type)
	{
	case	context::value_type_t::null_:
			{
			}
			break;

	case	context::value_type_t::number_:
			{
				size += sizeof(uint64_t);
			}
			break;

	case	context::value_type_t::double_:
			{
				size += sizeof(double);
			}
			break;

	case	context::value_type_t::string_:
			{
				size += get_size_of(*this->v.value_string);
			}
			break;

	case	context::value_type_t::bool_:
			{
				size += sizeof(char);
			}
			break;

	case	context::value_type_t::node_:
			{
				size += f_get_size_of_map_context(this->map_nodes);
			}
			break;

	case	context::value_type_t::buffer_:
			{
				size += get_size_of(this->v.value_buffer);
			}
			break;

	case	context::value_type_t::chrono_duration_:
			{
				size += get_size_of(this->v.value_uint64);
			}
			break;

	case	context::value_type_t::chrono_time_point_:
			{
				size += get_size_of(this->v.value_uint64);
			}
			break;

	case	context::value_type_t::ptr_:
			{
			}
			break;
	};

	// return) 
	return size;
}

bool context::equal(const context& _rhs) const noexcept
{
	switch (_rhs.v.type)
	{
	case	context::value_type_t::null_:
			return this->v.type == context::value_type_t::null_;
	case	context::value_type_t::number_:
			return this->equal(_rhs.v.value_int64);
	case	context::value_type_t::double_:
			return this->equal(_rhs.v.value_double);
	case	context::value_type_t::bool_:
	case	context::value_type_t::chrono_duration_:
	case	context::value_type_t::chrono_time_point_:
			return this->v.type == _rhs.v.type && this->v.value_int64 == _rhs.v.value_int64;
	case	context::value_type_t::string_:
			return this->equal(*_rhs.v.value_string);
	case	context::value_type_t::buffer_:
			return this->v.type == _rhs.v.type && this->v.value_buffer.size() == _rhs.v.value_buffer.size()
				&& (this->v.value_buffer.size() == 0 || memcmp(this->v.value_buffer.data(), _rhs.v.value_buffer.data(), this->v.value_buffer.size()) == 0);
	case	context::value_type_t::node_:
			{
				// check)
				if (this->v.type != context::value_type_t::node_)
					return false;

				// - valid child nodes must be equal (null nodes are ignored)
				const auto f_count_valid = [](const map_nodes_t& _map) { return std::count_if(_map.begin(), _map.end(), [](const auto& _iter) { return _iter.second.is_valid(); }); };

				if (f_count_valid(this->map_nodes) != f_count_valid(_rhs.map_nodes))
					return false;

				for (const auto& iter : _rhs.map_nodes)
				{
					if (iter.second.is_invalid())
						continue;

					auto iter_find = this->map_nodes.find(iter.first);

					if (iter_find == this->map_nodes.end() || iter_find->second.equal(iter.second) == false)
						return false;
				}
				return true;
			}
	case	context::value_type_t::ptr_:
			return this->v.type == _rhs.v.type && this->v.value_ptr == _rhs.v.value_ptr;
	default:
			return false;
	};
}

bool context::greater(const context& _rhs) const noexcept
{
	switch (_rhs.v.type)
	{
	case	context::value_type_t::number_:
			return this->greater(_rhs.v.value_int64);
	case	context::value_type_t::double_:
			return this->greater(_rhs.v.value_double);
	case	context::value_type_t::bool_:
	case	context::value_type_t::chrono_duration_:
	case	context::value_type_t::chrono_time_point_:
			return this->v.type == _rhs.v.type && this->v.value_int64 > _rhs.v.value_int64;
	case	context::value_type_t::string_:
			return this->greater(*_rhs.v.value_string);
	default:
			return false;
	};
}

bool context::equal(std::string_view _rhs) const noexcept
{
	// check) 
	if(this->v.type != context::value_type_t::string_)
		return false;

	// return) 
	return (*this->v.value_string) == _rhs;
}

bool context::greater(std::string_view _rhs) const noexcept
{
	// check) 
	if(this->v.type != context::value_type_t::string_)
		return false;

	// return) 
	return (*this->v.value_string) > _rhs;
}

bool context::less(std::string_view _rhs) const noexcept
{
	// check) 
	if(this->v.type != context::value_type_t::string_)
		return false;

	// return) 
	return (*this->v.value_string) < _rhs;
}

bool context::equal(int64_t _rhs) const noexcept
{
	// check) 
	if(this->v.type != context::value_type_t::number_)
		return false;

	// return) 
	return this->v.value_int64 == _rhs;
}

bool context::greater(int64_t _rhs) const noexcept
{
	// check) 
	if(this->v.type != context::value_type_t::number_)
		return false;

	// return) 
	return this->v.value_int64 > _rhs;
}

bool context::less(int64_t _rhs) const noexcept
{
	// check) 
	if(this->v.type != context::value_type_t::number_)
		return false;

	// return) 
	return this->v.value_int64 < _rhs;
}

bool context::equal(uint64_t _rhs) const noexcept 
{
	// check) 
	if(this->v.type != context::value_type_t::number_)
		return false;

	// return) 
	return this->v.value_uint64 == _rhs;
}

bool context::greater(uint64_t _rhs) const noexcept 
{
	// check) 
	if(this->v.type != context::value_type_t::number_)
		return false;

	// return) 
	return this->v.value_uint64 > _rhs;
}

bool context::less(uint64_t _rhs) const noexcept 
{
	// check) 
	if(this->v.type != context::value_type_t::number_)
		return false;

	// return) 
	return this->v.value_uint64 < _rhs;
}

bool context::equal(double _rhs) const noexcept
{
	if (this->v.type == context::value_type_t::number_)
		return static_cast<double>(this->v.value_int64) == _rhs;
	else if (this->v.type == context::value_type_t::double_)
		return this->v.value_double == _rhs;

	// return) 
	return false;
}

bool context::greater(double _rhs) const noexcept
{
	if (this->v.type == context::value_type_t::number_)
		return static_cast<double>(this->v.value_int64) > _rhs;
	else if (this->v.type == context::value_type_t::double_)
		return this->v.value_double > _rhs;

	// return) 
	return false;
}

bool context::less(double _rhs) const noexcept
{
	if (this->v.type == context::value_type_t::number_)
		return static_cast<double>(this->v.value_int64) < _rhs;
	else if (this->v.type == context::value_type_t::double_)
		return this->v.value_double < _rhs;

	// return) 
	return false;
}

void context::append(const context& _context)
{
	// check)
	if (this == &_context)
		return;

	for (auto& iter : _context.map_nodes)
	{
		// check) skip null node
		if (iter.second.is_invalid())
			continue;

		auto iter_find = this->map_nodes.find(iter.first);

		// 1) object + object: merge
		if (iter_find != this->map_nodes.end() && iter_find->second.v.type == context::value_type_t::node_ && iter.second.v.type == context::value_type_t::node_)
		{
			iter_find->second.append(iter.second);
			continue;
		}

		// 2) others: added or overwritten
		auto& node = this->map_nodes.insert_or_assign(iter.first, iter.second).first->second;
		node.pnode_parent = this;
	}

	// 3) validate
	if (this->map_nodes.empty() == false && this->v.type == context::value_type_t::null_)
		this->_validate_parent_nodes();
}

void context::append(context&& _context)
{
	// check)
	if (this == &_context)
		return;

	for (auto& iter : _context.map_nodes)
	{
		// check) skip null node
		if (iter.second.is_invalid())
			continue;

		auto iter_find = this->map_nodes.find(iter.first);

		// 1) object + object: merge
		if (iter_find != this->map_nodes.end() && iter_find->second.v.type == context::value_type_t::node_ && iter.second.v.type == context::value_type_t::node_)
		{
			iter_find->second.append(std::move(iter.second));
			continue;
		}

		// 2) others: added or overwritten
		auto& node = this->map_nodes.insert_or_assign(iter.first, std::move(iter.second)).first->second;
		node.pnode_parent = this;
	}
	_context.clear();

	// 3) validate
	if (this->map_nodes.empty() == false && this->v.type == context::value_type_t::null_)
		this->_validate_parent_nodes();
}

CGDK::context operator + (const CGDK::context& _lhs, const CGDK::context& _rhs)
{
	CGDK::context result = _lhs;

	result.append(_rhs);

	return result;
}

CGDK::context operator + (CGDK::context&& _lhs, CGDK::context&& _rhs)
{
	CGDK::context result = std::move(_lhs);

	result.append(std::move(_rhs));

	return result;
}

}
