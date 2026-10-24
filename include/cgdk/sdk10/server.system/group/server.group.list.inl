namespace CGDK
{

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
group::list<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::list() noexcept :
	m_enable_enter(false)
{
	this->process_set_member_capacity(SIZE_MAX);
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
group::list<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::list(size_t _max_member) noexcept :
	m_enable_enter(false)
{
	this->process_set_member_capacity(_max_member);
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
group::list<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::~list() noexcept
{
	this->leave_all();
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
void group::list<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::process_set_member_capacity(size_t _max_count)
{

	std::lock_guard lock(this->m_lockable_group);

	this->m_count_max_member = _max_count;

#if defined(_MSC_VER)
	__if_exists(container_t::reserve)
	{
		this->m_container_member.reserve((_max_count>256) ? 256:_max_count);
	}
#endif
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
typename group::list<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::member_data_t& group::list<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::member_data(const member_t* _pmember)
{
	return ((member_set_t*)_pmember->member_set())->m_member_data;
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
typename group::list<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::member_snapshot_t group::list<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::get_member_snapshot() const
{
	return this->m_member_snapshot.get(this->m_lockable_group, [this]() { return this->_build_member_snapshot(); });
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
bool group::list<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::for_each_member_snapshot(const std::function<bool (member_t*)>& _function) const
{
	auto snapshot = this->get_member_snapshot();

	for (auto* iter : snapshot)
	{
		if (_function(iter) == false)
			return false;
	}

	return true;
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
group::member_snapshot<typename group::list<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::member_t>* group::list<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::_build_member_snapshot() const
{
	auto psnapshot = member_snapshot<member_t>::create(this->m_container_member.size());

	for (const auto& iter : this->m_container_member)
	{
		psnapshot->push_back((*iter).get());
	}

	return psnapshot;
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
object_ptr<_TMEMBERBASE> group::list<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::find_member(const std::function<bool (_TMEMBERBASE*)>& _pred)
{
	auto snapshot = this->get_member_snapshot();

	for (auto* iter : snapshot)
	{
		if (_pred(iter) == true)
			return object_ptr<_TMEMBERBASE>(iter);
	}

	return object_ptr<_TMEMBERBASE>();
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
bool group::list<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::for_each_member(const std::function<bool (_TMEMBERBASE*)>& _function)
{
	auto snapshot = this->get_member_snapshot();

	for (auto* iter : snapshot)
	{
		if (_function(iter) == false)
			return false;
	}

	return true;
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
void group::list<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::_attach_member_to_list_head(member_t* _pmember)
{
	this->push_front(member_set_t(this));

	auto& rSeat = this->m_container_member.front();

	rSeat.member(_pmember);
	rSeat.iter = this->m_container_member.begin();

	_pmember->member_set(&rSeat);

	this->member_group_as((typename _TMEMBER::group_base_t*)this, _pmember);
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
void group::list<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::_attach_member_to_list_tail(member_t* _pmember)
{
#if defined(_MSC_VER)
	__if_exists(container_t::attach)
#endif
	{
		this->m_container_member.push_back(member_set_t(this));

		auto& rSeat = this->m_container_member.back();

		rSeat.member(_pmember);
		rSeat.iter = --this->m_container_member.end();

		_pmember->member_set(&rSeat);
	}

	this->member_group_as((typename _TMEMBER::group_base_t*)this, _pmember);
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
void group::list<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::_detach_member(member_base_t* _pmember) noexcept
{
#if defined(_MSC_VER)
	__if_exists(container_t::detach)
#endif
	{
		auto* pmember_set = (member_set_t*)_pmember->member_set();

		this->m_container_member.erase(pmember_set->iter);

		_pmember->reset_member_set();
	}

	Igroup<_TMEMBERBASE>::reset_member_group(_pmember);
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
bool group::list<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::process_is_enable_member_enter() const noexcept
{

	std::lock_guard lock(this->m_lockable_enter);

	return this->m_enable_enter;
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
bool group::list<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::process_enable_member_enter(bool _enable)
{
	SCOPED_LOCK(this->m_lockable_enter)
	{
		if(this->m_enable_enter == _enable)
			return false;

		this->m_enable_enter = _enable;

		if(_enable)
			this->on_enable_enter();
		else
			this->on_disable_enter();
	}

	return true;
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
result_code group::list<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::process_enter(member_base_t* _pmember, param_t& _param)
{
	if(_pmember == nullptr)
		return eRESULT::INVALID_ARGUMENT;

	SCOPED_LOCK(this->member_group_lock(_pmember))
	SCOPED_LOCK(this->m_lockable_enter)
	{
		if(_pmember->_is_member_of(this) == true)
			return eRESULT::ALREADY_EXIST;

		CGASSERT_ERROR(_pmember->_get_group() != this);

		if(this->m_enable_enter == false)
			return eRESULT::FAIL_DISABLED;

		if(this->is_member_full())
			return eRESULT::MEMBER_FULL;

		auto result = this->_process_member_entering(_pmember, _param);

		if(result != eRESULT::SUCCESS)
			return result;

		_pmember->_leave_group();

		SCOPED_LOCK(this->m_lockable_group)
		{
			typename member_snapshot_holder<member_t>::write_scope scope_write(this->m_member_snapshot);

			this->_attach_member(_pmember);

			try
			{
				this->_process_member_entered(_pmember, _param);
			}

			catch (std::exception& _e)
			{

				LOG_FATAL << "(excp) exception complete[on_member_entered]"sv;

				auto is_executed = throwable::execute_exception_handler(this, _e, __FUNCTION__);

				this->_detach_member(_pmember);

				if(is_executed)
					return eRESULT::EXCEPTION;

				throw;
			}

			catch (...)
			{

				LOG_FATAL << "(excp) exception complete[on_member_entered]"sv;

				auto is_executed = throwable::execute_exception_handler(this, __FUNCTION__);

				this->_detach_member(_pmember);

				if(is_executed)
					return eRESULT::EXCEPTION;

				throw;
			}
		}

		CGASSERT_ERROR(_pmember->_get_group() == this);
	}

	return eRESULT::SUCCESS;
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
void group::list<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::leave_all() noexcept
{
	std::vector<object_ptr<member_t>> vector_member;

	for(;;)
	{
		SCOPED_LOCK(this->m_lockable_group)
		{
			vector_member.reserve(this->m_container_member.size());

			for(const auto& iter : this->m_container_member)
			{
				vector_member.push_back(*iter);
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

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
void group::list<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::process_leave(member_base_t* _pmember, uintptr_t _param) noexcept
{

	CGASSERT_FATAL(_pmember != nullptr);

	SCOPED_LOCK(this->member_group_lock(_pmember))
	{
		uintptr_t result = 0;

		SCOPED_LOCK(this->m_lockable_group)
		{
			if(_pmember->_is_member_of(this) == false)
				return;

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

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
result_code group::list<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::_process_member_entering(member_t* _pmember, param_t& _param)
{
	return	this->on_member_entering(_pmember, _param);
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
void group::list<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::_process_member_entered(member_t* _pmember, param_t& _param)
{

	this->on_member_entered(_pmember, _param);
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
uintptr_t group::list<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::_process_member_leaving(member_t* _pmember, uintptr_t _param) noexcept
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

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
void group::list<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::_process_member_leaved(member_t* _pmember, uintptr_t _result) noexcept
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
