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
# 1) build version
CMAKE_MINIMUM_REQUIRED ( VERSION 3.20 )
CMAKE_POLICY ( VERSION 3.20 )

# 2) project name
PROJECT ( "CGDK10.system.object" )

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

#template instantiation

#definitions
definitions.cgdk.cpp
definitions.string_function.cpp
string_functions.cpp
definitions.factory.cpp
definitions.execute.cpp
definitions.schedulable.cpp
definitions.object.cpp
definitions.admin.cpp

#api
api.queue_executable.cpp
api.io_context.Nepoll.cpp
api.io_context.epoll_queued.cpp
api.io_context.epoll_direct.cpp
api.io_context.uring.cpp
api.io_context.uring_cq.cpp
api.file.cpp
api.file_ex.cpp
api.file.linux.cpp
api.manager_thread.cpp
api.system.linux.cpp
api.thread.cpp
api.threads.cpp
functions.cpp
chrono.cpp
chrono_time.cpp
context.cpp
context.json.cpp
Ireferenceable.cpp

#archive
archive.file.cpp
archive.file_write.cpp

#object
object.Istate.cpp
object.Ninitializable.cpp
object.Npausable.cpp
object.Nstartable.cpp

#class
#class_manager.cpp

#executor
Iexecutor.cpp
executor.list_priority.cpp
executor.manager.cpp
executor.io_context.cpp
executor.io_context_threads.cpp
executor.queue_list.cpp
executor.queue_list_threads.cpp
executor.strand.cpp
executor.scheduler.cpp
executor_system.cpp

#coroutine
coroutine.cpp

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
factory.manager.print.cpp
factory.manager.print_sub.cpp
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
log.record.process.cpp

#compress
util.compress_zip.cpp

#edc
util.edc.check_sum.cpp
util.edc.crc.cpp
util.edc.md5.cpp
util.base64.cpp
)

# 2) linux only
SET (SRC_FILES_LINUX
buffer.explicit_instantiation.cpp
#ZLIB
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

# 3) windows only
SET (SRC_FILES_WINDOWS
allocator_manager.cpp
api.file.android.cpp
api.file.windows_native.cpp
api.io_context.iocp_direct.cpp
api.io_context.iocp_queued.cpp
api.io_context.Niocp.cpp
api.io_context.rio.cpp
api.system.android.cpp
api.system.windows.cpp
log_source.cpp
signal_handler.cpp
)


#-----------------------------------------------------------------------------
# compile
#-----------------------------------------------------------------------------
# 1) library
CGDK_ADD_LIBRARY ( CGDK10.system.object STATIC
	SOURCES         ${SRC_FILES}
	LINUX_SOURCES   ${SRC_FILES_LINUX}
	WINDOWS_SOURCES ${SRC_FILES_WINDOWS}
	PCH             pch.h
	SVN_VERSION
)

# - zlib is vendored third-party : silence its intentional -Wimplicit-fallthrough (do not touch upstream source)
IF (NOT MSVC)
	SET_SOURCE_FILES_PROPERTIES (
		zlib/adler32.c zlib/compress.c zlib/crc32.c zlib/deflate.c
		zlib/gzclose.c zlib/gzlib.c zlib/gzread.c zlib/gzwrite.c
		zlib/infback.c zlib/inffast.c zlib/inflate.c zlib/inftrees.c
		zlib/trees.c zlib/uncompr.c zlib/zutil.c
		PROPERTIES COMPILE_OPTIONS "-Wno-implicit-fallthrough"
	)
ENDIF ()

# 2) build alias for convenience
IF (TARGET ${CGDK_TARGET})
	ADD_CUSTOM_TARGET ( object DEPENDS ${CGDK_TARGET} )
ENDIF ()