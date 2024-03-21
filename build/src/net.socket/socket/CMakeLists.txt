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

# ----------------------------------------------------------------------------
# setting
# ----------------------------------------------------------------------------
# 1) project name
PROJECT ( "CGDK10.net.socket" )

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
SET ( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../../../lib/cgdk/sdk10/${DIST} )
SET ( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../../../lib/cgdk/sdk10/${DIST} )
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


# ----------------------------------------------------------------------------
# source files
# ----------------------------------------------------------------------------
SET (SRC_FILES
# ...
pch.cpp

#common
${CMAKE_CURRENT_SOURCE_DIR}/../../common/cgdk_info.cpp

#definitions
net.definitions.socket.cpp

#net.api
api.net.socket.cpp

#net.io.interfaces
net.io.Ibinder.cpp
net.io.Ireconnectable.cpp
net.io.Irestorable.cpp
net.io.Irestore_server.cpp
net.io.Isocket.cpp

#net.io.add_on
net.io.add_on.keep_alive.cpp
net.io.add_on.manager_socket_sustain.cpp
net.io.add_on.socket_sustain.cpp

#net.net.io.binder
net.io.binder.manager.cpp
net.io.binder.tcp.cpp
net.io.binder.udp.cpp

#net.io.connect_requestable
net.io.connect_requestable.tcp.cpp
net.io.connect_requestable.tcp_reconnect.cpp
net.io.connect_requestable.tcp_reconnect.manager_reconnector.cpp

# net.io.connectable
net.io.connectable.disconnect_manager.cpp
net.io.connectable.tcp.cpp

# net.io.connective
net.io.connective.acceptor_tcp.cpp
net.io.connective.connector_tcp.cpp
net.io.connective.manager.cpp

# net.io.packetable
net.io.packetable.datagram.cpp
net.io.packetable.datagram_response.cpp
net.io.packetable.stream_json.cpp

# net.io.messageable
net.io.messageable.queue.cpp

# net.io.receivable
net.io.receivable.datagram.epoll.cpp
net.io.receivable.stream.cpp
net.io.receivable.stream_crypt.cpp
net.io.receivable.stream_crypt.epoll.cpp
net.io.receivable.stream.epoll.cpp
net.io.receivable.stream_queue.epoll.cpp

# net.io.restore
net.io.restorable.clinet.cpp
net.io.restorable.server.cpp
net.io.restore_manager.cpp
net.io.retransferable.cpp

# net.io.send_request
net.io.send_request.udp_resend.cpp

# neti.ip.sendable
net.io.sendable.Itcp_gather.cpp
net.io.sendable.tcp_sync.epoll.cpp
net.io.sendable.tcp_sync_buffered.epoll.cpp
net.io.sendable.tcp_async.epoll.cpp
net.io.sendable.tcp_async_gather.epoll.cpp
net.io.sendable.tcp_async_gather_buffered.epoll.cpp
net.io.sendable.tcp_crypt.cpp
net.io.sendable.tcp_crypt.epoll.cpp
net.io.sendable.udp.epoll.cpp

# sender
net.io.sender.stream.cpp

# net.io.socket
net.io.socket.cpp
net.io.socket.tcp.cpp
net.io.socket.udp.cpp
net.io.socket_secure.epoll.cpp

# net.io
net.io.tcp_client_crypt.cpp
net.io.tcp_server_crypt.cpp
net.io.udp_resend.cpp

# net.ip
net.ip.address_v4.cpp
net.ip.address_v6.cpp
net.ip.network_v4.cpp
net.ip.network_v6.cpp

# send queue
net.send_queue.cpp

# message
net.message.cpp

#net.io.Statistics
net.statistics.cpp


)


# ----------------------------------------------------------------------------
# compile
# ----------------------------------------------------------------------------
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
