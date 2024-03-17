#include "pch.h"

using namespace CGDK;


class schedulable::manager::executable_schedulable : 
	virtual public CGDK::Iexecutable,
	public Npoolable<schedulable::manager::executable_schedulable>,
	public factory::_traits_system
{
public:
		object_ptr<Ischedulable> pschedulable;
		object_ptr<schedulable::manager> pscheduler;

protected:
	virtual	intptr_t			process_execute(intptr_t _return, size_t _param) override;
};

intptr_t schedulable::manager::executable_schedulable::process_execute(intptr_t /*_return*/, size_t /*_param*/)
{
	// check)
	CGASSERT_ERROR(this->pscheduler.exist());

	// declare)
	object_ptr<Ischedulable> pschedulable_temp = std::move(this->pschedulable);

	// check)
	CGASSERT_ERROR(pschedulable_temp.exist());

	// statistics)
	pschedulable_temp->statistics_on_extra();
	++pschedulable_temp->m_statistics_schedulable.count_execute;

	// 1) execute
	this->pscheduler->process_execute_schedulable(std::move(pschedulable_temp));

	// return)
	return 0;
}

schedulable::manager::manager() noexcept :
	Nexecutor(),
	m_flag_run(0)
{
	// 1) set object type
	this->set_object_type(eOBJECT_TYPE::EXECUTOR_SCHEDULABLE);

	// 2) set executor type
	this->m_statistics.type = eEXECUTOR_TYPE::SYSTEM;

	// 3) set gc_type of factory
	//executable_schedulable::get_factory()->set_gc_type(eFACTORY_GC_TYPE::POOLING);

	// 4) register to factory manager
	this->process_register_executor();
}

schedulable::manager::manager(std::string_view _name) noexcept :
	Nexecutor(_name),
	m_flag_run(0)
{
	// 1) set object type
	this->set_object_type(eOBJECT_TYPE::EXECUTOR_SCHEDULABLE);

	// 2) set executor type
	this->m_statistics.type = eEXECUTOR_TYPE::SYSTEM;

	// 3) set gc_type of factory
	//executable_schedulable::get_factory()->set_gc_type(eFACTORY_GC_TYPE::POOLING);

	// 4) register to executor manager
	this->process_register_executor();
}

schedulable::manager::~manager() noexcept
{
	// 1) stop manager
	this->schedulable::manager::stop();

	// 2) unregister from executor manager
	this->process_unregister_executor();
}

result_code schedulable::manager::start(context& /*_context*/)
{
	// 1) set m_flag_run 'true' and get old state
	const int state_old = this->m_flag_run.exchange(1);

	// check) return if aleady running
	RETURN_IF(state_old != 0, eRESULT::ALEADY_DONE);

	// 2) reserve
	scoped_lock(this->m_lockable_priorityqueue)
	{
		// - clear & reserve
		scoped_lock(this->m_lockable_set_schedulable)
		{
			this->m_set_schedulable.clear();
		}
		this->m_priorityqueue_schedulable.reserve(16384);
	}

	try
	{
		// 4) begin thread
		this->begin_thread(fn_thread, this);
	}
	catch (...)
	{
		// - stop
		this->stop();

		// reraise)
		throw;
	}

	return result_code(eRESULT::SUCCESS);
}

result_code schedulable::manager::stop() noexcept
{
	// 1) set m_flag_run 'false' and get old state
	const int state_old = this->m_flag_run.exchange(0);

	// check) return if not running
	RETURN_IF(state_old == 0, eRESULT::ALEADY_DONE);

	// 2) cancel all executable
	this->process_cancel_schedulable_all();

	// 3) wait until thread is terminated
	this->wait();

	// return) 
	return result_code(eRESULT::SUCCESS);
}

void schedulable::manager::print_statistics() noexcept
{
	//try
	//{
	//	// 1) get instance
	//	auto instance = instance();

	//	// 2) alloc shared_buffer for log text
	//	auto buf_output = alloc_shared_buffer((static_cast<size_t>(instance->size()) + 4) * 256);

	//	buf_output.append_text("\x1b[37m" "---------------------------------------------------------------------------------------------------------------------------------\n" "\x1b[0m");
	//	buf_output.append_text("\x1b[37m" "{:30s} {:8s}{:>5s}{:>9s}{:>13s}{:>13s}\n" "\x1b[0m",
	//		"name",
	//		"type",
	//		"id",
	//		"thread",
	//		"executed",
	//		"pended"
	//	);
	//	buf_output.append_text("\x1b[90m" "---------------------------------------------------------------------------------------------------------------------------------\n" "\x1b[0m");

	//	scoped_shared_lock(instance->m_lockable_list_executor)
	//	{
	//		for (auto& iter : instance->m_list_executor)
	//		{
	//			// check)
	//			CGASSERT_ERROR(iter != nullptr);

	//			// check)
	//			CONTINUE_IF(iter == nullptr);

	//			// - get statistics
	//			auto& statistics = iter->statistics();

	//			std::string_view executor_name = (statistics.name.size() < 30) ? std::string_view(statistics.name) : std::string_view(statistics.name.c_str(), 30);

	//			auto alloc_execute = statistics.now.count_execute.load();

	//			if (statistics.type == eEXECUTOR_TYPE::USER)
	//			{
	//				// "1. name" "2. type"
	//				if (alloc_execute != 0)
	//				{
	//					buf_output.append_text("\x1b[92m" "{:30s} {:8s}" "\x1b[0m", executor_name, to_string<char>(statistics.type));
	//				}
	//				else
	//				{
	//					if (_using_only == true)
	//						continue;

	//					buf_output.append_text("\x1b[32m" "{:30s} {:8s}" "\x1b[0m", executor_name, to_string<char>(statistics.type));
	//				}

	//				// - "3. id"
	//				{
	//					auto count = statistics.id;

	//					if (count != 0)
	//					{
	//						buf_output.append_text(" {:>4}", count);
	//					}
	//					else
	//					{
	//						buf_output.append_text("\x1b[90m" "    0" "\x1b[0m");
	//					}
	//				}

	//				// - "4. thread_count"
	//				{
	//					if (statistics.now.thread_count != 0)
	//					{
	//						buf_output.append_text(" {:>8}", statistics.now.thread_count);
	//					}
	//					else
	//					{
	//						buf_output.append_text("\x1b[90m" "        0" "\x1b[0m");
	//					}
	//				}

	//				// - "5. executed"
	//				{
	//					auto count = statistics.now.count_execute.load();

	//					if (count != 0)
	//					{
	//						buf_output.append_text(" {:>9}", count);
	//					}
	//					else
	//					{
	//						buf_output.append_text("\x1b[90m" "         0" "\x1b[0m");
	//					}
	//				}

	//				// - "6. pended"
	//				{
	//					auto* pqueue = dynamic_cast<executor::Iqueue*>(iter);

	//					if (pqueue != nullptr)
	//					{
	//						auto count = pqueue->pended_count();

	//						if (count != 0)
	//						{
	//							buf_output.append_text(" {:>12}", count);
	//						}
	//						else
	//						{
	//							buf_output.append_text("\x1b[90m" "            0" "\x1b[0m");
	//						}
	//					}
	//				}

	//				// - new line~
	//				buf_output.append_text("\n");
	//			}
	//			else
	//			{
	//				// "1. name" "2. type"
	//				if (alloc_execute != 0)
	//				{
	//					buf_output.append_text("\x1b[37m" "{:30s} {:8s}" "\x1b[0m", executor_name, to_string<char>(statistics.type));
	//				}
	//				else
	//				{
	//					if (_using_only == true)
	//						continue;

	//					buf_output.append_text("\x1b[90m" "{:30s} {:8s}" "\x1b[0m", executor_name, to_string<char>(statistics.type));
	//				}

	//				// - "3. id"
	//				{
	//					auto count = statistics.id;

	//					if (count != 0)
	//					{
	//						buf_output.append_text(" {:>4}", count);
	//					}
	//					else
	//					{
	//						buf_output.append_text("\x1b[90m" "    0" "\x1b[0m");
	//					}
	//				}

	//				// - "4. thread_count"
	//				{
	//					if (statistics.now.thread_count != 0)
	//					{
	//						buf_output.append_text(" {:>8}", statistics.now.thread_count);
	//					}
	//					else
	//					{
	//						buf_output.append_text("\x1b[90m" "        0" "\x1b[0m");
	//					}
	//				}

	//				// - "5. executed"
	//				{
	//					auto count = statistics.now.count_execute.load();

	//					if (count != 0)
	//					{
	//						buf_output.append_text(" {:>12}", count);
	//					}
	//					else
	//					{
	//						buf_output.append_text("\x1b[90m" "            0" "\x1b[0m");
	//					}
	//				}

	//				// - "6. pended"
	//				{
	//					auto* pqueue = dynamic_cast<executor::Iqueue*>(iter);

	//					if (pqueue != nullptr)
	//					{
	//						auto count = pqueue->pended_count();

	//						if (count != 0)
	//						{
	//							buf_output.append_text(" {:>12}", count);
	//						}
	//						else
	//						{
	//							buf_output.append_text("\x1b[90m" "            0" "\x1b[0m");
	//						}
	//					}
	//				}

	//				// - new line~
	//				buf_output.append_text("\n");
	//			}
	//		}
	//	}
	//	buf_output.append_text("\x1b[37m" "---------------------------------------------------------------------------------------------------------------------------------\n" "\x1b[0m");

	//	// 3) 최종 출력
	//#if defined(_WIN32)
	//	DWORD number_written = 0;
	//	WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), buf_output.data(), buf_output.size<DWORD>(), &number_written, nullptr);
	//#else
	//	fwrite(buf_output.data(), 1, buf_output.size<int>(), stdout);
	//#endif
	//}
	//catch (...)
	//{
	//}
}

void schedulable::manager::process_cancel_schedulable_all() noexcept
{
	// declare) 
	std::set<object_ptr<Ischedulable>> set_schedulable;

	// 1) terminate execting loop
	scoped_lock(this->m_lockable_priorityqueue)
	{
		// - swap
		scoped_lock(this->m_lockable_set_schedulable)
		{
			set_schedulable.swap(this->m_set_schedulable);
		}

		// - clear priority queue
		if (this->m_priorityqueue_schedulable.empty())
		{
			this->m_cv_priorityqueue_exist.notify_one();
		}
		else
		{
			this->m_priorityqueue_schedulable.clear();
			this->m_cv_priorityqueue_in_time.notify_one();
		}
	}

	// 2) call 'process_on_unregister' all
	for(auto& iter: set_schedulable)
	{
		iter->process_on_unregister();
	}
}

bool schedulable::manager::process_post(object_ptr<CGDK::Iexecutable>&& /*_pexecutable*/)
{
	// Error) 이 함수의 Post execute는 불능화되어 있다! 따라서 호출되면 안됀다.
	CGASSERT_ERROR(false);

	// return) 
	return false;
}

bool schedulable::manager::process_execute(chrono::tick::duration /*_ticks_wait*/, intptr_t /*_option*/)
{
	// 1) get current thread
	auto pthread = api::get_current_thread();

	while (this->m_flag_run)
	{
		try
		{
			while (this->m_flag_run)
			{
				conditional_lock(this->m_lockable_priorityqueue, lock_wait)
				{
					// 1) wait until
					if (this->m_priorityqueue_schedulable.empty())
					{
						this->m_cv_priorityqueue_exist.wait(lock_wait);
					}

					// 2) get now tick
					auto tick_now = chrono::tick::now();

					// statistics) 
					pthread->statistics_executing_begin(tick_now);

					// 3) execute 
					while (this->m_priorityqueue_schedulable.empty() == false)
					{
						// - get first eecutable object
						auto* pschedulable = this->m_priorityqueue_schedulable.at(0);

						// - get next tick
						auto next_tick = pschedulable->next_tick();

						// check) wait if not yet 
						if(next_tick > tick_now)
						{
							// statistics) 
							pthread->statistics_executing_end();

							// - wait time
							auto time_wait = next_tick - tick_now;

							// - sleep
							this->m_cv_priorityqueue_in_time.wait_for(lock_wait, time_wait);

							// - get now tick
							tick_now = chrono::tick::now();

							// statistics) 
							pthread->statistics_executing_begin(tick_now);

							// continue)
							continue;
						}

						// - remove 'executable'
						this->_pop_schedulable();

						// - get executor
						auto& pexecutor = pschedulable->m_pexecutor;

						// check) set system executor if executor is nullptr
						if (pexecutor.empty())
						{
							pexecutor = system_executor::executor();
						}

						// - execute 'executable'
						auto pexecutable_scheduler = make_object<executable_schedulable>();
						pexecutable_scheduler->pscheduler = this;
						pexecutable_scheduler->pschedulable = pschedulable;

						// - post execut
						pexecutor->post(std::move(pexecutable_scheduler));

						// statistics)
						++this->m_statistics.now.count_execute;
					}

					// statistics) 
					pthread->statistics_executing_end();
				}
			}
		}
		// On Exception) 
		catch (std::exception& _exception)
		{
			// - Exception Handler를 실행한다.
			throwable::execute_exception_handler(this, _exception, __FUNCTION__);
		}
	}

	// return)
	return true;
}

bool schedulable::manager::process_attach(object_ptr<Ischedulable>&& _pschedulable)
{
	// check) 
	CGASSERT_ERROR(_pschedulable != nullptr);

	// check) 
	CGASSERT_ERROR(_pschedulable != nullptr);

	// check) 
	RETURN_IF(_pschedulable == nullptr, false);

	scoped_lock(this->m_lockable_priorityqueue)
	{
		// check) aleady registered
		RETURN_IF(_pschedulable->m_pschedulable_manager.exist(), false);

		// 1) attach
		_pschedulable->m_pschedulable_manager = this;

		try
		{
			scoped_lock(this->m_lockable_set_schedulable)
			{
				// check) 
				CGASSERT_ERROR(this->m_set_schedulable.find(_pschedulable) == this->m_set_schedulable.end());

				// 2) push
				auto result = _pschedulable->process_on_register();

				// check) 
				RETURN_IF(result == false, false);

				// 3) push_back
				this->m_set_schedulable.emplace(_pschedulable);
			}

			// 3) push schedulable
			this->_push_schedulable(_pschedulable);
		}
		catch (...)
		{
			// - detach from set'schedulable'
			scoped_lock(this->m_lockable_set_schedulable)
			{
				this->m_set_schedulable.erase(_pschedulable);
			}

			// - ...
			_pschedulable->m_pschedulable_manager.reset();

			// reraise)
			throw;
		}
	}

	// return) 
	return true;
}

bool schedulable::manager::process_detach(Ischedulable* _pschedulable) noexcept
{
	// check) 
	RETURN_IF(_pschedulable == nullptr, false);

	// declare)
	object_ptr<schedulable::manager> phold_this;

	scoped_lock(this->m_lockable_priorityqueue)
	{
		// check) 
		RETURN_IF(_pschedulable->m_pschedulable_manager != this, false);

		scoped_lock(this->m_lockable_set_schedulable)
		{
			// 1) detach from set
			auto iter_find = this->m_set_schedulable.find(_pschedulable);

			// check)
			RETURN_IF(iter_find == this->m_set_schedulable.end(), false);

			// 2) erase from set
			this->m_set_schedulable.erase(iter_find);
		}

		// 3) get first executable object
		auto iter_end = this->m_priorityqueue_schedulable.end();
		auto iter = std::find_if(this->m_priorityqueue_schedulable.begin(), iter_end, [=](const Ischedulable* iter) { return iter == _pschedulable;});

		// 4) remove schedulable if exist
		if(iter != iter_end)
		{
			// - get last item
			auto ptarget = std::move(this->m_priorityqueue_schedulable.back());
			auto tick_compare = ptarget->next_tick();

			// - get pos_now of item
			auto pos_now = static_cast<size_t>(iter - this->m_priorityqueue_schedulable.begin());

			// check) 
			if (iter == this->m_priorityqueue_schedulable.begin())
			{
				// - notify one
				this->m_cv_priorityqueue_in_time.notify_one();
			}

			// - remove tail of heap
			this->m_priorityqueue_schedulable.pop_back();

			// check) 
			if (pos_now != this->m_priorityqueue_schedulable.size())
			{
				// check) 
				CGASSERT_ERROR(pos_now < this->m_priorityqueue_schedulable.size());

				// case A) up heap
				if (pos_now != 0 && this->m_priorityqueue_schedulable.at((pos_now - 1) >> 1)->next_tick() > tick_compare)
				{
					// - ...
					auto iter_child = iter;

					do
					{
						// - minus 1 then divied 2
						pos_now = (pos_now - 1) >> 1;

						// - get left node
						auto iter_now = this->m_priorityqueue_schedulable.begin() + pos_now;

						// check) 
						BREAK_IF((*iter_now)->next_tick() <= tick_compare);

						// - move to child node
						*iter_child = std::move(*iter_now);
						iter_child = iter_now;
					} while (pos_now != 0);

					// - 
					*iter_child = std::move(ptarget);
				}
				// case B) down heap
				else
				{
					pos_now = (pos_now << 1) + 1;

					// - size & parent
					auto pos_max = this->m_priorityqueue_schedulable.size() - 1;
					auto iter_parent = iter;

					while (pos_now <= pos_max)
					{
						// - get left node
						auto iter_now = this->m_priorityqueue_schedulable.begin() + pos_now;

						// - select right node if right node is larger then left node
						if (pos_now < pos_max && (*iter_now)->next_tick() >(*(iter_now + 1))->next_tick())
						{
							++iter_now;
						}

						// check) 
						BREAK_IF(tick_compare <= (*iter_now)->next_tick());

						// - move to parent node
						*iter_parent = std::move(*iter_now);
						iter_parent = iter_now;

						// - set new pos_now ( pos_now x 2 + 1 )
						pos_now = (pos_now << 1) + 1;
					}

					*iter_parent = std::move(ptarget);
				}
			}
		}

		// 5) reset schedulable
		phold_this = std::move(_pschedulable->m_pschedulable_manager);
	}

	// 6) Hook
	_pschedulable->process_on_unregister();

	// 7) reset hold
	phold_this.reset();

	// return) 
	return true;
}

void schedulable::manager::_push_schedulable(Ischedulable* _pschedulable)
{
	// check)
	CGASSERT_ERROR(_pschedulable->m_pschedulable_manager == this);

	// check)
	RETURN_IF(_pschedulable->next_tick() == (chrono::tick::max)());

	// 1) store pos_now
	auto pos_now = this->m_priorityqueue_schedulable.size();

	// 2) push back item
	this->m_priorityqueue_schedulable.emplace_back(_pschedulable);

	// 3) heap up
	auto iter_child = --this->m_priorityqueue_schedulable.end();

	while (pos_now != 0)
	{
		// - 빼기 1 후 나누기 2한다.
		pos_now = ((pos_now - 1) >> 1);

		// - ...
		auto iter_now = this->m_priorityqueue_schedulable.begin() + pos_now;

		// check) 
		BREAK_IF((*iter_now)->next_tick() <= _pschedulable->next_tick());

		// - move to down
		*iter_child = std::move(*iter_now);
		iter_child = iter_now;
	}

	// 4) ...
	*iter_child = std::move(_pschedulable);

	// check)
	CGASSERT_ERROR(this->m_priorityqueue_schedulable.empty() == false);

	// 5) notify_one
	if (this->m_priorityqueue_schedulable.size() == 1)
	{
		this->m_cv_priorityqueue_exist.notify_one();
	}
	else if (_pschedulable == this->m_priorityqueue_schedulable[0])
	{
		this->m_cv_priorityqueue_in_time.notify_one();
	}
}

void schedulable::manager::_pop_schedulable() noexcept
{
	// 1) 
	auto ptarget = std::move(this->m_priorityqueue_schedulable.back());
	auto tick_compare = ptarget->next_tick();

	// 2) target객체를 제거하고 제일 마지막 객체를 그 위치로 가져 온다.
	this->m_priorityqueue_schedulable.pop_back();

	// check) 
	RETURN_IF(this->m_priorityqueue_schedulable.empty())

	// 3) Size & Parent
	auto pos_max = this->m_priorityqueue_schedulable.size() - 1;
	auto pos_now = size_t(1);
	auto iter_parent = this->m_priorityqueue_schedulable.begin();

	// 4) down heap
	while (pos_now <= pos_max)
	{
		// - get now node
		auto iter_now = this->m_priorityqueue_schedulable.begin() + pos_now;

		// - Left가 더 크면 Right 선택한다.
		if (pos_now < pos_max && (*iter_now)->next_tick() > (*(iter_now + 1))->next_tick())
		{
			++iter_now;
			++pos_now;
		}

		// check) Child가 더 크면 끝냄.
		BREAK_IF(tick_compare <= (*iter_now)->next_tick());

		// - move to up
		*iter_parent = std::move(*iter_now);
		iter_parent = iter_now;

		// - set child node as new pos_now (pos_now = pos_now x 2 + 1)
		pos_now = (pos_now << 1) + 1;
	}

	*iter_parent = std::move(ptarget);
}

void schedulable::manager::process_execute_schedulable(object_ptr<Ischedulable>&& _pschedulable)
{
	scoped_lock(this->m_lockable_priorityqueue)
	{
		// check)
		RETURN_IF(_pschedulable->m_pschedulable_manager != this);

		// check)
		RETURN_IF(_pschedulable->next_tick() == (chrono::tick::max)());
	}

	// 1) execute
	auto result = _pschedulable->process_schedule();

	// check)
	RETURN_IF(result == false);

	// 2) re-push
	scoped_lock(this->m_lockable_priorityqueue)
	{
		// check)
		RETURN_IF(_pschedulable->m_pschedulable_manager != this);

		// - push again
		this->_push_schedulable(_pschedulable);
	}
}

uint32_t schedulable::manager::fn_thread(schedulable::manager* _this)
{
	// statistics)
	++_this->m_statistics.now.thread_count;

	// 1) call 'execute'
	auto result = _this->execute(chrono::tick::duration(), 0);

	// statistics)
	--_this->m_statistics.now.thread_count;

	// return) 
	return result;
}
