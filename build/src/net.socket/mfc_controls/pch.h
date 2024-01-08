//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                  MFC Controls for Network Socket Classes                  *
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

#ifndef PCH_H
#define PCH_H

//-----------------------------------------------------------------------------
// Definitions
//-----------------------------------------------------------------------------
#ifndef VC_EXTRALEAN
	#define VC_EXTRALEAN					// Exclude rarely-used stuff from Windows headers
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit
#define	_CRT_SECURE_NO_WARNINGS

// Debug) 
#ifdef NDEBUG
	#ifndef _SECURE_SCL
		#define	_SECURE_SCL	0
	#endif
	#ifndef _HAS_ITERATOR_DEBUGGING
		#define _HAS_ITERATOR_DEBUGGING 0
	#endif
#endif

// Debug)
#ifndef NDEBUG
	#define _CRTDBG_MAP_ALLOC
#endif

#include "cgdk/sdk10/prefix.h"
#undef _INC_MALLOC

#pragma warning (disable:5054)

#include <afxwin.h>							// MFC core and standard components
#include <afxext.h>						    // MFC extensions

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxole.h>							// MFC OLE classes
#include <afxodlgs.h>						// MFC OLE dialog classes
#include <afxdisp.h>						// MFC Automation classes
#endif // _AFX_NO_OLE_SUPPORT

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>						// MFC support for Internet Explorer 4 Common Controls
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>							// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#if _MSC_VER>=_MSC_VER_2008
	#include <afxcontrolbars.h>
#endif

#pragma warning (default:5054)


// ----------------------------------------------------------------------------
// Include
// ----------------------------------------------------------------------------
// 1) Main Include
#include "cgdk/sdk10/net.socket.mfc_controls.h"

// 2) svn version
#if defined(_MSC_VER)
	#include "svn_version.h"
#else
	#ifndef _SVN_VERSION_H_
	#define _SVN_VERSION_H_

	#define SVN_LOCAL_MODIFICATIONS       0						// 1 if there are modifications to the local working copy, 0 otherwise
	#define SVN_COMMIT_REVISION           2310					// Highest committed revision number in the working copy
	#define SVN_COMMIT_REVISION_STRING    "2310"				// Highest committed revision number in the working copy
	#define SVN_UPDATE_REVISION_STRING    "2310"				// Highest committed revision number in the working copy
	#define SVN_TIME_NOW_STRING           "2018/07/13 08:54:17"	// Current system date &amp; time

	#endif
#endif

// 4) CGDK info
#include "../../common/version.h"

// 3) Main Include
#include "resource.h"

#endif //PCH_H
