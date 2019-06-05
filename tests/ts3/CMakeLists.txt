cmake_minimum_required(VERSION 2.8)
enable_language(CXX)
enable_language(C)

#set( CMAKE_C_COMPILER  ${COMPILER_PATH}${CROSS_COMPILE}gcc )
#set( CMAKE_CXX_COMPILER  ${COMPILER_PATH}${CROSS_COMPILE}g++ )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
#set(CMAKE_BUILD_TYPE "Release")
#set(CMAKE_BUILD_TYPE "Debug")
include(FindPkgConfig)
enable_testing()
find_package(Boost COMPONENTS chrono system)

if(CMAKE_CROSSCOMPILING)
	link_directories( /home/kjx/opt-arm64/lib )
endif()

#pkg_check_modules (LIBGTEST REQUIRED gtest)
add_definitions( "-g -Wall -pthread" )
#add_definitions( "-DGTEST_HAS_PTHREAD=1" )
if(Boost_FOUND)
	message(STATUS "Use Boost version ${Boost_VERSION}")
	add_definitions( "-DBOOSTVER=${Boost_VERSION}" )
	include_directories(${Boost_INCLUDE_DIRS})
	set(TIMELIBS benchmark pthread ${Boost_LIBRARIES} ${Boost_chrono_LIBRARY})
else()
	set(TIMELIBS benchmark pthread)
endif()

project(Time)

include_directories(.)
include_directories(../../include)

add_executable(TimeBench
        test_bench.cc)
target_link_libraries(TimeBench ${TIMELIBS})
add_test(test ./TimeBench)

add_executable(ts3Test
		test.cc)
target_link_libraries(ts3Test gtest pthread)
add_test(test ./ts3Test)
