//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                            server.query.redis                             *
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
// query::redis::session
//
//
// ----------------------------------------------------------------------------
class query::redis::session :
	public						net::io::connect_requestable::Ntcp_reconnect,
	public						net::io::socket::tcp::Ndefault,
	public						net::io::connectable::tcp::Ndefault,
	public						net::io::sendable::Ntcp_async_gather_buffered,
	public						net::io::sender::Nresp,
	public						net::io::receivable::tcp::Ndefault,
	public						net::io::packetable::Nresp,
	virtual public				Imessageable
{
public:
			session();
	virtual	~session() noexcept;

public:
			struct REQUEST_QUERY
			{
				object_ptr<Iquery>		pquery;
				shared_buffer			buf_request;
				std::function<void()>	response;
			};

			bool				execute_query(REQUEST_QUERY&& _query);
			void				cancel_query_all() noexcept;
			void				add_query_reference();
			void				release_query_reference() noexcept;

protected:
	virtual void				on_request_connect(const net::sockaddr& _paddress_peer) override;
	virtual void				on_connect() override;
	virtual void				on_fail_connect(uint64_t _disconnect_reason) override;
	virtual	void				on_disconnect(uint64_t _disconnect_reason) override;

	virtual	result_code			process_message(sMESSAGE& _msg) override;

			std::recursive_mutex m_cs_queue_query;
			circular_list<REQUEST_QUERY> m_queue_query;
			object_ptr<querier>	m_pquerier;

			REQUEST_QUERY		pop_query() noexcept;
			void				push_query(REQUEST_QUERY _request_info);

			// - registered to / unregistered from the querier ('m_pquerier' is changed in 'm_cs_queue_query'. the old one is returned)
			//   detached : its last release does not go back to free_session(). '_is_in_use' : a query still uses it
			object_ptr<querier>	process_attach_querier(querier* _pquerier) noexcept;
			object_ptr<querier>	process_detach_querier(bool& _is_in_use) noexcept;
			// - the requests waiting for responses are completed with no result ('_pquerier' : statistics. nullptr : not counted)
			void				process_cancel_query_all(querier* _pquerier) noexcept;

			// - the number of the queries attached. the session is in use while it is not 0 (statistics: alloc at 0->1, free at 1->0)
			std::atomic<int>	m_query_reference;
			// - taken out of the querier's stack by a dedicated query (alloc_session()). a shared session stays in the stack
			//   (accessed holding the querier's 'm_lockable_query')
			bool				m_is_dedicated = false;
			friend class querier;
			friend class Iquery;
};


}