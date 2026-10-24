#include "cgdk/sdk10/server.service.doss.h"

#include <iostream>
#if defined(_WIN32)
	#include <conio.h>
#endif

using namespace CGDK;

class simple_repository : public CGDK::doss::repository::rdb<>
{
public:
	// - the transaction handlers are coroutines : 'co_await' the db queries / other servers inside them,
	//   set the result with _ptransactable->set_result() and 'co_return' the result code.
	virtual	CGDK::coroutine::task<CGDK::result_code>	on_request_checkin(CGDK::object_ptr<CGDK::doss::Itransactable> _ptransactable) override
	{
		//std::cout << "[repository] on_checkin"sv << '\n';
		_ptransactable->set_result(eRESULT::SUCCESS);
		co_return eRESULT::SUCCESS;
	}
	virtual	CGDK::coroutine::task<CGDK::result_code>	on_request_checkout(CGDK::object_ptr<CGDK::doss::Itransactable> _ptransactable) override
	{
		//std::cout << "[repository] on_checkout"sv << '\n';
		_ptransactable->set_result(eRESULT::SUCCESS);
		co_return eRESULT::SUCCESS;
	}
	virtual	CGDK::coroutine::task<CGDK::result_code>	on_request_command(CGDK::object_ptr<CGDK::doss::Itransactable> _ptransactable) override
	{
		//std::cout << "[repository] on_transaction"sv << '\n';
		_ptransactable->set_result(eRESULT::SUCCESS);
		co_return eRESULT::SUCCESS;
	}
	virtual	CGDK::coroutine::task<CGDK::result_code>	on_request_update(CGDK::object_ptr<CGDK::doss::Itransactable> _ptransactable) override
	{
		//std::cout << "[repository] on_update"sv << '\n';
		_ptransactable->set_result(eRESULT::SUCCESS);
		co_return eRESULT::SUCCESS;
	}
};

void print_status(doss::Nrepository<>* _prepository)
{
	std::cout << "m_statistics_checkin -> "sv << (uint64_t)_prepository->statistics().m_statistics_checkin << "\n";
	std::cout << "m_statistics_checkin_request -> "sv << (uint64_t)_prepository->statistics().m_statistics_checkin_request << "\n";
	std::cout << "m_statistics_checkin_success -> "sv << (uint64_t)_prepository->statistics().m_statistics_checkin_success << "\n";
	std::cout << "m_statistics_checkin_fail -> "sv << (uint64_t)_prepository->statistics().m_statistics_checkin_fail << "\n";
	std::cout << "m_statistics_checkout_request -> "sv << (uint64_t)_prepository->statistics().m_statistics_checkout_request << "\n";
	std::cout << "m_statistics_checkout_success -> "sv << (uint64_t)_prepository->statistics().m_statistics_checkout_success << "\n";
	std::cout << "m_statistics_checkout_fail -> "sv << (uint64_t)_prepository->statistics().m_statistics_checkout_fail << "\n";
	std::cout << "m_statistics_checkout_forcible -> "sv << (uint64_t)_prepository->statistics().m_statistics_checkout_forcible << "\n";
	std::cout << "m_statistics_transaction_request -> "sv << (uint64_t)_prepository->statistics().m_statistics_command_request << "\n";
	std::cout << "m_statistics_transaction_success -> "sv << (uint64_t)_prepository->statistics().m_statistics_command_success << "\n";
	std::cout << "m_statistics_transaction_fail -> "sv << (uint64_t)_prepository->statistics().m_statistics_command_fail << "\n";

	auto count_i = 0;
	for (auto& iter : _prepository->m_map_entity_by_eid)
	{
		if (iter.second->get_queued_transaction_count() != 0)
		{
			++count_i;
		}
	}
	std::cout << "entity - command not completed ->"sv << count_i << '\n';

	auto count_j = 0;
	for (auto& iter : _prepository->m_map_entity_by_eid)
	{
		if (iter.second->get_checkin_count() != 0)
		{
			++count_j;
		}
	}
	std::cout << "entity - not checkouted -> "sv << count_j << '\n';
}

int main()
{
	// 1) create repository
	auto prepository = make_own<simple_repository>();

	// 2) set name
	prepository->name("test_repository"sv);

	// 3) start repository
	prepository->start();

	// 4) create depot
	auto pdepot_remote = make_own<doss::depot>();

	// 5) set name
	pdepot_remote->name("test_remote_repository"sv);

	// 6) ...
	pdepot_remote->set_repository(prepository);

	// 7) start
	pdepot_remote->start();

	// 8) wait util press 'esc'
	while (_getch() != 0x1b)
	{
		print_status(prepository);
	}

	// return)
	return 0;
}
