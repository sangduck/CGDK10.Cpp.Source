#include "pch.h"
#include "cgdk/sdk10/server.group.h"
#include "cgdk/sdk10/server.observer.h"
#include "cgdk/sdk10/server.container.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

namespace CGDK
{
namespace test_group_leave_all
{

// ============================================================================
// test objects
// ============================================================================
// 1) member (group member + sender)
class member :
	public						Igroupable<member>,
	public						net::io::Isender
{
public:
	virtual	bool				process_send(shared_buffer&& /*_buffer*/, std::size_t _count_message, uint64_t /*_option*/) override { this->m_count_send += _count_message; return true; }

public:
			std::atomic<size_t>	m_count_send {0};
			int64_t				m_key {0};
};

// 2) observer (observable member + sender)
//    - process_send runs in the lock of the receiver (m_lockable_send) and calls m_on_send in it
class observer :
	public						Iobserver<observer>,
	public						net::io::Isender
{
public:
	virtual	bool				process_send(shared_buffer&& /*_buffer*/, std::size_t _count_message, uint64_t /*_option*/) override { std::lock_guard lock(this->m_lockable_send); this->m_count_send += _count_message; if (this->m_on_send) this->m_on_send(); return true; }

public:
			std::atomic<size_t>	m_count_send {0};
	mutable	lock_object			m_lockable_send;
			std::function<void ()> m_on_send;
};

// 3) sender (group_set / group_map member)
class sender : public net::io::Isender
{
public:
	virtual	bool				process_send(shared_buffer&& /*_buffer*/, std::size_t _count_message, uint64_t /*_option*/) override { std::lock_guard lock(this->m_lockable_send); this->m_count_send += _count_message; if (this->m_on_send) this->m_on_send(); return true; }

public:
			std::atomic<size_t>	m_count_send {0};
	mutable	lock_object			m_lockable_send;
			std::function<void ()> m_on_send;
};

// 4) Nmap needs the key of member
class nmap : public group::Nmap<member, int64_t>
{
public:
	using group::Nmap<member, int64_t>::Nmap;
protected:
	virtual	int64_t				process_get_member_key(member* _pmember) override { return _pmember->m_key; }
};

class nmap_sender : public group::Nmap_sender<member, int64_t>
{
public:
	using group::Nmap_sender<member, int64_t>::Nmap_sender;
protected:
	virtual	int64_t				process_get_member_key(member* _pmember) override { return _pmember->m_key; }
};

// 5) counts the member events of group
template <class TGROUP>
class counting : public TGROUP
{
public:
	using TGROUP::TGROUP;

public:
			std::atomic<int64_t> m_count_entered {0};
			std::atomic<int64_t> m_count_leaved {0};
			std::function<void (member*)> m_on_leaved;

protected:
	virtual	void				on_member_entered(member* /*_pmember*/, sMESSAGE& /*_param*/) override { ++this->m_count_entered; }
	virtual	void				on_member_leaved(member* _pmember, uintptr_t /*_result*/) override { ++this->m_count_leaved; if (this->m_on_leaved) this->m_on_leaved(_pmember); }
};

using group_list		 = counting<group::list<member>>;
using group_nmap		 = counting<nmap>;
using group_array		 = counting<group::array<member>>;
using group_array_seat	 = counting<group::array_seat<member>>;

// 6) enter functions (group::array has no enter(). members enter by member(_pmember, _seat))
inline void enter_member(group_list* _pgroup, member* _pmember, int64_t) { _pgroup->enter(_pmember); }
inline void enter_member(group_nmap* _pgroup, member* _pmember, int64_t) { _pgroup->enter(_pmember); }
inline void enter_member(group_array_seat* _pgroup, member* _pmember, int64_t) { _pgroup->enter(_pmember); }
inline void enter_member(group_array* _pgroup, member* _pmember, int64_t _seat) { sMESSAGE msg; _pgroup->member(_pmember, _seat, msg); }

template <class TGROUP>
object_ptr<TGROUP> make_group(size_t _capacity)
{
	auto pgroup = make_object<TGROUP>(_capacity);
	pgroup->enable_member_enter(true);
	return pgroup;
}

std::vector<object_ptr<member>> make_members(size_t _count)
{
	std::vector<object_ptr<member>> vector_member;
	for (size_t i = 0; i < _count; ++i)
	{
		auto pmember = make_object<member>();
		pmember->m_key = static_cast<int64_t>(i);
		vector_member.push_back(std::move(pmember));
	}
	return vector_member;
}

shared_buffer make_message()
{
	auto buf = alloc_shared_buffer(8);
	buf.append<uint32_t>(8);
	buf.append<uint32_t>(1);
	return buf;
}

// - runs _function. exits the process when it does not return in _timeout (deadlock)
template <class F>
void run_with_deadlock_watchdog(F&& _function, std::chrono::seconds _timeout)
{
	std::mutex lockable;
	std::condition_variable cv;
	bool is_done = false;

	std::thread thread_run([&]()
	{
		_function();
		{
			std::lock_guard lock(lockable);
			is_done = true;
		}
		cv.notify_all();
	});

	{
		std::unique_lock lock(lockable);
		if (cv.wait_for(lock, _timeout, [&]() { return is_done; }) == false)
		{
			std::fprintf(stderr, "[group_leave_all] deadlock detected (no progress in %d seconds)\n", static_cast<int>(_timeout.count()));
			std::fflush(stderr);
			std::_Exit(3);
		}
	}

	thread_run.join();
}

uint32_t next_random(uint32_t& _state) noexcept
{
	_state ^= _state << 13;
	_state ^= _state >> 17;
	_state ^= _state << 5;
	return _state;
}


// ============================================================================
// leave_all - basic
// ============================================================================
template <class TGROUP>
void check_leave_all_basic()
{
	const size_t COUNT = 8;
	auto pgroup = make_group<TGROUP>(COUNT);
	auto vector_member = make_members(COUNT);

	// - empty group
	pgroup->leave_all();
	EXPECT_EQ(pgroup->member_count(), 0u);

	// - enter all
	for (size_t i = 0; i < COUNT; ++i)
		enter_member(pgroup.get(), vector_member[i].get(), static_cast<int64_t>(i));
	EXPECT_EQ(pgroup->member_count(), COUNT);

	// - leave_all
	pgroup->leave_all();

	EXPECT_EQ(pgroup->member_count(), 0u);
	EXPECT_EQ(pgroup->m_count_entered.load(), static_cast<int64_t>(COUNT));
	EXPECT_EQ(pgroup->m_count_leaved.load(), static_cast<int64_t>(COUNT));
	for (auto& iter : vector_member)
		EXPECT_TRUE(iter->get_group().empty());
}

TEST(group_leave_all, list_basic)		{ check_leave_all_basic<group_list>(); }
TEST(group_leave_all, nmap_basic)		{ check_leave_all_basic<group_nmap>(); }
TEST(group_leave_all, array_basic)		{ check_leave_all_basic<group_array>(); }
TEST(group_leave_all, array_seat_basic)	{ check_leave_all_basic<group_array_seat>(); }

// - empty seats in front of / between members (leave_all looped forever on an empty seat)
TEST(group_leave_all, array_with_empty_seat)
{
	auto pgroup = make_group<group_array>(8);
	auto vector_member = make_members(2);

	enter_member(pgroup.get(), vector_member[0].get(), 1);
	enter_member(pgroup.get(), vector_member[1].get(), 5);
	ASSERT_EQ(pgroup->member_count(), 2u);

	run_with_deadlock_watchdog([&]() { pgroup->leave_all(); }, std::chrono::seconds(30));

	EXPECT_EQ(pgroup->member_count(), 0u);
	EXPECT_EQ(pgroup->m_count_leaved.load(), 2);
	EXPECT_TRUE(vector_member[0]->get_group().empty());
	EXPECT_TRUE(vector_member[1]->get_group().empty());
}

TEST(group_leave_all, array_seat_with_empty_seat)
{
	auto pgroup = make_group<group_array_seat>(8);
	auto vector_member = make_members(3);

	for (auto& iter : vector_member)
		pgroup->enter(iter.get());

	// - makes an empty seat between members
	vector_member[1]->leave_group();
	ASSERT_EQ(pgroup->member_count(), 2u);

	run_with_deadlock_watchdog([&]() { pgroup->leave_all(); }, std::chrono::seconds(30));

	EXPECT_EQ(pgroup->member_count(), 0u);
	EXPECT_EQ(pgroup->m_count_leaved.load(), 3);
	for (auto& iter : vector_member)
		EXPECT_TRUE(iter->get_group().empty());
}

// - a member moved to another group after leave_all took the member list must stay in that group
TEST(group_leave_all, member_moved_during_leave_all_stays)
{
	auto pgroup = make_group<group_list>(SIZE_MAX);
	auto pgroup_other = make_group<group_list>(SIZE_MAX);
	auto vector_member = make_members(2);

	pgroup->enter(vector_member[0].get());
	pgroup->enter(vector_member[1].get());

	// - while member[0] is leaving (leave_all is running), member[1] moves to the other group
	pgroup->m_on_leaved = [&](member* _pmember)
	{
		if (_pmember == vector_member[0].get())
			pgroup_other->enter(vector_member[1].get());
	};

	pgroup->leave_all();
	pgroup->m_on_leaved = nullptr;

	EXPECT_EQ(pgroup->member_count(), 0u);
	EXPECT_TRUE(vector_member[0]->get_group().empty());
	EXPECT_TRUE(vector_member[1]->is_member_of(pgroup_other.get()));
	EXPECT_EQ(pgroup_other->member_count(), 1u);

	pgroup_other->leave_all();
	EXPECT_TRUE(vector_member[1]->get_group().empty());
}


// ============================================================================
// leave_all - concurrency
//   thread 1    : leave_all (group a / group b)
//   thread 2, 3 : enter(group a) -> member->leave_group() (member lock -> group lock)
//   thread 4    : moves members between group a and group b
//   leave_all locked the group and then the members (group lock -> member lock)
//   so it deadlocked with the threads above.
// ============================================================================
template <class TGROUP>
void check_leave_all_concurrent()
{
	const size_t COUNT = 16;
	const auto DURATION = std::chrono::milliseconds(700);

	// - capacity == member count (group::array_seat is full when all members are in one group)
	auto pgroup_a = make_group<TGROUP>(COUNT);
	auto pgroup_b = make_group<TGROUP>(COUNT);
	auto vector_member = make_members(COUNT);

	std::atomic<bool> is_stop {false};
	std::atomic<int64_t> count_leave_all {0};
	std::atomic<int64_t> count_operation {0};
	std::atomic<int64_t> count_exception {0};

	run_with_deadlock_watchdog([&]()
	{
		std::vector<std::thread> vector_thread;

		// - an exception thrown in a worker thread is counted (not std::terminate)
		auto run_guarded = [&](auto _function)
		{
			return [&count_exception, _function]()
			{
				try { _function(); }
				catch (...) { ++count_exception; }
			};
		};

		// 1) leave_all
		vector_thread.emplace_back(run_guarded([&]()
		{
			while (is_stop.load(std::memory_order_relaxed) == false)
			{
				pgroup_a->leave_all();
				if ((++count_leave_all & 7) == 0)
					pgroup_b->leave_all();
			}
		}));

		// 2) enter / leave_group
		for (uint32_t t = 0; t < 2; ++t)
		{
			vector_thread.emplace_back(run_guarded([&, t]()
			{
				uint32_t state = 0x9E3779B9u + t;
				while (is_stop.load(std::memory_order_relaxed) == false)
				{
					auto index = next_random(state) % COUNT;
					auto* pmember = vector_member[index].get();
					enter_member(pgroup_a.get(), pmember, static_cast<int64_t>(index));
					pmember->leave_group();
					++count_operation;
				}
			}));
		}

		// 3) move between group a and group b
		vector_thread.emplace_back(run_guarded([&]()
		{
			uint32_t state = 0x85EBCA6Bu;
			while (is_stop.load(std::memory_order_relaxed) == false)
			{
				auto index = next_random(state) % COUNT;
				auto* pmember = vector_member[index].get();
				enter_member(pgroup_b.get(), pmember, static_cast<int64_t>(index));
				enter_member(pgroup_a.get(), pmember, static_cast<int64_t>(index));
				++count_operation;
			}
		}));

		std::this_thread::sleep_for(DURATION);
		is_stop = true;

		for (auto& iter : vector_thread)
			iter.join();
	}, std::chrono::seconds(60));

	// - every thread made progress
	EXPECT_GT(count_leave_all.load(), 0);
	EXPECT_GT(count_operation.load(), 0);
	EXPECT_EQ(count_exception.load(), 0);

	pgroup_a->leave_all();
	pgroup_b->leave_all();

	// check) all members left / entered == leaved
	EXPECT_EQ(pgroup_a->member_count(), 0u);
	EXPECT_EQ(pgroup_b->member_count(), 0u);
	EXPECT_EQ(pgroup_a->m_count_entered.load(), pgroup_a->m_count_leaved.load());
	EXPECT_EQ(pgroup_b->m_count_entered.load(), pgroup_b->m_count_leaved.load());
	for (auto& iter : vector_member)
		EXPECT_TRUE(iter->get_group().empty());
}

TEST(group_leave_all, list_concurrent)			{ check_leave_all_concurrent<group_list>(); }
TEST(group_leave_all, nmap_concurrent)			{ check_leave_all_concurrent<group_nmap>(); }
TEST(group_leave_all, array_concurrent)			{ check_leave_all_concurrent<group_array>(); }
TEST(group_leave_all, array_seat_concurrent)	{ check_leave_all_concurrent<group_array_seat>(); }


// ============================================================================
// lock order of replacing a member
//   Nmap::process_enter : the old member of the same key left in the group lock
//                         (member lock -> enter lock -> group lock -> old member lock)
//   array::member(seat) : the member on the seat left in the enter lock
//                         (member lock -> enter lock -> member lock on the seat)
//   so they deadlocked with leave_group() of the old member (member lock -> group lock),
//   with the members swapping their seats / groups (member lock -> enter lock).
// ============================================================================
// 1) records the member events of group in order
//    'D' on_member_duplicated, 'e' on_member_entering, 'E' on_member_entered, 'L' on_member_leaved
using EVENT = std::pair<char, member*>;

template <class TGROUP>
class event_logging : public TGROUP
{
public:
	using TGROUP::TGROUP;

public:
			std::vector<EVENT>	events() const { std::lock_guard lock(this->m_lockable_event); return this->m_vector_event; }
			void				clear_events() { std::lock_guard lock(this->m_lockable_event); this->m_vector_event.clear(); }

			std::atomic<bool>	m_is_logging {true};

protected:
			void				push_event(char _type, member* _pmember) { if (this->m_is_logging.load() == false) return; std::lock_guard lock(this->m_lockable_event); this->m_vector_event.emplace_back(_type, _pmember); }

	virtual	result_code			on_member_entering(member* _pmember, sMESSAGE& _param) override { this->push_event('e', _pmember); return TGROUP::on_member_entering(_pmember, _param); }
	virtual	void				on_member_entered(member* _pmember, sMESSAGE& _param) override { TGROUP::on_member_entered(_pmember, _param); this->push_event('E', _pmember); }
	virtual	void				on_member_leaved(member* _pmember, uintptr_t _result) override { TGROUP::on_member_leaved(_pmember, _result); this->push_event('L', _pmember); }

	mutable	std::mutex			m_lockable_event;
			std::vector<EVENT>	m_vector_event;
};

// 2) Nmap replacing the old member of the same key (on_member_duplicated returns SUCCESS)
class group_nmap_replace : public event_logging<group_nmap>
{
public:
	using event_logging<group_nmap>::event_logging;

public:
			std::atomic<int64_t> m_count_duplicated {0};
			std::function<void (member*, member*)> m_on_duplicated;

protected:
	virtual	result_code			on_member_duplicated(const int64_t& /*_key*/, member* _pmember_new, member* _pmember_old, sMESSAGE& /*_param*/) override
	{
		++this->m_count_duplicated;
		this->push_event('D', _pmember_new);
		if (this->m_on_duplicated)
			this->m_on_duplicated(_pmember_new, _pmember_old);
		return eRESULT::SUCCESS;
	}
};

using group_array_logging = event_logging<group_array>;

// 3) calls each function repeatedly on its own thread (in the deadlock watchdog)
//    returns the call count of each function. an exception is counted in _count_exception
std::vector<int64_t> run_threads_repeatedly(const std::vector<std::function<void ()>>& _vector_function, std::atomic<int64_t>& _count_exception)
{
	const auto DURATION = std::chrono::milliseconds(700);

	std::atomic<bool> is_stop {false};
	std::vector<std::atomic<int64_t>> vector_count(_vector_function.size());

	run_with_deadlock_watchdog([&]()
	{
		std::vector<std::thread> vector_thread;

		for (size_t i = 0; i < _vector_function.size(); ++i)
		{
			vector_thread.emplace_back([&, i]()
			{
				while (is_stop.load(std::memory_order_relaxed) == false)
				{
					try { _vector_function[i](); }
					catch (...) { ++_count_exception; }
					++vector_count[i];
				}
			});
		}

		std::this_thread::sleep_for(DURATION);
		is_stop = true;

		for (auto& iter : vector_thread)
			iter.join();
	}, std::chrono::seconds(60));

	std::vector<int64_t> vector_result;
	for (auto& iter : vector_count)
		vector_result.push_back(iter.load());
	return vector_result;
}

// 4) the member on each seat is the member of the group at that seat
void check_array_consistent(group_array_logging* _pgroup, size_t _capacity)
{
	size_t count = 0;
	for (size_t i = 0; i < _capacity; ++i)
	{
		auto pmember = _pgroup->member(static_cast<int64_t>(i));
		CONTINUE_IF(pmember.empty());

		++count;
		EXPECT_TRUE(pmember->is_member_of(_pgroup));
		EXPECT_EQ(group_array_logging::member_serial(pmember.get()), static_cast<group_member_id_t>(i));
	}
	EXPECT_EQ(count, _pgroup->member_count());
}

// - Nmap: the new member of the same key replaces the old member (old member leaves -> new member enters)
TEST(group_lock_order, nmap_duplicated_key_replaces)
{
	auto pgroup = make_group<group_nmap_replace>(SIZE_MAX);
	auto vector_member = make_members(2);
	auto* pmember_a = vector_member[0].get();
	auto* pmember_b = vector_member[1].get();
	pmember_b->m_key = pmember_a->m_key;

	ASSERT_TRUE(pgroup->enter(pmember_a) == eRESULT::SUCCESS);
	pgroup->clear_events();

	ASSERT_TRUE(pgroup->enter(pmember_b) == eRESULT::SUCCESS);

	EXPECT_EQ(pgroup->events(), (std::vector<EVENT>{ {'D', pmember_b}, {'e', pmember_b}, {'L', pmember_a}, {'E', pmember_b} }));
	EXPECT_TRUE(pmember_a->get_group().empty());
	EXPECT_TRUE(pmember_b->is_member_of(pgroup.get()));
	EXPECT_EQ(pgroup->member_count(), 1u);
	EXPECT_TRUE(pgroup->find_member(pmember_b->m_key) == pmember_b);

	// - the member already in the group
	pgroup->clear_events();
	EXPECT_TRUE(pgroup->enter(pmember_b) == eRESULT::ALREADY_EXIST);
	EXPECT_TRUE(pgroup->events().empty());

	pgroup->leave_all();
	EXPECT_TRUE(pmember_b->get_group().empty());
}

// - Nmap: the old member moved to other group in on_member_duplicated stays in that group
TEST(group_lock_order, nmap_duplicated_old_member_moved_stays)
{
	auto pgroup = make_group<group_nmap_replace>(SIZE_MAX);
	auto pgroup_other = make_group<group_list>(SIZE_MAX);
	auto vector_member = make_members(2);
	auto* pmember_a = vector_member[0].get();
	auto* pmember_b = vector_member[1].get();
	pmember_b->m_key = pmember_a->m_key;

	ASSERT_TRUE(pgroup->enter(pmember_a) == eRESULT::SUCCESS);
	pgroup->clear_events();

	pgroup->m_on_duplicated = [&](member* /*_pmember_new*/, member* _pmember_old) { pgroup_other->enter(_pmember_old); };
	ASSERT_TRUE(pgroup->enter(pmember_b) == eRESULT::SUCCESS);
	pgroup->m_on_duplicated = nullptr;

	EXPECT_EQ(pgroup->events(), (std::vector<EVENT>{ {'D', pmember_b}, {'L', pmember_a}, {'e', pmember_b}, {'E', pmember_b} }));
	EXPECT_TRUE(pmember_a->is_member_of(pgroup_other.get()));
	EXPECT_TRUE(pmember_b->is_member_of(pgroup.get()));
	EXPECT_EQ(pgroup->member_count(), 1u);
	EXPECT_EQ(pgroup_other->member_count(), 1u);

	pgroup->leave_all();
	pgroup_other->leave_all();
	EXPECT_TRUE(pmember_a->get_group().empty());
	EXPECT_TRUE(pmember_b->get_group().empty());
}

// - Nmap: members of the same key enter in turn
//   while the old member leaves / locks itself and then the group (member lock -> group lock)
TEST(group_lock_order, nmap_duplicated_key_concurrent)
{
	const size_t COUNT = 4;
	auto pgroup = make_group<group_nmap_replace>(SIZE_MAX);
	pgroup->m_is_logging = false;
	auto vector_member = make_members(COUNT);
	for (auto& iter : vector_member)
		iter->m_key = 0;

	std::atomic<int64_t> count_exception {0};
	std::atomic<int64_t> count_over {0};

	auto vector_count = run_threads_repeatedly(
	{
		// 1) enters the members of the same key in turn (the old member leaves)
		[&, index = size_t(0)]() mutable { pgroup->enter(vector_member[index++ % COUNT].get()); },
		[&, index = size_t(1)]() mutable { pgroup->enter(vector_member[index++ % COUNT].get()); },

		// 2) the old member leaves (member lock -> group lock)
		[&, index = size_t(2)]() mutable { vector_member[index++ % COUNT]->leave_group(); },

		// 3) locks the old member and then the group (member lock -> group lock)
		[&]()
		{
			auto pmember = pgroup->find_member(int64_t(0));
			RETURN_IF(pmember.empty());

			std::lock_guard lock(pmember->m_lockable_group);
			std::this_thread::yield();
			if (pgroup->find_member(int64_t(0)).exist() && pgroup->member_count() > 1)
				++count_over;
		},
	}, count_exception);

	for (auto iter : vector_count)
		EXPECT_GT(iter, 0);
	EXPECT_EQ(count_exception.load(), 0);
	EXPECT_EQ(count_over.load(), 0);
	EXPECT_GT(pgroup->m_count_duplicated.load(), 0);
	EXPECT_LE(pgroup->member_count(), 1u);

	pgroup->leave_all();

	EXPECT_EQ(pgroup->member_count(), 0u);
	EXPECT_EQ(pgroup->m_count_entered.load(), pgroup->m_count_leaved.load());
	for (auto& iter : vector_member)
		EXPECT_TRUE(iter->get_group().empty());
}

// - Nmap: two members of the same key enter the group of each other (member lock -> member lock)
TEST(group_lock_order, nmap_duplicated_key_swap_concurrent)
{
	auto pgroup_1 = make_group<group_nmap_replace>(SIZE_MAX);
	auto pgroup_2 = make_group<group_nmap_replace>(SIZE_MAX);
	pgroup_1->m_is_logging = false;
	pgroup_2->m_is_logging = false;
	auto vector_member = make_members(2);
	auto* pmember_a = vector_member[0].get();
	auto* pmember_b = vector_member[1].get();
	pmember_b->m_key = pmember_a->m_key;

	std::atomic<int64_t> count_exception {0};

	auto vector_count = run_threads_repeatedly(
	{
		[&]() { pgroup_1->enter(pmember_a); pgroup_2->enter(pmember_a); },
		[&]() { pgroup_2->enter(pmember_b); pgroup_1->enter(pmember_b); },
	}, count_exception);

	for (auto iter : vector_count)
		EXPECT_GT(iter, 0);
	EXPECT_EQ(count_exception.load(), 0);
	EXPECT_GT(pgroup_1->m_count_duplicated.load() + pgroup_2->m_count_duplicated.load(), 0);
	EXPECT_LE(pgroup_1->member_count(), 1u);
	EXPECT_LE(pgroup_2->member_count(), 1u);

	pgroup_1->leave_all();
	pgroup_2->leave_all();

	EXPECT_EQ(pgroup_1->m_count_entered.load(), pgroup_1->m_count_leaved.load());
	EXPECT_EQ(pgroup_2->m_count_entered.load(), pgroup_2->m_count_leaved.load());
	EXPECT_TRUE(pmember_a->get_group().empty());
	EXPECT_TRUE(pmember_b->get_group().empty());
}

// - array: a member enters the seat of other member
//   (entering -> the member on the seat leaves -> the member leaves its old seat -> entered)
TEST(group_lock_order, array_member_replaces)
{
	const size_t CAPACITY = 4;
	auto pgroup = make_group<group_array_logging>(CAPACITY);
	auto vector_member = make_members(2);
	auto* pmember_a = vector_member[0].get();
	auto* pmember_b = vector_member[1].get();
	sMESSAGE msg;

	pgroup->member(pmember_a, 1, msg);
	pgroup->member(pmember_b, 2, msg);
	pgroup->clear_events();

	// - a moves to the seat of b
	EXPECT_TRUE(pgroup->member(pmember_a, 2, msg) == pmember_b);
	EXPECT_EQ(pgroup->events(), (std::vector<EVENT>{ {'e', pmember_a}, {'L', pmember_b}, {'L', pmember_a}, {'E', pmember_a} }));
	EXPECT_TRUE(pmember_b->get_group().empty());
	EXPECT_TRUE(pgroup->member(2) == pmember_a);
	EXPECT_TRUE(pgroup->member(1).empty());
	EXPECT_EQ(pgroup->member_count(), 1u);

	// - b enters the empty seat (no member leaves)
	pgroup->clear_events();
	EXPECT_TRUE(pgroup->member(pmember_b, 1, msg).empty());
	EXPECT_EQ(pgroup->events(), (std::vector<EVENT>{ {'e', pmember_b}, {'E', pmember_b} }));

	// - the member already on the seat
	pgroup->clear_events();
	EXPECT_TRUE(pgroup->member(pmember_b, 1, msg).empty());
	EXPECT_TRUE(pgroup->events().empty());
	EXPECT_EQ(pgroup->member_count(), 2u);
	check_array_consistent(pgroup.get(), CAPACITY);

	pgroup->leave_all();
	EXPECT_TRUE(pmember_a->get_group().empty());
	EXPECT_TRUE(pmember_b->get_group().empty());
}

// - array: member(nullptr, seat) empties the seat (the member on it leaves, returned). a seat out of the range does nothing
//   (member(nullptr, seat) dereferenced the null member at the first lock. an out of range seat called on_member_entering and threw)
TEST(group_lock_order, array_member_null_empties_seat)
{
	const size_t CAPACITY = 4;
	auto pgroup = make_group<group_array_logging>(CAPACITY);
	auto vector_member = make_members(1);
	auto* pmember_a = vector_member[0].get();
	sMESSAGE msg;

	pgroup->member(pmember_a, 1, msg);
	pgroup->clear_events();

	// - an empty seat
	EXPECT_TRUE(pgroup->member(nullptr, 2, msg).empty());
	EXPECT_TRUE(pgroup->events().empty());

	// - the seat of a
	EXPECT_TRUE(pgroup->member(nullptr, 1, msg) == pmember_a);
	EXPECT_EQ(pgroup->events(), (std::vector<EVENT>{ {'L', pmember_a} }));
	EXPECT_TRUE(pmember_a->get_group().empty());
	EXPECT_TRUE(pgroup->member(1).empty());
	EXPECT_EQ(pgroup->member_count(), 0u);

	// - out of the range
	pgroup->clear_events();
	EXPECT_TRUE(pgroup->member(nullptr, -1, msg).empty());
	EXPECT_TRUE(pgroup->member(nullptr, static_cast<int64_t>(CAPACITY), msg).empty());
	EXPECT_TRUE(pgroup->member(pmember_a, static_cast<int64_t>(CAPACITY), msg).empty());
	EXPECT_TRUE(pgroup->events().empty());
	EXPECT_TRUE(pmember_a->get_group().empty());
	check_array_consistent(pgroup.get(), CAPACITY);
}

// - array: two members swap their seats (member lock -> enter lock -> member lock)
TEST(group_lock_order, array_swap_seat_concurrent)
{
	const size_t CAPACITY = 4;
	auto pgroup = make_group<group_array_logging>(CAPACITY);
	pgroup->m_is_logging = false;
	auto vector_member = make_members(2);
	auto* pmember_a = vector_member[0].get();
	auto* pmember_b = vector_member[1].get();

	std::atomic<int64_t> count_exception {0};

	auto vector_count = run_threads_repeatedly(
	{
		[&]() { sMESSAGE msg; pgroup->member(pmember_a, 1, msg); pgroup->member(pmember_a, 2, msg); },
		[&]() { sMESSAGE msg; pgroup->member(pmember_b, 2, msg); pgroup->member(pmember_b, 1, msg); },

		// - locks the member and then the group / leaves (member lock -> group lock)
		[&, index = size_t(0)]() mutable
		{
			auto* pmember = vector_member[index++ & 1].get();
			{
				std::lock_guard lock(pmember->m_lockable_group);
				(void)pgroup->member(1);
			}
			if ((index & 7) == 0)
				pmember->leave_group();
		},
	}, count_exception);

	for (auto iter : vector_count)
		EXPECT_GT(iter, 0);
	EXPECT_EQ(count_exception.load(), 0);
	EXPECT_LE(pgroup->member_count(), 2u);
	check_array_consistent(pgroup.get(), CAPACITY);

	pgroup->leave_all();

	EXPECT_EQ(pgroup->member_count(), 0u);
	EXPECT_EQ(pgroup->m_count_entered.load(), pgroup->m_count_leaved.load());
	EXPECT_TRUE(pmember_a->get_group().empty());
	EXPECT_TRUE(pmember_b->get_group().empty());
}

// - array: two members enter the seat of each other in the other group (member lock -> member lock)
TEST(group_lock_order, array_swap_group_concurrent)
{
	const size_t CAPACITY = 4;
	auto pgroup_1 = make_group<group_array_logging>(CAPACITY);
	auto pgroup_2 = make_group<group_array_logging>(CAPACITY);
	pgroup_1->m_is_logging = false;
	pgroup_2->m_is_logging = false;
	auto vector_member = make_members(2);
	auto* pmember_a = vector_member[0].get();
	auto* pmember_b = vector_member[1].get();

	std::atomic<int64_t> count_exception {0};

	auto vector_count = run_threads_repeatedly(
	{
		[&]() { sMESSAGE msg; pgroup_1->member(pmember_a, 0, msg); pgroup_2->member(pmember_a, 0, msg); },
		[&]() { sMESSAGE msg; pgroup_2->member(pmember_b, 0, msg); pgroup_1->member(pmember_b, 0, msg); },
	}, count_exception);

	for (auto iter : vector_count)
		EXPECT_GT(iter, 0);
	EXPECT_EQ(count_exception.load(), 0);
	check_array_consistent(pgroup_1.get(), CAPACITY);
	check_array_consistent(pgroup_2.get(), CAPACITY);

	pgroup_1->leave_all();
	pgroup_2->leave_all();

	EXPECT_EQ(pgroup_1->m_count_entered.load(), pgroup_1->m_count_leaved.load());
	EXPECT_EQ(pgroup_2->m_count_entered.load(), pgroup_2->m_count_leaved.load());
	EXPECT_TRUE(pmember_a->get_group().empty());
	EXPECT_TRUE(pmember_b->get_group().empty());
}


// ============================================================================
// CGDK::seat
//   seat::reset() put seat 'begin' into the unused slot 0 of the 1-based heap.
//   so a seat of N had N-1 seats (1 .. N-1) and seat 0 was never allocated.
// ============================================================================
TEST(group_seat, alloc_free)
{
	const size_t COUNT = 8;
	seat<group_member_id_t> seat_alloc(COUNT);

	EXPECT_EQ(seat_alloc.max_seat(), COUNT);
	EXPECT_EQ(seat_alloc.remained_seat(), COUNT);
	EXPECT_FALSE(seat_alloc.empty());

	// - all seats from 0 in ascending order
	for (size_t i = 0; i < COUNT; ++i)
		EXPECT_EQ(seat_alloc.alloc_seat(), static_cast<group_member_id_t>(i));
	EXPECT_TRUE(seat_alloc.empty());
	EXPECT_EQ(seat_alloc.remained_seat(), 0u);
	EXPECT_EQ(seat_alloc.alloc_seat(), -1);

	// - the smallest free seat first
	seat_alloc.free_seat(5);
	seat_alloc.free_seat(0);
	seat_alloc.free_seat(3);
	EXPECT_EQ(seat_alloc.remained_seat(), 3u);
	EXPECT_EQ(seat_alloc.alloc_seat(), 0);
	EXPECT_EQ(seat_alloc.alloc_seat(), 3);
	EXPECT_EQ(seat_alloc.alloc_seat(), 5);
	EXPECT_EQ(seat_alloc.alloc_seat(), -1);

	// - reset
	seat_alloc.reset();
	EXPECT_EQ(seat_alloc.remained_seat(), COUNT);
	EXPECT_EQ(seat_alloc.alloc_seat(), 0);

	// - create again (smaller / larger / zero)
	seat_alloc.create(2);
	EXPECT_EQ(seat_alloc.max_seat(), 2u);
	EXPECT_EQ(seat_alloc.alloc_seat(), 0);
	EXPECT_EQ(seat_alloc.alloc_seat(), 1);
	EXPECT_EQ(seat_alloc.alloc_seat(), -1);

	seat_alloc.create(100);
	EXPECT_EQ(seat_alloc.max_seat(), 100u);
	EXPECT_EQ(seat_alloc.remained_seat(), 100u);
	group_member_id_t seat_last = -1;
	for (size_t i = 0; i < 100; ++i)
		seat_last = seat_alloc.alloc_seat();
	EXPECT_EQ(seat_last, 99);
	EXPECT_TRUE(seat_alloc.empty());

	seat_alloc.create(0);
	EXPECT_EQ(seat_alloc.max_seat(), 0u);
	EXPECT_TRUE(seat_alloc.empty());
	EXPECT_EQ(seat_alloc.remained_seat(), 0u);
	EXPECT_EQ(seat_alloc.alloc_seat(), -1);
}

TEST(group_seat, begin_step)
{
	seat<int> seat_alloc(3, 10, 5);

	EXPECT_EQ(seat_alloc.max_seat(), 3u);
	EXPECT_EQ(seat_alloc.alloc_seat(), 10);
	EXPECT_EQ(seat_alloc.alloc_seat(), 15);
	EXPECT_EQ(seat_alloc.alloc_seat(), 20);
	EXPECT_EQ(seat_alloc.alloc_seat(), -1);

	seat_alloc.free_seat(15);
	EXPECT_EQ(seat_alloc.alloc_seat(), 15);
}

TEST(group_seat, not_created)
{
	seat<int> seat_alloc;

	EXPECT_TRUE(seat_alloc.empty());
	EXPECT_EQ(seat_alloc.max_seat(), 0u);
	EXPECT_EQ(seat_alloc.remained_seat(), 0u);
	EXPECT_EQ(seat_alloc.alloc_seat(), -1);
}

// - random alloc/free compared with std::set (alloc_seat returns the smallest free seat)
TEST(group_seat, random_against_set)
{
	const size_t COUNT = 37;
	seat<group_member_id_t> seat_alloc(COUNT);
	std::set<group_member_id_t> set_free;
	std::vector<group_member_id_t> vector_used;
	for (size_t i = 0; i < COUNT; ++i)
		set_free.insert(static_cast<group_member_id_t>(i));

	uint32_t state = 0x2545F491u;
	for (int n = 0; n < 20000; ++n)
	{
		if (vector_used.empty() || (set_free.empty() == false && (next_random(state) & 1) != 0))
		{
			auto seat_index = seat_alloc.alloc_seat();
			ASSERT_EQ(seat_index, *set_free.begin());
			set_free.erase(set_free.begin());
			vector_used.push_back(seat_index);
		}
		else
		{
			auto pos = next_random(state) % vector_used.size();
			seat_alloc.free_seat(vector_used[pos]);
			set_free.insert(vector_used[pos]);
			vector_used.erase(vector_used.begin() + pos);
		}
		ASSERT_EQ(seat_alloc.remained_seat(), set_free.size());
	}
}


// ============================================================================
// group::array_seat - seats
//   group::array_seat of capacity N had N-1 seats. the N-th enter() increased the member count
//   and then threw OUT_OF_RANGE (the count stayed larger than the members).
// ============================================================================
TEST(group_array_seat, full_capacity)
{
	const size_t COUNT = 8;
	auto pgroup = make_group<group_array_seat>(COUNT);
	auto vector_member = make_members(COUNT + 1);

	// - N members in capacity N. the seats are 0 .. N-1 in the order of entering
	for (size_t i = 0; i < COUNT; ++i)
	{
		EXPECT_TRUE(pgroup->enter(vector_member[i].get()) == eRESULT::SUCCESS);
		EXPECT_EQ(pgroup->member_serial(vector_member[i].get()), static_cast<group_member_id_t>(i));
		EXPECT_EQ(pgroup->member(static_cast<group_member_id_t>(i)).get(), vector_member[i].get());
	}
	EXPECT_EQ(pgroup->member_count(), COUNT);
	EXPECT_TRUE(pgroup->is_member_full());

	// - full : MEMBER_FULL (no exception, the count is not changed)
	auto* pmember_over = vector_member[COUNT].get();
	result_code result = eRESULT::SUCCESS;
	EXPECT_NO_THROW(result = pgroup->enter(pmember_over));
	EXPECT_TRUE(result == eRESULT::MEMBER_FULL);
	EXPECT_EQ(pgroup->member_count(), COUNT);
	EXPECT_TRUE(pmember_over->get_group().empty());

	// - the next member takes the empty seat
	vector_member[3]->leave_group();
	EXPECT_EQ(pgroup->member_count(), COUNT - 1);
	EXPECT_TRUE(pgroup->member(3).empty());
	EXPECT_TRUE(pgroup->enter(pmember_over) == eRESULT::SUCCESS);
	EXPECT_EQ(pgroup->member_serial(pmember_over), 3);
	EXPECT_EQ(pgroup->member_count(), COUNT);

	// - seat 0
	auto pmember_left = pgroup->leave_by_seat(0, 0);
	EXPECT_EQ(pmember_left.get(), vector_member[0].get());
	EXPECT_EQ(pgroup->member_count(), COUNT - 1);
	EXPECT_TRUE(pgroup->enter(vector_member[0].get()) == eRESULT::SUCCESS);
	EXPECT_EQ(pgroup->member_serial(vector_member[0].get()), 0);

	pgroup->leave_all();

	EXPECT_EQ(pgroup->member_count(), 0u);
	EXPECT_EQ(pgroup->m_count_entered.load(), pgroup->m_count_leaved.load());
	for (auto& iter : vector_member)
		EXPECT_TRUE(iter->get_group().empty());
}

// - member_capacity() re-creates the seats
//   (the seats were not re-created. a smaller capacity gave a seat out of the container)
TEST(group_array_seat, change_capacity)
{
	auto pgroup = make_group<group_array_seat>(8);
	auto vector_member = make_members(12);

	// - smaller
	pgroup->member_capacity(4);
	for (size_t i = 0; i < 4; ++i)
		EXPECT_TRUE(pgroup->enter(vector_member[i].get()) == eRESULT::SUCCESS);
	EXPECT_TRUE(pgroup->enter(vector_member[4].get()) == eRESULT::MEMBER_FULL);
	EXPECT_EQ(pgroup->member_count(), 4u);
	pgroup->leave_all();

	// - larger
	pgroup->member_capacity(12);
	for (size_t i = 0; i < 12; ++i)
	{
		EXPECT_TRUE(pgroup->enter(vector_member[i].get()) == eRESULT::SUCCESS);
		EXPECT_EQ(pgroup->member_serial(vector_member[i].get()), static_cast<group_member_id_t>(i));
	}
	EXPECT_EQ(pgroup->member_count(), 12u);
	pgroup->leave_all();

	EXPECT_EQ(pgroup->member_count(), 0u);
	EXPECT_EQ(pgroup->m_count_entered.load(), pgroup->m_count_leaved.load());
}


// ============================================================================
// group::Nmap - find_member overloads
//   find_member(key) on a non-const Nmap was ambiguous (ISO C++) between find_member(const _TKEY&) const
//   and find_member(const std::function&). GCC warned and MSVC chose silently.
// ============================================================================
TEST(group_nmap, find_member_overloads)
{
	auto pgroup = make_group<group_nmap>(SIZE_MAX);
	auto vector_member = make_members(3);
	for (auto& iter : vector_member)
		pgroup->enter(iter.get());

	const int64_t key = 1;
	const group_nmap* pgroup_const = pgroup.get();

	// - key (rvalue / lvalue / const group / not found)
	EXPECT_EQ(pgroup->find_member(int64_t(2)).get(), vector_member[2].get());
	EXPECT_EQ(pgroup->find_member(key).get(), vector_member[1].get());
	EXPECT_EQ(pgroup_const->find_member(int64_t(0)).get(), vector_member[0].get());
	EXPECT_TRUE(pgroup->find_member(int64_t(7)).empty());

	// - predicate
	EXPECT_EQ(pgroup->find_member([](member* _pmember) { return _pmember->m_key == 2; }).get(), vector_member[2].get());
	EXPECT_TRUE(pgroup->find_member([](member*) { return false; }).empty());

	pgroup->leave_all();
}


// ============================================================================
// broadcast (member list is copied in the lock and sent out of the lock)
// ============================================================================
template <class TGROUP>
void check_sender(TGROUP* _pgroup, std::vector<object_ptr<member>>& _vector_member)
{
	auto counts = [&]() { std::vector<size_t> v; for (auto& iter : _vector_member) v.push_back(iter->m_count_send.load()); return v; };
	auto* pmember_except = _vector_member[1].get();
	auto buf = make_message();

	// 1) send (process_send)
	_pgroup->send(buf);
	EXPECT_EQ(counts(), (std::vector<size_t>{1, 1, 1}));

	// 2) send_except
	_pgroup->send_except(pmember_except, buf);
	EXPECT_EQ(counts(), (std::vector<size_t>{2, 1, 2}));

	// 3) send_conditional
	_pgroup->send_conditional(buf, [&](const member* _pmember) { return _pmember == pmember_except; });
	EXPECT_EQ(counts(), (std::vector<size_t>{2, 2, 2}));
}

template <class TGROUP>
void check_sender_data(TGROUP* _pgroup, std::vector<object_ptr<member>>& _vector_member)
{
	auto counts = [&]() { std::vector<size_t> v; for (auto& iter : _vector_member) v.push_back(iter->m_count_send.load()); return v; };
	auto* pmember_except = _vector_member[1].get();

	// 1) send_except<T>
	_pgroup->send_except(pmember_except, uint32_t(7));
	EXPECT_EQ(counts(), (std::vector<size_t>{3, 2, 3}));

	// 2) send_conditional<T>
	_pgroup->send_conditional(uint32_t(7), [&](const member* _pmember) { return _pmember != pmember_except; });
	EXPECT_EQ(counts(), (std::vector<size_t>{4, 2, 4}));
}

// - send_to(seat) through Isender_peer (const shared_buffer& / shared_buffer&&)
template <class TGROUP>
void check_send_to(TGROUP* _pgroup, std::vector<object_ptr<member>>& _vector_member, const std::vector<int>& _vector_seat, int _seat_empty)
{
	net::io::Isender_peer* ppeer = _pgroup;
	auto buf = make_message();

	for (size_t i = 0; i < _vector_member.size(); ++i)
	{
		auto count = _vector_member[i]->m_count_send.load();
		EXPECT_TRUE(ppeer->send_to(_vector_seat[i], buf));
		EXPECT_TRUE(ppeer->send_to(_vector_seat[i], make_message()));
		EXPECT_EQ(_vector_member[i]->m_count_send.load(), count + 2);
	}

	EXPECT_FALSE(ppeer->send_to(_seat_empty, make_message()));
	EXPECT_FALSE(ppeer->send_to(-1, make_message()));
	EXPECT_FALSE(ppeer->send_to(static_cast<int>(_pgroup->member_capacity()), make_message()));
}

TEST(group_sender, list_sender)
{
	auto pgroup = make_group<group::list_sender<member>>(SIZE_MAX);
	auto vector_member = make_members(3);
	for (auto& iter : vector_member)
		pgroup->enter(iter.get());

	check_sender(pgroup.get(), vector_member);
	check_sender_data(pgroup.get(), vector_member);

	pgroup->leave_all();
}

// - group::array_sender did not implement Isender_peer::send_to(int, shared_buffer&&).
//   so make_object<group::array_sender<...>>() did not compile (abstract class)
TEST(group_sender, array_sender)
{
	auto pgroup = make_group<group::array_sender<member>>(8);
	auto vector_member = make_members(3);

	// - empty seats between members
	sMESSAGE msg;
	pgroup->member(vector_member[0].get(), 1, msg);
	pgroup->member(vector_member[1].get(), 3, msg);
	pgroup->member(vector_member[2].get(), 6, msg);

	check_sender(pgroup.get(), vector_member);
	check_sender_data(pgroup.get(), vector_member);
	check_send_to(pgroup.get(), vector_member, {1, 3, 6}, 2);

	pgroup->leave_all();
}

TEST(group_sender, array_seat_sender)
{
	auto pgroup = make_group<group::array_seat_sender<member>>(8);
	auto vector_member = make_members(4);
	for (auto& iter : vector_member)
		pgroup->enter(iter.get());

	// - empty seat between members (seats 0, 2, 3)
	vector_member[1]->leave_group();
	vector_member.erase(vector_member.begin() + 1);

	check_sender(pgroup.get(), vector_member);
	check_sender_data(pgroup.get(), vector_member);
	check_send_to(pgroup.get(), vector_member, {0, 2, 3}, 1);

	pgroup->leave_all();
}

TEST(group_sender, nmap_sender)
{
	auto pgroup = make_group<nmap_sender>(SIZE_MAX);
	auto vector_member = make_members(3);
	for (auto& iter : vector_member)
		pgroup->enter(iter.get());

	check_sender(pgroup.get(), vector_member);

	pgroup->leave_all();
}

TEST(group_sender, observable_list_sender)
{
	auto pobservable = make_object<observable::list_sender<observer>>();
	std::vector<object_ptr<observer>> vector_observer;
	for (int i = 0; i < 3; ++i)
		vector_observer.push_back(make_object<observer>());

	for (auto& iter : vector_observer)
		pobservable->subscribe(iter.get(), 0);

	auto counts = [&]() { std::vector<size_t> v; for (auto& iter : vector_observer) v.push_back(iter->m_count_send.load()); return v; };
	auto* pobserver_except = vector_observer[1].get();
	auto buf = make_message();

	pobservable->send(buf);
	EXPECT_EQ(counts(), (std::vector<size_t>{1, 1, 1}));

	pobservable->send_except(pobserver_except, buf);
	EXPECT_EQ(counts(), (std::vector<size_t>{2, 1, 2}));

	pobservable->send_conditional(buf, [&](const observer* _pobserver) { return _pobserver == pobserver_except; });
	EXPECT_EQ(counts(), (std::vector<size_t>{2, 2, 2}));

	for (auto& iter : vector_observer)
		iter->unsubscribe_all();

	EXPECT_EQ(pobservable->count(), 0u);
}

TEST(group_sender, group_set)
{
	auto pset = make_object<net::io::sender::group_set<sender>>();
	std::vector<object_ptr<sender>> vector_sender;
	for (int i = 0; i < 3; ++i)
	{
		vector_sender.push_back(make_object<sender>());
		pset->join(vector_sender.back().get());
	}

	pset->send(make_message());
	for (auto& iter : vector_sender)
		EXPECT_EQ(iter->m_count_send.load(), 1u);

	pset->leave(vector_sender[1].get());
	pset->send(make_message());
	EXPECT_EQ(vector_sender[0]->m_count_send.load(), 2u);
	EXPECT_EQ(vector_sender[1]->m_count_send.load(), 1u);
	EXPECT_EQ(vector_sender[2]->m_count_send.load(), 2u);

	pset->leave_all();
}


// ============================================================================
// broadcast of observable / container / net::io::sender
//   (receiver list is copied in the lock and sent out of the lock)
// ============================================================================
template <class TRECEIVER>
std::vector<size_t> get_send_counts(const std::vector<object_ptr<TRECEIVER>>& _vector_receiver)
{
	std::vector<size_t> vector_count;
	for (auto& iter : _vector_receiver)
		vector_count.push_back(iter->m_count_send.load());
	return vector_count;
}

// - returns the function which returns the send counts after now
template <class TRECEIVER>
auto make_sent_counter(const std::vector<object_ptr<TRECEIVER>>& _vector_receiver)
{
	return [&_vector_receiver, vector_base = get_send_counts(_vector_receiver)]()
	{
		auto vector_count = get_send_counts(_vector_receiver);
		for (size_t i = 0; i < vector_count.size(); ++i)
			vector_count[i] -= vector_base[i];
		return vector_count;
	};
}

// 1) send / send_except / send_conditional (3 receivers)
template <class TSENDER, class TRECEIVER>
void check_send_except_conditional(TSENDER* _psender, std::vector<object_ptr<TRECEIVER>>& _vector_receiver)
{
	auto sent = make_sent_counter(_vector_receiver);
	auto* preceiver_except = _vector_receiver[1].get();

	_psender->send(make_message());
	EXPECT_EQ(sent(), (std::vector<size_t>{1, 1, 1}));

	_psender->send_except(preceiver_except, make_message());
	EXPECT_EQ(sent(), (std::vector<size_t>{2, 1, 2}));

	_psender->send_conditional(make_message(), [&](const TRECEIVER* _preceiver) { return _preceiver == preceiver_except; });
	EXPECT_EQ(sent(), (std::vector<size_t>{2, 2, 2}));
}

// 2) send out of the lock (3 receivers)
//    - re-entrance : a receiver leaves in its process_send.
//                    (the container was modified while it was iterated in the lock)
//    - lock order  : thread 1 sends (container lock -> receiver lock)
//                    thread 2 locks a receiver and leaves/joins (receiver lock -> container lock)
//                    it deadlocked when the container sent in its lock.
template <class TRECEIVER, class FSEND, class FJOIN, class FLEAVE>
void check_send_out_of_lock(std::vector<object_ptr<TRECEIVER>>& _vector_receiver, FSEND _send, FJOIN _join_receiver, FLEAVE _leave_receiver)
{
	auto sent = make_sent_counter(_vector_receiver);

	// 1) re-entrance
	_vector_receiver[0]->m_on_send = [&]() { _leave_receiver(0); };
	_send();
	_vector_receiver[0]->m_on_send = nullptr;
	EXPECT_EQ(sent(), (std::vector<size_t>{1, 1, 1}));

	_send();
	EXPECT_EQ(sent(), (std::vector<size_t>{1, 2, 2}));

	_join_receiver(0);
	_send();
	EXPECT_EQ(sent(), (std::vector<size_t>{2, 3, 3}));

	// 2) lock order
	std::atomic<int64_t> count_send {0};
	std::atomic<int64_t> count_rejoin {0};

	run_with_deadlock_watchdog([&]()
	{
		std::atomic<bool> is_stop {false};

		std::thread thread_send([&]()
		{
			while (is_stop.load(std::memory_order_relaxed) == false)
			{
				_send();
				++count_send;
			}
		});

		std::thread thread_rejoin([&]()
		{
			while (is_stop.load(std::memory_order_relaxed) == false)
			{
				{
					std::lock_guard lock(_vector_receiver[1]->m_lockable_send);
					_leave_receiver(1);
					_join_receiver(1);
				}
				++count_rejoin;
				std::this_thread::yield();
			}
		});

		std::this_thread::sleep_for(std::chrono::milliseconds(300));
		is_stop = true;

		thread_send.join();
		thread_rejoin.join();
	}, std::chrono::seconds(60));

	EXPECT_GT(count_send.load(), 0);
	EXPECT_GT(count_rejoin.load(), 0);
}

TEST(group_sender, group_map)
{
	auto pmap = make_object<net::io::sender::group_map<int64_t, sender>>();
	std::vector<object_ptr<sender>> vector_sender;
	for (int64_t i = 0; i < 3; ++i)
	{
		vector_sender.push_back(make_object<sender>());
		pmap->join(i, vector_sender.back().get());
	}

	// - the key joined already is not replaced
	pmap->join(0, vector_sender[2].get());
	EXPECT_TRUE(pmap->find(0) == vector_sender[0]);

	pmap->send(make_message());
	EXPECT_EQ(get_send_counts(vector_sender), (std::vector<size_t>{1, 1, 1}));

	pmap->leave(1);
	EXPECT_TRUE(pmap->find(1).empty());
	pmap->send(make_message());
	EXPECT_EQ(get_send_counts(vector_sender), (std::vector<size_t>{2, 1, 2}));

	pmap->leave_all();
	pmap->send(make_message());
	EXPECT_EQ(get_send_counts(vector_sender), (std::vector<size_t>{2, 1, 2}));
}

TEST(group_sender, group_map_send_out_of_lock)
{
	auto pmap = make_object<net::io::sender::group_map<int64_t, sender>>();
	std::vector<object_ptr<sender>> vector_sender;
	for (int64_t i = 0; i < 3; ++i)
	{
		vector_sender.push_back(make_object<sender>());
		pmap->join(i, vector_sender.back().get());
	}

	check_send_out_of_lock(vector_sender,
		[&]() { pmap->send(make_message()); },
		[&](size_t _index) { pmap->join(static_cast<int64_t>(_index), vector_sender[_index].get()); },
		[&](size_t _index) { pmap->leave(static_cast<int64_t>(_index)); });

	pmap->leave_all();
}

TEST(group_sender, group_set_send_out_of_lock)
{
	auto pset = make_object<net::io::sender::group_set<sender>>();
	std::vector<object_ptr<sender>> vector_sender;
	for (int i = 0; i < 3; ++i)
	{
		vector_sender.push_back(make_object<sender>());
		pset->join(vector_sender.back().get());
	}

	check_send_out_of_lock(vector_sender,
		[&]() { pset->send(make_message()); },
		[&](size_t _index) { pset->join(vector_sender[_index].get()); },
		[&](size_t _index) { pset->leave(vector_sender[_index].get()); });

	pset->leave_all();
}

// 3) observable sender
//    - _subscribe(pobservable, key, pobserver, ocid)
//    - an observer can subscribe one observable by several channels. send_except excludes all of them.
template <class TOBSERVABLE, class FSUBSCRIBE>
void check_observable_sender(FSUBSCRIBE _subscribe)
{
	auto pobservable = make_object<TOBSERVABLE>();
	std::vector<object_ptr<observer>> vector_observer;
	for (size_t i = 0; i < 3; ++i)
	{
		vector_observer.push_back(make_object<observer>());
		_subscribe(pobservable.get(), static_cast<int64_t>(i), vector_observer.back().get(), 0);
	}

	// 1) send / send_except / send_conditional
	check_send_except_conditional(pobservable.get(), vector_observer);

	// 2) out of the lock
	check_send_out_of_lock(vector_observer,
		[&]() { pobservable->send(make_message()); },
		[&](size_t _index) { _subscribe(pobservable.get(), static_cast<int64_t>(_index), vector_observer[_index].get(), 0); },
		[&](size_t _index) { vector_observer[_index]->unsubscribe(0); });

	// 3) observer[1] subscribes by channel 1 too
	_subscribe(pobservable.get(), 3, vector_observer[1].get(), 1);
	ASSERT_EQ(pobservable->count(), 4u);

	auto sent = make_sent_counter(vector_observer);
	auto* pobserver_except = vector_observer[1].get();

	pobservable->send(make_message());
	EXPECT_EQ(sent(), (std::vector<size_t>{1, 2, 1}));

	pobservable->send_except(pobserver_except, make_message());
	EXPECT_EQ(sent(), (std::vector<size_t>{2, 2, 2}));

	pobservable->send_conditional(make_message(), [&](const observer* _pobserver) { return _pobserver == pobserver_except; });
	EXPECT_EQ(sent(), (std::vector<size_t>{2, 4, 2}));

	// 4) unsubscribe all
	for (auto& iter : vector_observer)
		iter->unsubscribe_all();

	EXPECT_EQ(pobservable->count(), 0u);
}

TEST(group_sender, observable_list_sender_send_out_of_lock)
{
	check_observable_sender<observable::list_sender<observer>>([](auto* _pobservable, int64_t, observer* _pobserver, observer::observer_channel_t _ocid) { _pobservable->subscribe(_pobserver, _ocid); });
}

// 4) observable::map / observable::set sender
TEST(group_sender, observable_map_sender_send_out_of_lock)
{
	check_observable_sender<observable::map_sender<observer, int64_t>>([](auto* _pobservable, int64_t _key, observer* _pobserver, observer::observer_channel_t _ocid) { _pobservable->subscribe(_key, _pobserver, _ocid); });
}

TEST(group_sender, observable_set_sender_send_out_of_lock)
{
	check_observable_sender<observable::set_sender<observer>>([](auto* _pobservable, int64_t, observer* _pobserver, observer::observer_channel_t _ocid) { _pobservable->subscribe(_pobserver, _ocid); });
}

// 5) container sender
//    - _register(pcontainer, key, psender)
template <class TCONTAINER, class FREGISTER>
void check_container_sender(FREGISTER _register)
{
	auto pcontainer = make_object<TCONTAINER>();

	std::vector<object_ptr<sender>> vector_sender;
	for (size_t i = 0; i < 3; ++i)
	{
		vector_sender.push_back(make_object<sender>());
		_register(pcontainer.get(), static_cast<int64_t>(i), vector_sender.back().get());
	}
	ASSERT_EQ(pcontainer->count(), 3u);

	// 1) send / send_except / send_conditional
	check_send_except_conditional(pcontainer.get(), vector_sender);

	// 2) out of the lock
	check_send_out_of_lock(vector_sender,
		[&]() { pcontainer->send(make_message()); },
		[&](size_t _index) { _register(pcontainer.get(), static_cast<int64_t>(_index), vector_sender[_index].get()); },
		[&](size_t _index) { pcontainer->unregister_object(vector_sender[_index].get()); });

	// 3) unregister all
	pcontainer->unregister_object_all();
	EXPECT_EQ(pcontainer->count(), 0u);
}

TEST(group_sender, container_list_sender_send_out_of_lock)
{
	check_container_sender<container::list_sender<sender>>([](auto* _pcontainer, int64_t, sender* _psender) { _pcontainer->register_object(_psender); });
}

TEST(group_sender, container_set_sender_send_out_of_lock)
{
	check_container_sender<container::set_sender<sender>>([](auto* _pcontainer, int64_t, sender* _psender) { _pcontainer->register_object(_psender); });
}

TEST(group_sender, container_map_sender_send_out_of_lock)
{
	check_container_sender<container::map_sender<sender, int64_t>>([](auto* _pcontainer, int64_t _key, sender* _psender) { _pcontainer->register_object(_key, _psender); });
}


// ============================================================================
// base classes of observable / container
//   (observable::list / observable::map / observable::set / container::* were never instantiated.
//    the explicit instantiations at the end of this file compile all their members)
// ============================================================================
std::vector<object_ptr<observer>> make_observers(size_t _count)
{
	std::vector<object_ptr<observer>> vector_observer;
	for (size_t i = 0; i < _count; ++i)
		vector_observer.push_back(make_object<observer>());
	return vector_observer;
}

bool is_observing(observer* _pobserver)
{
	return _pobserver->get_observable(0).exist() || _pobserver->get_observable(1).exist();
}

TEST(group_observable, map_basic)
{
	using observable_t = observable::map<observer, int64_t>;

	auto pobservable = make_object<observable_t>();
	auto vector_observer = make_observers(3);

	// 1) subscribe
	EXPECT_EQ(pobservable->subscribe(0, vector_observer[0].get(), 0, 10), eRESULT::SUCCESS);
	EXPECT_EQ(pobservable->subscribe(1, vector_observer[1].get(), 0, 11), eRESULT::SUCCESS);
	EXPECT_EQ(pobservable->subscribe(2, vector_observer[2].get(), 0, 12), eRESULT::SUCCESS);
	EXPECT_EQ(pobservable->subscribe(0, vector_observer[2].get(), 1, 0), eRESULT::ALREADY_EXIST);	// - the key subscribed already
	EXPECT_EQ(pobservable->subscribe(3, vector_observer[1].get(), 1, 13), eRESULT::SUCCESS);		// - observer[1] by channel 1 too
	EXPECT_EQ(pobservable->count(), 4u);
	EXPECT_TRUE(vector_observer[1]->is_observable(pobservable.get(), 1));
	EXPECT_EQ(observable_t::get_observer_data(vector_observer[0].get()), 10);

	// 2) find_observer / for_each
	EXPECT_TRUE(pobservable->find_observer(int64_t(1)) == vector_observer[1]);
	EXPECT_TRUE(pobservable->find_observer(int64_t(9)).empty());
	EXPECT_TRUE(pobservable->find_observer([&](observer* _pobserver) { return _pobserver == vector_observer[2].get(); }) == vector_observer[2]);

	size_t count_for_each = 0;
	EXPECT_TRUE(pobservable->for_each([&](observer*) { ++count_for_each; return true; }));
	EXPECT_EQ(count_for_each, 4u);
	EXPECT_FALSE(pobservable->for_each([](observer*) { return false; }));

	// 3) update_observer_data
	int64_t data = 20;
	EXPECT_TRUE(pobservable->update_observer_data(vector_observer[0].get(), data, 0));
	EXPECT_EQ(observable_t::get_observer_data(vector_observer[0].get()), 20);
	EXPECT_FALSE(pobservable->update_observer_data(vector_observer[0].get(), data, 1));
	data = 22;
	EXPECT_TRUE(pobservable->update_observer_data(int64_t(2), data));
	EXPECT_EQ(observable_t::get_observer_data(vector_observer[2].get()), 22);
	EXPECT_FALSE(pobservable->update_observer_data(int64_t(9), data));

	// 4) unsubscribe
	EXPECT_FALSE(pobservable->unsubscribe(int64_t(9)));		// - the key not subscribed
	EXPECT_TRUE(pobservable->unsubscribe(int64_t(2)));
	EXPECT_FALSE(is_observing(vector_observer[2].get()));
	EXPECT_FALSE(pobservable->unsubscribe(vector_observer[2].get(), 0));
	EXPECT_TRUE(pobservable->unsubscribe(vector_observer[1].get(), 1));
	EXPECT_FALSE(vector_observer[1]->is_observable(pobservable.get(), 1));
	EXPECT_EQ(pobservable->count(), 2u);

	// 5) unsubscribe_all (with the observer of channel 1)
	EXPECT_EQ(pobservable->subscribe(3, vector_observer[2].get(), 1), eRESULT::SUCCESS);
	pobservable->unsubscribe_all();
	EXPECT_EQ(pobservable->count(), 0u);
	for (auto& iter : vector_observer)
		EXPECT_FALSE(is_observing(iter.get()));
}

TEST(group_observable, set_basic)
{
	using observable_t = observable::set<observer>;

	auto pobservable = make_object<observable_t>();
	auto vector_observer = make_observers(3);

	// 1) subscribe
	EXPECT_EQ(pobservable->subscribe(vector_observer[0].get(), 0, 10), eRESULT::SUCCESS);
	EXPECT_EQ(pobservable->subscribe(vector_observer[1].get(), 0, 11), eRESULT::SUCCESS);
	EXPECT_EQ(pobservable->subscribe(vector_observer[2].get()), eRESULT::SUCCESS);
	EXPECT_EQ(pobservable->subscribe(vector_observer[0].get(), 0), eRESULT::ALREADY_EXIST);
	EXPECT_EQ(pobservable->subscribe(vector_observer[1].get(), 1, 13), eRESULT::SUCCESS);			// - observer[1] by channel 1 too
	EXPECT_EQ(pobservable->count(), 4u);
	EXPECT_EQ(observable_t::get_observer_data(vector_observer[0].get()), 10);

	// 2) find_observer / for_each
	EXPECT_TRUE(pobservable->find_observer([&](observer* _pobserver) { return _pobserver == vector_observer[2].get(); }) == vector_observer[2]);
	EXPECT_TRUE(pobservable->find_observer([](observer*) { return false; }).empty());

	size_t count_for_each = 0;
	EXPECT_TRUE(pobservable->for_each([&](observer*) { ++count_for_each; return true; }));
	EXPECT_EQ(count_for_each, 4u);

	// 3) update_observer_data
	int64_t data = 20;
	EXPECT_TRUE(pobservable->update_observer_data(vector_observer[0].get(), data, 0));
	EXPECT_EQ(observable_t::get_observer_data(vector_observer[0].get()), 20);
	EXPECT_FALSE(pobservable->update_observer_data(vector_observer[2].get(), data, 1));

	// 4) unsubscribe
	EXPECT_TRUE(pobservable->unsubscribe(vector_observer[2].get(), 0));
	EXPECT_FALSE(pobservable->unsubscribe(vector_observer[2].get(), 0));
	EXPECT_EQ(pobservable->count(), 3u);

	// 5) unsubscribe_all (with the observer of channel 1)
	pobservable->unsubscribe_all();
	EXPECT_EQ(pobservable->count(), 0u);
	for (auto& iter : vector_observer)
		EXPECT_FALSE(is_observing(iter.get()));
}

TEST(group_observable, list_basic)
{
	using observable_t = observable::list<observer>;

	auto pobservable = make_object<observable_t>();
	auto vector_observer = make_observers(3);

	// - capacity is unlimited by default
	EXPECT_EQ(pobservable->capacity(), SIZE_MAX);

	// 1) subscribe
	EXPECT_EQ(pobservable->subscribe(vector_observer[0].get(), 0, 10), eRESULT::SUCCESS);
	EXPECT_EQ(pobservable->subscribe(vector_observer[1].get(), 0, 11), eRESULT::SUCCESS);
	EXPECT_EQ(pobservable->subscribe(vector_observer[2].get()), eRESULT::SUCCESS);
	EXPECT_EQ(pobservable->subscribe(vector_observer[0].get(), 0), eRESULT::ALREADY_EXIST);
	EXPECT_EQ(pobservable->subscribe(vector_observer[1].get(), 1, 13), eRESULT::SUCCESS);			// - observer[1] by channel 1 too
	EXPECT_EQ(pobservable->subscribe(vector_observer[2].get(), 1, 14), eRESULT::SUCCESS);			// - observer[2] by channel 1 too
	EXPECT_EQ(pobservable->count(), 5u);
	EXPECT_TRUE(vector_observer[1]->is_observable(pobservable.get(), 1));
	EXPECT_EQ(observable_t::get_observer_data(vector_observer[0].get()), 10);
	EXPECT_EQ(observable_t::get_observer_data(vector_observer[1].get()), 13);						// - the data of the last subscription

	// 2) find_observer / for_each (in the order of subscription)
	EXPECT_TRUE(pobservable->find_observer([&](observer* _pobserver) { return _pobserver == vector_observer[2].get(); }) == vector_observer[2]);
	EXPECT_TRUE(pobservable->find_observer([](observer*) { return false; }).empty());

	std::vector<observer*> vector_visited;
	EXPECT_TRUE(pobservable->for_each([&](observer* _pobserver) { vector_visited.push_back(_pobserver); return true; }));
	EXPECT_EQ(vector_visited, (std::vector<observer*>{ vector_observer[0].get(), vector_observer[1].get(), vector_observer[2].get(), vector_observer[1].get(), vector_observer[2].get() }));

	size_t count_for_each = 0;
	EXPECT_FALSE(pobservable->for_each([&](observer*) { ++count_for_each; return false; }));
	EXPECT_EQ(count_for_each, 1u);

	// 3) update_observer_data
	int64_t data = 20;
	EXPECT_TRUE(pobservable->update_observer_data(vector_observer[0].get(), data, 0));
	EXPECT_EQ(observable_t::get_observer_data(vector_observer[0].get()), 20);
	EXPECT_FALSE(pobservable->update_observer_data(vector_observer[0].get(), data, 1));			// - not subscribed by channel 1

	// 4) unsubscribe
	EXPECT_TRUE(pobservable->unsubscribe(vector_observer[2].get(), 0));
	EXPECT_FALSE(pobservable->unsubscribe(vector_observer[2].get(), 0));
	EXPECT_TRUE(vector_observer[2]->is_observable(pobservable.get(), 1));							// - channel 1 stays
	EXPECT_EQ(pobservable->count(), 4u);

	// 5) unsubscribe_all (with the observers of channel 1)
	pobservable->unsubscribe_all();
	EXPECT_EQ(pobservable->count(), 0u);
	for (auto& iter : vector_observer)
		EXPECT_FALSE(is_observing(iter.get()));

	// 6) capacity
	pobservable->capacity(1);
	EXPECT_EQ(pobservable->capacity(), 1u);
	EXPECT_EQ(pobservable->subscribe(vector_observer[0].get()), eRESULT::SUCCESS);
	EXPECT_EQ(pobservable->subscribe(vector_observer[1].get()), eRESULT::MEMBER_FULL);
	pobservable->unsubscribe_all();
	EXPECT_EQ(pobservable->count(), 0u);
	EXPECT_FALSE(is_observing(vector_observer[0].get()));
}

// - _register(pcontainer, key, psender) returns result_code
template <class TCONTAINER, class FREGISTER>
void check_container_basic(FREGISTER _register)
{
	auto pcontainer = make_object<TCONTAINER>();

	// - capacity is unlimited by default
	EXPECT_EQ(pcontainer->capacity(), SIZE_MAX);

	// 1) register_object
	std::vector<object_ptr<sender>> vector_sender;
	for (size_t i = 0; i < 3; ++i)
	{
		vector_sender.push_back(make_object<sender>());
		EXPECT_EQ(_register(pcontainer.get(), static_cast<int64_t>(i), vector_sender.back().get()), eRESULT::SUCCESS);
	}
	EXPECT_EQ(_register(pcontainer.get(), 0, vector_sender[0].get()), eRESULT::ALREADY_EXIST);
	EXPECT_EQ(pcontainer->count(), 3u);

	// 2) find_object / for_each
	EXPECT_TRUE(pcontainer->find_object([&](sender* _psender) { return _psender == vector_sender[2].get(); }) == vector_sender[2]);
	EXPECT_TRUE(pcontainer->find_object([](sender*) { return false; }).empty());

	size_t count_for_each = 0;
	EXPECT_TRUE(pcontainer->for_each([&](sender*) { ++count_for_each; return true; }));
	EXPECT_EQ(count_for_each, 3u);
	EXPECT_FALSE(pcontainer->for_each([](sender*) { return false; }));

	// 3) unregister_object
	EXPECT_TRUE(pcontainer->unregister_object(vector_sender[1].get()));
	EXPECT_FALSE(pcontainer->unregister_object(vector_sender[1].get()));
	EXPECT_EQ(pcontainer->count(), 2u);

	// 4) unregister_object_all
	pcontainer->unregister_object_all();
	EXPECT_EQ(pcontainer->count(), 0u);

	// 5) capacity
	pcontainer->capacity(1);
	EXPECT_EQ(_register(pcontainer.get(), 0, vector_sender[0].get()), eRESULT::SUCCESS);
	EXPECT_EQ(_register(pcontainer.get(), 1, vector_sender[1].get()), eRESULT::MEMBER_FULL);
	pcontainer->unregister_object_all();
	EXPECT_EQ(pcontainer->count(), 0u);
}

TEST(group_container, list_basic)
{
	check_container_basic<container::list<sender>>([](auto* _pcontainer, int64_t, sender* _psender) { return _pcontainer->register_object(_psender); });
}

TEST(group_container, set_basic)
{
	check_container_basic<container::set<sender>>([](auto* _pcontainer, int64_t, sender* _psender) { return _pcontainer->register_object(_psender); });
}

TEST(group_container, map_basic)
{
	check_container_basic<container::map<sender, int64_t>>([](auto* _pcontainer, int64_t _key, sender* _psender) { return _pcontainer->register_object(_key, _psender); });

	// - unregister_object by key
	auto pcontainer = make_object<container::map<sender, int64_t>>();
	auto psender = make_object<sender>();
	EXPECT_EQ(pcontainer->register_object(5, psender.get()), eRESULT::SUCCESS);
	EXPECT_FALSE(pcontainer->unregister_object(int64_t(6)));
	EXPECT_TRUE(pcontainer->unregister_object(int64_t(5)));
	EXPECT_EQ(pcontainer->count(), 0u);
}


// ============================================================================
// copy-on-write member snapshot (group lock stage 3)
//   - get_member_snapshot() : published immutable member array, the group lock is not taken while it is valid
//   - a member list change (in the group lock) drops it. the next reader rebuilds it in the group lock
//   - in on_member_entered / on_member_leaving the live members are seen (private snapshot, not published)
// ============================================================================
namespace cow
{
	// - message : [size][type][phase 0..COUNT_PHASE-1]
	constexpr uint32_t TYPE_DELTA = 1;
	constexpr uint32_t TYPE_STATE = 2;
	constexpr size_t COUNT_PHASE = 8;

	inline shared_buffer make_message(uint32_t _type, const std::atomic<uint32_t>* _phase = nullptr, uint32_t _value = 0)
	{
		auto buf = alloc_shared_buffer(8 + 4 * COUNT_PHASE);
		buf.append<uint32_t>(static_cast<uint32_t>(8 + 4 * COUNT_PHASE));
		buf.append<uint32_t>(_type);
		for (size_t i = 0; i < COUNT_PHASE; ++i)
			buf.append<uint32_t>((_phase != nullptr) ? _phase[i].load(std::memory_order_acquire) : _value);
		return buf;
	}
}

// 1) receiver checking the order of messages (group member / observer / container object)
//    - phase (owned by the test) : 2*cycle = entering/in the group, 2*cycle+1 = left (leave() returned)
//    - DELTA started in the 'left' phase and delivered in the same phase -> the snapshot was taken after leave() returned -> error
//    - DELTA started in the 'in' phase and delivered in the same phase -> STATE of this cycle (sent in on_member_entered) must be received before
struct cow_receive_check
{
public:
			void				on_receive(const shared_buffer& _buffer, std::size_t _count_message) noexcept
	{
		this->m_count_send += _count_message;

		auto type = _buffer.front<uint32_t>(4);
		auto phase_sent = _buffer.front<uint32_t>(8 + 4 * this->m_index);

		if (type == cow::TYPE_STATE)
		{
			this->m_cycle_state.store(phase_sent, std::memory_order_release);
		}
		else if (type == cow::TYPE_DELTA && this->m_pphase != nullptr)
		{
			// - the cycle of STATE first : read after the phase, a leave + enter between the two loads would be counted as an error
			auto cycle_state = this->m_cycle_state.load(std::memory_order_acquire);
			auto phase_now = this->m_pphase->load(std::memory_order_acquire);

			if (phase_sent == phase_now)
			{
				if ((phase_now & 1) != 0)
					++this->m_count_after_leave;
				else if (cycle_state != phase_now / 2)
					++this->m_count_before_state;
			}
		}
	}

public:
			size_t				m_index {0};
			int64_t				m_key {0};
			const std::atomic<uint32_t>* m_pphase {nullptr};
			std::atomic<uint32_t> m_cycle_state {UINT32_MAX};
			std::atomic<size_t>	m_count_send {0};
			std::atomic<size_t>	m_count_after_leave {0};
			std::atomic<size_t>	m_count_before_state {0};
};

class cow_member :
	public						Igroupable<cow_member>,
	public						net::io::Isender,
	public						cow_receive_check
{
public:
	virtual	bool				process_send(shared_buffer&& _buffer, std::size_t _count_message, uint64_t /*_option*/) override { this->on_receive(_buffer, _count_message); return true; }
};

// 2) group : sends STATE(cycle) to the entering member in on_member_entered and records the snapshots seen in the events
//    (TBASE : group::list_sender / group::Nmap_sender)
class cow_nmap_sender : public group::Nmap_sender<cow_member, int64_t>
{
public:
	using group::Nmap_sender<cow_member, int64_t>::Nmap_sender;
protected:
	virtual	int64_t				process_get_member_key(cow_member* _pmember) override { return _pmember->m_key; }
};

template <class TBASE>
class cow_group_t : public TBASE
{
public:
	using TBASE::TBASE;

public:
			std::atomic<int64_t> m_count_entered {0};
			std::atomic<int64_t> m_count_leaved {0};
			std::atomic<int64_t> m_count_snapshot_error {0};
			bool				m_slow_entered {false};
			std::function<void (cow_member*)> m_on_entered;

protected:
	virtual	void				on_member_entered(cow_member* _pmember, sMESSAGE& /*_param*/) override
	{
		++this->m_count_entered;

		// - the live members (with the entering member) are seen in the event
		auto snapshot = this->get_member_snapshot();
		if (std::find(snapshot.begin(), snapshot.end(), _pmember) == snapshot.end() || snapshot.size() != this->member_count())
			++this->m_count_snapshot_error;

		// - (window) other threads broadcast now
		if (this->m_slow_entered)
			std::this_thread::yield();

		// - state of this cycle to the entering member
		if (_pmember->m_pphase != nullptr)
			_pmember->send(cow::make_message(cow::TYPE_STATE, nullptr, _pmember->m_pphase->load(std::memory_order_acquire) / 2));

		if (this->m_on_entered)
			this->m_on_entered(_pmember);
	}
	virtual	uintptr_t			on_member_leaving(cow_member* _pmember, uintptr_t _param) override
	{
		// - the leaving member is still seen in on_member_leaving
		auto snapshot = this->get_member_snapshot();
		if (std::find(snapshot.begin(), snapshot.end(), _pmember) == snapshot.end())
			++this->m_count_snapshot_error;
		return _param;
	}
	virtual	void				on_member_leaved(cow_member* _pmember, uintptr_t /*_result*/) override
	{
		++this->m_count_leaved;

		// - the left member is not seen any more
		auto snapshot = this->get_member_snapshot();
		if (std::find(snapshot.begin(), snapshot.end(), _pmember) != snapshot.end())
			++this->m_count_snapshot_error;
	}
};

using cow_group		 = cow_group_t<group::list_sender<cow_member>>;
using cow_group_nmap = cow_group_t<cow_nmap_sender>;
using cow_group_array		 = cow_group_t<group::array_sender<cow_member>>;
using cow_group_array_seat = cow_group_t<group::array_seat_sender<cow_member>>;

// - group::array has no enter() : a member takes the seat of its key
template <class TGROUP>
result_code cow_enter(TGROUP* _pgroup, cow_member* _pmember)
{
	if constexpr (std::is_base_of_v<group::array<cow_member>, TGROUP>)
	{
		sMESSAGE msg;
		_pgroup->member(_pmember, _pmember->m_key, msg);
		return _pmember->is_member_of(_pgroup) ? result_code(eRESULT::SUCCESS) : result_code(eRESULT::FAIL);
	}
	else
	{
		return _pgroup->enter(_pmember);
	}
}

// - array / array_seat allocate all the seats : 64
template <class TGROUP>
object_ptr<TGROUP> cow_make_group()
{
	constexpr bool is_array = std::is_base_of_v<group::array<cow_member>, TGROUP> || std::is_base_of_v<group::array_seat<cow_member>, TGROUP>;
	return make_group<TGROUP>(is_array ? 64 : SIZE_MAX);
}

inline std::vector<object_ptr<cow_member>> make_cow_members(size_t _count)
{
	std::vector<object_ptr<cow_member>> vector_member;
	for (size_t i = 0; i < _count; ++i)
	{
		auto pmember = make_object<cow_member>();
		pmember->m_index = i % cow::COUNT_PHASE;
		pmember->m_key = static_cast<int64_t>(i);
		vector_member.push_back(std::move(pmember));
	}
	return vector_member;
}

template <class TGROUP>
void check_cow_snapshot_basic()
{
	auto pgroup = cow_make_group<TGROUP>();
	auto vector_member = make_cow_members(4);

	// - empty group : empty snapshot
	EXPECT_EQ(pgroup->get_member_snapshot().size(), 0u);

	for (size_t i = 0; i < 3; ++i)
		cow_enter(pgroup.get(), vector_member[i].get());

	// - published snapshot is shared while the member list does not change
	auto snapshot_1 = pgroup->get_member_snapshot();
	auto snapshot_2 = pgroup->get_member_snapshot();
	ASSERT_EQ(snapshot_1.size(), 3u);
	EXPECT_EQ(snapshot_1.get(), snapshot_2.get());
	for (size_t i = 0; i < 3; ++i)
		EXPECT_EQ(snapshot_1.begin()[i], vector_member[i].get());

	// - enter : a new snapshot. the old one is not changed
	cow_enter(pgroup.get(), vector_member[3].get());
	auto snapshot_3 = pgroup->get_member_snapshot();
	EXPECT_NE(snapshot_3.get(), snapshot_1.get());
	EXPECT_EQ(snapshot_3.size(), 4u);
	EXPECT_EQ(snapshot_1.size(), 3u);

	// - leave : same
	vector_member[0]->leave_group();
	auto snapshot_4 = pgroup->get_member_snapshot();
	EXPECT_EQ(snapshot_4.size(), 3u);
	EXPECT_EQ(std::find(snapshot_4.begin(), snapshot_4.end(), vector_member[0].get()), snapshot_4.end());
	EXPECT_EQ(snapshot_3.size(), 4u);

	// - member_count and the snapshot
	EXPECT_EQ(pgroup->member_count(), snapshot_4.size());

	// - events saw the live members
	EXPECT_EQ(pgroup->m_count_snapshot_error.load(), 0);

	pgroup->leave_all();
	EXPECT_EQ(pgroup->get_member_snapshot().size(), 0u);
	EXPECT_EQ(pgroup->m_count_entered.load(), 4);
	EXPECT_EQ(pgroup->m_count_leaved.load(), 4);
}

// - a snapshot keeps its members alive after they left (and the group was destroyed)
template <class TGROUP>
void check_cow_snapshot_keeps_members()
{
	group::member_snapshot_ptr<cow_member> snapshot;
	std::vector<cow_member*> vector_raw;
	{
		auto pgroup = cow_make_group<TGROUP>();
		auto vector_member = make_cow_members(3);
		for (auto& iter : vector_member)
		{
			cow_enter(pgroup.get(), iter.get());
			vector_raw.push_back(iter.get());
		}

		snapshot = pgroup->get_member_snapshot();
		pgroup->leave_all();
	}

	ASSERT_EQ(snapshot.size(), 3u);
	for (size_t i = 0; i < 3; ++i)
	{
		EXPECT_EQ(snapshot.begin()[i], vector_raw[i]);
		EXPECT_EQ(snapshot.begin()[i]->reference_count(), 1);
		EXPECT_TRUE(snapshot.begin()[i]->get_group().empty());
	}
	snapshot = group::member_snapshot_ptr<cow_member>();
}

// - iterating a snapshot while members enter / leave (also in the loop body)
template <class TGROUP>
void check_cow_iterate_while_enter_leave()
{
	auto pgroup = cow_make_group<TGROUP>();
	auto vector_member = make_cow_members(8);
	for (size_t i = 0; i < 4; ++i)
		cow_enter(pgroup.get(), vector_member[i].get());

	auto snapshot = pgroup->get_member_snapshot();
	size_t count = 0;
	for (auto* iter : snapshot)
	{
		// - leave the current member and enter a new one in the loop
		iter->leave_group();
		cow_enter(pgroup.get(), vector_member[4 + count].get());
		iter->send(cow::make_message(cow::TYPE_STATE));
		++count;
	}
	EXPECT_EQ(count, 4u);
	EXPECT_EQ(pgroup->member_count(), 4u);

	auto snapshot_after = pgroup->get_member_snapshot();
	ASSERT_EQ(snapshot_after.size(), 4u);
	for (size_t i = 0; i < 4; ++i)
		EXPECT_EQ(snapshot_after.begin()[i], vector_member[4 + i].get());

	// - send : only the members now in the group
	pgroup->send(cow::make_message(cow::TYPE_STATE));
	for (size_t i = 0; i < 4; ++i)
		EXPECT_EQ(vector_member[i]->m_count_send.load(), 1u);
	for (size_t i = 4; i < 8; ++i)
		EXPECT_EQ(vector_member[i]->m_count_send.load(), 1u);

	EXPECT_EQ(pgroup->m_count_snapshot_error.load(), 0);
	pgroup->leave_all();
}

// - send in on_member_entered sees the entering member, send_except too
template <class TGROUP>
void check_cow_send_in_event_sees_live_members()
{
	auto pgroup = cow_make_group<TGROUP>();
	auto vector_member = make_cow_members(3);
	pgroup->m_on_entered = [&](cow_member* _pmember)
	{
		pgroup->send_except(_pmember, cow::make_message(cow::TYPE_STATE));
		pgroup->send(cow::make_message(cow::TYPE_STATE));
	};

	for (auto& iter : vector_member)
		cow_enter(pgroup.get(), iter.get());

	// - member i : send x (3 - i) + send_except x (2 - i)
	EXPECT_EQ(vector_member[0]->m_count_send.load(), 3u + 2u);
	EXPECT_EQ(vector_member[1]->m_count_send.load(), 2u + 1u);
	EXPECT_EQ(vector_member[2]->m_count_send.load(), 1u + 0u);
	EXPECT_EQ(pgroup->m_count_snapshot_error.load(), 0);

	pgroup->m_on_entered = nullptr;
	pgroup->leave_all();
}

// - concurrent broadcast + enter / leave
//   . DELTA from a broadcast that started after leave() returned is not delivered (same as the snapshot in the lock)
//   . DELTA of a cycle is not delivered before the STATE sent in on_member_entered of the cycle
//   . event count : entered == leaved, no member left in the group
template <class TGROUP>
void check_cow_concurrent(bool _slow_entered)
{
	const size_t COUNT_MEMBER = cow::COUNT_PHASE;
	const size_t COUNT_CHURN = 4;
	const size_t COUNT_SENDER = 4;
	const auto DURATION = std::chrono::milliseconds(700);

	auto pgroup = cow_make_group<TGROUP>();
	pgroup->m_slow_entered = _slow_entered;
	auto vector_member = make_cow_members(COUNT_MEMBER);

	std::atomic<uint32_t> phase[cow::COUNT_PHASE];
	for (size_t i = 0; i < cow::COUNT_PHASE; ++i)
		phase[i].store(1);
	for (size_t i = 0; i < COUNT_MEMBER; ++i)
		vector_member[i]->m_pphase = &phase[i];

	std::atomic<bool> is_stop {false};
	std::atomic<size_t> count_send {0};

	run_with_deadlock_watchdog([&]()
	{
		std::vector<std::thread> vector_thread;

		// - churn : member i is entered / left only by thread (i % COUNT_CHURN)
		for (size_t t = 0; t < COUNT_CHURN; ++t)
		{
			vector_thread.emplace_back([&, t]()
			{
				uint32_t cycle = 1;
				while (is_stop.load() == false)
				{
					for (size_t i = t; i < COUNT_MEMBER; i += COUNT_CHURN)
					{
						phase[i].store(2 * cycle, std::memory_order_release);
						cow_enter(pgroup.get(), vector_member[i].get());
						std::this_thread::yield();
						vector_member[i]->leave_group();
						phase[i].store(2 * cycle + 1, std::memory_order_release);
					}
					++cycle;
				}
			});
		}

		// - senders
		for (size_t t = 0; t < COUNT_SENDER; ++t)
		{
			vector_thread.emplace_back([&, t]()
			{
				while (is_stop.load() == false)
				{
					auto buf = cow::make_message(cow::TYPE_DELTA, phase);
					if ((t & 1) == 0)
						pgroup->send(buf);
					else
						pgroup->send_except(nullptr, buf);
					++count_send;
				}
			});
		}

		std::this_thread::sleep_for(DURATION);
		is_stop = true;
		for (auto& iter : vector_thread)
			iter.join();
	}, std::chrono::seconds(60));

	size_t after_leave = 0, before_state = 0, delivered = 0;
	for (auto& iter : vector_member)
	{
		after_leave += iter->m_count_after_leave.load();
		before_state += iter->m_count_before_state.load();
		delivered += iter->m_count_send.load();
	}

	EXPECT_GT(count_send.load(), 0u);
	EXPECT_GT(delivered, 0u);
	EXPECT_EQ(after_leave, 0u);
	EXPECT_EQ(before_state, 0u);
	EXPECT_EQ(pgroup->m_count_snapshot_error.load(), 0);
	EXPECT_EQ(pgroup->m_count_entered.load(), pgroup->m_count_leaved.load());
	EXPECT_EQ(pgroup->member_count(), 0u);
	EXPECT_EQ(pgroup->get_member_snapshot().size(), 0u);
	for (auto& iter : vector_member)
		EXPECT_TRUE(iter->get_group().empty());
}


// - concurrent broadcast while members move between two groups (snapshot of each group, leave_all at the end)
template <class TGROUP>
void check_cow_concurrent_move_between_groups()
{
	const size_t COUNT_MEMBER = 32;
	auto pgroup_a = cow_make_group<TGROUP>();
	auto pgroup_b = cow_make_group<TGROUP>();
	auto vector_member = make_cow_members(COUNT_MEMBER);
	std::atomic<bool> is_stop {false};

	run_with_deadlock_watchdog([&]()
	{
		std::vector<std::thread> vector_thread;
		for (size_t t = 0; t < 4; ++t)
		{
			vector_thread.emplace_back([&, t]()
			{
				uint32_t state = static_cast<uint32_t>(t * 7919 + 1);
				while (is_stop.load() == false)
				{
					auto& pmember = vector_member[next_random(state) % COUNT_MEMBER];
					if (next_random(state) & 1)
						cow_enter(pgroup_a.get(), pmember.get());
					else
						cow_enter(pgroup_b.get(), pmember.get());
				}
			});
		}
		for (size_t t = 0; t < 4; ++t)
		{
			vector_thread.emplace_back([&, t]()
			{
				while (is_stop.load() == false)
				{
					auto buf = cow::make_message(cow::TYPE_STATE);
					((t & 1) ? pgroup_a : pgroup_b)->send(buf);
					auto snapshot = ((t & 1) ? pgroup_b : pgroup_a)->get_member_snapshot();
					for (auto* iter : snapshot)
						EXPECT_NE(iter, nullptr);
				}
			});
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		is_stop = true;
		for (auto& iter : vector_thread)
			iter.join();
	}, std::chrono::seconds(60));

	EXPECT_EQ(pgroup_a->member_count() + pgroup_b->member_count(), COUNT_MEMBER);
	EXPECT_EQ(pgroup_a->get_member_snapshot().size() + pgroup_b->get_member_snapshot().size(), COUNT_MEMBER);

	pgroup_a->leave_all();
	pgroup_b->leave_all();
	EXPECT_EQ(pgroup_a->m_count_entered.load() + pgroup_b->m_count_entered.load(), pgroup_a->m_count_leaved.load() + pgroup_b->m_count_leaved.load());
	EXPECT_EQ(pgroup_a->m_count_snapshot_error.load() + pgroup_b->m_count_snapshot_error.load(), 0);
	for (auto& iter : vector_member)
		EXPECT_EQ(iter->reference_count(), 1);
}

// - for_each_member_snapshot : leave_group in _function, stop by false
template <class TGROUP>
void check_cow_for_each_member_snapshot()
{
	auto pgroup = cow_make_group<TGROUP>();
	auto vector_member = make_cow_members(6);
	for (auto& iter : vector_member)
		cow_enter(pgroup.get(), iter.get());

	size_t count = 0;
	EXPECT_TRUE(pgroup->for_each_member_snapshot([&](cow_member* _pmember) { _pmember->leave_group(); ++count; return true; }));
	EXPECT_EQ(count, 6u);
	EXPECT_EQ(pgroup->member_count(), 0u);
	EXPECT_EQ(pgroup->m_count_leaved.load(), 6);

	for (auto& iter : vector_member)
		cow_enter(pgroup.get(), iter.get());

	count = 0;
	EXPECT_FALSE(pgroup->for_each_member_snapshot([&](cow_member*) { return ++count < 3; }));
	EXPECT_EQ(count, 3u);

	EXPECT_EQ(pgroup->m_count_snapshot_error.load(), 0);
	pgroup->leave_all();
}

// - a broadcast with the published snapshot does not wait for the group lock (held by other thread)
template <class TGROUP>
void check_cow_send_does_not_wait_group_lock()
{
	auto pgroup = cow_make_group<TGROUP>();
	auto vector_member = make_cow_members(4);
	for (auto& iter : vector_member)
		cow_enter(pgroup.get(), iter.get());

	// - publish
	EXPECT_EQ(pgroup->get_member_snapshot().size(), 4u);

	std::atomic<bool> is_sent {false};
	std::thread thread_send;
	{
		std::unique_lock lock(pgroup->get_group_lockable());

		thread_send = std::thread([&]() { pgroup->send(cow::make_message(cow::TYPE_STATE)); is_sent = true; });

		for (int i = 0; i < 300 && is_sent.load() == false; ++i)
			std::this_thread::sleep_for(std::chrono::milliseconds(10));

		EXPECT_TRUE(is_sent.load());
	}
	thread_send.join();

	for (auto& iter : vector_member)
		EXPECT_EQ(iter->m_count_send.load(), 1u);

	pgroup->leave_all();
}


// - runs for group::list_sender, group::Nmap_sender, group::array_sender and group::array_seat_sender
TEST(group_cow, list_snapshot_basic)		{ check_cow_snapshot_basic<cow_group>(); }
TEST(group_cow, nmap_snapshot_basic)		{ check_cow_snapshot_basic<cow_group_nmap>(); }
TEST(group_cow, array_snapshot_basic)		{ check_cow_snapshot_basic<cow_group_array>(); }
TEST(group_cow, array_seat_snapshot_basic)		{ check_cow_snapshot_basic<cow_group_array_seat>(); }
TEST(group_cow, list_snapshot_keeps_members)		{ check_cow_snapshot_keeps_members<cow_group>(); }
TEST(group_cow, nmap_snapshot_keeps_members)		{ check_cow_snapshot_keeps_members<cow_group_nmap>(); }
TEST(group_cow, array_snapshot_keeps_members)		{ check_cow_snapshot_keeps_members<cow_group_array>(); }
TEST(group_cow, array_seat_snapshot_keeps_members)		{ check_cow_snapshot_keeps_members<cow_group_array_seat>(); }
TEST(group_cow, list_iterate_while_enter_leave)		{ check_cow_iterate_while_enter_leave<cow_group>(); }
TEST(group_cow, nmap_iterate_while_enter_leave)		{ check_cow_iterate_while_enter_leave<cow_group_nmap>(); }
TEST(group_cow, array_iterate_while_enter_leave)		{ check_cow_iterate_while_enter_leave<cow_group_array>(); }
TEST(group_cow, array_seat_iterate_while_enter_leave)		{ check_cow_iterate_while_enter_leave<cow_group_array_seat>(); }
TEST(group_cow, list_send_in_event_sees_live_members)		{ check_cow_send_in_event_sees_live_members<cow_group>(); }
TEST(group_cow, nmap_send_in_event_sees_live_members)		{ check_cow_send_in_event_sees_live_members<cow_group_nmap>(); }
TEST(group_cow, array_send_in_event_sees_live_members)		{ check_cow_send_in_event_sees_live_members<cow_group_array>(); }
TEST(group_cow, array_seat_send_in_event_sees_live_members)		{ check_cow_send_in_event_sees_live_members<cow_group_array_seat>(); }
TEST(group_cow, list_concurrent_send_enter_leave)		{ check_cow_concurrent<cow_group>(false); }
TEST(group_cow, nmap_concurrent_send_enter_leave)		{ check_cow_concurrent<cow_group_nmap>(false); }
TEST(group_cow, array_concurrent_send_enter_leave)		{ check_cow_concurrent<cow_group_array>(false); }
TEST(group_cow, array_seat_concurrent_send_enter_leave)		{ check_cow_concurrent<cow_group_array_seat>(false); }
TEST(group_cow, list_concurrent_send_slow_entered)		{ check_cow_concurrent<cow_group>(true); }
TEST(group_cow, nmap_concurrent_send_slow_entered)		{ check_cow_concurrent<cow_group_nmap>(true); }
TEST(group_cow, array_concurrent_send_slow_entered)		{ check_cow_concurrent<cow_group_array>(true); }
TEST(group_cow, array_seat_concurrent_send_slow_entered)		{ check_cow_concurrent<cow_group_array_seat>(true); }
TEST(group_cow, list_concurrent_move_between_groups)		{ check_cow_concurrent_move_between_groups<cow_group>(); }
TEST(group_cow, nmap_concurrent_move_between_groups)		{ check_cow_concurrent_move_between_groups<cow_group_nmap>(); }
TEST(group_cow, array_concurrent_move_between_groups)		{ check_cow_concurrent_move_between_groups<cow_group_array>(); }
TEST(group_cow, array_seat_concurrent_move_between_groups)		{ check_cow_concurrent_move_between_groups<cow_group_array_seat>(); }
TEST(group_cow, list_for_each_member_snapshot)		{ check_cow_for_each_member_snapshot<cow_group>(); }
TEST(group_cow, nmap_for_each_member_snapshot)		{ check_cow_for_each_member_snapshot<cow_group_nmap>(); }
TEST(group_cow, array_for_each_member_snapshot)		{ check_cow_for_each_member_snapshot<cow_group_array>(); }
TEST(group_cow, array_seat_for_each_member_snapshot)		{ check_cow_for_each_member_snapshot<cow_group_array_seat>(); }
TEST(group_cow, list_send_does_not_wait_group_lock)		{ check_cow_send_does_not_wait_group_lock<cow_group>(); }
TEST(group_cow, nmap_send_does_not_wait_group_lock)		{ check_cow_send_does_not_wait_group_lock<cow_group_nmap>(); }
TEST(group_cow, array_send_does_not_wait_group_lock)		{ check_cow_send_does_not_wait_group_lock<cow_group_array>(); }
TEST(group_cow, array_seat_send_does_not_wait_group_lock)		{ check_cow_send_does_not_wait_group_lock<cow_group_array_seat>(); }

// - Nmap : the old member of the same key is replaced in one write scope (a snapshot has the old one or the new one)
class cow_nmap_replace : public cow_group_nmap
{
public:
	using cow_group_nmap::cow_group_nmap;
protected:
	virtual	result_code			on_member_duplicated(const int64_t& /*_key*/, cow_member* /*_pmember_new*/, cow_member* /*_pmember_old*/, sMESSAGE& /*_param*/) override { return eRESULT::SUCCESS; }
};

TEST(group_cow, nmap_duplicated_key_replaces)
{
	auto pgroup = make_group<cow_nmap_replace>(SIZE_MAX);
	auto vector_member = make_cow_members(3);
	vector_member[2]->m_key = vector_member[0]->m_key;

	pgroup->enter(vector_member[0].get());
	pgroup->enter(vector_member[1].get());
	auto snapshot_old = pgroup->get_member_snapshot();
	ASSERT_EQ(snapshot_old.size(), 2u);

	// - member 2 replaces member 0 (same key)
	EXPECT_EQ(pgroup->enter(vector_member[2].get()), eRESULT::SUCCESS);
	auto snapshot_new = pgroup->get_member_snapshot();
	EXPECT_EQ(snapshot_new.size(), 2u);
	EXPECT_EQ(std::find(snapshot_new.begin(), snapshot_new.end(), vector_member[0].get()), snapshot_new.end());
	EXPECT_NE(std::find(snapshot_new.begin(), snapshot_new.end(), vector_member[2].get()), snapshot_new.end());
	EXPECT_TRUE(vector_member[0]->get_group().empty());

	// - the old snapshot is not changed
	EXPECT_EQ(snapshot_old.size(), 2u);
	EXPECT_NE(std::find(snapshot_old.begin(), snapshot_old.end(), vector_member[0].get()), snapshot_old.end());

	EXPECT_EQ(pgroup->m_count_entered.load(), 3);
	EXPECT_EQ(pgroup->m_count_leaved.load(), 1);
	EXPECT_EQ(pgroup->m_count_snapshot_error.load(), 0);
	pgroup->leave_all();
}


// ============================================================================
// copy-on-write snapshot of observable::* / container::* (group lock stage 3)
//   - get_observer_snapshot() / get_object_snapshot() : same as get_member_snapshot() of the groups
//   - subscribe / register_object, unsubscribe / unregister_object change the list (the events run in the lock)
// ============================================================================
// 1) receivers
class cow_observer :
	public						Iobserver<cow_observer>,
	public						net::io::Isender,
	public						cow_receive_check
{
public:
	virtual	bool				process_send(shared_buffer&& _buffer, std::size_t _count_message, uint64_t /*_option*/) override { this->on_receive(_buffer, _count_message); return true; }
};

class cow_object :
	public						net::io::Isender,
	public						cow_receive_check
{
public:
	virtual	bool				process_send(shared_buffer&& _buffer, std::size_t _count_message, uint64_t /*_option*/) override { this->on_receive(_buffer, _count_message); return true; }
};

// 2) subjects : send STATE(cycle) in the 'registered' event, check the snapshots seen in the events
template <class TSNAPSHOT, class TRECEIVER>
size_t cow_count_in(const TSNAPSHOT& _snapshot, const TRECEIVER* _preceiver)
{
	return static_cast<size_t>(std::count(_snapshot.begin(), _snapshot.end(), _preceiver));
}

template <class TRECEIVER>
void cow_send_state(TRECEIVER* _preceiver)
{
	if (_preceiver->m_pphase != nullptr)
		_preceiver->send(cow::make_message(cow::TYPE_STATE, nullptr, _preceiver->m_pphase->load(std::memory_order_acquire) / 2));
}

template <class TBASE>
class cow_observable_t : public TBASE
{
public:
	using TBASE::TBASE;
	using observer_channel_t = typename TBASE::observer_channel_t;
	using observer_data_t = typename TBASE::observer_data_t;

public:
			std::atomic<int64_t> m_count_registered {0};
			std::atomic<int64_t> m_count_unregistered {0};
			std::atomic<int64_t> m_count_snapshot_error {0};
			size_t				m_count_before_unregister {0};	// in the lock

protected:
	virtual	void				on_observer_registered(cow_observer* _pobserver, observer_channel_t /*_ocid*/, const observer_data_t& /*_observer_data*/) override
	{
		++this->m_count_registered;
		auto snapshot = this->get_observer_snapshot();
		if (cow_count_in(snapshot, _pobserver) == 0 || snapshot.size() != this->count())
			++this->m_count_snapshot_error;
		cow_send_state(_pobserver);
	}
	virtual	uintptr_t			on_observer_unregistering(cow_observer* _pobserver, observer_channel_t /*_ocid*/) override
	{
		this->m_count_before_unregister = cow_count_in(this->get_observer_snapshot(), _pobserver);
		if (this->m_count_before_unregister == 0)
			++this->m_count_snapshot_error;
		return 0;
	}
	virtual	void				on_observer_unregistered(cow_observer* _pobserver, observer_channel_t /*_ocid*/, uintptr_t /*_result*/) override
	{
		++this->m_count_unregistered;
		if (cow_count_in(this->get_observer_snapshot(), _pobserver) + 1 != this->m_count_before_unregister)
			++this->m_count_snapshot_error;
	}
};

template <class TBASE>
class cow_container_t : public TBASE
{
public:
	using TBASE::TBASE;
	using object_data_t = typename TBASE::object_data_t;

public:
			std::atomic<int64_t> m_count_registered {0};
			std::atomic<int64_t> m_count_unregistered {0};
			std::atomic<int64_t> m_count_snapshot_error {0};

protected:
	virtual	void				on_object_registered(cow_object* _pobject, const object_data_t& /*_object_data*/) override
	{
		++this->m_count_registered;
		auto snapshot = this->get_object_snapshot();
		if (cow_count_in(snapshot, _pobject) != 1 || snapshot.size() != this->count())
			++this->m_count_snapshot_error;
		cow_send_state(_pobject);
	}
	virtual	uintptr_t			on_object_unregistering(cow_object* _pobject) override
	{
		if (cow_count_in(this->get_object_snapshot(), _pobject) != 1)
			++this->m_count_snapshot_error;
		return 0;
	}
	virtual	void				on_object_unregistered(cow_object* _pobject, uintptr_t /*_result*/) override
	{
		++this->m_count_unregistered;
		if (cow_count_in(this->get_object_snapshot(), _pobject) != 0)
			++this->m_count_snapshot_error;
	}
};

// 3) ops : register / unregister / snapshot / lock (IS_MAP : the key of the receiver)
template <bool IS_MAP>
struct cow_observable_ops
{
	using receiver_t = cow_observer;
	template <class T> static result_code	reg(T* _p, cow_observer* _r) { if constexpr (IS_MAP) return _p->subscribe(_r->m_key, _r, 0); else return _p->subscribe(_r, 0); }
	template <class T> static void			unreg(T*, cow_observer* _r) { _r->unsubscribe(0); }
	template <class T> static auto			snapshot(T* _p) { return _p->get_observer_snapshot(); }
	template <class T> static lock_object&	lockable(T* _p) { return _p->get_observer_lockable(); }
	template <class T, class F> static bool	for_each_snapshot(T* _p, F&& _f) { return _p->for_each_snapshot(std::forward<F>(_f)); }
};

template <bool IS_MAP>
struct cow_container_ops
{
	using receiver_t = cow_object;
	template <class T> static result_code	reg(T* _p, cow_object* _r) { if constexpr (IS_MAP) return _p->register_object(_r->m_key, _r); else return _p->register_object(_r); }
	template <class T> static void			unreg(T* _p, cow_object* _r) { _p->unregister_object(_r); }
	template <class T> static auto			snapshot(T* _p) { return _p->get_object_snapshot(); }
	template <class T> static lock_object&	lockable(T* _p) { return static_cast<lock_object&>(*_p); }
	template <class T, class F> static bool	for_each_snapshot(T* _p, F&& _f) { return _p->for_each_snapshot(std::forward<F>(_f)); }
};

using cow_observable_list	 = cow_observable_t<observable::list_sender<cow_observer>>;
using cow_observable_set	 = cow_observable_t<observable::set_sender<cow_observer>>;
using cow_observable_map	 = cow_observable_t<observable::map_sender<cow_observer, int64_t>>;
using cow_container_list	 = cow_container_t<container::list_sender<cow_object>>;
using cow_container_set		 = cow_container_t<container::set_sender<cow_object>>;
using cow_container_map		 = cow_container_t<container::map_sender<cow_object, int64_t>>;

template <class TRECEIVER>
std::vector<object_ptr<TRECEIVER>> make_cow_receivers(size_t _count)
{
	std::vector<object_ptr<TRECEIVER>> vector_receiver;
	for (size_t i = 0; i < _count; ++i)
	{
		auto preceiver = make_object<TRECEIVER>();
		preceiver->m_index = i % cow::COUNT_PHASE;
		preceiver->m_key = static_cast<int64_t>(i);
		vector_receiver.push_back(std::move(preceiver));
	}
	return vector_receiver;
}

// 4) snapshot : shared while not changed, immutable, follows register / unregister
template <class TSUBJECT, class TOPS>
void check_cow_subject_basic()
{
	auto psubject = make_object<TSUBJECT>();
	auto vector_receiver = make_cow_receivers<typename TOPS::receiver_t>(4);

	EXPECT_EQ(TOPS::snapshot(psubject.get()).size(), 0u);

	for (size_t i = 0; i < 3; ++i)
		EXPECT_EQ(TOPS::reg(psubject.get(), vector_receiver[i].get()), eRESULT::SUCCESS);

	auto snapshot_1 = TOPS::snapshot(psubject.get());
	auto snapshot_2 = TOPS::snapshot(psubject.get());
	ASSERT_EQ(snapshot_1.size(), 3u);
	EXPECT_EQ(snapshot_1.get(), snapshot_2.get());
	for (size_t i = 0; i < 3; ++i)
		EXPECT_EQ(cow_count_in(snapshot_1, vector_receiver[i].get()), 1u);

	// - register : a new snapshot, the old one is not changed
	EXPECT_EQ(TOPS::reg(psubject.get(), vector_receiver[3].get()), eRESULT::SUCCESS);
	auto snapshot_3 = TOPS::snapshot(psubject.get());
	EXPECT_NE(snapshot_3.get(), snapshot_1.get());
	EXPECT_EQ(snapshot_3.size(), 4u);
	EXPECT_EQ(snapshot_1.size(), 3u);

	// - unregister
	TOPS::unreg(psubject.get(), vector_receiver[0].get());
	auto snapshot_4 = TOPS::snapshot(psubject.get());
	EXPECT_EQ(snapshot_4.size(), 3u);
	EXPECT_EQ(cow_count_in(snapshot_4, vector_receiver[0].get()), 0u);
	EXPECT_EQ(psubject->count(), snapshot_4.size());

	// - send : the receivers now registered
	psubject->send(cow::make_message(cow::TYPE_STATE));
	EXPECT_EQ(vector_receiver[0]->m_count_send.load(), 0u);
	for (size_t i = 1; i < 4; ++i)
		EXPECT_EQ(vector_receiver[i]->m_count_send.load(), 1u);

	for (size_t i = 1; i < 4; ++i)
		TOPS::unreg(psubject.get(), vector_receiver[i].get());

	EXPECT_EQ(TOPS::snapshot(psubject.get()).size(), 0u);
	EXPECT_EQ(psubject->m_count_registered.load(), 4);
	EXPECT_EQ(psubject->m_count_unregistered.load(), 4);
	EXPECT_EQ(psubject->m_count_snapshot_error.load(), 0);

	// - the old snapshots keep the receivers alive
	snapshot_1 = decltype(snapshot_1)();
	snapshot_2 = decltype(snapshot_2)();
	snapshot_3 = decltype(snapshot_3)();
	snapshot_4 = decltype(snapshot_4)();
	for (auto& iter : vector_receiver)
		EXPECT_EQ(iter->reference_count(), 1);
}

// 5) for_each_snapshot : unregister in _function, stop by false
template <class TSUBJECT, class TOPS>
void check_cow_subject_for_each_snapshot()
{
	auto psubject = make_object<TSUBJECT>();
	auto vector_receiver = make_cow_receivers<typename TOPS::receiver_t>(5);
	for (auto& iter : vector_receiver)
		TOPS::reg(psubject.get(), iter.get());

	size_t count = 0;
	EXPECT_TRUE(TOPS::for_each_snapshot(psubject.get(), [&](typename TOPS::receiver_t* _preceiver) { TOPS::unreg(psubject.get(), _preceiver); ++count; return true; }));
	EXPECT_EQ(count, 5u);
	EXPECT_EQ(psubject->count(), 0u);

	for (auto& iter : vector_receiver)
		TOPS::reg(psubject.get(), iter.get());
	count = 0;
	EXPECT_FALSE(TOPS::for_each_snapshot(psubject.get(), [&](typename TOPS::receiver_t*) { return ++count < 2; }));
	EXPECT_EQ(count, 2u);

	for (auto& iter : vector_receiver)
		TOPS::unreg(psubject.get(), iter.get());
	EXPECT_EQ(psubject->m_count_snapshot_error.load(), 0);
}

// 6) a send with the published snapshot does not wait for the lock (held by other thread)
template <class TSUBJECT, class TOPS>
void check_cow_subject_send_does_not_wait_lock()
{
	auto psubject = make_object<TSUBJECT>();
	auto vector_receiver = make_cow_receivers<typename TOPS::receiver_t>(4);
	for (auto& iter : vector_receiver)
		TOPS::reg(psubject.get(), iter.get());

	EXPECT_EQ(TOPS::snapshot(psubject.get()).size(), 4u);

	std::atomic<bool> is_sent {false};
	std::thread thread_send;
	{
		std::unique_lock lock(TOPS::lockable(psubject.get()));

		thread_send = std::thread([&]() { psubject->send(cow::make_message(cow::TYPE_STATE)); is_sent = true; });

		for (int i = 0; i < 300 && is_sent.load() == false; ++i)
			std::this_thread::sleep_for(std::chrono::milliseconds(10));

		EXPECT_TRUE(is_sent.load());
	}
	thread_send.join();

	for (auto& iter : vector_receiver)
		EXPECT_EQ(iter->m_count_send.load(), 1u);

	for (auto& iter : vector_receiver)
		TOPS::unreg(psubject.get(), iter.get());
}

// 7) concurrent send + register / unregister
//    . DELTA from a send that started after unregister returned is not delivered
//    . DELTA of a cycle is not delivered before the STATE sent in the 'registered' event of the cycle
template <class TSUBJECT, class TOPS>
void check_cow_subject_concurrent()
{
	const size_t COUNT_RECEIVER = cow::COUNT_PHASE;
	const size_t COUNT_CHURN = 4;
	const size_t COUNT_SENDER = 4;

	auto psubject = make_object<TSUBJECT>();
	auto vector_receiver = make_cow_receivers<typename TOPS::receiver_t>(COUNT_RECEIVER);

	std::atomic<uint32_t> phase[cow::COUNT_PHASE];
	for (size_t i = 0; i < cow::COUNT_PHASE; ++i)
		phase[i].store(1);
	for (size_t i = 0; i < COUNT_RECEIVER; ++i)
		vector_receiver[i]->m_pphase = &phase[i];

	std::atomic<bool> is_stop {false};
	std::atomic<size_t> count_send {0};

	run_with_deadlock_watchdog([&]()
	{
		std::vector<std::thread> vector_thread;
		for (size_t t = 0; t < COUNT_CHURN; ++t)
		{
			vector_thread.emplace_back([&, t]()
			{
				uint32_t cycle = 1;
				while (is_stop.load() == false)
				{
					for (size_t i = t; i < COUNT_RECEIVER; i += COUNT_CHURN)
					{
						phase[i].store(2 * cycle, std::memory_order_release);
						TOPS::reg(psubject.get(), vector_receiver[i].get());
						std::this_thread::yield();
						TOPS::unreg(psubject.get(), vector_receiver[i].get());
						phase[i].store(2 * cycle + 1, std::memory_order_release);
					}
					++cycle;
				}
			});
		}
		for (size_t t = 0; t < COUNT_SENDER; ++t)
		{
			vector_thread.emplace_back([&, t]()
			{
				while (is_stop.load() == false)
				{
					auto buf = cow::make_message(cow::TYPE_DELTA, phase);
					if ((t & 1) == 0)
						psubject->send(buf);
					else
						psubject->send_except(nullptr, buf);
					++count_send;
				}
			});
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		is_stop = true;
		for (auto& iter : vector_thread)
			iter.join();
	}, std::chrono::seconds(60));

	size_t after_leave = 0, before_state = 0, delivered = 0;
	for (auto& iter : vector_receiver)
	{
		after_leave += iter->m_count_after_leave.load();
		before_state += iter->m_count_before_state.load();
		delivered += iter->m_count_send.load();
	}

	EXPECT_GT(count_send.load(), 0u);
	EXPECT_GT(delivered, 0u);
	EXPECT_EQ(after_leave, 0u);
	EXPECT_EQ(before_state, 0u);
	EXPECT_EQ(psubject->m_count_snapshot_error.load(), 0);
	EXPECT_EQ(psubject->m_count_registered.load(), psubject->m_count_unregistered.load());
	EXPECT_EQ(psubject->count(), 0u);
	EXPECT_EQ(TOPS::snapshot(psubject.get()).size(), 0u);
}

// 8) observable : an observer subscribed by 2 channels is in the snapshot twice (and receives twice)
TEST(group_cow, observable_two_channels)
{
	auto pobservable = make_object<cow_observable_list>();
	auto vector_observer = make_cow_receivers<cow_observer>(2);
	EXPECT_EQ(pobservable->subscribe(vector_observer[0].get(), 0), eRESULT::SUCCESS);
	EXPECT_EQ(pobservable->subscribe(vector_observer[0].get(), 1), eRESULT::SUCCESS);
	EXPECT_EQ(pobservable->subscribe(vector_observer[1].get(), 0), eRESULT::SUCCESS);

	auto snapshot = pobservable->get_observer_snapshot();
	EXPECT_EQ(snapshot.size(), 3u);
	EXPECT_EQ(cow_count_in(snapshot, vector_observer[0].get()), 2u);

	pobservable->send(cow::make_message(cow::TYPE_STATE));
	EXPECT_EQ(vector_observer[0]->m_count_send.load(), 2u);
	EXPECT_EQ(vector_observer[1]->m_count_send.load(), 1u);

	// - send_except : all the channels of the observer are excluded
	pobservable->send_except(vector_observer[0].get(), cow::make_message(cow::TYPE_STATE));
	EXPECT_EQ(vector_observer[0]->m_count_send.load(), 2u);
	EXPECT_EQ(vector_observer[1]->m_count_send.load(), 2u);

	vector_observer[0]->unsubscribe(1);
	EXPECT_EQ(cow_count_in(pobservable->get_observer_snapshot(), vector_observer[0].get()), 1u);

	for (auto& iter : vector_observer)
		iter->unsubscribe_all();
	EXPECT_EQ(pobservable->get_observer_snapshot().size(), 0u);
	EXPECT_EQ(pobservable->m_count_snapshot_error.load(), 0);
}

#define COW_SUBJECT_TESTS(name, subject, ops) \
	TEST(group_cow, name##_basic)						{ check_cow_subject_basic<subject, ops>(); } \
	TEST(group_cow, name##_for_each_snapshot)			{ check_cow_subject_for_each_snapshot<subject, ops>(); } \
	TEST(group_cow, name##_send_does_not_wait_lock)		{ check_cow_subject_send_does_not_wait_lock<subject, ops>(); } \
	TEST(group_cow, name##_concurrent_send_register)	{ check_cow_subject_concurrent<subject, ops>(); }

COW_SUBJECT_TESTS(observable_list, cow_observable_list, cow_observable_ops<false>)
COW_SUBJECT_TESTS(observable_set,  cow_observable_set,  cow_observable_ops<false>)
COW_SUBJECT_TESTS(observable_map,  cow_observable_map,  cow_observable_ops<true>)
COW_SUBJECT_TESTS(container_list,  cow_container_list,  cow_container_ops<false>)
COW_SUBJECT_TESTS(container_set,   cow_container_set,   cow_container_ops<false>)
COW_SUBJECT_TESTS(container_map,   cow_container_map,   cow_container_ops<true>)

// ============================================================================
// user code out of the group lock (remaining defects of the group lock)
//   - for_each_member / find_member call the callback over the member snapshot out of the group lock.
//     (a callback locking another member deadlocked with a thread moving that member between groups :
//      group lock -> member lock vs member lock -> enter lock -> group lock)
//   - group::array / array_seat : member_capacity() with members (the seats were re-created under the members)
//     and seats out of the range
//   - container::* / observable::* : for_each / find_* out of the lock, the object is held while the events run,
//     unregister_object(raw) makes no object_ptr of the raw pointer before it is found
// ============================================================================
template <class TGROUP>
void check_user_code_locks_other_member()
{
	const size_t COUNT = 16;
	auto pgroup_a = make_group<TGROUP>(COUNT);
	auto pgroup_b = make_group<TGROUP>(COUNT);
	auto vector_member = make_members(COUNT);
	for (size_t i = 0; i < COUNT; ++i)
		enter_member(pgroup_a.get(), vector_member[i].get(), static_cast<int64_t>(i));

	std::atomic<int64_t> count_exception {0};
	std::atomic<int64_t> count_visit {0};

	// - locks another member in the callback (member lock)
	auto lock_other = [&](uint32_t& _state)
	{
		auto* pother = vector_member[next_random(_state) % COUNT].get();
		std::lock_guard lock(pother->m_lockable_group);
		(void)pother->get_group();
		++count_visit;
	};

	// - moves a member to the other group (member lock -> enter lock -> group lock of the both groups)
	auto move = [&](uint32_t& _state)
	{
		auto* pmember = vector_member[next_random(_state) % COUNT].get();
		if (pmember->is_member_of(pgroup_a.get()))
			enter_member(pgroup_b.get(), pmember, pmember->m_key);
		else
			enter_member(pgroup_a.get(), pmember, pmember->m_key);
	};

	auto vector_count = run_threads_repeatedly(
	{
		[&, state = uint32_t(0x1234567)]() mutable { move(state); },
		[&, state = uint32_t(0x2345678)]() mutable { move(state); },
		[&, state = uint32_t(0x3456789)]() mutable { pgroup_a->for_each_member([&](member*) { lock_other(state); return true; }); },
		[&, state = uint32_t(0x456789a)]() mutable { pgroup_b->for_each_member([&](member*) { lock_other(state); return true; }); },
		[&, state = uint32_t(0x56789ab)]() mutable { (void)pgroup_a->find_member([&](member*) { lock_other(state); return false; }); },

		// - leaves in the callback (the snapshot keeps the members)
		[&, state = uint32_t(0x6789abc)]() mutable { pgroup_b->for_each_member([&](member* _pmember) { if ((next_random(state) & 7) == 0) _pmember->leave_group(); return true; }); },
	}, count_exception);

	for (auto iter : vector_count)
		EXPECT_GT(iter, 0);
	EXPECT_EQ(count_exception.load(), 0);
	EXPECT_GT(count_visit.load(), 0);

	pgroup_a->leave_all();
	pgroup_b->leave_all();

	EXPECT_EQ(pgroup_a->member_count(), 0u);
	EXPECT_EQ(pgroup_b->member_count(), 0u);
	EXPECT_EQ(pgroup_a->m_count_entered.load(), pgroup_a->m_count_leaved.load());
	EXPECT_EQ(pgroup_b->m_count_entered.load(), pgroup_b->m_count_leaved.load());
	for (auto& iter : vector_member)
		EXPECT_TRUE(iter->get_group().empty());
}

// - for_each_member / find_member in the events see the live members
template <class TGROUP>
class user_code_in_event : public TGROUP
{
public:
	using TGROUP::TGROUP;

public:
			std::atomic<int64_t> m_count_error {0};

protected:
	virtual	void				on_member_entered(member* _pmember, sMESSAGE& _param) override
	{
		TGROUP::on_member_entered(_pmember, _param);

		size_t count = 0;
		bool is_found = false;
		this->for_each_member([&](member* _pmember_iter) { ++count; is_found |= (_pmember_iter == _pmember); return true; });
		if (is_found == false || count != this->member_count())
			++this->m_count_error;
	}
	virtual	uintptr_t			on_member_leaving(member* _pmember, uintptr_t _param) override
	{
		if (this->find_member([&](member* _pmember_iter) { return _pmember_iter == _pmember; }).empty())
			++this->m_count_error;
		return TGROUP::on_member_leaving(_pmember, _param);
	}
};

template <class TGROUP>
void check_for_each_member_semantics()
{
	const size_t COUNT = 8;
	auto pgroup = make_group<user_code_in_event<TGROUP>>(COUNT);

	{
		auto vector_member = make_members(COUNT);
		for (size_t i = 0; i < COUNT; ++i)
			enter_member(pgroup.get(), vector_member[i].get(), static_cast<int64_t>(i));
		EXPECT_EQ(pgroup->member_count(), COUNT);

		// 1) all the members. stops at false
		size_t count = 0;
		EXPECT_TRUE(pgroup->for_each_member([&](member*) { ++count; return true; }));
		EXPECT_EQ(count, COUNT);

		count = 0;
		EXPECT_FALSE(pgroup->for_each_member([&](member*) { return ++count < 3; }));
		EXPECT_EQ(count, 3u);

		// 2) find_member
		EXPECT_TRUE(pgroup->find_member([](member* _pmember) { return _pmember->m_key == 5; }) == vector_member[5]);
		EXPECT_TRUE(pgroup->find_member([](member*) { return false; }).empty());

		// 3) the user code may lock the group (recursive)
		count = 0;
		SCOPED_LOCK(pgroup->get_group_lockable())
		{
			EXPECT_TRUE(pgroup->for_each_member([&](member*) { ++count; return true; }));
		}
		EXPECT_EQ(count, COUNT);
	}

	// 4) all the members leave in the callback. the group has the last references : the members of the snapshot are alive
	std::vector<int64_t> vector_key;
	EXPECT_TRUE(pgroup->for_each_member([&](member* _pmember)
	{
		if (vector_key.empty())
			pgroup->leave_all();
		vector_key.push_back(_pmember->m_key);
		return true;
	}));
	std::sort(vector_key.begin(), vector_key.end());
	EXPECT_EQ(vector_key.size(), COUNT);
	for (size_t i = 0; i < vector_key.size(); ++i)
		EXPECT_EQ(vector_key[i], static_cast<int64_t>(i));

	EXPECT_EQ(pgroup->member_count(), 0u);
	EXPECT_EQ(pgroup->m_count_error.load(), 0);
	EXPECT_EQ(pgroup->m_count_entered.load(), pgroup->m_count_leaved.load());
}

TEST(group_user_code, list_callback_locks_other_member)			{ check_user_code_locks_other_member<group_list>(); }
TEST(group_user_code, nmap_callback_locks_other_member)			{ check_user_code_locks_other_member<group_nmap>(); }
TEST(group_user_code, array_callback_locks_other_member)		{ check_user_code_locks_other_member<group_array>(); }
TEST(group_user_code, array_seat_callback_locks_other_member)	{ check_user_code_locks_other_member<group_array_seat>(); }
TEST(group_user_code, list_for_each_member)						{ check_for_each_member_semantics<group_list>(); }
TEST(group_user_code, nmap_for_each_member)						{ check_for_each_member_semantics<group_nmap>(); }
TEST(group_user_code, array_for_each_member)					{ check_for_each_member_semantics<group_array>(); }
TEST(group_user_code, array_seat_for_each_member)				{ check_for_each_member_semantics<group_array_seat>(); }

// - member_capacity() with members throws (the seats were re-created under the members : dangling member_set). seats out of the range
template <class TGROUP>
void check_array_capacity_with_members()
{
	const size_t CAPACITY = 4;
	auto pgroup = make_group<TGROUP>(CAPACITY);
	auto vector_member = make_members(2);
	for (size_t i = 0; i < vector_member.size(); ++i)
		enter_member(pgroup.get(), vector_member[i].get(), static_cast<int64_t>(i));

	// 1) with members
	EXPECT_THROW(pgroup->member_capacity(8), throwable::failure);
	EXPECT_EQ(pgroup->member_capacity(), CAPACITY);
	EXPECT_EQ(pgroup->member_count(), 2u);
	for (size_t i = 0; i < vector_member.size(); ++i)
	{
		EXPECT_TRUE(pgroup->member(static_cast<group_member_id_t>(i)) == vector_member[i]);
		EXPECT_EQ(pgroup->member_serial(vector_member[i].get()), static_cast<group_member_id_t>(i));
	}

	// 2) seats out of the range
	EXPECT_TRUE(pgroup->leave_by_seat(-1, 0).empty());
	EXPECT_TRUE(pgroup->leave_by_seat(static_cast<group_member_id_t>(CAPACITY), 0).empty());
	EXPECT_TRUE(pgroup->member(-1).empty());
	EXPECT_TRUE(pgroup->member(static_cast<group_member_id_t>(CAPACITY)).empty());
	EXPECT_EQ(pgroup->member_count(), 2u);

	// 3) without members
	pgroup->leave_all();
	pgroup->member_capacity(8);
	EXPECT_EQ(pgroup->member_capacity(), 8u);
	enter_member(pgroup.get(), vector_member[0].get(), 7);
	EXPECT_TRUE(vector_member[0]->is_member_of(pgroup.get()));

	pgroup->leave_all();
	EXPECT_EQ(pgroup->member_count(), 0u);
	EXPECT_EQ(pgroup->m_count_entered.load(), pgroup->m_count_leaved.load());
}

TEST(group_user_code, array_capacity_with_members)				{ check_array_capacity_with_members<group_array>(); }
TEST(group_user_code, array_seat_capacity_with_members)			{ check_array_capacity_with_members<group_array_seat>(); }

// - leave_by_seat races member_capacity() of the empty group (std::out_of_range in noexcept leave_by_seat)
template <class TGROUP>
void check_array_capacity_races_seat()
{
	auto pgroup = make_group<TGROUP>(8);
	auto vector_member = make_members(1);
	std::atomic<int64_t> count_exception {0};

	auto vector_count = run_threads_repeatedly(
	{
		[&]() { enter_member(pgroup.get(), vector_member[0].get(), 7); (void)pgroup->leave_by_seat(7, 0); pgroup->leave_all(); },
		[&]() { (void)pgroup->leave_by_seat(7, 0); (void)pgroup->member(7); },
		[&]() { try { pgroup->member_capacity(4); pgroup->member_capacity(8); } catch (throwable::failure&) {} },
	}, count_exception);

	for (auto iter : vector_count)
		EXPECT_GT(iter, 0);
	EXPECT_EQ(count_exception.load(), 0);

	pgroup->leave_all();
	EXPECT_EQ(pgroup->m_count_entered.load(), pgroup->m_count_leaved.load());
}

TEST(group_user_code, array_capacity_races_seat)				{ check_array_capacity_races_seat<group_array>(); }
TEST(group_user_code, array_seat_capacity_races_seat)			{ check_array_capacity_races_seat<group_array_seat>(); }

// 1) objects checking their life
const uint32_t TRACKED_MAGIC = 0x7a11ce5d;

class tracked_object : public net::io::Isender
{
public:
								tracked_object() noexcept { ++s_count_alive; }
	virtual						~tracked_object() noexcept { if (this->m_on_destroy) this->m_on_destroy(this); this->m_magic = 0; --s_count_alive; }

	virtual	bool				process_send(shared_buffer&& /*_buffer*/, std::size_t /*_count_message*/, uint64_t /*_option*/) override { return true; }

public:
	volatile uint32_t			m_magic = TRACKED_MAGIC;
			std::function<void (tracked_object*)> m_on_destroy;
	static inline std::atomic<int64_t> s_count_alive {0};
};

class tracked_observer :
	public						Iobserver<tracked_observer>,
	public						net::io::Isender
{
public:
								tracked_observer() noexcept { ++s_count_alive; }
	virtual						~tracked_observer() noexcept { this->m_magic = 0; --s_count_alive; }

	virtual	bool				process_send(shared_buffer&& /*_buffer*/, std::size_t /*_count_message*/, uint64_t /*_option*/) override { return true; }

public:
	volatile uint32_t			m_magic = TRACKED_MAGIC;
	static inline std::atomic<int64_t> s_count_alive {0};
};

// 2) containers / observables checking the object in the events
template <class TBASE>
class checking_container : public TBASE
{
public:
	using TBASE::TBASE;

public:
			std::atomic<int64_t> m_count_dead {0};
			std::atomic<int64_t> m_count_unregistered {0};
			std::function<void (tracked_object*)> m_on_unregistered;

protected:
	virtual	uintptr_t			on_object_unregistering(tracked_object* _pobject) override { if (_pobject->m_magic != TRACKED_MAGIC) ++this->m_count_dead; return 0; }
	virtual	void				on_object_unregistered(tracked_object* _pobject, uintptr_t /*_result*/) override
	{
		++this->m_count_unregistered;
		if (_pobject->m_magic != TRACKED_MAGIC)
			++this->m_count_dead;
		if (this->m_on_unregistered)
			this->m_on_unregistered(_pobject);
	}
};

template <class TBASE>
class checking_observable : public TBASE
{
public:
	using TBASE::TBASE;
	using observer_channel_t = typename TBASE::observer_channel_t;

public:
			std::atomic<int64_t> m_count_dead {0};
			std::atomic<int64_t> m_count_unregistered {0};

protected:
	virtual	uintptr_t			on_observer_unregistering(tracked_observer* _pobserver, observer_channel_t /*_ocid*/) override { if (_pobserver->m_magic != TRACKED_MAGIC) ++this->m_count_dead; return 0; }
	virtual	void				on_observer_unregistered(tracked_observer* _pobserver, observer_channel_t /*_ocid*/, uintptr_t /*_result*/) override
	{
		++this->m_count_unregistered;
		if (_pobserver->m_magic != TRACKED_MAGIC)
			++this->m_count_dead;
	}
};

using checking_container_list	 = checking_container<container::list<tracked_object>>;
using checking_container_set	 = checking_container<container::set<tracked_object>>;
using checking_container_map	 = checking_container<container::map<tracked_object, int64_t>>;
using checking_observable_list	 = checking_observable<observable::list<tracked_observer>>;
using checking_observable_set	 = checking_observable<observable::set<tracked_observer>>;
using checking_observable_map	 = checking_observable<observable::map<tracked_observer, int64_t>>;

inline result_code register_tracked(checking_container_list* _pcontainer, int64_t, tracked_object* _pobject) { return _pcontainer->register_object(_pobject); }
inline result_code register_tracked(checking_container_set* _pcontainer, int64_t, tracked_object* _pobject) { return _pcontainer->register_object(_pobject); }
inline result_code register_tracked(checking_container_map* _pcontainer, int64_t _key, tracked_object* _pobject) { return _pcontainer->register_object(_key, _pobject); }
inline result_code subscribe_tracked(checking_observable_list* _pobservable, int64_t, tracked_observer* _pobserver) { return _pobservable->subscribe(_pobserver); }
inline result_code subscribe_tracked(checking_observable_set* _pobservable, int64_t, tracked_observer* _pobserver) { return _pobservable->subscribe(_pobserver); }
inline result_code subscribe_tracked(checking_observable_map* _pobservable, int64_t _key, tracked_observer* _pobserver) { return _pobservable->subscribe(_key, _pobserver, 0); }

// 3) unregister_object(raw) of the object whose last reference is in the container
//    (on_object_unregistered got the raw pointer after erase released the object)
//    unregister_object(this) in the destructor of an object not registered
//    (list / set made an object_ptr of the object in its final release : double release)
template <class TCONTAINER>
void check_container_unregister_last_reference()
{
	auto pcontainer = make_object<TCONTAINER>();
	const auto count_alive = tracked_object::s_count_alive.load();

	tracked_object* pobject_raw = nullptr;
	{
		auto pobject = make_object<tracked_object>();
		pobject_raw = pobject.get();
		EXPECT_EQ(register_tracked(pcontainer.get(), 1, pobject_raw), eRESULT::SUCCESS);
	}
	EXPECT_EQ(tracked_object::s_count_alive.load(), count_alive + 1);

	EXPECT_TRUE(pcontainer->unregister_object(pobject_raw));
	EXPECT_EQ(pcontainer->m_count_unregistered.load(), 1);
	EXPECT_EQ(pcontainer->m_count_dead.load(), 0);
	EXPECT_EQ(tracked_object::s_count_alive.load(), count_alive);
	EXPECT_EQ(pcontainer->count(), 0u);

	{
		auto pobject = make_object<tracked_object>();
		pobject->m_on_destroy = [&](tracked_object* _pobject) { EXPECT_FALSE(pcontainer->unregister_object(_pobject)); };
	}
	EXPECT_EQ(tracked_object::s_count_alive.load(), count_alive);
}

TEST(group_container_safety, list_unregister_last_reference)	{ check_container_unregister_last_reference<checking_container_list>(); }
TEST(group_container_safety, set_unregister_last_reference)		{ check_container_unregister_last_reference<checking_container_set>(); }
TEST(group_container_safety, map_unregister_last_reference)		{ check_container_unregister_last_reference<checking_container_map>(); }

// 4) for_each / find_object callbacks unregister the object and use it while other threads register / unregister
//    (the container has the only reference of the objects)
template <class TCONTAINER>
void check_container_callback_races_unregister()
{
	const uint32_t COUNT_KEY = 32;
	auto pcontainer = make_object<TCONTAINER>();
	const auto count_alive = tracked_object::s_count_alive.load();

	std::atomic<int64_t> count_exception {0};
	std::atomic<int64_t> count_dead {0};

	auto vector_count = run_threads_repeatedly(
	{
		[&, state = uint32_t(0x1234567)]() mutable { auto pobject = make_object<tracked_object>(); (void)register_tracked(pcontainer.get(), next_random(state) % COUNT_KEY, pobject.get()); },
		[&, state = uint32_t(0x2345678)]() mutable { auto pobject = make_object<tracked_object>(); (void)register_tracked(pcontainer.get(), next_random(state) % COUNT_KEY, pobject.get()); },
		[&]() { pcontainer->for_each([&](tracked_object* _pobject) { pcontainer->unregister_object(_pobject); if (_pobject->m_magic != TRACKED_MAGIC) ++count_dead; return true; }); },
		[&]() { (void)pcontainer->find_object([&](tracked_object* _pobject) { pcontainer->unregister_object(_pobject); if (_pobject->m_magic != TRACKED_MAGIC) ++count_dead; return false; }); },
		[&]() { pcontainer->for_each_snapshot([&](tracked_object* _pobject) { return _pobject->m_magic == TRACKED_MAGIC || (++count_dead, true); }); },
		[&]() { pcontainer->unregister_object_all(); },
	}, count_exception);

	for (auto iter : vector_count)
		EXPECT_GT(iter, 0);
	EXPECT_EQ(count_exception.load(), 0);
	EXPECT_EQ(count_dead.load(), 0);
	EXPECT_EQ(pcontainer->m_count_dead.load(), 0);

	pcontainer->unregister_object_all();
	EXPECT_EQ(pcontainer->count(), 0u);
	EXPECT_EQ(tracked_object::s_count_alive.load(), count_alive);
}

TEST(group_container_safety, list_callback_races_unregister)	{ check_container_callback_races_unregister<checking_container_list>(); }
TEST(group_container_safety, set_callback_races_unregister)		{ check_container_callback_races_unregister<checking_container_set>(); }
TEST(group_container_safety, map_callback_races_unregister)		{ check_container_callback_races_unregister<checking_container_map>(); }

// 5) map::unregister_object_all() unregistered the object registered by the same key after its copy
TEST(group_container_safety, map_unregister_all_keeps_new_object_of_key)
{
	auto pcontainer = make_object<checking_container_map>();
	auto pobject_a = make_object<tracked_object>();
	auto pobject_b = make_object<tracked_object>();
	auto pobject_c = make_object<tracked_object>();

	EXPECT_EQ(pcontainer->register_object(0, pobject_a.get()), eRESULT::SUCCESS);
	EXPECT_EQ(pcontainer->register_object(1, pobject_b.get()), eRESULT::SUCCESS);

	// - when a is unregistered : key 1 is unregistered (b) and registered again (c)
	pcontainer->m_on_unregistered = [&](tracked_object* _pobject)
	{
		if (_pobject != pobject_a.get())
			return;
		EXPECT_TRUE(pcontainer->unregister_object(int64_t(1)));
		EXPECT_EQ(pcontainer->register_object(1, pobject_c.get()), eRESULT::SUCCESS);
	};

	pcontainer->unregister_object_all();

	EXPECT_EQ(pcontainer->count(), 1u);
	EXPECT_TRUE(pcontainer->find_object([&](tracked_object* _pobject) { return _pobject == pobject_c.get(); }) == pobject_c);

	pcontainer->m_on_unregistered = nullptr;
	pcontainer->unregister_object_all();
	EXPECT_EQ(pcontainer->count(), 0u);
	EXPECT_EQ(pcontainer->m_count_dead.load(), 0);
}

// 6) container map / set of a derived object type (_TOBJECT != _TOBJECTBASE) : unregister_object of Iserver_container<_TOBJECTBASE>
//    (unregister_object(_TOBJECT*) 'override' did not compile)
class tracked_object_derived : public tracked_object
{
};

TEST(group_container_safety, base_type_unregister)
{
	{
		auto pcontainer = make_object<container::set<tracked_object_derived, int, tracked_object>>();
		auto pobject = make_object<tracked_object_derived>();
		EXPECT_EQ(pcontainer->register_object(pobject.get()), eRESULT::SUCCESS);
		Iserver_container<tracked_object>* pinterface = pcontainer.get();
		EXPECT_TRUE(pinterface->unregister_object(pobject.get()));
		EXPECT_FALSE(pinterface->unregister_object(pobject.get()));
		EXPECT_EQ(pcontainer->count(), 0u);
	}
	{
		auto pcontainer = make_object<container::map<tracked_object_derived, int64_t, int, tracked_object>>();
		auto pobject = make_object<tracked_object_derived>();
		EXPECT_EQ(pcontainer->register_object(3, pobject.get()), eRESULT::SUCCESS);
		Iserver_container<tracked_object>* pinterface = pcontainer.get();
		EXPECT_TRUE(pinterface->unregister_object(pobject.get()));
		EXPECT_EQ(pcontainer->count(), 0u);
	}
}

// 7) unsubscribe(raw) of the observer whose last reference is in the observable
//    (on_observer_unregistered got the raw pointer after erase released the observer, and the lock of the observer was unlocked after it)
template <class TOBSERVABLE>
void check_observable_unsubscribe_last_reference()
{
	auto pobservable = make_object<TOBSERVABLE>();
	const auto count_alive = tracked_observer::s_count_alive.load();

	tracked_observer* pobserver_raw = nullptr;
	{
		auto pobserver = make_object<tracked_observer>();
		pobserver_raw = pobserver.get();
		EXPECT_EQ(subscribe_tracked(pobservable.get(), 1, pobserver_raw), eRESULT::SUCCESS);
	}
	EXPECT_EQ(tracked_observer::s_count_alive.load(), count_alive + 1);

	EXPECT_TRUE(pobservable->unsubscribe(pobserver_raw, 0));
	EXPECT_EQ(pobservable->m_count_unregistered.load(), 1);
	EXPECT_EQ(pobservable->m_count_dead.load(), 0);
	EXPECT_EQ(tracked_observer::s_count_alive.load(), count_alive);
	EXPECT_EQ(pobservable->count(), 0u);
}

TEST(group_container_safety, observable_list_unsubscribe_last_reference)	{ check_observable_unsubscribe_last_reference<checking_observable_list>(); }
TEST(group_container_safety, observable_set_unsubscribe_last_reference)		{ check_observable_unsubscribe_last_reference<checking_observable_set>(); }
TEST(group_container_safety, observable_map_unsubscribe_last_reference)		{ check_observable_unsubscribe_last_reference<checking_observable_map>(); }

// 8) observable for_each / find_observer run out of the lock : the callback may unsubscribe and lock other observers
template <class TOBSERVABLE>
void check_observable_callback_unsubscribes()
{
	const size_t COUNT = 8;
	auto pobservable = make_object<TOBSERVABLE>();
	const auto count_alive = tracked_observer::s_count_alive.load();

	for (size_t i = 0; i < COUNT; ++i)
	{
		auto pobserver = make_object<tracked_observer>();
		EXPECT_EQ(subscribe_tracked(pobservable.get(), static_cast<int64_t>(i), pobserver.get()), eRESULT::SUCCESS);
	}

	size_t count = 0;
	EXPECT_FALSE(pobservable->for_each([&](tracked_observer*) { return ++count < 2; }));
	EXPECT_EQ(count, 2u);
	EXPECT_TRUE(pobservable->find_observer([](tracked_observer*) { return false; }).empty());

	// - the observable has the last references
	count = 0;
	EXPECT_TRUE(pobservable->for_each([&](tracked_observer* _pobserver)
	{
		pobservable->unsubscribe(_pobserver, 0);
		if (_pobserver->m_magic == TRACKED_MAGIC)
			++count;
		return true;
	}));
	EXPECT_EQ(count, COUNT);
	EXPECT_EQ(pobservable->count(), 0u);
	EXPECT_EQ(pobservable->m_count_dead.load(), 0);
	EXPECT_EQ(tracked_observer::s_count_alive.load(), count_alive);
}

TEST(group_container_safety, observable_list_callback_unsubscribes)	{ check_observable_callback_unsubscribes<checking_observable_list>(); }
TEST(group_container_safety, observable_set_callback_unsubscribes)	{ check_observable_callback_unsubscribes<checking_observable_set>(); }
TEST(group_container_safety, observable_map_callback_unsubscribes)	{ check_observable_callback_unsubscribes<checking_observable_map>(); }

// 9) unsubscribe / update_observer_data of an observer not subscribed, called in its destructor
//    (observable::set / list made an object_ptr of the observer in its final release : 0 -> 1 -> 0, double release)
class destroying_observer :
	public						Iobserver<destroying_observer>,
	public						net::io::Isender
{
public:
	virtual						~destroying_observer() noexcept { if (this->m_on_destroy) this->m_on_destroy(this); ++s_count_destroyed; }

	virtual	bool				process_send(shared_buffer&& /*_buffer*/, std::size_t /*_count_message*/, uint64_t /*_option*/) override { return true; }

public:
			std::function<void (destroying_observer*)> m_on_destroy;
	static inline std::atomic<int64_t> s_count_destroyed {0};
};

template <class TOBSERVABLE, bool IS_MAP>
void check_observable_call_in_destructor()
{
	auto pobservable = make_object<TOBSERVABLE>();
	const auto count_destroyed = destroying_observer::s_count_destroyed.load();

	auto subscribe = [&](destroying_observer* _pobserver, int64_t _key)
	{
		if constexpr (IS_MAP)
			return pobservable->subscribe(_key, _pobserver, 0);
		else
			return pobservable->subscribe(_pobserver);
	};

	// - an observer subscribed (the observable is not empty)
	auto pobserver_other = make_object<destroying_observer>();
	EXPECT_EQ(subscribe(pobserver_other.get(), 1), eRESULT::SUCCESS);

	size_t count_call = 0;
	{
		auto pobserver = make_object<destroying_observer>();
		pobserver->m_on_destroy = [&](destroying_observer* _pobserver)
		{
			typename TOBSERVABLE::observer_data_t data {};
			EXPECT_FALSE(pobservable->update_observer_data(_pobserver, data, 0));
			EXPECT_FALSE(pobservable->unsubscribe(_pobserver, 0));
			++count_call;
		};
	}
	EXPECT_EQ(count_call, 1u);
	EXPECT_EQ(destroying_observer::s_count_destroyed.load(), count_destroyed + 1);
	EXPECT_EQ(pobservable->count(), 1u);

	pobservable->unsubscribe_all();
	EXPECT_EQ(pobservable->count(), 0u);
}

TEST(group_container_safety, observable_list_call_in_destructor)	{ check_observable_call_in_destructor<observable::list<destroying_observer>, false>(); }
TEST(group_container_safety, observable_set_call_in_destructor)		{ check_observable_call_in_destructor<observable::set<destroying_observer>, false>(); }
TEST(group_container_safety, observable_map_call_in_destructor)		{ check_observable_call_in_destructor<observable::map<destroying_observer, int64_t>, true>(); }

}
}

// ============================================================================
// explicit instantiation
//   - compiles all the members of the classes (the members not used are not compiled by implicit instantiation)
// ============================================================================
template class CGDK::observable::list<CGDK::test_group_leave_all::observer>;
template class CGDK::observable::list_sender<CGDK::test_group_leave_all::observer>;
template class CGDK::observable::map<CGDK::test_group_leave_all::observer, int64_t>;
template class CGDK::observable::set<CGDK::test_group_leave_all::observer>;
template class CGDK::observable::map_sender<CGDK::test_group_leave_all::observer, int64_t>;
template class CGDK::observable::set_sender<CGDK::test_group_leave_all::observer>;
template class CGDK::Iserver_container<CGDK::test_group_leave_all::sender>;
template struct CGDK::container::OBJECT_SET<CGDK::test_group_leave_all::sender>;
template struct CGDK::container::OBJECT_SET_DATA<CGDK::test_group_leave_all::sender, int>;
template class CGDK::container::list<CGDK::test_group_leave_all::sender>;
template class CGDK::container::set<CGDK::test_group_leave_all::sender>;
template class CGDK::container::map<CGDK::test_group_leave_all::sender, int64_t>;
template class CGDK::container::list_sender<CGDK::test_group_leave_all::sender>;
template class CGDK::container::set_sender<CGDK::test_group_leave_all::sender>;
template class CGDK::container::map_sender<CGDK::test_group_leave_all::sender, int64_t>;
template class CGDK::container::set<CGDK::test_group_leave_all::tracked_object_derived, int, CGDK::test_group_leave_all::tracked_object>;
template class CGDK::container::map<CGDK::test_group_leave_all::tracked_object_derived, int64_t, int, CGDK::test_group_leave_all::tracked_object>;
