#*****************************************************************************
#*                                                                           *
#*                      Cho sanghyun's Game Classes II                       *
#*                      Ver 10.0 / release 2019.12.11                        *
#*                                                                           *
#*                      CMake - CGDK10.system.unit_test                      *
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
# unit tests (system)
#  - built only when GoogleTest is found (ex: 'dnf install gtest-devel', 'apt install libgtest-dev')
#  - executables are written to 'build/src/unit_test'
#-----------------------------------------------------------------------------
FIND_PACKAGE ( GTest QUIET )

IF (GTest_FOUND OR GTEST_FOUND)
	ADD_SUBDIRECTORY (./unit_test.object.buffer)
	ADD_SUBDIRECTORY (./unit_test.object.context)
	ADD_SUBDIRECTORY (./unit_test.object.execute)
	ADD_SUBDIRECTORY (./unit_test.object.ptr)
	ADD_SUBDIRECTORY (./unit_test.object.state)
	ADD_SUBDIRECTORY (./unit_test.object.dump)
ELSE ()
	MESSAGE ( STATUS "GTest not found: system unit tests are skipped" )
ENDIF ()
