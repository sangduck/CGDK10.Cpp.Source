#*****************************************************************************
#*                                                                           *
#*                      Cho sanghyun's Game Classes II                       *
#*                      Ver 10.0 / release 2019.12.11                        *
#*                                                                           *
#*                     CMake - common build rules (cgdk)                     *
#*                                                                           *
#*  Shared by every CMakeLists.txt under build/src.                          *
#*  Builds on Linux (gcc/clang, single-config) and Windows (MSVC,            *
#*  Visual Studio multi-config) with the same output names/paths as the      *
#*  hand-written vcxproj files.                                              *
#*                                                                           *
#*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
#*                (c) 2016 Cho Sanghyun All right reserved.                  *
#*                          http://www.CGCII.co.kr                           *
#*                                                                           *
#*****************************************************************************
INCLUDE_GUARD ( GLOBAL )

#-----------------------------------------------------------------------------
# 1) directories
#-----------------------------------------------------------------------------
GET_FILENAME_COMPONENT ( CGDK_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE )
SET ( CGDK_BUILD_SRC_DIR  "${CGDK_ROOT}/build/src" )
SET ( CGDK_INCLUDE_DIR    "${CGDK_ROOT}/include" )
SET ( CGDK_COMMON_DIR     "${CGDK_BUILD_SRC_DIR}/common" )
SET ( CGDK_EXECUTE_DIR    "${CGDK_ROOT}/build/execute" )
SET ( CGDK_DLL_DIR        "${CGDK_ROOT}/dll" )
SET ( CGDK_SVN_VERSION_DIR "${CGDK_INCLUDE_DIR}/cgdk/sdk10" )

#-----------------------------------------------------------------------------
# 2) platform
#-----------------------------------------------------------------------------
#    - windows : "windows"
#    - linux   : "linux_<arch>_glibc<version>_<compiler><major>" (compatible libraries : see cgdk_platform.cmake)
INCLUDE ( ${CMAKE_CURRENT_LIST_DIR}/cgdk_platform.cmake )
SET ( CGDK_LIB_DIR "${CGDK_ROOT}/lib/cgdk/sdk10/${CGDK_PLATFORM_OS}" )
# - windows: libraries of each platform toolset are separated (lib/cgdk/sdk10/windows/v143, v145 ... see include/cgdk/sdk10/lib_path.h)
IF (MSVC AND DEFINED MSVC_TOOLSET_VERSION)
	SET ( CGDK_LIB_DIR "${CGDK_LIB_DIR}/v${MSVC_TOOLSET_VERSION}" )
ENDIF ()

# - alias of the old directory name (lib/cgdk/sdk10/<distro> -> <platform>)
#   for the projects using the distro name (ID of /etc/os-release) as the library directory
#   * an existing directory of the old layout (not empty) is not replaced (warning)
IF (CGDK_PLATFORM_DISTRO)
	SET ( _alias "${CGDK_ROOT}/lib/cgdk/sdk10/${CGDK_PLATFORM_DISTRO}" )
	FILE ( MAKE_DIRECTORY "${CGDK_LIB_DIR}" )

	IF (IS_SYMLINK "${_alias}")
		FILE ( READ_SYMLINK "${_alias}" _alias_target )
		IF (NOT _alias_target STREQUAL CGDK_PLATFORM_OS)
			FILE ( REMOVE "${_alias}" )
		ENDIF ()
	ELSEIF (IS_DIRECTORY "${_alias}")
		FILE ( GLOB _alias_items "${_alias}/*" )
		IF (_alias_items)
			MESSAGE ( WARNING "'${_alias}' is a directory of the old layout : remove it to make it an alias of '${CGDK_PLATFORM_OS}'" )
		ELSE ()
			FILE ( REMOVE_RECURSE "${_alias}" )
		ENDIF ()
	ENDIF ()

	IF (NOT EXISTS "${_alias}" AND NOT IS_SYMLINK "${_alias}")
		FILE ( CREATE_LINK "${CGDK_PLATFORM_OS}" "${_alias}" SYMBOLIC )
	ENDIF ()
ENDIF ()

#-----------------------------------------------------------------------------
# 3) configurations
#    - Visual Studio : multi-config (Debug;Release)
#    - Linux(make)   : CMAKE_BUILD_TYPE (Debug/Release, default Debug)
#-----------------------------------------------------------------------------
GET_PROPERTY ( CGDK_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG )
IF (CGDK_MULTI_CONFIG)
	SET ( CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE )
	SET ( BUILD_TYPE "$<CONFIG>" )
ELSE ()
	IF (NOT CMAKE_BUILD_TYPE)
		SET ( CMAKE_BUILD_TYPE "Debug" CACHE STRING "Debug or Release" FORCE )
	ENDIF ()
	IF (NOT CMAKE_BUILD_TYPE MATCHES "^(Debug|Release|Debug_Dynamic|Release_Dynamic)$")
		MESSAGE ( SEND_ERROR "'CMAKE_BUILD_TYPE' is not mathed (${CMAKE_BUILD_TYPE})" )
	ENDIF ()
	SET ( BUILD_TYPE ${CMAKE_BUILD_TYPE} )
ENDIF ()

SET ( CMAKE_CXX_STANDARD 20 )
SET ( CMAKE_CXX_STANDARD_REQUIRED ON )

# debug configurations ('Debug', 'Debug_Dynamic')
SET ( CGDK_IS_DEBUG "$<IN_LIST:$<CONFIG>,Debug;Debug_Dynamic>" )

# MSVC: warning level / exception model are set per target (/W4, /EHa)
IF (MSVC)
	STRING ( REGEX REPLACE "/W[0-4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" )
	STRING ( REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" )
	STRING ( REGEX REPLACE "/W[0-4]" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" )
ENDIF ()

#-----------------------------------------------------------------------------
# 4) windows: vcpkg (manifest at repository root, same folder the vcxproj use)
#-----------------------------------------------------------------------------
IF (WIN32)
	IF (NOT DEFINED CGDK_VCPKG_TRIPLET)
		SET ( CGDK_VCPKG_TRIPLET "x64-windows" )
	ENDIF ()
	SET ( CGDK_VCPKG_DIR "${CGDK_ROOT}/vcpkg_installed/${CGDK_VCPKG_TRIPLET}" CACHE PATH "vcpkg installed tree (include/lib/debug/lib)" )
	IF (NOT EXISTS "${CGDK_VCPKG_DIR}/include")
		MESSAGE ( WARNING "vcpkg packages not found at '${CGDK_VCPKG_DIR}'. Build once with the vcxproj (or run 'vcpkg install' at ${CGDK_ROOT}) first." )
	ENDIF ()
	FIND_PROGRAM ( CGDK_SUBWCREV SubWCRev PATHS "$ENV{ProgramFiles}/TortoiseSVN/bin" )
ENDIF ()

#-----------------------------------------------------------------------------
# 5) cgdk_output_name(<var> <name>)
#    windows : <name>_Windows10.0_x64_<config>   (same as vcxproj TargetName)
#    linux   : <name>_linux.<BUILD_TYPE>          (same as previous CMake)
#-----------------------------------------------------------------------------
FUNCTION ( CGDK_TARGET_NAME OUT NAME )
	IF (WIN32)
		SET ( ${OUT} "${NAME}" PARENT_SCOPE )
	ELSE ()
		SET ( ${OUT} "${NAME}_linux.${CMAKE_BUILD_TYPE}" PARENT_SCOPE )
	ENDIF ()
ENDFUNCTION ()

#-----------------------------------------------------------------------------
# 6) svn_version.h
#    windows : SubWCRev.exe (TortoiseSVN)  - same as vcxproj PreBuildEvent
#    linux   : include/cgdk/sdk10/svn_version.sh
#-----------------------------------------------------------------------------
FUNCTION ( CGDK_ADD_SVN_VERSION TARGET )
	SET ( _template "${CGDK_SVN_VERSION_DIR}/svn_version.template" )
	SET ( _output   "${CMAKE_CURRENT_SOURCE_DIR}/svn_version.h" )
	IF (WIN32)
		IF (NOT CGDK_SUBWCREV)
			IF (NOT EXISTS "${_output}")
				MESSAGE ( WARNING "SubWCRev.exe not found: '${_output}' can't be generated" )
			ENDIF ()
			RETURN ()
		ENDIF ()
		ADD_CUSTOM_TARGET ( ${TARGET}_svn_version
			COMMAND "${CGDK_SUBWCREV}" "${CMAKE_CURRENT_SOURCE_DIR}" "${_template}" "${_output}"
			WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
			BYPRODUCTS "${_output}"
			COMMENT "Generating svn_version.h (SubWCRev)"
			VERBATIM )
	ELSE ()
		# generated on every build (same as SubWCRev on windows)
		#  - replaced only when the revision or the modification state is changed
		#    (not recompiled when only the build time is changed. see cgdk_update_svn_version.cmake)
		#  - the generated file goes to the binary dir : the build dirs of one source tree (ex. bld/ and bld_debug/)
		#    built at once wrote and removed the same '<source dir>/svn_version.h.tmp' ("FILE failed to open")
		SET ( _generated "${CMAKE_CURRENT_BINARY_DIR}/svn_version.h.generated" )
		ADD_CUSTOM_TARGET ( ${TARGET}_svn_version
			COMMAND bash "${CGDK_SVN_VERSION_DIR}/svn_version.sh" "${CMAKE_CURRENT_SOURCE_DIR}" "${_template}" "${_generated}"
			COMMAND ${CMAKE_COMMAND} -DSOURCE=${_generated} -DTARGET=${_output} -P "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/cgdk_update_svn_version.cmake"
			BYPRODUCTS "${_output}"
			WORKING_DIRECTORY "${CGDK_SVN_VERSION_DIR}"
			COMMENT "Generating svn_version.h from SVN info"
			VERBATIM )
	ENDIF ()
	SET_TARGET_PROPERTIES ( ${TARGET}_svn_version PROPERTIES FOLDER "svn_version" )
	ADD_DEPENDENCIES ( ${TARGET} ${TARGET}_svn_version )
ENDFUNCTION ()

#-----------------------------------------------------------------------------
# 7) compile options (per target, same as vcxproj / previous CMake flags)
#-----------------------------------------------------------------------------
FUNCTION ( CGDK_SET_COMPILE_OPTIONS TARGET )
	CMAKE_PARSE_ARGUMENTS ( ARG "UNICODE;MFC;LIB" "" "" ${ARGN} )
	TARGET_INCLUDE_DIRECTORIES ( ${TARGET} PRIVATE
		"${CGDK_COMMON_DIR}"
		"${CGDK_INCLUDE_DIR}"
		"${CMAKE_CURRENT_SOURCE_DIR}" )
	IF (MSVC)
		TARGET_COMPILE_DEFINITIONS ( ${TARGET} PRIVATE
			$<${CGDK_IS_DEBUG}:_DEBUG> $<$<NOT:${CGDK_IS_DEBUG}>:NDEBUG>
			$<$<BOOL:${ARG_LIB}>:_LIB>
			$<$<BOOL:${ARG_UNICODE}>:UNICODE> $<$<BOOL:${ARG_UNICODE}>:_UNICODE> )
		# - exception model : Debug /EHsc, Release /EHa (a crash is thrown as throwable::system_exception)
		TARGET_COMPILE_OPTIONS ( ${TARGET} PRIVATE /W4 /sdl /permissive- /MP /wd4819
			$<$<NOT:${CGDK_IS_DEBUG}>:/EHa> $<${CGDK_IS_DEBUG}:/EHsc> )
		IF (ARG_MFC)
			TARGET_COMPILE_DEFINITIONS ( ${TARGET} PRIVATE _WINDOWS _AFXEXT _AFXDLL )
		ENDIF ()
		IF (EXISTS "${CGDK_VCPKG_DIR}/include")
			TARGET_INCLUDE_DIRECTORIES ( ${TARGET} PRIVATE "${CGDK_VCPKG_DIR}/include" )
			TARGET_LINK_DIRECTORIES ( ${TARGET} PRIVATE
				$<$<CONFIG:Debug>:${CGDK_VCPKG_DIR}/debug/lib>
				$<$<NOT:$<CONFIG:Debug>>:${CGDK_VCPKG_DIR}/lib> )
		ENDIF ()
		TARGET_LINK_DIRECTORIES ( ${TARGET} PRIVATE "${CGDK_ROOT}/lib" )
	ELSE ()
		# - exception model : Release -fnon-call-exceptions (a crash signal is thrown as throwable::system_exception)
		TARGET_COMPILE_OPTIONS ( ${TARGET} PRIVATE
			-DC_FLAGS -fexceptions -g3 -Wall -Wextra
			# - Ireferenceable (virtual base of ~70 interfaces) assigns as a no-op by design (the refcount
			#   is object identity, never moved), so -Wvirtual-move-assign is a false positive here (C++ only)
			$<$<COMPILE_LANGUAGE:CXX>:-Wno-virtual-move-assign>
			$<${CGDK_IS_DEBUG}:-D_DEBUG -Wformat=2 -Wformat-security>
			$<$<NOT:${CGDK_IS_DEBUG}>:-DNDEBUG -O3 -Wformat>
			$<$<NOT:${CGDK_IS_DEBUG}>:-fnon-call-exceptions> )
	ENDIF ()
ENDFUNCTION ()

#-----------------------------------------------------------------------------
# 8) CGDK_ADD_LIBRARY(<name> STATIC|SHARED
#        SOURCES <common sources...>
#        [LINUX_SOURCES <...>] [WINDOWS_SOURCES <...>]
#        [PCH <header>]           precompiled header (default: none)
#        [UNICODE]                UNICODE/_UNICODE (vcxproj CharacterSet=Unicode)
#        [MFC]                    MFC extension DLL (windows only)
#        [SVN_VERSION]            generate svn_version.h before compile
#        [WINDOWS_ONLY]           skip on non-windows platforms
#        [WINDOWS_LIBS <...>] [LINUX_LIBS <...>]   link libraries (SHARED only)
#        [DEPENDS <cgdk module names...>] )         build order
#    creates target  : ${CGDK_TARGET} (also returned in parent scope)
#-----------------------------------------------------------------------------
FUNCTION ( CGDK_ADD_LIBRARY NAME TYPE )
	CMAKE_PARSE_ARGUMENTS ( ARG "UNICODE;MFC;SVN_VERSION;WINDOWS_ONLY" "PCH" "SOURCES;LINUX_SOURCES;WINDOWS_SOURCES;WINDOWS_LIBS;LINUX_LIBS;WINDOWS_DEFINITIONS;WINDOWS_OPTIONS;DEPENDS" ${ARGN} )
	IF (ARG_WINDOWS_ONLY AND NOT WIN32)
		RETURN ()
	ENDIF ()

	SET ( _sources ${ARG_SOURCES} )
	IF (WIN32)
		LIST ( APPEND _sources ${ARG_WINDOWS_SOURCES} )
	ELSE ()
		LIST ( APPEND _sources ${ARG_LINUX_SOURCES} )
	ENDIF ()

	CGDK_TARGET_NAME ( _target ${NAME} )
	IF (ARG_MFC)
		# VS generator reads CMAKE_MFC_FLAG from the directory scope (2: shared MFC)
		SET ( CMAKE_MFC_FLAG 2 PARENT_SCOPE )
	ENDIF ()
	ADD_LIBRARY ( ${_target} ${TYPE} ${_sources} )

	# - compile options
	SET ( _opts )
	IF (ARG_UNICODE)
		LIST ( APPEND _opts UNICODE )
	ENDIF ()
	IF (ARG_MFC)
		LIST ( APPEND _opts MFC )
	ENDIF ()
	IF ("${TYPE}" STREQUAL "STATIC")
		LIST ( APPEND _opts LIB )
	ENDIF ()
	CGDK_SET_COMPILE_OPTIONS ( ${_target} ${_opts} )
	IF (MSVC)
		TARGET_COMPILE_DEFINITIONS ( ${_target} PRIVATE ${ARG_WINDOWS_DEFINITIONS} )
		TARGET_COMPILE_OPTIONS ( ${_target} PRIVATE ${ARG_WINDOWS_OPTIONS} )
	ENDIF ()

	# - precompiled header
	IF (ARG_PCH)
		TARGET_PRECOMPILE_HEADERS ( ${_target} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:${ARG_PCH}>" )
	ENDIF ()

	# - output name / directory
	IF (WIN32)
		FOREACH (_cfg Debug Release)
			STRING ( TOUPPER ${_cfg} _CFG )
			SET_TARGET_PROPERTIES ( ${_target} PROPERTIES
				OUTPUT_NAME_${_CFG}              "${NAME}_Windows10.0_x64_${_cfg}"
				PDB_NAME_${_CFG}                 "${NAME}_Windows10.0_x64_${_cfg}"
				ARCHIVE_OUTPUT_DIRECTORY_${_CFG} "${CGDK_LIB_DIR}"
				LIBRARY_OUTPUT_DIRECTORY_${_CFG} "${CGDK_DLL_DIR}"
				RUNTIME_OUTPUT_DIRECTORY_${_CFG} "${CGDK_DLL_DIR}"
				PDB_OUTPUT_DIRECTORY_${_CFG}     "${CGDK_DLL_DIR}"
				COMPILE_PDB_NAME_${_CFG}         "${NAME}_Windows10.0_x64_${_cfg}"
				COMPILE_PDB_OUTPUT_DIRECTORY_${_CFG} "${CGDK_LIB_DIR}" )
		ENDFOREACH ()
		# vcpkg is wired explicitly (CGDK_VCPKG_DIR); keep msbuild's vcpkg integration off
		SET_TARGET_PROPERTIES ( ${_target} PROPERTIES VS_GLOBAL_VcpkgEnabled "false" )
	ELSE ()
		SET_TARGET_PROPERTIES ( ${_target} PROPERTIES
			ARCHIVE_OUTPUT_DIRECTORY "${CGDK_LIB_DIR}"
			LIBRARY_OUTPUT_DIRECTORY "${CGDK_LIB_DIR}" )
	ENDIF ()

	# - link libraries (shared library only)
	IF (NOT "${TYPE}" STREQUAL "STATIC")
		IF (WIN32)
			TARGET_LINK_LIBRARIES ( ${_target} PRIVATE ${ARG_WINDOWS_LIBS} )
		ELSE ()
			TARGET_LINK_LIBRARIES ( ${_target} PRIVATE ${ARG_LINUX_LIBS} )
		ENDIF ()
	ENDIF ()

	# - registry for CGDK_FINALIZE (link order of the DLLs)
	IF ("${TYPE}" STREQUAL "STATIC")
		SET_PROPERTY ( GLOBAL APPEND PROPERTY CGDK_STATIC_TARGETS ${_target} )
	ELSE ()
		SET_PROPERTY ( GLOBAL APPEND PROPERTY CGDK_SHARED_TARGETS ${_target} )
	ENDIF ()

	# - build order
	FOREACH (_dep ${ARG_DEPENDS})
		CGDK_TARGET_NAME ( _dep_target ${_dep} )
		IF (TARGET ${_dep_target})
			ADD_DEPENDENCIES ( ${_target} ${_dep_target} )
		ENDIF ()
	ENDFOREACH ()

	# - svn_version.h
	IF (ARG_SVN_VERSION)
		CGDK_ADD_SVN_VERSION ( ${_target} )
	ENDIF ()

	MESSAGE ( STATUS "cgdk: ${NAME} (${TYPE}, ${CGDK_PLATFORM_OS}) -> ${CGDK_LIB_DIR}" )
	SET ( CGDK_TARGET ${_target} PARENT_SCOPE )
ENDFUNCTION ()

#-----------------------------------------------------------------------------
# 9) CGDK_ADD_EXECUTABLE(<name> SOURCES ... [LINUX_SOURCES ...] [WINDOWS_SOURCES ...]
#        [PCH <header>] [UNICODE] [LIBS ...] [LINUX_LIBS ...] [WINDOWS_LIBS ...]
#        [OUTPUT_DIR <dir>] [DEPENDS ...])
#-----------------------------------------------------------------------------
FUNCTION ( CGDK_ADD_EXECUTABLE NAME )
	CMAKE_PARSE_ARGUMENTS ( ARG "UNICODE" "PCH;OUTPUT_DIR" "SOURCES;LINUX_SOURCES;WINDOWS_SOURCES;LIBS;WINDOWS_LIBS;LINUX_LIBS;DEPENDS" ${ARGN} )
	SET ( _sources ${ARG_SOURCES} )
	IF (WIN32)
		LIST ( APPEND _sources ${ARG_WINDOWS_SOURCES} )
	ELSE ()
		LIST ( APPEND _sources ${ARG_LINUX_SOURCES} )
	ENDIF ()
	ADD_EXECUTABLE ( ${NAME} ${_sources} )
	IF (ARG_UNICODE)
		CGDK_SET_COMPILE_OPTIONS ( ${NAME} UNICODE )
	ELSE ()
		CGDK_SET_COMPILE_OPTIONS ( ${NAME} )
	ENDIF ()
	IF (ARG_PCH)
		TARGET_PRECOMPILE_HEADERS ( ${NAME} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:${ARG_PCH}>" )
	ENDIF ()
	IF (NOT ARG_OUTPUT_DIR)
		SET ( ARG_OUTPUT_DIR "${CGDK_EXECUTE_DIR}" )
	ENDIF ()
	IF (WIN32)
		FOREACH (_cfg Debug Release)
			STRING ( TOUPPER ${_cfg} _CFG )
			SET_TARGET_PROPERTIES ( ${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${_CFG} "${ARG_OUTPUT_DIR}" )
		ENDFOREACH ()
		TARGET_LINK_LIBRARIES ( ${NAME} PRIVATE ${ARG_LIBS} ${ARG_WINDOWS_LIBS} )
	ELSE ()
		SET_TARGET_PROPERTIES ( ${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${ARG_OUTPUT_DIR}" )
		TARGET_LINK_LIBRARIES ( ${NAME} PRIVATE ${ARG_LIBS} ${ARG_LINUX_LIBS} )
	ENDIF ()
	FOREACH (_dep ${ARG_DEPENDS})
		CGDK_TARGET_NAME ( _dep_target ${_dep} )
		IF (TARGET ${_dep_target})
			ADD_DEPENDENCIES ( ${NAME} ${_dep_target} )
		ENDIF ()
	ENDFOREACH ()
	SET ( CGDK_TARGET ${NAME} PARENT_SCOPE )
ENDFUNCTION ()

#-----------------------------------------------------------------------------
# 10) CGDK_ADD_ALIAS(<alias> <module names...>)
#     convenience target (e.g. 'make system') that builds the listed modules
#     which exist on the current platform
#-----------------------------------------------------------------------------
FUNCTION ( CGDK_ADD_ALIAS ALIAS )
	ADD_CUSTOM_TARGET ( ${ALIAS} )
	FOREACH (_module ${ARGN})
		CGDK_TARGET_NAME ( _t ${_module} )
		IF (TARGET ${_t})
			ADD_DEPENDENCIES ( ${ALIAS} ${_t} )
		ELSEIF (TARGET ${_module})
			ADD_DEPENDENCIES ( ${ALIAS} ${_module} )
		ENDIF ()
	ENDFOREACH ()
ENDFUNCTION ()

#-----------------------------------------------------------------------------
# 11) CGDK_FINALIZE()
#     call once after every module was added (build/src/CMakeLists.txt).
#     The DLLs (mfc_controls) link the static libraries and the DLLs added
#     before them through '#pragma comment(lib, ...)' in the cgdk headers,
#     so they must be built after those when everything is in one solution.
#-----------------------------------------------------------------------------
FUNCTION ( CGDK_FINALIZE )
	GET_PROPERTY ( _static GLOBAL PROPERTY CGDK_STATIC_TARGETS )
	GET_PROPERTY ( _shared GLOBAL PROPERTY CGDK_SHARED_TARGETS )
	SET ( _before )
	FOREACH (_dll ${_shared})
		IF (_static OR _before)
			ADD_DEPENDENCIES ( ${_dll} ${_static} ${_before} )
		ENDIF ()
		LIST ( APPEND _before ${_dll} )
	ENDFOREACH ()
ENDFUNCTION ()
