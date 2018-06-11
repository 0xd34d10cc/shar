cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(shar CXX)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE HEADERS "src/*.hpp")
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(shar ${SOURCES} ${HEADERS})

# platfor specific libraries for screen_capture_lite
if(WIN32)
  set(SC_PLATFORM_LIBS Dwmapi)
elseif(APPLE)
  find_library(corefoundation_lib CoreFoundation)
  find_library(cocoa_lib Cocoa)
  set(SC_PLATFORM_LIBS ${corefoundation_lib} ${cocoa_lib})
else()
  find_package(X11 REQUIRED)
  if(!X11_XTest_FOUND)
    message(FATAL_ERROR "X11 extensions are required, but not found!")
  endif()
  if(!X11_Xfixes_LIB)
    message(FATAL_ERROR "X11 fixes extension is required, but not found!")
  endif()
  find_package(Threads REQUIRED)
  set(SC_PLATFORM_LIBS
    ${X11_LIBRARIES}
    ${X11_Xfixes_LIB}
    ${X11_XTest_LIB}
    ${X11_Xinerama_LIB}
    ${CMAKE_THREAD_LIBS_INIT}
  )
endif()
  
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Boost 1.67 REQUIRED COMPONENTS system)
if(!Boost_FOUND)
  message(FATAL_ERROR "Boost not found!")
endif()

IF(!WIN32)
  # to get nice debug info
  set(CMAKE_CXX_FLAGS "-g3 -fno-omit-frame-pointer")
  # enable LTO
  set_property(TARGET shar PROPERTY INTERPROCEDURAL_OPTIMIZATION True)

  target_link_libraries(shar dl)
endif()

target_include_directories(shar PRIVATE glfw)
target_link_libraries(shar
  screen_capture_lite 
  ${SC_PLATFORM_LIBS} 
  glfw 
  ${OPENGL_gl_LIBRARY}
  ${Boost_LIBRARIES}
)