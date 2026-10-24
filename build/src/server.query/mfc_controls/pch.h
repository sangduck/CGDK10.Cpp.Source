// pch.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently

#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN						// Exclude rarely-used stuff from Windows headers
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit
#define	_CRT_SECURE_NO_WARNINGS

// Debug)
#ifndef NDEBUG
	#define _CRTDBG_MAP_ALLOC
#endif

#include "cgdk/sdk10/prefix.h"
#undef _INC_MALLOC

#pragma warning (disable:5054)

#include <afxwin.h>							// MFC core and standard components
#include <afxext.h>							// MFC extensions

#ifndef _XPREPENDX_NO_OLE_SUPPORT
#include <afxole.h>							// MFC OLE classes
#include <afxodlgs.h>						// MFC OLE dialog classes
#include <afxdisp.h>						// MFC Automation classes
#endif // _XPREPENDX_NO_OLE_SUPPORT

#ifndef _XPREPENDX_NO_OLE_SUPPORT
#include <afxdtctl.h>						// MFC support for Internet Explorer 4 Common Controls
#endif
#ifndef _XPREPENDX_NO_XPREPENDXCMN_SUPPORT
#include <afxcmn.h>							// MFC support for Windows Common Controls
#endif // _XPREPENDX_NO_XPREPENDXCMN_SUPPORT

#pragma warning (default:5054)


// ----------------------------------------------------------------------------
// Include
// ----------------------------------------------------------------------------
// 1) Main Include
#include "cgdk/sdk10/server.query.mfc_controls.h"

// 2) svn version file
#include "svn_version.h"

// 3) license files
#include "../../common/version.h"

// 4) resource
#include "resource.h"
