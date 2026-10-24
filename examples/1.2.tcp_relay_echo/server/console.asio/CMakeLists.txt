#*****************************************************************************
#*                                                                           *
#*                      Cho sanghyun's Game Classes II                       *
#*                      Ver 10.0 / release 2019.12.11                        *
#*                                                                           *
#*         CMake - CGDK10.example.tcp_relay_echo.server.asio.console         *
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
PROJECT ( "CGDK10.tcp_relay_echo.server.asio.console" )

# 3) project version
SET ( PROJECT_VERSION_MAJOR 10 )
SET ( PROJECT_VERSION_MINOR 0 )
SET ( PROJECT_VERSION_PATCH 100 )
SET ( PROJECT_VERSION_TWEAK 1 )
SET ( DEFAULT_BUILD_TYPE Debug )

# 4) default BUILD_TYPE
IF (NOT DEFINED CMAKE_BULD_TYPE)
	SET (CMAKE_BULD_TYPE "Debug")
ENDIF()

# 5) find os platform
#    (compatible platform : 'linux_<arch>_glibc<version>_<compiler><major>' / 'windows' - see build/cmake/cgdk_platform.cmake)
INCLUDE ( ${CMAKE_CURRENT_SOURCE_DIR}/../../../../build/cmake/cgdk_platform.cmake )

# 6) asio
SET(Boost_USE_STATIC_LIBS OFF)
SET(Boost_USE_MULTITHREAD ON)
FIND_PACKAGE(Boost REQUIRED CONFIG)
IF(Boost_FOUND)
  INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
  LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
ENDIF(Boost_FOUND)
IF(TARGET Boost::headers)
  SET(USED_LIBS Boost::headers)
ELSEIF(TARGET Boost::boost)
  SET(USED_LIBS Boost::boost)
ELSE()
  SET(USED_LIBS)
ENDIF()

# 7) directories
SET ( CMAKE_RUNTIME_OTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../execute/server)
SET ( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../execute/server)

# 8) compile options
set ( CMAKE_CXX_STANDARD 20)
set ( CMAKE_CXX_STANDARD_REQUIRED ON)
SET ( CMAKE_CXX_FLAGS "-DC_FLAGS -fexceptions -std=c++20 -g -Wall -Wextra" )
SET ( CMAKE_CXX_FLAGS_DEBUG "-D_DEBUG -Wformat=2 -Wformat-security" )
SET ( CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3 -Wformat -fnon-call-exceptions" )


# ----------------------------------------------------------------------------
# source files
# ----------------------------------------------------------------------------
SET (SRC_FILES

# core (asio)
asio/asio.Nstatistics.cpp
asio/asio.Nconnective.cpp
asio/asio.Nconnector.cpp
asio/asio.Nacceptor.cpp
asio/asio.Isocket_tcp.cpp
asio/asio.Nsocket_tcp.cpp
asio/asio.Nsocket_tcp_client.cpp
asio/asio.system.cpp

# util linux
util.linux.cpp

# socket and group
socket_test.cpp

# echo server console
CGDK10.tcp_relay_echo.server.asio.console.cpp
)


# ----------------------------------------------------------------------------
# compile
# ----------------------------------------------------------------------------
IF (${CMAKE_BUILD_TYPE} MATCHES "Debug")
	SET (BUILD_MESSAGE "Debug")
	SET (BUILD_TYPE "Debug")
ELSEIF (${CMAKE_BUILD_TYPE} MATCHES "Release")
	SET (BUILD_MESSAGE "Release")
	SET (BUILD_TYPE "Release")
ELSE ()
	message (SEND_ERROR "'CMAKE_BUILD_TYPE' is not mathed")
ENDIF ()

# 2) messages
MESSAGE (STATUS "compile ${PROJECT_NAME}")
MESSAGE (STATUS "  - configure['${BUILD_MESSAGE}']")
MESSAGE (STATUS "  - platform['${PLATFORM_OS}']")
MESSAGE (STATUS "  - output to['${EXECUTABLE_OUTPUT_PATH}']")

# 3) executable
ADD_EXECUTABLE(${PROJECT_NAME}.${BUILD_TYPE} ${SRC_FILES})

# 4) link
TARGET_LINK_LIBRARIES( ${PROJECT_NAME}.${BUILD_TYPE}
	${USED_LIBS}
	rt
	pthread
)
