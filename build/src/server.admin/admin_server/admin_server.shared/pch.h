//*****************************************************************************
//*                                                                           *
//*                             CGCII Trade System                            *
//*                                                                           *
//*                               Control Server                              *
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
// 1) CGCII Components
#include "CGNetSocketTemplates.h"
#include "CGServerTimeEventClasses.h"
#include "CGServerLogClasses.h"
#include "CGServerGroupTemplates.h"
#include "CGServerTimeEventClasses.h"
#include "CGServerqueryODBCClasses.h"
#include "CGServerAdminClasses.h"

// 2) Version
#include "SVN_Version.h"


//-----------------------------------------------------------------
// 3. Header files - Project
//-----------------------------------------------------------------
// 1) Commons
#include "./../../common/common.server.h"

// 2) Definitions
#include "DefinitionControlServer.h"

// 3) Event Log
#include "CServerEventLog.h"

// 4) Deposit Confirm
#include "ServerDepositConfirm.h"

// 5) Isocket_server
#include "Isocket_server.h"


// 5) Server
#include "ServerSocket.h"
#include "ServerMarket.h"
#include "IServerGroup.h"
#include "ServerGroupImmigration.h"
#include "ServerGroupLogin.h"
#include "ServerGroupAccount.h"
#include "ServerGroupSession.h"
#include "ServerGroupTrade.h"
#include "ServerGroupQuot.h"
#include "ServerGroupChattingManager.h"
#include "ServerGroupChattingChannel.h"
#include "ServerGroupNotify.h"

// 6) Host
#include "HostAgentServerEntity.h"
#include "HostAgentServerEvent.h"
#include "HostagentSocket.h"
#include "HostagentGroup.h"
#include "HostAgentGroupImmigration.h"

// 7) Time Events
#include "TimeEventDefinitions.h"
#include "TimeEventServerOperation.h"
#include "TimeEventServer.h"
#include "TimeEventManager.h"

// 8) Querier/query
#include "QuerierServer.h"
#include "query/queryEventHostCertify.h"
#include "query/queryEventHostClose.h"
#include "query/queryEventServerCertify.h"
#include "query/queryEventServerClose.h"
#include "query/queryEventServerState.h"
#include "query/queryEventTradeMarketState.h"
#include "query/queryEventTradeMarketStatus.h"
#include "query/queryTimeEventRegister.h"
#include "query/queryTimeEventUnregister.h"
#include "query/queryTimeEventUpdate.h"
#include "query/queryTimeEventProcess.h"
#include "query/queryTimeEventDone.h"

// 9) Admin Client
#include "AdminClientSocket.h"
#include "AdminClientGroup.h"
#include "AdminClientGroupImmigration.h"

// 10) Service
#include "Service.h"


//-----------------------------------------------------------------
// 4. extern
//-----------------------------------------------------------------
// 1) Service & Log
extern	own_ptr<service>			g_pservice;
extern	own_ptr<log_hub_archive>	g_plog;
