//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                            Server Log Classes                             *
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
// logger_slack (include "cgdk/sdk10/server.log.slack.h")
//
//  - sends the logs to Slack asynchronously
//    1) the callers only queue the logs (Nlogger_async)
//    2) the execution of this logger makes the messages and queues them (never waits)
//    3) the messages are requested one by one (in order) : the next one is requested
//       when the response of the previous one comes (on the io thread)
//
//  - two ways
//    1) bot token      : "token" (xoxb-...) + "channel" -> https://slack.com/api/chat.postMessage
//                        the sub logs of a log are sent to the thread of its message
//    2) incoming webhook : "webhook_url" (https://hooks.slack.com/services/...)
//                        a webhook can not make a thread : the sub logs are appended to the message
//
//  - settings (json)
//    "token" / "token_env"             : the bot token (or the name of the environment variable having it)
//    "channel" / "channel_env"         : the channel ("#alerts" or the id) for the bot token
//    "webhook_url" / "webhook_url_env" : the url of the incoming webhook (or the environment variable)
//    "types"                           : the log types sent : ["fatal", "error", "warning", "info", "progress",
//                                        "debug", "user", "system"] or "fatal,error" (default ["fatal", "error"])
//    "title"                           : the first line of a message (default : the name of this logger)
//    "show_time"                       : true : "[time] [TYPE] text" / false : "[TYPE] text" (default true)
//    "ansi"                            : the ansi escape sequences (sgr) in the logs (default "format")
//                                        "format" : converted to the slack format (bold *, italic _, strike ~)
//                                                   and the color bar of each log by its type (as logger_console)
//                                                   (slack shows neither html nor the color of a text)
//                                        "raw"    : sent as they are
//                                        "strip"  : removed
//    "max_message_length"              : a message is split by it (default 3000, 500 ~ 39000)
//    "max_queue"                       : the messages waiting to be sent (default 1000 : more are dropped)
//    "timeout"                         : the timeout of a request in ms (default 10000)
//    "filter"                          : the filter of every logger (optional)
//
//-----------------------------------------------------------------------------
class logger_slack : public Nlogger_async
{
public:
			enum class eANSI : uint32_t
			{
				format = 0,		// - converted to the slack format (and the color bar)
				raw = 1,		// - as it is
				strip = 2		// - removed
			};

			struct INITIALIZE_PARAMETER
			{
				std::string_view			token;			// - bot token (with channel)
				std::string_view			channel;
				std::string_view			webhook_url;	// - or incoming webhook
				std::string_view			title;
				std::string_view			types;			// - "fatal,error" (empty : default)
				bool						is_show_time{ true };
				eANSI						ansi{ eANSI::format };
				size_t						max_message_length{ 3000 };
			};

			logger_slack();
	virtual ~logger_slack() noexcept;

			result_code			initialize() { return this->object::Ninitializable::initialize(); }
	virtual	result_code			initialize(context& _context) override { return this->object::Ninitializable::initialize(_context); }
			result_code			initialize(const INITIALIZE_PARAMETER& _initialize_param);
			result_code			initialize(const std::string_view _token, const std::string_view _channel, const std::string_view _title = std::string_view());

	[[nodiscard]] size_t		get_sent_count() const noexcept { return this->m_count_sent.load(std::memory_order_relaxed); }
	[[nodiscard]] size_t		get_failed_count() const noexcept { return this->m_count_failed.load(std::memory_order_relaxed); }
	[[nodiscard]] size_t		get_dropped_count() const noexcept { return this->m_count_dropped.load(std::memory_order_relaxed); }

	// - the slack text of a text (utf-8) : '&', '<', '>' escaped and the ansi escape sequences processed by _ansi
	//   (_style : the sgr style carried to the next text (bold 1, italic 2, strike 4))
	static	void				append_slack_text(std::string& _out, std::string_view _text, eANSI _ansi, uint32_t& _style);
	static	std::string			to_slack_text(std::string_view _text, eANSI _ansi = eANSI::format) { std::string out; uint32_t style = 0; append_slack_text(out, _text, _ansi, style); return out; }

protected:
	// - a part of a message (a log and its sub logs) : the color bar is by its type
			struct sSEGMENT
			{
				std::string_view	color;
				std::string			text;
			};

	// - a message waiting to be sent
			struct sPOST
			{
				std::vector<sSEGMENT>			vector_segment;
				std::shared_ptr<std::string>	pthread_ts;		// - (sub logs) sent to the thread of this ts (empty : a message)
				std::shared_ptr<std::string>	pts_result;		// - (a log with sub logs) its ts is set here
			};

	virtual	void				process_log_batch(std::vector<object_ptr<LOG_RECORD>>& _vector_plog_record) override;
	virtual result_code			_process_initializing(context& _context) override;
	virtual result_code			_process_destroying() noexcept override;

			void				_append_record(std::string& _out, const LOG_RECORD& _record, size_t _depth) const;
			void				_append_sub_records(std::string& _out, const LOG_RECORD& _record, size_t _depth) const;
			std::string_view	_get_color(const LOG_RECORD& _record) const noexcept;
			bool				_is_sent(const LOG_RECORD& _record) const noexcept;

	// - the segments -> the posts (split by m_max_message_length) -> queued
			void				_queue_segments(std::vector<sSEGMENT>&& _vector_segment, const std::shared_ptr<std::string>& _pthread_ts, const std::shared_ptr<std::string>& _pts_result);

	// - sending (one at a time)
			void				_process_send_next();
			bool				_process_request(const std::shared_ptr<sPOST>& _ppost, uint64_t _id);
			void				_process_on_response(uint64_t _id, bool _is_success, const std::string& _body);
			void				_process_check_timeout();
			std::string			_make_body(const sPOST& _post) const;

			std::string			m_host;
			std::string			m_path;
			std::string			m_token;
			std::string			m_channel;
			std::string			m_title;
			uint32_t			m_mask_types{ 0 };
			bool				m_is_show_time{ true };
			eANSI				m_ansi{ eANSI::format };
			size_t				m_max_message_length{ 3000 };
			size_t				m_max_queue{ 1000 };
			std::chrono::milliseconds m_timeout{ 10000 };

	// - the queue of the messages (m_lockable_send)
			std::mutex			m_lockable_send;
			std::condition_variable m_cv_send;
			std::deque<std::shared_ptr<sPOST>> m_queue_post;
			std::shared_ptr<sPOST> m_ppost_sending;			// - the message in flight
			uint64_t			m_id_request{ 0 };			// - the id of the request in flight (a late response of a timed out one is ignored)
			std::chrono::steady_clock::time_point m_time_request;
			bool				m_is_send_failed{ false };	// - the error is logged only when the sending fails first

			std::atomic<size_t>	m_count_sent{ 0 };
			std::atomic<size_t>	m_count_failed{ 0 };
			std::atomic<size_t>	m_count_dropped{ 0 };
};


}
