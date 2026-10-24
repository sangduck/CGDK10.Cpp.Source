#*****************************************************************************
#*                                                                           *
#*                      Cho sanghyun's Game Classes II                       *
#*                      Ver 10.0 / release 2019.12.11                        *
#*                                                                           *
#*                     CMake - CGDK10.net.socket.unit_test                    *
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

FIND_PACKAGE ( GTest QUIET )
IF (GTest_FOUND OR GTEST_FOUND)
	ADD_SUBDIRECTORY (./unit_test.net_socket.http)

	# - kafka (librdkafka: vcpkg on windows, 'librdkafka-dev' on linux. the mock cluster of librdkafka is used: no broker is needed)
	FIND_PATH ( CGDK_RDKAFKA_INCLUDE_DIR librdkafka/rdkafkacpp.h )
	IF (CGDK_RDKAFKA_INCLUDE_DIR)
		ADD_SUBDIRECTORY (./unit_test.net_socket.kafka)
	ELSE ()
		MESSAGE ( STATUS "librdkafka not found: unit_test.net_socket.kafka is skipped" )
	ENDIF ()
ELSE ()
	MESSAGE ( STATUS "GTest not found: net.socket unit tests are skipped" )
ENDIF ()
