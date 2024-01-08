#include "pch.h"

using namespace CGDK;


net::io::restore_manager::restore_manager() :
	Nnameable("restore manager")
{
	m_tick_timeout = chrono::seconds(10);
}
net::io::restore_manager::~restore_manager() noexcept
{

}

void net::io::restore_manager::set_suspend_timeout(chrono::tick::duration _tick_duration)
{
	this->m_tick_timeout = _tick_duration;
}

object_ptr<net::io::Irestorable_server>	net::io::restore_manager::process_alloc_restorable()
{
	return make_object<net::io::Irestorable_server>();
}

void net::io::restore_manager::restore_restorable(Irestore_server* _prestore_server, buffer_view _buffer)
{
	// check) _prestore_server가 nullptr이면 안됀다.
	CGASSERT_ERROR(_prestore_server != nullptr);

	// check) 먼저 Server ID가 동일한지 확인한다.
	THROW_IF(_prestore_server == nullptr, throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 1) 데이터를 읽어들이기 위한 임시 버퍼
	_buffer.extract(size(8));

	// 2) 메시지의 데이터들을 읽어들인다.
	auto id_restore_server = _buffer.extract<RESTORE_SERVER_ID>();
	auto id_restore_client = _buffer.extract<RESTORE_CLIENT_ID>();

	// 3) message를 얻어낸다.
	auto ackReceived = _buffer.extract<SEQUENCE_ID>();
	auto ack_sended = _buffer.extract<SEQUENCE_ID>();

	// check) 먼저 Server ID가 동일한지 확인한다.
	THROW_IF(this->m_id_restore_server != id_restore_server, throwable::failure(eRESULT::DOMAIN_ERROR))

	// declare) 
	object_ptr<net::io::Irestorable_server>	prestorable_server;
	result_code result;

	scoped_lock(this->m_map_restorable)
	{
		// 1) 먼저 존재하는 것 중에 있는지 찾는다.
		auto iter_find = this->m_map_restorable.find(id_restore_client);

		// 2) 만약 찾았을 경우 그것을 사용한다.
		if(iter_find != this->m_map_restorable.end())
		{
			// - 저장한다.
			prestorable_server = iter_find->second;

			// - 일단 제거한다.
			this->m_map_restorable.erase(iter_find);

			// - 찾은 것이 복원 가능한지 여부를 확인한다.
			//   복원 가능(true) -> 그것을 사용한다.
			//   복원 불가능(false) -> 신규 발급한다.
			auto result_nested = prestorable_server->process_get_restore_result(id_restore_server, id_restore_client, ackReceived, ack_sended);

			// - 복원 가능(true)이면 그것을 m_map_restorable을 제거하고 사용한다.
			if(result_nested != eRESULT::RESTORE)
			{
				prestorable_server->process_reset_io_restorable();
			}
		}

		// 3-1) 찾지 못했으면 신규로 생성한 후 전달한다.
		if(prestorable_server.empty())
		{
			// - 새로운 Restorable 객체를 생성한다.
			prestorable_server = _prestore_server->process_alloc_restorable();

			// - [RESTORE_SERVER_ID]를 써넣는다.
			prestorable_server->m_id_restore_server = m_id_restore_server;

			// - Result를 eRESULT::RESET로...
			result = eRESULT::RESET;
		}
		// 3-2) 찾았으면 그것을 사용해 RESTORE하도록 한다.
		else
		{
			// - Result를 eRESULT::RESTORE로...
			result = eRESULT::RESTORE;
		}

		// 4) 새로운 [RESTORE_CLIENT_ID]를 발급받고 기타 등등 설정한다.
		prestorable_server->m_id_restore_client = process_generate_RESTORE_CLIENT_ID();
		prestorable_server->m_prestore_server = _prestore_server;
		_prestore_server->m_prestorable = prestorable_server;

		// 5) 
		if (m_map_restorable.size() == 0)
		{
			system_executor::unregister_schedulable(this);
		}
	}

	// - IORestore를 요청하는 메시지를 전송한다.
	prestorable_server->process_request_io_restore(result, ackReceived);
}

bool net::io::restore_manager::suspend_restorable(Irestore_server* _prestore_server, uint64_t _disconnect_reason) noexcept
{
	// check) Active close이면 무조건 강제 종료한다.
	RETURN_IF((_disconnect_reason & DISCONNECT_REASON_ACTIVE) != 0, false);

	// check)
	RETURN_IF(_prestore_server == nullptr, false);

	// declare) 
	object_ptr<Irestorable_server>	prestorable_server;
	bool result;

	scoped_lock(m_map_restorable)
	{
		// check) 
		CGASSERT_ERROR(_prestore_server->m_prestore_manager==this);

		// check)
		RETURN_IF(_prestore_server->m_prestore_manager!=this, false);

		// 1) Restorable Server를 얻는다.
		prestorable_server = _prestore_server->m_prestorable.get();

		// check) 
		CGASSERT_ERROR(prestorable_server != nullptr);

		// check) prestorable_server가 nullptr이면 false를 리턴하고 끝낸다.
		RETURN_IF(prestorable_server==nullptr, false);

		// 2) Restore Client ID를 얻는다.
		auto id_restore_client = prestorable_server->m_id_restore_client;

		// 3) 먼저 존재하는 것 중에 있는지 찾는다.
		auto iter_find = this->m_map_restorable.find(id_restore_client);

		// check) 찾았다면 그건 잘못된 것이다.
		CGASSERT_ERROR(iter_find == this->m_map_restorable.end());

		// 4) 존재하지 않는다면 추가한다.(존재하지 않는 것이 정상이다.)
		if(iter_find == this->m_map_restorable.end())
		{
			this->m_map_restorable.emplace(id_restore_client, prestorable_server);

			result = true;
		}
		else
		{
			prestorable_server->m_id_restore_client.reset();
			prestorable_server->m_prestore_server.reset();

			result = false;
		}

		// 5) reset한다.
		_prestore_server->m_prestorable.reset();

		// 6) unregister shcedulable
		if (result == true && this->m_map_restorable.size() == 1)
		{
			system_executor::register_schedulable(this);
		}
	}

	// 6-1) Restore manager에서 suspend가 성공했을 경우 on_suspend함수가 호출된다.
	if (result == true)
	{
		prestorable_server->process_on_suspend(_disconnect_reason);
	}
	// 6-2) Restore manager에서 suspend가 실패했을 경우 process_complete_disconnect함수가 호출된다.
	//      (일만적으로 process_complete_disconnect내에서 on_disconnect()가 호출된다.)
	else
	{
		// - process_complete_disconnect를 호출한다.
		prestorable_server->process_complete_disconnect();

		// - reset한다.
		prestorable_server->process_reset_io_restorable();
	}

	// return)
	return true;
}

RESTORE_CLIENT_ID net::io::restore_manager::process_generate_RESTORE_CLIENT_ID()
{
	// declare) 
	RESTORE_CLIENT_ID id_restore_client;

	static uint64_t sXXX = 0;

	id_restore_client.id_key = ++sXXX;
	id_restore_client.id_certify = rand();

	// return)
	return	id_restore_client;
}

void net::io::restore_manager::on_execute()
{
	// 1) tick_now를 얻는다.
	auto tick_now = chrono::tick::now();

	// declare)
	std::vector<object_ptr<Irestorable_server>> vector_restorable;

	// 2) Timeout된 
	scoped_lock(this->m_map_restorable)
	{
		for(auto iter = this->m_map_restorable.begin(); iter != this->m_map_restorable.end();)
		{
			// - duration을 얻는다.
			auto duration = tick_now - iter->second->get_io_suspend_tick();

			// check) Timeout이 지난 Irestorable_server를 골라낸다.
			if(duration > m_tick_timeout)
			{
				vector_restorable.emplace_back(iter->second);

				iter = this->m_map_restorable.erase(iter);
			}
			else
			{
				++iter;
			}
		}
	}

	// 3) Timeout된 Irestorable_server들을 Closesocket한다.
	for(auto& iter: vector_restorable)
	{
		iter->closesocket();
	}
}

void net::io::restore_manager::initialize_instance()
{
	scoped_lock(m_lockable_create)
	{
		// check) 
		RETURN_IF(m_pinstance.exist());

		// 1) create instance
		m_pinstance = _make_own_raw<restore_manager>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL);
	}
}
void net::io::restore_manager::set_instance(own_ptr<restore_manager>&& _pinstance) noexcept
{
	// declare)
	own_ptr<restore_manager> pinstance_pre;

	scoped_lock(m_lockable_create)
	{
		// 1) move old instance
		pinstance_pre = std::move(m_pinstance);

		// 2) set new instance
		m_pinstance = std::move(_pinstance);
	}

	// 3) destroy instance
	if (pinstance_pre.exist())
	{
		// - unregister schedulable
		system_executor::unregister_schedulable(pinstance_pre);

		// - reset old instance
		pinstance_pre.reset();
	}
}

void net::io::restore_manager::reset_instance() noexcept
{
	// declare) 
	own_ptr<restore_manager> pinstance;

	scoped_lock(m_lockable_create)
	{
		// 1) move instance
		pinstance = std::move(m_pinstance);
	}

	// 2) reset old instance
	if (pinstance.exist())
	{
		// - unregister schedulable
		system_executor::unregister_schedulable(pinstance);

		// - reset instance (destroy)
		pinstance.reset();
	}
}

result_code net::io::restore_manager::destroy() noexcept
{
	// return) 
	return eRESULT::SUCCESS;
}
