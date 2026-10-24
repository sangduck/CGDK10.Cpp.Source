#*****************************************************************************
#*                                                                           *
#*                      Cho sanghyun's Game Classes II                       *
#*                      Ver 10.0 / release 2019.12.11                        *
#*                                                                           *
#*                      CMake - CGDK10.net.socket.socket                     *
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
PROJECT ( "CGDK10.net.socket.socket" )

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
net.definitions.socket.cpp

#net.api
api.net.socket.cpp

#net.emergency
net.emergency.acceptor.cpp
net.emergency.manager.cpp

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
net.io.connective.acceptor_tcp.process.cpp
net.io.connective.acceptor_tcp.uring.cpp
net.io.connective.acceptor_tcp.parameters.cpp
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
net.io.receivable.stream.epoll.cpp
net.io.receivable.stream.uring.cpp
net.io.receivable.stream_queue.epoll.cpp

# net.io.restore
net.io.restorable.client.cpp
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
net.io.sendable.tcp_async_gather.uring.cpp
net.io.sendable.tcp_async_gather_buffered.epoll.cpp
net.io.sendable.tcp_async_gather_buffered.uring.cpp
net.io.sendable.udp.epoll.cpp

# sender
net.io.sender.stream.cpp

# net.io.socket
net.io.socket.cpp
net.io.socket.tcp.cpp
net.io.socket.udp.cpp

# net.io
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
net.statistics.connective.cpp
net.statistics.socket.cpp
net.statistics.traffic.cpp
)

# 2) linux only
SET (SRC_FILES_LINUX

)

# 3) windows only
SET (SRC_FILES_WINDOWS
net.io.receivable.datagram.iocp.cpp
net.io.receivable.executable_receive.cpp
net.io.receivable.stream.iocp.cpp
net.io.receivable.stream.rio.cpp
net.io.receivable.stream_queue.iocp.cpp
net.io.receivable.stream_queue.rio.cpp
net.io.sendable.executable_send_buffer.cpp
net.io.sendable.executable_send_buffer_list.cpp
net.io.sendable.gathering_queue.cpp
net.io.sendable.gathering_queue_buffered.cpp
net.io.sendable.tcp_async_gather_buffered_raw.iocp.cpp
net.io.sendable.tcp_async_gather_raw.iocp.cpp
net.io.sendable.tcp_sync.iocp.cpp
net.io.sendable.tcp_async.rio.cpp
net.io.sendable.tcp_async_gather.iocp.cpp
net.io.sendable.tcp_async_gather.rio.cpp
net.io.sendable.tcp_async_gather_buffered.iocp.cpp
net.io.sendable.tcp_async.iocp.cpp
net.io.sendable.tcp_sync_buffered.iocp.cpp
net.io.sendable.udp.iocp.cpp
net.io.sendable.udp.rio.cpp
)


#-----------------------------------------------------------------------------
# compile
#-----------------------------------------------------------------------------
# 1) library
CGDK_ADD_LIBRARY ( CGDK10.net.socket STATIC
	SOURCES         ${SRC_FILES}
	LINUX_SOURCES   ${SRC_FILES_LINUX}
	WINDOWS_SOURCES ${SRC_FILES_WINDOWS}
	PCH             pch.h
	SVN_VERSION
)

# 2) build alias for convenience
IF (TARGET ${CGDK_TARGET})
	ADD_CUSTOM_TARGET ( socket DEPENDS ${CGDK_TARGET} )
ENDIF ()