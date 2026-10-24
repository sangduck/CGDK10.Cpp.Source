//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                          network socket classes                           *
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
// net::io::Irestorable
//
//-----------------------------------------------------------------------------
class net::io::Irestorable :
	virtual public				net::io::Iconnectable,
	virtual public				net::io::Isender,
	virtual public				net::io::Imessageable
{
public:
			bool				send_reliable (const shared_buffer& _buffer);

			bool				request_io_restore ();
	[[nodiscard]] bool			is_io_suspending () const noexcept { return m_flag_suspended;}
	[[nodiscard]] auto			get_io_suspend_tick () const noexcept { return m_tick_suspend;}

	[[nodiscard]] const auto&	get_restore_server_id () const noexcept { return m_id_restore_server;}
	[[nodiscard]] const auto&	get_restore_client_id () const noexcept { return m_id_restore_client;}

protected:
	virtual	void				process_request_io_restore (result_code _result, sequence_id_t _ack_received) PURE;
	virtual	void				process_response_io_restore (result_code _result, sequence_id_t _ack_received) PURE;
	virtual	void				process_complete_io_restore (result_code _result) PURE;

public:
			void				process_reset_io_restorable () noexcept;
			result_code			process_get_restore_result (const restore_server_id_t& _id_restoreserver, const restore_client_io_t& _id_restoreclient, sequence_id_t _ack_received, sequence_id_t _ack_sent);

	virtual	result_code			process_message (sMESSAGE& _msg) override;
			result_code			process_message_SEND_MESSAGE_ACK_REQUIRED (sMESSAGE_NETWORK& _msg);
			result_code			process_message_ACK (sMESSAGE_NETWORK& _msg);
			result_code			process_message_RESPONSE_IO_RESTORE (sMESSAGE_NETWORK& _msg);
			result_code			process_message_COMPLETE_IO_RESTORE (sMESSAGE_NETWORK& _msg);

	[[nodiscard]] shared_buffer	message_REQUEST_IO_RESTORE ();
	[[nodiscard]] shared_buffer	message_SEND_ACK (sequence_id_t _ack_received);
	[[nodiscard]] shared_buffer	message_RESPONSE_IO_RESTORE (result_code _result);
	[[nodiscard]] shared_buffer	message_RESTORE_COMPLETE (result_code _result);

public:
			restore_server_id_t	m_id_restore_server;
			restore_client_io_t	m_id_restore_client;

			bool				m_flag_suspended = false;
			chrono::tick::time_point m_tick_suspend;
			sequence_id_t			m_ack_received = 0;
			sequence_id_t			m_ack_sent = 0;
	mutable	lock_object			m_lockable_list_sending;
			circular_list<shared_buffer> m_list_sending;
};


}