add_executable(play
               play.cpp
               ../texture.cpp
               ../texture.hpp
               ../window.cpp
               ../window.hpp
               ../metrics.cpp
               ../metrics.hpp
               ../processors/frame_file_reader.cpp
               ../processors/frame_file_reader.hpp
               ../processors/display.cpp
               ../processors/display.hpp
               ../primitives/frame.cpp
               ../primitives/frame.hpp)

target_include_directories(play 
                           PRIVATE ..
                           PRIVATE SYSTEM ${CONAN_INCLUDE_DIRS_SPDLOG}
                          )
target_link_libraries(play
                      ${CONAN_LIBS_GLFW}
                      ${OPENGL_gl_LIBRARY}
                      ${CONAN_LIBS_FMT}
                      )
target_compile_options(play PRIVATE ${SHAR_COMPILE_OPTIONS})
target_compile_definitions(play PRIVATE ${SHAR_COMPILE_DEFINITIONS})

# recorder
add_executable(record
               record.cpp
               ../metrics.cpp
               ../metrics.hpp
               ../processors/screen_capture.cpp
               ../processors/screen_capture.hpp
               ../processors/frame_file_writer.cpp
               ../processors/frame_file_writer.hpp
               ../primitives/frame.cpp
               ../primitives/frame.hpp
               ../signal_handler.cpp
               ../signal_handler.hpp)
target_include_directories(record
                           PRIVATE ..
                           PRIVATE SYSTEM ${CONAN_INCLUDE_DIRS_SPDLOG}
                          )
target_link_libraries(record
                      ${CONAN_LIBS_SCREENCAPTURELITE}
                      ${SC_PLATFORM_LIBS}
                      ${CONAN_LIBS_FMT}
                      )
target_compile_options(record PRIVATE ${SHAR_COMPILE_OPTIONS})
target_compile_definitions(record PRIVATE ${SHAR_COMPILE_DEFINITIONS})


# full pipeline test
add_executable(pipeline_test
               full_pipeline_test.cpp
               ../packet.cpp
               ../packet.hpp
               ../texture.cpp
               ../texture.hpp
               ../window.cpp
               ../window.hpp
               ../metrics.cpp
               ../metrics.hpp
               ../codecs/convert.cpp
               ../codecs/convert.hpp
               ../codecs/ffmpeg/encoder.cpp
               ../codecs/ffmpeg/encoder.hpp
               ../codecs/ffmpeg/decoder.cpp
               ../codecs/ffmpeg/decoder.hpp
               ../processors/screen_capture.cpp
               ../processors/screen_capture.hpp
               ../processors/h264encoder.cpp
               ../processors/h264encoder.hpp
               ../processors/h264decoder.cpp
               ../processors/h264decoder.hpp
               ../processors/packet_sender.cpp
               ../processors/packet_sender.hpp
               ../processors/packet_receiver.cpp
               ../processors/packet_receiver.hpp
               ../processors/display.cpp
               ../processors/display.hpp
               ../primitives/frame.cpp
               ../primitives/frame.hpp)

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

target_compile_options(pipeline_test PRIVATE ${SHAR_COMPILE_OPTIONS})
target_compile_definitions(pipeline_test PRIVATE ${SHAR_COMPILE_DEFINITIONS})