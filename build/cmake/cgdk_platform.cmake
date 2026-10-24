#*****************************************************************************
#*                                                                           *
#*                      Cho sanghyun's Game Classes II                       *
#*                      Ver 10.0 / release 2019.12.11                        *
#*                                                                           *
#*                  CMake - platform (library directory name)                *
#*                                                                           *
#*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
#*                (c) 2016 Cho Sanghyun All right reserved.                  *
#*                          http://www.CGCII.co.kr                           *
#*                                                                           *
#*****************************************************************************
#
# platform name of the library directory (lib/cgdk/sdk10/<platform>)
#  - the libraries in the same directory are compatible (can be linked and executed together)
#
#    windows : "windows"
#    linux   : "linux_<arch>_<libc><version>_<compiler><major version>"
#              ex) linux_x86_64_glibc2.39_gcc14 : CentOS Stream 10, Rocky Linux 10, AlmaLinux 10
#                  linux_x86_64_glibc2.39_gcc13 : Ubuntu 24.04
#                  linux_x86_64_glibc2.43_gcc15 : Ubuntu 26.04
#      - arch     : x86_64 / aarch64 ...
#      - libc     : objects built with a newer glibc can't be linked / executed with an older glibc
#      - compiler : the ABI of the C++20 standard library can be changed between the major versions
#
# variables)
#  - CGDK_PLATFORM_OS     : platform name (above)
#  - CGDK_PLATFORM_DISTRO : ID of /etc/os-release (ubuntu, centos ...) (linux only)
#  - PLATFORM_OS          : same as CGDK_PLATFORM_OS (the name used by the CMakeLists.txt of tutorials / examples)
#
# * must be included after PROJECT() (the C++ compiler must be known)
#
#-----------------------------------------------------------------------------

# 1) evaluate once per configure (stored as global properties)
GET_PROPERTY ( _cgdk_platform_os GLOBAL PROPERTY CGDK_PLATFORM_OS )

IF (NOT _cgdk_platform_os)
	IF (WIN32)
		SET ( _cgdk_platform_os "windows" )
		SET ( _cgdk_platform_distro "" )

	ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Linux")
		# - arch
		STRING ( TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _arch )
		IF (_arch STREQUAL "amd64")
			SET ( _arch "x86_64" )
		ELSEIF (_arch STREQUAL "arm64")
			SET ( _arch "aarch64" )
		ENDIF ()

		# - libc ("glibc 2.39")
		EXECUTE_PROCESS (
			COMMAND getconf GNU_LIBC_VERSION
			OUTPUT_VARIABLE _libc
			RESULT_VARIABLE _result
			OUTPUT_STRIP_TRAILING_WHITESPACE
			ERROR_QUIET )
		IF (_result EQUAL 0 AND _libc MATCHES "^glibc ([0-9]+\\.[0-9]+)")
			SET ( _libc "glibc${CMAKE_MATCH_1}" )
		ELSE ()
			EXECUTE_PROCESS ( COMMAND ldd --version OUTPUT_VARIABLE _ldd ERROR_VARIABLE _ldd )
			IF (_ldd MATCHES "musl")
				SET ( _libc "musl" )
			ELSE ()
				SET ( _libc "libc" )
			ENDIF ()
		ENDIF ()

		# - compiler (gcc14, clang19 ...)
		IF (NOT CMAKE_CXX_COMPILER_ID)
			MESSAGE ( FATAL_ERROR "cgdk_platform.cmake must be included after PROJECT() (the C++ compiler is unknown)" )
		ENDIF ()
		IF (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
			SET ( _compiler "gcc" )
		ELSEIF (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
			SET ( _compiler "clang" )
		ELSE ()
			STRING ( TOLOWER "${CMAKE_CXX_COMPILER_ID}" _compiler )
		ENDIF ()
		STRING ( REGEX MATCH "^[0-9]+" _compiler_major "${CMAKE_CXX_COMPILER_VERSION}" )

		SET ( _cgdk_platform_os "linux_${_arch}_${_libc}_${_compiler}${_compiler_major}" )

		# - distro (ID of /etc/os-release)
		SET ( _cgdk_platform_distro "" )
		IF (EXISTS "/etc/os-release")
			FILE ( STRINGS "/etc/os-release" _id REGEX "^ID=" )
			STRING ( REGEX REPLACE "^ID=\"?([^\"]*)\"?$" "\\1" _cgdk_platform_distro "${_id}" )
		ENDIF ()

	ELSE ()
		MESSAGE ( FATAL_ERROR "unsupported OS: ${CMAKE_SYSTEM_NAME}" )
	ENDIF ()

	SET_PROPERTY ( GLOBAL PROPERTY CGDK_PLATFORM_OS "${_cgdk_platform_os}" )
	SET_PROPERTY ( GLOBAL PROPERTY CGDK_PLATFORM_DISTRO "${_cgdk_platform_distro}" )
	MESSAGE ( STATUS "CGDK platform: ${_cgdk_platform_os} (${_cgdk_platform_distro})" )
ENDIF ()

# 2) variables
GET_PROPERTY ( CGDK_PLATFORM_OS GLOBAL PROPERTY CGDK_PLATFORM_OS )
GET_PROPERTY ( CGDK_PLATFORM_DISTRO GLOBAL PROPERTY CGDK_PLATFORM_DISTRO )
SET ( PLATFORM_OS "${CGDK_PLATFORM_OS}" )
