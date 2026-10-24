//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                        network socket kafka classes                       *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
//*  Best for Game Developement and Optimized for Game Developement.          *
//*                                                                           *
//*                (c) 2003. Cho Sanghyun. All right reserved.                *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************

#pragma once

namespace CGDK
{
//-----------------------------------------------------------------------------
//
// net::io::kafka::Nproducer
//
// 1. net::io::kafka::Nproducer
//    1) kafka producer. (librdkafka RdKafka::Producer)
//    2) produce() requests to produce a message and returns at once. the
//       delivery is reported by on_delivery() on the poll thread.
//    3) stop() waits for the remained messages until 'flush_timeout'.
//
// 2. context
//    (common: see net::io::kafka::Nclient)
//    1) "topic"                the default topic of produce()
//    2) "flush_timeout"        milliseconds (default: 10000)
//    3) "block_on_queue_full"  "true": produce() blocks while the local queue is full
//                              "false": produce() fails (default)
//
//    ex) the name of the object is "foo"
//
//        context context_setting;
//        context_setting["foo"]["bootstrap_servers"]      = "192.168.0.100:9092";
//        context_setting["foo"]["topic"]                  = "test_topic";
//        context_setting["foo"]["properties"]["linger.ms"] = 5;
//        context_setting["foo"]["properties"]["acks"]     = "all";
//
//        pproducer->name("foo");
//        pproducer->start(context_setting);
//
//    or
//        pproducer->start({ "192.168.0.100:9092", "test_topic" });	// sKAFKA_PRODUCER_PARAMETER
//
//-----------------------------------------------------------------------------
class net::io::kafka::Nproducer :
	public						net::io::kafka::Nclient,
	virtual public				net::io::kafka::Iproducer
{
public:
			using START_PARAMETER = sKAFKA_PRODUCER_PARAMETER;

protected:
			Nproducer() noexcept;
	virtual	~Nproducer() noexcept;

public:
	// 1) start
			result_code			start() { return this->object::Nstartable::start(); }
			result_code			start(const START_PARAMETER& _parameter) { context context_parameter; context_parameter = &_parameter; return this->start(context_parameter); }
			result_code			start(const context& _context) { context context_copy(_context); return this->start(context_copy); }
	virtual	result_code			start(context& _context) override { return this->object::Ninitializable_startable::start(_context); }

	// 2) flush (waits until all messages are delivered)
			result_code			flush(chrono::tick::duration _timeout = std::chrono::seconds(10));

	// 3) informations
	[[nodiscard]] const std::string& default_topic() const noexcept { return this->m_default_topic; }
	[[nodiscard]] std::size_t	pending_count() const noexcept;

	// 4) native handle
	[[nodiscard]] std::shared_ptr<RdKafka::Producer> native_handle() const noexcept { std::shared_lock lock(this->m_lockable_handle); return this->m_phandle; }

protected:
	//! @brief called when the delivery of a message is reported. (on the poll thread)
	virtual	void				on_delivery(sKAFKA_DELIVERY& /*_delivery*/) {}

public:
	[[nodiscard]] virtual std::string_view process_get_type_name() const noexcept override { return "KAFKA_PRODUCER"; }
	virtual	void				process_open(context& _context) override;
	virtual	void				process_close_handle() noexcept override;
	virtual	void				process_poll(std::chrono::milliseconds _timeout) override;
	virtual	bool				process_produce(std::string_view _topic, int32_t _partition, buffer_view _buf_key, buffer_view _buf_message, object_ptr<Ireferenceable>&& _powner) override;
	virtual	void				process_complete_produce(sKAFKA_DELIVERY& _delivery) override;

protected:
			class delivery_report_callback : public RdKafka::DeliveryReportCb
			{
			public:
				explicit delivery_report_callback(Nproducer* _pproducer) noexcept : m_pproducer(_pproducer) {}
				virtual void	dr_cb(RdKafka::Message& _message) override;
			private:
				Nproducer*		m_pproducer;
			};

	mutable	std::shared_mutex	m_lockable_handle;
			std::shared_ptr<RdKafka::Producer> m_phandle;
			delivery_report_callback m_delivery_report_callback{ this };
			std::string			m_default_topic;
			chrono::tick::duration m_flush_timeout{ std::chrono::seconds{10} };
			bool				m_block_on_queue_full{ false };
};


}
