cmake_minimum_required(VERSION 3.8)
enable_language(CXX)
enable_language(C)

include(FindPkgConfig)
enable_testing()
find_package(Boost COMPONENTS chrono system)

if(CMAKE_CROSSCOMPILING)
	link_directories( /home/kjx/opt-arm64/lib )
endif()

add_definitions( "-g -Wall -pthread" )
#add_definitions( "-D_NTEST" )
#add_definitions( "-DWITH_MILLISEC" )
if(Boost_FOUND)
	message(STATUS "Use Boost version ${Boost_VERSION}")
	add_definitions( "-DBOOSTVER=${Boost_VERSION}" )
	include_directories(${Boost_INCLUDE_DIRS})
	set(TIMELIBS pthread ${Boost_LIBRARIES} ${Boost_chrono_LIBRARY})
else()
	set(TIMELIBS pthread)
endif()

project(timer)

include_directories(.)
include_directories(../../include)

add_executable(itimer
	itimer.cc)
target_link_libraries(itimer pthread)

add_executable(ttimer
	ttimer.cc)
target_link_libraries(ttimer pthread rt)

add_executable(atimer
	atimer.cc)
target_link_libraries(atimer fmt pthread rt)
