add_executable(shar
               # application-level code               
               app/main.cpp
               app/metrics_reporter.cpp
               app/metrics_reporter.hpp
               app/signal_handler.cpp
               app/signal_handler.hpp
               # common code
               common/metrics.cpp
               common/metrics.hpp
               common/config.hpp
               # utils
               utils/disable_warnings_push.hpp
               utils/disable_warnings_pop.hpp
               utils/newtype.hpp
               # encoder subsystem
               encoder/convert.cpp
               encoder/convert.hpp
               encoder/ffmpeg.cpp
               encoder/ffmpeg.hpp
               encoder/encoder.cpp
               encoder/encoder.hpp
               # capture subsystem
               capture/capture.cpp
               capture/capture.hpp
               capture/frame.cpp
               capture/frame.hpp
               # network subsystem
               network/network.cpp
               network/network.hpp
               network/packet.cpp
               network/packet.hpp
               )

target_include_directories(shar
                           PRIVATE .
                           PRIVATE ./utils
                           PRIVATE ./common
                           PRIVATE SYSTEM ${CONAN_INCLUDE_DIRS_BOOST}
                           PRIVATE SYSTEM ${CONAN_INCLUDE_DIRS_FFMPEG}
                           PRIVATE SYSTEM ${CONAN_INCLUDE_DIRS_SPDLOG}
                           )

target_link_libraries(shar
                      ${SC_PLATFORM_LIBS}
                      ${CONAN_LIBS_SCREENCAPTURELITE}
                      ${CONAN_LIBS_FFMPEG}
                      ${CONAN_LIBS_LIBX264}
                      ${CONAN_LIBS_BOOST}
                      ${CONAN_LIBS_FMT}
                      )

if (WIN32)
  target_link_libraries(shar ws2_32 iphlpapi)
endif ()

target_compile_definitions(shar
                           PRIVATE MINIUPNP_STATICLIB
                           PRIVATE ${SHAR_COMPILE_DEFINITIONS})
target_compile_options(shar PRIVATE ${SHAR_COMPILE_OPTIONS})

IF (NOT WIN32)
    message("-- Enabling debug info")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3 -fno-omit-frame-pointer")

    if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
      message("-- Enabling LTO")
      set_property(TARGET shar PROPERTY INTERPROCEDURAL_OPTIMIZATION True)
    endif ()
endif ()

