#*****************************************************************************
#*                                                                           *
#*                      Cho sanghyun's Game Classes II                       *
#*                       Ver 10.0 / release 2019.12.11                       *
#*                                                                           *
#*            CMake - CGDK10.server_admin.admin_server.console               *
#*                                                                           *
#*                                                                           *
#*                                                                           *
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
PROJECT ( "CGDK10.server_admin.admin_server.console" )

# 3) project version
SET ( PROJECT_VERSION_MAJOR 10 )
SET ( PROJECT_VERSION_MINOR 0 )
SET ( PROJECT_VERSION_PATCH 100 )
SET ( PROJECT_VERSION_TWEAK 1 )

# 4) common build rules
INCLUDE ( ${CMAKE_CURRENT_SOURCE_DIR}/../../../../cmake/cgdk.cmake )


#-----------------------------------------------------------------------------
# source files
#  - main/pch.h are UTF-8 (BOM) so that gcc and msvc can both read them.
#  - pch.cpp is only for the windows (vcxproj) build.
#-----------------------------------------------------------------------------
SET (SRC_FILES
admin_server.cpp
admin_server.policy.cpp
admin_server.control.cpp
CGDK10.server_admin.admin_server.console.cpp
)


#-----------------------------------------------------------------------------
# compile
#-----------------------------------------------------------------------------
CGDK_ADD_EXECUTABLE ( ${PROJECT_NAME}
	SOURCES		${SRC_FILES}
	PCH			pch.h
	LINUX_LIBS
		CGDK10.server.service_linux.${BUILD_TYPE}
		CGDK10.server.time_event_linux.${BUILD_TYPE}
		CGDK10.server.log_linux.${BUILD_TYPE}
		CGDK10.net.socket.http_linux.${BUILD_TYPE}
		CGDK10.net.socket.crypt_linux.${BUILD_TYPE}
		CGDK10.net.socket_linux.${BUILD_TYPE}
		CGDK10.system.object_linux.${BUILD_TYPE}
		CGDK10.system.util.crypt_linux.${BUILD_TYPE}
		crypto
		ssl
		rt
		pthread
	DEPENDS
		CGDK10.server.service
		CGDK10.server.time_event
		CGDK10.server.log
		CGDK10.net.socket.http
		CGDK10.net.socket.crypt
		CGDK10.net.socket
		CGDK10.system.object
		CGDK10.system.util.crypt
	OUTPUT_DIR	"${CGDK_EXECUTE_DIR}/CGDK10.server_admin.admin_server.console"
)
TARGET_LINK_DIRECTORIES ( ${PROJECT_NAME} PRIVATE "${CGDK_LIB_DIR}" )

IF (NOT WIN32)
	SET_TARGET_PROPERTIES ( ${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}_linux.${BUILD_TYPE}" )
ENDIF ()

# Build alias for convenience
ADD_CUSTOM_TARGET ( admin_server DEPENDS ${PROJECT_NAME} )
