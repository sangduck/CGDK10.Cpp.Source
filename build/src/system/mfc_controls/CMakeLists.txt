#*****************************************************************************
#*                                                                           *
#*                      Cho sanghyun's Game Classes II                       *
#*                      Ver 10.0 / release 2019.12.11                        *
#*                                                                           *
#*                     CMake - CGDK10.system.mfc_controls                    *
#*                                                                           *
#*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
#*  Best for Game Developement and Optimized for Game Developement.          *
#*                                                                           *
#*                (c) 2003. Cho Sanghyun. All right reserved.                *
#*                          http://www.CGCII.co.kr                           *
#*                                                                           *
#*****************************************************************************

#-----------------------------------------------------------------------------
# setting
#-----------------------------------------------------------------------------
# 1) build version
CMAKE_MINIMUM_REQUIRED ( VERSION 3.20 )
CMAKE_POLICY ( VERSION 3.20 )

# 2) project name
PROJECT ( "CGDK10.system.mfc_controls" )

# 3) project version
SET ( PROJECT_VERSION_MAJOR 10 )
SET ( PROJECT_VERSION_MINOR 0 )
SET ( PROJECT_VERSION_PATCH 100 )
SET ( PROJECT_VERSION_TWEAK 1 )

# 4) common build rules (platform, compile options, output name/directory)
INCLUDE ( ${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake/cgdk.cmake )


#-----------------------------------------------------------------------------
# source files
#-----------------------------------------------------------------------------
# 1) common (linux & windows)
SET (SRC_FILES

)

# 2) linux only
SET (SRC_FILES_LINUX

)

# 3) windows only
SET (SRC_FILES_WINDOWS
pch.cpp
CGDK10.system.mfc_controls.cpp
child_dialog.cpp
child_executor_list.cpp
child_reference_list.cpp
child_schedulable_list.cpp
child_thread_list.cpp
dialog_amount.cpp
dialog_amount_shrink.cpp
dialog_CGDK_component_info.cpp
dialog_object_factory_info.cpp
dialog_object_factory_status.cpp
dialog_reference_status.cpp
dialog_thread_status.cpp
dllmain.cpp
listbox.cpp
listbox_log.cpp
static_graph.cpp
CGDK10.system.mfc_controls.rc
CGDK10.system.mfc_controls.def
)


#-----------------------------------------------------------------------------
# compile
#-----------------------------------------------------------------------------
# 1) library
CGDK_ADD_LIBRARY ( CGDK10.system.mfc_controls SHARED
	SOURCES         ${SRC_FILES}
	LINUX_SOURCES   ${SRC_FILES_LINUX}
	WINDOWS_SOURCES ${SRC_FILES_WINDOWS}
	PCH             pch.h
	UNICODE
	MFC
	SVN_VERSION
	WINDOWS_DEFINITIONS _CGMFCCONTROLS_EXT
	WINDOWS_OPTIONS -Zm256
	WINDOWS_LIBS    $<IF:${CGDK_IS_DEBUG},zlibd,zlib> libssl libcrypto
	DEPENDS         CGDK10.system.object CGDK10.system.util.crypt
)

# 2) build alias for convenience
IF (TARGET ${CGDK_TARGET})
	ADD_CUSTOM_TARGET ( system.mfc_controls DEPENDS ${CGDK_TARGET} )
ENDIF ()