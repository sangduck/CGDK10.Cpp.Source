//*****************************************************************************
//*                                                                           *
//*                       CGCII Sample Login System (1)                       *
//*                                                                           *
//*                                Lobby Server                               *
//*                                                                           *
//*  Programmed by Cho SangHyun(sangduck@cgcii.co.kr)                         *
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
const uint32_t	CGNOTIFY_SERVER_UPDATE			 = (eMESSAGE::USER | 1);

const uint32_t	CGNOTIFY_SERVER_SOCKET_ADD		 = (eMESSAGE::USER | 2);
const uint32_t	CGNOTIFY_SERVER_SOCKET_REMOVE	 = (eMESSAGE::USER | 3);
const uint32_t	CGNOTIFY_SERVER_SOCKET_UPDATE	 = (eMESSAGE::USER | 4);
