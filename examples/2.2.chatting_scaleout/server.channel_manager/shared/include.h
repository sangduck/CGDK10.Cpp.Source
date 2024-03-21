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
namespace channel
{
	class manager;

	namespace server
	{
		class group;
		class group_immigration;
		class socket:
	}
}


namespace user
{
	class manager;

	class group_immigration;
	class socket;
}



//-----------------------------------------------------------------
// 2. Header files - Engine
//-----------------------------------------------------------------
// 1) Engine 
#include "cgdk/sdk10/net.socket.h"
#include "cgdk/sdk10/server.time_event.h"
#include "cgdk/sdk10/server.log.h"
#include "cgdk/sdk10/server.service.h"
#include "cgdk/sdk10/server.group.h"

using namespace CGDK;

//-----------------------------------------------------------------
// 3. 
//-----------------------------------------------------------------
const uint32_t	CGNOTIFY_SERVER_UPDATE		 = (eMESSAGE::USER | 1);
const uint32_t	CGNOTIFY_CHANNEL_ADD		 = (eMESSAGE::USER | 2);
const uint32_t	CGNOTIFY_CHANNEL_REMOVE		 = (eMESSAGE::USER | 3);
const uint32_t	CGNOTIFY_CHANNEL_UPDATE		 = (eMESSAGE::USER | 4);

//-----------------------------------------------------------------
// 4. 
//-----------------------------------------------------------------
// 1) definitions
#include "../../common/definitions.h"
#include "../../common/definition_message.h"
#include "../../common/Igroup_list.h"

// 2) user
#include "user.socket.h"
#include "user.group_immigration.h"
#include "user.manager.h"

// 3) channels
#include "server_channel.socket.h"
#include "server_channel.group.h"
#include "server_channel_group_waiting.h"
#include "server_channel_group_chatting.h"

// 4) service
#include "service.h"

// 5) service object
extern own_ptr<service> g_pservice;
