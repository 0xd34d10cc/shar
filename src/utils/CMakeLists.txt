add_executable(play
               play.cpp
               ../texture.cpp
               ../window.cpp
               ../processors/frame_file_reader.cpp
               ../primitives/image.cpp)

target_include_directories(play 
                           PRIVATE ..
                           PRIVATE SYSTEM ${CONAN_INCLUDE_DIRS_SPDLOG}
                          )
target_link_libraries(play
                      ${CONAN_LIBS_GLFW}
                      ${OPENGL_gl_LIBRARY}
                      ${CONAN_LIBS_FMT}
                      )
target_compile_options(play PRIVATE ${ENABLED_WARNINGS})

# recorder
add_executable(record
               record.cpp
               ../processors/screen_capture.cpp
               ../processors/frame_file_writer.cpp
               ../primitives/image.cpp
               ../signal_handler.cpp)
target_include_directories(record
                           PRIVATE ..
                           PRIVATE SYSTEM ${CONAN_INCLUDE_DIRS_SPDLOG}
                          )
target_link_libraries(record
                      ${CONAN_LIBS_SCREENCAPTURELITE}
                      ${SC_PLATFORM_LIBS}
                      ${CONAN_LIBS_FMT}
                      )
target_compile_options(record PRIVATE ${ENABLED_WARNINGS})


# full pipeline test
add_executable(pipeline_test
               full_pipeline_test.cpp
               ../packet.cpp
               ../texture.cpp
               ../window.cpp
               ../codecs/convert.cpp
               ../codecs/ffmpeg/encoder.cpp
               ../codecs/ffmpeg/decoder.cpp
               ../processors/screen_capture.cpp
               ../processors/h264encoder.cpp
               ../processors/h264decoder.cpp
               ../processors/packet_sender.cpp
               ../processors/packet_receiver.cpp
               ../primitives/image.cpp)

target_include_directories(pipeline_test
                           PRIVATE ..
                           PRIVATE SYSTEM ${CONAN_INCLUDE_DIRS_GLFW}
                           PRIVATE SYSTEM ${CONAN_INCLUDE_DIRS_BOOST}
                           PRIVATE SYSTEM ${CONAN_INCLUDE_DIRS_OPENH264}
                           PRIVATE SYSTEM ${CONAN_INCLUDE_DIRS_SPDLOG}
                           )

target_link_libraries(pipeline_test
                      ${CONAN_LIBS_SCREENCAPTURELITE}
                      ${SC_PLATFORM_LIBS}
                      ${CONAN_LIBS_GLFW}
                      ${CONAN_LIBS_OPENH264}
                      ${OPENGL_gl_LIBRARY}
                      ${CONAN_LIBS_BOOST}
                      ${CONAN_LIBS_LIBX264}
                      ${CONAN_LIBS_FMT}
                      ${CONAN_LIBS_FFMPEG}
                      )

target_compile_options(pipeline_test PRIVATE ${ENABLED_WARNINGS})