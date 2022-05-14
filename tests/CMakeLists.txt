cmake_minimum_required(VERSION 3.8)
enable_language(CXX)
enable_language(C)

#set( CMAKE_C_COMPILER  ${COMPILER_PATH}${CROSS_COMPILE}gcc )
#set( CMAKE_CXX_COMPILER  ${COMPILER_PATH}${CROSS_COMPILE}g++ )

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
	message(STATUS "Build on Linux")
else()
	message(STATUS "Build on FreeBSD or like")
	include_directories( /usr/local/include )
	link_directories ( /usr/local/lib )
endif()

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17" )
set(CMAKE_BUILD_TYPE "Release")
#set(CMAKE_BUILD_TYPE "Debug")
add_definitions( "-g -Wall -pthread" )

project(tests)
enable_testing()

add_subdirectory( ts3 )
add_subdirectory( pitch )
add_subdirectory( cray )
add_subdirectory( mold )
add_subdirectory( ring )
add_subdirectory( timer )
