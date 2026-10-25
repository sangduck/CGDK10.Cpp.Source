#*****************************************************************************
#*                                                                           *
#*                      Cho sanghyun's Game Classes II                       *
#*                      Ver 10.0 / release 2019.12.11                        *
#*                                                                           *
#*               CMake - CGDK10.system.unit_test.object.execute              *
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
PROJECT ( "CGDK10.unit_test.system.object.execute" )

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

#CGDK/License
test_coroutine.cpp
test_executable.cpp
test_executor.cpp
test_schedulable.cpp
test_time.cpp
bench_post.cpp

#io_uring completion backend (linux 6.0+ : skipped elsewhere)
test_io_context_uring_cq.cpp

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
	CGDK10.system.object_linux.${BUILD_TYPE}
	rt
	pthread
)

# Build alias for convenience
ADD_CUSTOM_TARGET(execute DEPENDS ${BUILD_NAME}_linux.${BUILD_TYPE})
