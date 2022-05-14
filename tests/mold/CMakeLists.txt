cmake_minimum_required(VERSION 3.8)
enable_language(CXX)
enable_language(C)

option(WITH_TCMALLOC	"Use tcmalloc from gperftools" OFF)
option(WITH_JEMALLOC	"Use jemalloc from FaceBook" OFF)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
set(CMAKE_BUILD_TYPE "Release")
#set(CMAKE_BUILD_TYPE "Debug")

add_definitions( "-Wpedantic -Wall -pthread" )
add_definitions( "-DELPP_THREAD_SAFE" )
include(FindPkgConfig)

enable_testing()

find_package(Boost 1.53)

if(Boost_FOUND)
	message(STATUS "Use Boost version ${Boost_VERSION}")
	include_directories(${Boost_INCLUDE_DIRS})
endif()

if (WITH_TCMALLOC)
	pkg_check_modules (LIBTCMALLOC libtcmalloc_minimal)
endif()

if (WITH_JEMALLOC)
	pkg_check_modules (JEMALLOC jemalloc)
endif()

project(moldTest)

include_directories(.)
include_directories(../../include)
#pkg_check_modules (LIBGTEST REQUIRED gtest)
#find_package(GTest REQUIRED)

add_executable(mold.test
		test.cc
		testData.h
		testData.cc
        )
target_link_libraries(mold.test gtest pthread)

add_executable(mold.bench
        bench.cc
		testData.h
		testData.cc
        )
if (LIBTCMALLOC_FOUND)
	message (STATUS "Using tcmalloc...")
	target_link_libraries(mold.bench benchmark pthread "${LIBTCMALLOC_LDFLAGS}")
elseif(JEMALLOC_FOUND)
	message (STATUS "Using jemalloc...")
	target_link_libraries(mold.bench benchmark pthread "${JEMALLOC_LDFLAGS}")
else()
	target_link_libraries(mold.bench benchmark pthread)
endif()

add_test(test ./mold.test)
#add_test(bench ./mold.bench)
SET_TESTS_PROPERTIES(test PROPERTIES ENVIRONMENT "CTEST_OUTPUT_ON_FAILURE = 1")
