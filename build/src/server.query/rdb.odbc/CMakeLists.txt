#*****************************************************************************
#*                                                                           *
#*                      Cho sanghyun's Game Classes II                       *
#*                      Ver 10.0 / release 2019.12.11                        *
#*                                                                           *
#*                     CMake - CGDK10.server.query.odbc                      *
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
PROJECT ( "CGDK10.server.query.rdb.odbc" )

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

#...
pch.cpp

#common
${CGDK_COMMON_DIR}/cgdk_info.cpp

#definitions
server.query.rdb.odbc.definitions.cpp

# manager
server.query.rdb.odbc.manager.cpp

# data source &  session
server.query.rdb.odbc.data_source.cpp
server.query.rdb.odbc.session.cpp

#querier
server.query.rdb.odbc.querier.cpp

# query
server.query.rdb.odbc.query_ODBC_GET_VALUE.cpp
server.query.rdb.odbc.Iquery.cpp
server.query.rdb.odbc.Nquery.cpp

# scoped transaction
server.query.rdb.odbc.transaction_guard.cpp

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
# 1) library
CGDK_ADD_LIBRARY ( CGDK10.server.query.rdb.odbc STATIC
	SOURCES         ${SRC_FILES}
	LINUX_SOURCES   ${SRC_FILES_LINUX}
	WINDOWS_SOURCES ${SRC_FILES_WINDOWS}
	PCH             pch.h
	SVN_VERSION
)

# 2) build alias for convenience
IF (TARGET ${CGDK_TARGET})
	ADD_CUSTOM_TARGET ( rdb.odbc DEPENDS ${CGDK_TARGET} )
ENDIF ()