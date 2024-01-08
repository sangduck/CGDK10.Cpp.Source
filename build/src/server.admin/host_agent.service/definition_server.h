//*****************************************************************************
//*                                                                           *
//*                      Cho SangHyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                          Server Admin Host Agent                          *
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
//----------------------------------------------------------------------------
// Definitions
//----------------------------------------------------------------------------
#define	SERVICE_DISPLAYNAME		"CGCII HostAgent"
#define	SERVICE_DESCRIPTION		"CGCII Admin System's Host Agent"

const uint32_t	CGNETMESSAGE_SERVER_REGISTER = (eMESSAGE::SERVER::ADMIN::BASE | CODE_TYPE::TAIL_SUB_0 | 0);
const uint32_t	CGNETMESSAGE_SERVER_UNREGISTER = (eMESSAGE::SERVER::ADMIN::BASE | CODE_TYPE::TAIL_SUB_0 | 1);
const uint32_t	CGNETMESSAGE_SERVER_UPDATE = (eMESSAGE::SERVER::ADMIN::BASE | CODE_TYPE::TAIL_SUB_0 | 2);
const uint32_t	CGNETMESSAGE_SERVER_ENTER = (eMESSAGE::SERVER::ADMIN::BASE | CODE_TYPE::TAIL_SUB_0 | 3);
const uint32_t	CGNETMESSAGE_SERVER_LEAVE = (eMESSAGE::SERVER::ADMIN::BASE | CODE_TYPE::TAIL_SUB_0 | 4);
const uint32_t	eMESSAGE_UPDATE_HOST_INFO = (eMESSAGE::SERVER::ADMIN::BASE | CODE_TYPE::TAIL_SUB_0 | 5);
const uint32_t	eMESSAGE_UPDATE_SYSTEMSTATUS_INFO = (eMESSAGE::SERVER::ADMIN::BASE | CODE_TYPE::TAIL_SUB_0 | 6);


//----------------------------------------------------------------------------
// observer
//----------------------------------------------------------------------------
const uint64_t	OCID_SYSTEM = 0;
const uint64_t	OCID_FRAME1 = 1;
const uint64_t	OCID_FRAME2 = 2;
const uint64_t	OCID_FRAME3 = 3;


//----------------------------------------------------------------------------
// web message
//----------------------------------------------------------------------------
// 1) 'A' frame
#define MESSAGE_WEB_GENERAL                 "ASGN" // general
#define MESSAGE_WEB_SET_FRAME_PAGE          "ASCF" // complete load page (A)
#define MESSAGE_WEB_SYSTEM_STATUS           "ASST" // system info


// 2) host info
// - c ->
#define MESSAGE_WEB_SERVER_LIST_ADD         "SVAD"  // server_list add + id
#define MESSAGE_WEB_SERVER_LIST_REMOVE      "SVRV"  // server_list remove + id
#define MESSAGE_WEB_SERVER_LIST_UPDATE      "SVUD"  // server_list update + id
#define MESSAGE_WEB_SERVER_LIST_INITIALIZE  "SVIN"  // server_list initialize + id
#define MESSAGE_WEB_SERVER_LIST_START       "SVST"  // server_list start + id
#define MESSAGE_WEB_SERVER_LIST_STOP        "SVSP"  // server_list stop + id
#define MESSAGE_WEB_SERVER_LIST_DESTROY     "SVDT"  // server_list destroy + id
#define MESSAGE_WEB_SERVER_LIST_KILL        "SVKI"  // server_list kill + id
// - s -> c
#define MESSAGE_WEB_SERVER_LIST             "SELI"  // server list
                                                    // - command 'A' => add
                                                    // - command 'D' => delete
                                                    // - command 'U' => update

// - c -> s
#define MESSAGE_WEB_EVENT_LIST_ADD          "EVAD"  // event_list add + id
#define MESSAGE_WEB_EVENT_LIST_REMOVE       "EVRV"  // event_list remove + id
#define MESSAGE_WEB_EVENT_LIST_START        "EVST"  // event_list start + id
#define MESSAGE_WEB_EVENT_LIST_PAUSE        "EVPU"  // event_list pause + id
#define MESSAGE_WEB_EVENT_LIST_STOP         "EVSP"  // event_list stop + id
// - s -> c
#define MESSAGE_WEB_EVENT_LIST              "EVLI"  // event list
                                                    // - command 'A' => add
                                                    // - command 'D' => delete
                                                    // - command 'U' => update
// - s -> c
#define MESSAGE_WEB_LOG                     "LOGS"  // logs

// - s -> c
#define MESSAGE_WEB_GENERAL_HOST_INFO       "GNRL"  // - general


// 3) server info - frame (B~ )
#define MESSAGE_WEB_SET_FRAME_PAGE_B        "BSCF"  // complete load page (B)
#define MESSAGE_WEB_TEMP                    "BNGR"  // ?
#define MESSAGE_WEB_SERVER_INFO_SERVER_LIST "BSLA"  // server list
                                                    // - command 'A' => add
                                                    // - command 'D' => delete
                                                    // - command 'U' => update
#define MESSAGE_WEB_SEVER_INFO_SELECT       "BSRS"  // select server


// 4) server info - object (C~)
#define MESSAGE_WEB_LOG_OBJECT_INFO          "CLOG"  // logs
#define MESSAGE_WEB_GENERAL_OBJECT_INFO      "CGNR"  // - general
#define MESSAGE_WEB_EVENT_LIST_OBJECT_INFO   "CVLI"  // event list
                                                    // - command 'A' => add
                                                    // - command 'D' => delete
                                                    // - command 'U' => update



// 5) server info - factory (D~)
// s -> c
#define MESSAGE_WEB_FACTORY_LIST_ADD         "CAGA"  // factory list - add
#define MESSAGE_WEB_FACTORY_LIST_UPDATE      "CAGU"  // factory list - update
// c -> s
#define MESSAGE_WEB_FACTORY_FILTER_MEMORY    "CFGA"  // factory list - filter memory block 
                                                     // - param1 'C' => checked
                                                     // - param1 'U' => unchecked
#define MESSAGE_WEB_FACTORY_FILTER_SYSTEM    "CFGB"  // factory list - filter system
                                                     // - param1 'C' => checked
                                                     // - param1 'U' => unchecked
#define MESSAGE_WEB_FACTORY_FILTER_USER      "CFGC"  // factory list - filter user
                                                     // - param1 'C' => checked
                                                     // - param1 'U' => unchecked

// 6) server info - executor (E~)
// s -> c
#define MESSAGE_WEB_THREAD_LIST             "CTLO"  // executor - thread_list
#define MESSAGE_WEB_EXECUTOR_LIST           "CELO"  // executor - executor_list
#define MESSAGE_WEB_EXECUTABLE_LIST         "CEBO"  // executor - executable_list
#define MESSAGE_WEB_SCHEDULABLE_LIST        "CSLO"  // executor - schedulable_list
#define MESSAGE_WEB_EXECUTOR_BUTTON_TOGGLE  "CVAD"  // server_list add + id


// 7) server info - network (E~)
// s -> c
#define MESSAGE_WEB_CONNECTIVE_LIST         "CCTO"  // network - connective_list
#define MESSAGE_WEB_BINDER_LIST             "CBDO"  // network - binder_list


//
#define TOTAL_STATISTICS_INFO   (1440 * 7)