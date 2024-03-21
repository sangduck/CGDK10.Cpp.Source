#*****************************************************************************
#*                                                                           *
#*                      Cho sanghyun's Game Classes II                       *
#*                       Ver 10.0 / release 2019.12.11                       *
#*                                                                           *
#*               CMake - CGDK10.server.admin.host_agent.service              *
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
# 1) project name
PROJECT ( "CGDK10.server.admin.host_agent.service" )

# 2) project version
SET ( PROJECT_VERSION_MAJOR 10 )
SET ( PROJECT_VERSION_MINOR 0 )
SET ( PROJECT_VERSION_PATCH 100 )
SET ( PROJECT_VERSION_TWEAK 1 )

# 3) build version
CMAKE_MINIMUM_REQUIRED ( VERSION 3.20 )
CMAKE_POLICY( VERSION 3.20 )

# 4) default BUILD_TYPE
IF (NOT DEFINED CMAKE_BULD_TYPE)
	SET (CMAKE_BULD_TYPE "Debug")
ENDIF()

# 5) directories
SET ( CMAKE_RUNTIME_OTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../../execute )
SET ( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../execute )

# 6) compile options
set ( CMAKE_CXX_STANDARD 20)
set ( CMAKE_CXX_STANDARD_REQUIRED ON)
SET ( CMAKE_CXX_FLAGS "-DC_FLAGS -fexceptions -std=c++20" )
SET ( CMAKE_CXX_FLAGS_DEBUG "-D_DEBUG -g1 -Wall -Wextra" )
SET ( CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3 -Wall -Wextra" )

# 7) include directories
INCLUDE_DIRECTORIES(
	${CMAKE_CURRENT_SOURCE_DIR}/../../../../include
	./
)

# 8) library directories
LINK_DIRECTORIES(
	${CMAKE_CURRENT_SOURCE_DIR}/../../../../lib/cgdk/sdk10/${DIST}
)

#-----------------------------------------------------------------------------
# source files
#-----------------------------------------------------------------------------
SET (SRC_FILES
#...
pch.cpp

# server entities
server_entity.cpp

# server
socket_server.cpp
group_server.cpp
group_server_immigration.cpp

# web client
socket_web.cpp
socket_websocket.cpp
group_websocket.cpp
group_websocket_immigration.cpp

# web observable
Iobservable_websocket.cpp
observable_websocket_host_info.cpp
observable_websocket_server_info.cpp
observable_websocket_server_overview_info.cpp
observable_websocket_server_factory_info.cpp
observable_websocket_server_executor_info.cpp
observable_websocket_server_network_info.cpp
observable_websocket_server_querier_info.cpp
observable_websocket_server_crash_info.cpp
observable_websocket_system_info.cpp

#webpages
webpage_frame_htm.cpp
webpage_host_info_htm.cpp
webpage_host_log_htm.cpp
webpage_reject_htm.cpp
webpage_server_info_htm.cpp
webpage_server_overview_info_htm.cpp
webpage_server_factory_info_htm.cpp
webpage_server_executor_info_htm.cpp
webpage_server_network_info_htm.cpp
webpage_server_querier_info_htm.cpp
webpage_server_crash_info_htm.cpp
webpage_server_setting_htm.cpp
webpage_server_statistics_htm.cpp
webpage_time_event_info_htm.cpp

# control
Isocket_control.cpp
socket_control.cpp
socket_control_admin_server.cpp
group_control.cpp
group_control_immigration.cpp

# service
service.cpp

# main
CGDK10.server.admin.host_agent.service.cpp
)


#-----------------------------------------------------------------------------
# compile
#-----------------------------------------------------------------------------
# 1) set variable
IF (${CMAKE_BUILD_TYPE} MATCHES "Debug")
	SET (BUILD_MESSAGE "Debug")
	SET (BUILD_TYPE "Debug")
ELSEIF (${CMAKE_BUILD_TYPE} MATCHES "Release")
	SET (BUILD_MESSAGE "Release")
	SET (BUILD_TYPE "Release")
ELSE ()
	MESSAGE (SEND_ERROR "'CMAKE_BUILD_TYPE' is not mathed")
ENDIF ()

# 2) messages
MESSAGE (STATUS "compile ${PROJECT_NAME}")
MESSAGE (STATUS "  - configure['${BUILD_MESSAGE}']")

# 3) compile
ADD_EXECUTABLE(${PROJECT_NAME} ${SRC_FILES})
TARGET_PRECOMPILE_HEADERS(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:pch.h>")

# 4) configure file
#CONFIGURE_FILE ( version.h.in version.h )

# 5) add library
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} 
	CGDK10.server.service_linux.${BUILD_TYPE}
	CGDK10.server.time_event_linux.${BUILD_TYPE}
	CGDK10.server.log_linux.${BUILD_TYPE}
	CGDK10.net.socket.http_linux.${BUILD_TYPE}
	CGDK10.net.socket_linux.${BUILD_TYPE}
	CGDK10.system.util.crypt_linux.${BUILD_TYPE}
	CGDK10.system.object_linux.${BUILD_TYPE}
	crypto
	ssl
	rt
	pthread
)
