#include "pch.h"
#include "cgdk/sdk10/server.group.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

// ----------------------------------------------------------------------------
// group lock stage 2 with sockets (net::socket::tcp<> : net::io::sendable::Ntcp_async_gather)
//
// - the server sockets are the members of a group. the peers (clients) receive what they send.
// - "deferred" is checked by the peer : nothing arrives while the group lock is held (the system call was not made)
// ----------------------------------------------------------------------------
namespace CGDK
{
namespace test_group_send_defer_socket
{
using namespace std::chrono_literals;

// - message : [u32 size][u32 seq]
static shared_buffer make_message(uint32_t _seq)
{
	auto buf = alloc_shared_buffer(8);
	buf.append<uint32_t>(8);
	buf.append<uint32_t>(_seq);
	return buf;
}

// 1) server socket (a member of the group)
class member_socket :
	public						net::socket::tcp<>,
	public						Igroupable<member_socket>
{
protected:
	virtual	void				on_connect() override;
	virtual	void				on_disconnect(uint64_t /*_reason*/) override { this->leave_group(); }
	virtual	result_code			on_message(sMESSAGE_NETWORK& /*_msg*/) override { return eRESULT::DONE; }
};

class socket_group : public group::list_sender<member_socket>
{
public:
	using group::list_sender<member_socket>::list_sender;

			std::function<void (member_socket*)> m_on_entered;

protected:
	virtual	void				on_member_entered(member_socket* _pmember, sMESSAGE& /*_param*/) override { if (this->m_on_entered) this->m_on_entered(_pmember); }
};

static object_ptr<socket_group> g_pgroup;

void member_socket::on_connect() { auto pgroup = g_pgroup; if (pgroup.exist()) pgroup->enter(this); }

// 2) peer (client)
class peer_socket :
	public						net::socket::tcp<>,
	public						net::io::connect_requestable::tcp::Ndefault
{
public:
			std::atomic<bool>	m_is_connected{ false };
			std::atomic<bool>	m_is_disconnected{ false };
	mutable	std::mutex			m_lockable;
			std::vector<uint32_t> m_received;

			std::vector<uint32_t> received() const { std::lock_guard lock(this->m_lockable); return this->m_received; }
			std::size_t			count_received() const { std::lock_guard lock(this->m_lockable); return this->m_received.size(); }

protected:
	virtual	void				on_connect() override { this->m_is_connected = true; }
	virtual	void				on_fail_connect(uint64_t /*_reason*/) override {}
	virtual	void				on_disconnect(uint64_t /*_reason*/) override { this->m_is_disconnected = true; }
	virtual	result_code			on_message(sMESSAGE_NETWORK& _msg) override
	{
		const auto seq = *reinterpret_cast<const uint32_t*>(_msg.buf_message.data() + 4);
		std::lock_guard lock(this->m_lockable);
		this->m_received.push_back(seq);
		return eRESULT::DONE;
	}
};

static uint16_t test_port() { return static_cast<uint16_t>(32400 + api::get_process_id() % 500); }

template <class TPRED>
static bool wait_until(TPRED _pred, std::chrono::milliseconds _timeout = 10s)
{
	const auto time_end = std::chrono::steady_clock::now() + _timeout;
	while (_pred() == false)
	{
		if (std::chrono::steady_clock::now() >= time_end)
			return false;
		std::this_thread::sleep_for(2ms);
	}
	return true;
}

static std::vector<uint32_t> seq_range(uint32_t _from, uint32_t _to)
{
	std::vector<uint32_t> v;
	for (auto i = _from; i <= _to; ++i)
		v.push_back(i);
	return v;
}

// - the server (acceptor + group) and _count peers connected
class group_send_defer_socket : public ::testing::Test
{
protected:
	static constexpr std::size_t COUNT_PEER = 4;

	void SetUp() override
	{
		g_pgroup = make_object<socket_group>();
		g_pgroup->enable_member_enter(true);

		this->m_pacceptor = make_own<net::acceptor<member_socket>>();
		net::acceptor<member_socket>::START_PARAMETER parameter;
		parameter.local_endpoint = net::ip::tcp::endpoint(net::ip::address_v4::loopback(), test_port());
		parameter.reallocate_port_at_duplicate_port_error = true;
		ASSERT_EQ(this->m_pacceptor->start(parameter), eRESULT::SUCCESS);

		const auto port = this->m_pacceptor->local_endpoint().port();
		ASSERT_NE(port, 0);

		for (std::size_t i = 0; i < COUNT_PEER; ++i)
		{
			auto ppeer = make_own<peer_socket>();
			ppeer->start({ { net::ip::address_v4::loopback(), port } });
			this->m_vector_peer.push_back(std::move(ppeer));
		}

		ASSERT_TRUE(wait_until([&] { for (auto& iter : this->m_vector_peer) if (iter->m_is_connected == false) return false; return g_pgroup->member_count() == COUNT_PEER; }));

		// - members (the order of the peers is not the order of the members : matched by the local port)
		SCOPED_LOCK(g_pgroup->get_group_lockable())
		{
			for (auto& iter_peer : this->m_vector_peer)
			{
				for (auto& iter : *g_pgroup)
				{
					if (iter.member()->remote_endpoint().port() == iter_peer->local_endpoint().port())
						this->m_vector_member.push_back(iter.member());
				}
			}
		}
		ASSERT_EQ(this->m_vector_member.size(), COUNT_PEER);
	}

	void TearDown() override
	{
		send_defer::enable(true);

		for (auto& iter : this->m_vector_peer)
			iter->stop();
		this->m_vector_peer.clear();
		this->m_vector_member.clear();

		if (this->m_pacceptor.exist())
			this->m_pacceptor->stop();

		if (g_pgroup.exist())
			g_pgroup->leave_all();
		g_pgroup.reset();
	}

	bool						wait_received(std::size_t _count, std::chrono::milliseconds _timeout = 10s)
	{
		return wait_until([&] { for (auto& iter : this->m_vector_peer) if (iter->count_received() < _count) return false; return true; }, _timeout);
	}
	std::size_t					count_received_all() const
	{
		std::size_t count = 0;
		for (auto& iter : this->m_vector_peer)
			count += iter->count_received();
		return count;
	}

	own_ptr<net::acceptor<member_socket>> m_pacceptor;
	std::vector<own_ptr<peer_socket>> m_vector_peer;
	std::vector<object_ptr<member_socket>> m_vector_member;	// - [i] is connected to m_vector_peer[i]
};

// 0) control : not deferred (enable(false)) => arrives while the lock is held
TEST_F(group_send_defer_socket, control_not_deferred)
{
	send_defer::enable(false);

	SCOPED_LOCK(g_pgroup->get_group_lockable())
	{
		for (auto& iter : this->m_vector_member)
			iter->send(make_message(1));

		EXPECT_TRUE(this->wait_received(1, 5s));
	}
}

// 1) the sends in the group lock arrive after the lock is released (in order)
TEST_F(group_send_defer_socket, sent_after_group_lock_released)
{
	SCOPED_LOCK(g_pgroup->get_group_lockable())
	{
		for (uint32_t seq = 1; seq <= 3; ++seq)
			for (auto& iter : this->m_vector_member)
				iter->send(make_message(seq));

		// - group broadcast in the lock
		g_pgroup->send(make_message(4));

		EXPECT_EQ(send_defer::count(), COUNT_PEER);

		// - nothing arrives while it is held
		std::this_thread::sleep_for(100ms);
		EXPECT_EQ(this->count_received_all(), 0u);
	}

	EXPECT_EQ(send_defer::count(), 0u);
	ASSERT_TRUE(this->wait_received(4));
	for (auto& iter : this->m_vector_peer)
		EXPECT_EQ(iter->received(), seq_range(1, 4));
}

// 2) broadcast in on_member_entered : arrives after the enter
TEST_F(group_send_defer_socket, broadcast_in_member_entered)
{
	std::atomic<std::size_t> count_received_in_event{ 0 };
	std::atomic<std::size_t> count_event{ 0 };

	g_pgroup->m_on_entered = [&](member_socket*)
	{
		const auto count_before = this->count_received_all();
		g_pgroup->send(make_message(100));
		std::this_thread::sleep_for(50ms);
		count_received_in_event += this->count_received_all() - count_before;
		++count_event;
	};

	// - a new peer enters
	auto ppeer = make_own<peer_socket>();
	ppeer->start({ { net::ip::address_v4::loopback(), this->m_pacceptor->local_endpoint().port() } });

	ASSERT_TRUE(wait_until([&] { return count_event == 1; }));
	EXPECT_EQ(count_received_in_event, 0u);

	ASSERT_TRUE(this->wait_received(1));
	ASSERT_TRUE(wait_until([&] { return ppeer->count_received() == 1; }));
	for (auto& iter : this->m_vector_peer)
		EXPECT_EQ(iter->received(), std::vector<uint32_t>{ 100 });
	EXPECT_EQ(ppeer->received(), std::vector<uint32_t>{ 100 });

	g_pgroup->m_on_entered = nullptr;
	ppeer->stop();
}

// 3) send and closesocket in the group lock : the messages arrive before the close
TEST_F(group_send_defer_socket, send_then_closesocket_in_lock)
{
	SCOPED_LOCK(g_pgroup->get_group_lockable())
	{
		this->m_vector_member[0]->send(make_message(1));
		this->m_vector_member[0]->send(make_message(2));
		this->m_vector_member[0]->closesocket();

		this->m_vector_member[1]->send(make_message(1));
	}

	ASSERT_TRUE(wait_until([&] { return this->m_vector_peer[0]->m_is_disconnected.load(); }));
	EXPECT_EQ(this->m_vector_peer[0]->received(), seq_range(1, 2));

	ASSERT_TRUE(wait_until([&] { return this->m_vector_peer[1]->count_received() == 1; }));
}

// 4) send and disconnect (graceful) in the group lock
TEST_F(group_send_defer_socket, send_then_disconnect_in_lock)
{
	SCOPED_LOCK(g_pgroup->get_group_lockable())
	{
		for (uint32_t seq = 1; seq <= 5; ++seq)
			this->m_vector_member[0]->send(make_message(seq));
		this->m_vector_member[0]->disconnect();
	}

	ASSERT_TRUE(wait_until([&] { return this->m_vector_peer[0]->m_is_disconnected.load(); }));
	EXPECT_EQ(this->m_vector_peer[0]->received(), seq_range(1, 5));
}

// 5) another thread closes the socket while this thread defers : sent before the close
TEST_F(group_send_defer_socket, closesocket_by_another_thread_sends_deferred)
{
	auto pmember = this->m_vector_member[0];

	std::mutex lockable;
	std::condition_variable cv;
	int step = 0;

	std::thread thread_defer([&]
	{
		send_defer::scope scope;

		pmember->send(make_message(1));
		pmember->send(make_message(2));

		{ std::lock_guard lock(lockable); step = 1; }
		cv.notify_all();

		std::unique_lock lock(lockable);
		cv.wait(lock, [&] { return step == 2; });
	});

	{
		std::unique_lock lock(lockable);
		cv.wait(lock, [&] { return step == 1; });
	}

	std::this_thread::sleep_for(50ms);
	EXPECT_EQ(this->m_vector_peer[0]->count_received(), 0u);

	pmember->closesocket();

	{ std::lock_guard lock(lockable); step = 2; }
	cv.notify_all();
	thread_defer.join();

	ASSERT_TRUE(wait_until([&] { return this->m_vector_peer[0]->m_is_disconnected.load(); }));
	EXPECT_EQ(this->m_vector_peer[0]->received(), seq_range(1, 2));
}

// 6) an exception leaves the group lock : sent
TEST_F(group_send_defer_socket, exception_in_lock)
{
	try
	{
		SCOPED_LOCK(g_pgroup->get_group_lockable())
		{
			this->m_vector_member[0]->send(make_message(1));
			throw std::runtime_error("test");
		}
	}
	catch (const std::runtime_error&)
	{
	}

	EXPECT_EQ(send_defer::count(), 0u);
	ASSERT_TRUE(wait_until([&] { return this->m_vector_peer[0]->count_received() == 1; }));
}

// 7) the sends of another thread while deferred are sent after them (in order)
TEST_F(group_send_defer_socket, order_with_other_thread)
{
	auto pmember = this->m_vector_member[0];

	SCOPED_LOCK(g_pgroup->get_group_lockable())
	{
		pmember->send(make_message(1));
		pmember->send(make_message(2));

		std::thread([pmember] { pmember->send(make_message(3)); pmember->send(make_message(4)); }).join();

		pmember->send(make_message(5));

		std::this_thread::sleep_for(50ms);
		EXPECT_EQ(this->m_vector_peer[0]->count_received(), 0u);
	}

	ASSERT_TRUE(wait_until([&] { return this->m_vector_peer[0]->count_received() == 5; }));
	EXPECT_EQ(this->m_vector_peer[0]->received(), seq_range(1, 5));
}

// 8) many sends in the group lock : sent earlier (no overflow), in order
TEST_F(group_send_defer_socket, many_sends_in_lock)
{
	static constexpr uint32_t COUNT = 2000;

	SCOPED_LOCK(g_pgroup->get_group_lockable())
	{
		for (uint32_t seq = 1; seq <= COUNT; ++seq)
			ASSERT_TRUE(this->m_vector_member[0]->send(make_message(seq)));
	}

	ASSERT_TRUE(wait_until([&] { return this->m_vector_peer[0]->count_received() == COUNT; }));
	EXPECT_EQ(this->m_vector_peer[0]->received(), seq_range(1, COUNT));
}

// 9) a thread block (sync query) in the group lock : sent at once
TEST_F(group_send_defer_socket, thread_block_sends_at_once)
{
	SCOPED_LOCK(g_pgroup->get_group_lockable())
	{
		this->m_vector_member[0]->send(make_message(1));

		{
			scoped_thread_block thread_block;

			// - the deferred are sent and the sends in it are not deferred
			EXPECT_TRUE(wait_until([&] { return this->m_vector_peer[0]->count_received() == 1; }, 5s));

			this->m_vector_member[0]->send(make_message(2));
			EXPECT_TRUE(wait_until([&] { return this->m_vector_peer[0]->count_received() == 2; }, 5s));
		}

		// - deferred again after the block (an idle socket : member[0] may be still sending 2 - then 3 is sent after it by the sending thread)
		this->m_vector_member[0]->send(make_message(3));
		this->m_vector_member[1]->send(make_message(1));
		std::this_thread::sleep_for(50ms);
		EXPECT_EQ(this->m_vector_peer[1]->count_received(), 0u);
	}

	ASSERT_TRUE(wait_until([&] { return this->m_vector_peer[0]->count_received() == 3; }));
	EXPECT_EQ(this->m_vector_peer[0]->received(), seq_range(1, 3));
	ASSERT_TRUE(wait_until([&] { return this->m_vector_peer[1]->count_received() == 1; }));
}

// 10) concurrent : threads broadcast in the group lock and send without it
TEST_F(group_send_defer_socket, concurrent_broadcast)
{
	static constexpr uint32_t COUNT_THREAD = 4;
	static constexpr uint32_t COUNT_LOOP = 300;

	std::vector<std::thread> vector_thread;
	for (uint32_t t = 0; t < COUNT_THREAD; ++t)
	{
		vector_thread.emplace_back([&, t]
		{
			for (uint32_t i = 0; i < COUNT_LOOP; ++i)
			{
				SCOPED_LOCK(g_pgroup->get_group_lockable())
				{
					g_pgroup->send(make_message(t * 100000 + i + 1));
				}
				this->m_vector_member[i % COUNT_PEER]->send(make_message(t * 100000 + 50000 + i));
			}
		});
	}
	for (auto& iter : vector_thread)
		iter.join();

	const std::size_t count_expected = COUNT_THREAD * COUNT_LOOP + COUNT_THREAD * COUNT_LOOP / COUNT_PEER;
	ASSERT_TRUE(this->wait_received(count_expected));

	for (auto& iter : this->m_vector_peer)
	{
		const auto received = iter->received();
		EXPECT_EQ(received.size(), count_expected);

		// - the sends of a thread arrive in order
		std::vector<int64_t> last(COUNT_THREAD * 2, -1);
		for (auto seq : received)
		{
			const auto key = (seq / 100000) * 2 + ((seq % 100000) >= 50000 ? 1 : 0);
			EXPECT_LT(last[key], static_cast<int64_t>(seq));
			last[key] = seq;
		}
	}
}

}
}
