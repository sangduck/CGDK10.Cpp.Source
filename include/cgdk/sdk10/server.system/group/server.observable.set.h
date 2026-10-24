 //*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                         Socket template Classes                           *
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

//-----------------------------------------------------------------------------
/**

 @class		set
 @brief		
 @todo		
 @ref		
 @details	\n
 일반적인 선형 Container를 가진 Observer 관리자.
 Array/Linked-List Container로 Observer를 관리한다.

*///-----------------------------------------------------------------------------
template <class _TOBSERVER, class _TOBSERVER_DATA=int64_t, class _observer_base_t=_TOBSERVER>
class observable::set :
	virtual public				Iobservable<_observer_base_t>
{
public:
	using observer_t  = _TOBSERVER;
	using observer_channel_t = typename _observer_base_t::observer_channel_t;
	using observer_base_t = _observer_base_t;
	using observer_data_t = _TOBSERVER_DATA;
	using observer_set_t = OBSERVER_SET_DATA<observer_t, observer_data_t>;
	// - orders the observers by (address, ocid). (unsubscribe / update_observer_data find the observer without making an object_ptr of it)
	struct observer_key
	{
		const observer_t*	pobserver;
		observer_channel_t	ocid;
	};
	struct observer_less
	{
		using is_transparent = void;
		[[nodiscard]] static bool _less(const observer_t* _lhs, observer_channel_t _lhs_ocid, const observer_t* _rhs, observer_channel_t _rhs_ocid) noexcept { std::less<const observer_t*> less; return less(_lhs, _rhs) ? true : less(_rhs, _lhs) ? false : (_lhs_ocid < _rhs_ocid); }
		[[nodiscard]] bool		operator()(const observer_set_t& _lhs, const observer_set_t& _rhs) const noexcept { return _less(_lhs.pobserver.get(), _lhs.ocid, _rhs.pobserver.get(), _rhs.ocid); }
		[[nodiscard]] bool		operator()(const observer_set_t& _lhs, const observer_key& _rhs) const noexcept { return _less(_lhs.pobserver.get(), _lhs.ocid, _rhs.pobserver, _rhs.ocid); }
		[[nodiscard]] bool		operator()(const observer_key& _lhs, const observer_set_t& _rhs) const noexcept { return _less(_lhs.pobserver, _lhs.ocid, _rhs.pobserver.get(), _rhs.ocid); }
	};
	using container_t = std::set<observer_set_t, observer_less>;
	using observer_snapshot_t = group::member_snapshot_ptr<observer_t>;
	using iterator_t = typename container_t::iterator;
	using const_iterator_t = typename container_t::const_iterator;

public:
			set(size_t _max_observer = SIZE_MAX) noexcept;
	virtual	~set() noexcept;

public:
			result_code			subscribe(observer_t* _pobserver) { observer_data_t temp_observer_data{}; return subscribe(_pobserver, 0, temp_observer_data); }
			result_code			subscribe(observer_t* _pobserver, observer_channel_t _ocid)	{ observer_data_t temp_observer_data{}; return subscribe(_pobserver, _ocid, temp_observer_data);}
	virtual	result_code			subscribe(observer_t* _pobserver, observer_channel_t _ocid, const observer_data_t& _observer_data);
	virtual	bool				unsubscribe(observer_t* _pobserver, observer_channel_t _ocid = 0) noexcept override;
	virtual	void				unsubscribe_all() noexcept override;

	static	observer_data_t&	get_observer_data(observer_t* _pobserver) { return *static_cast<observer_data_t*>(Iobservable<_observer_base_t>::_get_observer_data(_pobserver)); }
			bool				update_observer_data(observer_t* _pobserver, observer_data_t& _observer_data, observer_channel_t _ocid = 0);

	[[nodiscard]] size_t		count() const noexcept { std::lock_guard lock(this->m_lockable_observable); return _get_observer_count(); }
	[[nodiscard]] size_t		capacity() const noexcept { std::lock_guard lock(this->m_lockable_observable); return _get_observer_capacity(); }
			void				capacity(size_t _max_count);
	[[nodiscard]] size_t		room_count() const noexcept { std::lock_guard lock(this->m_lockable_observable);return _get_observer_room_count(); }

	[[nodiscard]] bool			empty() const noexcept { std::lock_guard lock(this->m_lockable_observable); return _is_observer_empty(); }
	[[nodiscard]] bool			exist() const noexcept { std::lock_guard lock(this->m_lockable_observable); return _is_observer_exist(); }
	[[nodiscard]] bool			is_full() const noexcept { std::lock_guard lock(this->m_lockable_observable); return _is_observer_full(); }
	[[nodiscard]] bool			is_room() const noexcept { std::lock_guard lock(this->m_lockable_observable); return _is_observer_room(); }

	[[nodiscard]] iterator_t	begin() noexcept { return m_container_observer.begin();}
	[[nodiscard]] const_iterator_t begin() const noexcept { return m_container_observer.begin();}
	[[nodiscard]] iterator_t	end() noexcept { return m_container_observer.end();}
	[[nodiscard]] const_iterator_t end() const noexcept { return m_container_observer.end();}

	//! @brief returns the first observer _pred returned true for. _pred runs out of the lock over the observer snapshot (see for_each)
	[[nodiscard]] object_ptr<observer_t> find_observer(const std::function<bool (observer_t*)>& _pred) const noexcept;

	//! @brief calls _function for the observers of the snapshot out of the lock (same as for_each_snapshot) @return false _function returned false
	// - an observer can be unsubscribed before or while _function runs.
			bool				for_each(const std::function<bool (observer_t*)>& _function);

	//! @brief observers as an immutable array (copy-on-write). no lock while the published one is valid. in on_observer_registered / on_observer_unregistering / on_observer_unregistered : the live observers
	[[nodiscard]] observer_snapshot_t get_observer_snapshot() const;
	//! @brief calls _function for the observers of the snapshot out of the lock. (observers may be registered / unregistered in _function) @return false _function returned false
			bool				for_each_snapshot(const std::function<bool (observer_t*)>& _function) const;

protected:
	// - lock of the events (lock order : observer (m_lockable_vector_observable) -> m_lockable_observable_enter -> m_lockable_observable)
	//   on_observer_registering : in the lock of the observer and m_lockable_observable_enter
	//   on_observer_registered / on_observer_unregistering / on_observer_unregistered / on_observer_data_updating : in the lock of the observer and m_lockable_observable
	//   do not lock another observer (or wait for a thread which locks one) in them (deadlock). the observer is held (object_ptr) while they run.
	virtual	result_code			on_observer_registering(observer_t* /*_pobserver*/, observer_channel_t /*_ocid*/, observer_data_t& /*_observer_data*/){ return eRESULT::SUCCESS;}
	virtual	void				on_observer_registered(observer_t* /*_pobserver*/, observer_channel_t /*_ocid*/, const observer_data_t& /*_observer_data*/) {}
	virtual	uintptr_t			on_observer_unregistering(observer_t* /*_pobserver*/, observer_channel_t /*_ocid*/) { return 0;}
	virtual	void				on_observer_unregistered(observer_t* /*_pobserver*/, observer_channel_t /*_ocid*/, uintptr_t /*_result*/){}
	virtual	uintptr_t			on_observer_data_updating(observer_t* /*_pobserver*/, observer_channel_t /*_ocid*/, observer_data_t& /*_observer_data*/) { return 0; }
	virtual	void				on_observer_data_update(observer_t* /*_pobserver*/, observer_channel_t /*_ocid*/, uintptr_t /*_result*/) {}

private:
	[[nodiscard]] size_t		_get_observer_count() const noexcept { return m_container_observer.size();}
	[[nodiscard]] size_t		_get_observer_room_count() const noexcept {	return m_count_max_observer-m_container_observer.size();}
	[[nodiscard]] size_t		_get_observer_capacity() const noexcept { return m_count_max_observer;}
			
	[[nodiscard]] bool			_is_observer_full() const noexcept { return m_container_observer.size()>=m_count_max_observer;}
	[[nodiscard]] bool			_is_observer_empty() const noexcept { return m_container_observer.empty();}
	[[nodiscard]] bool			_is_observer_room() const noexcept { return m_container_observer.size()<m_count_max_observer;}
	[[nodiscard]] bool			_is_observer_exist() const noexcept { return !m_container_observer.empty();}
	[[nodiscard]] group::member_snapshot<observer_t>* _build_observer_snapshot() const;

protected:
			container_t			m_container_observer;
			size_t				m_count_max_observer;

	// - published observer snapshot (changed in m_lockable_observable, read without it)
	mutable	group::member_snapshot_holder<observer_t> m_observer_snapshot;
};

template <class _TOBSERVER, class _TOBSERVER_DATA, class _observer_base_t>
observable::set<_TOBSERVER, _TOBSERVER_DATA, _observer_base_t>::set(size_t _max_observer) noexcept :
	m_count_max_observer(_max_observer)
{
}

template <class _TOBSERVER, class _TOBSERVER_DATA, class _observer_base_t>
observable::set<_TOBSERVER, _TOBSERVER_DATA, _observer_base_t>::~set() noexcept
{
}

template <class _TOBSERVER, class _TOBSERVER_DATA, class _observer_base_t>
result_code observable::set<_TOBSERVER, _TOBSERVER_DATA, _observer_base_t>::subscribe(_TOBSERVER* _pobserver, observer_channel_t _ocid, const observer_data_t& _observer_data)
{
	// check) _pobserver가 nullptr이면 안 된다.
	RETURN_IF(_pobserver == nullptr, eRESULT::INVALID_ARGUMENT);
	
	SCOPED_LOCK(_pobserver->m_lockable_vector_observable)
	SCOPED_LOCK(this->m_lockable_observable_enter)
	{
		// check) 꽉 찼는가?
		RETURN_IF(is_full(), eRESULT::MEMBER_FULL);

		// declare) 
		observer_set_t temp_observer_set(_pobserver, _ocid, _observer_data);

		SCOPED_LOCK(this->m_lockable_observable)
		{
			// check) 이미 같은 Observer가 존재하는지 확인한다.
			auto iter_find = m_container_observer.find(temp_observer_set);

			// check) 이미 붙어 있다면 끝낸다.
			RETURN_IF(iter_find != m_container_observer.end(), eRESULT::ALREADY_EXIST);
		}

		// 1) on_observer_registering()함수를 호출한다.
		auto result = this->on_observer_registering(_pobserver, _ocid, temp_observer_set.observer_data);

		// check) 입장 조건에 맞지 않는다!
		RETURN_IF(result != eRESULT::SUCCESS, result);

		// 2) 이전 ocid로 등록된 Observable을 찾는다.
		_pobserver->_unsubscribe(_ocid);

		SCOPED_LOCK(this->m_lockable_observable)
		{
			typename group::member_snapshot_holder<observer_t>::write_scope scope_write(this->m_observer_snapshot);

			// 4) 추가한다.
			auto result_insert = m_container_observer.insert(temp_observer_set);

			// 5) pobserver에 추가한다.
			_pobserver->_add_observable(this, _ocid);

			// 6) observer_data 설정
			this->_set_observer_data(_pobserver, const_cast<observer_data_t*>(&result_insert.first->observer_data));

			// 7) on_observer_registered()함수를 호출한다.
			try
			{
				this->on_observer_registered(_pobserver, _ocid, temp_observer_set.observer_data);
			}
			// exception) rollback한다.
			catch(std::exception& e)
			{
				// log)
				LOG_FATAL << "(excp) exception completion[on_observer_registered]"sv;

				// - execute_exception_handle을 호출한다.
				auto is_executed = throwable::execute_exception_handler(this, e, __FUNCTION__);

				// - 제거
				_pobserver->_popback_observable();

				// - Observer를 제거한다.
				m_container_observer.erase(result_insert.first);

				// 4) observer_data 리셋
				this->_reset_observer_data(_pobserver);

				// check) 
				RETURN_IF(is_executed, eRESULT::EXCEPTION);

				// reraise) 
				throw;
			}
		}
	}

	// return) success
	return eRESULT::SUCCESS;
}

template <class _TOBSERVER, class _TOBSERVER_DATA, class _observer_base_t>
bool observable::set<_TOBSERVER, _TOBSERVER_DATA, _observer_base_t>::unsubscribe(_TOBSERVER* _pobserver, observer_channel_t _ocid) noexcept
{
	// check) _pobserver가 nullptr이여서는 안 된다.
	CGASSERT_ERROR(_pobserver != nullptr);

	// check) _pobserver가 nullptr이여서는 안 된다.
	RETURN_IF(_pobserver == nullptr, false);

	// declare) holds the observer till return (erase releases the reference of the observable)
	object_ptr<observer_t> pobserver_hold;

	SCOPED_LOCK(_pobserver->m_lockable_vector_observable)
	SCOPED_LOCK(this->m_lockable_observable)
	{
		// declare) 
		uintptr_t result = 0;

		// 1) 먼저 이미 등록되어 있는지 찾는다.
		auto iter_find = m_container_observer.find(observer_key{ _pobserver, _ocid });

		// check) 찾지 못했으면 끝낸다.
		RETURN_IF(iter_find == m_container_observer.end(), false);

		typename group::member_snapshot_holder<observer_t>::write_scope scope_write(this->m_observer_snapshot);

		// - holds the observer till on_observer_unregistered() and the return
		pobserver_hold = iter_find->pobserver;

		// 2) on_observer_unregistering()함수를 호출한다.
		try
		{
			result = this->on_observer_unregistering(_pobserver, _ocid);
		}
		// exception) 
		catch(std::exception& /*_e*/)
		{
			LOG_FATAL << "(excp) exception occurred in 'on_observer_unregistering' function on observer"sv;
		}

		// 2) 제거한다.
		m_container_observer.erase(iter_find);

		// 3) 제거한다.
		_pobserver->_erase_observable(_ocid);

		// 4) observer_data 리셋
		this->_reset_observer_data(_pobserver);

		// 5) on_observer_unregistered()함수를 호출한다.
		try
		{
			this->on_observer_unregistered(_pobserver, _ocid, result);
		}
		// exception) 
		catch(std::exception& /*_e*/)
		{
			LOG_FATAL << "(excp) exception occurred in 'on_observer_unregistered' function on observer"sv;
		}
	}

	// return) 
	return true;
}

template <class _TOBSERVER, class _TOBSERVER_DATA, class _observer_base_t>
void observable::set<_TOBSERVER, _TOBSERVER_DATA, _observer_base_t>::unsubscribe_all() noexcept
{
	// declare) 
	container_t	vector_observer;

	// 1) 복사한다.
	SCOPED_LOCK(this->m_lockable_observable)
	{
		vector_observer = m_container_observer;
	}

	// 2) 모두 unregister한다.
	for(auto& iter: vector_observer)
	{
		unsubscribe(iter.pobserver, iter.ocid);
	}
}

template <class _TOBSERVER, class _TOBSERVER_DATA, class _observer_base_t>
bool observable::set<_TOBSERVER, _TOBSERVER_DATA, _observer_base_t>::update_observer_data(observer_t* _pobserver, observer_data_t& _observer_data, observer_channel_t _ocid)
{
	// check) _pobserver가 nullptr이여서는 안 된다.
	CGASSERT_FATAL(_pobserver != nullptr);
	
	// declare) 
	uintptr_t result;
	
	SCOPED_LOCK(_pobserver->m_lockable_vector_observable)
	{
		SCOPED_LOCK(this->m_lockable_observable)
		{
			// 1) 먼저 이미 등록되어 있는지 찾는다.
			auto iter_find = m_container_observer.find(observer_key{ _pobserver, _ocid });

			// check) 찾지 못했으면 끝낸다.
			RETURN_IF(iter_find==m_container_observer.end(), false);

			// 2) on_observer_unregistering()함수를 호출한다.
			result = this->on_observer_data_updating(_pobserver, _ocid, _observer_data);

			// 3) observer_data를 Update한다.
			iter_find->set_observer_data(_observer_data);
		}

		// 3) Update...
		this->on_observer_data_update(_pobserver, _ocid, result);
	}

	// return)
	return true;
}

template <class _TOBSERVER, class _TOBSERVER_DATA, class _observer_base_t>
void observable::set<_TOBSERVER, _TOBSERVER_DATA, _observer_base_t>::capacity(size_t _max_count)
{
	SCOPED_LOCK(this->m_lockable_observable)
	{
		// 1) 최대 Observer수를 설정한다.
		m_count_max_observer = _max_count;
	
		// 2) Reserve한다.(Reserver함수가 있을 경우에만...)
	#if defined(_MSC_VER)
		__if_exists(container_t::reserve)
		{
			m_container_observer.reserve((_max_count>256) ? 256:_max_count);
		}
	#endif
	}
}

template <class _TOBSERVER, class _TOBSERVER_DATA, class _observer_base_t>
object_ptr<_TOBSERVER> observable::set<_TOBSERVER, _TOBSERVER_DATA, _observer_base_t>::find_observer(const std::function<bool (_TOBSERVER*)>& _pred) const noexcept
{
	// 1) snapshot (_pred runs out of the lock)
	auto snapshot = this->get_observer_snapshot();

	for (auto* iter : snapshot)
	{
		if (_pred(iter) == true)
			return object_ptr<observer_t>(iter);
	}

	// return)
	return object_ptr<observer_t>();
}

template <class _TOBSERVER, class _TOBSERVER_DATA, class _observer_base_t>
bool observable::set<_TOBSERVER, _TOBSERVER_DATA, _observer_base_t>::for_each(const std::function<bool (_TOBSERVER*)>& _function)
{
	// 1) snapshot (_function runs out of the lock)
	return this->for_each_snapshot(_function);
}


template <class _TOBSERVER, class _TOBSERVER_DATA, class _observer_base_t>
typename observable::set<_TOBSERVER, _TOBSERVER_DATA, _observer_base_t>::observer_snapshot_t observable::set<_TOBSERVER, _TOBSERVER_DATA, _observer_base_t>::get_observer_snapshot() const
{
	return this->m_observer_snapshot.get(this->m_lockable_observable, [this]() { return this->_build_observer_snapshot(); });
}

template <class _TOBSERVER, class _TOBSERVER_DATA, class _observer_base_t>
bool observable::set<_TOBSERVER, _TOBSERVER_DATA, _observer_base_t>::for_each_snapshot(const std::function<bool (observer_t*)>& _function) const
{
	auto snapshot = this->get_observer_snapshot();

	for (auto* iter : snapshot)
	{
		if (_function(iter) == false)
			return false;
	}

	return true;
}

template <class _TOBSERVER, class _TOBSERVER_DATA, class _observer_base_t>
group::member_snapshot<typename observable::set<_TOBSERVER, _TOBSERVER_DATA, _observer_base_t>::observer_t>* observable::set<_TOBSERVER, _TOBSERVER_DATA, _observer_base_t>::_build_observer_snapshot() const
{
	auto psnapshot = group::member_snapshot<observer_t>::create(this->m_container_observer.size());

	for (const auto& iter : this->m_container_observer)
	{
		psnapshot->push_back(iter.pobserver.get());
	}

	return psnapshot;
}

}