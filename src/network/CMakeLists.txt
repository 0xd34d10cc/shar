add_library(network
    network.cpp
    network.hpp
    packet.cpp
    packet.hpp
    packetizer.hpp
    packetizer.cpp
    fragment.hpp
    fragment.cpp
)

target_include_directories(network
    PUBLIC ..
    PUBLIC ../utils
    PUBLIC ../common
    PUBLIC SYSTEM ${CONAN_INCLUDE_DIRS_BOOST}
    PUBLIC SYSTEM ${CONAN_INCLUDE_DIRS_SPDLOG}
)

target_link_libraries(network
    ${CONAN_LIBS_BOOST}
    ${CONAN_LIBS_FMT}
)

target_compile_definitions(network
                           PRIVATE ${SHAR_COMPILE_DEFINITIONS})
target_compile_options(network PRIVATE ${SHAR_COMPILE_OPTIONS})


add_subdirectory(rtnet)
add_subdirectory(tests)