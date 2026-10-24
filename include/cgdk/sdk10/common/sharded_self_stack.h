//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                                                                           *
//*                      sharded intrusive free stack                         *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
//*                (c) 2003. Cho Sanghyun. All right reserved.                *
//*                                                                           *
//*****************************************************************************

#pragma once

//-----------------------------------------------------------------------------
//
// sharded_self_stack<TDATA>
//
//  - an intrusive free stack (TDATA->Next) split into N independently locked
//    shards. the object pool's central stack was a single-mutex stack, and it
//    was the bottleneck when many threads allocate while one thread (the
//    consumer) frees : every thread contended on one mutex.
//  - each thread uses its own shard (stable, assigned on first use), so push /
//    pop normally touch a shard that few other threads touch. pop scans the
//    other shards only when its own shard is empty (so nothing is stranded).
//  - same interface as locked_self_stack. an occasional spurious empty pop only
//    makes the pool create one more object (bounded by existing_limits + GC),
//    never corrupts.
//
//-----------------------------------------------------------------------------
#include <atomic>
#include <mutex>
#include <new>

namespace CGDK
{

template <class TDATA>
class sharded_self_stack
{
public:
	using value_type = TDATA;
	using size_type = size_t;
	using _mytype = sharded_self_stack<TDATA>;

	template <class TOTHER_DATA>
	struct rebind { using other = sharded_self_stack<TOTHER_DATA>; };

	// - number of shards (power of two)
	static constexpr size_t SHARD_COUNT = 16;
	static constexpr size_t SHARD_MASK = SHARD_COUNT - 1;

public:
			sharded_self_stack() noexcept = default;
			~sharded_self_stack() noexcept = default;

			size_t				size() const noexcept;
			void				clear() noexcept;
			void				push(TDATA _pdata) noexcept;
			void				push(TDATA _pfirst, TDATA _plast, int _count) noexcept;
			TDATA				pop() noexcept;
			TDATA				pop_all() noexcept;

private:
	// - one shard : its own lock, head and depth (padded to avoid false sharing)
	struct alignas(64) sSHARD
	{
		std::mutex			m_lockable;
		TDATA				m_phead = nullptr;
		std::atomic<int64_t> m_depth{ 0 };
	};

	// - the shard of the calling thread (assigned once, round-robin, stable)
	static size_t			_thread_shard() noexcept
	{
		static std::atomic<size_t> s_next{ 0 };
		thread_local const size_t t_index = s_next.fetch_add(1, std::memory_order_relaxed) & SHARD_MASK;
		return t_index;
	}

	sSHARD					m_shards[SHARD_COUNT];
};

template <class TDATA>
size_t sharded_self_stack<TDATA>::size() const noexcept
{
	// - approximate total (each shard's depth is atomic, summed without a global lock)
	int64_t total = 0;
	for (size_t i = 0; i < SHARD_COUNT; ++i)
		total += this->m_shards[i].m_depth.load(std::memory_order_relaxed);
	return (total > 0) ? static_cast<size_t>(total) : 0;
}

template <class TDATA>
void sharded_self_stack<TDATA>::clear() noexcept
{
	this->pop_all();
}

template <class TDATA>
void sharded_self_stack<TDATA>::push(TDATA _pdata) noexcept
{
	auto& shard = this->m_shards[_thread_shard()];

	std::lock_guard<std::mutex> lock(shard.m_lockable);

	_pdata->Next = shard.m_phead;
	shard.m_phead = _pdata;
	shard.m_depth.store(shard.m_depth.load(std::memory_order_relaxed) + 1, std::memory_order_relaxed);
}

template <class TDATA>
void sharded_self_stack<TDATA>::push(TDATA _pfirst, TDATA _plast, int _count) noexcept
{
	auto& shard = this->m_shards[_thread_shard()];

	std::lock_guard<std::mutex> lock(shard.m_lockable);

	_plast->Next = shard.m_phead;
	shard.m_phead = _pfirst;
	shard.m_depth.store(shard.m_depth.load(std::memory_order_relaxed) + _count, std::memory_order_relaxed);
}

template <class TDATA>
TDATA sharded_self_stack<TDATA>::pop() noexcept
{
	const size_t home = _thread_shard();

	// 1) the thread's own shard first (the common case : little contention)
	{
		auto& shard = this->m_shards[home];
		std::lock_guard<std::mutex> lock(shard.m_lockable);
		if (shard.m_phead != nullptr)
		{
			auto data = shard.m_phead;
			shard.m_phead = data->Next;
			shard.m_depth.store(shard.m_depth.load(std::memory_order_relaxed) - 1, std::memory_order_relaxed);
			data->Next = nullptr;
			return data;
		}
	}

	// 2) empty : take from another shard so nothing is stranded
	for (size_t i = 1; i < SHARD_COUNT; ++i)
	{
		auto& shard = this->m_shards[(home + i) & SHARD_MASK];

		// - a quick unlocked peek avoids locking obviously-empty shards
		if (shard.m_depth.load(std::memory_order_relaxed) <= 0)
			continue;

		std::lock_guard<std::mutex> lock(shard.m_lockable);
		if (shard.m_phead != nullptr)
		{
			auto data = shard.m_phead;
			shard.m_phead = data->Next;
			shard.m_depth.store(shard.m_depth.load(std::memory_order_relaxed) - 1, std::memory_order_relaxed);
			data->Next = nullptr;
			return data;
		}
	}

	// return) all shards empty
	return nullptr;
}

template <class TDATA>
TDATA sharded_self_stack<TDATA>::pop_all() noexcept
{
	// - drain every shard and concatenate into one chain (used by GC / shutdown, not a hot path)
	TDATA head = nullptr;
	TDATA tail = nullptr;

	for (size_t i = 0; i < SHARD_COUNT; ++i)
	{
		auto& shard = this->m_shards[i];

		TDATA chain;
		{
			std::lock_guard<std::mutex> lock(shard.m_lockable);
			chain = shard.m_phead;
			shard.m_phead = nullptr;
			shard.m_depth.store(0, std::memory_order_relaxed);
		}

		if (chain == nullptr)
			continue;

		// - find the end of this shard's chain
		TDATA chain_tail = chain;
		while (chain_tail->Next != nullptr)
			chain_tail = chain_tail->Next;

		// - append
		if (head == nullptr)
			head = chain;
		else
			tail->Next = chain;
		tail = chain_tail;
	}

	return head;
}

}
