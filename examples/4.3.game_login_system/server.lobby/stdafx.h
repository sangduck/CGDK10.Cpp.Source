#pragma once

//-----------------------------------------------------------------
// 1. Header files - Platform
//-----------------------------------------------------------------
#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN							// 거의 사용되지 않는 내용은 Windows 헤더에서 제외합니다.
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // 일부 CString 생성자는 명시적으로 선언됩니다.

// MFC의 공통 부분과 무시 가능한 경고 메시지에 대한 숨기기를 해제합니다.
#define _AFX_ALL_WARNINGS

#include <afxwin.h>								// MFC 핵심 및 표준 구성 요소입니다.
#include <afxext.h>								// MFC 확장입니다.
#include <afxdisp.h>							// MFC 자동화 클래스입니다.

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>							// Internet Explorer 4 공용 컨트롤에 대한 MFC 지원입니다.
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>								// Windows 공용 컨트롤에 대한 MFC 지원입니다.
#endif // _AFX_NO_AFXCMN_SUPPORT				

#include <afxcontrolbars.h>						// MFC의 리본 및 컨트롤 막대 지원

#ifdef _UNICODE
	#if defined _M_IX86
		#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
	#elif defined _M_X64
		#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
	#else
		#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
	#endif
#endif

#if defined(_DEBUG) && !defined(NDEBUG)
	#define	SERVER_CONFIGURE	L"debug"
#else
	#define	SERVER_CONFIGURE	L"release"
#endif


//-----------------------------------------------------------------
// 2. 
//-----------------------------------------------------------------
// 1)  Server Login
namespace ServerLobby
{
	class	CSocket;
}

// 2)  Server Game
namespace User
{
	class	CGroupSocket;
	class	CGroupImmigrationSocket;
	class	CSocket;
}

// 3) Service
class CService;


//-----------------------------------------------------------------
// 3. Header files - Engine
//-----------------------------------------------------------------
// 1) Engine 
#include "CGNetSocketMFCControls.h"
#include "CGServerMFCControls.h"
#include "CGServerQueryMFCControls.h"
#include "CGNetSocketTemplates.h"
#include "CGServerGroupTemplates.h"
#include "CGServerQueryDBClasses.h"
#include "CGServerAdminClasses.h"


//-----------------------------------------------------------------
// 4. Header files - 
//-----------------------------------------------------------------
 // 1) Definitions
const uint32_t	CGNOTIFY_SERVER_UPDATE			 = (CGMESSAGE::USER | 1);
const uint32_t	CGNOTIFY_LOGINSERVER_CONNECT	 = (CGMESSAGE::USER | 2);
const uint32_t	CGNOTIFY_LOGINSERVER_DISCONNECT	 = (CGMESSAGE::USER | 3);
const uint32_t	CGNOTIFY_LOGINSERVER_UPDATE		 = (CGMESSAGE::USER | 4);

const uint32_t	CGNOTIFY_USER_CONNECT			 = (CGMESSAGE::USER | 5);
const uint32_t	CGNOTIFY_USER_DISCONNECT		 = (CGMESSAGE::USER | 6);
const uint32_t	CGNOTIFY_USER_UPDATE			 = (CGMESSAGE::USER | 7);


//-----------------------------------------------------------------
// 5. 
//-----------------------------------------------------------------
// 1) Definitions
#include "../Common/Definitions.h"
#include "LobbyServerDefinition.h"
#include "svn_version.h"

// 2) Server Lobby
#include "ServerLoginSocket.h"

// 3) User
#include "UserSocket.h"
#include "UserGroupImmigrationSocket.h"
#include "UserGroupSocket.h"

// 4) Service
#include "Service.h"


//-----------------------------------------------------------------
// 6. extern
//-----------------------------------------------------------------
// 1) ...
extern	CGPTR<CService>		g_pservice;
extern	CGPTR<CCGLogHub>	g_pLog;
