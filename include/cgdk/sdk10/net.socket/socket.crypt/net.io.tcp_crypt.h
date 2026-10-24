//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                         Server Security Controls                          *
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
// net::io::receivable::Nstream_crypt
//
// 1. net::io::receivable::Nstream_crypt란!
//    1) TCP의 send/receive/connect 등을 수행할 수 있는 socket임. 
//    2) 기본적으로 재사용을 위한 Recycle처리가 들어 있음.
//    3) 소량의 연결처리를 전제로 하였으므로 socket manager와 socket Pool 등은
//       지원하지 않는다.
//    4) 그래도 과거에 비해 반응속도가 크게 증가하였고 각종 Pending현상으로 인한
//       Failure를 거의 없앴다.
//
//
// 2. 핵심 function의 기능 설명
//    1) 처리함수들 기본적인 I/O처리를 위한 처리함수들이다.
//					내부적으로 각종 I/O를 처리하기 위해 필요한 함수들에 대해서 정의한 것이다.
//    2) 진행함수들	socket의 각 동작을 정의하는 함수들이다.
//    3) User함수들	재정의를 위해 만들어진 함수이다. 프로그래머가 이 부분들을 재정의
//					함을써 socket을 게임에 맞게 만들게 된다.
//
// 3. 주의 사항
//  run을 항상 일정 시간마다 실행을 시켜 I/O에 대한 처리를 반듯이 해주어야 한다.
//
//-----------------------------------------------------------------------------
class net::io::Ntcp_crypt :
	public						net::io::connectable::Ntcp,
	public						net::io::receivable::Nstream_crypt,
	public						net::io::sendable::Ntcp_crypt_async_gather_buffered,
	virtual public				net::io::Isender,
	virtual public				Icryptable,
	public						net::io::messageable::Nbase
{
protected:
	virtual	void				on_connecting() {}

protected:
	virtual	void				process_certify_client_info(const buffer_view& _pclient_info);
	virtual	auto				process_create_cryptor_for_server(const buffer_view& _pclient_info) -> std::pair<object_ptr<Icryptor>, shared_buffer>;
	static	auto				process_create_cryptor_for_server_pncode(const buffer_view& _pclient_info) -> std::pair<object_ptr<Icryptor>, shared_buffer>;
	static	auto				process_create_cryptor_for_server_none(const buffer_view& _pclient_info) -> std::pair<object_ptr<Icryptor>, shared_buffer>;

	virtual	void				process_request_certification ();
	virtual void				process_initialize_cryptor_for_client(const shared_buffer& _buffer);
	static	auto				process_initialize_cryptor_for_client_pncode(buffer_view _buf_initlaizion) -> object_ptr<Icryptor>;
	static	auto				process_initialize_cryptor_for_client_none(buffer_view /*_buf_initlaizion*/) -> object_ptr<Icryptor>;

	virtual void				process_on_connect () override;
	virtual	void				process_on_disconnect (uint64_t _disconnect_reason, eSOCKET_STATE _socket_state) override;
	virtual	void				process_on_fail_connect (uint64_t _disconnect_reason) override;

	virtual	result_code			process_message (sMESSAGE& _msg) override;
			void				on_message_NETWORK_SECURITY_ACCEPT_CERTIFICATION (const shared_buffer& _message);
			void				on_message_NETWORK_SECURITY_REQUEST_CERTIFICATION (const shared_buffer& _buffer_message);
	static	shared_buffer		message_NETWORK_SECURITY_ACCEPT_CERTIFICATION(buffer_view _buf_initialize_info);

protected:
			class executable_disconnect;
			friend class executable_disconnect;
};


}