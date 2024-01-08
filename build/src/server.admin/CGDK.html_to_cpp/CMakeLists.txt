#*****************************************************************************
#*                                                                           *
#*                      Cho sanghyun's Game Classes II                       *
#*                       Ver 10.0 / release 2019.12.11                       *
#*                                                                           *
#*               CMake - CGDK10.server.admin.host_agent.service              *
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
# 1) project name
PROJECT ( "CGDK10.html_to_cpp" )

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
SET ( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/cgdk/sdk10 )
SET ( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/cgdk/sdk10 )
SET ( CMAKE_RUNTIME_OTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../../execute )
SET ( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../execute )

# 6) compile options
set ( CMAKE_CXX_STANDARD 20)
set ( CMAKE_CXX_STANDARD_REQUIRED ON)
SET ( CMAKE_CXX_FLAGS "-DC_FLAGS -fexceptions -std=c++20" )
SET ( CMAKE_CXX_FLAGS_DEBUG "-D_DEBUG -g3 -Wall -Wextra" )
SET ( CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3 -Wall -Wextra" )

# 7) include directories
INCLUDE_DIRECTORIES(
	${CMAKE_CURRENT_SOURCE_DIR}/../../../include
	./
)

# 8) library directories
LINK_DIRECTORIES(
	${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/cgdk/sdk10
)

#-----------------------------------------------------------------------------
# source files
#-----------------------------------------------------------------------------
SET (SRC_FILES
#...
CGDK.html_to_cpp.cpp
)


#-----------------------------------------------------------------------------
# compile
#-----------------------------------------------------------------------------
# 1) messages
MESSAGE (STATUS "compile ${PROJECT_NAME}")

# 2) compile
ADD_EXECUTABLE(${PROJECT_NAME} ${SRC_FILES})

# 3) configure file
#CONFIGURE_FILE ( version.h.in version.h )

# 4) add library
TARGET_LINK_LIBRARIES( ${PROJECT_NAME}
	rt
)
