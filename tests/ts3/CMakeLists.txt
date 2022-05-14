cmake_minimum_required(VERSION 3.8)
enable_language(CXX)
enable_language(C)

include(FindPkgConfig)
enable_testing()
find_package(Boost COMPONENTS chrono system)

if(CMAKE_CROSSCOMPILING)
	link_directories( /home/kjx/opt-arm64/lib )
endif()

#pkg_check_modules (LIBGTEST REQUIRED gtest)
if(WITH_GBENCH)
	set(BENCH_FOUND	ON)
else()
pkg_check_modules (BENCH benchmark)
endif()
add_definitions( "-g -Wall -pthread -D_NTEST" )
if(Boost_FOUND)
	message(STATUS "Use Boost version ${Boost_VERSION}")
	add_definitions( "-DBOOSTVER=${Boost_VERSION}" )
	include_directories(${Boost_INCLUDE_DIRS})
	set(TIMELIBS pthread ${Boost_LIBRARIES} ${Boost_chrono_LIBRARY})
else()
	set(TIMELIBS pthread)
endif()

project(Time)

include_directories(.)
include_directories(../../include)

if (BENCH_FOUND)
	message(STATUS "Build benchmark for Time")
add_executable(TimeBench
	test_bench.cc)
target_link_libraries(TimeBench benchmark ${TIMELIBS})
add_test(test.ts3 ./TimeBench)
endif()

add_executable(ts3Test
	test.cc)
target_link_libraries(ts3Test gtest pthread)
add_test(test ./ts3Test)
