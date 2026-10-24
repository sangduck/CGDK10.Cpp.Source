namespace CGDK
{

template <class _TMEMBER, class _TPARAM, class _TMEMBERBASE>
group::count<_TMEMBER, _TPARAM, _TMEMBERBASE>::count(size_t _max_member) :
	m_enable_enter(false),
	m_count_member(0)
{
	this->process_set_member_capacity(_max_member);
}

template <class _TMEMBER, class _TPARAM, class _TMEMBERBASE>
group::count<_TMEMBER, _TPARAM, _TMEMBERBASE>::~count() noexcept
{
}

template <class _TMEMBER, class _TPARAM, class _TMEMBERBASE>
bool group::count<_TMEMBER, _TPARAM, _TMEMBERBASE>::process_is_enable_member_enter() const noexcept
{

	std::lock_guard lock(this->m_lockable_enter);

	return this->m_enable_enter;
}

template <class _TMEMBER, class _TPARAM, class _TMEMBERBASE>
bool group::count<_TMEMBER, _TPARAM, _TMEMBERBASE>::process_enable_member_enter(bool _enable)
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

template <class _TMEMBER, class _TPARAM, class _TMEMBERBASE>
result_code group::count<_TMEMBER, _TPARAM, _TMEMBERBASE>::process_enter(member_base_t* _pmember, param_t& _param)
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

		_pmember->add_ref();

		_pmember->_leave_group();

		SCOPED_LOCK(this->m_lockable_group)
		{

			++this->m_count_member;

			this->member_group_as((typename _TMEMBER::group_base_t*)this, _pmember);

			try
			{
				this->_process_member_entered(_pmember, _param);
			}

			catch(std::exception& _e)
			{

				LOG_FATAL << "(excp) exception complete[on_member_entered]"sv;

				auto is_executed = throwable::execute_exception_handler(this, _e, __FUNCTION__);

				--this->m_count_member;

				this->Igroup<_TMEMBERBASE>::reset_member_group(_pmember);

				_pmember->release();

				RETURN_IF(is_executed, eRESULT::EXCEPTION);

				throw;
			}

			catch (...)
			{

				LOG_FATAL << "(excp) exception complete[on_member_entered]"sv;

				auto is_executed = throwable::execute_exception_handler(this, __FUNCTION__);

				--this->m_count_member;

				this->Igroup<_TMEMBERBASE>::reset_member_group(_pmember);

				_pmember->release();

				RETURN_IF(is_executed, eRESULT::EXCEPTION);

				throw;
			}
		}
	}

	return eRESULT::SUCCESS;
}

template <class _TMEMBER, class _TPARAM, class _TMEMBERBASE>
void group::count<_TMEMBER, _TPARAM, _TMEMBERBASE>::process_leave(member_base_t* _pmember, uintptr_t _param) noexcept
{

	CGASSERT_FATAL(_pmember != nullptr)

	SCOPED_LOCK(this->member_group_lock(_pmember))
	{

		uintptr_t result = 0;

		SCOPED_LOCK(this->m_lockable_group)
		{

			RETURN_IF(_pmember->_is_member_of(this) == false);

			result = this->_process_member_leaving(_pmember, _param);

			Igroup<_TMEMBERBASE>::reset_member_group(_pmember);

			--this->m_count_member;
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

	_pmember->release();
}

template <class _TMEMBER, class _TPARAM, class _TMEMBERBASE>
result_code group::count<_TMEMBER, _TPARAM, _TMEMBERBASE>::_process_member_entering(member_t* _pmember, param_t& _param)
{

	return	this->on_member_entering(_pmember, _param);
}

template <class _TMEMBER, class _TPARAM, class _TMEMBERBASE>
void group::count<_TMEMBER, _TPARAM, _TMEMBERBASE>::_process_member_entered(member_t* _pmember, param_t& _param)
{

	this->on_member_entered(_pmember, _param);
}

template <class _TMEMBER, class _TPARAM, class _TMEMBERBASE>
uintptr_t group::count<_TMEMBER, _TPARAM, _TMEMBERBASE>::_process_member_leaving(member_t* _pmember, uintptr_t _param) noexcept
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

	return	result;
}

template <class _TMEMBER, class _TPARAM, class _TMEMBERBASE>
void group::count<_TMEMBER, _TPARAM, _TMEMBERBASE>::_process_member_leaved(member_t* _pmember, uintptr_t _result) noexcept
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
