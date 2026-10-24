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
// 8. auto (send_defer::auto_scope : the executor opens it around each executable it runs. linux)
//    - out of a group lock and a send cork, the consecutive sends of the thread to one socket are deferred.
//      they are sent when the thread sends to another socket, or when the executable returns.
//      (a tick sending many messages to each socket : one send system call per socket instead of one per message)
//    - the other flushes above apply too (close, scoped_thread_block, the limits of the sendable, send_defer::flush())
//    - send_defer::enable_auto(false) stops it. a thread out of an executor (std::thread) does not defer : use send_defer::scope.
// 9. batch (send_defer::begin_batch / end_batch : the io_uring completion executor opens it around a batch of completions. linux)
//    - the sends of the handlers of the batch are deferred like in a group lock and sent at its end (the replies to the completions
//      of a socket in a batch : one send). the age of a send cork is checked in it (is_cork_age_checked), a group lock in it sends
//      its sends at its release like out of a batch, send_defer::enable(false) does not stop it.
//    - the executor sends what was deferred when the batch ran for a while (before the next completion) : a long handler holds the
//      sends of the others in the batch about that long.
//    - the auto (8) does not work in a batch : the batch defers all the sends of it.
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

	//! @brief defers the consecutive sends of this thread to one socket in the scope (opened by the executor around an executable)
	class auto_scope
	{
	public:
		auto_scope() noexcept { send_defer::enter_auto(); }
		~auto_scope() noexcept { send_defer::leave_auto(); }
		auto_scope(const auto_scope&) = delete;
		auto_scope& operator=(const auto_scope&) = delete;
	};

public:
	//! @brief is this thread deferring the sends to _psocket? (the sendable calls it at every send, and push() right after it)
	//!        auto : a send to another socket than the last one sends what was deferred to the last one first
	[[nodiscard]] static bool	is_deferring(const void* _psocket) noexcept
	{
		t_is_auto_push = false;

		if (t_count_suspend != 0)
			return false;

		if (t_depth != 0 && (t_depth_batch != 0 || s_enable.load(std::memory_order_relaxed)))
			return true;

		if (_psocket == nullptr)
			return false;

		if (t_psocket_cork == _psocket)
			return true;

		// - auto
		if (t_depth_auto == 0 || s_enable_auto.load(std::memory_order_relaxed) == false)
			return false;

		if (t_psocket_auto != _psocket)
		{
			_flush_auto();
			t_psocket_auto = _psocket;
		}

		t_is_auto_push = true;

		return true;
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
			t_is_auto_push = false;
			return false;
		}

		++t_count;

		// - auto : the entry of the socket deferred now (sent when the thread sends to another socket)
		if (t_is_auto_push)
		{
			t_index_auto = t_list.size() - 1;
			t_is_auto_push = false;
		}

		return true;
	}

	//! @brief sends all deferred sends of this thread now.
	static	void				flush() noexcept { if (t_count != 0) _flush(0); }

	//! @brief enables deferring in the group lock. (default enabled)
	static	void				enable(bool _enable = true) noexcept { s_enable.store(_enable, std::memory_order_relaxed); }
	[[nodiscard]] static bool	is_enabled() noexcept { return s_enable.load(std::memory_order_relaxed); }
	//! @brief enables the auto deferring in the executables run by an executor. (default enabled)
	static	void				enable_auto(bool _enable = true) noexcept { s_enable_auto.store(_enable, std::memory_order_relaxed); }
	[[nodiscard]] static bool	is_auto_enabled() noexcept { return s_enable_auto.load(std::memory_order_relaxed); }

	// - auto (auto_scope) : the sends of the last socket are sent at the end. the list is released at the end of the outermost one
	static	void				enter_auto() noexcept { _flush_auto(); t_psocket_auto = nullptr; ++t_depth_auto; }
	static	void				leave_auto() noexcept
	{
		CGASSERT_ERROR(t_depth_auto != 0);

		_flush_auto();
		t_psocket_auto = nullptr;

		if (--t_depth_auto == 0 && t_depth == 0 && t_psocket_cork == nullptr && t_count != 0)
			_flush(0);
	}

	//! @brief the depth of the group locks (and scopes) this thread holds
	[[nodiscard]] static std::size_t depth() noexcept { return t_depth; }
	//! @brief the count of the sendables this thread deferred
	[[nodiscard]] static std::size_t count() noexcept { return t_count; }

	// - group lock (group_lock_object, scope)
	static	void				enter() noexcept { if (t_depth++ == t_depth_batch) t_index_scope = t_count; }
	static	void				leave() noexcept { CGASSERT_ERROR(t_depth > t_depth_batch); if (--t_depth == t_depth_batch && t_count > t_index_scope) _flush(t_index_scope); }

	// - thread block (scoped_thread_block) : sends all and does not defer until resumed
	static	void				suspend() noexcept { ++t_count_suspend; flush(); }
	static	void				resume() noexcept { CGASSERT_ERROR(t_count_suspend != 0); --t_count_suspend; }

	// - batch (the io_uring completion executor around a batch of completions) : false = not opened (in a group lock, a cork or a thread
	//   block : a handler pumping the executor). is_cork_age_checked() : no group lock held (a batch or none : the age of a cork is checked)
	[[nodiscard]] static bool	begin_batch() noexcept { if (t_depth != 0 || t_count_suspend != 0 || t_psocket_cork != nullptr) return false; t_depth = t_depth_batch = 1; return true; }
	static	void				end_batch() noexcept { CGASSERT_ERROR(t_depth == 1 && t_depth_batch == 1); t_depth = t_depth_batch = 0; flush(); }
	[[nodiscard]] static bool	is_cork_age_checked() noexcept { return t_depth == t_depth_batch; }

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
		if (t_depth > t_depth_batch)
			t_index_scope = 0;
		// - in a batch => sent at the end of the batch
		else if (t_depth == 0)
			flush();
	}

private:
	struct ENTRY
	{
		Isend_deferrable*			psendable;
		object_ptr<Ireferenceable>	phold;
	};

	// - auto : sends what this thread deferred to the last socket (its entry stays in the list till _flush releases it)
	static	void				_flush_auto() noexcept
	{
		const auto index = t_index_auto;

		t_index_auto = NO_INDEX;

		if (index >= t_list.size())
			return;

		auto psendable = t_list[index].psendable;

		if (psendable == nullptr)
			return;

		t_list[index].psendable = nullptr;

		psendable->process_flush_deferred_send();
	}

	static	void				_flush(std::size_t _index_from) noexcept
	{
		// - the entries from _index_from are sent and released : the auto entry among them too
		if (t_index_auto != NO_INDEX && t_index_auto >= _index_from)
			t_index_auto = NO_INDEX;

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
	static inline thread_local std::size_t t_depth_batch = 0;			// - the depth opened by a batch (begin_batch : 1. 0 : none)
	static inline thread_local const void* t_psocket_cork = nullptr;
	static inline thread_local std::vector<ENTRY> t_list;
	static inline std::atomic<bool> s_enable{ true };

	// - auto
	static constexpr std::size_t NO_INDEX = static_cast<std::size_t>(-1);
	static inline thread_local std::size_t t_depth_auto = 0;
	static inline thread_local const void* t_psocket_auto = nullptr;	// - the socket the thread sent to last (in an auto_scope)
	static inline thread_local std::size_t t_index_auto = NO_INDEX;		// - the entry of t_list deferred to t_psocket_auto
	static inline thread_local bool t_is_auto_push = false;				// - is_deferring() returned true for the auto : the next push() is the entry of t_psocket_auto
	static inline std::atomic<bool> s_enable_auto{ true };
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
