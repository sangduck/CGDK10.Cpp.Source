// pch.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently

#pragma once

#ifndef VC_EXTRALEAN
	#define VC_EXTRALEAN					// Exclude rarely-used stuff from Windows headers
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit
//#define	_CRT_SECURE_NO_WARNINGS

#include "cgdk/sdk10/prefix.h"
#undef _INC_MALLOC

#include <afxwin.h>							// MFC core and standard components
#include <afxext.h>							// MFC extensions

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


// ----------------------------------------------------------------------------
// Include
// ----------------------------------------------------------------------------
// 1) Main Include
#include "cgdk/sdk10/server.system.mfc_controls.h"

// 2) svn version file
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

// 3) license files
#include "../../common/version.h"

// 4) resources
#include "resource.h"
