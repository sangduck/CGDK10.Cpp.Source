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

//-----------------------------------------------------------------------------
//
// logger_slack
//
//  - include this header to use logger_slack
//    1) logger_slack is declared
//    2) "slack" is registered to create_logger
//       (the "type" : "slack" of a service / logger_hub "child" setting)
//    3) windows : the http / ssl libraries are linked by this header
//       linux   : link CGDK10.net.socket.http_linux, CGDK10.net.socket.crypt_linux,
//                 CGDK10.system.util.crypt_linux, ssl and crypto
//
//  - the programs not including this header don't need the http / ssl libraries
//
//-----------------------------------------------------------------------------
#include <condition_variable>
#include <deque>
#include "cgdk/sdk10/server.log.h"
#include "cgdk/sdk10/net.socket.http.h"
#include "cgdk/sdk10/server.system/log/server.logger_slack.h"

namespace CGDK
{
	namespace
	{
		// - registers "slack" (every translation unit including this header : the same creator)
		const bool _is_logger_slack_registered = (register_logger_creator("slack"sv, []() -> own_ptr<Nlogger> { return make_own<logger_slack>(); }), true);
	}
}
