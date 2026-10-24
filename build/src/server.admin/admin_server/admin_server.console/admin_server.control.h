//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                           Server Admin - Admin Server                     *
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
//-----------------------------------------------------------------------------
//
// admin::socket_control (http control api)
//
//  - setting: "acceptor_control": { "local_endpoint": {...}, "certify_key", "allowed_addresses" }
//    (없으면 127.0.0.1:21888, 로컬 접속만 허용)
//  - 인증
//    1) allowed_addresses : 허용된 주소만 접속한다. (certify_key도 allowed_addresses도 없으면 loopback만 허용)
//    2) certify_key       : 'X-CGDK-Key: <key>' 또는 'Authorization: Bearer <key>' 헤더가 같아야 한다.
//       또는 요청마다 HMAC-SHA256 서명 ('X-CGDK-Timestamp', 'X-CGDK-Signature'. "require_signature": true면 서명만 받는다.)
//    3) key를 여러 번 틀리면 그 주소를 잠시 막는다. (429. "auth_fail_max", "auth_fail_window", "auth_block_time")
//  - 상태를 바꾸는 요청(POST/PUT)은 '(audit)' log를 남긴다.
//  - api : admin_server.policy.h (parse_control_route) 참고
//
//      curl http://127.0.0.1:21888/api/instances
//      curl -X POST http://127.0.0.1:21888/api/instances/21101/start
//      curl -X PUT  http://127.0.0.1:21888/api/instances/21101/desired/running
//      curl http://127.0.0.1:21888/metrics
//
//-----------------------------------------------------------------------------
namespace admin
{
	class socket_control;
}

class admin::socket_control :
	public						net::socket::http_server,
	public						net::io::add_on::Nsocket_sustain,
	public						Nmessage_transmitter
{
public:
			socket_control();

protected:
	virtual void				on_connect() override;
	virtual result_code			on_message(sMESSAGE_HTTP_REQUEST& _msg) override;
	virtual result_code			on_message(sMESSAGE_HTTP_WEBSOCKET& /*_msg*/) override { return eRESULT::DONE; }
	virtual result_code			on_message(sMESSAGE_NETWORK& /*_msg*/) override { return eRESULT::BYPASS; }

			void				process_send_response(const sMESSAGE_HTTP_REQUEST& _msg, eHTTP_STATUS_CODE _status_code, std::string_view _content_type, const std::string& _body, std::string_view _retry_after = {});
			bool				process_check_key(const sMESSAGE_HTTP_REQUEST& _msg) const;
	static	std::string_view	to_method_string(eHTTP_METHOD _method) noexcept;

			std::atomic<bool>	m_is_rejected{ false };
};
