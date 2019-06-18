cmake_minimum_required(VERSION 2.8)
enable_language(CXX)
enable_language(C)

#set( CMAKE_C_COMPILER  ${COMPILER_PATH}${CROSS_COMPILE}gcc )
#set( CMAKE_CXX_COMPILER  ${COMPILER_PATH}${CROSS_COMPILE}g++ )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_BUILD_TYPE "Release")
#set(CMAKE_BUILD_TYPE "Debug")
add_definitions( "-g -Wall -pthread" )

enable_testing()

add_subdirectory( ts3 )
add_subdirectory( pitch )
add_subdirectory( cray )
add_subdirectory( mold )
add_subdirectory( ring )
