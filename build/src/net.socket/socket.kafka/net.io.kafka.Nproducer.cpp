#include "pch.h"

using namespace CGDK;


//-----------------------------------------------------------------------------
// the opaque of a produced message (holds '_powner' until the delivery is reported)
//-----------------------------------------------------------------------------
struct sKAFKA_PRODUCE_OWNER
{
	object_ptr<Ireferenceable>	powner;
};


net::io::kafka::Nproducer::Nproducer() noexcept
{
}

net::io::kafka::Nproducer::~Nproducer() noexcept
{
	// - closes the handle if it is not closed
	this->Nproducer::process_close_handle();
}

result_code net::io::kafka::Nproducer::flush(chrono::tick::duration _timeout)
{
	// 1) get handle
	auto phandle = this->native_handle();

	// check)
	RETURN_IF(!phandle, result_code(eRESULT::NOT_READY));

	// 2) flush
	auto error = phandle->flush(static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(_timeout).count()));

	// return)
	return (error == RdKafka::ERR_NO_ERROR) ? result_code(eRESULT::SUCCESS) : result_code(eRESULT::TIME_OUT);
}

std::size_t net::io::kafka::Nproducer::pending_count() const noexcept
{
	// 1) get handle
	auto phandle = this->native_handle();

	// check)
	RETURN_IF(!phandle, 0);

	// return)
	return static_cast<std::size_t>(phandle->outq_len());
}

void net::io::kafka::Nproducer::process_open(context& _context)
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
		parameter = get_parameter_kafka_producer(_context, this->m_enable_progress_log);
	}

	// 2) create configure
	auto pconfigure = this->process_create_configure(parameter);

	// 3) delivery report callback
	{
		std::string errstr;
		auto result = pconfigure->set("dr_cb", &this->m_delivery_report_callback, errstr);

		// check)
		THROW_IF(result != RdKafka::Conf::CONF_OK, throwable::failure(eRESULT::FAIL, 0, CGDK::stream_string("fail to set kafka delivery report callback (", errstr, ")")));
	}

	// 4) create producer
	std::string errstr;
	std::shared_ptr<RdKafka::Producer> phandle(RdKafka::Producer::create(pconfigure.get(), errstr));

	// check)
	THROW_IF(!phandle, throwable::failure(eRESULT::FAIL_TO_CREATE, 0, CGDK::stream_string("fail to create kafka producer (", errstr, ")")), LOG_ERROR << "  @ (err ) fail to create kafka producer ("sv << errstr << ") '"sv << this->name() << "'"sv);

	// 5) set parameters
	this->m_default_topic = parameter.topic;
	this->m_flush_timeout = parameter.flush_timeout;
	this->m_block_on_queue_full = parameter.block_on_queue_full;

	// 6) set handle
	{
		std::unique_lock lock(this->m_lockable_handle);

		this->m_phandle = std::move(phandle);
	}

	// log)
	EXECUTE_IF(this->m_enable_progress_log, LOG_INFO << "  + kafka producer created (bootstrap.servers: '"sv << this->m_bootstrap_servers << "', topic: '"sv << this->m_default_topic << "')"sv);
}

void net::io::kafka::Nproducer::process_close_handle() noexcept
{
	// declare)
	std::shared_ptr<RdKafka::Producer> phandle;

	// 1) get handle
	{
		std::unique_lock lock(this->m_lockable_handle);

		phandle = std::move(this->m_phandle);
	}

	// check)
	RETURN_IF(!phandle);

	// 2) waits for the remained messages
	auto error = phandle->flush(static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(this->m_flush_timeout).count()));

	// 3) purges the messages not delivered (their delivery reports are called with ERR__PURGE_QUEUE/ERR__PURGE_INFLIGHT)
	if (error != RdKafka::ERR_NO_ERROR)
	{
		// log)
		LOG_ERROR << "(err ) kafka producer['"sv << this->name() << "'] "sv << phandle->outq_len() << " message(s) are not delivered ("sv << RdKafka::err2str(error) << ")"sv;

		// - purge
		phandle->purge(RdKafka::Producer::PURGE_QUEUE | RdKafka::Producer::PURGE_INFLIGHT);

		// - serves the delivery reports
		phandle->poll(0);
	}

	// 4) destroy
	phandle.reset();
}

void net::io::kafka::Nproducer::process_poll(std::chrono::milliseconds _timeout)
{
	// 1) get handle
	auto phandle = this->native_handle();

	// check)
	if (!phandle)
	{
		std::this_thread::sleep_for(_timeout);
		return;
	}

	// 2) serves the delivery reports and the events
	phandle->poll(static_cast<int>(_timeout.count()));
}

bool net::io::kafka::Nproducer::process_produce(std::string_view _topic, int32_t _partition, buffer_view _buf_key, buffer_view _buf_message, object_ptr<Ireferenceable>&& _powner)
{
	// lock) the handle is not closed while producing
	std::shared_lock lock(this->m_lockable_handle);

	// check) not started
	RETURN_IF(!this->m_phandle, false);

	// 1) topic (empty: the default topic)
	std::string topic = _topic.empty() ? this->m_default_topic : std::string(_topic);

	// check)
	THROW_IF(topic.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "kafka topic is empty (no default topic)"sv));

	// 2) flags
	int flags = 0;

	// - copy if no owner
	if (_powner.empty())
		flags |= RdKafka::Producer::RK_MSG_COPY;

	// - block if the queue is full
	if (this->m_block_on_queue_full)
		flags |= RdKafka::Producer::RK_MSG_BLOCK;

	// 3) owner (released when the delivery is reported)
	sKAFKA_PRODUCE_OWNER* popaque = _powner.exist() ? new sKAFKA_PRODUCE_OWNER{ std::move(_powner) } : nullptr;

	// 4) produce
	auto error = this->m_phandle->produce(
		topic,
		_partition,
		flags,
		const_cast<char*>(_buf_message.data<char>()), _buf_message.size(),
		_buf_key.data<char>(), _buf_key.size(),
		0,
		popaque);

	// check)
	if (error != RdKafka::ERR_NO_ERROR)
	{
		// - release the owner
		delete popaque;

		// log)
		LOG_ERROR << "(err ) fail to produce kafka message (topic: '"sv << topic << "', "sv << RdKafka::err2str(error) << ") "sv << this->process_get_type_name() << "['"sv << this->name() << "']"sv;

		// return)
		return false;
	}

	// return)
	return true;
}

void net::io::kafka::Nproducer::process_complete_produce(sKAFKA_DELIVERY& _delivery)
{
	// log)
	EXECUTE_IF(_delivery.result != eRESULT::SUCCESS, LOG_ERROR << "(err ) fail to deliver kafka message (topic: '"sv << _delivery.topic << "', "sv << _delivery.reason << ") "sv << this->process_get_type_name() << "['"sv << this->name() << "']"sv);

	// 1) on_delivery
	try
	{
		this->on_delivery(_delivery);
	}
	catch (std::exception& _e)
	{
		LOG_ERROR << "(excp) exception on 'on_delivery' "sv << this->process_get_type_name() << "['"sv << this->name() << "'] ("sv << std::string_view(_e.what()) << ")"sv;
	}
	catch (...)
	{
		LOG_ERROR << "(excp) exception on 'on_delivery' "sv << this->process_get_type_name() << "['"sv << this->name() << "']"sv;
	}
}

void net::io::kafka::Nproducer::delivery_report_callback::dr_cb(RdKafka::Message& _message)
{
	// 1) take the owner
	std::unique_ptr<sKAFKA_PRODUCE_OWNER> powner(static_cast<sKAFKA_PRODUCE_OWNER*>(_message.msg_opaque()));

	// declare)
	auto topic = _message.topic_name();
	auto reason = _message.errstr();

	// 2) delivery
	sKAFKA_DELIVERY delivery;
	delivery.result = (_message.err() == RdKafka::ERR_NO_ERROR) ? result_code(eRESULT::SUCCESS) : result_code(eRESULT::FAIL);
	delivery.error_code = static_cast<int>(_message.err());
	delivery.reason = reason;
	delivery.topic = topic;
	delivery.partition = _message.partition();
	delivery.offset = _message.offset();
	delivery.buf_key = buffer_view(const_cast<void*>(_message.key_pointer()), _message.key_len());
	delivery.buf_message = buffer_view(_message.payload(), _message.len());

	// 3) process
	this->m_pproducer->process_complete_produce(delivery);
}
