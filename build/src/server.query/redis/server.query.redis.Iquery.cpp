#include "pch.h"
#include <charconv>
#include <cstring>
#include <span>

using namespace CGDK;


query::redis::Iquery::~Iquery()			
{
}

void query::redis::Iquery::wait()
{
	SCOPED_LOCK_NAMED(this->m_lockable_state_query, cs)
	{
		// check)
		RETURN_IF(this->m_state_query != eOBJECT_STATE::REQUESTING,);

		// 1) wait
		this->m_cv_complete.wait(cs);
	}
}

void query::redis::Iquery::process_set_session(session* _psession)
{
	SCOPED_LOCK(this->m_lockable_state_query)
	{
		// check) 같은 session이면 여기서 끝낸다.
		RETURN_IF(this->m_psession == _psession);

		// check) 
		CGASSERT_ERROR(this->m_state_query == eOBJECT_STATE::NONE);

		// 1) 기존 session이 있으면 그걸 먼저 해제해준다.
		if (this->m_psession.exist())
			this->m_psession->release_query_reference();

		// 2) 새로운 session을 설정한다.
		this->m_psession = _psession;

		// 3) set state
		this->m_state_query = eOBJECT_STATE::READY;

		// 4) 새로 설정된 SEssion의 QueryReference 카운트를 증가시킨다.
		if (_psession != nullptr)
		{
			_psession->add_query_reference();
		}
	}
}

void query::redis::Iquery::close_query() noexcept
{
	SCOPED_LOCK(this->m_lockable_state_query)
	{
		// check) 
		RETURN_IF(this->m_state_query != eOBJECT_STATE::PROCESSED && this->m_state_query != eOBJECT_STATE::REQUESTING);

		// 1) reset
		this->m_buffer_query.clear();
		this->m_result.reset();

		// 2) set 
		this->m_state_query = eOBJECT_STATE::READY;
	}
}

void query::redis::Iquery::process_response()
{
	SCOPED_LOCK(this->m_lockable_state_query)
	{
		// check)
		RETURN_IF(this->m_state_query!= eOBJECT_STATE::REQUESTING);

		// 2) set state
		this->m_state_query = eOBJECT_STATE::PROCESSED;

		// 3) requesting
		this->m_cv_complete.notify_one();
	}

	// - coroutine) resume the coroutine awaiting this request
	this->_process_on_complete();
}

void query::redis::Iquery::process_execute_query()
{
	// 1) set query state
	this->m_state_query = eOBJECT_STATE::PROCESSED;
}

void query::redis::Iquery::process_reset_session() noexcept
{
	SCOPED_LOCK(this->m_lockable_state_query)
	{
		// check) session이 없으면 그냥 여기서 끝낸다.
		RETURN_IF(this->m_psession.empty());

		// 1) close query
		this->close_query();

		// 2) session의 Query Reference를 줄인다.
		this->m_psession->release_query_reference();

		// 3) session 리셋
		this->m_psession.reset();

		// 4) set state
		this->m_state_query = eOBJECT_STATE::NONE;
	}
	
	// 5) Notify하기
	this->_process_notify_to_requestor();
}

void query::redis::Iquery::_process_notify_to_requestor() noexcept
{
	// check) requestor가 없으면 끝낸다.
	RETURN_IF(this->m_prequestor.empty())

	// 1) ProcessCompleteQuery를 호출한다.
	try
	{
		this->m_prequestor->process_notify_complete_query(this);
	}
	catch (...)
	{
	}
		
	// 2) reset requestor
	this->m_prequestor.reset();
}

shared_buffer generate_redis_command(std::string_view _command)
{
	// - the arguments are separated by blanks (' ', '\t'). the whole '_command' is read (no length limit, no '\0' end)
	//   (it was written to a fixed 4KB buffer : a longer command was cut at 4096 characters and written past the buffer)
	const auto is_blank = [](char _c) noexcept { return _c == ' ' || _c == '\t'; };
	const auto count_digits = [](std::size_t _value) noexcept { std::size_t n = 1; while (_value >= 10) { _value /= 10; ++n; } return n; };
	const auto write_number = [](char* _pos, std::size_t _value) noexcept { return std::to_chars(_pos, _pos + 20, _value).ptr; };

	// 1) count the arguments and the bytes ('$<length>\r\n<argument>\r\n' each)
	std::size_t count = 0;
	std::size_t size_arguments = 0;
	for (std::size_t pos = 0; pos < _command.size();)
	{
		while (pos < _command.size() && is_blank(_command[pos]))
			++pos;
		if (pos == _command.size())
			break;

		const auto pos_begin = pos;
		while (pos < _command.size() && !is_blank(_command[pos]))
			++pos;

		const auto length = pos - pos_begin;
		size_arguments += 1 + count_digits(length) + 2 + length + 2;
		++count;
	}

	// check) no argument
	THROW_IF(count == 0, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, std::string_view("redis command has no argument")));

	// 2) alloc ('*<count>\r\n' + the arguments)
	const auto size_command = 1 + count_digits(count) + 2 + size_arguments;
	auto buf_command = alloc_shared_buffer(size_command);
	char* str_now = buf_command.data<char>();

	// 3) write head
	*(str_now++) = '*';
	str_now = write_number(str_now, count);
	*(str_now++) = '\r';
	*(str_now++) = '\n';

	// 4) write arguments
	for (std::size_t pos = 0; pos < _command.size();)
	{
		while (pos < _command.size() && is_blank(_command[pos]))
			++pos;
		if (pos == _command.size())
			break;

		const auto pos_begin = pos;
		while (pos < _command.size() && !is_blank(_command[pos]))
			++pos;

		const auto length = pos - pos_begin;
		*(str_now++) = '$';
		str_now = write_number(str_now, length);
		*(str_now++) = '\r';
		*(str_now++) = '\n';
		memcpy(str_now, _command.data() + pos_begin, length);
		str_now += length;
		*(str_now++) = '\r';
		*(str_now++) = '\n';
	}

	// check)
	CGASSERT_ERROR(static_cast<std::size_t>(str_now - buf_command.data<char>()) == size_command);

	// 5) set size
	buf_command.add_size(size_command);

	// return)
	return buf_command;
}

void query::redis::Iquery::execute_query_sync(std::string_view _command)
{
	// check) _pquery must not be nullptr
	CGASSERT_ERROR(_command.empty() == false);

	// check) throw exception if _pquery is nullptr
	THROW_IF(_command.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, std::string_view("parameter '_command' is nullptr")));

	// 1) process execute
	_process_execute_query(_command);
}

void query::redis::Iquery::_process_execute_query(std::string_view _command)
{
	// check) _pquery must not be nullptr
	CGASSERT_ERROR(_command.empty() == false);

	// 1) generate redis command
	auto buf_query = generate_redis_command(_command);

	SCOPED_LOCK_NAMED(this->m_lockable_state_query, cs)
	{
		// check) 
		CGASSERT_ERROR(this->m_state_query == eOBJECT_STATE::READY);

		// check) must be prepare
		THROW_IF(this->m_state_query != eOBJECT_STATE::READY, throwable::failure(eRESULT::NOT_READY));

		// 2) set query
		this->m_buffer_query = buf_query;

		try
		{
			// 3) ...
			this->m_state_query = eOBJECT_STATE::REQUESTING;

			// declare) 
			CGDK::query::redis::session::REQUEST_QUERY request_info;
			request_info.pquery = this;
			request_info.buf_request = std::move(buf_query);

			// notify) thread block
			scoped_thread_block thread_block;

			// 4) query를 요구한다.
			this->m_psession->execute_query(std::move(request_info));

			// - wait for the response (the state is changed by process_response())
			while (this->m_state_query == eOBJECT_STATE::REQUESTING)
				this->m_cv_complete.wait(cs);
		}
		catch (throwable::failure& /*_e*/)
		{
			// - set state (roll back) 
			this->m_state_query = eOBJECT_STATE::READY;

			// reraise) 
			throw;
		}
		catch (...)
		{
			// - set state (roll back) 
			this->m_state_query = eOBJECT_STATE::READY;

			// reraise) 
			throw;
		}
	}
}

void query::redis::Iquery::post_execute_query(std::string&& _command, std::function<void()>&& _response)
{
	// check) _pquery must not be nullptr
	CGASSERT_ERROR(_command.empty() == false);

	// check) throw exception if _pquery is nullptr
	THROW_IF(_command.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, std::string_view("parameter '_command' is nullptr")));

	// 1) post execute query
	this->_process_post_execute_query(std::move(_command), std::move(_response));
}

void query::redis::Iquery::_process_post_execute_query(std::string&& _command, std::function<void()>&& _response)
{
	// check) _pquery must not be nullptr
	CGASSERT_ERROR(_command.empty() == false);

	// 1) generate redis command
	auto buf_query = generate_redis_command(_command);

	SCOPED_LOCK_NAMED(this->m_lockable_state_query, cs)
	{
		// check) 
		CGASSERT_ERROR(this->m_state_query >= eOBJECT_STATE::READY);

		// check) must be prepare
		THROW_IF(this->m_state_query < eOBJECT_STATE::READY, throwable::failure(eRESULT::NOT_READY));

		// 1) query command를 작성한다.
		if (this->m_state_query == eOBJECT_STATE::READY)
		{
			// - query string
			this->m_buffer_query = buf_query;

			// - set state
			this->m_state_query = eOBJECT_STATE::REQUESTING;
		}

		try
		{
			// 2) set request query
			CGDK::query::redis::session::REQUEST_QUERY request_info;
			request_info.pquery = this;
			request_info.buf_request = std::move(buf_query);
			request_info.response = std::move(_response);

			// - no scoped_thread_block : the post does not wait for the response. (it flushed the deferred sends : a send system
			//   call per command, and the thread block of the executor per command. the sends of the posts are deferred now like
			//   any send : auto_scope in an executable, send_defer::scope out of it)

			// 3) query를 요구한다.
			this->m_psession->execute_query(std::move(request_info));
		}
		catch (...)
		{
			// - rollback state
			this->m_state_query = eOBJECT_STATE::READY;

			// reraise)
			throw;
		}
	}
}

shared_buffer generate_redis_batch(std::span<const std::string_view> commands)
{
	const auto blank = [](char c) { return c == ' ' || c == '\t'; };
	const auto digits = [](std::size_t n) { std::size_t d = 1; while (n >= 10) { n /= 10; ++d; } return d; };
	const auto add = [](std::size_t& total, std::size_t n) {
		THROW_IF(n > (std::numeric_limits<std::size_t>::max)() - total, throwable::failure(eRESULT::OVERFLOW_ERROR));
		total += n;
	};
	THROW_IF(commands.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT));
	std::size_t bytes = 0;
	for (auto command : commands)
	{
		std::size_t count = 0;
		for (std::size_t pos = 0; pos < command.size();)
		{
			while (pos < command.size() && blank(command[pos])) ++pos;
			if (pos == command.size()) break;
			const auto start = pos;
			while (pos < command.size() && !blank(command[pos])) ++pos;
			const auto length = pos - start;
			add(bytes, length);
			add(bytes, digits(length) + 5);
			++count;
		}
		THROW_IF(count == 0, throwable::failure(eRESULT::INVALID_ARGUMENT));
		add(bytes, digits(count) + 3);
	}
	auto buffer = alloc_shared_buffer(bytes);
	char* out = buffer.data<char>();
	const auto number = [&out, end = buffer.data<char>() + bytes](std::size_t n) {
		out = std::to_chars(out, end, n).ptr;
	};
	for (auto command : commands)
	{
		std::size_t count = 0;
		bool in_argument = false;
		for (char c : command)
		{
			if (blank(c)) in_argument = false;
			else if (!in_argument) { ++count; in_argument = true; }
		}
		*out++ = '*'; number(count); *out++ = '\r'; *out++ = '\n';
		for (std::size_t pos = 0; pos < command.size();)
		{
			while (pos < command.size() && blank(command[pos])) ++pos;
			if (pos == command.size()) break;
			const auto start = pos;
			while (pos < command.size() && !blank(command[pos])) ++pos;
			const auto length = pos - start;
			*out++ = '$'; number(length); *out++ = '\r'; *out++ = '\n';
			std::memcpy(out, command.data() + start, length); out += length;
			*out++ = '\r'; *out++ = '\n';
		}
	}
	CGASSERT_ERROR(static_cast<std::size_t>(out - buffer.data<char>()) == bytes);
	buffer.add_size(bytes);
	return buffer;
}