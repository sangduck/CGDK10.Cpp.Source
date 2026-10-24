//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                              object classes                               *
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

  class factory::object
 
*/
//-----------------------------------------------------------------------------
template <class T>
// inherited classes)
class factory::object : 
	virtual public				Ifactory
{
// constructor/destructor) 
public:
			object() noexcept;
	virtual	~object() noexcept;

// public) 
public:
	virtual	result_code			destroy() noexcept override;

	virtual	size_t				prepare(size_t /*_amount*/) override { return 0;}
	virtual	size_t				shrink(size_t /*_amount*/) noexcept override { return 0;}
	virtual	void				sustain(eFACTORY_GC_LEVEL _level = eFACTORY_GC_LEVEL::NORMAL) override;
	virtual	void				set_gc_type(eFACTORY_GC_TYPE _gc_type) noexcept override { this->m_statistics.gc_type = _gc_type; }

	[[nodiscard]] virtual auto	object_size() const noexcept -> size_t override { return sizeof(T);}
	[[nodiscard]] virtual auto	stacked_count() const noexcept -> size_t override { return 0;}
	[[nodiscard]] virtual auto	existing_limits() const noexcept -> size_t override { return 0;}

public:
	static auto					instance(CGNEW_DEBUG_INFO_PARAMETERS) -> object_ptr<object<T>>;

	#if !defined(_USE_TLS_FACTORY_STATISTICS)
			auto				statistics_on_alloc_create() noexcept { ++this->m_statistics.now.alloc_create; ++this->m_statistics.now.existing; return ++this->m_statistics.now.in_using; }
			void				statistics_on_free_delete() noexcept { ++this->m_statistics.now.free_delete; --this->m_statistics.now.existing; --this->m_statistics.now.in_using; }
	#else
			auto				statistics_on_alloc_create() noexcept { auto ptls_data = this->_get_tls_data(); if (ptls_data == nullptr) { this->_process_statistics_thread_exited(true); return; } ++ptls_data->statistics.now.alloc; ++ptls_data->statistics.now.alloc_create; }
			void				statistics_on_free_delete() noexcept { auto ptls_data = this->_get_tls_data(); if (ptls_data == nullptr) { this->_process_statistics_thread_exited(false); return; } ++ptls_data->statistics.now.free; ++ptls_data->statistics.now.free_delete; }

			// - tls data는 thread가 소유한다. (object_pool_tls과 같은 방식)
			//   pfactory : 이 tls data를 쓰는 factory (factory가 소멸되면 nullptr)
			//   refs     : thread와 factory가 하나씩 잡는다. 둘 다 놓으면 지운다. (thread가 살아있는 동안은 지워지지 않는다.)
			struct sTLS_DATA
			{
				factory::sSTATISTICS_TLS statistics;
				std::atomic<object<T>*> pfactory{ nullptr };
				std::atomic<int32_t> refs{ 0 };
			};
			std::mutex			m_lockable_vector_tls_data;
			std::vector<sTLS_DATA*> m_vector_tls_data;
	thread_local static sTLS_DATA* m_tls_data;

		// - thread가 만든 tls data 목록 (thread 종료 시 놓는다.)
		struct sTLS_OWNER
		{
			std::vector<sTLS_DATA*> vector_tls_data;
			~sTLS_OWNER() noexcept { object<T>::_process_tls_owner_exit(*this); }
		};
	thread_local static bool	m_tls_owner_destroyed;

	// - m_tls_data는 thread_local이라 소멸된 factory(같은 T)의 tls data를 가리킬 수 있다. 주인(pfactory)이 이 factory가 아니면 다시 찾는다.
	//   (thread 종료 처리 뒤에는 nullptr)
	[[nodiscard]] sTLS_DATA*	_get_tls_data() noexcept { auto p = this->m_tls_data; if (p == nullptr || p->pfactory.load(std::memory_order_relaxed) != this) { p = this->_alloc_tls_data(); } return p; }
	[[nodiscard]] sTLS_DATA*	_alloc_tls_data() noexcept;
	[[nodiscard]] static sTLS_OWNER& _get_tls_owner() noexcept { thread_local sTLS_OWNER owner; return owner; }
	static	void				_process_tls_owner_exit(sTLS_OWNER& _owner) noexcept;
	static	void				_release_tls_data(sTLS_DATA* _ptls_data) noexcept { if (_ptls_data->refs.fetch_sub(1, std::memory_order_acq_rel) == 1) delete _ptls_data; }
			void				_process_statistics_thread_exited(bool _is_alloc) noexcept;
	#endif

protected:
	// static)
	static own_ptr<object<T>>	m_s_pinstance;

private:
	virtual	void				close() noexcept override {}
};

template <class T>
own_ptr<factory::object<T>> factory::object<T>::m_s_pinstance;

#if defined(_USE_TLS_FACTORY_STATISTICS)
template <class T>
thread_local typename factory::object<T>::sTLS_DATA* factory::object<T>::m_tls_data = nullptr;

template <class T>
thread_local bool factory::object<T>::m_tls_owner_destroyed = false;

template <class T>
typename factory::object<T>::sTLS_DATA* factory::object<T>::_alloc_tls_data() noexcept
{
	// check) thread is exiting (tls owner is already destroyed) : statistics go to the factory directly
	if (m_tls_owner_destroyed)
		return nullptr;

	// declare)
	auto& owner = _get_tls_owner();
	sTLS_DATA* p = nullptr;

	try
	{
		// 1) reuse tls data released by destroyed factory
		for (auto iter : owner.vector_tls_data)
		{
			if (iter->pfactory.load(std::memory_order_acquire) == nullptr && iter->refs.load(std::memory_order_acquire) == 1)
			{
				iter->statistics = factory::sSTATISTICS_TLS{};
				p = iter;
				break;
			}
		}

		// 2) create new
		if (p == nullptr)
		{
			p = new sTLS_DATA();

			try
			{
				owner.vector_tls_data.push_back(p);
			}
			catch (...)
			{
				delete p;
				throw;
			}
		}

		// 3) bind
		p->refs.store(2, std::memory_order_relaxed);
		p->pfactory.store(this, std::memory_order_release);

		// 4) register
		{
			std::lock_guard<std::mutex> lock(this->m_lockable_vector_tls_data);

			this->m_vector_tls_data.push_back(p);
		}
	}
	catch (...)
	{
		// - release (registering failed)
		if (p != nullptr)
		{
			p->pfactory.store(nullptr, std::memory_order_relaxed);
			p->refs.store(1, std::memory_order_relaxed);
		}
		return nullptr;
	}

	// 5) set
	this->m_tls_data = p;

	// return)
	return p;
}

template <class T>
void factory::object<T>::_process_tls_owner_exit(sTLS_OWNER& _owner) noexcept
{
	// 1) tls data of this thread is not used any more
	m_tls_owner_destroyed = true;
	m_tls_data = nullptr;

	// 2) release tls data (the factory deletes it at sustain() or at destruction)
	for (auto iter : _owner.vector_tls_data)
		_release_tls_data(iter);

	_owner.vector_tls_data.clear();
}

template <class T>
void factory::object<T>::_process_statistics_thread_exited(bool _is_alloc) noexcept
{
	// lock)
	std::lock_guard<std::mutex> lock(this->m_lockable_vector_tls_data);

	// 1) apply to the factory statistics (sustain() applies tls statistics under the same lock)
	if (_is_alloc)
	{
		++this->m_statistics.now.alloc_create;
		++this->m_statistics.now.existing;
		++this->m_statistics.now.in_using;
	}
	else
	{
		++this->m_statistics.now.free_delete;
		--this->m_statistics.now.existing;
		--this->m_statistics.now.in_using;
	}
}
#endif

template <class T>
factory::object<T>::object() noexcept
{
	// 1) set default name
#ifdef __cpp_rtti
	this->name(_factory_name<T>::get_default_name());
#else
	this->name(__FUNCTION__);
#endif
	// 2) set factory type
	this->m_statistics.type = _get_default_factory_type<T>();
	this->m_statistics.object_type = _get_object_type<T>();

	// 주의) 여기서 process_register_factory()를 절대 호출하지 않는다. (무한 순환 루프에 빠질 수 있기 때문에...)
	this->set_gc_type(eFACTORY_GC_TYPE::NO_POOLING);
}

template <class T>
factory::object<T>::~object() noexcept
{
	// 1) unregister from factory manager
	this->process_unregister_factory();

#if defined(_USE_TLS_FACTORY_STATISTICS)
	// 2) detach and release tls data
	//    - tls data of a living thread is not deleted here. (the thread reads 'pfactory' of it - see _get_tls_data())
	//      it is reused or deleted by the thread. (_alloc_tls_data(), _process_tls_owner_exit())
	std::lock_guard<std::mutex> lock(this->m_lockable_vector_tls_data);

	for (auto iter : this->m_vector_tls_data)
	{
		iter->pfactory.store(nullptr, std::memory_order_release);
		_release_tls_data(iter);
	}

	this->m_vector_tls_data.clear();
#endif
}

template <class T>
result_code factory::object<T>::destroy() noexcept
{
	// 2) reset factory instance
	m_s_pinstance.reset();
	
	// return) 
	return result_code(eRESULT::SUCCESS);
}

template <class T>
void factory::object<T>::sustain(eFACTORY_GC_LEVEL /*_level*/)
{
#if defined(_USE_TLS_FACTORY_STATISTICS)
	// lock)
	std::lock_guard<std::mutex> lock(m_lockable_vector_tls_data);

	// 1) prepare
	this->m_statistics.now.tls_stack_depth = 0;
	this->m_statistics.now.tls_stack_depth_threshold = 0;

	// 2) update
	for (auto iter = this->m_vector_tls_data.begin(); iter != this->m_vector_tls_data.end();)
	{
		auto ptls_data = *iter;

		ptls_data->statistics.update_statistics(this->m_statistics.now, this->gc_type());

		// - delete tls data of exited thread (only this factory holds it)
		if (ptls_data->refs.load(std::memory_order_acquire) == 1)
		{
			iter = this->m_vector_tls_data.erase(iter);
			_release_tls_data(ptls_data);
			continue;
		}

		++iter;
	}
#endif
}

template <class T>
inline object_ptr<factory::object<T>> factory::object<T>::instance(CGNEW_DEBUG_INFO_PARAMETERS)
{
	// check) 이미 만들어졌으면 바로 돌려준다. (acquire : 생성이 끝난 factory만 보인다)
	if (auto p = m_s_pinstance.get_acquire(); p != nullptr)
		return object_ptr<factory::object<T>>(p);

	{
		static lock_object cs_create;

		// lock)
		std::lock_guard lock(cs_create);

		// check) return if already created
		RETURN_IF(m_s_pinstance.exist(), m_s_pinstance);

		// 1) create instance (release : 생성이 끝난 뒤에 보이게 한다)
		m_s_pinstance.exchange_release(_make_own_raw<object<T>>(CGNEW_DEBUG_INFO_PARAMETERS_PASSING));

		// 2) register to factory manager (MUST Here!)
		m_s_pinstance->process_register_factory();
	}

	// return) 
	return m_s_pinstance;
}


}