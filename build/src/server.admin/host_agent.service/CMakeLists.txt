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
# 1) build version
CMAKE_MINIMUM_REQUIRED ( VERSION 3.20 )
CMAKE_POLICY ( VERSION 3.20 )

# 2) project name
PROJECT ( "CGDK10.server.admin.host_agent" )

# 3) project version
SET ( PROJECT_VERSION_MAJOR 10 )
SET ( PROJECT_VERSION_MINOR 0 )
SET ( PROJECT_VERSION_PATCH 100 )
SET ( PROJECT_VERSION_TWEAK 1 )

# 4) common build rules
INCLUDE ( ${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake/cgdk.cmake )


#-----------------------------------------------------------------------------
# source files (same as CGDK10.server.admin.host_agent.service.shared.vcxitems)
#-----------------------------------------------------------------------------
SET (SRC_FILES
pch.cpp

# instances
server_admin.Iinstance.cpp
server_admin.instance_host.cpp
server_admin.instance_manager.cpp
server_admin.instance_service.cpp
server_admin.instance_service_local.cpp
server_admin.instance_service_remote.cpp

# peer (service)
Igroup.cpp
socket_peer.cpp
group_peer_immigration.cpp
group_peer_server.cpp
group_peer_client.cpp

# admin server
socket_admin_server.cpp

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

# webpages
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
webpage_login_htm.cpp

# service
service.cpp

# main
CGDK10.server.admin.host_agent.service.cpp
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
		CGDK10.system.util.crypt_linux.${BUILD_TYPE}
		CGDK10.system.object_linux.${BUILD_TYPE}
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
		CGDK10.system.util.crypt
		CGDK10.system.object
)
CGDK_ADD_SVN_VERSION ( ${PROJECT_NAME} )
TARGET_LINK_DIRECTORIES ( ${PROJECT_NAME} PRIVATE "${CGDK_LIB_DIR}" )

# - output name (windows vcxproj: CGDK10.server.admin.host_agent[.Debug])
IF (NOT WIN32)
	SET_TARGET_PROPERTIES ( ${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}_linux.${BUILD_TYPE}" )
ENDIF ()

# Build alias for convenience
ADD_CUSTOM_TARGET ( host_agent DEPENDS ${PROJECT_NAME} )
