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
// ----------------------------------------------------------------------------
//
// socket_admin_server (HostAgent -> AdminServer)
//
// ----------------------------------------------------------------------------
class socket_admin_server :
	public						net::io::connect_requestable::Ntcp_reconnect,
	public						net::socket::tcp<>,
	public						Nmessage_transmitter
{
public:
	// - admin server에 인증(INSTANCE_CERTIFY)된 후에만 전송한다.
	//   (인증 전의 상태는 INSTANCE_CERTIFY_REQUEST로 한꺼번에 전달된다.)
			bool				send_if_certified(const shared_buffer& _buffer);
			bool				is_certified() const noexcept { return this->m_is_certified; }

	// - setting이 바뀌었으면(remote_endpoint, certify_key) 다시 접속한다. (reload. return: 다시 접속하면 true)
			bool				apply_setting(context& _context);

private:
	virtual	void				on_initialize(context& _context) override;
	virtual	void				on_destroy() override;
	virtual	void				on_start(context& _context) override;
	virtual	void				on_stop() override;
	virtual void				on_connect() override;
	virtual void				on_disconnect(uint64_t _disconnect_reason) override;
	virtual void				on_certify() {}

	virtual result_code			on_message(sMESSAGE_NETWORK& _msg);
			result_code			on_message_server_system_CERTIFY_RESPONSE(sMESSAGE_NETWORK& _msg);
			result_code			on_message_server_system_INSTANCE_CERTIFY_RESPONSE(sMESSAGE_NETWORK& _msg);
			result_code			on_message_server_system_REQUEST_UPDATE_SETTING(sMESSAGE_NETWORK& _msg);
			result_code			on_message_server_system_REQUEST_SERVICE(sMESSAGE_NETWORK& _msg);

			shared_buffer		process_make_message_INSTANCE_CERTIFY_REQUEST();
			void				process_certify_failed(eRESULT _result);
			void				process_certify_succeeded();
			void				send_request_result(server_system::iid_t _iid, uint32_t _request, uint64_t _request_id, eRESULT _result, std::string_view _message);

			std::atomic<bool>	m_is_certified{ false };
	static	std::string			_get_certify_key(context& _context);
	static	std::string			_get_remote_endpoint_string(context& _context);

			mutable std::mutex	m_lockable_setting;						// - m_str_remote_endpoint, m_certify_key (on_start/apply_setting <-> on_connect(socket thread))
			std::string			m_str_remote_endpoint;					// - "address:port" (reload에서 비교한다.)
			std::string			m_certify_key;							// - 'socket_to_admin_server'.'certify_key' (or CGDK_ADMIN_CERTIFY_KEY)
			chrono::tick::duration m_reconnect_interval_base{ chrono::tick::duration::zero() };
			int					m_count_certify_fail{ 0 };
			bool				m_is_failure_counted{ false };
};