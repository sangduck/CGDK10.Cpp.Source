//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*               example : emergency acceptor and the signals                *
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
//
//  1) a normal echo server on port 20000 (served by the system_executor)
//
//  2) net::emergency::manager handles the signals of the process
//     - shutdown signal  (windows: ctrl+c / linux: SIGTERM, SIGINT)   : the main loop ends, everything is stopped normally
//     - emergency signal (windows: ctrl+break / linux: SIGUSR1)       : the emergency acceptor is created on port 20001
//                                                                       (its own executor, an executor per connected socket)
//     - terminate signal (windows: raise(SIGTERM) / linux: SIGUSR2)   : a dump, every connection is closed, the process ends
//
//  3) the emergency socket (port 20001) takes text commands : [uint32 length][text]
//     - "status"           the state of the acceptors and the executors
//     - "jam"              blocks every thread of the system_executor for 20s (the normal echo stops, the emergency channel works on)
//     - "stuck"            blocks the thread of this emergency socket for 20s (the other emergency sockets work on)
//     - "throw"            throws an exception in the handler (this socket is closed, nothing else dies)
//     - "emergency" / "shutdown" / "terminate"                 : net::emergency::manager::trigger()
//     - "raise_emergency" / "raise_shutdown" / "raise_terminate" : std::raise() of the default signal (the real signal handler)
//     - anything else      echoed back
//
//  4) keys : '1' emergency  '2' shutdown  '3' terminate  'j' jam  's' status  ESC exit
//
//*****************************************************************************
#include "cgdk/sdk10/net.socket.h"

#include <iostream>
#include <sstream>
#include <csignal>
#if defined(_WIN32)
	#include <conio.h>
	#include <io.h>
#else
	#include <unistd.h>
#endif

using namespace CGDK;
using namespace std::literals;

namespace
{
	std::atomic<bool> g_is_shutdown{ false };
	std::atomic<int>  g_count_jam{ 0 };

	constexpr uint16_t PORT_NORMAL = 20000;
	constexpr uint16_t PORT_EMERGENCY = 20001;
	constexpr auto DURATION_JAM = std::chrono::seconds(20);
	constexpr auto DURATION_STUCK = std::chrono::seconds(20);

	// - the text of a message : [uint32 length][text]
	std::string_view _text_of(const sMESSAGE_NETWORK& _msg)
	{
		const auto& buf = _msg.buf_message;
		RETURN_IF(buf.size() <= sizeof(uint32_t), std::string_view());
		return std::string_view(reinterpret_cast<const char*>(buf.data()) + sizeof(uint32_t), buf.size() - sizeof(uint32_t));
	}

	// - a message of a text
	shared_buffer _message_of(std::string_view _text)
	{
		auto buf = alloc_shared_buffer(sizeof(uint32_t) + _text.size());
		buf.append<uint32_t>(static_cast<uint32_t>(sizeof(uint32_t) + _text.size()));
		buf.append_text(_text);
		return buf;
	}

	bool _is_console_input()
	{
	#if defined(_WIN32)
		return _isatty(_fileno(stdin)) != 0;
	#else
		return isatty(fileno(stdin)) != 0;
	#endif
	}

	int _default_signal(net::emergency::manager::eSIGNAL _signal)
	{
		net::emergency::manager::SETTING setting;
		switch (_signal)
		{
		case net::emergency::manager::eSIGNAL::SHUTDOWN:	return setting.signals_shutdown.empty() ? 0 : setting.signals_shutdown.front();
		case net::emergency::manager::eSIGNAL::EMERGENCY:	return setting.signals_emergency.empty() ? 0 : setting.signals_emergency.front();
		case net::emergency::manager::eSIGNAL::TERMINATE:	return setting.signals_terminate.empty() ? 0 : setting.signals_terminate.front();
		default:											return 0;
		}
	}
}

//-----------------------------------------------------------------------------
// a logger writing every log line to stdout (the lines of the library : net::emergency::manager ...)
//-----------------------------------------------------------------------------
class logger_stdout : public Ilogger, public Idisposable::Ndelete	// - deleted when the last reference (the default logger) is released
{
	virtual void process_log(object_ptr<LOG_RECORD>&& _plog_record) override
	{
		RETURN_IF(_plog_record.empty() || _plog_record->buf_message.empty());

		// 1) text
		auto count_line = _plog_record->line_count();
		auto buf = alloc_shared_buffer(LOG_RECORD::get_buffer_size(_plog_record->buf_message, _plog_record->char_encoding, eCHARACTER_SET::UTF8) + 128 + count_line * 128);

		LOG_RECORD::append_text_date(buf, _plog_record->time_occurred, eCHARACTER_SET::UTF8);

		size_t pos = sizeof(COUNT_T);
		for (; count_line != 0; --count_line)
		{
			pos = _plog_record->get_log_message(buf, eCHARACTER_SET::UTF8, pos);
			buf.append<char>('\n');
			if (count_line > 1)
				buf.append_text("                      "sv);
		}

		// 2) write
		std::lock_guard lock(this->m_lockable);
		std::cout.write(buf.data(), buf.size());
		std::cout.flush();
	}

	std::mutex m_lockable;
};

// - blocks every thread of the system_executor for a while (the jammed server)
void jam_system_executor()
{
	// - the deadlock observer must not dump/terminate the process for this test
	api::manager_thread::instance()->deadlock_dump(false);
	api::manager_thread::instance()->deadlock_terminate(false);

	auto count_thread = system_executor::thread_count();

	LOG_WARNING << "  ! jam : "sv << count_thread << " threads of the system executor are blocked for "sv << DURATION_JAM.count() << "s"sv;

	for (std::size_t i = 0; i < count_thread; ++i)
	{
		system_executor::post(make_executable([]()
		{
			++g_count_jam;
			api::sleep_for(DURATION_JAM);
			--g_count_jam;
		}));
	}
}

//-----------------------------------------------------------------------------
// the normal socket : echo (the system_executor)
//-----------------------------------------------------------------------------
class socket_echo : public net::socket::tcp_buffered<>
{
	virtual result_code on_message(sMESSAGE_NETWORK& _msg) override
	{
		this->send(_msg.buf_message);
		return eRESULT::DONE;
	}
};

own_ptr<net::acceptor<socket_echo>> g_pacceptor;

std::string status_string()
{
	std::ostringstream out;

	// 1) normal
	out << "normal(" << PORT_NORMAL << "): sockets=" << (g_pacceptor.exist() ? g_pacceptor->count() : 0)
		<< ", system executor threads=" << system_executor::thread_count() << " (jammed:" << g_count_jam.load() << ")";

	// 2) emergency
	auto pacceptor_emergency = net::emergency::manager::acceptor();

	if (pacceptor_emergency.empty())
	{
		out << " | emergency(" << PORT_EMERGENCY << "): not created";
	}
	else
	{
		auto pexecutor = pacceptor_emergency->own_executor();

		out << " | emergency(" << PORT_EMERGENCY << "): " << (pacceptor_emergency->is_listening() ? "listening" : "NOT listening")
			<< ", sockets=" << pacceptor_emergency->count()
			<< ", socket executors=" << pacceptor_emergency->socket_executor_count()
			<< ", executor threads=" << (pexecutor.exist() ? pexecutor->existing_thread_count() : 0);
	}

	return out.str();
}

//-----------------------------------------------------------------------------
// the emergency socket : commands (its own executor)
//-----------------------------------------------------------------------------
class socket_emergency : public net::socket::tcp_buffered<>
{
	virtual void on_connect() override
	{
		LOG_WARNING << "  ! [emergency] connected "sv << to_string(this->remote_endpoint()) << " (executor:'"sv << (this->io_context().exist() ? dynamic_cast<object::Inameable*>(this->io_context().get())->name() : std::string()) << "')"sv;
	}

	virtual void on_disconnect(uint64_t _disconnect_reason) override
	{
		LOG_WARNING << "  ! [emergency] disconnected "sv << to_string(this->remote_endpoint()) << " (reason:"sv << _disconnect_reason << ")"sv;
	}

	virtual result_code on_message(sMESSAGE_NETWORK& _msg) override
	{
		auto command = _text_of(_msg);

		LOG_INFO << "  - [emergency] command '"sv << command << "' from "sv << to_string(this->remote_endpoint());

		if (command == "status"sv)
		{
			this->send(_message_of(status_string()));
		}
		else if (command == "jam"sv)
		{
			jam_system_executor();
			this->send(_message_of("jammed"sv));
		}
		else if (command == "stuck"sv)
		{
			this->send(_message_of("stuck"sv));
			api::sleep_for(DURATION_STUCK);
			this->send(_message_of("unstuck"sv));
		}
		else if (command == "throw"sv)
		{
			throw std::runtime_error("test exception on the emergency socket");
		}
		else if (command == "emergency"sv)
		{
			net::emergency::manager::trigger(net::emergency::manager::eSIGNAL::EMERGENCY);
			this->send(_message_of("triggered"sv));
		}
		else if (command == "shutdown"sv)
		{
			this->send(_message_of("triggered"sv));
			net::emergency::manager::trigger(net::emergency::manager::eSIGNAL::SHUTDOWN);
		}
		else if (command == "terminate"sv)
		{
			this->send(_message_of("triggered"sv));
			net::emergency::manager::trigger(net::emergency::manager::eSIGNAL::TERMINATE);
		}
		else if (command == "raise_emergency"sv || command == "raise_shutdown"sv || command == "raise_terminate"sv)
		{
			auto signal_role = (command == "raise_emergency"sv) ? net::emergency::manager::eSIGNAL::EMERGENCY : (command == "raise_shutdown"sv) ? net::emergency::manager::eSIGNAL::SHUTDOWN : net::emergency::manager::eSIGNAL::TERMINATE;
			auto signal_number = _default_signal(signal_role);

			this->send(_message_of(CGDK::stream_string("raise ", signal_number)));

			if (signal_number != 0)
				std::raise(signal_number);
		}
		else
		{
			this->send(_msg.buf_message);
		}

		return eRESULT::DONE;
	}
};

//-----------------------------------------------------------------------------
// main
//-----------------------------------------------------------------------------
int main()
{
	// 0) the logs of the library to stdout
	set_default_logger(object_ptr<Ilogger>(new logger_stdout()));

	std::cout << " ---------------------------------------------------\n"sv
			  << "    CGCII emergency acceptor / signal example\n"sv
			  << "\n"sv
			  << "    normal echo    : port "sv << PORT_NORMAL << "\n"sv
			  << "    emergency      : port "sv << PORT_EMERGENCY << " (created on the emergency signal)\n"sv
			  << "\n"sv
			  << "    keys : '1' emergency  '2' shutdown  '3' terminate  'j' jam  's' status  ESC exit\n"sv
			  << " ---------------------------------------------------\n"sv;

	// 1) the emergency manager : the signals, the emergency acceptor (created on the emergency signal)
	net::emergency::manager::SETTING setting;

	setting.fn_shutdown = []() { g_is_shutdown = true; };	// - the main loop ends : the normal termination below

	net::emergency::manager::install<socket_emergency>({ { net::ip::tcp::v6(), PORT_EMERGENCY } }, std::move(setting));

	// 2) the normal acceptor (the system_executor)
	g_pacceptor = make_own<net::acceptor<socket_echo>>();
	g_pacceptor->start({ { net::ip::tcp::v6(), PORT_NORMAL } });

	// 3) loop
	const bool is_console = _is_console_input();
	auto tick_status = chrono::tick::now();

	while (g_is_shutdown == false)
	{
		api::sleep_for(std::chrono::milliseconds(100));

		// - status
		if ((chrono::tick::now() - tick_status) >= std::chrono::seconds(5))
		{
			tick_status = chrono::tick::now();
			std::cout << "  . " << status_string() << "\n";
		}

		// - keys
		if (is_console && _kbhit())
		{
			int ch = _getch();

			switch (ch)
			{
			case 27:	g_is_shutdown = true; break;
			case '1':	net::emergency::manager::trigger(net::emergency::manager::eSIGNAL::EMERGENCY); break;
			case '2':	net::emergency::manager::trigger(net::emergency::manager::eSIGNAL::SHUTDOWN); break;
			case '3':	net::emergency::manager::trigger(net::emergency::manager::eSIGNAL::TERMINATE); break;
			case 'j':	jam_system_executor(); break;
			case 's':	std::cout << "  . " << status_string() << "\n"; break;
		#if defined(_WIN32)
			case 0xe0:	_getch(); break;
		#endif
			default:	break;
			}
		}
	}

	// 4) the normal termination : the normal acceptor, then the system_executor (the emergency acceptor is stopped there)
	std::cout << "  . shutdown : "sv << status_string() << "\n"sv;

	g_pacceptor->stop();
	g_pacceptor.destroy();

	system_executor::destroy_instance();

	std::cout << "  . terminated normally\n"sv;

	// 5) the logger
	reset_default_logger();

	// return)
	return 0;
}
