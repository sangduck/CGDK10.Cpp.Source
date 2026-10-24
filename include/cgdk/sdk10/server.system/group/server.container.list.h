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

 @class		list
 @brief		
 @todo		
 @ref		
 @details	\n
 일반적인 선형 Container를 가진 Object 관리자.
 Array/Linked-List Container로 object를 관리한다.

*///-----------------------------------------------------------------------------
template <class _TOBJECT, class _TOBJECT_DATA = int, class _TOBJECTBASE = _TOBJECT>
class container::list :
	virtual public				Iserver_container<_TOBJECT>
{
public:
	using object_t = _TOBJECT;
	using object_base_t = _TOBJECTBASE;
	using object_data_t = _TOBJECT_DATA;
	using object_set_t = OBJECT_SET_DATA<object_t, object_data_t>;
	using container_t = circular_list<object_set_t>;
	using object_snapshot_t = group::member_snapshot_ptr<object_t>;
	using iterator_t = typename container_t::iterator;
	using const_iterator_t = typename container_t::const_iterator;

public:
			result_code			register_object(object_t* _pobject) { object_data_t observer_data {}; return register_object(_pobject, observer_data);}
	virtual	result_code			register_object(object_t* _pobject, const object_data_t& _object_data);
	//! @brief unregisters _pobject. (no reference of _pobject is taken before it is found : it can be called in the final release of _pobject)
	virtual	bool				unregister_object(object_t* _pobject) noexcept override;
	virtual	void				unregister_object_all() noexcept override;

	[[nodiscard]] size_t		count() const noexcept { std::lock_guard lock(this->m_lockable_container); return _get_object_count(); }
	[[nodiscard]] size_t		capacity() const noexcept { std::lock_guard lock(this->m_lockable_container); return _get_max_object_count(); }
			void				capacity(size_t _max_count);
	[[nodiscard]] size_t		room_count() const noexcept { std::lock_guard lock(this->m_lockable_container); return _get_object_room_count();}

	[[nodiscard]] bool			empty() const noexcept { std::lock_guard lock(this->m_lockable_container); return _is_object_empty(); }
	[[nodiscard]] bool			exist() const noexcept { std::lock_guard lock(this->m_lockable_container); return _is_object_exist(); }
	[[nodiscard]] bool			is_full() const noexcept { std::lock_guard lock(this->m_lockable_container); return _is_object_full(); }
	[[nodiscard]] bool			is_room() const noexcept { std::lock_guard lock(this->m_lockable_container); return _is_object_room(); }

	[[nodiscard]] iterator_t	begin() noexcept { return this->m_container_object.begin();}
	[[nodiscard]] const_iterator_t begin() const noexcept { return this->m_container_object.begin();}
	[[nodiscard]] iterator_t	end() noexcept { return this->m_container_object.end();}
	[[nodiscard]] const_iterator_t end() const noexcept { return this->m_container_object.end();}

	//! @brief returns the first object _pred returned true for. _pred runs out of the lock over the object snapshot (see for_each)
	[[nodiscard]] object_ptr<object_t> find_object(const std::function<bool (object_t*)>& _pred) const noexcept;

	//! @brief calls _function for the objects of the snapshot out of the lock (same as for_each_snapshot) @return false _function returned false
	// - an object can be unregistered before or while _function runs.
			bool				for_each(const std::function<bool (object_t*)>& _function);

	//! @brief objects as an immutable array (copy-on-write). no lock while the published one is valid. in on_object_registered / on_object_unregistering / on_object_unregistered : the live objects
	[[nodiscard]] object_snapshot_t get_object_snapshot() const;
	//! @brief calls _function for the objects of the snapshot out of the lock. (objects may be registered / unregistered in _function) @return false _function returned false
			bool				for_each_snapshot(const std::function<bool (object_t*)>& _function) const;

protected:
	// - lock of the events : on_object_registering in m_lockable_container_enter, the others in m_lockable_container.
	//   do not wait for another thread which locks this container in them (deadlock). the object is held (object_ptr) while they run.
	virtual	result_code			on_object_registering (object_t* /*_pobject*/, const object_data_t& /*_object_data*/){ return eRESULT::SUCCESS;}
	virtual	void				on_object_registered (object_t* /*_pobject*/, const object_data_t& /*_object_data*/) {}
	virtual	uintptr_t			on_object_unregistering (object_t* /*_pobject*/) { return 0;}
	virtual	void				on_object_unregistered (object_t* /*_pobject*/, uintptr_t /*_result*/){}

private:
	[[nodiscard]] size_t		_get_object_count() const noexcept { return this->m_container_object.size();}
	[[nodiscard]] size_t		_get_object_room_count() const noexcept { return this->m_capacity - this->m_container_object.size();}
	[[nodiscard]] size_t		_get_max_object_count() const noexcept { return this->m_capacity;}
			
	[[nodiscard]] bool			_is_object_full() const noexcept { return this->m_container_object.size()>=this->m_capacity;}
	[[nodiscard]] bool			_is_object_empty() const noexcept { return this->m_container_object.empty();}
	[[nodiscard]] bool			_is_object_room() const noexcept { return this->m_container_object.size()<this->m_capacity;}
	[[nodiscard]] bool			_is_object_exist() const noexcept { return !this->m_container_object.empty();}
	[[nodiscard]] group::member_snapshot<object_t>* _build_object_snapshot() const;

protected:
			container_t			m_container_object;
			size_t				m_capacity = SIZE_MAX;

	// - published object snapshot (changed in m_lockable_container, read without it)
	mutable	group::member_snapshot_holder<object_t> m_object_snapshot;
};

template <class _TOBJECT, class _TOBJECT_DATA, class _TOBJECTBASE>
result_code container::list<_TOBJECT, _TOBJECT_DATA, _TOBJECTBASE>::register_object(object_t* _pobject,const object_data_t& _object_data)
{
	// check) _pobject가 nullptr이면 안 된다.
	RETURN_IF(_pobject == nullptr, eRESULT::INVALID_ARGUMENT);
	
	SCOPED_LOCK(this->m_lockable_container_enter)
	{
		// check) 꽉 찼는가?
		RETURN_IF(is_full(), eRESULT::MEMBER_FULL);

		// declare) 
		object_set_t temp_object_set(_pobject, _object_data);

		// 1) 이미 존재하는지 확인한다.
		SCOPED_LOCK(this->m_lockable_container)
		{
			for (auto& iter : this->m_container_object)
			{
				// check) 이미 존재하면 끝낸다.
				RETURN_IF(iter == temp_object_set, eRESULT::ALREADY_EXIST);
			}
		}

		// 2) on_object_registering()함수를 호출한다.
		auto result = this->on_object_registering(_pobject, _object_data);

		// check) 입장 조건에 맞지 않는다!
		RETURN_IF(result != eRESULT::SUCCESS, result);

		SCOPED_LOCK(this->m_lockable_container)
		{
			typename group::member_snapshot_holder<object_t>::write_scope scope_write(this->m_object_snapshot);

			// 3) 추가한다.
			this->m_container_object.push_back(temp_object_set);

			// 4) on_object_registered()함수를 호출한다.
			try
			{
				this->on_object_registered(_pobject, _object_data);
			}
			// exception) rollback한다.
			catch(std::exception& e)
			{
				// log)
				LOG_FATAL << "(excp) exception completion[on_object_registered]"sv;

				// - exception complete
				auto is_executed = throwable::execute_exception_handler(this, e, __FUNCTION__);

				// - object를 제거한다.
				this->m_container_object.pop_back();

				// check) 
				RETURN_IF(is_executed, eRESULT::EXCEPTION);

				// reraise) 
				throw;
			}
		}
	}

	// return) 성공이다!!!
	return	eRESULT::SUCCESS;
}

template <class _TOBJECT, class _TOBJECT_DATA, class _TOBJECTBASE>
bool container::list<_TOBJECT, _TOBJECT_DATA, _TOBJECTBASE>::unregister_object(object_t* _pobject) noexcept
{
	// check) _pobject가 nullptr이여서는 안 된다.
	CGASSERT_ERROR(_pobject != nullptr);

	// check) 
	RETURN_IF(_pobject == nullptr, false);

	// declare) holds the object till return (erase releases the reference of the container)
	object_ptr<object_t> pobject;

	SCOPED_LOCK(this->m_lockable_container)
	{
		// declare) 
		uintptr_t result = 0;

		// 1) 먼저 이미 등록되어 있는지 찾는다. (by the raw pointer. no object_ptr of _pobject before it is found)
		auto iter_find = std::find_if(this->m_container_object.begin(), this->m_container_object.end(), [=](const object_set_t& _iter) { return _iter.pobject.get() == _pobject; });

		// check) 찾지 못했으면 끝낸다.
		RETURN_IF(iter_find== this->m_container_object.end(), false);

		typename group::member_snapshot_holder<object_t>::write_scope scope_write(this->m_object_snapshot);

		// - holds the object till on_object_unregistered() and the return
		pobject = iter_find->pobject;

		// 2) on_object_unregistering()함수를 호출한다.
		try
		{
			result = on_object_unregistering(pobject.get());
		}
		// exception) 
		catch(...)
		{
			LOG_FATAL << "(prgr) exception occurred in 'on_object_unregistering' function on object"sv;
		}

		// 3) 제거한다.
		this->m_container_object.erase(iter_find);

		// 4) on_object_unregistered()함수를 호출한다.
		try
		{
			this->on_object_unregistered(pobject.get(), result);
		}
		// exception) 
		catch(...)
		{
			LOG_FATAL << "(prgr) exception occurred in 'on_object_unregistered' function on object"sv;
		}
	}

	// return) 
	return true;
}

template <class _TOBJECT, class _TOBJECT_DATA, class _TOBJECTBASE>
void container::list<_TOBJECT, _TOBJECT_DATA, _TOBJECTBASE>::unregister_object_all() noexcept
{
	// declare) 
	container_t vector_object;

	// 1) 복사한다.
	SCOPED_LOCK(this->m_lockable_container)
	{
		vector_object = this->m_container_object;
	}

	// 2) 모두 unregister한다.
	for(auto& iter: vector_object)
	{
		this->unregister_object(iter.pobject);
	}
}

template <class _TOBJECT, class _TOBJECT_DATA, class _TOBJECTBASE>
void container::list<_TOBJECT, _TOBJECT_DATA, _TOBJECTBASE>::capacity(size_t _max_count)
{
	SCOPED_LOCK(this->m_lockable_container)
	{
		// 1) 최대 Object수를 설정한다.
		m_capacity = _max_count;
	
		// 2) Reserve한다.(Reserver함수가 있을 경우에만...)
	#if defined(_MSC_VER)
		__if_exists(container_t::reserve)
		{
			this->m_container_object.reserve((_max_count>256) ? 256:_max_count);
		}
	#endif
	}
}

template <class _TOBJECT, class _TOBJECT_DATA, class _TOBJECTBASE>
object_ptr<_TOBJECT> container::list<_TOBJECT, _TOBJECT_DATA, _TOBJECTBASE>::find_object(const std::function<bool (object_t*)>& _pred) const noexcept
{
	// 1) snapshot (_pred runs out of the lock)
	auto snapshot = this->get_object_snapshot();

	for (auto* iter : snapshot)
	{
		if (_pred(iter) == true)
			return object_ptr<object_t>(iter);
	}

	// return) 찾지 못했다..
	return	object_ptr<object_t>();
}

template <class _TOBJECT, class _TOBJECT_DATA, class _TOBJECTBASE>
bool container::list<_TOBJECT, _TOBJECT_DATA, _TOBJECTBASE>::for_each(const std::function<bool (object_t*)>& _function)
{
	// 1) snapshot (_function runs out of the lock)
	return this->for_each_snapshot(_function);
}


template <class _TOBJECT, class _TOBJECT_DATA, class _TOBJECTBASE>
typename container::list<_TOBJECT, _TOBJECT_DATA, _TOBJECTBASE>::object_snapshot_t container::list<_TOBJECT, _TOBJECT_DATA, _TOBJECTBASE>::get_object_snapshot() const
{
	return this->m_object_snapshot.get(this->m_lockable_container, [this]() { return this->_build_object_snapshot(); });
}

template <class _TOBJECT, class _TOBJECT_DATA, class _TOBJECTBASE>
bool container::list<_TOBJECT, _TOBJECT_DATA, _TOBJECTBASE>::for_each_snapshot(const std::function<bool (object_t*)>& _function) const
{
	auto snapshot = this->get_object_snapshot();

	for (auto* iter : snapshot)
	{
		if (_function(iter) == false)
			return false;
	}

	return true;
}

template <class _TOBJECT, class _TOBJECT_DATA, class _TOBJECTBASE>
group::member_snapshot<typename container::list<_TOBJECT, _TOBJECT_DATA, _TOBJECTBASE>::object_t>* container::list<_TOBJECT, _TOBJECT_DATA, _TOBJECTBASE>::_build_object_snapshot() const
{
	auto psnapshot = group::member_snapshot<object_t>::create(this->m_container_object.size());

	for (const auto& iter : this->m_container_object)
	{
		psnapshot->push_back(iter.pobject.get());
	}

	return psnapshot;
}

}
