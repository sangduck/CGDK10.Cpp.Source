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

 @class		object_pool_tls
 @brief		객체의 할당과 해제의 효율을 향상시키기 위한 객체 풀
 @todo		
 @ref		Ipoolable, Npoolable
 @details	\n
 객체의 동적 할당과 해제의 효율성을 높이기 위한 풀 객체이다.
 내부적으로 락프리로 동작하기 때문에 매우 고성능을 제공한다.

 풀의 대상 객체가 되려면 반드시 Ipoolable<T>를 상속받은 클래스여야 한다.

 풀에 할당을 요구하면 먼저 가비지 스택를 먼저 검사한다. 만약 스택에 객체가 있을 경우 그것을 할당해준다.
 만약 스택에 객체가 하나도 없다면 새로 생성해서 할당해준다.
 할당해준 객체가 다시 돌아오게 되면 가비지 스택에 저장해 놓았다가 다음 할당 요구때 할당해준다.
 다만 너무 많은 객체가 가비지 스택에 존재할 경우 일정량을 해제해 적절한 수량을 유지한다.

 풀에서 객체를 할당하기 위해서는 alloc()함수를 호출하며 할당 했던 객체를 해제하기 위해서는 free()함수를 사용한다.
 prepare()함수를 사용하면 미리 객체를 생성해놓을 수도 있다.
 shrink()함수를 사용하면 쌓여있는 가비지들을 할당 해제할 수도 있다.

*/
//-----------------------------------------------------------------------------
template <class T>
class factory::object_pool_tls : 
// inherited classes)
	public						factory::Iobject<T>
{
// constructor/destructor) 
public:
			object_pool_tls() : object_pool_tls(std::string_view(), eFACTORY_TYPE::POOL_USER) { this->set_gc_type(eFACTORY_GC_TYPE::DEFAULT); }
			object_pool_tls(const std::string_view _name, eFACTORY_TYPE _factory_type = eFACTORY_TYPE::DEFAULT) : factory::Iobject<T> (_name, _factory_type) { this->set_gc_type(eFACTORY_GC_TYPE::DEFAULT); }
	virtual	~object_pool_tls() noexcept;

// public) 
public:
	virtual	size_t				prepare(size_t _amount) override;
	virtual	size_t				shrink(size_t _amount) noexcept override;
	virtual	void				sustain(eFACTORY_GC_LEVEL _level= eFACTORY_GC_LEVEL::NORMAL) noexcept override;

	[[nodiscard]] virtual auto	object_size() const noexcept -> size_t override { return sizeof(T);}
	[[nodiscard]] virtual auto	stacked_count() const noexcept -> size_t override { return this->m_stack_object.size();}
	[[nodiscard]] virtual auto	existing_limits() const noexcept -> size_t override { return this->m_statistics.now.existing_limits;}
	virtual	void				set_gc_type(eFACTORY_GC_TYPE _gc_type) noexcept override;

	[[nodiscard]] T*			alloc(CGNEW_DEBUG_INFO_PARAMETERS_WITH_DEFAULT);
			void				dealloc(T* _pobject) noexcept;

		// - thread마다 따로 할당되는 tls data는 다른 thread의 tls data와 cache line을 공유하지 않도록 64 byte 정렬한다. (false sharing 방지)
		//   (_alloc_tls_data()에서 aligned new로 직접 할당한다.)
	#if defined(_MSC_VER)
		#pragma warning(push)
		#pragma warning(disable: 4324)	// structure was padded due to alignment specifier
	#endif
			struct alignas(64) sTLS_DATA
			{
				T*				tls_stack = nullptr;
				sSTATISTICS_TLS statistics;

				// - 아래 두 멤버는 64 byte 정렬 padding 자리에 둔다. (위 멤버의 offset과 sizeof(sTLS_DATA)는 그대로다.)
				//   ppool : 이 tls data를 쓰는 pool (pool이 소멸되면 nullptr)
				//   refs  : thread와 pool이 하나씩 잡는다. 둘 다 놓으면 지운다. (thread가 살아있는 동안은 지워지지 않는다.)
				std::atomic<object_pool_tls<T>*> ppool{ nullptr };
				std::atomic<int32_t> refs{ 0 };
			};
	#if defined(_MSC_VER)
		#pragma warning(pop)
	#endif
	mutable	std::mutex			m_lockable_vector_tls_data;
			std::vector<sTLS_DATA*> m_vector_tls_data;
	thread_local static sTLS_DATA* m_tls_data[54];
			int					m_local_data_index = 0;

		// - thread가 만든 tls data 목록 (thread 종료 시 tls stack을 비우고 놓는다.)
		struct sTLS_OWNER
		{
			std::vector<sTLS_DATA*> vector_tls_data;
			~sTLS_OWNER() noexcept { object_pool_tls<T>::_process_tls_owner_exit(*this); }
		};
	thread_local static bool	m_tls_owner_destroyed;

	// - m_tls_data는 thread_local이라 같은 T, 같은 index의 다른 pool(소멸된 pool 포함)의 tls data를 가리킬 수 있다. 주인(ppool)이 이 pool이 아니면 다시 찾는다.
	[[nodiscard]] sTLS_DATA*	_get_tls_data() noexcept { auto p = *(m_tls_data + this->m_local_data_index); if (p == nullptr || p->ppool.load(std::memory_order_relaxed) != this) { p = this->_alloc_tls_data();} return p; }
	[[nodiscard]] sTLS_DATA*	_alloc_tls_data();
	[[nodiscard]] static sTLS_OWNER& _get_tls_owner() noexcept { thread_local sTLS_OWNER owner; return owner; }
	static	void				_process_tls_owner_exit(sTLS_OWNER& _owner) noexcept;
	static	void				_release_tls_data(sTLS_DATA* _ptls_data) noexcept { if (_ptls_data->refs.fetch_sub(1, std::memory_order_acq_rel) == 1) delete _ptls_data; }
			void				_process_check_closed(sTLS_DATA* _ptls_data) noexcept { if (this->m_statistics.now.existing_limits < 0 && _ptls_data->statistics.now.tls_stack_depth_threshold >= 0) _ptls_data->statistics.now.tls_stack_depth_threshold = -1; }
			void				_process_purge_central_stack(sTLS_DATA* _ptls_data) noexcept;
			bool				_process_gathering_tls_statistics() noexcept;
			void				_free_all_tls_data() noexcept;
			T*					_process_pop_from_central_stack_or_create(sTLS_DATA* _ptls_data CGNEW_DEBUG_INFO_COMMA CGNEW_DEBUG_INFO_PARAMETERS);
			void 				_process_push_to_central_stack_or_delete(sTLS_DATA* _ptls_data, T* _pobject);
			void				_process_tls_redundancy(sTLS_DATA* _ptls_data, int32_t _count);
	[[nodiscard]] int32_t		_get_tls_batch() noexcept;

	// definitions) tls stack <-> central stack 배치 이동
	//  - TLS_POOLING에서 tls stack과 central stack 사이는 객체를 하나씩이 아니라 묶음(chain)으로 옮긴다.
	//    (alloc 시 central stack에서 묶음으로 가져와 tls stack에 채우고, free 시 tls stack에 모아 묶음으로 돌려준다.)
	//  - 묶음 크기(batch) = TLS_BATCH_BYTES / object_size()  (1 ~ TLS_BATCH_MAX)
	//  - free 쪽 tls stack은 tls_stack_depth_threshold가 작아도 batch개까지는 보관한다. (최소 캐시 용량)
	static constexpr int32_t	TLS_BATCH_MAX = 32;
	static constexpr size_t		TLS_BATCH_BYTES = 64 * 1024;
	static constexpr int32_t	TLS_THRESHOLD_MAX = INT32_MAX / 2;	// tls_stack_depth_threshold 증가 상한 (int32 overflow 방지)

private:
			object_pool_tls(const object_pool_tls<T>&) = delete;
			void				operator = (const object_pool_tls<T>&) = delete;

	template <class P>
	[[nodiscard]] std::enable_if_t<std::is_base_of_v<factory::_traits_has_pool, P>, T*>
								process_create_object(CGNEW_DEBUG_INFO_PARAMETERS)
	{
		// 1) 일단 새로 생성된다.
		auto pobject = this->create_object(CGNEW_DEBUG_INFO_PARAMETERS_PASSING);

		// 2) pool에 setting한다. (처리과정에서 pool의 reference Counting를 1증가시키게 된다.)
		pobject->m_ppool = this;
		pobject->m_pfactory = this;

		// statistics)
		this->m_existing_total.fetch_add(1, std::memory_order_relaxed);

		// return)
		return pobject;
	}

	template <class P>
	[[nodiscard]] std::enable_if_t<!std::is_base_of_v<factory::_traits_has_pool, P>, T*>
								process_create_object(CGNEW_DEBUG_INFO_PARAMETERS)
	{
		// 1) call 'create_object'
		auto pobject = this->create_object(CGNEW_DEBUG_INFO_PARAMETERS_PASSING);

		// 2) increase reference count (m_ppool에 setting하지 않으면 자체적으로 증가시킨다.)
		this->add_ref();

		// statistics)
		this->m_existing_total.fetch_add(1, std::memory_order_relaxed);

		// return) 성공!!
		return pobject;
	}

	virtual	void				process_tls_garbage_collection() override
	{
		// 1) get tls data
		auto ptls_data = this->_get_tls_data();

		// check) closed pool : empty tls stack of this thread
		this->_process_check_closed(ptls_data);

		// 2) get count of redundancy (this thread's tls_stack_depth_threshold)
		auto count_redundancy = ptls_data->statistics.now.tls_stack_depth - ptls_data->statistics.now.tls_stack_depth_threshold;

		// check)
		RETURN_IF(count_redundancy <= 0, );

		// 3) process garbage collection 
		this->_process_tls_redundancy(ptls_data, count_redundancy);
	}


private:
	// 3) 재고 객체 stack
	//    - central stack(lock)은 여러 thread가 계속 쓰는 cache line이다.
	//      tls fast path가 매번 읽는 멤버(m_local_data_index 등)와 cache line을 공유하지 않도록 앞뒤를 패딩으로 나눈다.
	//      (alignas(64)는 pool 객체가 64 byte 정렬로 할당된다는 보장이 없어(debug new 등) 쓰지 않는다.)
			std::byte			m_padding_stack_front[64]{};
			LOCKFREE_SELF_STACK<T*> m_stack_object;

	// 4) Pool Sustainment를 위한 변수들
	//    - m_existing_total : 생성되어 있는 객체 수(생성 - 파괴). 생성/파괴할 때만 relaxed로 증감한다.
	//      central stack 밖에 있는 객체 수(사용 중 + tls stack)는 _get_existing() = m_existing_total - central stack 깊이로 구한다.
	//      (central stack과 tls stack 사이의 배치 이동에서는 공유 카운터를 갱신하지 않는다.)
			std::atomic<int64_t> m_existing_total = 0;
			int64_t				m_existing_term_peak = 0;
			std::byte			m_padding_stack_back[64]{};
			int64_t				m_existing_smoothed_average = 0;
			int64_t				m_existing_variation = 0;
			std::atomic<int32_t> m_tls_batch = 0;	// 0 : 아직 계산 전 (_get_tls_batch())

	[[nodiscard]] int64_t		_get_existing() const noexcept { return this->m_existing_total.load(std::memory_order_relaxed) - static_cast<int64_t>(this->m_stack_object.size()); }
private:
	[[nodiscard]] virtual T* create_object(CGNEW_DEBUG_INFO_PARAMETERS) override;
	virtual	void				close() noexcept override;
};

template <class T>
thread_local typename factory::object_pool_tls<T>::sTLS_DATA* factory::object_pool_tls<T>::m_tls_data[54];

template <class T>
thread_local bool factory::object_pool_tls<T>::m_tls_owner_destroyed = false;

template <class T>
factory::object_pool_tls<T>::~object_pool_tls() noexcept
{
	CGASSERT_ERROR(this->m_existing_total == 0);

	// lock)
	std::lock_guard<std::mutex> lock(this->m_lockable_vector_tls_data);

	// 1) detach and release vector_tls_data
	//    - tls data of a living thread is not deleted here. (the thread reads 'ppool' of it - see _get_tls_data())
	//      it is reused or deleted by the thread. (_alloc_tls_data(), _process_tls_owner_exit())
	for (auto iter : this->m_vector_tls_data)
	{
		// - statistics (objects released after close())
		iter->statistics.update_statistics(this->m_statistics.now, this->gc_type());

		// - detach and release
		iter->ppool.store(nullptr, std::memory_order_release);
		_release_tls_data(iter);
	}

	this->m_vector_tls_data.clear();
}

template <class T>
void factory::object_pool_tls<T>::close() noexcept
{
	// 1) 최대 유지 개수를 -1로 한다.
	this->m_statistics.now.existing_limits = -1;

	// 2) close tls data of all threads (tls_stack_depth_threshold = -1)
	//    - tls stack of other threads is emptied by the thread itself. (at next free/gc or thread exit)
	this->_free_all_tls_data();

	// 3) empty tls stack of this thread (closed pool deletes objects - existing_limits is -1)
	auto ptls_data = *(this->m_tls_data + this->m_local_data_index);

	if (ptls_data != nullptr && ptls_data->ppool.load(std::memory_order_relaxed) == this)
	{
		ptls_data->statistics.now.tls_stack_depth_threshold = -1;

		if (ptls_data->tls_stack != nullptr)
			this->_process_tls_redundancy(ptls_data, 1);
	}
	else
	{
		ptls_data = nullptr;
	}

	// 4) free stacked object
	T* pobject;
	while ((pobject = this->m_stack_object.pop()) != nullptr)
	{
		// - 객체를 파괴한다.
		this->template process_destroy_object<T>(pobject);

		// statistics)
		this->m_existing_total.fetch_sub(1, std::memory_order_relaxed);

		if (ptls_data != nullptr)
		{
			--ptls_data->statistics.now.free_stack;
			++ptls_data->statistics.now.free_delete;
		}
		else
		{
			--this->m_statistics.now.existing;
			--this->m_statistics.now.free_stack;
			++this->m_statistics.now.free_delete;
		}
	}
}

template <class T>
typename factory::object_pool_tls<T>::sTLS_DATA* factory::object_pool_tls<T>::_alloc_tls_data()
{
	// declare)
	sTLS_DATA* p = nullptr;
	int32_t refs = 2;

	// 1) find tls data of this thread
	if (m_tls_owner_destroyed == false)
	{
		auto& owner = _get_tls_owner();

		// - already bound to this pool (other pool of same T and index used this slot)
		for (auto iter : owner.vector_tls_data)
		{
			if (iter->ppool.load(std::memory_order_relaxed) == this)
			{
				*(this->m_tls_data + this->m_local_data_index) = iter;
				return iter;
			}
		}

		// - reuse tls data released by destroyed pool
		for (auto iter : owner.vector_tls_data)
		{
			if (iter->ppool.load(std::memory_order_acquire) == nullptr && iter->refs.load(std::memory_order_acquire) == 1)
			{
				iter->tls_stack = nullptr;
				iter->statistics = sSTATISTICS_TLS{};
				p = iter;
				break;
			}
		}

		// - create new sTLS_DATA (aligned new - 'new' macro(debug new) is not used)
		if (p == nullptr)
		{
		#pragma push_macro("new")
		#undef new
			p = new sTLS_DATA;
		#pragma pop_macro("new")

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
	}
	// 2) thread is exiting (tls owner is already destroyed) : not cached in tls stack and the thread's reference is not released (never deleted)
	else
	{
	#pragma push_macro("new")
	#undef new
		p = new sTLS_DATA;
	#pragma pop_macro("new")

		p->statistics.now.tls_stack_depth_threshold = -1;
	}

	// 3) set default value;
	if (this->gc_type() == eFACTORY_GC_TYPE::TLS_POOLING_NO_GC && p->statistics.now.tls_stack_depth_threshold >= 0)
		p->statistics.now.tls_stack_depth_threshold = INT32_MAX;

	p->refs.store(refs, std::memory_order_relaxed);
	p->ppool.store(this, std::memory_order_release);

	// 4) register
	{
		// lock)
		std::lock_guard<std::mutex> lock(this->m_lockable_vector_tls_data);

		// check) closed pool
		if (this->m_statistics.now.existing_limits < 0)
			p->statistics.now.tls_stack_depth_threshold = -1;

		this->m_vector_tls_data.push_back(p);
	}

	// 5) set...
	*(this->m_tls_data + this->m_local_data_index) = p;

	return p;
}

template <class T>
void factory::object_pool_tls<T>::_process_tls_owner_exit(sTLS_OWNER& _owner) noexcept
{
	// 1) tls data of this thread is not used any more
	m_tls_owner_destroyed = true;

	for (auto& iter : m_tls_data)
		iter = nullptr;

	// 2) empty tls stack and release tls data
	for (auto iter : _owner.vector_tls_data)
	{
		// - no more tls caching
		iter->statistics.now.tls_stack_depth_threshold = -1;

		// - empty tls stack (to central stack or delete)
		//   (objects in tls stack hold the pool. so the pool is alive if tls stack is not empty)
		if (iter->tls_stack != nullptr)
		{
			auto ppool = iter->ppool.load(std::memory_order_acquire);

			if (ppool != nullptr)
			{
				object_ptr<object_pool_tls<T>> phold_pool = ppool;

				ppool->_process_tls_redundancy(iter, 1);
			}
		}

		// - release
		_release_tls_data(iter);
	}

	_owner.vector_tls_data.clear();
}

template <class T>
void factory::object_pool_tls<T>::_process_purge_central_stack(sTLS_DATA* _ptls_data) noexcept
{
	// declare) hold this pool while deleting objects (see _process_push_to_central_stack_or_delete())
	object_ptr<object_pool_tls<T>> phold_pool = this;

	// 1) delete all objects in central stack
	//    - an object pushed to central stack after close() emptied it (existing_limits was read before close())
	int32_t count_delete = 0;

	T* pobject;
	while ((pobject = this->m_stack_object.pop()) != nullptr)
	{
		this->template process_destroy_object<T>(pobject);
		++count_delete;
	}

	// statistics)
	this->m_existing_total.fetch_sub(count_delete, std::memory_order_relaxed);
	_ptls_data->statistics.now.free_stack -= count_delete;
	_ptls_data->statistics.now.free_delete += count_delete;
}

template <class T>
bool factory::object_pool_tls<T>::_process_gathering_tls_statistics() noexcept
{
	// declare)
	bool result = false;

	// 1) gathering tls data
	{
		// lock)
		std::lock_guard<std::mutex> lock(this->m_lockable_vector_tls_data);

		// - prepare
		this->m_statistics.now.tls_stack_depth = 0;
		this->m_statistics.now.tls_stack_depth_threshold = 0;

		// - update
		for (auto iter = this->m_vector_tls_data.begin(); iter != this->m_vector_tls_data.end();)
		{
			auto ptls_data = *iter;

			result |= ptls_data->statistics.update_statistics(this->m_statistics.now, this->gc_type());

			// - delete tls data of exited thread (only this pool holds it and tls stack was emptied at thread exit)
			if (ptls_data->refs.load(std::memory_order_acquire) == 1 && ptls_data->tls_stack == nullptr)
			{
				iter = this->m_vector_tls_data.erase(iter);
				_release_tls_data(ptls_data);
				continue;
			}

			++iter;
		}
	}

	// return)
	return result;
}

template <class T>
void factory::object_pool_tls<T>::_free_all_tls_data() noexcept
{
	// 1) close tls data of all threads and gather statistics
	//    - tls stack of other thread can't be accessed here (the thread may alloc/free at the same time).
	//      the thread empties its tls stack at next free/gc(tls_stack_depth_threshold < 0) or at thread exit. (_process_tls_owner_exit())
	{
		// lock)
		std::lock_guard<std::mutex> lock(this->m_lockable_vector_tls_data);

		// - prepare
		this->m_statistics.now.tls_stack_depth = 0;

		// - apply all to m_statistics
		for (auto iter : this->m_vector_tls_data)
		{
			// - set closed
			iter->statistics.now.tls_stack_depth_threshold = -1;

			// - process statistics
			iter->statistics.update_statistics(this->m_statistics.now, this->gc_type());
		}
	}
}

template <class T>
size_t factory::object_pool_tls<T>::prepare(size_t _amount)
{
	// check) 0보다 작거나 같으면 prepare할꺼 같으면 그냥 끝낸다.
	if(_amount <= 0)
		return 0;

	// 1) 임시로 사용할 List를 할당받는다.
	std::vector<T*> vector_created;

	// 2) Reserve한다.
	vector_created.reserve(_amount);

	try
	{
		// 3) prepare 수만큼 Loop를 돌면서 생성한다.
		for(; _amount>0; --_amount)
		{
			// declare) 새로 생성한다.
			auto pcreate = this->template process_create_object<T>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL);

			// - 생성된 객체를 임시 버퍼에 짚어 넣는다.
			vector_created.emplace_back(pcreate);
		}
	}
	// exception) 
	catch(...)
	{
	}

	// declare)
	auto size_created = static_cast<int32_t>(vector_created.size());

	// 4) 평균값을 수정한다.
	this->m_existing_smoothed_average += size_created;

	// statistics) 
	this->_get_tls_data()->statistics.now.statistics_on_prepare(size_created);

	// 6) store to stack (chain size of central stack is set by _get_tls_batch())
	(void)this->_get_tls_batch();

	for(auto& iter:vector_created)
		this->m_stack_object.push(iter);

	// return) 성공!!
	return vector_created.size();
}

template <class T>
size_t factory::object_pool_tls<T>::shrink(size_t _amount) noexcept
{
	// case) shrink exceed
	if(_amount == eFACTORY_SHRINK::EXCEED)
		_amount = static_cast<size_t>(this->m_statistics.now.existing - this->m_statistics.now.existing_limits);
	// case) shrink all
	else if(_amount == eFACTORY_SHRINK::ALL)
		_amount = static_cast<int>(m_stack_object.size());

	// check) return if _amount is 0
	if(_amount < 1)
		return 0;

	// declare)
	uint32_t count_shrink = 0;

	// 1) stack되어 있는 object을 tempShrink만큼 지운다. (chain 단위로 pop해 lock 횟수를 줄인다.)
	while(count_shrink < _amount)
	{
		// - Pop을 먼저한다.
		const auto count_remained = _amount - count_shrink;
		T* pobject_last;
		int count_pop;
		auto pobject = this->m_stack_object.pop((count_remained < 64) ? static_cast<int>(count_remained) : 64, pobject_last, count_pop);

		// check) pobject가 nullptr이면 안된다.
		if(pobject == nullptr)
			break;

		// - Object를 destroy한다. (pop한 chain은 nullptr로 끝난다.)
		while (pobject != nullptr)
		{
			auto pobject_next = pobject->Next;
			this->template process_destroy_object<T>(pobject);
			pobject = pobject_next;
		}

		count_shrink += static_cast<uint32_t>(count_pop);
	}

	// statistics)
	this->m_existing_total.fetch_sub(count_shrink, std::memory_order_relaxed);

	// statistics)
	this->_get_tls_data()->statistics.now.statistics_on_shrink(count_shrink);

	// 4) 평균값을 수정.
	this->m_existing_smoothed_average -= count_shrink;
	
	// return)
	return count_shrink;
}

template <class T>
void factory::object_pool_tls<T>::sustain(eFACTORY_GC_LEVEL _level) noexcept
{
	// 1) gathering tls statistics
	auto is_active = this->_process_gathering_tls_statistics();

	// check) is closed?
	if(this->m_statistics.now.existing_limits < 0)
		return;

	// 2) garbage collection
	auto ptls_data = this->_get_tls_data();

	// 3) get garbage count (this thread's tls_stack_depth_threshold)
	auto count_garbage = ptls_data->statistics.now.tls_stack_depth - ptls_data->statistics.now.tls_stack_depth_threshold;

	// 4) garbage collection
	if (count_garbage > 0)
		this->_process_tls_redundancy(ptls_data, count_garbage);

	// declare)
	auto temp_gc_type = this->gc_type();

	// 5) calculate m_statistics.now.existing_limits
	if (temp_gc_type == eFACTORY_GC_TYPE::POOLING
	 || temp_gc_type == eFACTORY_GC_TYPE::TLS_POOLING)
	{
		int64_t in_using_max;

		// - calculate using_max
		if (is_active == true)
			in_using_max = ((this->m_existing_term_peak * 9) >> 3) + 8;
		else
			in_using_max = this->m_existing_term_peak;

		// - get average of stacked
		auto in_using_smoothed_average = ((this->m_existing_smoothed_average * 896 + in_using_max * 128) >> 10);

		// - get distributions of stacked
		auto in_using_variation = ((this->m_existing_variation * 896 + 128 * ((in_using_smoothed_average > in_using_max) ? (in_using_smoothed_average - in_using_max) : (in_using_max - in_using_smoothed_average)) + 1) >> 10);

		// - get max existing stack
		if (this->m_statistics.now.existing_limits >= 0)
		{
			auto existing_limits = in_using_smoothed_average + (in_using_variation << 2);
			this->m_statistics.now.existing_limits = (existing_limits >= 0) ? existing_limits : 0;
		}

		// - update and reset values
		this->m_existing_smoothed_average = in_using_smoothed_average;
		this->m_existing_variation = in_using_variation;
		this->m_existing_term_peak = this->m_statistics.now.in_using;
	}

	// 6) get shrink count
	auto count_shrink = this->_get_shrink_count(_level);

	// 7) shrink 
	this->shrink(count_shrink);
}

template <class T>
T* factory::object_pool_tls<T>::create_object(CGNEW_DEBUG_INFO_PARAMETERS)
{
#pragma push_macro("new")
#undef new
	#ifdef _ENABLE_DEBUG_NEW
		#if defined(_WIN32)
			return new(_NORMAL_BLOCK, CGNEW_DEBUG_INFO_PARAMETERS_PASSING) T;
		#else
			return new T;
		#endif
	#else
		return new T;
	#endif
#pragma pop_macro("new")
}

template <class T>
void factory::object_pool_tls<T>::set_gc_type(eFACTORY_GC_TYPE _gc_type) noexcept
{
	// check)
	RETURN_IF(this->gc_type() == _gc_type);

	// 1) set gc_type
	this->m_statistics.gc_type = _gc_type;

	switch (_gc_type)
	{
	case	eFACTORY_GC_TYPE::NO_POOLING:
			this->m_statistics.now.existing_limits = 0;
			{
				// lock)
				std::lock_guard<std::mutex> lock(this->m_lockable_vector_tls_data);

				for (auto iter : m_vector_tls_data)
					iter->statistics.now.tls_stack_depth_threshold = 0;
			}
			break;

	case	eFACTORY_GC_TYPE::POOLING:
			{
				this->m_statistics.now.existing_limits = this->_get_existing();
				{
					// lock)
					std::lock_guard<std::mutex> lock(this->m_lockable_vector_tls_data);

					for (auto iter : m_vector_tls_data)
						iter->statistics.now.tls_stack_depth_threshold = 0;
				}
			}
			break;

	case	eFACTORY_GC_TYPE::POOLING_NO_GC:
			{
				this->m_statistics.now.existing_limits = INT64_MAX;
				{
					// lock)
					std::lock_guard<std::mutex> lock(this->m_lockable_vector_tls_data);

					for (auto iter : m_vector_tls_data)
						iter->statistics.now.tls_stack_depth_threshold = 0;
				}
			}
			break;

	case	eFACTORY_GC_TYPE::TLS_POOLING:
			{
				this->m_statistics.now.existing_limits = this->m_statistics.now.tls_stack_depth;
			}
			break;

	case	eFACTORY_GC_TYPE::TLS_POOLING_NO_GC:
			{
				this->m_statistics.now.existing_limits = INT64_MAX;
				{
					// lock)
					std::lock_guard<std::mutex> lock(this->m_lockable_vector_tls_data);

					for (auto iter : m_vector_tls_data)
						iter->statistics.now.tls_stack_depth_threshold = INT32_MAX;
				}
			}
			break;

	default:
			break;
	}
}

template <class T>
T* factory::object_pool_tls<T>::alloc(CGNEW_DEBUG_INFO_PARAMETERS)
{
	// declare) 
	T* pobject;

	// 1) get tls data
	auto ptls_data = this->_get_tls_data();

	// declare)
	auto& tls_stack = ptls_data->tls_stack;

	// 2.A) pop from tls_stack
	if (tls_stack != nullptr)
	{
		// statistics)
		ptls_data->statistics.now.statistics_on_alloc_stack_tls();

		// - pop tls_stacked object
		pobject = tls_stack;
		tls_stack = pobject->Next;
	}
	// 2.B) pop from tls_stack
	else
	{
		pobject = this->_process_pop_from_central_stack_or_create(ptls_data CGNEW_DEBUG_INFO_COMMA CGNEW_DEBUG_INFO_PARAMETERS_PASSING);
	}

	// debug)
	CGNEW_DEBUG_INFO_SET(pobject, _filename, _line);

	// 3) set null
	pobject->Next = nullptr;

	// 4) call 'process_on_pool_alloc'
	this->template process_on_alloc<T>(pobject);

	// return)
	return pobject;
}

template <class T>
T* factory::object_pool_tls<T>::_process_pop_from_central_stack_or_create(sTLS_DATA* _ptls_data CGNEW_DEBUG_INFO_COMMA CGNEW_DEBUG_INFO_PARAMETERS)
{
	// check) closed pool : no more tls caching in this thread
	this->_process_check_closed(_ptls_data);

	// declare)
	auto& temp_now = _ptls_data->statistics.now;
	const bool is_tls_pooling = (this->m_statistics.gc_type >= eFACTORY_GC_TYPE::TLS_POOLING) && (temp_now.tls_stack_depth_threshold >= 0);

	// 1) pop from central stack (TLS_POOLING : pop a batch(chain) at once)
	T* pobject_last;
	int count;
	auto pobject = this->m_stack_object.pop(is_tls_pooling ? this->_get_tls_batch() : 1, pobject_last, count);

	// 2.A) pop from central stack
	if (pobject != nullptr)
	{
		// - the rest of chain is stored to tls_stack
		if (count > 1)
		{
			pobject_last->Next = _ptls_data->tls_stack;
			_ptls_data->tls_stack = pobject->Next;
			temp_now.tls_stack_depth += (count - 1);
		}

		// statistics)
		temp_now.statistics_on_alloc_stack();
	}
	// 2.B) create new
	else
	{
		// - create new object
		pobject = this->template process_create_object<T>(CGNEW_DEBUG_INFO_PARAMETERS_PASSING);
		count = 1;

		// statistics)
		temp_now.statistics_on_alloc_create();
	}

	// 3) increase tls_stack_depth_threshold (as many as objects taken from central stack, saturated)
	if (is_tls_pooling)
	{
		const int32_t threshold = temp_now.tls_stack_depth_threshold;
		if (threshold < TLS_THRESHOLD_MAX)
			temp_now.tls_stack_depth_threshold = threshold + count;
	}

	// statistics) objects out of central stack (in using + tls stacked)
	auto count_central_using = this->_get_existing();

	// sustain)
	if (count_central_using > this->m_existing_term_peak)
	{
		if (this->m_statistics.gc_type != eFACTORY_GC_TYPE::NO_POOLING)
		{
			// - update m_existing_term_peak
			this->m_existing_term_peak = count_central_using;

			// - 사용 중인 개수가 최대할당치를 넘어설 경우 수정한다.
			if (count_central_using > this->m_statistics.now.existing_limits && this->m_statistics.now.existing_limits >= 0)
				this->m_statistics.now.existing_limits = static_cast<size_t>(((count_central_using * 9) >> 3));
		}
	}

	// return)
	return pobject;
}

template <class T>
void factory::object_pool_tls<T>::dealloc(T* _pobject) noexcept
{
	// check)
	CGASSERT_FATAL(_pobject != nullptr);

	// check)
	CGASSERT_FATAL(_pobject->reference_count() == 0);

	// check)
	CGASSERT_FATAL(_pobject->Next == nullptr);

	// 1) call 'process_on_pool_dealloc'
	this->template process_on_dealloc<T>(_pobject);

	// 2) get tls data
	auto ptls_data = this->_get_tls_data();

	// 3) get statistics.now
	auto& temp_now = ptls_data->statistics.now;

	// 4) get tls stack count 
	auto temp = temp_now.tls_stack_depth;

	// 5.A) push _object to tls stack
	if(temp < temp_now.tls_stack_depth_threshold)
	{
		// statistics)
		temp_now.tls_stack_depth = temp + 1;
		++temp_now.free;

		// - push to tls_stack
		{
			_pobject->Next = ptls_data->tls_stack;
			ptls_data->tls_stack = _pobject;
		}
	}
	// 5.B) push _object to central stack or delete _object
	else
	{
		this->_process_push_to_central_stack_or_delete(ptls_data, _pobject);
	}
}

template <class T>
void factory::object_pool_tls<T>::_process_push_to_central_stack_or_delete(sTLS_DATA* _ptls_data, T* _pobject)
{
	// check) closed pool : no more tls caching in this thread (tls stack is emptied at 3) below)
	this->_process_check_closed(_ptls_data);

	// declare)
	auto& temp_now = _ptls_data->statistics.now;

	// case) TLS_POOLING : keep '_pobject' in tls stack and move objects to central stack as a batch
	if (this->m_statistics.gc_type >= eFACTORY_GC_TYPE::TLS_POOLING && temp_now.tls_stack_depth_threshold >= 0)
	{
		// declare)
		const int32_t batch = this->_get_tls_batch();
		const int32_t depth = temp_now.tls_stack_depth;

		// - push to tls_stack
		_pobject->Next = _ptls_data->tls_stack;
		_ptls_data->tls_stack = _pobject;

		// statistics)
		temp_now.tls_stack_depth = depth + 1;
		++temp_now.free;

		// - minimum tls cache : keep up to 'batch' objects even if tls_stack_depth_threshold is less
		//   (a thread only freeing objects(ex. i/o thread) has tls_stack_depth_threshold 0)
		if (depth < batch)
			return;

		// - move 'batch' objects to central stack (one lock for 'batch' objects)
		this->_process_tls_redundancy(_ptls_data, batch);
		return;
	}

	// declare) hold this pool while deleting '_pobject'
	//  - deleting '_pobject' can release the last reference of this pool (ex. an object returned after the pool is closed and released at exit).
	//    then this pool and '_ptls_data' are deleted while they are still used below. so this pool is released at the end of this function.
	object_ptr<object_pool_tls<T>> phold_pool;

	// 1) push '_pobject' to central stack
	if (static_cast<int64_t>(this->m_stack_object.size()) < this->m_statistics.now.existing_limits)
	{
		// statistics)
		++_ptls_data->statistics.now.free;
		++_ptls_data->statistics.now.free_stack;

		// - enqueue
		this->m_stack_object.push(_pobject);

		// check) closed while pushing : delete objects in central stack
		if (this->m_statistics.now.existing_limits < 0)
			this->_process_purge_central_stack(_ptls_data);
	}
	// 2) delete '_pobject'
	else
	{
		// statistics)
		++_ptls_data->statistics.now.free;
		++_ptls_data->statistics.now.free_delete;

		// - hold this pool (released at the end of this function)
		phold_pool = this;

		// - destroy object
		this->template process_destroy_object<T>(_pobject);
		this->m_existing_total.fetch_sub(1, std::memory_order_relaxed);
	}

	// 3) get count of redundancy (this thread's tls_stack_depth_threshold)
	auto count_redundancy = temp_now.tls_stack_depth - temp_now.tls_stack_depth_threshold;

	// 4) process garbage collection
	if (count_redundancy > 0)
		this->_process_tls_redundancy(_ptls_data, count_redundancy);
}

template <class T>
void factory::object_pool_tls<T>::_process_tls_redundancy(sTLS_DATA* _ptls_data, int32_t _count_garbage)
{
	// check) 
	CGASSERT_ERROR(_count_garbage > 0);

	// declare)
	auto& temp_now = _ptls_data->statistics.now;

	// check) is closed? (closed pool or exiting thread) - move all objects of tls stack (closed pool deletes them : existing_limits is -1)
	if(temp_now.tls_stack_depth_threshold < 0)
		_count_garbage = INT32_MAX;

	// declare)
	const int32_t batch = this->_get_tls_batch();

	// declare)
	bool is_pushed = false;

	// declare) hold this pool while deleting objects (see _process_push_to_central_stack_or_delete())
	object_ptr<object_pool_tls<T>> phold_pool;

	// 1) move garbages to central stack chain by chain ('batch' objects per chain)
	while (_count_garbage > 0 && _ptls_data->tls_stack != nullptr)
	{
		// - get first garbage object from tls_stack
		auto pobject_first = _ptls_data->tls_stack;

		// - set last garbage object
		auto pobject_last = pobject_first;
		int32_t count = 1;
		const int32_t count_chain = (_count_garbage < batch) ? _count_garbage : batch;

		// - detach from tls_stack
		while(count < count_chain && pobject_last->Next != nullptr)
		{
			pobject_last = pobject_last->Next;
			++count;
		}
		_ptls_data->tls_stack = pobject_last->Next;

		// statistics)
		temp_now.tls_stack_depth -= count;
		_count_garbage -= count;

		// - push garbages to central stack (only if central stack doesn't exceed existing_limits)
		const auto existing_limits = this->m_statistics.now.existing_limits;

		if (this->m_stack_object.push(pobject_first, pobject_last, count, existing_limits))
		{
			is_pushed = true;

			// statistics)
			temp_now.free_stack += count;
			continue;
		}

		// - get count to delete (central stack can't exceed existing_limits)
		const auto count_room = existing_limits - static_cast<int64_t>(this->m_stack_object.size());
		const int32_t count_delete = (count_room >= count) ? 0 : ((count_room <= 0) ? count : count - static_cast<int32_t>(count_room));

		// - delete exceeded objects (hold this pool first : released at the end of this function)
		if (count_delete > 0 && phold_pool.empty())
			phold_pool = this;

		for (int32_t i = 0; i < count_delete; ++i)
		{
			auto pobject = pobject_first;
			pobject_first = pobject->Next;
			this->template process_destroy_object<T>(pobject);
		}

		// - push rest to central stack
		if (count_delete < count)
		{
			this->m_stack_object.push(pobject_first, pobject_last, count - count_delete);
			is_pushed = true;
		}

		// statistics)
		this->m_existing_total.fetch_sub(count_delete, std::memory_order_relaxed);
		temp_now.free_stack += (count - count_delete);
		temp_now.free_delete += count_delete;
	}

	// check) closed while pushing : delete objects in central stack
	if (is_pushed && this->m_statistics.now.existing_limits < 0)
		this->_process_purge_central_stack(_ptls_data);
}

template <class T>
int32_t factory::object_pool_tls<T>::_get_tls_batch() noexcept
{
	// 1) already calculated
	auto batch = this->m_tls_batch.load(std::memory_order_relaxed);
	if (batch != 0)
		return batch;

	// 2) calculate (object_size() is virtual - can't be calculated in the constructor (ex. memory block pool))
	auto size = this->object_size();
	if (size == 0)
		size = 1;

	auto count = TLS_BATCH_BYTES / size;
	batch = (count < 1) ? 1 : ((count > static_cast<size_t>(TLS_BATCH_MAX)) ? TLS_BATCH_MAX : static_cast<int32_t>(count));

	// 3) store (chain size of central stack = batch)
	this->m_stack_object.set_chain_size(batch);
	this->m_tls_batch.store(batch, std::memory_order_relaxed);

	// return)
	return batch;
}


}