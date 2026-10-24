#*****************************************************************************
#*                                                                           *
#*                      Cho sanghyun's Game Classes II                       *
#*                      Ver 10.0 / release 2019.12.11                        *
#*                                                                           *
#*                         CMake - CGDK.net.socket                           *
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
PROJECT ( "CGDK10.net.socket" )

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
ADD_SUBDIRECTORY (./socket)

# 2) socket.crypt
ADD_SUBDIRECTORY (./socket.crypt)

# 3) socket.http
ADD_SUBDIRECTORY (./socket.http)

# 4) socket.mail
ADD_SUBDIRECTORY (./socket.mail)

# 5) socket.kafka (librdkafka: vcpkg on windows, 'librdkafka-dev' on linux)
IF (WIN32)
	ADD_SUBDIRECTORY (./socket.kafka)
ELSE ()
	FIND_PATH ( CGDK_RDKAFKA_INCLUDE_DIR librdkafka/rdkafkacpp.h )
	IF (CGDK_RDKAFKA_INCLUDE_DIR)
		ADD_SUBDIRECTORY (./socket.kafka)
	ELSE ()
		MESSAGE ( STATUS "librdkafka not found: CGDK10.net.socket.kafka is skipped" )
	ENDIF ()
ENDIF ()

# windows only
IF (WIN32)
	ADD_SUBDIRECTORY (./mfc_controls)
ENDIF ()

# 6) unit test (GoogleTest)
ADD_SUBDIRECTORY (./unit_test)

# Build alias for convenience
CGDK_ADD_ALIAS ( net.socket CGDK10.net.socket CGDK10.net.socket.crypt CGDK10.net.socket.http CGDK10.net.socket.mail CGDK10.net.socket.kafka CGDK10.net.socket.mfc_controls )