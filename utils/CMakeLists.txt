add_executable(play
               play.cpp
               ../src/texture.cpp
               ../src/window.cpp
               ../src/processors/frame_file_reader.cpp
               ../src/primitives/image.cpp)

target_include_directories(play PRIVATE ../src)
target_link_libraries(play
                      ${CONAN_LIBS_GLFW}
                      ${OPENGL_gl_LIBRARY})
target_compile_options(play PRIVATE ${ENABLED_WARNINGS})

# recorder
add_executable(record
               record.cpp
               ../src/processors/capture_frame_provider.cpp
               ../src/processors/frame_file_writer.cpp
               ../src/primitives/image.cpp)
target_include_directories(record PRIVATE ../src)
target_link_libraries(record
                      screen_capture_lite
                      ${SC_PLATFORM_LIBS})
target_compile_options(record PRIVATE ${ENABLED_WARNINGS})


# encoder latency test
add_executable(latency
               latency.cpp
               ../src/packet.cpp
               ../src/codecs/h264/encoder.cpp
               ../src/codecs/h264/decoder.cpp
               ../src/processors/frame_file_reader.cpp
               ../src/processors/h264encoder.cpp
               ../src/processors/h264decoder.cpp
               ../src/primitives/image.cpp)
target_include_directories(latency PRIVATE ../src)
target_link_libraries(latency
                      ${CONAN_LIBS_OPENH264})
target_compile_options(latency PRIVATE ${ENABLED_WARNINGS})
