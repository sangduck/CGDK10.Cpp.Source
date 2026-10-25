#*****************************************************************************
#*                                                                           *
#*                      Cho sanghyun's Game Classes II                       *
#*                      Ver 10.0 / release 2019.12.11                        *
#*                                                                           *
#*                 CMake - CGDK10.unit_test.net_socket.http                       *
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
CMAKE_POLICY( VERSION 3.20 )

# 2) project name
PROJECT ( "CGDK10.unit_test.net_socket.http" )

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
INCLUDE ( ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../build/cmake/cgdk_platform.cmake )

# 6) directories
SET ( CMAKE_RUNTIME_OTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../../unit_test )
SET ( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../unit_test )

# 7) compile options
set ( CMAKE_CXX_STANDARD 20)
set ( CMAKE_CXX_STANDARD_REQUIRED ON)
SET ( CMAKE_CXX_FLAGS "-DC_FLAGS -fexceptions -std=c++20 -g -Wall -Wextra -Wno-virtual-move-assign" )
SET ( CMAKE_CXX_FLAGS_DEBUG "-D_DEBUG -Wformat=2 -Wformat-security" )
SET ( CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3 -Wformat -fnon-call-exceptions" )

# 8) include directories
INCLUDE_DIRECTORIES(
	${CMAKE_CURRENT_SOURCE_DIR}/../../../../../include
	./
	${GTEST_INCLUDE_DIRS}
)

# 9) library directories
LINK_DIRECTORIES(
	${CMAKE_CURRENT_SOURCE_DIR}/../../../../../lib/cgdk/sdk10/${PLATFORM_OS}
)

# 10) requires
FIND_PACKAGE(GTest REQUIRED)


#-----------------------------------------------------------------------------
# source files
#-----------------------------------------------------------------------------
SET (SRC_FILES
#pre-compiled header
pch.cpp

#http / websocket parser
test_http_parser.cpp

#tcp message framing
test_tcp_packet.cpp

#ssl loopback (a self-signed certificate made by the test)
test_ssl_loopback.cpp

#tcp_crypt (pn_code) : concurrent senders, encrypted data in the receive of ACCEPT_CERTIFICATION, reconnect after a close while sending
test_tcp_crypt.cpp

#http client response timeout
test_http_client_timeout.cpp

#UDS (Unix domain socket, linux)
test_uds.cpp

#the io_context of an accepted socket (the hook, the io_context of the acceptor, the system executor)
test_acceptor_io_context.cpp

#linux socket performance features (recv buffer, MSG_ZEROCOPY, UDP recvmmsg/sendmmsg, cpu affinity, TCP_DEFER_ACCEPT, io_uring)
test_linux_io.cpp

#linux socket benchmarks (CGDK_BENCH=<seconds> : epoll / io_uring echo throughput)
bench_linux_io.cpp

#tcp socket option (TCP_NODELAY, TCP_QUICKACK, SO_BUSY_POLL, TCP_NOTSENT_LOWAT, TCP Fast Open)
test_socket_option.cpp

#from_string (address string -> sockaddr : "any", "localhost", the port)
test_sockaddr_from_string.cpp

#connect_requestable : a failed connect (immediate / later), reconnection after it
test_connect_requestable.cpp
test_emergency.cpp

#a handler that blocks does not hold the other sockets (io_uring completion : the ring of a blocked worker is lent)
test_blocking_handler.cpp

#io_uring completion : the send path (the SEND ops : a full socket buffer, send then close, close while sending, the send queue limit)
test_uring_send.cpp

#send_defer auto : the sends of an executable run by the executor (one socket after another, close, exception, other threads)
test_send_defer_auto.cpp

#linux UDP send : the wait for EPOLLOUT (EAGAIN injected : sendto / sendmmsg of this executable are replaced)
test_udp_send_epollout.cpp

#linux TCP receive : the data that came with the FIN is received before the socket is closed
test_tcp_receive_at_close.cpp

#linux acceptor : accept with no fd left (EMFILE : the pending connections are closed, the workers do not spin)
test_accept_fd_exhausted.cpp

#linux : the io backend an io_context got is logged (io_uring enabled / not used / disabled)
test_io_backend_log.cpp

#linux epoll / io_uring readiness : the registration mode of a TCP socket (EPOLLET for small receives, EPOLLONESHOT for large ones)
test_epoll_receive_mode.cpp

#linux epoll / io_uring readiness : an EPOLLOUT posted while the socket processes an event comes (the registered mask was not cleared)
test_epollout_rearm.cpp
)


#-----------------------------------------------------------------------------
# compile
#-----------------------------------------------------------------------------
# 1) build name
SET (BUILD_NAME ${PROJECT_NAME})

# 2) set variable
IF (${CMAKE_BUILD_TYPE} MATCHES "Debug")
	SET (BUILD_MESSAGE "Debug")
	SET (BUILD_TYPE "Debug")
ELSEIF (${CMAKE_BUILD_TYPE} MATCHES "Release")
	SET (BUILD_MESSAGE "Release")
	SET (BUILD_TYPE "Release")
ELSE ()
	MESSAGE (SEND_ERROR "'CMAKE_BUILD_TYPE' is not mathed")
ENDIF ()

# 3) messages
MESSAGE (STATUS "compile ${PROJECT_NAME}")
MESSAGE (STATUS "  - configure['${BUILD_MESSAGE}']")
MESSAGE (STATUS "  - platform['${PLATFORM_OS}']")
MESSAGE (STATUS "  - output to['${EXECUTABLE_OUTPUT_PATH}']")

# 4) executable
ADD_EXECUTABLE(${BUILD_NAME}.${BUILD_TYPE} ${SRC_FILES})

# 5) link
TARGET_LINK_LIBRARIES( ${BUILD_NAME}.${BUILD_TYPE}
	${GTEST_LIBRARIES}
	${GTEST_MAIN_LIBRARIES}
	CGDK10.net.socket.http_linux.${BUILD_TYPE}
	CGDK10.net.socket.crypt_linux.${BUILD_TYPE}
	CGDK10.net.socket_linux.${BUILD_TYPE}
	CGDK10.system.object_linux.${BUILD_TYPE}
	CGDK10.system.util.crypt_linux.${BUILD_TYPE}
	crypto
	ssl
	rt
	pthread
)

# 6) build the libraries first
FOREACH (DEPEND_TARGET CGDK10.net.socket.http CGDK10.net.socket.crypt CGDK10.net.socket CGDK10.system.object CGDK10.system.util.crypt)
	IF (TARGET ${DEPEND_TARGET})
		ADD_DEPENDENCIES ( ${BUILD_NAME}.${BUILD_TYPE} ${DEPEND_TARGET} )
	ENDIF ()
ENDFOREACH ()

# Build alias for convenience
ADD_CUSTOM_TARGET(net_socket_http_test DEPENDS ${BUILD_NAME}_linux.${BUILD_TYPE})
