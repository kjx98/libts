cmake_minimum_required(VERSION 3.8)
enable_language(CXX)
enable_language(C)

#set( CMAKE_C_COMPILER  ${COMPILER_PATH}${CROSS_COMPILE}gcc )
#set( CMAKE_CXX_COMPILER  ${COMPILER_PATH}${CROSS_COMPILE}g++ )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
#set(CMAKE_BUILD_TYPE "Release")
#set(CMAKE_BUILD_TYPE "Debug")
include(FindPkgConfig)
find_package(Boost COMPONENTS chrono system)

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
project(ringTest)

include_directories(.)
include_directories(../../include)

add_executable(ringTest
        test.cc)
target_link_libraries(ringTest ${TIMELIBS})
