add_executable(server
               main.cpp
               ../packet.cpp
               ../config.hpp
               ../codecs/convert.cpp
               ../codecs/ffmpeg/encoder.cpp
               ../processors/screen_capture.cpp
               ../processors/packet_sender.cpp
               ../processors/h264encoder.cpp
               ../primitives/image.cpp
               )

target_include_directories(server
                           PRIVATE ..
                           PRIVATE SYSTEM ${CONAN_INCLUDE_DIRS_BOOST}
                           PRIVATE SYSTEM ${CONAN_INCLUDE_DIRS_FFMPEG}
                           PRIVATE SYSTEM ${CONAN_INCLUDE_DIRS_SPDLOG}
                           )

target_link_libraries(server
                      ${SC_PLATFORM_LIBS}
                      ${CONAN_LIBS_SCREENCAPTURELITE}
                      ${CONAN_LIBS_FFMPEG}
                      ${CONAN_LIBS_LIBX264}
                      ${CONAN_LIBS_BOOST}
                      ${CONAN_LIBS_FMT}
                      )

target_compile_options(server PRIVATE ${ENABLED_WARNINGS})

