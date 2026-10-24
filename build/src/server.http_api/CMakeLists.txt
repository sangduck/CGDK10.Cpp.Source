#*****************************************************************************
#*                                                                           *
#*                      Cho sanghyun's Game Classes II                       *
#*                      Ver 10.0 / release 2019.12.11                        *
#*                                                                           *
#*                      CMake - CGDK.server.http_api                         *
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
PROJECT ( "CGDK10.server.http_api" )

# 3) project version
SET ( PROJECT_VERSION_MAJOR 10 )
SET ( PROJECT_VERSION_MINOR 0 )
SET ( PROJECT_VERSION_PATCH 100 )
SET ( PROJECT_VERSION_TWEAK 1 )

# 4) common build rules
INCLUDE ( ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/cgdk.cmake )


#-----------------------------------------------------------------------------
# compile
#-----------------------------------------------------------------------------
# 1) oauth
#ADD_SUBDIRECTORY (./oauth)
ADD_SUBDIRECTORY (./bank)
ADD_SUBDIRECTORY (./kakao)
ADD_SUBDIRECTORY (./line)
ADD_SUBDIRECTORY (./naver)
ADD_SUBDIRECTORY (./google)
ADD_SUBDIRECTORY (./apple)

# Build alias for convenience
CGDK_ADD_ALIAS ( http_api CGDK10.server.http_api.bank CGDK10.server.http_api.kakao CGDK10.server.http_api.line CGDK10.server.http_api.naver CGDK10.server.http_api.google CGDK10.server.http_api.apple )