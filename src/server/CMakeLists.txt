add_executable(server 
        main.cpp
        ../packet.cpp
               ../codecs/openh264/encoder.cpp
               ../processors/screen_capture.cpp
        ../processors/packet_sender.cpp
        ../processors/h264encoder.cpp
        ../primitives/image.cpp
        )

target_include_directories(server 
        PRIVATE .. 
        PRIVATE SYSTEM ${CONAN_INCLUDE_DIRS_BOOST}
        PRIVATE SYSTEM ${CONAN_INCLUDE_DIRS_OPENH264}
        )

target_link_libraries(server
        ${SC_PLATFORM_LIBS}
        screen_capture_lite
        ${CONAN_LIBS_OPENH264}
        ${CONAN_LIBS_BOOST}
        )

target_compile_options(server PRIVATE ${ENABLED_WARNINGS})

