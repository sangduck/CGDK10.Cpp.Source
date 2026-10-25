#*****************************************************************************
#*                                                                           *
#*                      Cho sanghyun's Game Classes II                       *
#*                      Ver 10.0 / release 2019.12.11                        *
#*                                                                           *
#*                    CMake - benchmark query redis (linux)                  *
#*                                                                           *
#*  -DCGDK_ROOT=<dir with include/ and lib/>  (default: ../..)               *
#*  -DBENCH_3RD=<install prefix of hiredis and redis-plus-plus>              *
#*  -DBOOST_INCLUDE=<dir with boost/ (1.84 or newer : Boost.Redis)>          *
#*                                                                           *
#*                (c) 2003. Cho Sanghyun. All right reserved.                *
#*                          http://www.CGCII.co.kr                           *
#*                                                                           *
#*****************************************************************************
CMAKE_MINIMUM_REQUIRED ( VERSION 3.20 )
PROJECT ( "CGDK10.benchmark.query_redis" CXX C )

SET ( CMAKE_CXX_STANDARD 20 )
SET ( CMAKE_CXX_STANDARD_REQUIRED ON )
IF ( NOT CMAKE_BUILD_TYPE )
	SET ( CMAKE_BUILD_TYPE Release )
ENDIF ()
SET ( CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG" )

SET ( CGDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH "CGDK root" )
SET ( BENCH_3RD "" CACHE PATH "install prefix of hiredis / redis-plus-plus" )
SET ( BOOST_INCLUDE "" CACHE PATH "boost include dir" )

FIND_PACKAGE ( Threads REQUIRED )

# 1) hiredis
ADD_EXECUTABLE ( bench_hiredis bench_hiredis.cpp )
TARGET_INCLUDE_DIRECTORIES ( bench_hiredis PRIVATE ${BENCH_3RD}/include )
TARGET_LINK_LIBRARIES ( bench_hiredis PRIVATE ${BENCH_3RD}/lib64/libhiredis.a Threads::Threads )

# 2) redis-plus-plus
ADD_EXECUTABLE ( bench_redispp bench_redispp.cpp )
TARGET_INCLUDE_DIRECTORIES ( bench_redispp PRIVATE ${BENCH_3RD}/include )
TARGET_LINK_LIBRARIES ( bench_redispp PRIVATE ${BENCH_3RD}/lib64/libredis++.a ${BENCH_3RD}/lib64/libhiredis.a Threads::Threads )

# 3) Boost.Redis (header only + <boost/redis/src.hpp>)
ADD_EXECUTABLE ( bench_boost_redis bench_boost_redis.cpp )
TARGET_INCLUDE_DIRECTORIES ( bench_boost_redis PRIVATE ${BOOST_INCLUDE} )
TARGET_COMPILE_DEFINITIONS ( bench_boost_redis PRIVATE BOOST_ASIO_NO_DEPRECATED )
TARGET_LINK_LIBRARIES ( bench_boost_redis PRIVATE ssl crypto Threads::Threads )

# 4) CGDK10 query::redis
INCLUDE ( ${CGDK_ROOT}/build/cmake/cgdk_platform.cmake OPTIONAL RESULT_VARIABLE CGDK_PLATFORM_FOUND )
ADD_EXECUTABLE ( bench_cgdk bench_cgdk.cpp )
TARGET_INCLUDE_DIRECTORIES ( bench_cgdk PRIVATE ${CGDK_ROOT}/include )
TARGET_LINK_DIRECTORIES ( bench_cgdk PRIVATE ${CGDK_ROOT}/lib/cgdk/sdk10/${PLATFORM_OS} )
TARGET_LINK_LIBRARIES ( bench_cgdk PRIVATE
	CGDK10.server.query.redis_linux.Release
	CGDK10.server.query_linux.Release
	CGDK10.net.socket_linux.Release
	CGDK10.system.object_linux.Release
	rt
	Threads::Threads
)

# Opt-in real Redis checks; run explicitly against an isolated test Redis.
ADD_EXECUTABLE ( check_batch check_batch.cpp )
TARGET_INCLUDE_DIRECTORIES ( check_batch PRIVATE ${CGDK_ROOT}/include )
TARGET_LINK_DIRECTORIES ( check_batch PRIVATE ${CGDK_ROOT}/lib/cgdk/sdk10/${PLATFORM_OS} )
TARGET_LINK_LIBRARIES ( check_batch PRIVATE
	CGDK10.server.query.redis_linux.Release
	CGDK10.server.query_linux.Release
	CGDK10.net.socket_linux.Release
	CGDK10.system.object_linux.Release
	rt
	Threads::Threads
)

ADD_EXECUTABLE ( bench_resp_parser bench_resp_parser.cpp )
TARGET_INCLUDE_DIRECTORIES ( bench_resp_parser PRIVATE ${CGDK_ROOT}/include )
TARGET_LINK_DIRECTORIES ( bench_resp_parser PRIVATE ${CGDK_ROOT}/lib/cgdk/sdk10/${PLATFORM_OS} )
TARGET_LINK_LIBRARIES ( bench_resp_parser PRIVATE
    CGDK10.server.query.redis_linux.Release
    CGDK10.server.query_linux.Release
    CGDK10.net.socket_linux.Release
    CGDK10.system.object_linux.Release
    rt
    Threads::Threads
)