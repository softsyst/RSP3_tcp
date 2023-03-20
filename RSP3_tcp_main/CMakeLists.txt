########################################################################
# Project setup
########################################################################
cmake_minimum_required(VERSION 3.0)
project(RSP3_tcp CXX)

find_package(Threads REQUIRED)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
find_package(LibSDRplay)

if (NOT LIBSDRPLAY_FOUND)
    message(FATAL_ERROR "SDRPlay development files not found!")
endif ()
message(STATUS "LIBSDRPLAY_INCLUDE_DIRS - ${LIBSDRPLAY_INCLUDE_DIRS}")
message(STATUS "LIBSDRPLAY_LIBRARIES - ${LIBSDRPLAY_LIBRARIES}")


# CMP0075 Include file check macros honor CMAKE_REQUIRED_LIBRARIES
if(POLICY CMP0075)
    cmake_policy(SET CMP0075 NEW)
endif()

#set(CMAKE_VERBOSE_MAKEFILE on)

#select the release build type by default to get optimization flags
if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release")
   message(STATUS "Build type not specified: defaulting to release.")
endif(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "")

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/Modules)

if(NOT LIB_INSTALL_DIR)
   set(LIB_INSTALL_DIR lib)
endif()

if (NOT RELEASE_INSTALL_DIR)
	set (RELEASE_INSTALL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Release)
endif()
########################################################################
# Add subdirectories
########################################################################
add_subdirectory(src)

########################################################################
# Compiler specific setup
########################################################################
if(CMAKE_COMPILER_IS_GNUCC)
    add_compile_options(-Wall)
    add_compile_options(-Wextra)
    add_compile_options(-Wsign-compare)
    add_compile_options(-Wdeclaration-after-statement)
    #http://gcc.gnu.org/wiki/Visibility
    add_compile_options(-fvisibility=hidden)
elseif(MSVC14)
    #pthread-w32 issue, timespec is now part of time.h
    #ADD_DEFINITIONS(-D_TIMESPEC_DEFINED)
endif()

ADD_DEFINITIONS(-DDEBUG) 

########################################################################
# Find build dependencies
########################################################################
#if(NOT WIN32)
#list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
#endif


if(WIN32 AND NOT MINGW)
    set(THREADS_USE_PTHREADS_WIN32 true)
endif()

if(NOT THREADS_FOUND)
    message(FATAL_ERROR "pthreads required to compile RSP3_tcp")
endif()

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include_sdrplay
    ${THREADS_PTHREADS_INCLUDE_DIR}
)
