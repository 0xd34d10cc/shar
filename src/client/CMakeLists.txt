add_executable(client
               main.cpp
               options.cpp
               ../texture.cpp
               ../window.cpp
               ../packet.cpp
               ../codecs/convert.cpp
               ../codecs/ffmpeg/decoder.cpp
               ../processors/packet_receiver.cpp
               ../processors/h264decoder.cpp
               ../primitives/image.cpp
               )

target_include_directories(client
                           PRIVATE ..
                           PRIVATE SYSTEM ${CONAN_INCLUDE_DIRS_GLFW}
                           PRIVATE SYSTEM ${CONAN_INCLUDE_DIRS_BOOST}
                           PRIVATE SYSTEM ${CONAN_INCLUDE_DIRS_SPDLOG}
                           )

target_link_libraries(client
                      ${OPENGL_gl_LIBRARY}
                      ${CONAN_LIBS_GLFW}
                      ${CONAN_LIBS_FFMPEG}
                      ${CONAN_LIBS_LIBX264}
                      ${CONAN_LIBS_BOOST}
                      ${CONAN_LIBS_FMT}
                      )

target_compile_options(client PRIVATE ${ENABLED_WARNINGS})
