//*****************************************************************************
//*                                                                           *
//*                       CGCII Sample Login System (1)                       *
//*                                                                           *
//*                                Login Server                               *
//*                                                                           *
//*  Programmed by Cho SangHyun(sangduck@cgcii.co.kr) 2016/02/01.             *
//*                                                                           *
//*  Updated : 2016/02/01                                                     *
//*                                                                           *
//*****************************************************************************

#pragma once
//-----------------------------------------------------------------------------
//
//  Definitions
//
//-----------------------------------------------------------------------------
 // 1) Definitions
const uint32_t	CGNOTIFY_SERVER_UPDATE			 = (eMESSAGE::USER | 1);
const uint32_t	CGNOTIFY_LOBBYSERVER_CONNECT	 = (eMESSAGE::USER | 2);
const uint32_t	CGNOTIFY_LOBBYSERVER_DISCONNECT	 = (eMESSAGE::USER | 3);
const uint32_t	CGNOTIFY_LOBBYSERVER_UPDATE		 = (eMESSAGE::USER | 4);
