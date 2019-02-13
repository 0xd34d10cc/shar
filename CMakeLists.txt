cmake_minimum_required(VERSION 3.9 FATAL_ERROR)
project(shar CXX)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)


# platfor specific libraries for screen_capture_lite
if (WIN32)
    set(SC_PLATFORM_LIBS Dwmapi)
elseif (APPLE)
    find_package(Threads REQUIRED)
    find_library(corefoundation_lib CoreFoundation REQUIRED)
    find_library(cocoa_lib Cocoa REQUIRED)
    find_library(coremedia_lib CoreMedia REQUIRED)
    find_library(avfoundation_lib AVFoundation REQUIRED)
    find_library(coregraphics_lib CoreGraphics REQUIRED)
    find_library(corevideo_lib CoreVideo REQUIRED)
    
    set(SC_PLATFORM_LIBS 
            ${corefoundation_lib}
            ${cocoa_lib}
            ${coremedia_lib}
            ${avfoundation_lib}
            ${coregraphics_lib}
            ${corevideo_lib})
else ()
    find_package(X11 REQUIRED)
    if (!X11_XTest_FOUND)
        message(FATAL_ERROR "X11 extensions are required, but not found!")
    endif ()
    if (!X11_Xfixes_LIB)
        message(FATAL_ERROR "X11 fixes extension is required, but not found!")
    endif ()
    find_package(Threads REQUIRED)
    set(SC_PLATFORM_LIBS
            ${X11_LIBRARIES}
            ${X11_Xfixes_LIB}
            ${X11_XTest_LIB}
            ${X11_Xinerama_LIB}
            ${CMAKE_THREAD_LIBS_INIT}
            )
endif ()

set(OpenGL_GL_PREFERENCE "GLVND")
find_package(OpenGL REQUIRED)

# Setup warnings
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    # using Clang
    set(ENABLED_WARNINGS
        -Werror
        -Weverything
        -Wno-c++98-compat
        -Wno-c++98-compat-pedantic
        -Wno-padded
        -Wno-newline-eof
        -Wno-missing-braces
        -Wno-global-constructors
        -Wno-exit-time-destructors
        -Wno-unused-command-line-argument # because ccache
        -Wno-undefined-func-template # false positives
        -Wno-weak-vtables
        ) 
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # using GCC
    set(ENABLED_WARNINGS -Wall -Wextra -pedantic)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    # using Intel C++
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    # using Visual Studio C++
endif ()

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