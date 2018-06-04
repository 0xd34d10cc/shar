cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(shar)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE HEADERS "src/*.hpp")
file(GLOB_RECURSE SOURCES "src/*.cpp")

# build 3rdparty
add_subdirectory(3rdparty/screen_capture_lite)
add_subdirectory(3rdparty/libde265)
add_subdirectory(3rdparty/x265/source)

add_executable(shar ${SOURCES} ${HEADERS})
target_include_directories(shar PRIVATE 
  "3rdparty/screen_capture_lite/include" 
  "3rdparty/libde265" 
  "3rdparty/x265/source" 
  "3rdparty/x265/source/common"
)

# set(X265_BUILD 158)
add_definitions(-DHIGH_BIT_DEPTH=0 -DX265_DEPTH=8)

# platfor specific libraries for screen_capture_lite
if(WIN32)
  set(SHAR_PLATFORM_LIBS Dwmapi)
elseif(APPLE)
  find_library(corefoundation_lib CoreFoundation)
  find_library(cocoa_lib Cocoa)
  set(SHAR_PLATFORM_LIBS ${corefoundation_lib} ${cocoa_lib})
else()
  find_package(X11 REQUIRED)
  if(!X11_XTest_FOUND)
    message(FATAL_ERROR "X11 extensions are required, but not found!")
  endif()
  if(!X11_Xfixes_LIB)
    message(FATAL_ERROR "X11 fixes extension is required, but not found!")
  endif()
  find_package(Threads REQUIRED)
  set(SHAR_PLATFORM_LIBS
    ${X11_LIBRARIES}
    ${X11_Xfixes_LIB}
    ${X11_XTest_LIB}
    ${X11_Xinerama_LIB}
    ${CMAKE_THREAD_LIBS_INIT}
  )
endif()
  
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

IF(!WIN32)
  # to get nice debug info
  set(CMAKE_CXX_FLAGS "-g3 -fno-omit-frame-pointer")
  # enable LTO
  set_property(TARGET shar PROPERTY INTERPROCEDURAL_OPTIMIZATION True)
endif()

target_include_directories(shar PRIVATE glfw)
target_link_libraries(shar 
  screen_capture_lite 
  de265 
  x265-static
  ${SHAR_PLATFORM_LIBS} 
  glfw 
  ${OPENGL_gl_LIBRARY}
  dl
  numa
  boost_system
)