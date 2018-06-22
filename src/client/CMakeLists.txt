add_executable(client 
        main.cpp
        ../texture.cpp
        ../window.cpp
        ../packet.cpp
        ../codecs/h264/decoder.cpp
        ../processors/packet_receiver.cpp
        ../processors/h264decoder.cpp
        ../primitives/image.cpp
        )

target_include_directories(client 
        PRIVATE .. 
        SYSTEM ${CONAN_INCLUDE_DIRS_GLFW}
        )


target_link_libraries(client
        ${CONAN_LIBS_GLFW}
        ${CONAN_LIBS_OPENH264}
        ${OPENGL_gl_LIBRARY}
        ${CONAN_LIBS_BOOST}
        )

