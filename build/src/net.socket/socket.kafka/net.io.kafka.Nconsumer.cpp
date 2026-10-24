#include "pch.h"

using namespace CGDK;


//-----------------------------------------------------------------------------
// memory_message
//
//  - the source memory of 'buf_message' of sMESSAGE_KAFKA (not copied)
//  - the consumed message is kept alive while 'buf_message' is referenced
//  - holds the native handle too. (librdkafka destroys the handle only after
//    all its messages are destroyed : stop() does not wait for the messages
//    kept by the application. the handle is destroyed with the last message)
//-----------------------------------------------------------------------------
class net::io::kafka::Nconsumer::memory_message : public Imemory
{
public:
			memory_message(std::shared_ptr<RdKafka::KafkaConsumer> _phandle, std::unique_ptr<RdKafka::Message>&& _pmessage) :
				Imemory(static_cast<char*>(_pmessage->payload()), _pmessage->len()),
				m_topic(_pmessage->topic_name()),
				m_phandle(std::move(_phandle)),
				m_pmessage(std::move(_pmessage))
			{
			}
	virtual	~memory_message() noexcept {}

public:
			std::string			m_topic;
			std::shared_ptr<RdKafka::KafkaConsumer> m_phandle;	// destroyed after 'm_pmessage'
			std::unique_ptr<RdKafka::Message> m_pmessage;
};


net::io::kafka::Nconsumer::Nconsumer() noexcept
{
}

net::io::kafka::Nconsumer::~Nconsumer() noexcept
{
	// - closes the handle if it is not closed
	this->Nconsumer::process_close_handle();
}

result_code net::io::kafka::Nconsumer::subscribe(const std::vector<std::string>& _topics)
{
	// 1) get handle
	auto phandle = this->native_handle();

	// check)
	RETURN_IF(!phandle, result_code(eRESULT::NOT_READY));

	// 2) subscribe
	auto error = phandle->subscribe(_topics);

	// check)
	if (error != RdKafka::ERR_NO_ERROR)
	{
		// log)
		LOG_ERROR << "(err ) fail to subscribe kafka topics ("sv << RdKafka::err2str(error) << ") "sv << this->process_get_type_name() << "['"sv << this->name() << "']"sv;

		// return)
		return result_code(eRESULT::FAIL);
	}

	// return)
	return result_code(eRESULT::SUCCESS);
}

result_code net::io::kafka::Nconsumer::unsubscribe()
{
	// 1) get handle
	auto phandle = this->native_handle();

	// check)
	RETURN_IF(!phandle, result_code(eRESULT::NOT_READY));

	// 2) unsubscribe
	auto error = phandle->unsubscribe();

	// return)
	return (error == RdKafka::ERR_NO_ERROR) ? result_code(eRESULT::SUCCESS) : result_code(eRESULT::FAIL);
}

std::vector<std::string> net::io::kafka::Nconsumer::subscription() const
{
	// declare)
	std::vector<std::string> topics;

	// 1) get handle
	auto phandle = this->native_handle();

	// 2) get subscription
	if (phandle)
		phandle->subscription(topics);

	// return)
	return topics;
}

result_code net::io::kafka::Nconsumer::commit(const sMESSAGE_KAFKA& _msg, bool _async)
{
	// check)
	RETURN_IF(_msg.pnative == nullptr, result_code(eRESULT::INVALID_ARGUMENT));

	// 1) get handle
	auto phandle = this->native_handle();

	// check)
	RETURN_IF(!phandle, result_code(eRESULT::NOT_READY));

	// 2) commit
	auto error = _async ? phandle->commitAsync(_msg.pnative) : phandle->commitSync(_msg.pnative);

	// return)
	return (error == RdKafka::ERR_NO_ERROR) ? result_code(eRESULT::SUCCESS) : result_code(eRESULT::FAIL);
}

result_code net::io::kafka::Nconsumer::commit(bool _async)
{
	// 1) get handle
	auto phandle = this->native_handle();

	// check)
	RETURN_IF(!phandle, result_code(eRESULT::NOT_READY));

	// 2) commit
	auto error = _async ? phandle->commitAsync() : phandle->commitSync();

	// return)
	return (error == RdKafka::ERR_NO_ERROR) ? result_code(eRESULT::SUCCESS) : result_code(eRESULT::FAIL);
}

void net::io::kafka::Nconsumer::process_open(context& _context)
{
	// declare)
	START_PARAMETER parameter;

	// 1) get parameter
	if (_context.type() == context::value_type_t::ptr_)
	{
		// - start(START_PARAMETER)
		parameter = *static_cast<const START_PARAMETER*>(_context.v.value_ptr);
		this->m_enable_progress_log = parameter.enable_progress_log;
	}
	else
	{
		// - from the context
		parameter = get_parameter_kafka_consumer(_context, this->m_enable_progress_log);
	}

	// check) "group.id" is required
	THROW_IF(parameter.group_id.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "kafka consumer 'group_id' is empty"sv), LOG_ERROR << "  @ (err ) kafka consumer 'group_id' is empty '"sv << this->name() << "'"sv);

	// 2) create configure
	auto pconfigure = this->process_create_configure(parameter);

	// 3) "group.id", "auto.offset.reset"
	this->process_set_configure(pconfigure.get(), "group.id"s, parameter.group_id, false);

	if (parameter.auto_offset_reset.empty() == false)
		this->process_set_configure(pconfigure.get(), "auto.offset.reset"s, parameter.auto_offset_reset, false);

	// 4) create consumer
	std::string errstr;
	std::shared_ptr<RdKafka::KafkaConsumer> phandle(RdKafka::KafkaConsumer::create(pconfigure.get(), errstr));

	// check)
	THROW_IF(!phandle, throwable::failure(eRESULT::FAIL_TO_CREATE, 0, CGDK::stream_string("fail to create kafka consumer (", errstr, ")")), LOG_ERROR << "  @ (err ) fail to create kafka consumer ("sv << errstr << ") '"sv << this->name() << "'"sv);

	// 5) subscribe
	if (parameter.topics.empty() == false)
	{
		auto error = phandle->subscribe(parameter.topics);

		// check)
		if (error != RdKafka::ERR_NO_ERROR)
		{
			// - close
			phandle->close();

			// log)
			LOG_ERROR << "  @ (err ) fail to subscribe kafka topics ("sv << RdKafka::err2str(error) << ") '"sv << this->name() << "'"sv;

			// raise)
			throw throwable::failure(eRESULT::FAIL, 0, CGDK::stream_string("fail to subscribe kafka topics (", RdKafka::err2str(error), ")"));
		}
	}

	// 6) set parameters
	this->m_group_id = parameter.group_id;

	// 7) set handle
	{
		std::unique_lock lock(this->m_lockable_handle);

		this->m_phandle = std::move(phandle);
	}

	// log)
	EXECUTE_IF(this->m_enable_progress_log, LOG_INFO << "  + kafka consumer created (bootstrap.servers: '"sv << this->m_bootstrap_servers << "', group.id: '"sv << this->m_group_id << "', topics: "sv << parameter.topics.size() << ")"sv);
}

void net::io::kafka::Nconsumer::process_close_handle() noexcept
{
	// declare)
	std::shared_ptr<RdKafka::KafkaConsumer> phandle;

	// 1) get handle
	{
		std::unique_lock lock(this->m_lockable_handle);

		phandle = std::move(this->m_phandle);
	}

	// check)
	RETURN_IF(!phandle);

	// 2) close (leaves the consumer group and commits the offsets)
	try
	{
		phandle->close();
	}
	catch (...)
	{
	}

	// 3) destroy
	phandle.reset();
}

void net::io::kafka::Nconsumer::process_poll(std::chrono::milliseconds _timeout)
{
	// 1) get handle
	auto phandle = this->native_handle();

	// check)
	if (!phandle)
	{
		std::this_thread::sleep_for(_timeout);
		return;
	}

	// 2) consume
	std::unique_ptr<RdKafka::Message> pmessage(phandle->consume(static_cast<int>(_timeout.count())));

	// check)
	RETURN_IF(!pmessage);

	switch (pmessage->err())
	{
	// case) message
	case	RdKafka::ERR_NO_ERROR:
			this->process_consume(std::move(pmessage));
			break;

	// case) no message
	case	RdKafka::ERR__TIMED_OUT:
	case	RdKafka::ERR__PARTITION_EOF:
			break;

	// case) error
	default:
			{
				// log)
				LOG_ERROR << "(err ) kafka consume error ("sv << pmessage->errstr() << ") "sv << this->process_get_type_name() << "['"sv << this->name() << "']"sv;

				// - on_error
				this->on_error(static_cast<int>(pmessage->err()), pmessage->errstr());
			}
			break;
	}
}

void net::io::kafka::Nconsumer::process_consume(std::unique_ptr<RdKafka::Message>&& _pmessage)
{
	// declare)
	auto len = _pmessage->len();
	auto partition = _pmessage->partition();
	auto offset = _pmessage->offset();
	auto timestamp = _pmessage->timestamp();
	auto buf_key = buffer_view(const_cast<void*>(_pmessage->key_pointer()), _pmessage->key_len());

	// 1) wrap the message as the source memory of the buffer (not copied)
	auto pmemory = make_object<memory_message>(this->native_handle(), std::move(_pmessage));

	// 2) buffer
	shared_buffer buf_message;
	buf_message = object_ptr<Imemory>(pmemory.get());
	buf_message ^= len;

	// 3) message
	sMESSAGE_KAFKA msg(this, buf_message);
	msg.topic = pmemory->m_topic;
	msg.partition = partition;
	msg.offset = offset;
	msg.timestamp = (timestamp.type != RdKafka::MessageTimestamp::MSG_TIMESTAMP_NOT_AVAILABLE) ? timestamp.timestamp : -1;
	msg.buf_key = buf_key;
	msg.pnative = pmemory->m_pmessage.get();

	// 4) process message (on_message)
	try
	{
		this->process_message(msg);
	}
	catch (std::exception& _e)
	{
		// log)
		LOG_ERROR << "(excp) exception on 'on_message' "sv << this->process_get_type_name() << "['"sv << this->name() << "'] ("sv << std::string_view(_e.what()) << ")"sv;

		// - call 'exception handler'
		throwable::execute_exception_handler(this, _e, __FUNCTION__);
	}
	catch (...)
	{
		// log)
		LOG_ERROR << "(excp) exception on 'on_message' "sv << this->process_get_type_name() << "['"sv << this->name() << "']"sv;
	}
}
