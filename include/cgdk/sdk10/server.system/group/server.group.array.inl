namespace CGDK
{

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
group::array<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::array(size_t _max_member) :
	m_enable_enter(false),
	m_count_member(0)
{
	this->process_set_member_capacity(_max_member);
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
group::array<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::~array() noexcept
{
	this->leave_all();
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
void group::array<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::process_set_member_capacity(size_t _max_count)
{

	std::lock_guard lock(this->m_lockable_group);

	if (this->m_count_member != 0)
		throw throwable::failure(eRESULT::INVALID_STATE);

	typename member_snapshot_holder<member_t>::write_scope scope_write(this->m_member_snapshot);

	this->m_count_max_member = _max_count;

	this->m_container_member.assign(_max_count, member_set_t(this));

	for(size_t i=0; i<_max_count; ++i)
	{
		this->m_container_member.at(i).member_serial(i);
	}
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
object_ptr<_TMEMBERBASE> group::array<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::find_member(const std::function<bool (_TMEMBERBASE*)>& _pred)
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
bool group::array<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::for_each_member(const std::function<bool (_TMEMBERBASE*)>& _function)
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
typename group::array<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::member_data_t& group::array<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::member_data(const member_t* _pmember)
{

	return ((member_set_t*)_pmember->member_set())->m_member_data;
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
typename group::array<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::member_snapshot_t group::array<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::get_member_snapshot() const
{
	return this->m_member_snapshot.get(this->m_lockable_group, [this]() { return this->_build_member_snapshot(); });
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
bool group::array<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::for_each_member_snapshot(const std::function<bool (member_t*)>& _function) const
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
group::member_snapshot<typename group::array<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::member_t>* group::array<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::_build_member_snapshot() const
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
result_code group::array<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::process_enter(member_t* , param_t& )
{
	throw throwable::failure(eRESULT::ACCESS_DENIED);
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
bool group::array<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::process_is_enable_member_enter() const noexcept
{

	std::lock_guard lock(this->m_lockable_enter);

	return m_enable_enter;
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
bool group::array<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::process_enable_member_enter(bool _enable)
{
	SCOPED_LOCK(m_lockable_enter)
	{

		RETURN_IF(m_enable_enter == _enable, false);

		m_enable_enter = _enable;

		if(_enable)
		{
			on_enable_enter();
		}
		else
		{
			on_disable_enter();
		}
	}

	return true;
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
object_ptr<typename group::array<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::member_t> group::array<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::member(member_t* _pmember, group_member_id_t _seat, param_t& _param)
{

	RETURN_IF(_seat<0 || _seat >= static_cast<group_member_id_t>(this->member_capacity()), object_ptr<member_t>());

	if(_pmember == nullptr)
	{
		return this->leave_by_seat(_seat, 0);
	}

	object_ptr<member_t> pmember_old;

	std::unique_lock lock_member(this->member_group_lock(_pmember));
	decltype(lock_member) lock_member_seat;
	std::unique_lock lock_enter(this->m_lockable_enter, std::defer_lock);

	for(;;)
	{

		auto pmember_seat = this->member(_seat);

		if(pmember_seat.exist() && pmember_seat != _pmember)
		{

			lock_member_seat = decltype(lock_member)(this->member_group_lock(pmember_seat.get()), std::defer_lock);

			if(lock_member_seat.try_lock() == false)
			{
				lock_member.unlock();

				std::lock(lock_member_seat, lock_member);
			}
		}

		lock_enter.lock();

		BREAK_IF(this->member(_seat) == pmember_seat);

		lock_enter.unlock();

		lock_member_seat = decltype(lock_member)();
	}

	{

		RETURN_IF(_pmember->_is_member_of(this) == true && (member_serial(_pmember) == _seat), object_ptr<member_t>());

		RETURN_IF(m_enable_enter == false, object_ptr<member_t>());

		RETURN_IF(this->is_member_full(), object_ptr<member_t>());

		auto result = this->_process_member_entering(_pmember, _param);

		RETURN_IF(result != eRESULT::SUCCESS, object_ptr<member_t>());

		pmember_old = this->leave_by_seat(_seat);

		if(lock_member_seat.owns_lock())
		{
			lock_member_seat.unlock();
		}

		if(_pmember != nullptr)
		{
			_pmember->_leave_group();
		}

		SCOPED_LOCK(this->m_lockable_group)
		{
			RETURN_IF(_seat >= static_cast<group_member_id_t>(this->m_container_member.size()), pmember_old);

			typename member_snapshot_holder<member_t>::write_scope scope_write(this->m_member_snapshot);

			auto& rSeat = this->m_container_member.at(_seat);

			rSeat = _pmember;

			RETURN_IF(_pmember == nullptr, pmember_old)

			++this->m_count_member;

			_pmember->member_set(&rSeat);

			this->member_group_as((typename _TMEMBER::group_base_t*)this, _pmember);

			try
			{
				this->_process_member_entered(_pmember, _param);
			}

			catch(std::exception& _e)
			{

				LOG_FATAL << "(excp) group: exception complete[on_member_entered]"sv;

				bool is_executed = throwable::execute_exception_handler(this, _e, __FUNCTION__);

				rSeat.reset_member();

				--this->m_count_member;

				_pmember->reset_member_set();

				Igroup<_TMEMBERBASE>::reset_member_group(_pmember);

				RETURN_IF(is_executed, object_ptr<member_t>());

				throw;
			}

			catch (...)
			{

				LOG_FATAL << "(excp) exception complete[on_member_entered]"sv;

				bool is_executed = throwable::execute_exception_handler(this, __FUNCTION__);

				rSeat.reset_member();

				--this->m_count_member;

				_pmember->reset_member_set();

				Igroup<_TMEMBERBASE>::reset_member_group(_pmember);

				RETURN_IF(is_executed, object_ptr<member_t>());

				throw;
			}
		}
	}

	return pmember_old;
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
void group::array<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::leave_all(uintptr_t _param) noexcept
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
			this->Igroup<_TMEMBERBASE>::leave(iter.get(), _param);
		}

		vector_member.clear();
	}
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
void group::array<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::process_leave(member_base_t* _pmember, uintptr_t _param) noexcept
{

	CGASSERT_ERROR(_pmember != nullptr)

	object_ptr<member_t> pobject = this->leave_by_seat(member_serial(_pmember), _param);

	CGASSERT_ERROR(pobject == _pmember);
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
object_ptr<typename group::array<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::member_t> group::array<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::leave_by_seat(group_member_id_t _seat, uintptr_t _param) noexcept
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

			result = this->_process_member_leaving(pmember, _param);

			auto& rSeat = this->m_container_member.at(_seat);

			rSeat.reset_member();

			pmember->reset_member_set();

			Igroup<_TMEMBERBASE>::reset_member_group(pmember);

			--this->m_count_member;
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

	return pmember;
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
object_ptr<typename group::array<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::member_t> group::array<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::member(group_member_id_t _seat)
{
	SCOPED_LOCK(this->m_lockable_group)
	{

		RETURN_IF(_seat<0 || _seat>=static_cast<group_member_id_t>(this->m_container_member.size()), object_ptr<member_t>());

		return this->m_container_member.at(_seat).member();
	}
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
result_code group::array<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::_process_member_entering(member_t* _pmember, param_t& _param)
{

	return this->on_member_entering(_pmember, _param);
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
void group::array<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::_process_member_entered(member_t* _pmember, param_t& _param)
{

	this->on_member_entered(_pmember, _param);
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
uintptr_t group::array<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::_process_member_leaving(member_t* _pmember, uintptr_t _param) noexcept
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

	return result;
}

template <class _TMEMBER, class _TPARAM, class _TMEMBER_DATA, class _TMEMBERBASE>
void group::array<_TMEMBER, _TPARAM, _TMEMBER_DATA, _TMEMBERBASE>::_process_member_leaved(member_t* _pmember, uintptr_t _result) noexcept
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
