#*****************************************************************************
#*                                                                           *
#*                      Cho sanghyun's Game Classes II                       *
#*                      Ver 10.0 / release 2019.12.11                        *
#*                                                                           *
#*                   CMake - CGDK10.server.service                           *
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
PROJECT ( "CGDK10.server.service" )

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

# ...
pch.cpp

#common
${CGDK_COMMON_DIR}/cgdk_info.cpp

#definitions
api.manager_performance_query.cpp
api.performance_counter.cpp
api.server.process_info.cpp
api.server_status_handle.cpp
api.service_handle.cpp
api.service.cpp

# definitions
server.definitions.admin.cpp
server.definitions.admin.server_info.cpp
server.definitions.admin.message.object.cpp
server.definitions.admin.message.network.cpp
server.definitions.admin.message.server.cpp
server.definitions.admin.message.admin.cpp
server.definitions.admin.message.admin_instance.cpp
server.definitions.admin.protocol.cpp

#
server.system_info.cpp
server.admin_info.binder.cpp
server.admin_info.connective.cpp
server.admin_info.factory.cpp
server.admin_info.time_event.cpp

# time_event for service
server.time_event.object.service_destroy.cpp
server.time_event.object.service_init.cpp
server.time_event.object.service_start.cpp
server.time_event.object.service_stop.cpp
server.time_event.schedule.service_destroy.cpp
server.time_event.schedule.service_init.cpp
server.time_event.schedule.service_start.cpp
server.time_event.schedule.service_stop.cpp

# admin
server_admin.group_socket.cpp
server_admin.socket.server.cpp
server_admin.host_instance_local.cpp
server_admin.host_instance_manager.cpp
server_admin.host_Ninstance.cpp
server_admin.socket.client.cpp
server_admin.service_functions.cpp
server_admin.service_instance_local.cpp
server_admin.service_instance_local.serialize.cpp
server_admin.service_instance_manager.cpp
server_admin.service_Ninstance.cpp
server.Nservice.cpp
server.Nservice_application.cpp
server.Nstatic_service_logger.cpp
server.service.cpp

)

# 2) linux only
SET (SRC_FILES_LINUX

)

# 3) windows only
SET (SRC_FILES_WINDOWS

)


#-----------------------------------------------------------------------------
# compile
#-----------------------------------------------------------------------------
# 1) library
CGDK_ADD_LIBRARY ( CGDK10.server.service STATIC
	SOURCES         ${SRC_FILES}
	LINUX_SOURCES   ${SRC_FILES_LINUX}
	WINDOWS_SOURCES ${SRC_FILES_WINDOWS}
	PCH             pch.h
	SVN_VERSION
)

# 2) build alias for convenience
IF (TARGET ${CGDK_TARGET})
	ADD_CUSTOM_TARGET ( system.service DEPENDS ${CGDK_TARGET} )
ENDIF ()