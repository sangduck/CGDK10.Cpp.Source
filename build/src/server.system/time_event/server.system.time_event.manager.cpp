#include "pch.h"

using namespace CGDK;


time_event::entity_manager::entity_manager() :
	Nexecutor("time event entity manager"),
	m_is_delete_done_entity(true)
{
	// 1) register to factory manager
	this->process_register_executor();

	// 2) set..
	m_statistics.type = eEXECUTOR_TYPE::SYSTEM;
}

time_event::entity_manager::~entity_manager() noexcept
{
	// 1) unregister to factory manager
	this->process_unregister_executor();
}

void time_event::entity_manager::initialize_instance()
{
	// declare) 
	context context_start;

	// 1) start
	initialize_instance(context_start);
}

void time_event::entity_manager::initialize_instance(context& _context)
{
	// declare)
	object_ptr<time_event::entity_manager> pentity_manager;

	scoped_lock(m_lockable_create)
	{
		// check) 
		RETURN_IF(m_pinstance.exist())

		// 1) create own
		m_pinstance = _make_own_raw<time_event::entity_manager>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL);

		// 2) set instance
		pentity_manager = m_pinstance;
	}

	// 3) start manager
	pentity_manager->start(_context);
}

void time_event::entity_manager::set_instance(own_ptr<entity_manager>&& _pinstance) noexcept
{
	// declare)
	own_ptr<time_event::entity_manager> pentity_manager_pre;

	scoped_lock(m_lockable_create)
	{
		// 1) move old instance
		pentity_manager_pre = std::move(m_pinstance);

		// 2) set new instance
		m_pinstance = std::move(_pinstance);
	}

	// 3) reset old instance
	pentity_manager_pre.reset();
}
void time_event::entity_manager::reset_instance() noexcept
{
	// declare)
	own_ptr<time_event::entity_manager> pentity_manager;

	scoped_lock(m_lockable_create)
	{
		// 1) move instance
		pentity_manager = std::move(m_pinstance);
	}

	// 2) reset old instance
	pentity_manager.reset();
}

void time_event::entity_manager::on_initializing(context& /*_context*/)
{
}

void time_event::entity_manager::on_initialize(context& /*_msg*/)
{
}

void time_event::entity_manager::on_destroying()
{
}

void time_event::entity_manager::on_destroy()
{
}

void time_event::entity_manager::on_starting(context& /*_msg*/)
{
}

void time_event::entity_manager::on_start(context& /*_msg*/)
{
	// trace) 
	LOG_PROGRESS << "@ 'time_event::entity_manager' start"sv;

	// 1) set executor
	this->m_pexecutor = system_executor::executor();

	// 2) set m_flag_run 'true' and get old state
	this->m_flag_run = 1;

	// 3) reserve
	scoped_lock(this->m_lock_priorityqueue)
	{
		this->m_priorityqueue_time_entity.reserve(16384);
	}

	try
	{
		// 4) begin thread
		begin_thread(fn_thread, this);
	}
	catch (...)
	{
		// - stop
		stop();

		// reraise)
		throw;
	}
}

void time_event::entity_manager::on_stopping()
{
	// 1) set m_flag_run 'false' and get old state
	this->m_flag_run = 0;

	// 2) cancel all executable
	this->unregister_event_all();

	// 3) wait until thread is terminated
	this->wait();

	// 4) reset executor
	this->m_pexecutor.reset();

	// trace) 
	LOG_PROGRESS << "@ 'time_event::entity_manager' stop"sv;
}

void time_event::entity_manager::on_stop()
{
}

result_code time_event::entity_manager::process_update(context& /*_context*/)
{
	return eRESULT::SUCCESS;
}

void time_event::entity_manager::unregister_event_all() noexcept
{
	// declare) 
	std::set<object_ptr<Ientity>> set_time_entity;

	// 1) terminate execting loop
	scoped_lock(this->m_lock_priorityqueue)
	{
		// - swap
		scoped_lock(this->m_set_time_entity)
		{
			set_time_entity.swap(this->m_set_time_entity);
		}

		if (this->m_priorityqueue_time_entity.empty())
		{
			this->m_cv_priorityqueue_exist.notify_one();
		}
		else
		{
			this->m_priorityqueue_time_entity.clear();
			this->m_cv_priorityqueue_in_time.notify_one();
		}
	}

	// 2) call 'stop' all
	for (auto& iter : set_time_entity)
	{
		// - start _ptime_entity
		iter->stop();

		// - transmit message
		transmit_message(sMESSAGE_NOTIFY(eMESSAGE::SERVER::TIME_EVENT::EVENT_UNREGISTER, 0, reinterpret_cast<uintptr_t>(iter.get())));
	}
}

object_ptr<time_event::Ientity> time_event::entity_manager::find_event(uint64_t _id_event) const noexcept
{
	scoped_lock(this->m_lock_priorityqueue)
	{
		// 1) find event
		for (auto& iter : this->m_set_time_entity)
		{
			if (iter->entity_status().id == _id_event)
			{
				return iter;
			}
		}
	}

	// return) failed
	return object_ptr<time_event::Ientity>();
}

bool time_event::entity_manager::register_class(std::string_view _class_name, std::function<own_ptr<time_event::Ientity>()>& _f_register)
{
	std::string str_name(_class_name);

	scoped_lock(this->m_map_time_class)
	{
		auto iter_find = this->m_map_time_class.find(str_name);

		if (iter_find == this->m_map_time_class.end())
		{
			this->m_map_time_class.insert({ std::move(str_name) , _f_register });
		}
		else
		{
			iter_find->second = _f_register;
		}
	}

	// returnn) 
	return true;
}

bool time_event::entity_manager::unregister_class(std::string_view _class_name)
{
	scoped_lock(this->m_map_time_class)
	{
		// 1) find
		auto iter_find = this->m_map_time_class.find(std::string(_class_name));

		// check) 
		RETURN_IF(iter_find == m_map_time_class.end(), false);

		// 2) erase ...
		m_map_time_class.erase(iter_find);
	}

	// returnn) 
	return true;
}

bool time_event::entity_manager::_is_exist(uint64_t _id_event) const noexcept
{
	// 1) find event
	for (auto& iter : this->m_set_time_entity)
	{
		if (iter->entity_status().id == _id_event)
		{
			return true;
		}
	}

	// return) failed
	return false;
}

bool time_event::entity_manager::_is_exist(Ientity* _pentity) const noexcept
{
	return this->m_set_time_entity.find(_pentity) != this->m_set_time_entity.end();
}

bool time_event::entity_manager::process_attach(object_ptr<time_event::Ientity>&& _ptime_entity)
{
	// check) 
	ERROR_RETURN_IF(_ptime_entity.empty(), false, LOG_ERROR << "(err ) _ptime_entity is nullptr"sv);

	// check) 
	RETURN_IF(get_object_state() != eOBJECT_STATE::RUNNING, false);

	// declare)
	object_ptr<time_event::Ientity> ptime_entity = std::move(_ptime_entity);

	scoped_lock(this->m_lock_priorityqueue)
	{
		scoped_lock(this->m_set_time_entity)
		{
			// 1) find 
			auto iter_find = this->m_set_time_entity.find(ptime_entity);

			// check) 
			RETURN_IF(iter_find != this->m_set_time_entity.end(), false);

			// check) 
			CGASSERT_ERROR(std::find(m_priorityqueue_time_entity.begin(), m_priorityqueue_time_entity.end(), ptime_entity) == m_priorityqueue_time_entity.end());

			// 2) push_back
			this->m_set_time_entity.insert(ptime_entity);
		}

		// log) 
		ptime_entity->_append_result_log("@register");

		// 2) set sevent setup time
		ptime_entity->entity_status().state = time_event::eSTATE::RUN;
		ptime_entity->entity_status().time_setup = chrono::time::utc::now();

		// 4) push event object
		try
		{
			this->_push_time_entity(ptime_entity);
		}
		catch (...)
		{
			// - detach from set'schedulable'
			scoped_lock(this->m_set_time_entity)
			{
				this->m_set_time_entity.erase(ptime_entity);
			}

			// - rollback
			scoped_lock(this->m_set_time_entity)
			{
				this->m_set_time_entity.erase(ptime_entity);
			}

			// - detach

			// reraise) 
			throw;
		}
	}

	// 5) transmit message
	this->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::SERVER::TIME_EVENT::EVENT_REGISTER, 0, reinterpret_cast<uintptr_t>(ptime_entity.get())));

	// return)
	return true;
}

bool time_event::entity_manager::process_detach(time_event::Ientity* _ptime_entity) noexcept
{
	// declare) 
	object_ptr<time_event::Ientity> ptime_entity;

	// check)
	ERROR_RETURN_IF(_ptime_entity == nullptr, ptime_entity, LOG_ERROR << "(err ) _ptime_entity is nullptr"sv);

	scoped_lock(this->m_lock_priorityqueue)
	{
		scoped_lock(this->m_set_time_entity)
		{
			// 1) find 
			auto iter_find = m_set_time_entity.find(_ptime_entity);

			// check) 
			RETURN_IF(iter_find == m_set_time_entity.end(), false);

			// check) 
			CGASSERT_ERROR(std::find(m_priorityqueue_time_entity.begin(), m_priorityqueue_time_entity.end(), _ptime_entity) == m_priorityqueue_time_entity.end());

			// 2) move pevent object
			ptime_entity = *iter_find;

			// 3) push_back
			m_set_time_entity.erase(iter_find);
		}

		// log)
		_ptime_entity->_append_result_log("@unregister");

		// 4) remove from heap
		this->_process_detach(ptime_entity);
	}

	// 5) start _ptime_entity
	_ptime_entity->stop();

	// 6) transmit message
	transmit_message(sMESSAGE_NOTIFY(eMESSAGE::SERVER::TIME_EVENT::EVENT_UNREGISTER, 0, reinterpret_cast<uintptr_t>(_ptime_entity)));

	// return)
	return true;
}

object_ptr<time_event::Ientity> time_event::entity_manager::unregister_entity(uint64_t _id_event) noexcept
{
	// declare) 
	object_ptr<time_event::Ientity> pentity;

	scoped_lock(m_lock_priorityqueue)
	{
		scoped_lock(m_set_time_entity)
		{
			// 1) 지울 Event를 찾는다.
			auto iter = m_set_time_entity.begin();
			auto iter_end = m_set_time_entity.end();

			for (; iter != iter_end; ++iter)
			{
				BREAK_IF((*iter)->entity_status().id == _id_event);
			}

			// check) 
			RETURN_IF(iter == iter_end, pentity);

			// 2) move pevent object
			pentity = *iter;

			// 3) erase event
			m_set_time_entity.erase(iter);
		}

		// log)
		pentity->_append_result_log("@unregister");

		// 4) remove from heap
		this->_process_detach(pentity);
	}

	// 5) start _ptime_entity
	pentity->stop();

	// 6) transmit message
	transmit_message(sMESSAGE_NOTIFY(eMESSAGE::SERVER::TIME_EVENT::EVENT_UNREGISTER, 0, reinterpret_cast<uintptr_t>(pentity.get())));

	// return)
	return pentity;
}

bool time_event::entity_manager::_process_detach(Ientity* _ptime_entity) noexcept
{
	//// check) 
	//RETURN_IF(_ptime_entity->m_pscheduler != this, false);

	// 1) get first executable object
	auto iter_end = this->m_priorityqueue_time_entity.end();
	auto iter = std::find_if(this->m_priorityqueue_time_entity.begin(), iter_end, [=](const object_ptr<Ientity>& iter) { return iter == _ptime_entity; });

	// 2) remove schedulable if exist
	if (iter != iter_end)
	{
		// - get last item
		auto ptarget = std::move(this->m_priorityqueue_time_entity.back());
		auto tick_compare = ptarget->entity_status().time_next;

		// - get pos_now of item
		auto pos_now = static_cast<size_t>(iter - this->m_priorityqueue_time_entity.begin());

		// check) 
		if (iter == this->m_priorityqueue_time_entity.begin())
		{
			// - notify one
			this->m_cv_priorityqueue_in_time.notify_one();
		}

		// - remove tail of heap
		this->m_priorityqueue_time_entity.pop_back();

		// check) 
		if (pos_now != this->m_priorityqueue_time_entity.size())
		{
			// check) 
			CGASSERT_ERROR(pos_now < this->m_priorityqueue_time_entity.size());

			// case A) up heap
			if (pos_now != 0 && this->m_priorityqueue_time_entity.at((pos_now - 1) >> 1)->entity_status().time_next > tick_compare)
			{
				// - ...
				auto iter_child = iter;

				do
				{
					// - minus 1 then divied 2
					pos_now = (pos_now - 1) >> 1;

					// - get left node
					auto iter_now = this->m_priorityqueue_time_entity.begin() + pos_now;

					// check) 
					BREAK_IF((*iter_now)->entity_status().time_next <= tick_compare);

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
				auto pos_max = this->m_priorityqueue_time_entity.size() - 1;
				auto iter_parent = iter;

				while (pos_now <= pos_max)
				{
					// - get left node
					auto iter_now = this->m_priorityqueue_time_entity.begin() + pos_now;

					// - select right node if right node is larger then left node
					if (pos_now < pos_max && (*iter_now)->entity_status().time_next >(*(iter_now + 1))->entity_status().time_next)
					{
						++iter_now;
					}

					// check) 
					BREAK_IF(tick_compare <= (*iter_now)->entity_status().time_next);

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

	// return) 
	return true;
}

void time_event::entity_manager::_push_time_entity(Ientity* _ptime_entity)
{
	//// check)
	//CGASSERT_ERROR(_ptime_entity->m_pscheduler == this);

	// 1) store pos_now
	auto pos_now = this->m_priorityqueue_time_entity.size();

	// 2) push back item
	this->m_priorityqueue_time_entity.push_back(_ptime_entity);

	// 3) heap up
	auto iter_child = --this->m_priorityqueue_time_entity.end();

	while (pos_now != 0)
	{
		// - 빼기 1 후 나누기 2한다.
		pos_now = ((pos_now - 1) >> 1);

		// - ...
		auto iter_now = this->m_priorityqueue_time_entity.begin() + pos_now;

		// check) 
		BREAK_IF((*iter_now)->entity_status().time_next <= _ptime_entity->entity_status().time_next);

		// - move to down
		*iter_child = std::move(*iter_now);
		iter_child = iter_now;
	}

	// 4) ...
	*iter_child = _ptime_entity;

	// check)
	CGASSERT_ERROR(this->m_priorityqueue_time_entity.empty() == false);

	// 5) notify_one
	if (this->m_priorityqueue_time_entity.size() == 1)
	{
		this->m_cv_priorityqueue_exist.notify_one();
	}
	else if (_ptime_entity == this->m_priorityqueue_time_entity[0])
	{
		this->m_cv_priorityqueue_in_time.notify_one();
	}
}

void time_event::entity_manager::_pop_time_entity() noexcept
{
	// 1) 
	auto ptarget = std::move(this->m_priorityqueue_time_entity.back());
	auto tick_compare = ptarget->entity_status().time_next;

	// 2) target객체를 제거하고 제일 마지막 객체를 그 위치로 가져 온다.
	this->m_priorityqueue_time_entity.pop_back();

	// check) 
	RETURN_IF(this->m_priorityqueue_time_entity.empty())

	// 3) size & parent
	auto pos_max = this->m_priorityqueue_time_entity.size() - 1;
	auto pos_now = size_t(1);
	auto iter_parent = this->m_priorityqueue_time_entity.begin();

	// 4) down heap
	while (pos_now <= pos_max)
	{
		// - get now node
		auto iter_now = this->m_priorityqueue_time_entity.begin() + pos_now;

		// - Left가 더 크면 Right 선택한다.
		if (pos_now < pos_max && (*iter_now)->entity_status().time_next >(*(iter_now + 1))->entity_status().time_next)
		{
			++iter_now;
			++pos_now;
		}

		// check) Child가 더 크면 끝냄.
		BREAK_IF(tick_compare <= (*iter_now)->entity_status().time_next);

		// - move to up
		*iter_parent = std::move(*iter_now);
		iter_parent = iter_now;

		// - set child node as new pos_now (pos_now = pos_now x 2 + 1)
		pos_now = (pos_now << 1) + 1;
	}

	*iter_parent = std::move(ptarget);
}

void time_event::entity_manager::process_notify_next_time_changed(time_event::Ientity* _ptime_entity)
{
	conditional_lock(m_lock_priorityqueue, lock_wait)
	{
		// check)
		RETURN_IF(this->m_priorityqueue_time_entity.empty());

		if (this->m_priorityqueue_time_entity.at(0) == _ptime_entity)
		{
			this->m_cv_priorityqueue_in_time.notify_one();
		}
	}
}

bool time_event::entity_manager::process_post(object_ptr<Iexecutable>&& /*_pexecutable*/)
{
	return true;
}

bool time_event::entity_manager::process_execute(chrono::tick::duration /*_ticks_wait*/, intptr_t /*_option*/)
{
	while (m_flag_run)
	{
		try
		{
			while (m_flag_run)
			{
				conditional_lock(this->m_lock_priorityqueue, lock_wait)
				{
					// 1) wait until
					if (this->m_priorityqueue_time_entity.empty())
					{
						this->m_cv_priorityqueue_exist.wait(lock_wait);
					}

					// 2) get now tick
					auto time_now = chrono::time::utc::now();

					// 3) execute 
					while (this->m_priorityqueue_time_entity.empty() == false)
					{
						// - get first eecutable object
						object_ptr<Ientity> ptime_entity = this->m_priorityqueue_time_entity.at(0);

						// - get next tick
						auto next_time = ptime_entity->entity_status().time_next;

						// check) wait if not yet 
						if(next_time > time_now)
						{
							// - wait time
							auto time_wait = next_time - time_now;

							// - sleep
							this->m_cv_priorityqueue_in_time.wait_for(lock_wait, time_wait);

							// - get now tick
							time_now = chrono::time::utc::now();

							// continue)
							continue;
						}

						// - remove 'executable'
						this->_pop_time_entity();

						// - get executor
						auto pexecutor = ptime_entity->executor();

						// check)
						if (pexecutor.empty())
						{
							pexecutor = this->executor();

							// check)
							BREAK_IF(pexecutor.empty());
						}

						// - post executor
						pexecutor->post(std::move(ptime_entity));
					}
				}	
			}
		}
		// On exception) 
		catch (std::exception& _exception)
		{
			// - Exception Handler를 실행한다.
			throwable::execute_exception_handler(this, _exception, __FUNCTION__);
		}
	}

	// return)
	return true;
}

uint32_t time_event::entity_manager::fn_thread(time_event::entity_manager* _this)
{
	_this->execute(chrono::tick::duration(), 0);

	return 0;
}
