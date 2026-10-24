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
// net::io::kafka::Nclient (cf. net::io::connect_requestable::Ntcp)
//
// 1. net::io::kafka::Nclient
//    1) the base of the kafka producer and consumer.
//    2) start(context) creates the native handle (librdkafka) with the
//       parameters in the context and begins the poll thread.
//       stop() ends the poll thread and closes the native handle.
//    3) the callbacks (on_message(), on_delivery(), on_error()) are called on
//       the poll thread.
//    4) the poll thread holds a reference of this object while it is running.
//       (call stop() to release it)
//
// 2. start process
//    start(context)
//      -> process_start()
//         1) "progress_log"
//         2) _process_starting()  -> on_starting()
//         3) process_open()       creates the native handle (parameters from the context)
//         4) begins the poll thread
//         5) _process_starting_child() -> on_start()
//
//    stop()
//      -> process_stop()
//         1) _process_stopping()  -> on_stopping()
//         2) ends the poll thread
//         3) process_close_handle()
//         4) _process_stopping_child() -> on_stop()
//
// 3. context (common)
//    1) "bootstrap_servers" (or "bootstrap.servers", "brokers", "servers", "server")
//       - "host1:port1,host2:port2"  (default: "localhost:9092")
//    2) "client_id" (or "client.id")
//    3) "properties"
//       - any librdkafka property. (ex: "linger.ms", "acks", "security.protocol")
//    4) "poll_interval"  (milliseconds. default: 100)
//    5) "progress_log"   "true"/"false"
//
//-----------------------------------------------------------------------------
class net::io::kafka::Nclient :
	public						object::Nnameable,
	public						object::Ninitializable_startable
{
protected:
			Nclient() noexcept;
	virtual	~Nclient() noexcept;

public:
	// 1) progress log
			void				enable_progress_log(bool _flag_enable = true) noexcept { this->m_enable_progress_log = _flag_enable; }
			void				disable_progress_log() noexcept { this->m_enable_progress_log = false; }
	[[nodiscard]] bool			progress_log() const noexcept { return this->m_enable_progress_log; }

	// 2) parameters (set on start)
	[[nodiscard]] const std::string& bootstrap_servers() const noexcept { return this->m_bootstrap_servers; }
	[[nodiscard]] auto			poll_interval() const noexcept { return this->m_poll_interval; }

	// 3) poll thread
	[[nodiscard]] bool			is_poll_thread() const noexcept { return this->m_id_thread_poll.load() == std::this_thread::get_id(); }

protected:
	//! @brief called when an error is reported by librdkafka. (on the poll thread)
	virtual	void				on_error(int /*_error_code*/, std::string_view /*_reason*/) {}

public:
	virtual	result_code			process_start(context& _context) override;
	virtual	result_code			process_stop() noexcept override;

	//! @brief 'KAFKA_PRODUCER', 'KAFKA_CONSUMER' (for log)
	[[nodiscard]] virtual std::string_view process_get_type_name() const noexcept PURE;
	//! @brief creates the native handle with the parameters in the context
	virtual	void				process_open(context& _context) PURE;
	//! @brief closes the native handle
	virtual	void				process_close_handle() noexcept PURE;
	//! @brief polls once on the poll thread (blocks at most '_timeout')
	virtual	void				process_poll(std::chrono::milliseconds _timeout) PURE;
	//! @brief the event of librdkafka (error, log, ...)
	virtual	void				process_event(RdKafka::Event& _event);

protected:
	[[nodiscard]] std::unique_ptr<RdKafka::Conf> process_create_configure(const sKAFKA_CLIENT_PARAMETER& _parameter);
			void				process_set_configure(RdKafka::Conf* _pconfigure, const std::string& _name, const std::string& _value, bool _enable_progress_log);
			void				_begin_poll_thread();
			bool				_end_poll_thread() noexcept;
			void				_process_poll_thread(std::stop_token _stop_token);

protected:
			class event_callback : public RdKafka::EventCb
			{
			public:
				explicit event_callback(Nclient* _pclient) noexcept : m_pclient(_pclient) {}
				virtual void	event_cb(RdKafka::Event& _event) override { this->m_pclient->process_event(_event); }
			private:
				Nclient*		m_pclient;
			};

			event_callback		m_event_callback{ this };
			object_ptr<api::thread> m_pthread_poll;
			std::atomic<std::thread::id> m_id_thread_poll{};
			std::atomic<bool>	m_is_close_deferred{ false };
			std::string			m_bootstrap_servers;
			chrono::tick::duration m_poll_interval{ std::chrono::milliseconds{100} };
			bool				m_enable_progress_log{ true };
};


}
