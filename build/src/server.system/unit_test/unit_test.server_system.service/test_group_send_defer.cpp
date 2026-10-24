#include "pch.h"
#include "cgdk/sdk10/server.group.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

// ----------------------------------------------------------------------------
// group lock stage 2 : the sends in a group lock are deferred until the outermost group lock is released
//
// - defer_member follows the protocol of the deferring sendable (net::io::sendable::Ntcp_async_gather)
//   without a socket : the first deferred send registers it (send_defer::push), the sends of the owner thread
//   are queued, the sends of other threads are queued after them, process_flush_deferred_send() sends them.
//   "sent" = moved to m_vector_sent (a socket would make the system call here)
// ----------------------------------------------------------------------------
namespace CGDK
{
namespace test_group_send_defer
{

static shared_buffer make_seq(uint32_t _seq)
{
	auto buf = alloc_shared_buffer(sizeof(uint32_t));
	buf.append<uint32_t>(_seq);
	return buf;
}

class defer_member :
	public						Igroupable<defer_member>,
	public						net::io::Isender,
	public						Isend_deferrable
{
public:
	virtual	bool				process_send(shared_buffer&& _buffer, std::size_t /*_count_message*/, uint64_t /*_option*/) override
	{
		const auto seq = *reinterpret_cast<const uint32_t*>(_buffer.data());
		const auto ptoken = send_defer::token();
		bool is_first = false;

		{
			std::lock_guard lock(this->m_lockable_send);

			// - the owner thread
			if (this->m_powner == ptoken)
			{
				this->m_vector_queued.push_back(seq);
				return true;
			}

			// - another thread is deferring => queued after them
			if (this->m_powner != nullptr)
			{
				this->m_vector_queued.push_back(seq);
				return true;
			}

			// - send at once
			if (send_defer::is_deferring(this) == false)
			{
				this->m_vector_sent.push_back(seq);
				++this->m_count_sent_at_once;
				return true;
			}

			// - first deferred send
			this->m_powner = ptoken;
			this->m_vector_queued.push_back(seq);
			is_first = true;
		}

		if (is_first && send_defer::push(this, object_ptr<Ireferenceable>(this)) == false)
			this->process_flush_deferred_send();

		return true;
	}

	virtual	void				process_flush_deferred_send() noexcept override
	{
		std::function<void ()> on_flush;

		{
			std::lock_guard lock(this->m_lockable_send);

			if (this->m_powner != send_defer::token())
				return;

			this->m_powner = nullptr;
			++this->m_count_flush;
			this->m_vector_sent.insert(this->m_vector_sent.end(), this->m_vector_queued.begin(), this->m_vector_queued.end());
			this->m_vector_queued.clear();

			on_flush = this->m_on_flush;
		}

		if (on_flush)
			on_flush();
	}

	// - like closesocket : takes over the deferred sends of any thread
	void						flush_on_close()
	{
		std::lock_guard lock(this->m_lockable_send);

		if (this->m_powner == nullptr)
			return;

		this->m_powner = nullptr;
		this->m_vector_sent.insert(this->m_vector_sent.end(), this->m_vector_queued.begin(), this->m_vector_queued.end());
		this->m_vector_queued.clear();
	}

	std::vector<uint32_t>		sent() const { std::lock_guard lock(this->m_lockable_send); return this->m_vector_sent; }
	std::size_t					count_sent() const { std::lock_guard lock(this->m_lockable_send); return this->m_vector_sent.size(); }
	bool						is_deferred() const { std::lock_guard lock(this->m_lockable_send); return this->m_powner != nullptr; }

public:
	mutable	std::mutex			m_lockable_send;
			const void*			m_powner{ nullptr };
			std::vector<uint32_t> m_vector_queued;
			std::vector<uint32_t> m_vector_sent;
			std::size_t			m_count_flush{ 0 };
			std::size_t			m_count_sent_at_once{ 0 };
			std::function<void ()> m_on_flush;
};

// - broadcasts in the member events (stage 1 : list_sender sends out of its container lock, but in the lock of process_enter/process_leave)
class group_broadcast : public group::list_sender<defer_member>
{
public:
	using group::list_sender<defer_member>::list_sender;

public:
			std::function<void (defer_member*)> m_on_entered;
			std::function<void (defer_member*)> m_on_leaved;

protected:
	virtual	void				on_member_entered(defer_member* _pmember, sMESSAGE& /*_param*/) override { if (this->m_on_entered) this->m_on_entered(_pmember); }
	virtual	void				on_member_leaved(defer_member* _pmember, uintptr_t /*_result*/) override { if (this->m_on_leaved) this->m_on_leaved(_pmember); }
};

static object_ptr<group_broadcast> make_group()
{
	auto pgroup = make_object<group_broadcast>();
	pgroup->enable_member_enter(true);
	return pgroup;
}

static std::vector<object_ptr<defer_member>> make_members(std::size_t _count)
{
	std::vector<object_ptr<defer_member>> vector_member;
	for (std::size_t i = 0; i < _count; ++i)
		vector_member.push_back(make_object<defer_member>());
	return vector_member;
}

static std::vector<uint32_t> seqs(std::initializer_list<uint32_t> _list) { return std::vector<uint32_t>(_list); }

// ----------------------------------------------------------------------------
// the lock wrapper
// ----------------------------------------------------------------------------
TEST(group_send_defer, group_lock_object_is_used)
{
	static_assert(std::is_same_v<std::decay_t<decltype(std::declval<group_broadcast&>().get_group_lockable())>, group_lock_object>);
	static_assert(std::is_same_v<std::decay_t<decltype(std::declval<defer_member&>().m_lockable_group)>, group_lock_object>);
	static_assert(std::is_same_v<CGDK::_unique_lock<group_lock_object>::type, std::unique_lock<group_lock_object>>);
	static_assert(std::is_same_v<CGDK::_unique_lock_named<group_lock_object>::type, std::unique_lock<group_lock_object>>);
	static_assert(std::is_base_of_v<lock_object, group_lock_object>);
}

TEST(group_send_defer, depth_counts_group_and_member_locks)
{
	auto pgroup = make_group();
	auto vector_member = make_members(1);

	EXPECT_EQ(send_defer::depth(), 0u);

	SCOPED_LOCK(pgroup->get_group_lockable())
	{
		EXPECT_EQ(send_defer::depth(), 1u);

		// - recursive
		SCOPED_LOCK(pgroup->get_group_lockable())
		{
			EXPECT_EQ(send_defer::depth(), 2u);
		}

		// - member lock
		{
			std::lock_guard lock(vector_member[0]->m_lockable_group);
			EXPECT_EQ(send_defer::depth(), 2u);
		}

		// - SCOPED_LOCK_NAMED / SCOPED_LOCK_LEAVE
		SCOPED_LOCK_NAMED(pgroup->get_group_lockable(), lock_named)
		{
			EXPECT_EQ(send_defer::depth(), 2u);
			SCOPED_LOCK_LEAVE(lock_named);
			EXPECT_EQ(send_defer::depth(), 1u);
		}

		// - try_lock
		{
			std::unique_lock lock(pgroup->get_group_lockable(), std::try_to_lock);
			EXPECT_TRUE(lock.owns_lock());
			EXPECT_EQ(send_defer::depth(), 2u);
		}

		// - locked as lock_object : an ordinary lock (no depth)
		{
			std::unique_lock<lock_object> lock(vector_member[0]->m_lockable_group);
			EXPECT_EQ(send_defer::depth(), 1u);
		}

		EXPECT_EQ(send_defer::depth(), 1u);
	}

	// - lock_group()/unlock_group()
	pgroup->lock_group();
	EXPECT_EQ(send_defer::depth(), 1u);
	pgroup->unlock_group();

	EXPECT_EQ(send_defer::depth(), 0u);
}

// ----------------------------------------------------------------------------
// deferring
// ----------------------------------------------------------------------------
TEST(group_send_defer, sent_when_outermost_group_lock_released)
{
	auto pgroup = make_group();
	auto pgroup_inner = make_group();
	auto vector_member = make_members(3);

	// - out of a group lock : at once
	vector_member[0]->send(make_seq(1));
	EXPECT_EQ(vector_member[0]->sent(), seqs({ 1 }));

	SCOPED_LOCK(pgroup->get_group_lockable())
	{
		for (auto& iter : vector_member)
		{
			iter->send(make_seq(10));
			iter->send(make_seq(11));
		}

		// - nested group lock (another group)
		SCOPED_LOCK(pgroup_inner->get_group_lockable())
		{
			vector_member[2]->send(make_seq(12));
		}

		// - still deferred : nothing sent in the lock
		EXPECT_EQ(send_defer::count(), 3u);
		for (auto& iter : vector_member)
			EXPECT_TRUE(iter->is_deferred());
		EXPECT_EQ(vector_member[0]->sent(), seqs({ 1 }));
		EXPECT_EQ(vector_member[1]->count_sent(), 0u);
		EXPECT_EQ(vector_member[2]->count_sent(), 0u);
	}

	EXPECT_EQ(send_defer::count(), 0u);
	EXPECT_EQ(vector_member[0]->sent(), seqs({ 1, 10, 11 }));
	EXPECT_EQ(vector_member[1]->sent(), seqs({ 10, 11 }));
	EXPECT_EQ(vector_member[2]->sent(), seqs({ 10, 11, 12 }));

	// - one flush per member
	for (auto& iter : vector_member)
		EXPECT_EQ(iter->m_count_flush, 1u);
}

TEST(group_send_defer, broadcast_in_member_entered_and_leaved)
{
	auto pgroup = make_group();
	auto vector_member = make_members(4);

	std::size_t count_sent_in_event = 0;
	std::size_t depth_in_event = 0;

	const auto sum_sent = [&]
	{
		std::size_t sum = 0;
		for (auto& iter : vector_member)
			sum += iter->count_sent();
		return sum;
	};

	// - broadcasts in on_member_entered (the member lock and the group lock are held) : nothing is sent in the event
	pgroup->m_on_entered = [&](defer_member*)
	{
		const auto sum_before = sum_sent();
		pgroup->send(make_seq(100));
		depth_in_event = send_defer::depth();
		count_sent_in_event += sum_sent() - sum_before;
	};

	for (auto& iter : vector_member)
		pgroup->enter(iter.get());

	EXPECT_NE(depth_in_event, 0u);
	EXPECT_EQ(count_sent_in_event, 0u);
	EXPECT_EQ(send_defer::count(), 0u);

	// - member i received one per member entered after it (and itself)
	for (std::size_t i = 0; i < vector_member.size(); ++i)
		EXPECT_EQ(vector_member[i]->count_sent(), vector_member.size() - i) << "member " << i;

	// - broadcasts in on_member_leaved (the member lock is held)
	pgroup->m_on_entered = nullptr;
	pgroup->m_on_leaved = [&](defer_member*)
	{
		const auto sum_before = sum_sent();
		pgroup->send(make_seq(200));
		depth_in_event = send_defer::depth();
		count_sent_in_event += sum_sent() - sum_before;
	};

	std::vector<std::size_t> count_before;
	for (auto& iter : vector_member)
		count_before.push_back(iter->count_sent());

	depth_in_event = 0;
	count_sent_in_event = 0;

	vector_member[0]->leave_group();

	EXPECT_NE(depth_in_event, 0u);
	EXPECT_EQ(count_sent_in_event, 0u);	// - nothing was sent in the event
	EXPECT_EQ(vector_member[0]->count_sent(), count_before[0]);	// - left already
	for (std::size_t i = 1; i < vector_member.size(); ++i)
		EXPECT_EQ(vector_member[i]->count_sent(), count_before[i] + 1) << "member " << i;

	pgroup->m_on_leaved = nullptr;
	pgroup->leave_all();
}

TEST(group_send_defer, sent_when_exception_leaves_the_lock)
{
	auto pgroup = make_group();
	auto vector_member = make_members(2);

	try
	{
		SCOPED_LOCK(pgroup->get_group_lockable())
		{
			vector_member[0]->send(make_seq(1));
			vector_member[1]->send(make_seq(2));
			throw std::runtime_error("test");
		}
	}
	catch (const std::runtime_error&)
	{
	}

	EXPECT_EQ(send_defer::depth(), 0u);
	EXPECT_EQ(send_defer::count(), 0u);
	EXPECT_EQ(vector_member[0]->sent(), seqs({ 1 }));
	EXPECT_EQ(vector_member[1]->sent(), seqs({ 2 }));
}

TEST(group_send_defer, order_with_other_thread_send)
{
	auto pgroup = make_group();
	auto vector_member = make_members(1);
	auto pmember = vector_member[0];

	SCOPED_LOCK(pgroup->get_group_lockable())
	{
		pmember->send(make_seq(1));
		pmember->send(make_seq(2));

		// - another thread (not in a group lock) : queued after them
		std::thread([pmember] { pmember->send(make_seq(3)); }).join();

		pmember->send(make_seq(4));

		EXPECT_EQ(pmember->count_sent(), 0u);
	}

	EXPECT_EQ(pmember->sent(), seqs({ 1, 2, 3, 4 }));
}

TEST(group_send_defer, close_takes_over_deferred_sends_of_another_thread)
{
	auto vector_member = make_members(1);
	auto pmember = vector_member[0];

	std::mutex lockable;
	std::condition_variable cv;
	int step = 0;

	std::thread thread_defer([&]
	{
		send_defer::scope scope;

		pmember->send(make_seq(1));

		{ std::lock_guard lock(lockable); step = 1; }
		cv.notify_all();

		std::unique_lock lock(lockable);
		cv.wait(lock, [&] { return step == 2; });
	});

	{
		std::unique_lock lock(lockable);
		cv.wait(lock, [&] { return step == 1; });
	}

	EXPECT_EQ(pmember->count_sent(), 0u);

	// - closing : sends what another thread deferred
	pmember->flush_on_close();
	EXPECT_EQ(pmember->sent(), seqs({ 1 }));

	{ std::lock_guard lock(lockable); step = 2; }
	cv.notify_all();
	thread_defer.join();

	// - the flush of the deferring thread did nothing
	EXPECT_EQ(pmember->sent(), seqs({ 1 }));
	EXPECT_EQ(pmember->m_count_flush, 0u);
}

TEST(group_send_defer, flush_suspend_enable_scope)
{
	auto pgroup = make_group();
	auto vector_member = make_members(1);
	auto pmember = vector_member[0];

	SCOPED_LOCK(pgroup->get_group_lockable())
	{
		// - flush()
		pmember->send(make_seq(1));
		send_defer::flush();
		EXPECT_EQ(pmember->sent(), seqs({ 1 }));

		// - suspend (scoped_thread_block) : sends the deferred and sends at once until resumed
		pmember->send(make_seq(2));
		send_defer::suspend();
		EXPECT_EQ(pmember->sent(), seqs({ 1, 2 }));
		pmember->send(make_seq(3));
		EXPECT_EQ(pmember->sent(), seqs({ 1, 2, 3 }));
		send_defer::resume();

		pmember->send(make_seq(4));
		EXPECT_EQ(pmember->count_sent(), 3u);
	}
	EXPECT_EQ(pmember->sent(), seqs({ 1, 2, 3, 4 }));

	// - enable(false)
	send_defer::enable(false);
	SCOPED_LOCK(pgroup->get_group_lockable())
	{
		pmember->send(make_seq(5));
		EXPECT_EQ(pmember->count_sent(), 5u);
	}
	send_defer::enable(true);

	// - scope without a group lock
	{
		send_defer::scope scope;
		pmember->send(make_seq(6));
		EXPECT_EQ(pmember->count_sent(), 5u);
	}
	EXPECT_EQ(pmember->sent(), seqs({ 1, 2, 3, 4, 5, 6 }));
}

TEST(group_send_defer, condition_variable_any_wait_flushes)
{
	auto pgroup = make_group();
	auto vector_member = make_members(1);
	auto pmember = vector_member[0];

	std::condition_variable_any cv;

	std::unique_lock lock(pgroup->get_group_lockable());

	pmember->send(make_seq(1));
	EXPECT_EQ(pmember->count_sent(), 0u);

	// - the wait unlocks the group lock => flushed
	cv.wait_for(lock, std::chrono::milliseconds(1));

	EXPECT_EQ(pmember->sent(), seqs({ 1 }));
	EXPECT_EQ(send_defer::depth(), 1u);

	lock.unlock();
	EXPECT_EQ(send_defer::depth(), 0u);
}

TEST(group_send_defer, flush_reentrance)
{
	auto pgroup = make_group();
	auto vector_member = make_members(3);

	// - the flush of member 0 sends to member 1 in a group lock and to member 2 in a scope
	vector_member[0]->m_on_flush = [&]
	{
		SCOPED_LOCK(pgroup->get_group_lockable())
		{
			vector_member[1]->send(make_seq(2));
			EXPECT_EQ(vector_member[1]->count_sent(), 0u);
		}
		EXPECT_EQ(vector_member[1]->sent(), seqs({ 2 }));

		vector_member[2]->send(make_seq(3));
	};

	SCOPED_LOCK(pgroup->get_group_lockable())
	{
		vector_member[0]->send(make_seq(1));
	}

	EXPECT_EQ(vector_member[0]->sent(), seqs({ 1 }));
	EXPECT_EQ(vector_member[1]->sent(), seqs({ 2 }));
	EXPECT_EQ(vector_member[2]->sent(), seqs({ 3 }));
	EXPECT_EQ(vector_member[2]->m_count_sent_at_once, 1u);
	EXPECT_EQ(send_defer::count(), 0u);
}

TEST(group_send_defer, cork_scope_kept_over_group_lock)
{
	auto pgroup = make_group();
	auto vector_member = make_members(2);
	auto pcork = vector_member[0];
	auto pother = vector_member[1];

	// - cork : the sends to the receiving socket (pcork) are deferred while receiving
	const auto psocket_before = send_defer::begin_cork(pcork.get());

	pcork->send(make_seq(1));
	pother->send(make_seq(1));
	EXPECT_EQ(pcork->count_sent(), 0u);
	EXPECT_EQ(pother->count_sent(), 1u);

	SCOPED_LOCK(pgroup->get_group_lockable())
	{
		pcork->send(make_seq(2));
		pother->send(make_seq(2));
	}

	// - the group lock sent the sends deferred in it. the cork socket waits for the end of the cork
	EXPECT_EQ(pother->sent(), seqs({ 1, 2 }));
	EXPECT_EQ(pcork->count_sent(), 0u);

	send_defer::end_cork(psocket_before);

	EXPECT_EQ(pcork->sent(), seqs({ 1, 2 }));
	EXPECT_EQ(send_defer::count(), 0u);
}

TEST(group_send_defer, concurrent_broadcast)
{
	static constexpr std::size_t COUNT_THREAD = 8;
	static constexpr std::size_t COUNT_LOOP = 2000;

	auto pgroup = make_group();
	auto vector_member = make_members(16);

	for (auto& iter : vector_member)
		pgroup->enter(iter.get());

	std::vector<std::thread> vector_thread;
	for (std::size_t t = 0; t < COUNT_THREAD; ++t)
	{
		vector_thread.emplace_back([&, t]
		{
			for (std::size_t i = 0; i < COUNT_LOOP; ++i)
			{
				SCOPED_LOCK(pgroup->get_group_lockable())
				{
					pgroup->send(make_seq(static_cast<uint32_t>(t * COUNT_LOOP + i + 1)));
				}

				// - without a group lock
				vector_member[i % vector_member.size()]->send(make_seq(0));
			}
			EXPECT_EQ(send_defer::count(), 0u);
		});
	}

	for (auto& iter : vector_thread)
		iter.join();

	for (auto& iter : vector_member)
	{
		EXPECT_FALSE(iter->is_deferred());
		EXPECT_EQ(iter->count_sent(), COUNT_THREAD * COUNT_LOOP + COUNT_THREAD * COUNT_LOOP / vector_member.size());

		// - the broadcasts of a thread are sent in order
		std::vector<int64_t> last(COUNT_THREAD, -1);
		for (auto seq : iter->sent())
		{
			if (seq == 0)
				continue;
			const auto t = (seq - 1) / COUNT_LOOP;
			EXPECT_LT(last[t], static_cast<int64_t>(seq));
			last[t] = seq;
		}
	}

	pgroup->leave_all();
}

}
}
