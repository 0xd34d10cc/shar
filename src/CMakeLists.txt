add_executable(shar
               main.cpp
               network/packet.cpp
               network/packet.hpp
               metrics.cpp
               metrics.hpp
               metrics_reporter.cpp
               metrics_reporter.hpp
               signal_handler.cpp
               signal_handler.hpp
               config.hpp
               codecs/convert.cpp
               codecs/convert.hpp
               codecs/ffmpeg/encoder.cpp
               codecs/ffmpeg/encoder.hpp
               processors/screen_capture.cpp
               processors/screen_capture.hpp
               processors/packet_forwarder.cpp
               processors/packet_forwarder.hpp
               processors/h264encoder.cpp
               processors/h264encoder.hpp
               primitives/frame.cpp
               primitives/frame.hpp
               )

target_include_directories(shar
                           PRIVATE .
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
    message("-- Enabling unix-specific options")

    # to get nice debug info
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3 -fno-omit-frame-pointer")

    if (NOT CMAKE_BUILD_TYPE EQUAL "DEBUG")
      # enable LTO
      set_property(TARGET shar PROPERTY INTERPROCEDURAL_OPTIMIZATION True)
    endif ()
endif ()

