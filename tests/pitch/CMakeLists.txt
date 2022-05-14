cmake_minimum_required(VERSION 3.8)
enable_language(CXX)
enable_language(C)

#set( CMAKE_C_COMPILER  ${COMPILER_PATH}${CROSS_COMPILE}gcc )
#set( CMAKE_CXX_COMPILER  ${COMPILER_PATH}${CROSS_COMPILE}g++ )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
set(CMAKE_BUILD_TYPE "Release")
#set(CMAKE_BUILD_TYPE "Debug")
include(FindPkgConfig)
enable_testing()

#pkg_check_modules (LIBGTEST REQUIRED gtest)
add_definitions( "-g -Wall -pthread" )
#add_definitions( "-DGTEST_HAS_PTHREAD=1" )
add_definitions( "-D_NTEST" )
set(TIMELIBS benchmark pthread)

project(Time)

include_directories(.)
include_directories(../../include)

add_executable(PitchBench
        test_bench.cc)
target_link_libraries(PitchBench ${TIMELIBS})
add_test(test ./PitchBench)

add_executable(pitchTest
		test.cc)
target_link_libraries(pitchTest gtest pthread)
add_test(test ./pitchTest)
