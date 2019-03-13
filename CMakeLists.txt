cmake_minimum_required(VERSION 3.9 FATAL_ERROR)
project(shar CXX)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/modules")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

enable_testing()

include(warnings)

set(SHAR_COMPILE_OPTIONS ${ENABLED_WARNINGS})

if(NOT CMAKE_BUILD_TYPE)
    message("-- CMAKE_BUILD_TYPE was not set")
    set(CMAKE_BUILD_TYPE "Debug")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("-- Building in debug mode")
    set(SHAR_COMPILE_DEFINITIONS SHAR_DEBUG_BUILD)
endif()

add_subdirectory(src)