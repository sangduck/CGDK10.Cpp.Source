#include "pch.h"

//-----------------------------------------------------------------------------
//
// doss rdb repository : checkin -> command -> checkout
//
//  - drives the doss runtime path headlessly (the tutorials were interactive only).
//  - a repository subclass counts every transaction handler call (the handlers are
//    coroutines : set_result() + co_return), so the test proves the request actually
//    reached the repository, not merely that the session API returned.
//  - the session API is used through its coroutine form ('.get()' waits for the whole
//    transaction coroutine to complete on the repository's executor thread).
//
//-----------------------------------------------------------------------------
using namespace CGDK;
using namespace std::literals;

namespace
{

// - a repository that records every handler call (each handler is a coroutine)
class test_repository : public doss::repository::rdb<>
{
public:
	std::atomic<int>	count_checkin{ 0 };
	std::atomic<int>	count_command{ 0 };
	std::atomic<int>	count_checkout{ 0 };
	std::atomic<int>	count_update{ 0 };

protected:
	virtual	coroutine::task<result_code> on_request_checkin(object_ptr<doss::Itransactable> _ptransactable) override
	{
		this->count_checkin.fetch_add(1, std::memory_order_relaxed);
		_ptransactable->set_result(eRESULT::SUCCESS);
		co_return eRESULT::SUCCESS;
	}
	virtual	coroutine::task<result_code> on_request_command(object_ptr<doss::Itransactable> _ptransactable) override
	{
		this->count_command.fetch_add(1, std::memory_order_relaxed);
		_ptransactable->set_result(eRESULT::SUCCESS);
		co_return eRESULT::SUCCESS;
	}
	virtual	coroutine::task<result_code> on_request_checkout(object_ptr<doss::Itransactable> _ptransactable) override
	{
		this->count_checkout.fetch_add(1, std::memory_order_relaxed);
		_ptransactable->set_result(eRESULT::SUCCESS);
		co_return eRESULT::SUCCESS;
	}
	virtual	coroutine::task<result_code> on_request_update(object_ptr<doss::Itransactable> _ptransactable) override
	{
		this->count_update.fetch_add(1, std::memory_order_relaxed);
		_ptransactable->set_result(eRESULT::SUCCESS);
		co_return eRESULT::SUCCESS;
	}
};

} // namespace


//-----------------------------------------------------------------------------
// checkin -> command -> checkout (the full happy path)
//-----------------------------------------------------------------------------
TEST(CGDK10_doss_rdb, checkin_command_checkout)
{
	// 1) create and start the repository (its executor thread processes the transactions)
	auto prepository = make_own<test_repository>();
	prepository->name("test_repository"sv);
	prepository->start();

	// 2) create a session (its session id is auto-assigned)
	auto psession = make_own<doss::session<>>();

	// - the entity id must carry the repository's own (generated) repository id, not a literal
	//   (the checkin cid is stamped with get_repository_id(); a mismatch makes commands NOT_READY)
	const auto id_entity = make_entity_id(prepository->get_repository_id(), sOBJECT_ID{ 10llu });

	// 3) checkin (wait for the whole transaction coroutine to complete)
	const auto result_checkin = psession->co_checkin(id_entity, prepository.get()).get();
	EXPECT_EQ(result_checkin, eRESULT::SUCCESS);
	EXPECT_EQ(prepository->count_checkin.load(), 1);
	EXPECT_TRUE(psession->get_entity().exist());

	// 4) command
	const auto result_command = psession->co_request(doss::make_command(eTRANSACTION_TYPE::COMMAND)).get();
	EXPECT_EQ(result_command, eRESULT::SUCCESS);
	EXPECT_EQ(prepository->count_command.load(), 1);

	// 5) checkout
	const auto result_checkout = psession->co_checkout().get();
	EXPECT_EQ(result_checkout, eRESULT::SUCCESS);
	EXPECT_EQ(prepository->count_checkout.load(), 1);
	EXPECT_FALSE(psession->get_entity().exist());

	// 6) repository statistics (the success counters)
	const auto& stat = static_cast<doss::Nrepository<>*>(prepository.get())->statistics();
	EXPECT_EQ(static_cast<uint64_t>(stat.m_statistics_checkin_success), 1u);
	EXPECT_EQ(static_cast<uint64_t>(stat.m_statistics_command_success), 1u);
	EXPECT_EQ(static_cast<uint64_t>(stat.m_statistics_checkout_success), 1u);

	// 7) teardown
	// teardown : checkout_all() + clear_entities() run in destroy() (breaks entity/session references)
	prepository->stop();
	prepository->destroy();
}


//-----------------------------------------------------------------------------
// command / checkout without a prior checkin must not reach the repository
//-----------------------------------------------------------------------------
TEST(CGDK10_doss_rdb, command_without_checkin_is_not_ready)
{
	auto prepository = make_own<test_repository>();
	prepository->name("test_repository_2"sv);
	prepository->start();

	auto psession = make_own<doss::session<>>();

	// - command without checkin : the session has no entity
	const auto result_command = psession->co_request(doss::make_command(eTRANSACTION_TYPE::COMMAND)).get();
	EXPECT_EQ(result_command, eRESULT::NOT_READY);
	EXPECT_EQ(prepository->count_command.load(), 0);

	// - checkout without checkin : likewise
	const auto result_checkout = psession->co_checkout().get();
	EXPECT_EQ(result_checkout, eRESULT::NOT_READY);
	EXPECT_EQ(prepository->count_checkout.load(), 0);

	// teardown : checkout_all() + clear_entities() run in destroy() (breaks entity/session references)
	prepository->stop();
	prepository->destroy();
}


//-----------------------------------------------------------------------------
// repeated checkin -> command(s) -> checkout on one session
//-----------------------------------------------------------------------------
TEST(CGDK10_doss_rdb, repeated_commands)
{
	auto prepository = make_own<test_repository>();
	prepository->name("test_repository_3"sv);
	prepository->start();

	auto psession = make_own<doss::session<>>();

	const auto id_entity = make_entity_id(prepository->get_repository_id(), sOBJECT_ID{ 20llu });

	ASSERT_EQ(psession->co_checkin(id_entity, prepository.get()).get(), eRESULT::SUCCESS);
	ASSERT_EQ(prepository->count_checkin.load(), 1) << "checkin handler not reached";
	ASSERT_TRUE(psession->get_entity().exist()) << "entity empty right after a successful checkin";
	ASSERT_TRUE(psession->get_checkin_id().exist()) << "checkin id (m_cid) empty right after a successful checkin";
	ASSERT_EQ(psession->get_checkin_id().id_entity, id_entity) << "checkin id entity mismatch";

	constexpr int COUNT_COMMAND = 8;
	for (int i = 0; i < COUNT_COMMAND; ++i)
		EXPECT_EQ(psession->co_request(doss::make_command(eTRANSACTION_TYPE::COMMAND)).get(), eRESULT::SUCCESS) << "command #" << i;

	EXPECT_EQ(prepository->count_command.load(), COUNT_COMMAND);

	ASSERT_EQ(psession->co_checkout().get(), eRESULT::SUCCESS);
	EXPECT_FALSE(psession->get_entity().exist());

	const auto& stat = static_cast<doss::Nrepository<>*>(prepository.get())->statistics();
	EXPECT_EQ(static_cast<uint64_t>(stat.m_statistics_command_success), static_cast<uint64_t>(COUNT_COMMAND));

	// teardown : checkout_all() + clear_entities() run in destroy() (breaks entity/session references)
	prepository->stop();
	prepository->destroy();
}
