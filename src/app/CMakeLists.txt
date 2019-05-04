add_executable(shar
               main.cpp
               broadcast.hpp
               broadcast.cpp
               view.hpp
               view.cpp
               signal_handler.cpp
               signal_handler.hpp
               )

target_include_directories(shar
                           PRIVATE ..
                           PRIVATE ../common
                           )

target_link_libraries(shar
                      capture
                      encoder
                      network
                      ui
                      common
                      )

target_compile_definitions(shar PRIVATE ${SHAR_COMPILE_DEFINITIONS})
target_compile_options(shar PRIVATE ${SHAR_COMPILE_OPTIONS})

IF (NOT WIN32)
    message("-- Enabling debug info")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3 -fno-omit-frame-pointer")

    if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
      message("-- Enabling LTO")
      set_property(TARGET shar PROPERTY INTERPROCEDURAL_OPTIMIZATION True)
    endif ()
endif ()