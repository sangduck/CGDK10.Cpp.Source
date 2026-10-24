#include "pch.h"
#include <charconv>

namespace CGDK
{

void net::io::connective::acceptor::Ntcp::process_get_acceptor_parameters(context& _context, START_PARAMETER& parameter)
{
	if (_context.type() == context::value_type_t::buffer_)
	{
		// - get parameter (reload)
		parameter = _context.v.value_buffer.front<START_PARAMETER>();

		// - casting to int64_t
		int64_t prepare_on_start = static_cast<int64_t>(parameter.prepare_on_start);
		int64_t must_prepare = static_cast<int64_t>(parameter.must_prepare);

		// trace)
		if (parameter.enable_progress_log)
		{
			if (prepare_on_start > 0)
				LOG_INFO << "  + parameter['prepare on start']: "sv << prepare_on_start;
			else if (prepare_on_start < 0)
				LOG_INFO << "  + parameter['prepare on start']: 'core' x "sv << prepare_on_start;
			else
				LOG_INFO << "  + parameter['prepare on start']: default"sv;

			// trace)
			if (must_prepare > 0)
				LOG_INFO << "  + parameter['must prepare']: "sv << must_prepare;
			else if (must_prepare < 0)
				LOG_INFO << "  + parameter['must prepare']: 'core' x "sv << must_prepare;
			else
				LOG_INFO << "  + parameter['must prepare']: default"sv;
		}
	}
	else
	{
		// 1) local address
		if (auto& context_find = _context.find("local_endpoint", "local endpoint"); context_find.is_valid())
		{
			// declare) 
			net::ip::tcp::resolver::results_type resolve_result;

			// - get endpoint
			auto listen_endpoint = get_parameter_endpoint(context_find, "local_endpoint"sv, true);

			// - set local endpoint
			resolve_result = net::ip::tcp::resolver().resolve(
				listen_endpoint.str_address,
				listen_endpoint.str_service,
				listen_endpoint.flag
			);

			// check) 
			if (resolve_result.empty())
			{
				// log)
				LOG_ERROR << " error) invalid address or port for bind (address:"sv << listen_endpoint.str_address << ", service:"sv << listen_endpoint.str_service << ")"sv;

				// throw) 
				throw throwable::failure(eRESULT::INVALID_ARGUMENT);
			}

			// - get one
			parameter.local_endpoint = *resolve_result.begin();

			// - set bind_any if str_address is nullptr 
			if (listen_endpoint.str_address.empty())
			{
				parameter.local_endpoint = net::ip::tcp::endpoint{ net::ip::address_v6().any(), parameter.local_endpoint.port() };
			}
		}

		// 2) accept address
		if (auto& context_find = _context.find("accept_endpoint", "accept endpoint"); context_find.is_valid())
		{
			// declare) 
			net::ip::tcp::resolver::results_type resolve_result;

			// - get endpoint
			auto accept_endpoint = get_parameter_endpoint(context_find, "accept_endpoint"sv, true);

			// - set accept endpoint
			resolve_result = net::ip::tcp::resolver().resolve(
				accept_endpoint.str_address,
				accept_endpoint.str_service,
				accept_endpoint.flag
			);

			// check) 
			if (resolve_result.empty())
			{
				// log)
				LOG_ERROR << " error) invalid address or port for bind (address:"sv << accept_endpoint.str_address << ", service:"sv << accept_endpoint.str_service << ")"sv;

				// throw) 
				throw throwable::failure(eRESULT::INVALID_ARGUMENT);
			}

			// - get first one
			parameter.accept_endpoint = *resolve_result.begin();

			// - if str_address is nullptr, set address as public ip in host
			if (parameter.accept_endpoint.address().is_loopback())
			{
				parameter.accept_endpoint.address(get_accept_endpoint().address());
			}

			// - if port is zero, set port as listen port
			if (parameter.accept_endpoint.port() == 0)
			{
				parameter.accept_endpoint.port(parameter.local_endpoint.port());
			}

			// trace)
			EXECUTE_IF(parameter.enable_progress_log, LOG_INFO << "  + parameter['acceptable endpoint']: "sv << to_string(parameter.accept_endpoint) << ":" << parameter.accept_endpoint.port());
		}

		// 4) [Optional] <prepare on start>값 읽기 (default = 0)
		if (auto& context_find = _context.find("prepare_on_start", "prepare on start"); context_find.is_valid())
		{
			// - get_string
			std::string str_find = context_find;

			// - "default"면 default값으로 설정
			if (CGDK::__stricmp(str_find, "default") == 0)
			{
				parameter.prepare_on_start = 0;

				// trace)
				EXECUTE_IF(parameter.enable_progress_log, LOG_INFO << "  + parameter['prepare on start']: 'default'"sv);
			}
			// - 문자열을 integer로 변경해 넣는다.
			else if (str_find.size() != 0 && (str_find[0] == 'x' || str_find[0] == 'X' || str_find[0] == '*'))
			{
				// - get multiply value
				int64_t prepare_on_start = 0;
				std::from_chars(str_find.data() + 1, str_find.data() + str_find.size(), prepare_on_start);

				// - set value
				parameter.prepare_on_start = static_cast<std::size_t>(-prepare_on_start);

				// trace)
				EXECUTE_IF(parameter.enable_progress_log, LOG_INFO << "  + parameter['prepare on start']: 'core' x "sv << prepare_on_start);
			}
			// - 문자열을 integer로 변경해 넣는다.
			else
			{
				// - get value
				std::int64_t prepare_on_start = 0;
				std::from_chars(str_find.data(), str_find.data() + str_find.size(), prepare_on_start);

				// - 
				if (prepare_on_start > 65536)
				{
					// trace)
					LOG_WARNING << "(warn) 'prepare on start' value exceeded 65536.(" << prepare_on_start << ")";
				}

				// set value
				parameter.prepare_on_start = static_cast<std::size_t>(prepare_on_start);

				// trace)
				EXECUTE_IF(parameter.enable_progress_log, LOG_INFO << "  + parameter['prepare on start']: "sv << parameter.prepare_on_start);
			}
		}

		// 5) [Optional] <must prepare>값 읽기 (Default=0)
		if (auto& context_find = _context.find("must_prepare", "must prepare"); context_find.is_valid())
		{
			// - get_string
			std::string str_find = context_find;

			// - to lower case
			auto value = __to_lower(std::string_view(str_find));

			// - "default"면 default값으로 설정
			if (value == "default")
			{
				parameter.must_prepare = 0;

				// trace)
				EXECUTE_IF(parameter.enable_progress_log, LOG_INFO << "  + parameter['must prepare']: 'default'"sv);
			}
			// - 문자열을 integer로 변경해 넣는다.
			else if (str_find.size() != 0 && (str_find[0] == 'x' || str_find[0] == 'X' || str_find[0] == '*'))
			{
				// - get multiply value
				int64_t must_prepare = 0;
				std::from_chars(str_find.data() + 1, str_find.data() + str_find.size(), must_prepare);

				// - set value
				parameter.must_prepare = static_cast<std::size_t>(-must_prepare);

				// trace)
				EXECUTE_IF(parameter.enable_progress_log, LOG_INFO << "  + parameter['must prepare']: 'core' x "sv << must_prepare);
			}
			// - 문자열을 integer로 변경해 넣는다.
			else
			{
				// - get value
				std::int64_t must_prepare = 0;
				std::from_chars(str_find.data(), str_find.data() + str_find.size(), must_prepare);

				// - 
				if (must_prepare > 65536)
				{
					// trace)
					LOG_WARNING << "(warn) 'must prepare' value exceeded 65536.("sv << must_prepare << ")"sv;
				}

				// set value
				parameter.must_prepare = static_cast<std::size_t>(must_prepare);

				// trace)
				EXECUTE_IF(parameter.enable_progress_log, LOG_INFO << "  + parameter['must prepare']: "sv << parameter.must_prepare);
			}
		}

		// 6) reuse address
		if (auto& context_find = _context.find("reuse_address", "reuse address"); context_find.is_valid())
		{
			// - get_string
			std::string str_find = context_find;

			// - to lower case
			auto value = __to_lower(std::string_view(str_find));

			// - "default"면 default값으로 설정
			if (value == "default")
			{
				parameter.reuse_address = true;

				// trace)
				EXECUTE_IF(parameter.enable_progress_log, LOG_INFO << "  + parameter['reuse address']: 'default(true)'"sv);
			}
			// - 문자열을 integer로 변경해 넣는다.
			else if (value == "true")
			{
				parameter.reuse_address = true;

				// trace)
				EXECUTE_IF(parameter.enable_progress_log, LOG_INFO << "  + parameter['reuse address']: true"sv);
			}
			// - 문자열을 integer로 변경해 넣는다.
			else if (value == "false")
			{
				parameter.reuse_address = false;

				// trace)
				EXECUTE_IF(parameter.enable_progress_log, LOG_INFO << "  + parameter['reuse address']: false"sv);
			}
		}

		// 8) 
		if (auto& context_find = _context.find("reallocate_port", "reallocate port", "reallocate_port_at_duplicate_port_error", "reallocate port at duplicate port error"); context_find.is_valid())
		{
			// - get_string
			std::string str_find = context_find;

			// - to lower case
			auto value = __to_lower(std::string_view(str_find));

			// - "default"면 default값으로 설정
			if (value == "default")
			{
				parameter.reallocate_port_at_duplicate_port_error = true;

				// trace)
				EXECUTE_IF(parameter.enable_progress_log, LOG_INFO << "  + parameter['reallocate port']: 'default(true)'"sv);
			}
			// - 문자열을 integer로 변경해 넣는다.
			else if (value == "true")
			{
				parameter.reallocate_port_at_duplicate_port_error = true;

				// trace)
				EXECUTE_IF(parameter.enable_progress_log, LOG_INFO << "  + parameter['reallocate port']: true"sv);
			}
			// - 문자열을 integer로 변경해 넣는다.
			else if (value == "false")
			{
				parameter.reallocate_port_at_duplicate_port_error = false;

				// trace)
				EXECUTE_IF(parameter.enable_progress_log, LOG_INFO << "  + parameter['reallocate port']: false"sv);
			}
		}

		// 9) cryption ("cryption": { "enable": true }로 설정하면 accept한 socket을 PNCODE로 암호송수신한다.)
		//    (socket class가 Icryptable(net::socket::tcp_crypt 등)일 때만 적용된다. client는 서버가 내려주는 type을 따라 자동으로 암호화된다.)
		if (auto& context_find = _context.find("cryption"); context_find.is_valid())
		{
			if (auto& context_enable = context_find.find("enable"); context_enable.is_valid())
			{
				// - get_string
				std::string str_find = context_enable;

				// - to lower case
				auto value = __to_lower(std::string_view(str_find));

				if (value == "true")
				{
					parameter.cryption_type = eCRYPTION_TYPE::PNCODE;

					// trace)
					EXECUTE_IF(parameter.enable_progress_log, LOG_INFO << "  + parameter['cryption']: enable (pncode)"sv);
				}
				else if (value == "false")
				{
					parameter.cryption_type = eCRYPTION_TYPE::NONE;

					// trace)
					EXECUTE_IF(parameter.enable_progress_log, LOG_INFO << "  + parameter['cryption']: disable"sv);
				}
			}
		}
	}

	// - enable_uds ("enable_uds": true/false) : accepts the same host connections through a UDS too (linux)
	if (_context.type() != context::value_type_t::buffer_)
	{
		if (auto& context_find = _context.find("enable_uds", "enable uds"); context_find.is_valid())
		{
			// - get_string
			std::string str_find = context_find;

			// - to lower case
			auto value = __to_lower(std::string_view(str_find));

			// - true/false
			if (value == "true" || value == "1")
				parameter.enable_uds = true;
			else if (value == "false" || value == "0" || value == "default")
				parameter.enable_uds = false;

			// trace)
			EXECUTE_IF(parameter.enable_progress_log, LOG_INFO << "  + parameter['enable_uds']: "sv << (parameter.enable_uds ? "true"sv : "false"sv));
		}

		// - uds_only ("uds_only": true/false) : accepts through the UDS only (linux)
		if (auto& context_find = _context.find("uds_only", "uds only"); context_find.is_valid())
		{
			// - get_string
			std::string str_find = context_find;

			// - to lower case
			auto value = __to_lower(std::string_view(str_find));

			// - true/false
			if (value == "true" || value == "1")
				parameter.uds_only = true;
			else if (value == "false" || value == "0" || value == "default")
				parameter.uds_only = false;

			// trace)
			EXECUTE_IF(parameter.enable_progress_log, LOG_INFO << "  + parameter['uds_only']: "sv << (parameter.uds_only ? "true"sv : "false"sv));
		}

		// - tcp_fastopen ("tcp_fastopen": true/false/<queue length>) : TCP Fast Open (RFC 7413) of the listen socket. the data in the SYN is accepted
		//   (linux : the max pending TFO connections, true = DEFAULT_TCP_FASTOPEN_QUEUE_LENGTH. sysctl net.ipv4.tcp_fastopen & 2 is required too. windows : enable)
		if (auto& context_find = _context.find("tcp_fastopen", "tcp fastopen", "fastopen"); context_find.is_valid())
		{
			// - get_string
			std::string str_find = context_find;

			// - to lower case
			auto value = __to_lower(std::string_view(str_find));

			// - true/false/queue length
			if (value == "true" || value == "enable")
			{
				parameter.tcp_fastopen = DEFAULT_TCP_FASTOPEN_QUEUE_LENGTH;
			}
			else if (value == "false" || value == "disable" || value == "default")
			{
				parameter.tcp_fastopen = 0;
			}
			else
			{
				int64_t queue_length = 0;
				std::from_chars(str_find.data(), str_find.data() + str_find.size(), queue_length);
				parameter.tcp_fastopen = static_cast<int>(std::max<int64_t>(queue_length, 0));
			}

			// trace)
			EXECUTE_IF(parameter.enable_progress_log, LOG_INFO << "  + parameter['tcp_fastopen']: "sv << parameter.tcp_fastopen);
		}

		// - socket_option ("socket_option": { "tcp_nodelay", "tcp_quickack", "busy_poll", "tcp_notsent_lowat" }) : the tcp socket option of the accepted sockets
		if (get_parameter_socket_option(_context, parameter.socket_option, parameter.enable_progress_log))
			parameter.flag_socket_option = 1;

		// - tcp_defer_accept ("tcp_defer_accept": seconds) : the accept wakes up when the first data of the connection arrives (linux)
		if (auto& context_find = _context.find("tcp_defer_accept", "defer_accept"); context_find.is_valid())
		{
			// - get value (seconds)
			std::size_t value = context_find;

			parameter.tcp_defer_accept = static_cast<int>(value);

			// trace)
			EXECUTE_IF(parameter.enable_progress_log, LOG_INFO << "  + parameter['tcp_defer_accept']: "sv << parameter.tcp_defer_accept);
		}

		// - cpu_affinity ("cpu_affinity": true/false) : an accepted socket runs on the io_context of the CPU that received it (linux)
		if (auto& context_find = _context.find("cpu_affinity", "enable_cpu_affinity"); context_find.is_valid())
		{
			// - get_string
			std::string str_find = context_find;

			// - to lower case
			auto value = __to_lower(std::string_view(str_find));

			// - true/false
			if (value == "true" || value == "1")
				parameter.enable_cpu_affinity = true;
			else if (value == "false" || value == "0" || value == "default")
				parameter.enable_cpu_affinity = false;

			// trace)
			EXECUTE_IF(parameter.enable_progress_log, LOG_INFO << "  + parameter['cpu_affinity']: "sv << (parameter.enable_cpu_affinity ? "true"sv : "false"sv));
		}
	}

	// 6) listen이 empty일 경우 default로 설정된다.
	if (parameter.local_endpoint.empty())
	{
		parameter.local_endpoint = end_point_type{ net::ip::address_v6::any(), this->process_get_default_port() };
	}
	// 7) listen이 empty일 경우 default로 설정된다.
	else if (parameter.local_endpoint.port() == 0)
	{
		// - get default port
		auto default_port = this->process_get_default_port();

		// - set default port
		parameter.local_endpoint.port(default_port);

		// log) 
		EXECUTE_IF(parameter.enable_progress_log, LOG_WARNING << "  ! 'listen port' set as default port (loopback, port:"sv << default_port << ")  "sv);
	}

	// check) 
	if (parameter.local_endpoint.port() == 0)
	{
		// log) 
		LOG_ERROR << "   (err ) invalid listen port (port number is '0')"sv;

		// raise)
		throw throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "invalid listen port (port number is '0')"sv);
	}
}


}
