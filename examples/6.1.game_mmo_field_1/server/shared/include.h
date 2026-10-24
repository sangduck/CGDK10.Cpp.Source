//*****************************************************************************
//*                                                                           *
//*                          CGCII mmog field system                          *
//*                                                                           *
//*                               field server                                *
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
// 1. CGCII header files
//-----------------------------------------------------------------
// 1) CGCII components
#include "cgdk/sdk10/net.socket.h"
#include "cgdk/sdk10/server.log.h"
#include "cgdk/sdk10/server.group.list.h"
#include "cgdk/sdk10/server.service.h"

// 2) CGDK namespace 
using namespace CGDK;


//-----------------------------------------------------------------
// 2. setting
//-----------------------------------------------------------------
// definition) 
#define	CONFIGURE_FILE	"configure/CGDK10.game_mmo_field_system.server.json"sv

//-----------------------------------------------------------------
// 3. definitions
//-----------------------------------------------------------------
class Igroup_user;
namespace area
{
	class Ientity;

	class group;
	class manager;
	class entity_player;
}

class account;
class account_manager;

//-----------------------------------------------------------------
// 4. header files - project
//-----------------------------------------------------------------
// 1) commons
#include "game_mmo_field_server.definition.h"
#include "../../common/definition.message.h"
#include "../../common/definitions.h"

// 2) socket
#include "socket_user.h"
#include "Igroup_user.h"
#include "group_user_immigration.h"

// 3) account manager
#include "account.h"
#include "account_manager.h"

// 4) area
#include "area_Ientity.h"
#include "area_entity_player.h"
#include "area_group.h"
#include "area_manager.h"

// 5) service
#include "service.h"

//-----------------------------------------------------------------
// 5. Global Variable
//-----------------------------------------------------------------
// 1) ...
extern own_ptr<service> g_pservice;

// debug) 
#ifdef _DEBUG
	#define new DEBUG_NEW
#endif
