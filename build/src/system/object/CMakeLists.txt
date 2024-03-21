#*****************************************************************************
#*                                                                           *
#*                      Cho sanghyun's Game Classes II                       *
#*                      Ver 10.0 / release 2019.12.11                        *
#*                                                                           *
#*                        CMake - CGDK10.system.object                       *
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
# 1) project name
PROJECT ( "CGDK10.system.object" )

# 2) project version
SET ( PROJECT_VERSION_MAJOR 10 )
SET ( PROJECT_VERSION_MINOR 0 )
SET ( PROJECT_VERSION_PATCH 100 )
SET ( PROJECT_VERSION_TWEAK 1 )

# 3) build version
CMAKE_MINIMUM_REQUIRED ( VERSION 3.20 )
CMAKE_POLICY( VERSION 3.20 )

# 4) default BUILD_TYPE
IF (NOT DEFINED CMAKE_BULD_TYPE)
	SET (CMAKE_BULD_TYPE "Debug")
ENDIF()

# 5) directories
SET ( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../../../lib/cgdk/sdk10/${DIST} )
SET ( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../../../lib/cgdk/sdk10/${DIST} )
SET ( CMAKE_RUNTIME_OTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../../execute )
SET ( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../execute )

# 6) compile options
set ( CMAKE_CXX_STANDARD 20)
set ( CMAKE_CXX_STANDARD_REQUIRED ON)
SET ( CMAKE_CXX_FLAGS "-DC_FLAGS -fexceptions -std=c++20" )
SET ( CMAKE_CXX_FLAGS_DEBUG "-D_DEBUG -g1 -Wall -Wextra" )
SET ( CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3 -Wall -Wextra" )

# 7) include directories
INCLUDE_DIRECTORIES(
	${CMAKE_CURRENT_SOURCE_DIR}/../../../../include
	./
)


#-----------------------------------------------------------------------------
# source files
#-----------------------------------------------------------------------------
SET (SRC_FILES
#...
pch.cpp

#common
${CMAKE_CURRENT_SOURCE_DIR}/../../common/cgdk_info.cpp

#definitions
definitions.cgdk.cpp
definitions.string_function.cpp
definitions.factory.cpp
definitions.execute.cpp
definitions.schedulable.cpp
definitions.object.cpp

#api
api.queue_executable.cpp
api.io_context.epoll.cpp
api.file.cpp
api.file_ex.cpp
api.file.linux.cpp
api.manager_thread.cpp
api.system.linux.cpp
api.thread.cpp
api.threads.cpp
functions.cpp
chrono_time.cpp
context.cpp

#archive
archive.file.cpp
archive.file_write.cpp

#object
object.Istate.cpp
object.Ninitializable.cpp
object.Npausable.cpp
object.Nstartable.cpp

#executor
Iexecutor.cpp
executor.list_priority.cpp
executor.manager.cpp
executor.io_context.cpp
executor.io_context_threads.cpp
executor.queue_list.cpp
executor.strand.cpp
executor.scheduler.cpp
executor_system.cpp

#message transmitter
Nmessage_transmitter.cpp
Nmessage_transmitter_single.cpp
Nmessage_transmitter_static.cpp

#executable
Iexecutable.cpp
executable.batch.cpp
executable.batch_wait.cpp

#schedulable
Ischedulable.cpp
schedulable.manager.cpp
schedulable.Iexecutable.cpp
schedulable.executable.cpp
schedulable.Nobject_startable.cpp
schedulable.system.exception_observer.cpp
schedulable.system.factory_sustain.cpp
schedulable.entity.cpp

#updatable
updatable.executable_batch.cpp

#factory
Ifactory.cpp
factory.manager.cpp
factory.memory_block.cpp

#dump
dump_manager.cpp
throwable.cpp

#resource
Iresource.cpp
resource.manager.cpp

#log
Ilogger.cpp
Ilog_source.cpp
log.stream.cpp
log.archive.cpp
log.functions.cpp
log.record.cpp

#compress
util.compress_zip.cpp

#edc
util.edc.check_sum.cpp
util.edc.crc.cpp
util.edc.md5.cpp
util.crypt.base64.cpp

)

#zLib
SET (SRC_FILES_ZLIB
zlib/adler32.c
zlib/compress.c
zlib/crc32.c
zlib/deflate.c
zlib/gzclose.c
zlib/gzread.c
zlib/gzwrite.c
zlib/infback.c
zlib/inffast.c
zlib/inflate.c
zlib/inftrees.c
zlib/trees.c
zlib/uncompr.c
zlib/zutil.c
)

#fmt
SET (SRC_FILES_FMT
${CMAKE_CURRENT_SOURCE_DIR}/../../../../include/cgdk/sdk10/system/object/fmt/src/format.cc
${CMAKE_CURRENT_SOURCE_DIR}/../../../../include/cgdk/sdk10/system/object/fmt/src/os.cc
)


#-----------------------------------------------------------------------------
# compile
#-----------------------------------------------------------------------------
# 1) set variable
IF (${CMAKE_BUILD_TYPE} MATCHES "Debug")
	SET (BUILD_MESSAGE "Debug")
	SET (BUILD_TYPE "Debug")
ELSEIF (${CMAKE_BUILD_TYPE} MATCHES "Debug_Dynamic")
	SET (BUILD_MESSAGE "Debug_Dynamic")
	SET (BUILD_TYPE "Debug_Dynamic")
ELSEIF (${CMAKE_BUILD_TYPE} MATCHES "Release")
	SET (BUILD_MESSAGE "Release")
	SET (BUILD_TYPE "Release")
ELSEIF (${CMAKE_BUILD_TYPE} MATCHES "Release_Dynamic")
	SET (BUILD_MESSAGE "Release_Dynamic")
	SET (BUILD_TYPE "Release_Dynamic")
ELSE ()
	MESSAGE ("CMAKE_BUILD_TYPE is not defined")
ENDIF ()

# 2) messages
MESSAGE (STATUS "compile ${PROJECT_NAME}")
MESSAGE (STATUS "  - configure['${BUILD_MESSAGE}']")

# 3) configure file
#CONFIGURE_FILE ( version.h.in version.h )

# 4) add library
ADD_LIBRARY(${PROJECT_NAME}_linux.${BUILD_TYPE} STATIC ${SRC_FILES} ${SRC_FILES_ZLIB} ${SRC_FILES_FMT})
TARGET_PRECOMPILE_HEADERS(${PROJECT_NAME}_linux.${BUILD_TYPE} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:pch.h>")
