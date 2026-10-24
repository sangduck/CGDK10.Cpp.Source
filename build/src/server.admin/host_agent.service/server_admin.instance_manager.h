//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                          Server Admin Host Agent                          *
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
#include <filesystem>

// - 설정 파일 (working directory 기준)
#define	SETTING_SERVER_LIST			"host_agent_server_list.json"
#define	SETTING_FILE_HOST_AGENT		"host_agent_setting.json"
// ----------------------------------------------------------------------------
//
// group_peer_immigration
//
// 1. group_peer_immigration는...
//     접속된 Server의 Socket을 관리하는 group이다.
//    내부적으로 Admin Client Socket을 관리하는 Admin Client group을 가지고 있다.
//
// ----------------------------------------------------------------------------
class server_system::instance_manager :
	public						object::Nnameable,
	public                      object::Ninitializable_startable,
	virtual public				Nmessage_transmitter_single,
	public						net::io::messageable::Nbase,
	public						schedulable::Nexecutable
{
public:
			object_ptr<Iinstance> register_instance(own_ptr<Iinstance>&& _pinstance);
			bool				unregister_instance(iid_t, socket_peer* _psocket) noexcept;
			void				unregister_instance_all() noexcept;

			object_ptr<Iinstance> find_instance(iid_t _id) const noexcept;
			object_ptr<Iinstance> find_instance_by_pid(uint64_t _pid) const noexcept;
			object_ptr<Iinstance> find_instance_by_pvoid(void* _id_select) const noexcept;

			shared_buffer		message_server_system_HOST_STATUS_UPDATE();

public:
	virtual	void				on_initialize(context& _context) override;
	virtual	void				on_destroy() override;
	virtual	void				on_start(context& _context) override;
	virtual	void				on_stop() override;
	virtual	void				on_execute() override;

	virtual result_code			on_message(sMESSAGE_NETWORK& _msg) override;
			result_code			on_message_WINDOWS_NOTIFY_UPDATE(sMESSAGE_NOTIFY& _msg);
			result_code			on_message_server_system_REQUEST_INIT_SERVICE(sMESSAGE_NETWORK& _msg);
			result_code			on_message_server_system_REQUEST_DESTROY_SERVICE(sMESSAGE_NETWORK& _msg);
			result_code			on_message_server_system_REQUEST_START_SERVICE(sMESSAGE_NETWORK& _msg);
			result_code			on_message_server_system_REQUEST_STOP_SERVICE(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_TIME_EVENT_EVENT_REGISTER(sMESSAGE& _msg);
			result_code			on_message_SERVER_TIME_EVENT_EVENT_UNREGISTER(sMESSAGE& _msg);
			result_code			on_message_SERVER_TIME_EVENT_EVENT_EXECUTE(sMESSAGE& _msg);
			result_code			on_message_SERVER_TIME_EVENT_EVENT_UPDATE(sMESSAGE& _msg);
			result_code			on_message_SYSTEM_LOG(sMESSAGE_LOG& _msg);

			void				process_load_host_server_info();
			void				process_save_host_server_info();
			void				process_load_server_list();
			std::vector<own_ptr<instance_service_local>> process_read_server_list();
			void				process_reload_server_list();
			void				process_reload_setting();
			void				process_check_reload();
			void				process_remove_pending();
			void				process_notify_list_changed(const std::vector<object_ptr<instance_service>>& vector_added, const std::vector<object_ptr<instance_service>>& vector_removed);
	static	std::filesystem::file_time_type _get_file_time(std::string_view _filename) noexcept;

			// - 설정 파일의 수정 시각 (바뀌면 on_execute에서 다시 읽는다.)
			std::filesystem::file_time_type m_time_server_list{};
			std::filesystem::file_time_type m_time_setting{};

			// - 목록에서 빠졌지만 실행 중이라 남겨둔 service (멈추면 지운다.)
			std::set<iid_t>		m_set_pending_remove;
			void				process_save_server_list();
			void				process_update_system_status_info();
			object_ptr<Iinstance> process_register_instance_host(own_ptr<Iinstance>&& _pinstance_host);
			object_ptr<Iinstance> process_register_instance_service(own_ptr<Iinstance>&& _pinstance_service);
			object_ptr<instance_service_local> _process_find_instance_by_pid(uint64_t _pid);
			object_ptr<instance_service_local> _process_find_instance_by_rank(server_system::sRANK _rank);
			object_ptr<instance_service_local> process_find_instance_by(uint64_t _pid, server_system::sRANK _rank);

			lock_object			m_lockable_instance;
			std::map<iid_t, own_ptr<Iinstance>> m_map_instance;
public:
	static	auto&				self() noexcept { return m_instance_host_self; }

	static	own_ptr<instance_host>					m_instance_host_self;
	static	object_ptr<api::performance_counter>	m_performance_processor;
	static	std::string								m_address_admin_server;

	static	std::atomic<uint64_t>					m_id_alloc;
	static	uint64_t								_alloc_object_id();
};
