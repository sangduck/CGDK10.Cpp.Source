namespace CGDK
{

template <class TENTITY, class TCREATE>
doss::repository::remote<TENTITY, TCREATE>::remote() :
	doss::Nrepository<TENTITY>(),
	Nmessage_transmitter(),
	net::io::connect_requestable::Ntcp_reconnect(),
	net::socket::tcp<>()
{
	object::Iidentifiable::set_object_type(eOBJECT_TYPE::REPOSITORY_REMOTE);

	this->Irepository::m_id_repository = sREPOSITORY_ID::generate();

	doss::manager_repository::get_instance()->register_repository(this);

	this->maximum_send_buffer_size(32768);
}

template <class TENTITY, class TCREATE>
doss::repository::remote<TENTITY, TCREATE>::~remote() noexcept
{
	doss::manager_repository::get_instance()->unregister_repository(this);
}

template <class TENTITY, class TCREATE>
void doss::repository::remote<TENTITY, TCREATE>::on_initializing(context& )
{
	Irepository::m_id_repository = sREPOSITORY_ID::generate();
}

template <class TENTITY, class TCREATE>
void doss::repository::remote<TENTITY, TCREATE>::on_initialize(context& )
{
}

template <class TENTITY, class TCREATE>
void doss::repository::remote<TENTITY, TCREATE>::on_destroying()
{
	this->checkout_all();
}

template <class TENTITY, class TCREATE>
void doss::repository::remote<TENTITY, TCREATE>::on_destroy()
{
	// - break the references between entities, transactions and sessions
	this->clear_entities();

	this->Irepository::m_id_repository.reset();
}

template <class TENTITY, class TCREATE>
void doss::repository::remote<TENTITY, TCREATE>::on_starting(context& )
{
	LOG_PROGRESS << "@ <BEGIN> start doss REPOSITYR_REMOTE['" << name() << "']";

	this->enable_reconnect();

	this->set_reconnect_interval(chrono::seconds(5));

	this->execute_interval(chrono::seconds(10));

	system_executor::register_schedulable(this);
}

template <class TENTITY, class TCREATE>
void doss::repository::remote<TENTITY, TCREATE>::on_start(context& )
{
	system_executor::unregister_schedulable(this);

	LOG_PROGRESS << "@ <END> start doss REPOSITORY_REMOTE['" << name() << "']";
}

template <class TENTITY, class TCREATE>
void doss::repository::remote<TENTITY, TCREATE>::on_stop_request()
{
	set_object_state_if(eOBJECT_STATE::ESTABLISHED, eOBJECT_STATE::RUNNING);
}

template <class TENTITY, class TCREATE>
void doss::repository::remote<TENTITY, TCREATE>::on_stopping()
{
	LOG_PROGRESS << "@ <BEGIN> stop doss REPOSITORY_REMOTE['" << name() << "']";

	this->disable_reconnect();
}

template <class TENTITY, class TCREATE>
void doss::repository::remote<TENTITY, TCREATE>::on_stop()
{
	LOG_PROGRESS << "@ <END> stop doss REPOSITORY_REMOTE['" << name() << "']";
}

template <class TENTITY, class TCREATE>
void doss::repository::remote<TENTITY, TCREATE>::on_request_connect(const net::sockaddr& _address)
{
	LOG_INFO << " > doss REPOSITORY_REMOTE['" << name() << "']: retry connecting to '" << to_string(_address) << "' ";
}

template <class TENTITY, class TCREATE>
void doss::repository::remote<TENTITY, TCREATE>::on_connect()
{
	LOG_INFO << " > doss REPOSITORY_REMOTE['" << name() << "']: socket connected (" << remote_endpoint().address().to_string() << ")";
	LOG_INFO << "     rid : " << this->m_id_repository.object_id().value;
	LOG_INFO << "     remote rid : " << this->m_id_repository.object_id().value;

	this->send(message_SERVER_DOSS_REQUEST_CERTIFY(this->m_id_repository, this->m_id_repository_remote_alloc, this->m_receive_sequence.load()));
}

template <class TENTITY, class TCREATE>
void doss::repository::remote<TENTITY, TCREATE>::on_fail_connect(uint64_t )
{
	LOG_INFO << " < doss REPOSITORY_REMOTE['" << name() << "']: fail to connect";
}

template <class TENTITY, class TCREATE>
void doss::repository::remote<TENTITY, TCREATE>::on_disconnect(uint64_t )
{
	LOG_INFO << " < doss REPOSITORY_REMOTE['" << name() << "']: socket disconnected (" << remote_endpoint().address().to_string() << ")";
}

template <class TENTITY, class TCREATE>
void doss::repository::remote<TENTITY, TCREATE>::on_execute()
{
}

template <class TENTITY, class TCREATE>
BEGIN_CGMESSAGE_MAP_TEMPLATE(doss::repository::remote, TENTITY, TCREATE)
	CGMESSAGE_TYPE_SUB_BUFFER_NETWORK
		ON_CGMESSAGE(eMESSAGE::SERVER::DOSS::RESPONSE_CERTIFY,	on_message_SERVER_DOSS_RESPONSE_CERTIFY, sMESSAGE_NETWORK)
		ON_CGMESSAGE(eMESSAGE::SERVER::DOSS::COMMAND_RESPONSE,	on_message_SERVER_DOSS_COMMAND_RESPONSE, sMESSAGE_NETWORK)
		ON_CGMESSAGE(eMESSAGE::SERVER::DOSS::CONTROL_REQUEST,	on_message_SERVER_DOSS_CONTROL_REQUEST, sMESSAGE_NETWORK)
		ON_CGMESSAGE(eMESSAGE::SERVER::DOSS::SEND_ACK,			on_message_SERVER_DOSS_SEND_ACK,		sMESSAGE_NETWORK)
	END_SUB_CGMESSAGE_MAP
END_CGMESSAGE_MAP

template <class TENTITY, class TCREATE>
result_code doss::repository::remote<TENTITY, TCREATE>::on_message(sMESSAGE_NETWORK& _msg)
{
	EXECUTE_CGMESSAGE_MAP(_msg);

	return eRESULT::BYPASS;
}

template <class TENTITY, class TCREATE>
result_code doss::repository::remote<TENTITY, TCREATE>::on_message_SERVER_DOSS_RESPONSE_CERTIFY(sMESSAGE_NETWORK& _msg)
{
	const uint32_t MESSAGE_SIZE = 8 + sizeof(sREPOSITORY_ID);

	CGASSERT_ERROR(_msg.buf_message.size() > MESSAGE_SIZE);

	THROW_IF(_msg.buf_message.size() <= MESSAGE_SIZE, throwable::failure(eRESULT::INVALID_MESSAGE_LENGTH, 0, std::string_view("received Message size is invalid")), LOG_ERROR << "(err ) invalid message size [value:" << _msg.buf_message.size() << ", Requried:" << MESSAGE_SIZE << "]");

	auto buf_recv = _msg.buf_message + offset(8);

	auto& rid_remote_alloc = buf_recv.extract<sREPOSITORY_ID>();
	auto vec_rid_remote = buf_recv.extract<std::vector<sREPOSITORY_ID>>();
	 buf_recv.extract<uint64_t>();

	LOG_INFO << " > doss REPOSITORY_REMOTE['" << name() << "']: remote repository CERTIFIED!! (" << remote_endpoint().address().to_string() << ")";

	process_reset_repository(rid_remote_alloc, std::move(vec_rid_remote));

	if (rid_remote_alloc.exist())
	{

		LOG_INFO << "     initialize doss ACCOUNT_INFO[remote rid : " << rid_remote_alloc.object_id().value << "]' ";

		this->set_object_state(eOBJECT_STATE::ESTABLISHED);

		this->m_cv_complete.notify_one();
	}

	else
	{

		LOG_INFO << "     recover doss ACCOUNT_INFO : " << Irepository::m_id_repository.object_id().value;
	}

	return	1;
}

template <class TENTITY, class TCREATE>
result_code doss::repository::remote<TENTITY, TCREATE>::on_message_SERVER_DOSS_COMMAND_RESPONSE(sMESSAGE_NETWORK& _msg)
{
	auto buf_temp = _msg.buf_message + offset(8);

	auto recv_seq = buf_temp.extract<uint64_t>();
	[[maybe_unused]] auto type = buf_temp.extract<eTRANSACTION_TYPE>();
	auto cid = buf_temp.extract<sCHECKIN_ID>();
	[[maybe_unused]] auto rid = buf_temp.extract<sREQUEST_ID>();
	auto result = buf_temp.extract<eRESULT>();
	auto buf_result = buf_temp.extract<shared_buffer>();

	auto pentity = this->find_entity(cid.id_entity);

	RETURN_IF(pentity.empty(), eRESULT::DONE);

	object_ptr<Itransactable> ptransactable;

	try
	{
		SCOPED_LOCK(pentity->m_lockable_queue_transaction)
		{

			CGASSERT_ERROR(pentity->m_queue_command.empty() == false);

			THROW_IF(pentity->m_queue_command.empty(), throwable::failure(eRESULT::NOT_EXIST));

			ptransactable = pentity->m_queue_command.front();

			CGASSERT_ERROR(ptransactable->m_rid == rid);
		}

		CGASSERT_ERROR(ptransactable->m_type == type);

		CGASSERT_ERROR(ptransactable->m_cid.id_entity == ptransactable->m_pentity->m_eid);

		ptransactable->m_cid = cid;

		// - resume the transaction coroutine waiting for the response (co_request_transaction co_awaits m_response)
		//   the coroutine sets the result and completes the transaction itself.
		CGASSERT_ERROR(ptransactable->m_response.has_value());
		if (ptransactable->m_response.has_value())
			ptransactable->m_response->set_result(std::make_pair(result, buf_result));
	}
	catch (...)
	{
	}

	process_receive_ack(recv_seq);

	return eRESULT::DONE;
}

template <class TENTITY, class TCREATE>
result_code doss::repository::remote<TENTITY, TCREATE>::on_message_SERVER_DOSS_CONTROL_REQUEST(sMESSAGE_NETWORK& _msg)
{
	auto buf_temp = _msg.buf_message + offset(8);

	auto recv_seq = buf_temp.extract<uint64_t>();
	auto command = buf_temp.extract<eTRANSACTION_TYPE>();
	auto cid = buf_temp.extract<sCHECKIN_ID>();
	 buf_temp.extract<sREQUEST_ID>();
	 buf_temp.extract<eRESULT>();

	auto pentity = this->find_entity(cid.id_entity);

	RETURN_IF(pentity.empty(), eRESULT::DONE);

	switch (command)
	{
	case	eTRANSACTION_TYPE::CHECKIN:
			break;

	case	eTRANSACTION_TYPE::COMMAND:
			break;

	case	eTRANSACTION_TYPE::UPDATE:
			break;

	case	eTRANSACTION_TYPE::CHECKOUT:
			SCOPED_LOCK(this->m_lockable_map_entity)
			{

				pentity->process_remove_cid(cid);

				doss::statistics::repository::increase(this->m_statistics.m_statistics_checkout_forcible);
			}
			break;

	case	eTRANSACTION_TYPE::CREATE:
			break;

	case	eTRANSACTION_TYPE::REMOVE:
			break;

	default:
			break;
	}

	this->process_receive_ack(recv_seq);

	return eRESULT::DONE;
}

template <class TENTITY, class TCREATE>
result_code doss::repository::remote<TENTITY, TCREATE>::on_message_SERVER_DOSS_SEND_ACK(sMESSAGE_NETWORK& _msg)
{
	auto buf_temp = _msg.buf_message + offset(8);

	auto send_seq = buf_temp.extract<uint64_t>();

	this->process_send_ack(send_seq);

	return eRESULT::DONE;
}

template <class TENTITY, class TCREATE>
object_ptr<TENTITY> doss::repository::remote<TENTITY, TCREATE>::process_alloc_entity(sENTITY_ID _entity_id)
{
	THROW_IF(this->m_id_repository.empty(), throwable::failure(eRESULT::NOT_READY));

	auto pentity = make_object<TCREATE>();

	pentity->m_eid = _entity_id;

	return pentity;
}

template <class TENTITY, class TCREATE>
void doss::repository::remote<TENTITY, TCREATE>::process_reset_repository(const sREPOSITORY_ID& _id_repository_remote, std::vector<sREPOSITORY_ID>&& _vec_repository)
{
	this->m_id_repository_remote_alloc = _id_repository_remote;
	this->m_vector_id_repository_remote = std::move(_vec_repository);

	SCOPED_LOCK(this->m_lockable_map_entity)
	{
		this->m_map_entity_by_eid.clear();
	}
}


template <class TENTITY, class TCREATE>
coroutine::task<result_code> doss::repository::remote<TENTITY, TCREATE>::co_request_transaction(object_ptr<Itransactable> _ptransactable)
{
	CGASSERT_ERROR(_ptransactable->m_prepository.exist());
	CGASSERT_ERROR(_ptransactable->m_pentity.exist());
	//  - CREATE / REMOVE are requested to the repository directly (without a session)
	CGASSERT_ERROR(_ptransactable->m_psession.exist() || _ptransactable->m_type == eTRANSACTION_TYPE::CREATE || _ptransactable->m_type == eTRANSACTION_TYPE::REMOVE);

	result_code	result = eRESULT::NOT_DEFINED;

	_ptransactable->m_result = eRESULT::REQUEST;

	if (_ptransactable->m_psession.exist())
	{
		_ptransactable->m_psession->process_entity_transaction_request(_ptransactable.get());
	}

	// - the response of the depot (resolved by on_message_SERVER_DOSS_COMMAND_RESPONSE)
	_ptransactable->m_response.emplace();
	auto response = *_ptransactable->m_response;

	// - true : wait for the response of the depot / false : completed locally (checkout, or a failure before sending)
	bool is_waiting = false;

	try
	{
		THROW_IF(this->get_object_state() != eOBJECT_STATE::ESTABLISHED, throwable::failure(eRESULT::NOT_READY));

		THROW_IF(this->m_id_repository.empty(), throwable::failure(eRESULT::NOT_READY));

		switch (_ptransactable->m_type)
		{
		case	eTRANSACTION_TYPE::CHECKIN:
				{
					doss::statistics::repository::increase(this->Irepository::m_statistics.m_statistics_checkin_request);

					THROW_IF(_ptransactable->m_cid.id_entity.empty(), throwable::failure(eRESULT::ALREADY_DONE));

					auto is_sent = this->process_send_restoreable(_ptransactable.get());

					THROW_IF(is_sent == false, throwable::failure(eRESULT::FAIL));

					is_waiting = true;
				}
				break;

		case	eTRANSACTION_TYPE::COMMAND:
				{
					doss::statistics::repository::increase(this->Irepository::m_statistics.m_statistics_command_request);

					THROW_IF(_ptransactable->m_cid.empty(), throwable::failure(eRESULT::NOT_READY));

					auto is_sent = this->process_send_restoreable(_ptransactable.get());

					THROW_IF(is_sent == false, throwable::failure(eRESULT::FAIL));

					is_waiting = true;
				}
				break;

		case	eTRANSACTION_TYPE::UPDATE:
				{
					doss::statistics::repository::increase(this->Irepository::m_statistics.m_statistics_update_request);

					THROW_IF(_ptransactable->m_cid.empty(), throwable::failure(eRESULT::NOT_READY));

					auto is_sent = this->process_send_restoreable(_ptransactable.get());

					THROW_IF(is_sent == false, throwable::failure(eRESULT::FAIL));

					is_waiting = true;
				}
				break;

		case	eTRANSACTION_TYPE::CHECKOUT:
				{
					doss::statistics::repository::increase(this->Irepository::m_statistics.m_statistics_checkout_request);

					auto* pentity = _ptransactable->m_pentity.get();

					CGASSERT_FATAL(pentity != nullptr);

					THROW_IF(pentity == nullptr, throwable::failure(eRESULT::NOT_EXIST));

					SCOPED_LOCK(this->m_lockable_map_entity)
					{
						auto is_success = pentity->process_remove_cid(_ptransactable->m_psession.get());

						THROW_IF(is_success == false, throwable::failure(eRESULT::NOT_EXIST));
					}

					THROW_IF(_ptransactable->m_cid.empty(), throwable::failure(eRESULT::NOT_READY));

					CGASSERT_ERROR(_ptransactable->m_cid.id_entity == _ptransactable->m_pentity->m_eid);

					// - checkout completes locally (the response is not waited for)
					result = eRESULT::SUCCESS;
					_ptransactable->set_result(result);

					this->process_send_restoreable(_ptransactable.get());
				}
				break;

		case	eTRANSACTION_TYPE::CREATE:
				{
					doss::statistics::repository::increase(this->Irepository::m_statistics.m_statistics_create_request);

					auto is_sent = this->process_send_restoreable(_ptransactable.get());

					THROW_IF(is_sent == false, throwable::failure(eRESULT::FAIL));

					is_waiting = true;
				}
				break;

		case	eTRANSACTION_TYPE::REMOVE:
				{
					doss::statistics::repository::increase(this->Irepository::m_statistics.m_statistics_remove_request);

					// - the depot finds the entity by the entity id of the checkin id
					if (_ptransactable->m_cid.id_entity.empty())
						_ptransactable->m_cid.id_entity = _ptransactable->m_pentity->m_eid;

					auto is_sent = this->process_send_restoreable(_ptransactable.get());

					THROW_IF(is_sent == false, throwable::failure(eRESULT::FAIL));

					is_waiting = true;
				}
				break;

		default:
				CGASSERT_ERROR(false);
				throw throwable::failure(eRESULT::INVALID_ARGUMENT);
				break;
		}
	}
	catch (throwable::failure & _e)
	{
		result = _e.reason();
		_ptransactable->_set_result_if_not_set(result);
		co_return result;
	}
	catch (...)
	{
		result = eRESULT::EXCEPTION;
		_ptransactable->_set_result_if_not_set(result);
		co_return result;
	}

	// - checkout (and the like) finished locally
	if (is_waiting == false)
		co_return _ptransactable->m_result;

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
	// - wait for the response of the depot ({result, result buffer})
	auto [result_response, buf_response] = co_await response;

	// - set the result on the transactable (the entity completes it)
	if (buf_response.empty())
		_ptransactable->_set_result_if_not_set(result_response);
	else
		_ptransactable->set_result(result_response, buf_response);

	co_return result_response;
}
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

template <class TENTITY, class TCREATE>
void doss::repository::remote<TENTITY, TCREATE>::process_fail_transaction(Itransactable* _ptransactable) noexcept
{
	switch (_ptransactable->m_type)
	{
	case	eTRANSACTION_TYPE::CHECKIN:
			this->process_send_restoreable_COMMAND_FAIL(_ptransactable);
			break;

	case	eTRANSACTION_TYPE::COMMAND:
			break;

	case	eTRANSACTION_TYPE::UPDATE:
			break;

	case	eTRANSACTION_TYPE::CHECKOUT:
			break;

	case	eTRANSACTION_TYPE::CREATE:
			break;

	case	eTRANSACTION_TYPE::REMOVE:
			break;

	default:
			CGASSERT_ERROR(false);
	}
}

template <class TENTITY, class TCREATE>
bool doss::repository::remote<TENTITY, TCREATE>::process_send_restoreable(Itransactable* _ptransactable)
{
	{
		std::lock_guard cs(this->m_lockable_send_sequence);

		uint64_t send_sequence = ++this->m_send_sequence;

		return this->send(message_SERVER_DOSS_COMMAND_REQUEST(send_sequence, _ptransactable));
	}
}

template <class TENTITY, class TCREATE>
bool doss::repository::remote<TENTITY, TCREATE>::process_send_restoreable_COMMAND_FAIL(Itransactable* _ptransactable)
{
	{
		std::lock_guard cs(this->m_lockable_send_sequence);

		uint64_t send_sequence = ++this->m_send_sequence;

		return this->send(message_SERVER_DOSS_COMMAND_FAIL(send_sequence, _ptransactable));
	}
}

template <class TENTITY, class TCREATE>
void doss::repository::remote<TENTITY, TCREATE>::process_send_ack(uint64_t _send_sequence) noexcept
{
	{
		std::lock_guard cs(this->m_lockable_send_sequence);

		CGASSERT_ERROR(this->m_send_sequence >= _send_sequence);

		CGASSERT_ERROR(this->m_send_sequence_ack < _send_sequence);

		this->m_send_sequence_ack = _send_sequence;

	}
}

template <class TENTITY, class TCREATE>
void doss::repository::remote<TENTITY, TCREATE>::process_receive_ack(uint64_t _receive_sequence) noexcept
{
	CGASSERT_ERROR(this->m_receive_sequence < _receive_sequence);

	this->m_receive_sequence.store(_receive_sequence);

	this->send(message_SERVER_DOSS_SEND_ACK(_receive_sequence));
}

}
