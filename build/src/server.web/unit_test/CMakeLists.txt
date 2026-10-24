#*****************************************************************************
#*                                                                           *
#*                      Cho sanghyun's Game Classes II                       *
#*                      Ver 10.0 / release 2019.12.11                        *
#*                                                                           *
#*                            CMake - CGDK.system                            *
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
PROJECT ( "CGDK10.system" )

# 3) project version
SET ( PROJECT_VERSION_MAJOR 10 )
SET ( PROJECT_VERSION_MINOR 0 )
SET ( PROJECT_VERSION_PATCH 100 )
SET ( PROJECT_VERSION_TWEAK 1 )


#-----------------------------------------------------------------------------
# compile
#-----------------------------------------------------------------------------
# 1) system
ADD_SUBDIRECTORY (./object)

# 2) util.crypt
ADD_SUBDIRECTORY (./util.crypt)

# 3) uint.test
#ADD_SUBDIRECTORY (./unit_test.log)
#ADD_SUBDIRECTORY (./unit_test.object.bench.lock)
#ADD_SUBDIRECTORY (./unit_test.object.bench.mem_tcmalloc)
#ADD_SUBDIRECTORY (./unit_test.object.bench.new)
#ADD_SUBDIRECTORY (./unit_test.object.bench.object_pool)
#ADD_SUBDIRECTORY (./unit_test.object.buffer)
#ADD_SUBDIRECTORY (./unit_test.object.context)
#ADD_SUBDIRECTORY (./unit_test.object.execute)
#ADD_SUBDIRECTORY (./unit_test.object.ptr)
#ADD_SUBDIRECTORY (./unit_test.object.state)
# Build alias for convenience
ADD_CUSTOM_TARGET(system DEPENDS ${BUILD_NAME}_linux.${BUILD_TYPE})
