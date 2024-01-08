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
//-----------------------------------------------------------------------------
//
// messages
//
//-----------------------------------------------------------------------------

const uint32_t	CGNOTIFY_UPDATE_SYSTEMSTATUS_INFO		 = (eMESSAGE::USER | 0);

const uint32_t	CGNOTIFY_HOST_SOCKET_CONNECT			 = (eMESSAGE::USER | 4);
const uint32_t	CGNOTIFY_HOST_SOCKET_DISCONNECT			 = (eMESSAGE::USER | 5);
const uint32_t	CGNOTIFY_HOST_SOCKET_UPDATE				 = (eMESSAGE::USER | 6);

const uint32_t	CGNOTIFY_SERVER_MARKET_ADD				 = (eMESSAGE::USER | 7);
const uint32_t	CGNOTIFY_SERVER_MARKET_REMOVE			 = (eMESSAGE::USER | 8);
const uint32_t	CGNOTIFY_SERVER_MARKET_UPDATE			 = (eMESSAGE::USER | 9);

const uint32_t	CGNOTIFY_SERVER_QUOT_ADD				 = (eMESSAGE::USER | 10);
const uint32_t	CGNOTIFY_SERVER_QUOT_REMOVE				 = (eMESSAGE::USER | 11);
const uint32_t	CGNOTIFY_SERVER_QUOT_UPDATE				 = (eMESSAGE::USER | 12);

const uint32_t	CGNOTIFY_SERVER_TIME_EVENT_LOG_ADD		 = (eMESSAGE::USER | 13);

const uint32_t	CGNOTIFY_SERVER_INIT_COMPLETE			 = (eMESSAGE::USER | 14);

const uint32_t	CGNOTIFY_CONFIRM_ADD					 = (eMESSAGE::USER | 15);
const uint32_t	CGNOTIFY_CONFIRM_REMOVE					 = (eMESSAGE::USER | 16);
const uint32_t	CGNOTIFY_CONFIRM_UPDATE					 = (eMESSAGE::USER | 17);

const uint32_t	CGNOTIFY_SERVER_TIME_EVENT_REGISTER		 = (eMESSAGE::USER | 18);
const uint32_t	CGNOTIFY_SERVER_TIME_EVENT_UNREGISTER	 = (eMESSAGE::USER | 19);
const uint32_t	CGNOTIFY_SERVER_TIME_EVENT_UPDATE		 = (eMESSAGE::USER | 20);


//----------------------------------------------------------------------------
// observer channel
//----------------------------------------------------------------------------
const uint64_t	OCID_SYSTEM = 0;
const uint64_t	OCID_FRAME1 = 1;
const uint64_t	OCID_FRAME2 = 2;
const uint64_t	OCID_FRAME3 = 3;
