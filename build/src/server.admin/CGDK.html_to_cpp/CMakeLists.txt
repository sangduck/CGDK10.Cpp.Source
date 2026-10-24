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
# 1) build version
CMAKE_MINIMUM_REQUIRED ( VERSION 3.20 )
CMAKE_POLICY ( VERSION 3.20 )

# 2) project name
PROJECT ( "CGDK10.html_to_cpp" )

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
SET (SRC_FILES
#...
CGDK.html_to_cpp.cpp
)


#-----------------------------------------------------------------------------
# compile
#-----------------------------------------------------------------------------
# 1) executable (output: build/execute)
CGDK_ADD_EXECUTABLE ( ${PROJECT_NAME}
	SOURCES     ${SRC_FILES}
	UNICODE
	LINUX_LIBS  rt
)

# 2) windows: same file name as the vcxproj (CGDK.html_to_cpp.exe), latest c++ standard
IF (WIN32)
	SET_TARGET_PROPERTIES ( ${PROJECT_NAME} PROPERTIES OUTPUT_NAME "CGDK.html_to_cpp" )
	TARGET_COMPILE_OPTIONS ( ${PROJECT_NAME} PRIVATE /std:c++latest )
ENDIF ()

# 3) build alias for convenience
ADD_CUSTOM_TARGET ( html_to_cpp DEPENDS ${PROJECT_NAME} )