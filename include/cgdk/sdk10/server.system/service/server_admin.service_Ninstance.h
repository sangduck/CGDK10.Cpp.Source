//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                           Server Admin Classes                            *
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

class server_system::service::Ninstance :
	virtual public				Imessageable,
	virtual public				net::io::Isender,
	public						Ibuffer_serializable
{
public:
			// 1) service info
			sSERVICE			info;
			sINSTANCE_KEY		instance_key;

			// 2) host instance
			server_system::iid_t host_instance_id{ 0 };

			// 3) executable
			std::vector<factory_snap> vector_executable;

public:
			auto				get_iid() const noexcept { return this->info.setting.iid; }
			auto				get_host_instance_iid() const noexcept { return this->host_instance_id; }
			auto 				get_sender() const noexcept { return this->m_psender; }
			auto 				set_sender(net::io::Isender* _psender) noexcept { this->m_psender = _psender; }
			void				reset_sender() noexcept { this->m_psender.reset(); }
			const auto&			get_lockable() const noexcept { return this->m_lockable_instance; }

			void				set_instance(service::Ninstance&& _service_instance);
			void				set_instance_setting(sSERVICE::sSETTING&& _service_setting);
			void				update_instance_status(sSERVICE_STATUS_UPDATE&& _service_info_update);

			void				register_factory_info(factory_snap&& _data);
			void				unregister_factory_info(uint64_t _index);
			void				update_factory_info(std::vector<factory_update_snap>&& _data);

			void				register_executor_info(executor_snap&& _data);
			void				unregister_executor_info(uint64_t _index);
			void				update_executor_info(std::vector<executor_update_snap>&& _data);
			void				update_schedulable_info(std::vector<schedulable_update_snap>&& _data);

			void				register_connective_info(connective_info_snap&& _data);
			void				unregister_connective_info(uint64_t _index);
			void				update_connective_info(std::vector<connective_info_update_snap>&& _data);
			void				register_binder_info(socket_info_snap&& _data);
			void				unregister_binder_info(uint64_t _index);
			void				update_binder_info(std::vector<socket_info_update_snap>&& _data);

			void				register_time_event_info(time_event_snap&& _data);
			void				unregister_time_event_info(uint64_t _index);
			void				update_time_event_info(std::vector<time_event_update_snap>&& _data);

			void				update_connective_statistics(const statistics::unit_connective_snap& _data);
			void				update_traffic_statistics(const statistics::unit_traffic_snap& _buf_data);

			void				add_log_records(circular_list<object_ptr<LOG_RECORD>>&& _list_log_records) noexcept;

			void				set_crash_info(shared_buffer _buf_data);
			void				update_crash_info(shared_buffer _buf_data);

			void				load_from(context _context);
public:
	virtual	bool				process_send(shared_buffer&& _buffer, std::size_t _count_message, uint64_t _option) override;

protected:
	//virtual result_code			process_message(CGDK::sMESSAGE&) override;
	virtual	void				process_serialize_in(buffer_view& _buffer) override;
	virtual	void				process_serialize_out(buffer& _buffer) override;
	virtual std::size_t			process_size_of() const override;

	mutable lock_object			m_lockable_instance;
			object_ptr<net::io::Isender> m_psender;
			friend class server_system::host::Ninstance;
};

}