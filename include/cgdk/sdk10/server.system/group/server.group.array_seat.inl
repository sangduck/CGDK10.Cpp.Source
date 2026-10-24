namespace CGDK
{

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
group::array_seat<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::array_seat(size_t _max_member) :
	m_enable_enter(false),
	m_count_member(0)
{
	this->process_set_member_capacity(_max_member);
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
group::array_seat<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::~array_seat() noexcept
{
	this->leave_all();
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
void group::array_seat<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::process_set_member_capacity(size_t _max_count)
{

	std::lock_guard lock(this->m_lockable_group);

	if (this->m_count_member != 0)
		throw throwable::failure(eRESULT::INVALID_STATE);

	typename member_snapshot_holder<member_t>::write_scope scope_write(this->m_member_snapshot);

	this->m_count_max_member = _max_count;

	this->m_seat.create(_max_count);

	this->m_container_member.assign(_max_count, member_set_t(this));

	for (size_t i = 0; i < _max_count; ++i)
	{
		this->m_container_member.at(i).member_serial(i);
	}
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
typename group::array_seat<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::member_data_t& group::array_seat<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::member_data(const member_t* _pmember)
{

	return ((member_set_t*)_pmember->member_set())->m_member_data;
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
typename group::array_seat<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::member_snapshot_t group::array_seat<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::get_member_snapshot() const
{
	return this->m_member_snapshot.get(this->m_lockable_group, [this]() { return this->_build_member_snapshot(); });
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
bool group::array_seat<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::for_each_member_snapshot(const std::function<bool (member_t*)>& _function) const
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
group::member_snapshot<typename group::array_seat<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::member_t>* group::array_seat<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::_build_member_snapshot() const
{
	auto psnapshot = member_snapshot<member_t>::create(this->m_count_member);

	for (const auto& iter : this->m_container_member)
	{
		if (iter.empty())
			continue;

		psnapshot->push_back((*iter).get());
	}

	return psnapshot;
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
object_ptr<_TMEMBERBASE> group::array_seat<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::find_member(const std::function<bool (_TMEMBERBASE*)>& _pred)
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
bool group::array_seat<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::for_each_member(const std::function<bool (_TMEMBERBASE*)>& _function)
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
bool group::array_seat<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::_attach_member(member_t* _pmember)
{

	auto seat_index = this->m_seat.alloc_seat();

	CGASSERT_ERROR(seat_index >= 0);

	RETURN_IF(seat_index < 0, false);

	auto& rSeat = this->m_container_member.at(static_cast<size_t>(seat_index));

	++this->m_count_member;

	rSeat.member(_pmember);

	_pmember->member_set(&rSeat);

	this->member_group_as((typename _TMEMBER::group_base_t*)this, _pmember);

	return true;
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
void group::array_seat<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::_detach_member(member_base_t* _pmember) noexcept
{

	auto seat_index = this->member_serial(_pmember);

	CGASSERT_ERROR(seat_index >= 0);

	RETURN_IF(seat_index < 0);

	auto& rSeat = this->m_container_member.at(static_cast<size_t>(seat_index));

	CGASSERT_ERROR(rSeat == _pmember);

	_pmember->reset_member_set();

	this->Igroup<_TMEMBERBASE>::reset_member_group(_pmember);

	rSeat.reset_member();

	this->m_seat.free_seat(seat_index);

	--this->m_count_member;
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
bool group::array_seat<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::process_is_enable_member_enter() const noexcept
{

	std::lock_guard lock(this->m_lockable_enter);

	return this->m_enable_enter;
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
bool group::array_seat<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::process_enable_member_enter(bool _enable)
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

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
result_code group::array_seat<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::process_enter(member_base_t* _pmember, param_t& _param)
{

	RETURN_IF(_pmember == nullptr, eRESULT::INVALID_ARGUMENT);

	SCOPED_LOCK(this->member_group_lock(_pmember))
	SCOPED_LOCK(this->m_lockable_enter)
	{

		RETURN_IF(_pmember->_is_member_of(this) == true, eRESULT::ALREADY_EXIST);

		RETURN_IF(this->m_enable_enter == false, eRESULT::FAIL_DISABLED);

		RETURN_IF(this->is_member_full(), eRESULT::MEMBER_FULL);

		auto result = this->_process_member_entering(_pmember, _param);

		RETURN_IF(result != eRESULT::SUCCESS, result);

		_pmember->_leave_group();

		SCOPED_LOCK(this->m_lockable_group)
		{
			typename member_snapshot_holder<member_t>::write_scope scope_write(this->m_member_snapshot);

			RETURN_IF(this->_attach_member(_pmember) == false, eRESULT::MEMBER_FULL);

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

			catch(...)
			{

				LOG_FATAL << "(excp) exception complete[on_member_entered]"sv;

				auto is_executed = throwable::execute_exception_handler(this, __FUNCTION__);

				this->_detach_member(_pmember);

				RETURN_IF(is_executed, eRESULT::EXCEPTION);

				throw;
			}
		}
	}

	return eRESULT::SUCCESS;
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
void group::array_seat<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::leave_all() noexcept
{
	std::vector<object_ptr<member_t>> vector_member;

	for(;;)
	{
		SCOPED_LOCK(this->m_lockable_group)
		{
			vector_member.reserve(this->m_count_member);

			for(const auto& iter : this->m_container_member)
			{
				CONTINUE_IF(iter.empty());

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
void group::array_seat<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::process_leave(member_base_t* _pmember, uintptr_t _param) noexcept
{

	CGASSERT_FATAL(_pmember != nullptr);

	SCOPED_LOCK(this->member_group_lock(_pmember))
	{

		uintptr_t result = 0;

		SCOPED_LOCK(this->m_lockable_group)
		{

			RETURN_IF(_pmember->_is_member_of(this) == false);

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
object_ptr<typename group::array_seat<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::member_t> group::array_seat<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::leave_by_seat(group_member_id_t _seat, uintptr_t _param) noexcept
{

	RETURN_IF(_seat<0 || _seat >= static_cast<group_member_id_t>(this->member_capacity()), object_ptr<member_t>());

	object_ptr<member_t> pmember;

	pmember = this->member(_seat);

	RETURN_IF(pmember.empty(), object_ptr<member_t>());

	SCOPED_LOCK(this->member_group_lock(pmember))
	{

		uintptr_t result = 0;

		SCOPED_LOCK(this->m_lockable_group)
		{

			RETURN_IF(_seat >= static_cast<group_member_id_t>(this->m_container_member.size()) || pmember != this->m_container_member.at(_seat).member(), object_ptr<member_t>());

			typename member_snapshot_holder<member_t>::write_scope scope_write(this->m_member_snapshot);

			try
			{
				result = this->on_member_leaving(pmember, _param);
			}

			catch(std::exception& _e)
			{

				LOG_FATAL << "(excp) exception occurred in 'on_meber_leaving' function on group"sv;

				throwable::execute_exception_handler(this, _e, __FUNCTION__);
			}

			catch (...)
			{

				LOG_FATAL << "(excp) exception occurred in 'on_meber_leaving' function on group"sv;

				throwable::execute_exception_handler(this, __FUNCTION__);
			}

			this->_detach_member(pmember);

		}

		try
		{
			this->_process_member_leaved(pmember, result);
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

	return	pmember;
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
object_ptr<typename group::array_seat<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::member_t> group::array_seat<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::member(group_member_id_t _seat) noexcept
{

	std::lock_guard lock(this->m_lockable_group);

	RETURN_IF(_seat < 0 || _seat>=static_cast<group_member_id_t>(this->m_container_member.size()), object_ptr<member_t>());

	return this->m_container_member.at(static_cast<size_t>(_seat)).member();
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
result_code group::array_seat<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::_process_member_entering(member_t* _pmember, param_t& _param)
{

	return this->on_member_entering(_pmember, _param);
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
void group::array_seat<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::_process_member_entered(member_t* _pmember, param_t& _param)
{

	this->on_member_entered(_pmember, _param);
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
uintptr_t group::array_seat<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::_process_member_leaving(member_t* _pmember, uintptr_t _param) noexcept
{

	uintptr_t result = 0;

	try
	{

		result = this->on_member_leaving(_pmember, _param);
	}
	catch (throwable::failure& _e)
	{
		result = _e.reason<uintptr_t>();
	}
	catch(...)
	{
		result = static_cast<uintptr_t>(eRESULT::FAIL);
	}

	return	result;
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
void group::array_seat<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::_process_member_leaved(member_t* _pmember, uintptr_t _result) noexcept
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
