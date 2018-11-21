add_executable(sender
               main.cpp
               ../network/packet.cpp
               ../network/packet.hpp
               ../network/forwarding.cpp
               ../network/forwarding.hpp
               ../metrics.cpp
               ../metrics.hpp
               ../metrics_reporter.cpp
               ../metrics_reporter.hpp
               ../signal_handler.cpp
               ../signal_handler.hpp
               ../config.hpp
               ../codecs/convert.cpp
               ../codecs/convert.hpp
               ../codecs/ffmpeg/encoder.cpp
               ../codecs/ffmpeg/encoder.hpp
               ../processors/screen_capture.cpp
               ../processors/screen_capture.hpp
               ../processors/packet_forwarder.cpp
               ../processors/packet_forwarder.hpp
               ../processors/h264encoder.cpp
               ../processors/h264encoder.hpp
               ../primitives/frame.cpp
               ../primitives/frame.hpp
               )

target_include_directories(sender
                           PRIVATE ..
                           PRIVATE SYSTEM ${CONAN_INCLUDE_DIRS_BOOST}
                           PRIVATE SYSTEM ${CONAN_INCLUDE_DIRS_FFMPEG}
                           PRIVATE SYSTEM ${CONAN_INCLUDE_DIRS_SPDLOG}
                           PRIVATE SYSTEM %{CONAN_INCLUDE_DIRS_MINIUPNPC}
                           )

target_link_libraries(sender
                      ${SC_PLATFORM_LIBS}
                      ${CONAN_LIBS_SCREENCAPTURELITE}
                      ${CONAN_LIBS_FFMPEG}
                      ${CONAN_LIBS_LIBX264}
                      ${CONAN_LIBS_BOOST}
                      ${CONAN_LIBS_FMT}
                      ${CONAN_LIBS_MINIUPNPC}
                      )

if (WIN32)
  target_link_libraries(sender ws2_32 iphlpapi)
endif ()

target_compile_definitions(sender
                           PRIVATE MINIUPNP_STATICLIB
                           PRIVATE ${SHAR_COMPILE_DEFINITIONS})
target_compile_options(sender PRIVATE ${SHAR_COMPILE_OPTIONS})

