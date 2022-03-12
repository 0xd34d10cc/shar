cmake_minimum_required(VERSION 3.9 FATAL_ERROR)
project(shar CXX)

add_definitions(-D_ITERATOR_DEBUG_LEVEL=0)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/modules")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

enable_testing()

include(warnings)
include(platform)

set(SHAR_COMPILE_OPTIONS ${ENABLED_WARNINGS} ${PLATFORM_OPTIONS})

if(NOT CMAKE_BUILD_TYPE)
    message("-- CMAKE_BUILD_TYPE was not set")
    set(CMAKE_BUILD_TYPE "Debug")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("-- Building in debug mode")
    set(SHAR_COMPILE_DEFINITIONS SHAR_DEBUG_BUILD)
endif()


add_subdirectory(3rdparty/screen_capture_lite)
set(CONAN_INCLUDE_DIRS_SCREENCAPTURE ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/screen_capture_lite/include)
set(CONAN_LIBS_SCREENCAPTURELITE screen_capture_lite)

add_subdirectory(src)