#*****************************************************************************
#*                                                                           *
#*                      Cho sanghyun's Game Classes II                       *
#*                      Ver 10.0 / release 2019.12.11                        *
#*                                                                           *
#*                       CMake - CGDK10.net.socket.kafka                     *
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
PROJECT ( "CGDK10.net.socket.kafka" )

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
net.definitions.kafka.cpp

#net.io.kafka
net.io.kafka.Nclient.cpp
net.io.kafka.Nproducer.cpp
net.io.kafka.Nconsumer.cpp
)

# 2) linux only
SET (SRC_FILES_LINUX

)

# 3) windows only
SET (SRC_FILES_WINDOWS

)


#-----------------------------------------------------------------------------
# compile
#-----------------------------------------------------------------------------
# 1) library (librdkafka: vcpkg on windows, 'librdkafka-dev' on linux)
CGDK_ADD_LIBRARY ( CGDK10.net.socket.kafka STATIC
	SOURCES         ${SRC_FILES}
	LINUX_SOURCES   ${SRC_FILES_LINUX}
	WINDOWS_SOURCES ${SRC_FILES_WINDOWS}
	PCH             pch.h
	SVN_VERSION
)

# 2) librdkafka include directory (linux: found by '../CMakeLists.txt'. ex: -DCMAKE_PREFIX_PATH=<librdkafka install prefix>)
IF (TARGET ${CGDK_TARGET} AND CGDK_RDKAFKA_INCLUDE_DIR)
	TARGET_INCLUDE_DIRECTORIES ( ${CGDK_TARGET} PRIVATE "${CGDK_RDKAFKA_INCLUDE_DIR}" )
ENDIF ()

# 3) build alias for convenience
IF (TARGET ${CGDK_TARGET})
	ADD_CUSTOM_TARGET ( socket.kafka DEPENDS ${CGDK_TARGET} )
ENDIF ()
