//*****************************************************************************
//*                                                                           *
//*                            lokon server system                            *
//*                                                                           *
//*                                  common                                   *
//*                                                                           *
//*  powerd by CGCII                                                          *
//*                                                                           *
//*  start : 2022/02/3                                                        *
//*                                                                           *
//*****************************************************************************

#pragma once
//-----------------------------------------------------------------------------
//  protocols
//-----------------------------------------------------------------------------
namespace CGDK
{
namespace eMESSAGE
{
	// default) 
	const uint32_t	NONE									 = 0;

	// 1) client
	const uint32_t	USER_LOGIN_REQUEST						 = CGDK::eMESSAGE::NETWORK::USER::BASE + 102;
	const uint32_t	USER_LOGIN_RESPONSE						 = CGDK::eMESSAGE::NETWORK::USER::BASE + 103;

	// 2) area
	const uint32_t	AREA_ENTER_REQUEST						 = CGDK::eMESSAGE::NETWORK::USER::BASE + 201;
	const uint32_t	AREA_ENTER_RESPONSE						 = CGDK::eMESSAGE::NETWORK::USER::BASE + 202;

	const uint32_t	AREA_INFO								 = CGDK::eMESSAGE::NETWORK::USER::BASE + 301;
	const uint32_t	AREA_ENTITY_LIST						 = CGDK::eMESSAGE::NETWORK::USER::BASE + 302;
	const uint32_t	AREA_ENTITY_READY						 = CGDK::eMESSAGE::NETWORK::USER::BASE + 303;
	const uint32_t	AREA_ENTITY_EVENT						 = CGDK::eMESSAGE::NETWORK::USER::BASE + 304;
}
}

