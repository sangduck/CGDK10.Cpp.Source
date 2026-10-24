#*****************************************************************************
#*                                                                           *
#*                      Cho sanghyun's Game Classes II                       *
#*                      Ver 10.0 / release 2019.12.11                        *
#*                                                                           *
#*                        CMake - CGDK.server.oauth                          *
#*                                                                           *
#*                                                                           *
#*                                                                           *
#*                                                                           *
#*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
#*  Best for Game Developement and Optimized for Game Developement.          *
#*                                                                           *
#*                (c) 2016 Cho Sanghyun All right reserved.                  *
#*                          http://www.CGCII.co.kr                           *
#*                                                                           *
#*****************************************************************************

#-----------------------------------------------------------------------------
# setting
#-----------------------------------------------------------------------------
# 1) build version
CMAKE_MINIMUM_REQUIRED ( VERSION 3.20 )
CMAKE_POLICY( VERSION 3.20 )

# 2) project name
PROJECT ( "CGDK10.server.admin" )

# 3) project version
SET ( PROJECT_VERSION_MAJOR 10 )
SET ( PROJECT_VERSION_MINOR 0 )
SET ( PROJECT_VERSION_PATCH 100 )
SET ( PROJECT_VERSION_TWEAK 1 )

# 4) common build rules
INCLUDE ( ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/cgdk.cmake )


#-----------------------------------------------------------------------------
# compile
#-----------------------------------------------------------------------------
# 1) html to cpp
ADD_SUBDIRECTORY (./CGDK.html_to_cpp)

# 2) host agent / admin server
ADD_SUBDIRECTORY (./host_agent.service)
ADD_SUBDIRECTORY (./admin_server/admin_server.console)

# 3) unit test
ADD_SUBDIRECTORY (./unit_test)

# Build alias for convenience
CGDK_ADD_ALIAS ( admin CGDK10.html_to_cpp CGDK10.server.admin.host_agent CGDK10.server_admin.admin_server.console )