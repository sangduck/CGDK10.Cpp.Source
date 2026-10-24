#*****************************************************************************
#*                                                                           *
#*                      Cho sanghyun's Game Classes II                       *
#*                      Ver 10.0 / release 2019.12.11                        *
#*                                                                           *
#*                   CMake - svn_version.h update (linux)                    *
#*                                                                           *
#*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
#*  Best for Game Developement and Optimized for Game Developement.          *
#*                                                                           *
#*                (c) 2003. Cho Sanghyun. All right reserved.                *
#*                          http://www.CGCII.co.kr                           *
#*                                                                           *
#*****************************************************************************
#
# usage : cmake -DSOURCE=<generated file> -DTARGET=<svn_version.h> -P cgdk_update_svn_version.cmake
#
#  - replaces 'TARGET' with 'SOURCE' only when they differ except the line of
#    'SVN_TIME_NOW_STRING' (the build time). the sources including svn_version.h
#    are not recompiled when only the time is changed.
#  - 'SOURCE' is removed.
#  - 'TARGET' is replaced by a rename (a build of another build dir of the same
#    source tree may read it at the same time : never sees a half written file)
#
#-----------------------------------------------------------------------------
IF (NOT DEFINED SOURCE OR NOT DEFINED TARGET)
	MESSAGE ( FATAL_ERROR "usage: cmake -DSOURCE=<file> -DTARGET=<file> -P cgdk_update_svn_version.cmake" )
ENDIF ()

FILE ( READ "${SOURCE}" _source )
SET ( _changed TRUE )

IF (EXISTS "${TARGET}")
	FILE ( READ "${TARGET}" _target )
	STRING ( REGEX REPLACE "#define[ \t]+SVN_TIME_NOW_STRING[^\n]*" "" _source_compare "${_source}" )
	STRING ( REGEX REPLACE "#define[ \t]+SVN_TIME_NOW_STRING[^\n]*" "" _target_compare "${_target}" )
	IF ("${_source_compare}" STREQUAL "${_target_compare}")
		SET ( _changed FALSE )
	ENDIF ()
ENDIF ()

IF (_changed)
	STRING ( RANDOM LENGTH 8 _suffix )
	FILE ( WRITE "${TARGET}.${_suffix}.tmp" "${_source}" )
	FILE ( RENAME "${TARGET}.${_suffix}.tmp" "${TARGET}" )
	MESSAGE ( STATUS "[ gen] updated ${TARGET}" )
ENDIF ()

FILE ( REMOVE "${SOURCE}" )
