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
// net::io::kafka::Nconsumer
//
// 1. net::io::kafka::Nconsumer
//    1) kafka consumer. (librdkafka RdKafka::KafkaConsumer, consumer group)
//    2) the consumed messages are passed to on_message() on the poll thread
//       in order. (sMESSAGE_KAFKA)
//    3) the offsets are committed automatically by default.
//       ("enable.auto.commit" of librdkafka. set it "false" in "properties" and
//       call commit() to commit manually)
//
// 2. context
//    (common: see net::io::kafka::Nclient)
//    1) "group_id" (or "group.id")     consumer group (required)
//    2) "topics" (or "topic")          topics to subscribe ("topic_a,topic_b" or ["topic_a", "topic_b"])
//    3) "auto_offset_reset" (or "auto.offset.reset")  "earliest", "latest"
//
//    ex) the name of the object is "foo"
//
//        context context_setting;
//        context_setting["foo"]["bootstrap_servers"] = "192.168.0.100:9092";
//        context_setting["foo"]["group_id"]          = "test_group";
//        context_setting["foo"]["topics"]            = "test_topic";
//        context_setting["foo"]["auto_offset_reset"] = "earliest";
//
//        pconsumer->name("foo");
//        pconsumer->start(context_setting);
//
//    or
//        pconsumer->start({ "192.168.0.100:9092", "test_group", { "test_topic" } });	// sKAFKA_CONSUMER_PARAMETER
//
//-----------------------------------------------------------------------------
class net::io::kafka::Nconsumer :
	public						net::io::kafka::Nclient,
	virtual public				net::io::kafka::Iconsumable,
	virtual public				net::io::kafka::Imessageable
{
public:
			using START_PARAMETER = sKAFKA_CONSUMER_PARAMETER;

protected:
			Nconsumer() noexcept;
	virtual	~Nconsumer() noexcept;

public:
	// 1) start
			result_code			start() { return this->object::Nstartable::start(); }
			result_code			start(const START_PARAMETER& _parameter) { context context_parameter; context_parameter = &_parameter; return this->start(context_parameter); }
			result_code			start(const context& _context) { context context_copy(_context); return this->start(context_copy); }
	virtual	result_code			start(context& _context) override { return this->object::Ninitializable_startable::start(_context); }

	// 2) subscription
			result_code			subscribe(const std::vector<std::string>& _topics);
			result_code			unsubscribe();
	[[nodiscard]] std::vector<std::string> subscription() const;

	// 3) commit (when "enable.auto.commit" is "false")
			result_code			commit(const sMESSAGE_KAFKA& _msg, bool _async = true);
			result_code			commit(bool _async = true);

	// 4) informations
	[[nodiscard]] const std::string& group_id() const noexcept { return this->m_group_id; }

	// 5) native handle
	[[nodiscard]] std::shared_ptr<RdKafka::KafkaConsumer> native_handle() const noexcept { std::shared_lock lock(this->m_lockable_handle); return this->m_phandle; }

public:
	[[nodiscard]] virtual std::string_view process_get_type_name() const noexcept override { return "KAFKA_CONSUMER"; }
	virtual	void				process_open(context& _context) override;
	virtual	void				process_close_handle() noexcept override;
	virtual	void				process_poll(std::chrono::milliseconds _timeout) override;
	virtual	void				process_consume(std::unique_ptr<RdKafka::Message>&& _pmessage) override;

protected:
			class memory_message;

	mutable	std::shared_mutex	m_lockable_handle;
			std::shared_ptr<RdKafka::KafkaConsumer> m_phandle;
			std::string			m_group_id;
};


}
