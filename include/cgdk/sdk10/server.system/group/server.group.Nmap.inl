namespace CGDK
{

template <class _TMEMBER, class _TKEY, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
group::Nmap<_TMEMBER, _TKEY, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::Nmap(size_t _max_member) noexcept :
	m_enable_enter(false)
{
	process_set_member_capacity(_max_member);
}

template <class _TMEMBER, class _TKEY, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
group::Nmap<_TMEMBER, _TKEY, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::~Nmap() noexcept
{
	leave_all();
}

template <class _TMEMBER, class _TKEY, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
void group::Nmap<_TMEMBER, _TKEY, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::process_set_member_capacity(size_t _max_count)
{

	std::lock_guard lock(this->m_lockable_group);

	m_count_max_member = _max_count;

#if defined(_MSC_VER)
	__if_exists(container_t::reserve)
	{
		this->m_container_member.reserve((_max_count>256) ? 256:_max_count);
	}
#endif
}

template <class _TMEMBER, class _TKEY, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
typename group::Nmap<_TMEMBER, _TKEY, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::member_data_t& group::Nmap<_TMEMBER, _TKEY, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::member_data(const member_t* _pmember)
{

	return ((member_set_t*)_pmember->member_set())->m_member_data;
}

template <class _TMEMBER, class _TKEY, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
typename group::Nmap<_TMEMBER, _TKEY, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::member_snapshot_t group::Nmap<_TMEMBER, _TKEY, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::get_member_snapshot() const
{
	return this->m_member_snapshot.get(this->m_lockable_group, [this]() { return this->_build_member_snapshot(); });
}

template <class _TMEMBER, class _TKEY, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
bool group::Nmap<_TMEMBER, _TKEY, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::for_each_member_snapshot(const std::function<bool (member_t*)>& _function) const
{
	auto snapshot = this->get_member_snapshot();

	for (auto* iter : snapshot)
	{
		if (_function(iter) == false)
			return false;
	}

	return true;
}

template <class _TMEMBER, class _TKEY, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
group::member_snapshot<typename group::Nmap<_TMEMBER, _TKEY, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::member_t>* group::Nmap<_TMEMBER, _TKEY, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::_build_member_snapshot() const
{
	auto psnapshot = member_snapshot<member_t>::create(this->m_container_member.size());

	for (const auto& iter : this->m_container_member)
	{
		psnapshot->push_back((*iter.second).get());
	}

	return psnapshot;
}

template <class _TMEMBER, class _TKEY, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
object_ptr<_TMEMBERBASE> group::Nmap<_TMEMBER, _TKEY, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::find_member(const std::function<bool (_TMEMBERBASE*)>& _pred)
{
	auto snapshot = this->get_member_snapshot();

	for (auto* iter : snapshot)
	{
		if (_pred(iter) == true)
			return object_ptr<_TMEMBERBASE>(iter);
	}

	return object_ptr<_TMEMBERBASE>();
}

template <class _TMEMBER, class _TKEY, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
object_ptr<_TMEMBER> group::Nmap<_TMEMBER, _TKEY, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::find_member(const _TKEY& _key) const noexcept
{

	std::lock_guard lock(this->m_lockable_group);

	auto iter_find = this->m_container_member.find(_key);

	RETURN_IF(iter_find == this->m_container_member.end(), object_ptr<member_t>());

	return iter_find->second.member();
}

template <class _TMEMBER, class _TKEY, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
object_ptr<_TMEMBER> group::Nmap<_TMEMBER, _TKEY, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::find_member(const _TKEY& _key) noexcept
{
	return static_cast<const Nmap*>(this)->find_member(_key);
}

template <class _TMEMBER, class _TKEY, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
bool group::Nmap<_TMEMBER, _TKEY, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::for_each_member(const std::function<bool (_TMEMBERBASE*)>& _function)
{
	auto snapshot = this->get_member_snapshot();

	for (auto* iter : snapshot)
	{
		if (_function(iter) == false)
			return false;
	}

	return true;
}

template <class _TMEMBER, class _TKEY, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
void group::Nmap<_TMEMBER, _TKEY, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::_attach_member(member_t* _pmember, const _TKEY& _key)
{

	CGASSERT_ERROR(this->m_container_member.find(_key)== this->m_container_member.end());

	auto result = this->m_container_member.insert(std::make_pair(_key, member_set_t(this)));

	result.first->second.member(_pmember);

	result.first->second.iter = result.first;

	_pmember->member_set(&result.first->second);

	this->member_group_as((typename _TMEMBER::group_base_t*)this, _pmember);
}

template <class _TMEMBER, class _TKEY, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
void group::Nmap<_TMEMBER, _TKEY, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::_detach_member(member_base_t* _pmember) noexcept
{

	member_set_t* pmember_set = (member_set_t*)_pmember->member_set();

	this->m_container_member.erase(pmember_set->iter);

	_pmember->reset_member_set();

	this->Igroup<_TMEMBERBASE>::reset_member_group(_pmember);
}

template <class _TMEMBER, class _TKEY, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
bool group::Nmap<_TMEMBER, _TKEY, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::process_is_enable_member_enter() const noexcept
{

	std::lock_guard lock(this->m_lockable_enter);

	return this->m_enable_enter;
}

template <class _TMEMBER, class _TKEY, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
bool group::Nmap<_TMEMBER, _TKEY, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::process_enable_member_enter(bool _enable)
{
	SCOPED_LOCK(this->m_lockable_enter)
	{

		RETURN_IF(this->m_enable_enter == _enable, false);

		this->m_enable_enter = _enable;

		if(_enable)
		{
			this->on_enable_enter();
		}
		else
		{
			this->on_disable_enter();
		}
	}

	return true;
}

template <class _TMEMBER, class _TKEY, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
result_code group::Nmap<_TMEMBER, _TKEY, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::process_enter(member_base_t* _pmember, param_t& _param)
{

	RETURN_IF(_pmember == nullptr, eRESULT::INVALID_ARGUMENT);

	std::unique_lock lock_member(this->member_group_lock(_pmember));
	decltype(lock_member) lock_member_old;
	std::unique_lock lock_enter(this->m_lockable_enter, std::defer_lock);

	auto key = this->process_get_member_key(_pmember);

	auto find_member_of_key = [this, &key]()
	{
		std::lock_guard lock(this->m_lockable_group);

		auto iter_find = this->m_container_member.find(key);

		return (iter_find != this->m_container_member.end()) ? iter_find->second.member() : object_ptr<_TMEMBER>();
	};

	object_ptr<_TMEMBER> pmember_old;

	for(;;)
	{

		pmember_old = find_member_of_key();

		if(pmember_old.exist() && pmember_old != _pmember)
		{

			lock_member_old = decltype(lock_member)(this->member_group_lock(pmember_old.get()), std::defer_lock);

			if(lock_member_old.try_lock() == false)
			{
				lock_member.unlock();

				std::lock(lock_member_old, lock_member);
			}
		}

		lock_enter.lock();

		BREAK_IF(find_member_of_key() == pmember_old);

		lock_enter.unlock();

		lock_member_old = decltype(lock_member)();
	}

	{

		RETURN_IF(_pmember->_is_member_of(this) == true, eRESULT::ALREADY_EXIST);

		CGASSERT_ERROR(_pmember->_get_group() != this);

		RETURN_IF(this->m_enable_enter == false, eRESULT::FAIL_DISABLED);

		RETURN_IF(this->is_member_full(), eRESULT::MEMBER_FULL);

		if(pmember_old.exist())
		{

			auto result = this->on_member_duplicated(key, _pmember, pmember_old, _param);

			RETURN_IF(result != eRESULT::SUCCESS, result);
		}

		auto result = this->_process_member_entering(_pmember, _param);

		RETURN_IF(result != eRESULT::SUCCESS, result);

		_pmember->_leave_group();

		SCOPED_LOCK(this->m_lockable_group)
		{
			typename member_snapshot_holder<member_t>::write_scope scope_write(this->m_member_snapshot);

			if(pmember_old.exist())
			{
				this->process_leave(pmember_old, 0);
			}

			if(lock_member_old.owns_lock())
			{
				lock_member_old.unlock();
			}

			this->_attach_member(_pmember, key);

			try
			{
				this->_process_member_entered(_pmember, _param);
			}

			catch(std::exception& _e)
			{

				LOG_FATAL << "(excp) exception complete[on_member_entered]"sv;

				auto is_executed = throwable::execute_exception_handler(this, _e, __FUNCTION__);

				this->_detach_member(_pmember);

				RETURN_IF(is_executed, eRESULT::EXCEPTION);

				throw;
			}

			catch (...)
			{

				LOG_FATAL << "(excp) exception complete[on_member_entered]"sv;

				auto is_executed = throwable::execute_exception_handler(this, __FUNCTION__);

				this->_detach_member(_pmember);

				RETURN_IF(is_executed, eRESULT::EXCEPTION);

				throw;
			}
		}

		CGASSERT_ERROR(_pmember->_get_group() == this);
	}

	return	eRESULT::SUCCESS;
}

template <class _TMEMBER, class _TKEY, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
void group::Nmap<_TMEMBER, _TKEY, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::leave_all() noexcept
{
	std::vector<object_ptr<member_t>> vector_member;

	for(;;)
	{
		SCOPED_LOCK(this->m_lockable_group)
		{
			vector_member.reserve(this->m_container_member.size());

			for(const auto& iter : this->m_container_member)
			{
				vector_member.push_back(*iter.second);
			}

			RETURN_IF(vector_member.empty());
		}

		for(auto& iter : vector_member)
		{
			this->Igroup<_TMEMBERBASE>::leave(iter.get());
		}

		vector_member.clear();
	}
}

template <class _TMEMBER, class _TKEY, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
void group::Nmap<_TMEMBER, _TKEY, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::process_leave(member_base_t* _pmember, uintptr_t _param) noexcept
{

	CGASSERT_FATAL(_pmember != nullptr);

	SCOPED_LOCK(this->member_group_lock(_pmember))
	{

		uintptr_t result = 0;

		SCOPED_LOCK(this->m_lockable_group)
		{

			RETURN_IF(_pmember->_is_member_of(this) == false);

			CGASSERT_ERROR(_pmember->_get_group() == this);

			typename member_snapshot_holder<member_t>::write_scope scope_write(this->m_member_snapshot);

			result = this->_process_member_leaving(_pmember, _param);

			this->_detach_member(_pmember);
		}

		try
		{
			this->_process_member_leaved(_pmember, result);
		}

		catch(std::exception& _e)
		{

			LOG_FATAL << "(excp) exception occurred in 'on_meber_leaved' function on group"sv;

			throwable::execute_exception_handler(this, _e, __FUNCTION__);
		}

		catch (...)
		{

			LOG_FATAL << "(excp) exception occurred in 'on_meber_leaved' function on group"sv;

			throwable::execute_exception_handler(this, __FUNCTION__);
		}
	}
}

template <class _TMEMBER, class _TKEY, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
result_code group::Nmap<_TMEMBER, _TKEY, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::_process_member_entering(member_t* _pmember, param_t& _param)
{

	return	this->on_member_entering(_pmember, _param);
}

template <class _TMEMBER, class _TKEY, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
void group::Nmap<_TMEMBER, _TKEY, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::_process_member_entered(member_t* _pmember, param_t& _param)
{

	this->on_member_entered(_pmember, _param);
}

template <class _TMEMBER, class _TKEY, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
uintptr_t group::Nmap<_TMEMBER, _TKEY, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::_process_member_leaving(member_t* _pmember, uintptr_t _param) noexcept
{

	uintptr_t result = 0;

	try
	{

		result = this->on_member_leaving(_pmember, _param);
	}
	catch(...)
	{
		result = static_cast<uintptr_t>(eRESULT::FAIL);
	}

	return result;
}

template <class _TMEMBER, class _TKEY, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
void group::Nmap<_TMEMBER, _TKEY, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::_process_member_leaved(member_t* _pmember, uintptr_t _result) noexcept
{
	try
	{

		this->on_member_leaved(_pmember, _result);
	}
	catch (...)
	{
	}
}

}
