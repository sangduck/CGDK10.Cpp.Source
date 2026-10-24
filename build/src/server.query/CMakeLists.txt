#*****************************************************************************
#*                                                                           *
#*                      Cho sanghyun's Game Classes II                       *
#*                      Ver 10.0 / release 2019.12.11                        *
#*                                                                           *
#*                        CMake - CGDK.server.query                          *
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
PROJECT ( "CGDK10.server.query" )

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
ADD_SUBDIRECTORY (./query)
ADD_SUBDIRECTORY (./rdb.odbc)
ADD_SUBDIRECTORY (./redis)
ADD_SUBDIRECTORY (./http)

# 2) unit test (built only when GoogleTest is found)
ADD_SUBDIRECTORY (./unit_test)

# windows only
IF (WIN32)
	ADD_SUBDIRECTORY (./mfc_controls)
ENDIF ()

# Build alias for convenience
CGDK_ADD_ALIAS ( server.query CGDK10.server.query CGDK10.server.query.rdb.odbc CGDK10.server.query.redis CGDK10.server.query.http CGDK10.server.kafka CGDK10.server.query.mfc_controls )