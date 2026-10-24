//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                             server.query.http                             *
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
// query::http::querier
//
//
// ----------------------------------------------------------------------------
class query::http::querier :
// inherited classes) 
	public						Iquerier,
	public						object::Nnameable,
	public						object::Ninitializable_startable
{
// constructor/destructor)
public:
			querier();
			querier(const std::string_view _name);
	virtual	~querier() noexcept;

// public)
public:
	// 1) open
			result_code			start() { return this->object::Nstartable::start();}
			result_code			start(const context& _context) { return this->object::Nstartable::start(_context);}
	virtual	result_code			start(context& _context) override { return this->object::Nstartable::start(_context);}
			result_code			start(Iexecutor* _pexecutor);

	// 2) execute_query/post_execute_query/Cancel
			//    - execute_query()      : without 'co_await' executed as before. with 'co_await' the coroutine is resumed when the response is received. (execute_result)
			//    - execute_query_sync() : always executed at once (waits for the response)
			template <class TQUERY = query>
			execute_result<object_ptr<TQUERY>> execute_query(const std::string_view _command);
			template <class TQUERY = query>
			object_ptr<TQUERY>	execute_query_sync(const std::string_view _command);
			//    - co_execute_query()      : 이 thread에서 바로 실행한다. (응답까지 블로킹. 'co_await'해도 suspend하지 않는다)
			//    - co_post_execute_query() : 요청을 보내고 응답이 오면 coroutine이 이어진다. ('co_await execute_query()'와 같다. thread는 기다리지 않는다)
			template <class TQUERY = query>
			execute_result<object_ptr<TQUERY>> co_execute_query(const std::string_view _command) { return execute_result<object_ptr<TQUERY>>::executed(this->execute_query_sync<TQUERY>(_command)); }
			template <class TQUERY = query>
			execute_result<object_ptr<TQUERY>> co_post_execute_query(const std::string_view _command);
			template <class TQUERY = query, class TFIRST, class... TARGS>
			object_ptr<TQUERY>	execute_query(const std::string_view _format, TFIRST&& _first, TARGS&&... _args);
	virtual	bool				cancel_query(Iserver_query* _pquery) noexcept;

			template <class TQUERY = query>
			void				post_execute_query(const std::string_view _command, const std::function<void(result_code,TQUERY*)>& _response);
			template <class TQUERY = query>
			void				post_execute_query(const std::string_view _command, std::function<void(result_code,TQUERY*)>&& _response);

	// 3) alloc_query/post_alloc_query
			template <class TQUERY = query>
	[[nodiscard]] object_ptr<TQUERY> alloc_query() { auto pquery = make_object<TQUERY>(); pquery->porcess_set_querier(this); return pquery; }
			template <class TQUERY = query>
			void				post_alloc_query(const std::function<void(TQUERY*)>& _response);
			template <class TQUERY = query>
			void				post_alloc_query(std::function<void(TQUERY*)>&& _response);

	// statistics) 
	[[nodiscard]] static std::size_t message_buffer_min_size() noexcept { return m_s_bytes_message_buffer_min_size;}
	static	void				message_buffer_min_size(std::size_t _size) { m_s_bytes_message_buffer_min_size =_size;}

// constructor/destructor)
protected:
	// 1) initialize/destroy
	virtual result_code			_process_initializing(context& _context) override;
	virtual result_code			_process_initialize(context& _context) override;
	virtual result_code			_process_destroying() noexcept override;
	virtual result_code			_process_destroy() noexcept override;

	// 2) start/stop
	virtual result_code			_process_starting(context& _context) override;
	virtual result_code			_process_start(context& _context) override;
	virtual result_code			_process_stopping() noexcept override;
	virtual result_code			_process_stop() noexcept override;

	// 3) ...
	virtual	void				process_execute_query(Iserver_query* _pquery);
	virtual	void				process_post_execute_query(Iserver_query* _pquery);

			void				_process_execute_query(query* _phttp_request);

	// 4) ...
			std::recursive_mutex m_lockable_query;
			object_ptr<Iexecutor> m_pexecutor;
			bool				m_is_create;

	static	std::size_t			m_s_bytes_message_buffer_min_size;
};

template <class TQUERY>
object_ptr<TQUERY> query::http::querier::execute_query_sync(const std::string_view _command)
{
	// check) _command이 nullptr이면 안 된다.
	CGASSERT_ERROR(_command.empty() == false);

	// check) _command가 nullptr이면 그냥 끝낸다.
	THROW_IF(_command.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, std::string_view("parameter['_command'] is nullptr")));

	// 1) alloc_query
	auto pquery = this->alloc_query<TQUERY>();

	// 2) execute query
	pquery->execute_query_sync(_command);

	// return)
	return pquery;
}

template <class TQUERY>
query::execute_result<object_ptr<TQUERY>> query::http::querier::execute_query(const std::string_view _command)
{
	// 1) out of a coroutine: executed at once (same as before)
	if (!is_deferred_execution())
		return execute_result<object_ptr<TQUERY>>::executed(this->execute_query_sync<TQUERY>(_command));

	// 2) in a coroutine: deferred ('co_await' sends the request and resumes the coroutine when the response is received)
	auto pquerier = object_ptr<querier>(this);

	return execute_result<object_ptr<TQUERY>>::deferred(
		[pquerier, command = std::string(_command)]() { return pquerier->template execute_query_sync<TQUERY>(command); },
		[pquerier, command = std::string(_command)](object_ptr<TQUERY>& _result, std::function<void()>&& _on_complete)
		{
			auto pquery = pquerier->template alloc_query<TQUERY>();
			pquery->_set_function_on_complete(std::move(_on_complete));
			_result = pquery;
			pquery->post_execute_query(command);
		});
}

template <class TQUERY>
query::execute_result<object_ptr<TQUERY>> query::http::querier::co_post_execute_query(const std::string_view _command)
{
	// - 'co_await'하면 요청을 보내고 응답이 오면 coroutine이 이어진다. (execute_query()의 coroutine 경로와 같다. thread는 기다리지 않는다)
	auto pquerier = object_ptr<querier>(this);

	return execute_result<object_ptr<TQUERY>>::deferred(
		[pquerier, command = std::string(_command)]() { return pquerier->template execute_query_sync<TQUERY>(command); },
		[pquerier, command = std::string(_command)](object_ptr<TQUERY>& _result, std::function<void()>&& _on_complete)
		{
			auto pquery = pquerier->template alloc_query<TQUERY>();
			pquery->_set_function_on_complete(std::move(_on_complete));
			_result = pquery;
			pquery->post_execute_query(command);
		});
}

template <class TQUERY, class TFIRST, class... TARGS>
object_ptr<TQUERY> query::http::querier::execute_query(const std::string_view _format, TFIRST&& _first, TARGS&&... _args)
{
	// check) _format이 nullptr이면 안 된다.
	CGASSERT_ERROR(_format.empty() == false);

	// check) _format가 nullptr이면 그냥 끝낸다.
	THROW_IF(_format.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, std::string_view("parameter['_command'] is nullptr")));

	// 1) alloc_query
	auto pquery = this->alloc_query<TQUERY>();

	// 2) execute query
	pquery->post_execute_query(_format, std::forward<TFIRST>(_first), std::forward<TARGS>(_args)...);

	// return)
	return pquery;
}

template <class TQUERY>
void query::http::querier::post_execute_query(const std::string_view _command, const std::function<void(result_code,TQUERY*)>& _response)
{
	// check) _command이 nullptr이면 안 된다.
	CGASSERT_ERROR(_command.empty() == false);

	// check) _command가 nullptr이면 그냥 끝낸다.
	THROW_IF(_command.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, std::string_view("parameter['_command'] is nullptr")));

	// 1) alloc_query
	auto pquery = this->alloc_query<TQUERY>();

	// 2) execute query
	pquery->post_execute_query(_command, _response);
}

template <class TQUERY>
void query::http::querier::post_execute_query(const std::string_view _command, std::function<void(result_code, TQUERY*)>&& _response)
{
	// check) _command이 nullptr이면 안 된다.
	CGASSERT_ERROR(_command.empty() == false);

	// check) _command가 nullptr이면 그냥 끝낸다.
	THROW_IF(_command.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, std::string_view("parameter['_command'] is nullptr")));

	// 1) alloc_query
	auto pquery = this->alloc_query<TQUERY>();

	// 2) execute query
	pquery->post_execute_query(_command, std::move(_response));
}


}