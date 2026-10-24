#include "pch.h"

using namespace CGDK;

allocator_manager::allocator_manager()
{

}

allocator_manager::~allocator_manager() noexcept
{

}

object_ptr<allocator_manager::allocator> allocator_manager::get_allocator(const std::string& _name) const
{
	// lock)
	std::lock_guard lock(this->m_lockable_map_allocator);

	// 1) find allocator
	auto iter_find = this->m_map_allocator.find(_name);

	// check) return if not found
	if(iter_find == this->m_map_allocator.end())
		return {};

	// 2) return allocator
	return iter_find->second.get_object_ptr();
}

void allocator_manager::register_allocator(std::string&& _name, own_ptr<allocator_manager::allocator>&& _allocator)
{
	// lock)
	std::lock_guard lock(this->m_lockable_map_allocator);

	// 1) find allocator
	auto iter_find = this->m_map_allocator.find(_name);

	// check) already registered
	if(iter_find != this->m_map_allocator.end())
		throw throwable::failure(eRESULT::ALREADY_EXIST, 0, "class already exists"sv);

	// 2) insert allocator
	this->m_map_allocator.insert({std::move(_name), std::move(_allocator)});
}

void allocator_manager::unregister_allocator(std::string&& _name) noexcept
{
	// lock)
	std::lock_guard lock(this->m_lockable_map_allocator);

	// 1) find allocator
	auto iter_find = this->m_map_allocator.find(_name);

	// check) return if not found
	if(iter_find == this->m_map_allocator.end())
		return;

	// 2) erase allocator
	this->m_map_allocator.erase(iter_find);
}
