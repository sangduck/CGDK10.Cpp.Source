#*****************************************************************************
#*                                                                           *
#*                      Cho sanghyun's Game Classes II                       *
#*                      Ver 10.0 / release 2019.12.11                        *
#*                                                                           *
#*                          CMake - tutorial.socket                          *
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
PROJECT ( "CGDK10.tutorial.socket" )

# 3) project version
SET ( PROJECT_VERSION_MAJOR 10 )
SET ( PROJECT_VERSION_MINOR 0 )
SET ( PROJECT_VERSION_PATCH 100 )
SET ( PROJECT_VERSION_TWEAK 1 )
SET ( DEFAULT_BUILD_TYPE Debug )


#-----------------------------------------------------------------------------
# compile
#-----------------------------------------------------------------------------
# 1) tcp_simple
ADD_SUBDIRECTORY (socket.01.tcp_simple)

# 2) tcp_send_receive
ADD_SUBDIRECTORY (socket.02.tcp_send_receive)

# 3) tcp_configure
ADD_SUBDIRECTORY (socket.03.tcp_configure)

# 4) udp_simple
ADD_SUBDIRECTORY (socket.04.udp_simple)

# 5) http
ADD_SUBDIRECTORY (socket.05.http)

# 6) websocket
ADD_SUBDIRECTORY (socket.06.websocket)

# ex1) tcp graceful close
ADD_SUBDIRECTORY (socket.ex01.tcp_graceful_close)

# ex2) tcp neet ip
ADD_SUBDIRECTORY (socket.ex02.net.ip)

# ex3) tcp reconneect
ADD_SUBDIRECTORY (socket.ex03.reconnect)

# ex4) unity 3D
# ex5) unrael 3D

# ex6) tcp protobuf
ADD_SUBDIRECTORY (socket.ex06.tcp_protobuf)

# ex7) tcp connector
ADD_SUBDIRECTORY (socket.ex07.tcp_connector)

# ex8) socket logger
ADD_SUBDIRECTORY (socket.ex08.socket_logger)

# ex9)lambda socket
ADD_SUBDIRECTORY (socket.ex09.lambda_socket)

# ex10) crypted socket
ADD_SUBDIRECTORY (socket.ex10.crypted_send_receive)

# ex11) ssl socket
ADD_SUBDIRECTORY (socket.ex11.ssl_send_receive)

# ex12) kafka (librdkafka : vcpkg on windows, 'librdkafka-dev' or -DCMAKE_PREFIX_PATH on linux)
FIND_PATH ( CGDK_RDKAFKA_INCLUDE_DIR librdkafka/rdkafkacpp.h )
IF (WIN32 OR CGDK_RDKAFKA_INCLUDE_DIR)
	ADD_SUBDIRECTORY (socket.ex12.kafka)
ELSE ()
	MESSAGE ( STATUS "librdkafka not found: socket.ex12.kafka is skipped" )
ENDIF ()

# ex13) mail (SMTP)
ADD_SUBDIRECTORY (socket.ex13.mail)

# ex14) UDS (same host)
ADD_SUBDIRECTORY (socket.ex14.uds)

# ex15) tcp socket option (TCP_NODELAY, TCP_QUICKACK, SO_BUSY_POLL, TCP_NOTSENT_LOWAT, TCP Fast Open)
ADD_SUBDIRECTORY (socket.ex15.socket_option)
# ex16) emergency acceptor and the signals (shutdown / emergency / terminate)
ADD_SUBDIRECTORY (socket.ex16.emergency_acceptor)
