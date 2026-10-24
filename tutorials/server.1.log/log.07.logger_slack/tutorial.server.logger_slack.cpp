#include "cgdk/sdk10/server.log.h"
#include "cgdk/sdk10/server.log.slack.h"	// - logger_slack ("slack" is registered to create_logger)

#include <iostream>

using namespace CGDK;

// ------------------------------------------------------------------
// tutorial) logger_slack
//
//  - sends the logs to Slack asynchronously
//    (the callers never wait : the messages are sent one by one in order on the io thread)
//
//  - two ways (set the environment variables before running)
//    1) bot token        : SLACK_BOT_TOKEN (xoxb-...) and SLACK_CHANNEL (#alerts or the id)
//                          the sub logs of a log are sent to the thread of its message
//    2) incoming webhook : SLACK_WEBHOOK_URL (https://hooks.slack.com/services/...)
//                          the sub logs are appended to the message
//
//  - the secrets (token, webhook url) are better not to be written in the setting file
//    ("token_env", "channel_env", "webhook_url_env" : the names of the environment variables)
//
// ------------------------------------------------------------------
// - the setting (json) : it can be a logger of a service setting ("type": "slack")
//   or a child of logger_hub too
constexpr std::string_view SETTING_SLACK = R"json(
{
	"token_env"         : "SLACK_BOT_TOKEN",
	"channel_env"       : "SLACK_CHANNEL",
	"webhook_url_env"   : "SLACK_WEBHOOK_URL",
	"types"             : ["fatal", "error", "warning"],
	"title"             : "*tutorial.logger_slack*",
	"show_time"         : true,
	"ansi"              : "format",
	"max_message_length": 3000,
	"max_queue"         : 1000,
	"timeout"           : 10000
}
)json";

int main()
{
	// ------------------------------------------------------------------
	// 1. create logger_slack
	// ------------------------------------------------------------------
	// 1) by the class (or create_logger("slack") : "slack" is registered by server.log.slack.h)
	auto plogger = make_own<logger_slack>();
	plogger->name("tutorial.logger_slack");

	// 2) initialize by the setting (json)
	//    - the token or the webhook url is needed : throws if none
	try
	{
		context setting;
		setting.from_json(SETTING_SLACK);

		plogger->initialize(setting);
	}
	catch (...)
	{
		std::cout << "fail to initialize logger_slack"sv << std::endl;
		std::cout << "  set SLACK_BOT_TOKEN and SLACK_CHANNEL, or SLACK_WEBHOOK_URL"sv << std::endl;
		return 0;
	}

	// - the same by INITIALIZE_PARAMETER
	//   logger_slack::INITIALIZE_PARAMETER param;
	//   param.webhook_url  = "https://hooks.slack.com/services/...";
	//   param.types        = "fatal,error,warning";
	//   param.is_show_time = false;
	//   param.ansi         = logger_slack::eANSI::format;
	//   plogger->initialize(param);

	std::cout << "logger_slack initialized"sv << std::endl;


	// ------------------------------------------------------------------
	// 2. logs
	//    - "types" : ["fatal", "error", "warning"] -> info isn't sent
	//    - each log is sent with the color bar of its type ("ansi": "format")
	// ------------------------------------------------------------------
	LOG(plogger, log_type_t::info)		<< "[A-1] INFO log (not sent : not in 'types')"sv;
	LOG(plogger, log_type_t::warning)	<< "[A-2] WARNING log"sv;
	LOG(plogger, log_type_t::error)		<< "[A-3] ERROR log"sv;
	LOG(plogger, log_type_t::fatal)		<< "[A-4] FATAL log"sv;

	// 1) ansi sgr (bold / italic / strike) -> the slack format ("ansi": "format")
	//    the colors of a text are removed (slack can't show them)
	//    "ansi": "raw" sends them as they are, "strip" removes them
	LOG(plogger, log_type_t::error)		<< "[B-1] \x1b[1mbold\x1b[0m \x1b[3mitalic\x1b[0m \x1b[9mstrike\x1b[0m \x1b[1;31mbold red\x1b[0m"sv;

	// 2) multi line
	LOG(plogger, log_type_t::error)		<< "[B-2] first line"sv << log_endl
										<< "[B-3] second line"sv;


	// ------------------------------------------------------------------
	// 3. a log with the sub logs
	//    - bot token : the sub logs are sent to the thread of its message
	//    - webhook   : the sub logs are appended to the message
	// ------------------------------------------------------------------
	{
		auto plog = make_object<LOG_RECORD>();
		plog->type = log_type_t::error;
		plog->level = log_level_t::normal();
		plog->time_occurred = chrono::time::utc::now();
		plog->set_message("[C-1] fail to process the request (see the thread)"sv);

		for (int i = 0; i < 3; ++i)
		{
			auto plog_sub = make_object<LOG_RECORD>();
			plog_sub->type = (i == 2) ? log_type_t::error : log_type_t::info;
			plog_sub->level = log_level_t::normal();
			plog_sub->time_occurred = chrono::time::utc::now();
			plog_sub->set_message(std::string_view((i == 2) ? "[C-2] step 2 : \x1b[1mfailed\x1b[0m" : ((i == 0) ? "[C-2] step 0 : ok" : "[C-2] step 1 : ok")));

			plog->add_sub_log(std::move(plog_sub));
		}

		plogger->trace(std::move(plog));
	}


	// ------------------------------------------------------------------
	// 4. destroy
	//    - waits until the queued messages are sent (up to 3 times of "timeout")
	// ------------------------------------------------------------------
	plogger->destroy();

	std::cout << "sent: "sv << plogger->get_sent_count()
			  << ", failed: "sv << plogger->get_failed_count()
			  << ", dropped: "sv << plogger->get_dropped_count() << std::endl;

	// return)
	return 0;
}
