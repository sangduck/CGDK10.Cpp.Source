#*****************************************************************************
#*                                                                           *
#*                      Cho sanghyun's Game Classes II                       *
#*                      Ver 10.0 / release 2019.12.11                        *
#*                                                                           *
#*                                   CMake                                   *
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
PROJECT ( "CGDK10.build.src" )

# 3) project version
SET ( PROJECT_VERSION_MAJOR 10 )
SET ( PROJECT_VERSION_MINOR 0 )
SET ( PROJECT_VERSION_PATCH 100 )
SET ( PROJECT_VERSION_TWEAK 1 )

# 4) common build rules
INCLUDE ( ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/cgdk.cmake )


#-----------------------------------------------------------------------------
# compile
#-----------------------------------------------------------------------------
# 1) system
ADD_SUBDIRECTORY (system)

# 2) net.socket
ADD_SUBDIRECTORY (net.socket)

# 3) net.update
ADD_SUBDIRECTORY (net.update)

# 4) net.p2p
ADD_SUBDIRECTORY (net.p2p)

# 5) server.system
ADD_SUBDIRECTORY (server.system)

# 6) server.query
ADD_SUBDIRECTORY (server.query)

# 7) server.oauth
ADD_SUBDIRECTORY (server.http_api)

# 8) server service
ADD_SUBDIRECTORY (server.service)

# 9) server web
ADD_SUBDIRECTORY (server.web)

# 10) server admin
ADD_SUBDIRECTORY (server.admin)

# 11) link order of the DLLs (mfc_controls)
CGDK_FINALIZE ()

# 12) build alias for convenience
CGDK_ADD_ALIAS ( core system net.socket net.update net.p2p server.system server.query http_api service server.web admin )