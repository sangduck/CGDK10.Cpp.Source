#*****************************************************************************
#*                                                                           *
#*                      Cho sanghyun's Game Classes II                       *
#*                      Ver 10.0 / release 2019.12.11                        *
#*                                                                           *
#*                     CMake - CGDK.net.p2p.turn.server                      *
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

# ----------------------------------------------------------------------------
# setting
# ----------------------------------------------------------------------------
# 1) build version
CMAKE_MINIMUM_REQUIRED ( VERSION 3.20 )
CMAKE_POLICY( VERSION 3.20 )

# 2) project name
PROJECT ( "CGDK10.net.p2p.turn.server" )

# 3) project version
SET ( PROJECT_VERSION_MAJOR 10 )
SET ( PROJECT_VERSION_MINOR 0 )
SET ( PROJECT_VERSION_PATCH 100 )
SET ( PROJECT_VERSION_TWEAK 1 )

# 4) default BUILD_TYPE
IF (NOT DEFINED CMAKE_BULD_TYPE)
	SET (CMAKE_BULD_TYPE "Debug")
ENDIF()

# 5) find os platform
#    (compatible platform : 'linux_<arch>_glibc<version>_<compiler><major>' / 'windows' - see build/cmake/cgdk_platform.cmake)
INCLUDE ( ${CMAKE_CURRENT_SOURCE_DIR}/../../../../build/cmake/cgdk_platform.cmake )

# 6) directories
SET ( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../../../lib/cgdk/sdk10/${PLATFORM_OS} )
SET ( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../../../lib/cgdk/sdk10/${PLATFORM_OS} )
SET ( CMAKE_RUNTIME_OTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../../execute )
SET ( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../execute )

# 7) compile options
set ( CMAKE_CXX_STANDARD 20)
set ( CMAKE_CXX_STANDARD_REQUIRED ON)
SET ( CMAKE_CXX_FLAGS "-DC_FLAGS -fexceptions -std=c++20 -g3 -Wall -Wextra -Wno-virtual-move-assign" )
SET ( CMAKE_CXX_FLAGS_DEBUG "-D_DEBUG -Wformat=2 -Wformat-security" )
SET ( CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3 -Wformat -fnon-call-exceptions" )

# 8) include directories
INCLUDE_DIRECTORIES(
	${CMAKE_CURRENT_SOURCE_DIR}/../../../../include
	${CMAKE_CURRENT_SOURCE_DIR}/../../system/util.crypt/openssl/include
	./
)

# 9) set svn files
SET (SVN_VERSION_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../include/cgdk/sdk10")
SET (SVN_VERSION_SCRIPT "${SVN_VERSION_DIR}/svn_version.sh")
SET (SVN_VERSION_TEMPLATE "${SVN_VERSION_DIR}/svn_version.template")
SET (SVN_VERSION_OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/svn_version.h")


# ----------------------------------------------------------------------------
# source files
# ----------------------------------------------------------------------------
SET (SRC_FILES
# ...
pch.cpp

#CGDK/License
${CMAKE_CURRENT_SOURCE_DIR}/../../license/license_info/license_info.cpp
${CMAKE_CURRENT_SOURCE_DIR}/../../license/license_info/license_file.cpp

#files


)


# ----------------------------------------------------------------------------
# compile
# ----------------------------------------------------------------------------
# 1) build name
SET (BUILD_NAME ${PROJECT_NAME})

# 2) set variable
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

# 3) messages
MESSAGE (STATUS "compile ${PROJECT_NAME}")
MESSAGE (STATUS "  - configure['${BUILD_MESSAGE}']")
MESSAGE (STATUS "  - platform['${PLATFORM_OS}']")
MESSAGE (STATUS "  - output to['${CMAKE_LIBRARY_OUTPUT_DIRECTORY}']")

# 4) configure file
#CONFIGURE_FILE ( version.h.in version.h )

# 5) add library

# 5) add custom command to generate svn_version.h
ADD_CUSTOM_COMMAND (
	OUTPUT ${SVN_VERSION_OUTPUT}
	COMMAND bash ${SVN_VERSION_SCRIPT} ${CMAKE_SOURCE_DIR} ${SVN_VERSION_TEMPLATE} ${SVN_VERSION_OUTPUT}
	DEPENDS ${SVN_VERSION_TEMPLATE}
	WORKING_DIRECTORY ${SVN_VERSION_DIR}
	COMMENT "Generating svn_version.h from SVN info"
)

# 6) create target to build svn_version.h before library
ADD_CUSTOM_TARGET (${PROJECT_NAME}_generate_svn_version ALL DEPENDS ${SVN_VERSION_OUTPUT})
ADD_LIBRARY(${BUILD_NAME}_linux.${BUILD_TYPE} STATIC ${SRC_FILES})
TARGET_PRECOMPILE_HEADERS(${BUILD_NAME}_linux.${BUILD_TYPE} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:pch.h>")
add_dependencies(${BUILD_NAME}_linux.${BUILD_TYPE} ${PROJECT_NAME}_generate_svn_version)

# Build alias for convenience
ADD_CUSTOM_TARGET(server DEPENDS ${BUILD_NAME}_linux.${BUILD_TYPE})
