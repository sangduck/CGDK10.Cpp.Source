#include "pch.h"
#include "net.io.connect_requestable.tcp_reconnect.manager_reconnector.h"

using namespace CGDK;


net::io::connect_requestable::Ntcp_reconnect::Ntcp_reconnect() noexcept : 
	net::io::connect_requestable::Ntcp()
{
}

net::io::connect_requestable::Ntcp_reconnect::Ntcp_reconnect(std::string_view _name) :
	net::io::connect_requestable::Ntcp(_name)
{
}

net::io::connect_requestable::Ntcp_reconnect::~Ntcp_reconnect() noexcept
{
	this->destroy();
}

bool net::io::connect_requestable::Ntcp_reconnect::process_request_connecting( const net::sockaddr& _address)
{
	// 1) _address가 nullptr이 아닐 경우 모든 List를 제거하고 _address하나만 등록한다.
	if(_address.is_unspecified() == false)
	{
		// - 모든 PeerAddress를 제거한다.
		this->remove_all_peer_address();

		// - 새로 들어온 PeerAddress를 추가한다.
		this->add_peer_address(_address);
	}

	// declare) 
	auto address_peer = move_next_peer_address();
	
	// 3) 접속 시도시 m_tick_disconnected 시간을 갱신한다.
	this->m_tick_disconnected = chrono::tick::now();
		
	// 4) connective를 this로...
	this->m_pconnectable_notify = this;

	// 5) Reconning을 false로 한다.
	this->m_flag_reconnecting = false;

	// 6) connect를 요청한다.
	auto result = net::io::connect_requestable::Ntcp::process_request_connecting(address_peer);
	
	// check) 해당하는 Address에 접속시도에 실패했다면... m_pconnectable_notify를 reset한다.
	if (result == false)
	{
		// - connective를 reset한다.
		this->m_pconnectable_notify = nullptr;
	}

	// return) 
	return result;
}

void net::io::connect_requestable::Ntcp_reconnect::process_request_reconnect()
{
	// check) executor 붙이기를 실패했을 경우 Reconnector를 reset한다.
	if(this->reconnection_enable() == false)
	{
		// - Reconnect를 Cancel한다.
		this->process_cancel_reconnector();

		// return) 
		return;
	}

	scoped_lock(this->m_lockable_reconnector)
	{
		// 1) Reconnection을 시작했음으로 설정한다.
		this->m_flag_reconnecting = true;

		// 2) Reconnector을 얻는다.
		if(this->m_pmanager_reconnector.empty())
		{
			// - 먼저 manager를 얻는다.
			this->m_pmanager_reconnector = manager_reconnector::instance();

			// - 등록한다.
			this->m_pmanager_reconnector->register_reconnector(this);
		}
	}
}

void net::io::connect_requestable::Ntcp_reconnect::process_cancel_reconnector() noexcept
{
	scoped_lock(this->m_lockable_reconnector)
	{
		// 1) Unreister한다.
		if(this->m_pmanager_reconnector.exist())
		{
			// - unregister한다.
			this->m_pmanager_reconnector->unregister_reconnector(this);

			// - reset한다.
			this->m_pmanager_reconnector.reset();
		}

		// 2) Reconnection을 false로...
		this->m_flag_reconnecting = false;
	}
}

void net::io::connect_requestable::Ntcp_reconnect::process_complete_connecting( uintptr_t _result)
{
#if defined(_WINSOCK2API_)
	//-----------------------------------------------------------------
	// 1. socket Option을 바꾼다.
	//
	// - 여기서 SO_UPDATE_CONNECT_CONTEXT option을 설정하는 것은 매우
	//   매우 중요하다.
	//   connect_async를 통해 접속을 받은 socket에 대해 이 옵션을 수행해
	//   주지 않으면 각종 socket관련 함수들 이 Not connect라며 Error를 
	//   내게 되므로 반드시 SO_UPDATE_CONNECT_CONTEXT 옵션을 설정해
	//   주어야 한다.
	//
	//-----------------------------------------------------------------
	if(_result == 0)
	{
		// 1) socket 속성을 Update함.
		bool result = api::socket::update_connect_context(native_handle());

		// check) socket Option의 수행에 실패했는가?
		EXECUTE_IF(result == false, LOG_DEBUG << "(err ) CGDK.net.socket: fail to setting 'SO_UPDATE_CONNECT_CONTEXT' option " __CGFUNCTION__ ""sv);
	}

#elif defined(_SYS_EPOLL_H)
	// cheeck) 중복처리 중이면 그냥 끝낸다.(epoll은 중복으로 들어 올 수 있다.)
	scoped_lock(this->m_epoll_event_cs)
	{
		// check)
		RETURN_IF(this->m_result_connecting != eRESULT::REQUEST);

		// - 처리된 것임을 표시한다.
		this->m_result_connecting = eRESULT::PROCESS;
	}

	//-----------------------------------------------------------------
	// 1. EPOLL event에 따라 _result값을 변경한다.
	//
	// - _result값이 실패면 1, 성공이면 0로 변경한다.
	// - EPOLL_OUT을 받게 되면 접속성공이므로 _result을 1로 설정하고
	//   아니면 0으로 설정한다.
	// - EPOLL일 경우 이 부분이 중복 호추로딜 수 있으므로 그에 대한
	// 	 처리가 필요하다.
	//
	//-----------------------------------------------------------------
	// 2-1) 접속에 실패했을 경우
	if (_result & (EPOLLHUP | EPOLLERR))
	{
		_result = 1;
	}
	// 2-2) 접속에 성공했을 경우
	else if (_result & EPOLLOUT)
	{
		_result = 0;
	}
	else
	{
		// log) 
		LOG_DEBUG << "(warning) undefined epoll event ['"sv << _result << "'] "sv << " " __CGFUNCTION__ ""sv;

		// - assert
		CGASSERT_ERROR(false);

		// - set result as i (failure) 
		_result = 1;
	}
#endif

	// 3) 완료 후 처리 - process_complete_connect
	auto result = this->process_complete_connect(_result);

#if defined(_WINSOCK2API_)
	// check) 만약 process_complete_connect가 false가 돌아온다면 Reconnect를 시도한다.
	//        (쉽게 말해서 접속에 실패했을 경우 재접속 처리한다.)
	//        다만 m_pconnectable_notify가 nullptr이 아닌 경우에만...
	if (result == false)
	{
		// - 재접속 시도를 한다.
		if (this->m_pconnectable_notify != nullptr && reconnection_enable())
		{
			// - connective를 reset한다.
			this->m_pconnectable_notify = nullptr;

			// - 재접속을 시도한다.
			this->process_request_reconnect();
		}
		else
		{
			// - 접속에 실패했으며 재접속 시도가 disable되었으므로 재접속처리를 종료한다.
			this->process_cancel_reconnector();

			// - Result를 설정한다.
			this->m_result_connecting = eRESULT::FAIL;

			// - connective를 reset한다.
			this->m_pconnectable_notify = nullptr;
		}
	}
	else
	{
		// - 접속에 성공했으므로 재접속처리를 종료한다.
		this->process_cancel_reconnector();

		// - Result를 설정한다.
		this->m_result_connecting = eRESULT::SUCCESS;
	}

#elif defined(_SYS_EPOLL_H)
	// check) 만약 process_complete_connect가 false가 돌아온다면 Reconnect를 시도한다.
	//        (쉽게 말해서 접속에 실패했을 경우 재접속 처리한다.)
	//        다만 m_connective가 nullptr이 아닌 경우에만...
	if (result == false)
	{
		if (this->m_pconnectable_notify != nullptr && this->reconnection_enable())
		{
			// - connective를 reset한다.
			this->m_pconnectable_notify = nullptr;

			// - 재접속을 시도한다.
			this->process_request_reconnect();
		}
		else
		{
			// - 접속에 성공했으므로 재접속처리를 종료한다.
			this->process_cancel_reconnector();

			// - Result를 설정한다.
			this->m_result_connecting = eRESULT::FAIL;

			// - connective를 reset한다.
			this->m_pconnectable_notify = nullptr;
		}
	}
	else
	{
		// - 접속에 성공했으므로 재접속처리를 종료한다.
		this->process_cancel_reconnector();

		// check)
		CGASSERT_ERROR(this->io_context().exist());

		// check)
		CGASSERT_ERROR(this->get_epoll_handle() != INVALID_SOCKET);

		// - attach to socket eexecutor
		auto result = this->io_context()->set_event(get_epoll_handle(), this->m_epoll_event_flag, m_pexecutable_socket);

		// - result를 설정한다.
		this->m_result_connecting = (result == true) ? eRESULT::SUCCESS : eRESULT::FAIL;
	}
#endif

	// statistics) connect/disconnect를 위해 실행중인 Overlapped I/O수를 줄임.
	this->statistics_on_decrease_async_con_dis();
}

void net::io::connect_requestable::Ntcp_reconnect::process_connective_connect( net::io::Iconnectable* /*_pconnectable*/, uintptr_t /*_result*/)
{
	// 1) CReconnector를 제거한다.
	this->process_cancel_reconnector();
}

void net::io::connect_requestable::Ntcp_reconnect::process_connective_disconnect( net::io::Iconnectable* /*_pconnectable*/) noexcept
{
	scoped_lock(m_lockable_reconnector)
	{
		// 1) connective를 reset한다.
		this->m_pconnectable_notify = nullptr;

		// 2) 접속 종료시 m_tick_disconnect시간을 설정한다.
		auto tick_now = chrono::tick::now();
		auto tick_differ_execute = tick_now - this->m_tick_disconnected;

		// check) 마지막 접속시도 시간보다 m_dwReconnectInterval이  
		//        Case 지나지 않았으면 -> 마지막 접속시간+재접속시도간격
		//        Case 지났으면       -> 현재시간+재접속시도간격
		if(tick_differ_execute < this->m_tick_reconnection_interval)
		{
			this->m_tick_disconnected = this->m_tick_disconnected + this->m_tick_reconnection_interval;
		}
		else
		{
			this->m_tick_disconnected = tick_now + this->m_tick_reconnection_interval;
		}
	}

	// 3) Reconnect를 진행한다.
	this->process_request_reconnect();
}

bool net::io::connect_requestable::Ntcp_reconnect::process_wait_reconnect()
{
	scoped_lock_named(this->m_lockable_reconnector, cs)
	{
		// check) 만약에 Disable상태라면 재접속을 하지 않는다.
		RETURN_IF(this->reconnection_enable() == false, true);

		// check) 현재 저버속 시도 중이면 여기서 끝낸다.
		RETURN_IF(this->m_flag_reconnecting == false, false);

		// 1) 접속 종료한 시점 이후의 tick을 구한다.
		auto tick_differ_execute = chrono::tick::now() - this->m_tick_disconnected;

		// check) 접속 종료후 m_tick_reconnection_interval보다 시간이 더 지나지 않았으면 그냥 끝낸다.
		RETURN_IF(tick_differ_execute < this->m_tick_reconnection_interval, false)

		// 2) 재접속 시도 시간을 설정한다.
		m_tick_disconnected	+= this->m_tick_reconnection_interval;

		// 3) Reconnection을 시작했음으로 설정한다.
		this->m_flag_reconnecting = false;

		// 4) on_request_reconnect함수를 호출한다.
		this->on_request_reconnect(nullptr);
	}

	// 5) 기존 socket을 닫는다.
	this->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);

	// declare) 
	net::sockaddr sockaddr_connect;

	// 6) binder를 먼저 얻는다.
	auto pbinder = dynamic_cast<net::io::Ibinder*>(this);
	
	// 7) bind socket
	if(pbinder != nullptr)
	{
		pbinder->bind();
	}
	
	// 8) connect한다.
	auto result = this->process_request_connecting(sockaddr_connect);

	// check) connect에 실패했으면 Reconnecting을 true로 바꿔 놓는다.
	if(result == false)
	{
		scoped_lock(this->m_lockable_reconnector)
		{
			// - Reconnection을 시작했음으로 설정한다.
			this->m_flag_reconnecting = true;

			// - 재시도 시간을 다시 되돌린다.
			this->m_tick_disconnected -= this->m_tick_reconnection_interval;
		}
	}

	// return)
	return	false;
}

result_code net::io::connect_requestable::Ntcp_reconnect::process_start(context& _context)
{
	// check) 
	CGASSERT_ERROR(this->socket_state() == eSOCKET_STATE::CLOSED);

	// declare) 
	START_PARAMETER parameter;

	// 1) parsing socket setting options
	if (_context.type() == eCONTEXT_VALUE_TYPE::BUFFER_)
	{
		parameter = _context.v.value_buffer.front<START_PARAMETER>();
	}
	else
	{
		parameter = get_parameter_connection(_context, false);
	}

	// 2) reconnection by option
	switch(parameter.reconnection_enable)
	{
	// case) enable reconnection
	case	2:
			{
				// - enable reconnection
				this->enable_reconnect();

				// - set reconnection interval
				if (parameter.reconnection_interval > chrono::tick::duration::zero())
				{
					this->m_tick_reconnection_interval = parameter.reconnection_interval;
				}
			}
			break;

	// case) disable reconnection
	case	1:
			{
				this->disable_reconnect();
			}
			break;

	// case) not assigned
	default:
			break;
	}

	// 4) call parent::process_start
	return net::io::connect_requestable::Ntcp::process_start(_context);
}

result_code net::io::connect_requestable::Ntcp_reconnect::process_stop() noexcept
{
	// 1) disable reconnect
	this->disable_reconnect();

	// 2) cancel reconnector and wait util complete
	this->process_cancel_reconnector();

	// 3) process_stopping = closesocket
	return net::io::connect_requestable::Ntcp::process_stop();
}

void net::io::connect_requestable::Ntcp_reconnect::detach_instance() noexcept
{
	net::io::connect_requestable::Ntcp_reconnect::manager_reconnector::detach_instance();
}

