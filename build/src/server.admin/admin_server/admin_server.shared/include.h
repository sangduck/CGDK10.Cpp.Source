//*****************************************************************************
//*                                                                           *
//*                             CGCII Trade System                            *
//*                                                                           *
//*                               Account Server                              *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                  ¨Ï2017. Cho SangHyun. All rights reserved                *
//*                          http://www.CGCIII.co.kr                          *
//*                                                                           *
//*****************************************************************************

#pragma once
//-----------------------------------------------------------------
// 1. CGCII Header files
//-----------------------------------------------------------------
// 1) user ODBC rdb module
#define _USE_ODBC
//#define _USE_OLEDB

// 2) CGCII Components
#include "cgdk/sdk10/net.socket.h"
#include "cgdk/sdk10/net.socket.http.h"
#include "cgdk/sdk10/server.time_event.h"
#include "cgdk/sdk10/server.log.h"
#include "cgdk/sdk10/server.group.h"
#include "cgdk/sdk10/server.query.rdb.h"
#include "cgdk/sdk10/server.service.h"

// 3) Version
#if defined(_MSC_VER)
	#include "svn_version.h"
#else
	#ifndef _SVN_VERSION_H_
	#define _SVN_VERSION_H_

	#define SVN_LOCAL_MODIFICATIONS       0						// 1 if there are modifications to the local working copy, 0 otherwise
	#define SVN_COMMIT_REVISION           2310					// Highest committed revision number in the working copy
	#define SVN_COMMIT_REVISION_STRING    "2310"				// Highest committed revision number in the working copy
	#define SVN_UPDATE_REVISION_STRING    "2310"				// Highest committed revision number in the working copy
	#define SVN_TIME_NOW_STRING           "2018/07/13 08:54:17"	// Current system date &amp; time

	#endif
#endif

// 4) CGDK namespace 
using namespace CGDK;


//-----------------------------------------------------------------
// 2. setting
//-----------------------------------------------------------------
#define _DB_MYSQL

// Definition) 
#define	LOG_FILE		"log/admin_server({:04}{:02}{:02}-{:02}{:02}{:02}).log"
#define	CONFIGURE_FILE	"configure/admin_server_setting.json"


//-----------------------------------------------------------------
// 3. Definitions
//-----------------------------------------------------------------
// 1) Service
class service;

namespace server_entity
{
	class Isocket;

	namespace host
	{
		class socket;
		class group_immigration;
		class group;
	};

	class socket;
	class group_immigration;
	class group;

	struct instance;
	class manager_instance;
	class server_event;
}

namespace client
{
	namespace app
	{
		class socket;
		class group;
		class group_immigration;
	}

	namespace web
	{
		class Iobservable;

		class socket;
		class group;
		class group_immigration;
	}
}

namespace time_event_server
{
	class manager;
	class operation;
	class server_operation;
}


class querier_server;

#define	_CGCII_SEND_TO_CLIENT


//-----------------------------------------------------------------
// 4. Header files - Project
//-----------------------------------------------------------------
// 1) Commons
#include "../../common/common.server.h"

// 2) definitions
#include "definition_admin_server.h"

// 3) querier
#include "query/Iquery_event.h"
//#include "query/query_event.host_Certify.h"
//#include "query/query_event.host_Close.h"
//#include "query/query_event.server_certify.h"
//#include "query/query_event.server_close.h"
//#include "query/query_event.server_state.h"
//#include "query/query_event.trade_market_close.h"
//#include "query/query_event.trade_market_open.h"
//#include "query/query_event.trade_market_state.h"
//#include "query/query_event.trade_market_status.h"
//#include "query/query_event.time_event.done.h"
//#include "query/query_event.time_event.process.h"
//#include "query/query_event.time_event.register.h"
//#include "query/query_event.time_event.unregister.h"
//#include "query/query_event.time_event.update.h"
#include "querier_server.h"

// 4) server entity
// - data
#include "server_entity.event.h"
#include "server_entity.instance.h"
#include "server_entity.manager_instance.h"

// - Isocket
#include "server_entity.Isocket.h"

// - host
#include "server_entity.host.socket.h"
#include "server_entity.host.group.h"
#include "server_entity.host.group_immigration.h"

// - peer
#include "server_entity.socket.h"
#include "server_entity.group_immigration.h"
#include "server_entity.group.h"

// - time event
#include "time_event.server_operation.h"
#include "time_event.server.h"
#include "time_event.manager.h"
#include "time_event.definitions.h"
#include "server_event_log.h"

// 5) client
// - application
#include "client.app.socket.h"
#include "client.app.group.h"
#include "client.app.group_immigration.h"

// - web
#include "client.web.socket.h"
#include "client.web.Iobservable.h"
#include "client.web.group_immigration.h"
#include "client.web.group.h"

// 6) service
#include "service.h"


//-----------------------------------------------------------------
// 5. extern
//-----------------------------------------------------------------
// 1) ...
extern	own_ptr<service>			g_pservice;
extern	own_ptr<logger_hub_archive>	g_plogger;
