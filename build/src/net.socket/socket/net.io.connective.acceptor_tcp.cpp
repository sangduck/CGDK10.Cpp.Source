#include "pch.h"

namespace CGDK
{

extern std::size_t g_max_accept_socket;

#if defined(_WINSOCK2API_)
class net::io::connective::acceptor::Ntcp::executable_connective :
	public						Iexecutable,
	public						Npoolable<executable_connective>,
	public						factory::_traits_system
{
public:
	virtual ~executable_connective() noexcept {}

public:
	void* get_connectable_buffer() noexcept { return reinterpret_cast<void*>(this->m_buffer_connectable); }

	DEFAULT_FACTORY_NAME(net::io::connective::acceptor::Ntcp::executable_connective)

private:
	virtual	intptr_t			process_execute(intptr_t _result, std::size_t /*_param*/) override
	{
		// - call 'process_connective_connect'
		this->m_pconnective->process_connective_connect(this->m_pconnectable, _result);
		
		// - reset
		this->m_pconnectable.reset();
		this->m_pconnective.reset(); 
	
		// return)
		return intptr_t(0);
	}

	// 3) on_final_release함수를 재정의한다.
	virtual	void				on_final_release() noexcept override { this->m_pconnectable.reset(); this->m_pconnective.reset(); }

public:
			object_ptr<Iconnectable> m_pconnectable;
			object_ptr<Iconnective>	m_pconnective;
			char				m_buffer_connectable[256]{};
};
#endif

net::io::connective::Iacceptor::end_point_type net::io::connective::acceptor::Ntcp::get_accept_endpoint() const noexcept
{
	// declare) 
	end_point_type temp_local_endpoint;

	// 1) 설정된 accept_endpoint가 존재하면 그것을 endpoint로 한다.
	if(this->m_accept_endpoint.exist())
	{
		// - 설정된 accept_endpoint를 복사한다.
		temp_local_endpoint = this->m_accept_endpoint;

		// - 설정된 accept_endpoint의 port값이 0이면 현재 bind된 port를 설정한다.
		if(temp_local_endpoint.port() == 0)
		{
			temp_local_endpoint.port(local_endpoint().port());
		}
	}
	// 2) 설정된 accept_endpoint가 없으면 
	else
	{
		// - socket Address를 얻는다.
		temp_local_endpoint = local_endpoint();

		// - bind_any일 경우 local address를 얻어서 설정한다.
		if (temp_local_endpoint.address().is_unspecified())
		{
			// declare) 
			net::sockaddr sockaddr_temp;

			// - Public Address를 얻는다.
			api::socket::instance().get_local_address_public_first(sockaddr_temp);

			// - Port만 읽어온다.
			sockaddr_temp.port(temp_local_endpoint.port());

			// - 값을 그대로 복사
			temp_local_endpoint = sockaddr_temp;
		}
	}

	// return)
	return temp_local_endpoint;
}

void net::io::connective::acceptor::Ntcp::set_accept_endpoint(const end_point_type& _local_endpoint) noexcept
{
	this->m_accept_endpoint = _local_endpoint;
}
void net::io::connective::acceptor::Ntcp::reset_accept_endpoint() noexcept
{
	this->m_accept_endpoint.clear();
}

void net::io::connective::acceptor::Ntcp::process_set_parameters(context& /*_context*/)
{
}

result_code net::io::connective::acceptor::Ntcp::_process_start(context& _context)
{
	// 1) starting 호출
	this->on_start(_context);

	// trace)
	EXECUTE_IF(this->m_enable_progress_log, LOG_PROGRESS << "@ <END> start ACCEPTOR['"sv << name() << "']"sv);

	// return)
	return eRESULT::SUCCESS;
}

result_code net::io::connective::acceptor::Ntcp::_process_stopping() noexcept
{
	// trace)
	EXECUTE_IF(this->m_enable_progress_log, LOG_PROGRESS << "@ <BEGIN> stop ACCEPTOR['"sv << name() << "']"sv);

#if defined(_SYS_EPOLL_H)
	// 1) reset m_pexecutable_accept
	this->m_pexecutable_accept.reset();
#endif
	
	// 2) on_stopping 함수를 호출한다.
	try
	{
		this->on_stopping();
	}
	catch(...)
	{
		// log) 
		LOG_ERROR << "@ (excp) exception on 'on_stopping' function call ACCEPTOR '"sv << name() << "' " __CGFUNCTION__ ""sv;
	}

	// 3) 먼저 Disable한다.
	this->disable();

	// 4) socket을 닫는다.
	this->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);

	// trace) 걸어놓은 Accept개수를 출력한다.
	EXECUTE_IF(this->m_enable_progress_log, LOG_INFO << "  < ACCEPTOR["sv << name() << "] closed"sv);

	// return)
	return eRESULT::SUCCESS;
}

result_code net::io::connective::acceptor::Ntcp::_process_stop() noexcept
{
	// 1) on_stop를 호출한다.
	try
	{
		this->on_stop();
	}
	catch(...)
	{
		// log) 
		LOG_ERROR << "@ (excp) exception on 'on_stop' function call ACCEPTOR '"sv << name() << "' " __CGFUNCTION__ ""sv;
	}

	// trace)
	EXECUTE_IF(this->m_enable_progress_log, LOG_PROGRESS << "@ <END> stop ACCEPTOR['"sv << name() << "']"sv);

	// return)
	return eRESULT::SUCCESS;
}

result_code net::io::connective::acceptor::Ntcp::_process_destroy() noexcept
{
	// 1) on_destroy
	try
	{
		this->on_destroy();
	}
	catch(...)
	{
		// log) 
		LOG_ERROR << "@ (excp) exception on 'on_destroy' function call ACCEPTOR '"sv << name() << "' " __CGFUNCTION__ ""sv;
	}

	// 2) 
	this->set_start_object(nullptr);

	// return)
	return eRESULT::SUCCESS;
}

bool net::io::connective::acceptor::Ntcp::process_set_enable(bool _is_state)
{
	// declare)
	int	pre_state;

	// 1) disable로 상태를 바꾼다.
	pre_state = this->m_is_enable.exchange((_is_state) ? 1 : 0);

	// check) 상태가 바뀌지 않았으면 그냥 끝낸다. (Nenable과 같다 : 바뀌었을 때만 알린다)
	//        (이전 : 'pre_state != 0'이라 enable->disable은 알리지 않고, 이미 disable인 것을 disable하면 알렸다.)
	RETURN_IF(pre_state == ((_is_state) ? 1 : 0), false);

	if (_is_state == true)
	{
		// trace) 성공한 Accept개수를 출력한다.
		EXECUTE_IF(this->m_enable_progress_log, LOG_INFO << "  (info) ACCEPTOR['" << name() << "'] enabled (hSocket:"sv << static_cast<int64_t>(this->native_handle()) << ") "sv);

		// notify) notify to observer
		transmit_message(sMESSAGE{ eMESSAGE::NETWORK::SOCKET::CONNECTIVE_ENABLE, static_cast<net::io::Iconnective*>(this) });

		// notify) notify to connective manager
		net::io::connective::manager::instance()->transmit_message(sMESSAGE{ eMESSAGE::NETWORK::SOCKET::CONNECTIVE_ENABLE, static_cast<net::io::Iconnective*>(this) });
	}
	else
	{
		// trace) 성공한 Accept개수를 출력한다.
		EXECUTE_IF(this->m_enable_progress_log, LOG_INFO << "  (info) ACCEPTOR['" << name() << "'] disabled (hSocket:"sv << static_cast<int64_t>(this->native_handle()) << ")"sv);

		// notify) message 날리기..
		try
		{
			// - notify to observer
			transmit_message(sMESSAGE{ eMESSAGE::NETWORK::SOCKET::CONNECTIVE_DISABLE, static_cast<net::io::Iconnective*>(this) });

			// - notify to connective manager
			net::io::connective::manager::instance()->transmit_message(sMESSAGE{ eMESSAGE::NETWORK::SOCKET::CONNECTIVE_DISABLE, static_cast<net::io::Iconnective*>(this) });
		}
		catch (...)
		{
			// log) 
			LOG_DEBUG << "@ (excp) exception on sending messages ACCEPTOR '" << name() << "' " __CGFUNCTION__ ""sv;
		}
	}

	// return) 
	return true;
}

bool net::io::connective::acceptor::Ntcp::process_get_enable() const
{
	return this->m_is_enable != 0;;
}

std::string SAFE_GET_NAME(Ireferenceable* _pobject)
{
	// 1) Inameable Interface를  얻는다.
	auto pnameable = dynamic_cast<object::Inameable*>(_pobject);

	// check) pnameable이 nullptr이면 그냥 ""를 리턴한다.
	RETURN_IF(pnameable == nullptr, std::string());

	// 2) Name을 얻어 리턴한다.
	return pnameable->name();
}

void net::io::connective::acceptor::Ntcp::process_create_socket(ADDRESS_FAMILY _address_family)
{
	// 1) socket 생성
#if defined(_WINSOCK2API_)
	auto handle_socket_new = api::socket::instance().create_socket(_address_family, SOCK_STREAM, IPPROTO_TCP, api::socket::instance().rio_is_enable() ? WSA_FLAG_REGISTERED_IO : WSA_FLAG_OVERLAPPED);

	// check) socket Option의 수행에 실패했는가?
	if (handle_socket_new == INVALID_SOCKET)
	{
		// log) 
		LOG_DEBUG << "@ (err ) fail to creating socket for ACCEPTOR. error_code:"sv << api::get_last_error() << " " __CGFUNCTION__ ""sv;

		// raise) 
		throw throwable::failure(eRESULT::API_FAILURE, api::get_last_error(), "fail to creating socket handle for acceptor"sv);
	}

	// log) 
	EXECUTE_IF(api::socket::instance().rio_is_enable() && this->m_enable_progress_log, LOG_INFO << "  - enabled 'REGISTERED_IO'");

#elif defined(_SYS_EPOLL_H)
	auto handle_socket_new = api::socket::instance().create_socket(_address_family, SOCK_STREAM, IPPROTO_TCP);

	// check) socket Option의 수행에 실패했는가?
	if (handle_socket_new == INVALID_SOCKET)
	{
		// log) 
		LOG_DEBUG << "@ (err ) fail to creating socket for ACCEPTOR. error_code:"sv << api::get_last_error() << " " __CGFUNCTION__ ""sv;

		// raise)
		throw throwable::failure(eRESULT::API_FAILURE, api::get_last_error(), "fail to creating socket handle for acceptor"sv);
	}

#endif

	// 2) set new socket handle
	CGSOCKET handle_socket_old = this->native_handle(handle_socket_new);

	// 3) close old socket handle
	if(handle_socket_old != INVALID_SOCKET)
	{
	#if defined(_SYS_EPOLL_H)
		// check) io_context must exist
		CGASSERT_ERROR(io_context().exist());

		// 4) detach socket from epoll
		io_context()->detach((CGHANDLE)handle_socket_old);
	#endif

		// - closesocket
		api::socket::closesocket(handle_socket_old);
	}

	// 4) set address family
	address_family(_address_family);

	// 5) set socket option (linux : nonblocking already. create_socket() uses SOCK_NONBLOCK)
#if defined(_WINSOCK2API_)
	api::socket::set_nonblock(handle_socket_new, true);
#endif

	// 6) get io_context
	auto pio_context = io_context();

	// 7) io_context가 없으면 Default io_context를 설정한다.
	if (pio_context.empty())
	{
		// - get system io_context
		pio_context = system_executor::executor();

		// - set io_context
		io_context(pio_context);

		// check) 
		CGASSERT_ERROR(pio_context.exist());
	}

	// 설명) windows의 경우 listen을 하기 전에 attach를 해야하고 linux의 경우 listen을 한후 attach해야한다.(EPOLLEXCLUSIVE 때문에...)

#if defined(_WINSOCK2API_)
	// 8) io_context에 붙인다.
	auto result = pio_context->attach((CGHANDLE)handle_socket_new);

	// check) result가 false이면 Exception 던진다.
	THROW_IF(result == false, throwable::failure(eRESULT::FAIL, 0, "fail to attaching socket to io_context"sv), LOG_DEBUG << "(err ) fail to attaching acceptor to io_context " __CGFUNCTION__ ""sv);

	// trace)
	EXECUTE_IF(this->m_enable_progress_log, LOG_INFO << "  > attached to EXECUTOR['"sv << SAFE_GET_NAME(pio_context) << "']"sv);
#endif
}

void net::io::connective::acceptor::Ntcp::process_prepare_socket(ADDRESS_FAMILY _address_family)
{
	// 1) create socket
	this->process_create_socket(_address_family);

	// 2) socket을 Closed로 한다.
	this->socket_state(eSOCKET_STATE::CLOSED);

	// statistics)
	this->reset_statistics();

	// 3) reset socket을 수행한다.
	this->on_prepare_socket();

	// trace) 
	EXECUTE_IF(this->m_enable_progress_log, LOG_INFO << "  > prepare socket (hSocket="sv << static_cast<int64_t>(this->native_handle()) << ")"sv);
}

void net::io::connective::acceptor::Ntcp::process_dispose_socket() noexcept
{
}

bool net::io::connective::acceptor::Ntcp::process_close_socket(uint64_t _disconnect_reason) noexcept
{
	// 1) socket Status를 Disconnected로...
	auto pre_socket_status = socket_state(eSOCKET_STATE::CLOSED);

	// check) 이전 Status도 Closed상태라면 그냥 함수를 종료한다.
	RETURN_IF(pre_socket_status == eSOCKET_STATE::CLOSED, false);

	// 2) Reason을 설정한다.
	this->m_disconnect_reason |= _disconnect_reason;
	this->m_disconnect_reason |= DISCONNECT_REASON_NODISCONNECTEX;

	// trace) 
	EXECUTE_IF(this->m_enable_progress_log, LOG_INFO << "  - ACCEPTOR['" << name() << "']: close socket (native_handle:"sv << static_cast<int64_t>(this->native_handle()) << ")"sv);

	// 3) on_closesocket함수를 호출한다.
	this->on_closesocket(this->m_disconnect_reason);

#if defined(_SYS_EPOLL_H)
	// - the accepts in process first (process_connective_accept() of an event taken before the state was CLOSED). the listen sockets
	//   are closed after them. (a socket accepted after the wait for the sockets below (count() == 0) was left in the list : its
	//   disconnect called the acceptor destroyed by then => "pure virtual method called". or the accept went to another socket
	//   given the closed fd number)
	this->_process_wait_accepting();

	// - close the UDS listen socket
	this->_process_close_uds();
#endif

	// 4) listen socket을 닫는다.
	auto socket_handle = this->native_handle(INVALID_SOCKET);

	// check) 
	RETURN_IF(socket_handle == INVALID_SOCKET, false);

	// 5) 모든 IO가 완료될 때까지 기다린다.
	//    - Accept중인 socket을 닫는다.
	//    - 접속된 socket을 완전히 제거한다.
	auto pio_context = this->io_context();

	// check) io_context must exist
	CGASSERT_ERROR(this->io_context().exist());

#if defined(_SYS_EPOLL_H)
	// 6) detach socket from epoll (io_uring completion : the multishot accept op is cancelled : it ends with -ECANCELED, the executable releases this acceptor)
	pio_context->detach((CGHANDLE)socket_handle);

	//    (the op holds the file of the listen socket : the cancel in the kernel and the delivery of the -ECANCELED completion are
	//     waited for. the kernel releases the request and its file reference in the task work of the owner of the ring, before the
	//     owner takes the CQE : a close() after only the cancel (the request out of the cancel table) left the port listening a
	//     moment (uds.listen_name_and_release : a connect right after stop() was accepted). the delivery runs no lock of the acceptor
	//     (process_connective_accept_cq) : no dead lock. called by the owner itself : the delivery is not waited for)
	if (this->_is_io_completion_accept())
		pio_context->wait_cancelled_fd((CGHANDLE)socket_handle, chrono::seconds(1), true);
#endif

	// 7) socket을 Delete한다.
	api::socket::closesocket(socket_handle);

	// 8) io_context를 돌려 IOCP의 I/O를 처리한다. (accept 대기 socket과 접속 socket이 모두 정리될 때까지. 단, 최대 MAX_WAIT까지만 기다린다.)
	//    - 좀비 socket(이미 CLOSED이고 진행 중인 I/O가 없고 disconnect 처리까지 끝난(m_pconnectable_notify == nullptr) socket)은
	//      close_connectable_all()로는 지워지지 않는다(이미 닫혀 있어 closesocket()이 바로 리턴한다). 이런 socket이 하나라도 남으면
	//      count()가 0이 되지 않아 stop이 끝나지 않으므로 여기서 직접 목록에서 뺀다.
	//    - MAX_WAIT을 넘기면 오류 로그를 남기고 루프를 빠져나간다. (남은 socket은 아래의 로그로 확인한다.)
	// definitions)
	constexpr auto MAX_WAIT = 30s;

	// - 좀비 socket을 목록에서 빼는 함수 (지운 수를 돌려준다)
	auto remove_zombie_connectable = [this]() noexcept -> std::size_t
	{
		// declare)
		std::vector<object_ptr<Iconnectable>> vector_zombie;

		try
		{
			// 1) 좀비를 모은다. (iterator가 살아 있는 동안 목록 lock이 잡힌다. 여기서는 모으기만 한다.)
			{
				auto piterator = this->alloc_iterator();

				for (auto pconnectable = piterator->get_next(); pconnectable != nullptr; pconnectable = piterator->get_next())
				{
					// check) CLOSED이고 진행 중인 I/O가 없고 disconnect 처리가 끝난 socket만
					CONTINUE_IF(pconnectable->socket_state() != eSOCKET_STATE::CLOSED);
					CONTINUE_IF(pconnectable->statistics_get_now_async_io() != 0);
					CONTINUE_IF(pconnectable->m_pconnectable_notify != nullptr);

					// - push
					vector_zombie.push_back(object_ptr<Iconnectable>(pconnectable));
				}
			}

			// 2) 목록에서 뺀다. (lock을 놓은 뒤에)
			for (auto& iter : vector_zombie)
			{
				// log)
				LOG_WARNING << "(warn) ACCEPTOR['"sv << name() << "']: zombie socket removed from list on stop (closed, no pending I/O, already disconnected)"sv;

				// - remove
				this->remove_connectable(iter);
			}
		}
		catch (...)
		{
			// log)
			LOG_ERROR << "(error) ACCEPTOR['"sv << name() << "']: exception while removing zombie sockets"sv;
		}

		// return)
		return vector_zombie.size();
	};

	// declare)
	auto tick_wait_begin = chrono::tick::now();

	while (this->m_socket_count_prepared_to_accept != 0 || count() != 0)
	{
		// - 모든 accepted된 socket을 종료한다.
		this->close_connectable_all();

		// - execute를 실행한다.
		pio_context->execute_all();

		// - 좀비 socket을 목록에서 뺀다.
		remove_zombie_connectable();

		// check) 너무 오래 기다렸으면 오류 로그를 남기고 빠져나간다.
		if ((chrono::tick::now() - tick_wait_begin) > MAX_WAIT)
		{
			// log)
			LOG_ERROR << log_level_t::important() << "@ (excp) ACCEPTOR['"sv << name() << "']: give up waiting for sockets to close (waited "sv << std::chrono::duration_cast<std::chrono::seconds>(MAX_WAIT).count() << "s, prepared:"sv << this->m_socket_count_prepared_to_accept << ", connected:"sv << static_cast<int64_t>(count()) << ")"sv;
			break;
		}

		// - Sleep하기.
		api::sleep_for(1ms);
	}

#if defined(_WINSOCK2API_)
	// check) 아직 Accept중인 socket이 남이 있을 겨우!
	EXECUTE_IF(this->m_socket_count_prepared_to_accept != 0, LOG_ERROR << log_level_t::important() << "@ (excp) listen socket is exist yet[prepared:"sv << this->m_socket_count_prepared_to_accept << "]"sv);

	// check) 아직 접속 종료되지 않았을 경우!
	EXECUTE_IF(count()!=0, LOG_ERROR << log_level_t::important() << "@ (excp) connected socket is exist yet[connected:"sv << static_cast<int64_t>(count()) << "]"sv);
#elif defined(_SYS_EPOLL_H)
	
#else
#endif

	// 10) io_context를 reset한다.
	this->reset_io_context();

	// return) 
	return true;
}

#if defined(_WINSOCK2API_)
void net::io::connective::acceptor::Ntcp::request_accept(ADDRESS_FAMILY _address_family)
{
	// check) Accept가 Disable된 상태라면 잘못된거다.(프로그래밍을 잘못 짠것임.)
	RETURN_IF(this->is_disable());

	// check) 소켓의 상태가 eSOCKET_STATE::LISTEN가 아니라면 false를 리턴한다.
	RETURN_IF(this->socket_state()!= eSOCKET_STATE::LISTEN);


	//-----------------------------------------------------------------
	// 1. connective Completor 준비하기.
	//-----------------------------------------------------------------
	// 1) 새로운 Accept를 댄다.
	auto pconnectable = this->process_alloc_connectable();

	// check) pconnectable가 nullptr이어서는 안된다.
	CGASSERT_ERROR(pconnectable != nullptr);

	// - the send queue limits back to the ones of the socket object (a socket from the pool kept the ones its last connection set).
	//   before the default of the accepted sockets below
	if (auto pgather = dynamic_cast<net::io::sendable::Itcp_gather*>(pconnectable.get()); pgather != nullptr)
		pgather->reset_send_queue_limit();

	// 2) io_context를 얻는다.
	auto pio_context = this->io_context();

	// check) io_context가 nullptr이면 그냥 끝낸다.
	RETURN_IF(pio_context.empty());

	// 2) socket을 reset한다.
	//    (hook : process_get_connectable_io_context() may give the socket its own io_context. default : the one of the acceptor)
	if (auto pio_context_connectable = this->process_get_connectable_io_context(pconnectable.get()); pio_context_connectable.exist())
	{
		pconnectable->io_context(std::move(pio_context_connectable));
	}
	else
	{
		pconnectable->io_context(pio_context);
	}

	// 3) socket option을 설정한다.
	pconnectable->maximum_send_buffer_size(net::io::socket::Ntcp::default_maximum_send_buffer_size());
	pconnectable->maximum_receive_buffer_size(net::io::socket::Ntcp::default_maximum_receive_buffer_size());

	// - 받은 접속(client)의 송신 queue 최대 크기 (기본 4MB, 연결 수가 많으므로 낮게 둔다.)
	//   (socket이 따로 설정하지 않아 전역 기본값인 경우에만 적용한다.)
	if (auto pgather = dynamic_cast<net::io::sendable::Itcp_gather*>(pconnectable.get()); pgather != nullptr && pgather->maximum_depth_of_send_buffer_bytes() == net::io::sendable::Itcp_gather::default_maximum_queued_bytes())
		pgather->maximum_depth_of_send_buffer_bytes(net::io::sendable::Itcp_gather::default_maximum_queued_bytes_accepted());

	// - 암호화 socket(Icryptable)이면 acceptor에 설정된 cryption type을 적용한다.
	if (auto pcryptable = dynamic_cast<Icryptable*>(pconnectable.get()); pcryptable != nullptr)
		pcryptable->process_set_cryption_type(this->m_cryption_type);

	// check) _address_family가 AF_UNSPEC이어서는 안된다.
	CGASSERT_ERROR(_address_family != AF_UNSPEC);

	// 4) io_context를 설정한다.
	try
	{
		pconnectable->prepare_socket(_address_family);
	}
	catch(...)
	{
		// -  reset connectable
		pconnectable.reset();

		// reraise)
		throw;
	}

	// check) pconnectable의 socket handle이 INVALID_SOCKET이여서는 안된다.
	CGASSERT_ERROR(pconnectable->native_handle() != INVALID_SOCKET);

	// 5) connectable manager를 nullptr로 설정한다.
	pconnectable->m_flexible.get<Iconnectable_manager*>() = nullptr;


	//-----------------------------------------------------------------
	// 2. socket Option설정하기.
	//-----------------------------------------------------------------
	{
		// 1) port를 reuse하도록 한다.(중요:diconnect_ex로 socket을 재활용하므로 port를 재활용(Reuse)하지 않으면 error를 발생시킬 수 있다.)
		api::socket::set_reuse_address(pconnectable->native_handle(), true);

		// 2) fast Loopback 활성화
		api::socket::set_loopback_fast_path(pconnectable->native_handle(), true);
	}

	//-----------------------------------------------------------------
	// 3. socket State 'eSOCKET_STATE::SYN'상태로 바꾸기.
	//-----------------------------------------------------------------
	{
		// 1) change socket state from 'eSOCKET_STATE::CLOSED' to 'eSOCKET_STATE::SYN'
		bool is_changed = pconnectable->set_socket_state_if(eSOCKET_STATE::CLOSED, eSOCKET_STATE::SYN);

		// check) socket state must be 'eSOCKET_STATE::CLOSED'
		CGASSERT_ERROR(is_changed == true);

		// check) socket state must be 'eSOCKET_STATE::CLOSED'
		EXECUTE_IF(is_changed != true, LOG_DEBUG << "@ (err ) accept_async() state of socket is not 'eSOCKET_STATE::CLOSED' " __CGFUNCTION__ ""sv);
	}

	//-----------------------------------------------------------------
	// 4. prepare executable_connective
	//-----------------------------------------------------------------
	// 1) alloc executable_connective object
	auto pexecutable_connective = make_object<executable_connective>();

	// check) 
	CGASSERT_ERROR(pexecutable_connective->reference_count() == 1);

	// 2) set variable 
	pexecutable_connective->m_pconnective = this;
	pexecutable_connective->m_pconnectable = pconnectable;

	// 3) increase reference count
	pexecutable_connective->add_ref();



	//-----------------------------------------------------------------
	// 5. process_request_connect
	//-----------------------------------------------------------------
	// - the socket option back to the defaults (a socket from the pool kept the option of its last acceptor or connection).
	//   on_request_accept() may set it per socket, the acceptor's socket_option() (when given) replaces it right before on_accept()
	pconnectable->socket_option(sSOCKET_OPTION_TCP{});

	// 1) on_request_accept
	this->on_request_accept(pconnectable);

	// declare)
	net::sockaddr address_peer;

	// 2) process request_connect
	pconnectable->process_request_connect(address_peer);


	//-----------------------------------------------------------------
	// 6. accept
	//-----------------------------------------------------------------
	// statistics) 
	this->statistics_on_increase_async();

	// 1) Accept를 수행한다.
	auto result = api::socket::accept_async(this->native_handle(), pconnectable->native_handle(), pexecutable_connective->get_connectable_buffer(), pexecutable_connective.get());

	// check) accept_async를 실패했다.
	if (result == false)
	{
		// - set last error
		pconnectable->m_last_socket_error = api::get_last_error();

		// trace) 
		LOG_DEBUG << "@ (err ) fail to 'api::socket::accept_async()'. error_code:" << api::get_last_error() << " (socket:" << static_cast<int64_t>(this->native_handle()) << ")";

		// - process_close_socket
		pconnectable->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE | DISCONNECT_REASON_FAIL_TO_SOCKET_ACCEPT);

		// statistics) 
		this->statistics_on_decrease_async();

		// - release reference count
		pexecutable_connective->release();

		// statistics)
		this->statistics_on_fail_connect();

		// throw) 
		throw throwable::failure(eRESULT::API_FAILURE, 0, "fail to accept socket"sv);
	}

	// 2) accept한 socket을 하나 더한다.
	++this->m_socket_count_prepared_to_accept;

	// statistics)
	this->statistics_on_try();

	// 3) 할당된 소켓수를 증가시킨다.
	++this->m_socket_count_allocated;
}

class net::io::connective::acceptor::Ntcp::executable_retry_request_accept :
	virtual public	Iexecutable,
	public			Npoolable<executable_retry_request_accept>,
	public			factory::_traits_system
{
public:
	virtual	intptr_t process_execute(intptr_t _result, std::size_t _param) override;

public:
	object_ptr<net::io::connective::acceptor::Ntcp>	m_pconnective_acceptor;
};

intptr_t net::io::connective::acceptor::Ntcp::executable_retry_request_accept::process_execute(intptr_t /*_result*/, std::size_t /*_param*/)
{
	// 1) Process disconnect
	this->m_pconnective_acceptor->process_retry_accept();

	// 2) Release
	this->m_pconnective_acceptor.reset();

	// return) Success!!
	return intptr_t(0);
}

void net::io::connective::acceptor::Ntcp::process_request_retry_accept()
{
	SCOPED_LOCK(this->m_lockable_retry_accept_now)
	{
		// check)
		RETURN_IF(this->m_retry_accept_now);

		// 1) set flag
		this->m_retry_accept_now = true;

		// 2) ...
		auto result = this->_process_request_retry_accept();

		// check)
		if (result == false)
		{
			this->m_retry_accept_now = false;
		}
	}
}

bool net::io::connective::acceptor::Ntcp::_process_request_retry_accept()
{
	// 1) make executable object
	auto pexecutable = make_object< executable_retry_request_accept>();

	// 2) set 'm_pconnective_acceptor' this
	pexecutable->m_pconnective_acceptor = this;

	// 3) request 
	return system_executor::post(pexecutable, chrono::tick::now() + 1s);

}

void net::io::connective::acceptor::Ntcp::process_retry_accept()
{
	// check)
	CGASSERT_ERROR(this->m_retry_accept_now == true);

	SCOPED_LOCK(this->m_lockable_retry_accept_now)
	{
		// check)
		if (is_disable())
		{
			// - set ...
			this->m_retry_accept_now = false;

			// return) 
			return;
		}
	}

	// 1) get param
	auto prepared_now = this->m_socket_count_prepared_to_accept.load();
	auto accept_count_must_prepare = this->m_accept_count_must_prepare;

	// check) 
	if (accept_count_must_prepare <= prepared_now)
	{
		SCOPED_LOCK(this->m_lockable_retry_accept_now)
		{
			this->m_retry_accept_now = false;
		}

		// return) 
		return;
	}

	// 2) accept걸어야 할 수를 계산한다.
	auto count_request_accept = accept_count_must_prepare - prepared_now;

	try
	{
		// 4) _count_accept_prepare_on_start만큼 accept를 건다.
		for (count_request_accept; count_request_accept > 0; --count_request_accept)
		{
			// - Accept를 수행한다.
			this->request_accept(this->m_address_family);
		}
	}
	catch (...)
	{
	}

	// 3) retry if prepared_socket is 0
	SCOPED_LOCK(this->m_lockable_retry_accept_now)
	{
		if (count_request_accept == 0)
		{
			this->m_retry_accept_now = false;
		}
		else
		{
			// - ...
			auto result = _process_request_retry_accept();

			// check)
			if (result == false)
			{
				this->m_retry_accept_now = false;
			}
		}
	}
}

void net::io::connective::acceptor::Ntcp::process_connective_connect( Iconnectable* _pconnectable, uintptr_t _result)
{
	//-----------------------------------------------------------------
	// ProcessCompleteAccept()함수는 크게 두 부분으로 나눌 수 있다.
	//
	// * 첫째는 Accept받은 socket의 접속처리 부분.
	// * 둘째는 새로운 Accept를 거는 부분
	//
	// 으로 나눌 수 있다.
	// 하나의 처리가 다른 하나의 처리에 영향을 미치지 않고 따로 Error나 Exception 
	// 처리를 수행한다.
	//-----------------------------------------------------------------
	// check) _pconnectable이 nullptr이 되면 안된다.
	CGASSERT_FATAL(_pconnectable != nullptr);


	//-----------------------------------------------------------------
	// 1. Accept완료를 했으므로 걸어 놓은 수를 줄인다.
	//-----------------------------------------------------------------
	// 1) Accept를 받았으므로 준비중인 Accept수를 하나 줄인다.
	--this->m_socket_count_prepared_to_accept;


	//-----------------------------------------------------------------
	// 2. 현재 acceptor가 Disable 혹은 소켓 최대 한도 초과 시
	//-----------------------------------------------------------------
	// check) Accept가 Disable된 상태나 최대 할당 수보다 많은 경우라면 끝낸다.
	if(this->is_disable() || api::socket::is_socket_count_overflowed())
	{
		// - reason
		auto disconnect_reason = (this->is_disable()) ? (DISCONNECT_REASON_FAIL | DISCONNECT_REASON_ABORTIVE | DISCONNECT_REASON_DISABLED) : (DISCONNECT_REASON_FAIL | DISCONNECT_REASON_ABORTIVE | DISCONNECT_REASON_OPEN_SOCKET_LIMITS);

		// - connectable의 socket을 닫는다.
		_pconnectable->process_close_socket(disconnect_reason);

		// - 할당된 소켓수를 증가시킨다.
		--this->m_socket_count_allocated;

		// return)
		return;
	}

	try
	{
		//-----------------------------------------------------------------
		// 3. Accept받은 socket의 처리.
		//-----------------------------------------------------------------
		// Case SUCCRESS) accept가 성공했을 경우
		//   - accept가 제대로 되었을 때는 accept처리를 한다.
		//   - accept 받은 socket의 option을 설정하고... (update_accept_context)
		//   - on_accept()함수를 호출하고...
		//   - _pconnectable의 process_complete_connect()함수를 호출하여 
		//     소켓별 접속처리를 진행한다.
		if ((_result == 0) && (count() < this->m_max_allocate) && (count() < g_max_accept_socket))
		{
			// 1) update accept context 옵션을 설정한다.
			{
				// - call 'update_accept_context'
				auto result = api::socket::update_accept_context(_pconnectable->native_handle(), this->native_handle());

				// check) socket Option에 실패했을 경우 socket을 닫는다.
				//        - socket의 Option 설정이 잘못되었을 경우 socket 자체가 망가진 것이다.
				//        - 따라서 Option 설정에 실패했을 경우 소켓을 닫고 릴리즈 한다.
				//        - 이과정은 아래 과정에서 실행될 것이다.
				if (result == false)
				{
					// - set last error
					this->m_last_socket_error = api::get_last_error();

					// log) 
					LOG_DEBUG << "(excp) fail to setting socket option 'SO_UPDATE_ACCEPT_CONTEXT' socket="sv << static_cast<uint64_t>(_pconnectable->native_handle()) << "/ error:"sv << api::get_last_error() << " " __CGFUNCTION__ ""sv;

					// raise) 
					throw throwable::failure(eRESULT::API_FAILURE, api::get_last_error(), "fail to setting socket option 'SO_UPDATE_ACCEPT_CONTEXT'"sv);
				}
			}

			// 2) call 'process_on_accept'
			// - the tcp socket option of the acceptor ("socket_option" / socket_option()) : set to the socket here, right before on_accept()
			//   (on_accept() may change it per socket. applied to the handle in process_complete_connect. not given : the socket keeps its own.
			//    not at the alloc : on windows the accept sockets are posted in advance (prepare_on_start) and a socket_option() set to the acceptor after start() would wait for them)
			if (this->m_flag_socket_option)
				_pconnectable->socket_option(this->m_socket_option);

			this->process_on_accept(_pconnectable);

			// 3) set variable
			//    주의) _pconnectable->m_pconnectable_notify = this 설정은 반드시 process_complete_connect(...)가 호출되기 전에 설정되어야 한다.
			//          process_complete_connect 처리 과정 중에 this->m_pconnectable_notify 값을 on_connect() 호출에서 인자로 넘긴다.
			_pconnectable->m_pconnectable_notify = this;

			// 주의) statistics_on_success_connect() 함수는 '_pconnectable->process_complete_connect(...)'의 처리 중 실패 시 statistics상 disconnect로 처리되므로
			// //    반드시 여기서 호출되어야 한다.
			//       이 처리과정은 '접속 성공' 루틴이므로 '_pconnectable->process_complete_connect(...)'에서 on_fail_connect(...)루틴을 타지는 않는다!

			// statistics) 
			this->statistics_on_success_connect();

			// 4) call 'process_complete_connect'
			if (_pconnectable->process_complete_connect(_result) == true)
			{
				// 설명) process_complete_connect가 성공했으므로 이후 실패는 접속종료로 처리된다.
				//       - 일단 on_connect()호출이 끝난 이후므로 이후에 실패 시 on_disconnect()가 호출되어야 한다.
				//         따라서 이후 실패처리는 접속 종료 처리로 진행된다.
				//       - 처리 실패 시 exception이 던져지고 catch에서 접속 종료 처리가 진행된다.

				// - call 'add_connectable'
				//   주의) add_connectable(_pconnectable)은 반드시 process_complete_connect(...)의 호출 이후에 한다.
				//         그렇지 않으면 on_connect()처리 도중에 acceptor의 connectable_list에 등록되므로 여기서 socket이 참조되어 처리가 될 수 있기 때문에 문제가 될 수 있다.
				//         즉 on_connect() 도중에 다른 처리가 진행될 수 있다.
				//         특히 epoll의 경우 100% 문제가 발생하므로 반드시 주의해야 한다.
				auto result = this->add_connectable(_pconnectable);

				// check)
				CGASSERT_ERROR(result == true);

				// check)
				THROW_IF(result == false, throwable::failure(eRESULT::FAIL));

				// check) add_connectable() 전에 이미 disconnect 처리가 끝난 socket인가?
				//        - process_complete_connect()가 receive를 걸고 돌아오므로, 접속 직후 끊긴 socket은 receive 실패가 다른 thread에서
				//          add_connectable()보다 먼저 process_connective_disconnect()를 실행할 수 있다. 그때 remove_connectable()은
				//          목록에 없어 아무것도 지우지 못하고 m_pconnectable_notify만 nullptr가 되고 m_socket_count_allocated만 줄어든다.
				//        - 그대로 두면 닫힌 socket이 목록에 영원히 남아(좀비) count()가 0이 되지 않고 stop 시 process_close_socket()이
				//          끝나지 않는다. (disconnect 처리는 이미 끝났으므로 여기서 목록에서 빼는 것 외에 더 할 일은 없다.)
				if (_pconnectable->m_pconnectable_notify == nullptr)
				{
					// log)
					LOG_WARNING << "(warn) ACCEPTOR['"sv << name() << "']: socket disconnected before add_connectable(). removed from list (socket:"sv << static_cast<int64_t>(_pconnectable->native_handle()) << ")"sv;

					// - remove from list
					this->remove_connectable(_pconnectable);
				}
			}
			else
			{
				// - rollback
				_pconnectable->m_pconnectable_notify = nullptr;
			}
		}
		else
		{
			// - process_on_fail_connect
			this->process_on_fail_accept(_pconnectable);

			// - closesocket
			_pconnectable->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE | DISCONNECT_REASON_CONNECTION_FULL);

			{
				// - call 'process_complete_connect'
				_pconnectable->process_complete_connect(1);

				// statistics) 
				this->statistics_on_fail_connect();

				// - decrease socket_count_allocated
				--this->m_socket_count_allocated;
			}
		}
	}
	catch(...)
	{
		// trace) 
		LOG_DEBUG << "@ (excp) ACCEPTOR['" << name() << "']: exception complete socket:" << (int64_t)((_pconnectable != nullptr) ? static_cast<int64_t>(_pconnectable->native_handle()) : 0) << " " __CGFUNCTION__ ""sv;

		// - connectable의 socket을 닫는다.
		_pconnectable->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE | DISCONNECT_REASON_EXCEPTION_OCCURRED);

		// statistics) 
		this->statistics_on_fail_connect();

		// - decrease socket_count_allocated
		--this->m_socket_count_allocated;
	}


	//-----------------------------------------------------------------
	// 4. prepare new accept
	//-----------------------------------------------------------------
	try
	{
		// 1) get prepare count
		std::size_t requested_count = (this->m_accept_count_must_prepare > this->m_socket_count_prepared_to_accept) ? this->m_accept_count_must_prepare - this->m_socket_count_prepared_to_accept : 0;
		
		// 2) request prepare accept
		for(; requested_count > 0 && this->m_socket_count_prepared_to_accept < this->m_accept_count_must_prepare; --requested_count)
		{
			this->request_accept(this->m_address_family);
		}

		// check)
		CGASSERT_ERROR(this->m_socket_count_prepared_to_accept >= 0);
	}
	catch(std::exception& e)
	{
		// trace) 
		LOG_DEBUG << "@ (excp) ACCEPTOR['" << name() << "']: exception complete socket:"sv << static_cast<int64_t>(this->native_handle()) << " " __CGFUNCTION__ ""sv;

		// - call 'exception handler'
		throwable::execute_exception_handler(this, e, __FUNCTION__);

		// - 
		if (this->m_socket_count_prepared_to_accept == 0 && api::get_last_error() == WSAETOOMANYREFS)
		{
			this->process_request_retry_accept();
		}
	}
	catch(...)
	{
		// trace) 
		LOG_DEBUG << "@ (excp) ACCEPTOR['" << name() << "']: exception complete socket:"sv << static_cast<int64_t>(this->native_handle()) << " " __CGFUNCTION__ ""sv;

		// - call 'exception handler'
		throwable::execute_exception_handler(this, throwable::failure(eRESULT::EXCEPTION), __FUNCTION__);

		// - 
		if (this->m_socket_count_prepared_to_accept == 0 && api::get_last_error() == WSAETOOMANYREFS)
		{
			this->process_request_retry_accept();
		}
	}

	// statistics)
	this->statistics_on_decrease_async();
}
#elif defined(_SYS_EPOLL_H)
bool net::io::connective::acceptor::Ntcp::request_accept(CGSOCKET _handle_socket_new, ADDRESS_FAMILY _address_family)
{
	//-----------------------------------------------------------------
	// 1. connective Completor 준비하기.
	//-----------------------------------------------------------------
	// 1) 새로운 socket을 할당받는다.
	auto pconnectable = process_alloc_connectable();

	// - the send queue limits back to the ones of the socket object (a socket from the pool kept the ones its last connection set)
	if (auto pgather = dynamic_cast<net::io::sendable::Itcp_gather*>(pconnectable.get()); pgather != nullptr)
		pgather->reset_send_queue_limit();

	// - 암호화 socket(Icryptable)이면 acceptor에 설정된 cryption type을 적용한다.
	if (auto pcryptable = dynamic_cast<Icryptable*>(pconnectable.get()); pcryptable != nullptr)
		pcryptable->process_set_cryption_type(this->m_cryption_type);

	// 2) get io_context
	auto pio_context = pconnectable->io_context();

	// 3) set native socket handle
	{
		// - set socket
		auto handle_socket_old = pconnectable->native_handle(_handle_socket_new);

		// - 기존에 설정되어 있던 소켓핸들이 INVALID_SOCKET이 아니라면 소켓을 닫는다.
		if (handle_socket_old != INVALID_SOCKET)
		{
			// check) io_context must exist
			if (pio_context.exist())
			{
				// - detach socket from epoll
				pio_context->detach((CGHANDLE)handle_socket_old);
			}

			// - closesocket
			api::socket::closesocket(handle_socket_old);
		}

		// check) _address_family가 AF_UNSPEC이어서는 안된다.
		CGASSERT_ERROR(_address_family != AF_UNSPEC);
		CGASSERT_ERROR(_address_family == AF_INET || _address_family == AF_INET6 || _address_family == AF_UNIX);

		// - set address family
		pconnectable->address_family(_address_family);

		// - nonblocking already (accept4 with SOCK_NONBLOCK)

		// - the io_context of the socket (like windows) : the hook first (process_get_connectable_io_context : an executor per socket,
		//   cpu affinity), else the one of the acceptor, else the system executor.
		//   (the io_context the socket had before - a pooled socket - is not used : the socket of an acceptor given an executor ran on
		//    the system executor here, while the header and the windows path give it the acceptor's)
		if (auto pio_context_connectable = this->process_get_connectable_io_context(pconnectable.get()); pio_context_connectable.exist())
			pio_context = std::move(pio_context_connectable);
		else if (auto pio_context_acceptor = this->io_context(); pio_context_acceptor.exist())
			pio_context = std::move(pio_context_acceptor);
		else
			pio_context = system_executor::executor();

		pconnectable->io_context(pio_context);

		// - prepare executable socket
		pconnectable->process_prepare_executable_socket();
	}

	// 3) io_context를 설정한다.
	pconnectable->prepare_socket(_address_family);

	// check) pconnectable의 socket handle이 INVALID_SOCKET이여서는 안된다.
	CGASSERT_ERROR(pconnectable->native_handle() != INVALID_SOCKET);

	// check) pconnectable의 socket handle이 INVALID_SOCKET이여서는 안된다.
	CGASSERT_ERROR(pconnectable->socket_state() == eSOCKET_STATE::CLOSED);

	// 4) reset connectable manager ptr
	pconnectable->m_flexible.get<Iconnectable_manager*>() = nullptr;


	//-----------------------------------------------------------------
	// 2. set socket state to SYN
	//-----------------------------------------------------------------
	// 1) change socket state from 'eSOCKET_STATE::CLOSED' to 'eSOCKET_STATE::SYN'
	bool is_changed = pconnectable->set_socket_state_if(eSOCKET_STATE::CLOSED, eSOCKET_STATE::SYN);

	// check) socket state must be 'eSOCKET_STATE::CLOSED'
	CGASSERT_ERROR(is_changed == true);

	// check) socket state must be 'eSOCKET_STATE::CLOSED'
	EXECUTE_IF(is_changed != true, LOG_DEBUG << "@ (err ) accept_async() state of socket is not 'CLOSED' " __CGFUNCTION__ ""sv);


	//-----------------------------------------------------------------
	// 3. process_request_connect
	//-----------------------------------------------------------------
	// - the socket option back to the defaults (a socket from the pool kept the option of its last acceptor or connection).
	//   on_request_accept() may set it per socket, the acceptor's socket_option() (when given) replaces it right before on_accept()
	pconnectable->socket_option(sSOCKET_OPTION_TCP{});

	// 1) on_request_accept를 호출한다.
	this->on_request_accept(pconnectable);

	// declare)
	net::sockaddr address_peer;

	// 2) process request_connect
	pconnectable->process_request_connect(address_peer);


	//-----------------------------------------------------------------
	// 4. process connective connect
	//-----------------------------------------------------------------
	// 1) increase allocated socket count
	++this->m_socket_count_allocated;

	// 2) call 'process_connective_connect'
	this->process_connective_connect(pconnectable, 0);

	// return)
	return true;
}

// - the acceptor whose accept this thread processes (process_close_socket() called in it : not waiting for itself)
thread_local const void* t_pacceptor_accepting = nullptr;	// - (net.io.connective.acceptor_tcp.uring.cpp uses it too)

void net::io::connective::acceptor::Ntcp::_process_wait_accepting() noexcept
{
	// definitions)
	constexpr auto MAX_WAIT = 30s;

	// - the accept of this thread (stop() in on_accept()/on_connect() ...) is not waited for
	const int count_self = (t_pacceptor_accepting == this) ? 1 : 0;
	const auto tick_wait_begin = chrono::tick::now();

	while (this->m_count_accepting.load(std::memory_order_seq_cst) > count_self)
	{
		// check)
		if ((chrono::tick::now() - tick_wait_begin) > MAX_WAIT)
		{
			LOG_ERROR << log_level_t::important() << "@ (excp) ACCEPTOR['"sv << name() << "']: give up waiting for the accepts in process (accepting:"sv << this->m_count_accepting.load() << ")"sv;
			break;
		}

		std::this_thread::yield();
	}
}

void net::io::connective::acceptor::Ntcp::process_connective_accept(uintptr_t _result, CGSOCKET _handle_listen, ADDRESS_FAMILY _address_family)
{
	// check) accept가 disable된 상태나 최대 할당 수보다 많은 경우라면 끝낸다.
	RETURN_IF(is_disable());

	// - in process : process_close_socket() waits for it before it closes the listen sockets. (counted before the state is read :
	//   process_close_socket() sets the state before it reads the count. one or the other sees it)
	this->m_count_accepting.fetch_add(1, std::memory_order_seq_cst);

	struct ACCEPTING
	{
		Ntcp*			pthis;
		const void*		pbefore;
		~ACCEPTING() noexcept { t_pacceptor_accepting = pbefore; pthis->m_count_accepting.fetch_sub(1, std::memory_order_seq_cst); }
	} accepting{ this, t_pacceptor_accepting };

	t_pacceptor_accepting = this;

	// check) closed (stop) meanwhile, or the event of a UDS listen socket closed meanwhile (taken before the close : the fd number
	//        may be another socket's by now)
	RETURN_IF(this->socket_state() == eSOCKET_STATE::CLOSED);
	RETURN_IF(_handle_listen != INVALID_SOCKET && _handle_listen != this->m_handle_uds_listen.load(std::memory_order_acquire));

	// - the listen socket (the acceptor's TCP socket or the UDS socket)
	const auto handle_listen = (_handle_listen != INVALID_SOCKET) ? _handle_listen : this->native_handle();
	const auto address_family = (_address_family != AF_UNSPEC) ? _address_family : this->m_address_family;

	if (_result & EPOLLIN)
	{
		for (;;)
		{
			// declare)
			net::sockaddr sockaddr_listen;
			socklen_t addr_len = sizeof(sockaddr_listen);

			// statistics)
			this->statistics_on_try();

			// 1) 접속해온 socket handle를 accept 받는다.
			CGSOCKET handle_socket_new = api::socket::accept(handle_listen, reinterpret_cast<::sockaddr*>(&sockaddr_listen), &addr_len);

			// check) break if no more pended accepting socket
			if (handle_socket_new < 0)
			{
				// - get error
				auto error_code = api::get_last_error();

				// check)
				BREAK_IF(error_code == EAGAIN || error_code == EWOULDBLOCK);

				// 설명) EMFILE, ENFILE, ENOBUFS, ENOMEM와 같은 이유로 accept를 받지 못할 경우 계속 event가 발생해
				//       CPU 100%를 차지할 수 있으므로 적절한 처리가 필요하다.

				// - set socket error code
				this->m_last_socket_error = error_code;

				switch (error_code)
				{
				// case) "too many opn file"
				case	EMFILE:
						break;

				// case) "file table overflow"
				case	ENFILE:
						break;

				// case) have no memory
				case	ENOBUFS:
				case	ENOMEM:
						break;
				}

				// statistics)
				this->statistics_on_fail_connect();

				// break) 
				break;
			}

			// 2) request accept
			this->_process_accepted(handle_socket_new, address_family);
		}
	}

	if (_result & EPOLLPRI)
	{
		this->statistics_on_decrease_async();
	}
}

void net::io::connective::acceptor::Ntcp::_process_accepted(CGSOCKET _handle_socket_new, ADDRESS_FAMILY _address_family) noexcept
{
	// - an accepted socket (the accept loop above, a CQE of the multishot accept of a completion io_context)
	try
	{
		this->request_accept(_handle_socket_new, _address_family);
	}
	catch (...)
	{
		// statistics)
		this->statistics_on_fail_connect();
	}
}

void net::io::connective::acceptor::Ntcp::process_closesocket_fail(net::io::Iconnectable* _pconnectable, disconnect_reason_t _disconnect_reason) noexcept
{
	// check) already closed by the connectable (e.g. process_complete_connect() closed the socket itself before failing)
	if (_pconnectable->native_handle() == INVALID_SOCKET)
		return;

	// 1) reset native socket handle
	_pconnectable->native_handle(INVALID_SOCKET);

	// 2) set disconnect reason
	_pconnectable->disconnect_reason(_disconnect_reason);

	// 3) get connectable tcp
	auto pconnectable_tcp = dynamic_cast<net::io::connectable::Ntcp*>(_pconnectable);

	if (pconnectable_tcp != nullptr)
	{
		// 4) complete closesocket
		pconnectable_tcp->process_complete_closesocket();

		// 5) reset executable socket (release _conneective) 
		pconnectable_tcp->process_reset_executable_socket();
	}
}

// - the end of the processing of an accepted socket (see process_connective_connect) : a close is done at once again, not queued
static void _process_end_processing_accept(net::io::Iconnectable* _pconnectable) noexcept
{
	SCOPED_LOCK(_pconnectable->m_epoll_event_cs)
	{
		_pconnectable->m_epoll_event_flag_queued = 0;
		_pconnectable->m_epoll_event_processing_now = false;
	}
}

void net::io::connective::acceptor::Ntcp::process_connective_connect(Iconnectable* _pconnectable, uintptr_t _result)
{
	// check) _pconnectable이 nullptr이 되면 안된다.
	CGASSERT_FATAL(_pconnectable != nullptr);

	// check) closesocket if accept is disabled
	if (this->is_disable() || api::socket::is_socket_count_overflowed())
	{
		// - reason
		auto disconnect_reason = (this->is_disable()) ? (DISCONNECT_REASON_FAIL | DISCONNECT_REASON_ABORTIVE | DISCONNECT_REASON_DISABLED) : (DISCONNECT_REASON_FAIL | DISCONNECT_REASON_ABORTIVE | DISCONNECT_REASON_OPEN_SOCKET_LIMITS);

		// - closesocket
		process_closesocket_fail(_pconnectable, disconnect_reason);

		// - decrease socket_count_allocated
		--this->m_socket_count_allocated;

		// return)
		return;
	}

	if ((_result == 0) && (count() < this->m_max_allocate) && (count() < g_max_accept_socket))
	{
		try
		{
			// 1) call 'process_on_accept'
			// - the tcp socket option of the acceptor ("socket_option" / socket_option()) : set to the socket here, right before on_accept()
			//   (on_accept() may change it per socket. applied to the handle in process_complete_connect. not given : the socket keeps its own.
			//    not at the alloc : on windows the accept sockets are posted in advance (prepare_on_start) and a socket_option() set to the acceptor after start() would wait for them)
			if (this->m_flag_socket_option)
				_pconnectable->socket_option(this->m_socket_option);

			this->process_on_accept(_pconnectable);

			// 2) set variable
			_pconnectable->m_pconnectable_notify = this;
		}
		catch (...)
		{
			// - process_on_fail_connect
			this->process_on_fail_accept(_pconnectable);

			// - close socket
			process_closesocket_fail(_pconnectable, DISCONNECT_REASON_FAIL | DISCONNECT_REASON_ABORTIVE | DISCONNECT_REASON_EXCEPTION_OCCURRED);

			// - decrease allocated socket count
			--this->m_socket_count_allocated;

			// return)
			return;
		}

		// statistics)
		this->statistics_on_success_connect();

		// - processing (like the connect of connect_requestable::Ntcp) : a close meanwhile (in on_connect, or by another thread till attached) is queued,
		//   not done : the handle stays open and this socket's till the attach below (closed at once, the fd number may be given to another socket
		//   before the attach : the attach registered the other socket's fd with this executable). the queued close is completed in 5)
		SCOPED_LOCK(_pconnectable->m_epoll_event_cs)
		{
			_pconnectable->m_epoll_event_flag_queued = 0;
			_pconnectable->m_epoll_event_processing_now = true;
		}

		// 3) call 'process_complete_connect'
		{
			// - call 'process_complete_connect'
			auto result = _pconnectable->process_complete_connect(_result);

			// check)
			if (result == false)
			{
				// - end of processing
				_process_end_processing_accept(_pconnectable);

				// - reset variable
				//   주의) _pconnectable->m_pconnectable_notify = this 설정은 반드시 process_complete_connect(...)가 호출되기 전에 설정되어야 한다.
				//         process_complete_connect 처리 과정 중에 m_pconnectable_notify 값을 on_connect() 호출에서 인자로 넘긴다.
				_pconnectable->m_pconnectable_notify = nullptr;

				// - decrease socket_count_allocated
				--this->m_socket_count_allocated;

				// - close socket
				process_closesocket_fail(_pconnectable, DISCONNECT_REASON_FAIL | DISCONNECT_REASON_ABORTIVE);

				// return)
				return;
			}
		}

		// 설명) 여기서부터 실패는 접속 종료 처리를 해야한다.
		//       - 일단 on_connect()호출이 끝난 이후므로 이후에 실패 시 on_disconnect()가 호출되어야 한다.
		//         따라서 이후 실패처리는 접속 종료 처리로 진행된다.
		//       - 처리 실패 시 exception이 던져지고 catch에서 접속 종료 처리가 진행된다.

		try
		{
			// 4) add to connected list
			//   주의) add_connectabl(_pconnectable)은 반드시 process_complete_connect(...)의 호출 이후에 한다.
			//         그렇지 않으면 on_connect()처리 도중에 acceptor의 connectable_list에 등록되므로 여기서 socket이 참조되어 처리가 될 수 있기 때문에 문제가 될 수 있다.
			//         즉, on_connect() 도중에 다른 처리가 진행될 수 있다.
			//         특히 epoll의 경우 100% 문제가 발생하므로 반드시 주의해야 한다.
			{
				// - call 'add_connectable'
				auto result = this->add_connectable(_pconnectable);

				// check)
				CGASSERT_ERROR(result == true);

				// check)
				THROW_IF(result == false, throwable::failure(eRESULT::FAIL));

				// check) add_connectable() 전에 이미 disconnect 처리가 끝난 socket인가?
				//        - process_complete_connect()가 receive를 걸고 돌아오므로, 접속 직후 끊긴 socket은 receive 실패가 다른 thread에서
				//          add_connectable()보다 먼저 process_connective_disconnect()를 실행할 수 있다. 그때 remove_connectable()은
				//          목록에 없어 아무것도 지우지 못하고 m_pconnectable_notify만 nullptr가 되고 m_socket_count_allocated만 줄어든다.
				//        - 그대로 두면 닫힌 socket이 목록에 영원히 남아(좀비) count()가 0이 되지 않고 stop 시 process_close_socket()이
				//          끝나지 않는다. (disconnect 처리는 이미 끝났으므로 여기서 목록에서 빼는 것 외에 더 할 일은 없다.)
				if (_pconnectable->m_pconnectable_notify == nullptr)
				{
					// log)
					LOG_WARNING << "(warn) ACCEPTOR['"sv << name() << "']: socket disconnected before add_connectable(). removed from list (socket:"sv << static_cast<int64_t>(_pconnectable->native_handle()) << ")"sv;

					// - end of processing
					_process_end_processing_accept(_pconnectable);

					// - remove from list
					this->remove_connectable(_pconnectable);

					// return) attach 하지 않고 끝낸다.
					return;
				}
			}

			// 5) attach to io_context
			//    주의) - io_context에 attach()는 반드시 process_complete_connect()가 실행된 이후에 처리되어야 한다.
			//            process_complete_connect()함수에서 on_connect()가 호출되기 때문에 이 함수 호출이 끝난 후에
			//            receive event를 받아야 하기 때문이다.
			//            또 이 과정에서 closesocket()이 바로 호출될 수 있기 때문에 그에 대한 대비 역시 필요하다.
			//            만약에 closesocket()이 호출되었다면 peio_context->attach가 실패할 것이다.
			//          - 이 시점부터 접속성공으로 처리된다. 따라서 이 이후에 실패 처리는 접속 종료로 처리한다.
			//            즉 통계적으로 on_fail_accept가 아니라 on_disconnect 처리된다.
			//            또 on_disconnect()함수도 호출된다.
			{
				// - get io_context
				//   (the io_context of the socket : set in request_accept(). the one of the acceptor, or the one given by process_get_connectable_io_context())
				auto pio_context = _pconnectable->io_context();
				if (pio_context.empty())
					pio_context = this->io_context();

				// check)
				CGASSERT_ERROR(pio_context.exist());

				// declare)
				bool is_closed = false;
				bool result = true;

				SCOPED_LOCK(_pconnectable->m_epoll_event_cs)
				{
					// - closed meanwhile (queued) : not attached. the disconnect is completed below
					is_closed = (_pconnectable->m_epoll_event_flag_queued & (EPOLLHUP | EPOLLRDHUP | EPOLLERR)) != 0;
					_pconnectable->m_epoll_event_flag_queued = 0;

					// 6) do start =>attach to socket eio_context
					//    - the epoll handle : still this socket's (a close is queued while processing). under the lock : an event of another thread waits for the end of processing
					if (is_closed == false)
					{
						result = _pconnectable->process_start_io(pio_context.get(), false);

						// - end of processing (failed : the close in the catch below is done, not queued)
						_pconnectable->m_epoll_event_processing_now = false;
					}
				}

				// - closed meanwhile : completes the disconnect (like process_close_socket() does when no event is processed)
				if (is_closed)
				{
					_pconnectable->process_complete_disconnect();

					// - end of processing
					_process_end_processing_accept(_pconnectable);
				}

				// check)
				THROW_IF(result == false, throwable::failure(eRESULT::FAIL));
			}
		}
		catch (...)
		{
			// - end of processing (a close is done, not queued)
			_process_end_processing_accept(_pconnectable);

			// - connectable의 socket을 닫는다.
			_pconnectable->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);

			// - get connectable tcp
			auto psocket_tcp = dynamic_cast<net::io::socket::Ntcp*>(_pconnectable);

			if (psocket_tcp != nullptr)
			{
				// - call 'process_complete_connectable'
				psocket_tcp->m_pconnectable->process_complete_disconnect();
			}
		}
	}
	else
	{
		// - process_on_fail_connect
		this->process_on_fail_accept(_pconnectable);

		// - connectable의 process_complete_connect()함수를 수행한다.
		_pconnectable->process_complete_connect(1);

		// - close socket
		process_closesocket_fail(_pconnectable, DISCONNECT_REASON_FAIL | DISCONNECT_REASON_ABORTIVE);

		// statistics) 접속 실패 횟수
		this->statistics_on_fail_connect();

		// 4) 할당된 소켓수를 줄인다.
		--this->m_socket_count_allocated;
	}
}
#else
#endif

void net::io::connective::acceptor::Ntcp::process_connective_disconnect( Iconnectable* _pconnectable)
{
	// 1) connectable manager에서 제거한다. 
	this->remove_connectable(_pconnectable);

	// 2) connectable의 pconnectable notify를 this로 한다.
	_pconnectable->m_pconnectable_notify = nullptr;

	// 3) Allocated된 socket수를 줄인다.
	--this->m_socket_count_allocated;

	// statistics)
	this->statistics_on_error_disconnect(_pconnectable->statistics_get_error_disconnected());

	// statistics) 
	this->statistics_on_disconnect();

#if defined(_WINSOCK2API_)
	// check) 만약 prepare된 숫자가 모자랄 경우 추가 Accept를 건다.
	if(this->is_enable() && this->m_accept_count_must_prepare > this->m_socket_count_prepared_to_accept)
	{
		try
		{
			// request accept
			this->request_accept(this->m_address_family); // (*) exception이 발생할 수 있다.

			// check)
			CGASSERT_ERROR(this->m_socket_count_allocated != 0)
		}
		catch(std::exception& e)
		{
			// trace) 
			LOG_DEBUG << "@ (excp) ACCEPTOR['" << name() << "']: exception complete socket:"sv << static_cast<int64_t>(this->native_handle()) << " " __CGFUNCTION__ ""sv;

			// - call 'exception handler'
			throwable::execute_exception_handler(this, e, __FUNCTION__);

			// - ...
			if (this->m_socket_count_prepared_to_accept == 0 && api::get_last_error() == WSAETOOMANYREFS)
			{
				this->process_request_retry_accept();
			}
		}
		catch(...)
		{
			// trace) 
			LOG_DEBUG << "@ (excp) ACCEPTOR['" << name() << "']: exception complete socket:"sv << static_cast<int64_t>(this->native_handle()) << " " __CGFUNCTION__ ""sv;

			// declare)
			throwable::failure e(eRESULT::EXCEPTION);

			// - call 'exception handler'
			throwable::execute_exception_handler(this, e, __FUNCTION__);

			// - 
			if (this->m_socket_count_prepared_to_accept == 0 && api::get_last_error() == WSAETOOMANYREFS)
			{
				this->process_request_retry_accept();
			}
		}
	}
#endif
}


}
