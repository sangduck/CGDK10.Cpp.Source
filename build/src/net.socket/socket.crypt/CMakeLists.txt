#*****************************************************************************
#*                                                                           *
#*                      Cho sanghyun's Game Classes II                       *
#*                      Ver 10.0 / release 2019.12.11                        *
#*                                                                           *
#*                       CMake - CGDK10.net.socket.crypt                     *
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
PROJECT ( "CGDK10.net.socket.crypt" )

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

# definitions
net.definitions.crypt.cpp

# net.io.connect_requestable
net.io.connect_requestable.tcp_reconnect_ssl.cpp
net.io.connect_requestable.tcp_ssl.cpp

# receivable
net.io.receivable.stream_crypt.cpp
net.io.receivable.stream_crypt.epoll.cpp

# sendable
net.io.sendable.tcp_crypt.cpp
net.io.sendable.tcp_crypt.epoll.cpp
net.io.sendable.tcp_crypt.async_gather_buffered.epoll.cpp
net.io.sendable.tcp_crypt.executable_buffer.cpp

# socket
net.io.socket_ssl.epoll.cpp

# tcp crypt
net.io.tcp_crypt.cpp

# acceptor socket
net.io.connective.acceptor_ssl.cpp
)

# 2) linux only
SET (SRC_FILES_LINUX

)

# 3) windows only
SET (SRC_FILES_WINDOWS
net.io.receivable.stream_crypt.iocp.cpp
net.io.sendable.tcp_crypt.async_gather_buffered.iocp.cpp
net.io.sendable.tcp_crypt.iocp.cpp
net.io.socket_ssl.iocp.cpp
)


#-----------------------------------------------------------------------------
# compile
#-----------------------------------------------------------------------------
# 1) library
CGDK_ADD_LIBRARY ( CGDK10.net.socket.crypt STATIC
	SOURCES         ${SRC_FILES}
	LINUX_SOURCES   ${SRC_FILES_LINUX}
	WINDOWS_SOURCES ${SRC_FILES_WINDOWS}
	PCH             pch.h
	UNICODE
	SVN_VERSION
)

# 2) build alias for convenience
IF (TARGET ${CGDK_TARGET})
	ADD_CUSTOM_TARGET ( socket.crypt DEPENDS ${CGDK_TARGET} )
ENDIF ()