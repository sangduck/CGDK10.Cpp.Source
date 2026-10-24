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
//
// CGDK::send_defer
//
// 1. While a thread holds a group lock (group_lock_object : the group lock and the member lock
//    of the group classes), the sends of the thread are not sent at once.
//    The sendable queues them, and they are sent when the outermost group lock is released.
//    (no send system call is made in the group lock : the group lock is held shorter.
//     the sends to the same socket in the lock are sent at once)
// 2. The order is kept.
//    - the sends to a socket are sent in the order they were requested.
//      (the sends of other threads to the socket while it is deferred are queued after them)
// 3. They are sent earlier
//    - when the socket is closed (closesocket) : the deferred sends are sent before it is closed.
//      (even if another thread deferred them)
//    - when the thread blocks (scoped_thread_block) : sync queries (redis, http...) send at once.
//    - when many sends are queued to a socket (the sendable decides)
//    - send_defer::flush()
// 4. An exception leaving the group lock releases the lock => they are sent.
// 5. The deferring sendable : net::io::sendable::Ntcp_async_gather (the others send at once)
// 6. send_defer::scope defers the sends without a group lock.
//    send_defer::enable(false) stops deferring in the group lock.
// 7. Do not wait for a response of a deferred request in the group lock.
//    (call send_defer::flush() before waiting, or wait in a scoped_thread_block)
//
//-----------------------------------------------------------------------------
class Isend_deferrable
{
public:
	virtual ~Isend_deferrable() noexcept {}

	//! @brief sends what the calling thread deferred. (does nothing if it was sent already or another thread took it over)
	virtual	void				process_flush_deferred_send() noexcept PURE;
};

class send_defer
{
public:
	//! @brief defers the sends of this thread in the scope (like a group lock)
	class scope
	{
	public:
		scope() noexcept { send_defer::enter(); }
		~scope() noexcept { send_defer::leave(); }
		scope(const scope&) = delete;
		scope& operator=(const scope&) = delete;
	};

public:
	//! @brief is this thread deferring the sends to _psocket?
	[[nodiscard]] static bool	is_deferring(const void* _psocket) noexcept
	{
		return t_count_suspend == 0 && ((t_depth != 0 && s_enable.load(std::memory_order_relaxed)) || (_psocket != nullptr && t_psocket_cork == _psocket));
	}
	//! @brief the token of this thread (the sendable keeps it as the owner of the deferred sends)
	[[nodiscard]] static const void* token() noexcept { return &t_depth; }

	//! @brief registers a sendable at its first deferred send. (process_flush_deferred_send() is called when flushed) @return false failed (send at once)
	[[nodiscard]] static bool	push(Isend_deferrable* _psendable, object_ptr<Ireferenceable>&& _phold) noexcept
	{
		try
		{
			t_list.push_back(ENTRY{ _psendable, std::move(_phold) });
		}
		catch (...)
		{
			return false;
		}

		++t_count;

		return true;
	}

	//! @brief sends all deferred sends of this thread now.
	static	void				flush() noexcept { if (t_count != 0) _flush(0); }

	//! @brief enables deferring in the group lock. (default enabled)
	static	void				enable(bool _enable = true) noexcept { s_enable.store(_enable, std::memory_order_relaxed); }
	[[nodiscard]] static bool	is_enabled() noexcept { return s_enable.load(std::memory_order_relaxed); }

	//! @brief the depth of the group locks (and scopes) this thread holds
	[[nodiscard]] static std::size_t depth() noexcept { return t_depth; }
	//! @brief the count of the sendables this thread deferred
	[[nodiscard]] static std::size_t count() noexcept { return t_count; }

	// - group lock (group_lock_object, scope)
	static	void				enter() noexcept { if (t_depth++ == 0) t_index_scope = t_count; }
	static	void				leave() noexcept { CGASSERT_ERROR(t_depth != 0); if (--t_depth == 0 && t_count > t_index_scope) _flush(t_index_scope); }

	// - thread block (scoped_thread_block) : sends all and does not defer until resumed
	static	void				suspend() noexcept { ++t_count_suspend; flush(); }
	static	void				resume() noexcept { CGASSERT_ERROR(t_count_suspend != 0); --t_count_suspend; }

	// - send cork : the sends to the receiving socket are deferred while the received messages are processed (net::io::send_cork : epoll)
	//   (the deferred sends of the cork socket are sent at the end of the cork. not at the end of a group lock in it)
	[[nodiscard]] static const void* cork_socket() noexcept { return t_psocket_cork; }
	[[nodiscard]] static const void* begin_cork(const void* _psocket) noexcept
	{
		const auto psocket_before = t_psocket_cork;

		// - the deferred sends of an outer cork are sent first
		if (t_depth == 0)
			flush();

		t_psocket_cork = _psocket;

		return psocket_before;
	}
	static	void				end_cork(const void* _psocket_before) noexcept
	{
		t_psocket_cork = _psocket_before;

		// - in a group lock => sent at the end of the group lock. (all of them)
		if (t_depth != 0)
			t_index_scope = 0;
		else
			flush();
	}

private:
	struct ENTRY
	{
		Isend_deferrable*			psendable;
		object_ptr<Ireferenceable>	phold;
	};

	static	void				_flush(std::size_t _index_from) noexcept
	{
		// 1) sends (the list can grow while sending : the sends deferred while sending are sent too)
		for (std::size_t i = _index_from; i < t_list.size(); ++i)
		{
			// - the iterator is not kept over the call (the list can grow)
			auto iter = t_list.begin() + static_cast<std::ptrdiff_t>(i);
			auto psendable = iter->psendable;

			if (psendable == nullptr)
				continue;

			iter->psendable = nullptr;

			psendable->process_flush_deferred_send();
		}

		// 2) releases from the back. (released out of the list : a release can defer or flush again)
		while (t_list.size() > _index_from)
		{
			auto& entry = t_list.back();

			// - deferred while releasing => sends it first
			if (entry.psendable != nullptr)
			{
				auto psendable = entry.psendable;
				entry.psendable = nullptr;
				psendable->process_flush_deferred_send();
				continue;
			}

			auto phold = std::move(entry.phold);

			t_list.pop_back();
			--t_count;
		}
	}

	// - the hot path (lock/unlock) reads only trivial thread_locals
	static inline thread_local std::size_t t_depth = 0;
	static inline thread_local std::size_t t_count = 0;
	static inline thread_local std::size_t t_index_scope = 0;
	static inline thread_local std::size_t t_count_suspend = 0;
	static inline thread_local const void* t_psocket_cork = nullptr;
	static inline thread_local std::vector<ENTRY> t_list;
	static inline std::atomic<bool> s_enable{ true };
};

//-----------------------------------------------------------------------------
//
// CGDK::group_lock_object
//
// - the lock_object of the group lock and the member lock of the group classes.
//   the sends in it are deferred until the outermost one is released. (send_defer)
// - std::lock_guard, std::unique_lock (CTAD), SCOPED_LOCK and lock()/unlock() defer.
//   locked as lock_object (ex. std::unique_lock<lock_object>) it is an ordinary lock_object. (sent at once)
//
//-----------------------------------------------------------------------------
class group_lock_object : public lock_object
{
public:
			void				lock() { lock_object::lock(); send_defer::enter(); }
	[[nodiscard]] bool			try_lock() { if (lock_object::try_lock() == false) return false; send_defer::enter(); return true; }
			void				unlock() noexcept { lock_object::unlock(); send_defer::leave(); }
};

// - SCOPED_LOCK (the partial specialization for std::recursive_mutex would lock it as lock_object)
template <> class _unique_lock<group_lock_object, void> { public: using type = std::unique_lock<group_lock_object>; };
template <> class _unique_lock_named<group_lock_object, void> { public: using type = std::unique_lock<group_lock_object>; };


}
