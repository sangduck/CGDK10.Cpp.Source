#*****************************************************************************
#*                                                                           *
#*                      Cho sanghyun's Game Classes II                       *
#*                      Ver 10.0 / release 2019.12.11                        *
#*                                                                           *
#*                        CMake - CGDK.server.system                         *
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
PROJECT ( "CGDK10.server.system" )

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
# 1) system
ADD_SUBDIRECTORY (./log)
ADD_SUBDIRECTORY (./time_event)
ADD_SUBDIRECTORY (./service)
# unit test (only when GoogleTest is found)
ADD_SUBDIRECTORY (./unit_test)

# windows only
IF (WIN32)
	ADD_SUBDIRECTORY (./mfc_controls)
ENDIF ()

# Build alias for convenience
CGDK_ADD_ALIAS ( server.system CGDK10.server.log CGDK10.server.time_event CGDK10.server.service CGDK10.server.mfc_controls )