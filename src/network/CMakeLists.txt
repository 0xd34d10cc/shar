add_library(network
    network.cpp
    network.hpp
    packetizer.hpp
    packetizer.cpp
    fragment.hpp
    fragment.cpp
    url.hpp
    url.cpp
    module.hpp
    tcp.hpp
    tcp.cpp
    rtp.hpp
    rtp.cpp
)

target_include_directories(network
    PUBLIC ..
    PUBLIC ../common
    PUBLIC SYSTEM ${CONAN_INCLUDE_DIRS_ASIO}
    PUBLIC SYSTEM ${CONAN_INCLUDE_DIRS_JSONFORMODERNCPP}
    PUBLIC SYSTEM ${CONAN_INCLUDE_DIRS_SPDLOG}
    PUBLIC SYSTEM "${CONAN_INCLUDE_DIRS_PROMETHEUS-CPP}/push/include"
    PUBLIC SYSTEM "${CONAN_INCLUDE_DIRS_PROMETHEUS-CPP}/pull/include"
    PUBLIC SYSTEM "${CONAN_INCLUDE_DIRS_PROMETHEUS-CPP}/core/include"
)

target_link_libraries(network
  ${CONAN_LIBS_FMT}
	rtnet
)

target_compile_definitions(network PRIVATE ${SHAR_COMPILE_DEFINITIONS})
target_compile_options(network PRIVATE ${SHAR_COMPILE_OPTIONS})


add_subdirectory(rtnet)
add_subdirectory(tests)