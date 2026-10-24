#*****************************************************************************
#*                                                                           *
#*                      Cho sanghyun's Game Classes II                       *
#*                      Ver 10.0 / release 2019.12.11                        *
#*                                                                           *
#*                         CMake - tutorial.execute                          *
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
PROJECT ( "CGDK10.tutorial.execute" )

# 3) project version
SET ( PROJECT_VERSION_MAJOR 10 )
SET ( PROJECT_VERSION_MINOR 0 )
SET ( PROJECT_VERSION_PATCH 100 )
SET ( PROJECT_VERSION_TWEAK 1 )
SET ( DEFAULT_BUILD_TYPE Debug )


#-----------------------------------------------------------------------------
# compile
#-----------------------------------------------------------------------------
# 1) time
ADD_SUBDIRECTORY (execute.01.time)

# 2) lock
ADD_SUBDIRECTORY (execute.02.lock)

# 3) executor
ADD_SUBDIRECTORY (execute.03.executor)

# 4) schedulable
ADD_SUBDIRECTORY (execute.04.schedulable)

# 5) strand
ADD_SUBDIRECTORY (execute.05.strand)
ADD_SUBDIRECTORY (execute.06.coroutine)

# 6) force raise
ADD_SUBDIRECTORY (execute.ex01.force_raise)

# 7) crash dump
ADD_SUBDIRECTORY (execute.ex02.crash_dump)

# 8) snap dump
ADD_SUBDIRECTORY (execute.ex03.snap_dump)

# 9) setting (json)
ADD_SUBDIRECTORY (execute.ex04.setting)
