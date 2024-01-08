//*****************************************************************************
//*                                                                           *
//*                             CGCII Trade System                            *
//*                                                                           *
//*                               Control Server                              *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                  ⓒ2017. Cho SangHyun. All rights reserved                *
//*                          http://www.CGCIII.co.kr                          *
//*                                                                           *
//*****************************************************************************

#pragma once
//-----------------------------------------------------------------------------
//
// server::group
//
//-----------------------------------------------------------------------------
class server_entity::manager_instance :
	public						net::io::messageable::Nbase,
	public						Nobject_state,
	public						Ninitializable,
	public						Nstartable,
	public						Nnameable
{
public:
			 manager_instance(std::string_view _name);
	virtual ~manager_instance();

public:
	// 1) 새로운 server_instance를 등록/등록해제
			object_ptr<instance> register_instance			(const server_admin::sSERVER_REGISTER& _server_register, Isocket* _psocket);
			void				unregister_instance			(const server_admin::sID& _id) noexcept;			

	// 2) server_instance의 접속/접속해제
			void				enter_instance				(server_admin::server_info&& _server_instance, server_entity::Isocket* _psocket);
			void				unregister_instance			(instance* _pserver_instance) noexcept;
			void				update_instance				(server_admin::server_info_update&& _server_info_update) noexcept;

			object_ptr<instance> find_instance				(const server_admin::sHOST_ID& _hid, const server_admin::sID& _id) const noexcept;
			object_ptr<instance> find_instance				(uint64_t _id) const noexcept;
			vector<object_ptr<instance>> search_instance	(const server_admin::sHOST_ID& _hid, const server_admin::sID& _id) const noexcept;

private:
	virtual	void				on_initializing				(context& _context) override;
	virtual	void				on_initialize				(context& _context) override;
	virtual	void				on_destroying				() override;
	virtual	void				on_destroy					() override;

	virtual	void				on_starting					(context& _context) override;
	virtual	void				on_start					(context& _context) override;
	virtual	void				on_stopping					() override;
	virtual	void				on_stop						() override;

public:
			lockable<>			m_lockable_register;
			std::map<uint64_t, object_ptr<instance>>	m_map_server_register;

public:
			shared_buffer		message_SERVER_ADMIN_HOST_SOCKET_LIST();
			shared_buffer		message_SERVER_ADMIN_SERVER_SETTING_LIST();
};
