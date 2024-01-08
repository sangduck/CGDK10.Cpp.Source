#*****************************************************************************
#*                                                                           *
#*                      Cho sanghyun's Game Classes II                       *
#*                      Ver 10.0 / release 2019.12.11                        *
#*                                                                           *
#*                     CMake - CGDK10.server.query.redis                     *
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
PROJECT ( "CGDK10.server.query.redis" )

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
SET ( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../../../lib/cgdk/sdk10 )
SET ( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../../../lib/cgdk/sdk10 )
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


#-----------------------------------------------------------------------------
# source files
#-----------------------------------------------------------------------------
SET (SRC_FILES
#...
pch.cpp

#CGDK/License
${CMAKE_CURRENT_SOURCE_DIR}/../../license/license_info/license_info.cpp
${CMAKE_CURRENT_SOURCE_DIR}/../../license/license_info/license_file.cpp

#definitions
server.query.redis.definitions.protocol.cpp

#socket
net.io.packetable.resp.cpp
net.io.sender.resp.cpp
net.resp.data.cpp

#querier
server.query.redis.Iquery.cpp
server.query.redis.querier.cpp
server.query.redis.query.cpp
server.query.redis.session.cpp
)


#-----------------------------------------------------------------------------
# compile
#-----------------------------------------------------------------------------
# 1) set variable
IF (${CMAKE_BUILD_TYPE} MATCHES "Debug")
	SET (BUILD_MESSAGE "Debug")
	SET (BUILD_TYPE "Debug")
ELSEIF (${CMAKE_BUILD_TYPE} MATCHES "Debug_Dynamic")
	SET (BUILD_MESSAGE "Debug_Dynamic")
	SET (BUILD_TYPE "Debug_Dynamic")
ELSEIF (${CMAKE_BUILD_TYPE} MATCHES "Release")
	SET (BUILD_MESSAGE "Release")
	SET (BUILD_TYPE "Release")
ELSEIF (${CMAKE_BUILD_TYPE} MATCHES "Release_Dynamic")
	SET (BUILD_MESSAGE "Release_Dynamic")
	SET (BUILD_TYPE "Release_Dynamic")
ELSE ()
	MESSAGE (SEND_ERROR "'CMAKE_BUILD_TYPE' is not mathed")
ENDIF ()

# 2) messages
MESSAGE (STATUS "compile ${PROJECT_NAME}")
MESSAGE (STATUS "  - configure['${BUILD_MESSAGE}']")

# 3) configure file
#CONFIGURE_FILE ( version.h.in version.h )

# 4) add library
ADD_LIBRARY(${PROJECT_NAME}_linux.${BUILD_TYPE} STATIC ${SRC_FILES})
TARGET_PRECOMPILE_HEADERS(${PROJECT_NAME}_linux.${BUILD_TYPE} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:pch.h>")
