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
)

target_link_libraries(network common rtnet)

if (WIN32)
  target_link_libraries(network ws2_32 iphlpapi)
endif()

target_compile_definitions(network PRIVATE ${SHAR_COMPILE_DEFINITIONS})
target_compile_options(network PRIVATE ${SHAR_COMPILE_OPTIONS})


add_subdirectory(rtnet)
add_subdirectory(tests)