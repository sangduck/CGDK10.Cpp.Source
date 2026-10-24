//*****************************************************************************
//*                                                                           *
//*                     CGCII simple chatting system (1)                      *
//*                                                                           *
//*                             chatting Server                               *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
//*  Best for Game Developement and Optimized for Game Developement.          *
//*                                                                           *
//*                 ¨Ï 2016 Cho Sanghyun All right reserved.                  *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************

#pragma once
//-----------------------------------------------------------------
// 1. setting
//-----------------------------------------------------------------
// definition) 
#define	LOG_FILE		"log/chatting_scaleoutserver_channel_manager_{}{}{}-{}{}{}.log"sv
#define	CONFIGURE_FILE	"configure/chatting_scaleoutserver_channel_manager.json"sv


//-----------------------------------------------------------------
// 2. definitions
//-----------------------------------------------------------------
namespace server
{
	class socket;
	class group_immigration;
	class group_channel;
}

namespace channel
{
	class manager;
}

namespace user
{
	class manager;
	class account;

	class group_immigration;
	class socket;
}



//-----------------------------------------------------------------
// 3. Header files - Engine
//-----------------------------------------------------------------
// 1) Engine 
#include "cgdk/sdk10/net.socket.h"
#include "cgdk/sdk10/server.time_event.h"
#include "cgdk/sdk10/server.log.h"
#include "cgdk/sdk10/server.service.h"
#include "cgdk/sdk10/server.group.h"

using namespace CGDK;

//-----------------------------------------------------------------
// 4. 
//-----------------------------------------------------------------
// 1) common
#include "../../common/definitions.h"
#include "../../common/definition_message.h"
#include "../../common/Igroup_list.h"

// 2) definitions
#include "server.channel_manager.definition.h"

// 3) user
#include "user.socket.h"
#include "user.group_immigration.h"
#include "user.manager.h"

// 4) channels
#include "server.socket.h"
#include "server.group_immigration.h"
#include "server.group_channel.h"

// 5) service
#include "service.h"

// 6) service entity
extern own_ptr<service> g_pservice;
