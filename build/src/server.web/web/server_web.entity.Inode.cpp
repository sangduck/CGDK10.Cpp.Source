#include "pch.h"

std::u8string url_encoding(std::u8string_view _url);

namespace CGDK
{

server::web::entity::Inode::Inode() noexcept
{
}

server::web::entity::Inode::~Inode() noexcept
{
	CGASSERT_ERROR(m_pentity_parent.empty());
}

bool server::web::entity::Inode::attach_child(Ientity* _pchild)
{
	// check) _pchild가 nullptr이면 안됀다.
	CGASSERT_ERROR(_pchild != nullptr);

	// check) _pchild가 nullptr이면 안됀다.
	THROW_IF(_pchild == nullptr, throwable::failure(eRESULT::LOGIC_ERROR));

	// check) 자기자신이면 안됀다.
	CGASSERT_ERROR(_pchild != this);

	// check) 
	THROW_IF(_pchild == this, throwable::failure(eRESULT::LOGIC_ERROR));

	// 1) get child name
	auto str_name = _pchild->get_name();
	auto str_utf8 = encode_string<char8_t, char>(str_name);

	// 2) url encoding
	//auto str_url = url_encoding(str_utf8);
	auto str_url_xxx = url_encoding(str_utf8);
	auto str_url = std::string(reinterpret_cast<const char*>(str_url_xxx.data()), str_url_xxx.size());

	// declare) 
	std::string str_key;

	scoped_lock(this->m_map_child)
	{
		// check) Child의 parent가 자기와 같아서도 안됀다.
		THROW_IF(_pchild->m_pentity_parent == this, throwable::failure(eRESULT::LOGIC_ERROR));

		// check) Child의 parent가 자기와 같아서도 안됀다.
		THROW_IF(_pchild->m_pentity_parent != nullptr, throwable::failure(eRESULT::LOGIC_ERROR));

		// 2) attach to child
		auto result = this->m_map_child.insert(std::make_pair(str_url, _pchild));

		// check) 
		RETURN_IF(result.second == false, false);

		// 3) Iterator를 넣는다.
		_pchild->m_iter_parent = result.first;
		_pchild->m_pentity_parent = this;
	}

	// return) 
	return true;
}

void server::web::entity::Inode::detach_child(Ientity* _pchild) noexcept
{
	// check) 
	RETURN_IF(_pchild == nullptr);

	// check) 자기자신이면 안됀다.
	CGASSERT_ERROR(_pchild != this);

	// check) 
	RETURN_IF(_pchild == this);

	// locked) 
	{
		// scoped_lock) 
		std::lock_guard<lockable<>> cs(this->m_map_child);

		// check) 
		RETURN_IF(_pchild->m_pentity_parent != this);

		// 1) 제거하기...
		this->m_map_child.erase(_pchild->m_iter_parent);

		// 2) 제거...
		_pchild->m_pentity_parent.reset();
	}
}

void server::web::entity::Inode::detach_child_all() noexcept
{
	// locked) 
	{
		// scoped_lock) 
		std::lock_guard<lockable<>> cs(this->m_map_child);

		// 1) 일단 부모 정보 모두 제거
		for (auto& iter: this->m_map_child)
		{
			iter.second->m_pentity_parent.reset();
		}

		// 2) Reset!
		this->m_map_child.clear();
	}
}

object_ptr<server::web::Ientity> server::web::entity::Inode::find_child_entity(std::string_view _source) const noexcept
{
	// locked) 
	{
		std::lock_guard<lockable<>> cs(const_cast<lockable<std::map<std::string, object_ptr<server::web::Ientity>>>&>(this->m_map_child));

		// 1) find
		auto iter_find = this->m_map_child.find(_source.data());

		// check) 
		RETURN_IF(iter_find == this->m_map_child.end(), object_ptr<server::web::Ientity>());

		// return) 
		return iter_find->second;
	}
}

// 1) Update
result_code server::web::entity::Inode::process_update(context& _context)
{
	scoped_lock(this->m_map_child)
	{
		for (auto& iter : this->m_map_child)
		{
			iter.second->process_update(_context);
		}
	}

	// return)
	return eRESULT::SUCCESS;
}


}