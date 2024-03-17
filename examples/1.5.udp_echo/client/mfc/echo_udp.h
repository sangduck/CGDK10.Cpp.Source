#pragma once

class socket_udp : public net::socket::udp
{
public:
	virtual result_code			on_message(sMESSAGE_NETWORK& _msg) override;
			int					m_idx_list = -1;
			std::vector<net::sockaddr> m_vector_socket_destination;
			std::atomic<uint64_t> m_statistics_send{ 0 };
			std::atomic<uint64_t> m_statistics_recv{ 0 };
			chrono::time::utc::time_point m_statistics_time;

			std::tuple<float, float, bool> statistics_update(chrono::time::utc::time_point _now) 
			{
				auto gap = _now - this->m_statistics_time;
				auto mgap = std::chrono::duration_cast<std::chrono::seconds>(gap).count();

				// check)
				RETURN_IF(mgap == 0, { 0.0f, 0.0f, false });

				auto now_sended = this->statistics_get_sended_message();
				auto now_received = this->statistics_get_received_message();

				auto f_mps_sended = (mgap != 0) ? static_cast<float>(now_sended - this->m_statistics_send) / mgap : 0;
				auto f_mps_received = (mgap != 0) ? static_cast<float>(now_received - this->m_statistics_recv) / mgap : 0;

				this->m_statistics_send = now_sended;
				this->m_statistics_recv = now_received;
				this->m_statistics_time = _now;

				return { f_mps_sended, f_mps_received, true };
			}
};

class test_udp_echo_client : 
	public object::Ninitializable_startable,
	public schedulable::Iexecutable
{
public:
			void				enable_traffic_test(bool _enable = true) noexcept;
			void				toggle_traffic_test() noexcept { this->enable_traffic_test(!this->m_enable_traffic_test); }
			void				enable_relay_echo_test(bool _enable = true) noexcept { this->m_enable_relay_echo_test = _enable; }
			void				toggle_relay_echo_test() noexcept { this->enable_relay_echo_test(!this->m_enable_relay_echo_test); }

			void				add_traffic_test_count_per_single(int64_t _count) noexcept;
			void				sub_traffic_test_count_per_single(int64_t _count) noexcept;
			void				set_traffic_test_count_per_single(int64_t _count) noexcept;

			void				increase_traffic_test_message_size();
			void				decrease_traffic_test_message_size();
			void				set_traffic_test_message_size(std::size_t _index);

			bool				request_bind(socket_udp* _psocket_udp, net::sockaddr _addr_bind);
			void				request_closesocket(socket_udp* _psocket_udp);
			void				request_closesocket_all();
			void				request_send();
			void				request_send_immidiately(int64_t _count);

public:
	// 1) send test
			std::atomic<uint64_t> m_traffic_send_requested = 0;
			bool				m_enable_traffic_test = false;
			std::size_t			m_traffic_test_selected = 0;
			int64_t				m_traffic_test_count_per_single = 0;
			shared_buffer		m_traffic_test_buf_message[11]{};

	static	bool				m_enable_relay_echo_test;

public:
	virtual	void				on_starting(context& _context) override;
	virtual	void				on_stopping() override;

public:
	virtual	intptr_t			process_execute(intptr_t /*_return*/, std::size_t /*_param*/);
			void				process_execute_send();

			void				process_traffic_test();

			void				process_send_message(std::size_t _index, int64_t _count);
			void				process_send_message(shared_buffer _buffer);
			void				process_ready_send_test_message(std::size_t _index);

			lockable<>			m_cs_set_sockets;
			std::set<object_ptr<socket_udp>> m_set_sockets;
};
