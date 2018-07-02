add_executable(server
               main.cpp
               ../packet.cpp
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
                           )

target_link_libraries(server
                      ${SC_PLATFORM_LIBS}
                      ${CONAN_LIBS_SCREENCAPTURELITE}
                      ${CONAN_LIBS_FFMPEG}
                      ${CONAN_LIBS_LIBX264}
                      ${CONAN_LIBS_BOOST}
                      )

target_compile_options(server PRIVATE ${ENABLED_WARNINGS})

