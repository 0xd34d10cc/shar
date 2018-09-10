add_executable(server
               main.cpp
               ../packet.cpp
               ../packet.hpp
               ../forwarding.cpp
               ../forwarding.hpp
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
               ../processors/packet_sender.cpp
               ../processors/packet_sender.hpp
               ../processors/h264encoder.cpp
               ../processors/h264encoder.hpp
               ../primitives/image.cpp
               ../primitives/image.hpp
               )

target_include_directories(server
                           PRIVATE ..
                           PRIVATE SYSTEM ${CONAN_INCLUDE_DIRS_BOOST}
                           PRIVATE SYSTEM ${CONAN_INCLUDE_DIRS_FFMPEG}
                           PRIVATE SYSTEM ${CONAN_INCLUDE_DIRS_SPDLOG}
                           PRIVATE SYSTEM %{CONAN_INCLUDE_DIRS_MINIUPNPC}
                           )

target_link_libraries(server
                      ${SC_PLATFORM_LIBS}
                      ${CONAN_LIBS_SCREENCAPTURELITE}
                      ${CONAN_LIBS_FFMPEG}
                      ${CONAN_LIBS_LIBX264}
                      ${CONAN_LIBS_BOOST}
                      ${CONAN_LIBS_FMT}
                      ${CONAN_LIBS_MINIUPNPC}
                      )

if (WIN32)
  target_link_libraries(server ws2_32 iphlpapi)
endif ()

target_compile_definitions(server PRIVATE MINIUPNP_STATICLIB)
target_compile_options(server PRIVATE ${ENABLED_WARNINGS})

