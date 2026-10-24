#include "pch.h"

namespace CGDK
{


//----------------------------------------------------------------------------
// 
//----------------------------------------------------------------------------
// 1) compiler
#if _MSC_VER>=_MSC_VER_2019
	#define	_COMPILER					CGDK::eCOMPILER::VISUALSTUDIO_16
#elif _MSC_VER>=_MSC_VER_2017
	#define	_COMPILER					CGDK::eCOMPILER::VISUALSTUDIO_15
#elif _MSC_VER>=_MSC_VER_2015
	#define	_COMPILER					CGDK::eCOMPILER::VISUALSTUDIO_14
#elif _MSC_VER>=_MSC_VER_2013
	#define	_COMPILER					CGDK::eCOMPILER::VISUALSTUDIO_13
#elif _MSC_VER>=_MSC_VER_2012
	#define	_COMPILER					CGDK::eCOMPILER::VISUALSTUDIO_12
#elif _MSC_VER>=_MSC_VER_2010
	#define	_COMPILER					CGDK::eCOMPILER::VISUALSTUDIO_10
#else
	#define	_COMPILER					CGDK::eCOMPILER::GCC
#endif

// 2) platform
#if defined(_M_X64)
	#define	_PLATFORM					CGDK::ePLATFORM::WINDOWS_X64
#elif defined(_M_IX86)
	#define	_PLATFORM					CGDK::ePLATFORM::WINDOWS_X86
#else
	#define	_PLATFORM					CGDK::ePLATFORM::WINDOWS_X64
#endif

// 3) configuration
#ifdef _DLL 
	#ifdef NDEBUG
		#define _CONFIGURATION			CGDK::eCONFIGURATION::RELEASE_DLL_
	#else
		#define _CONFIGURATION			CGDK::eCONFIGURATION::DEBUG_DLL_
	#endif
#else
	#ifdef NDEBUG
		#define _CONFIGURATION			CGDK::eCONFIGURATION::RELEASE_
	#else
		#define _CONFIGURATION			CGDK::eCONFIGURATION::DEBUG_
	#endif
#endif


chrono::time::utc::time_point get_build_date(const std::string_view _str_date, const std::string_view _str_time);
void add_component_info(sCOMPONENT_INFO&& _component) noexcept;

CGDK_INFO_CLASS::CGDK_INFO_CLASS()
{
	// declare) 
	sCOMPONENT_INFO temp;

	// 1) Component Info
	temp.component_info.name			 = CGDK_COMPONENT_NAME;
	temp.component_info.version_package	 = _CGDK_VERSION_PACKAGE;
	temp.component_info.version_sub		 = _CGDK_VERSION_SUB;
	temp.component_info.version_build	 = SVN_COMMIT_REVISION;
	temp.component_info.time_build		 = get_build_date(__DATE__, __TIME__);
	temp.component_info.compiler		 = _COMPILER;
	temp.component_info.platform		 = _PLATFORM;
	temp.component_info.configuration	 = _CONFIGURATION;
	temp.component_info.time_create		 = chrono::time::utc::now();

	// 2) add license component
	add_component_info(std::move(temp));
}


}