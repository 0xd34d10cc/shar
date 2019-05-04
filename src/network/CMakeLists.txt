add_library(network
    sender_factory.cpp
    sender_factory.hpp
    url.hpp
    url.cpp
    sender.hpp

    tcp/sender.hpp
    tcp/sender.cpp
    tcp/p2p_sender.hpp
    tcp/p2p_sender.cpp
    tcp/receiver.hpp
    tcp/receiver.cpp
    tcp/packet_parser.hpp
    tcp/packet_parser.cpp

    rtp/packetizer.hpp
    rtp/packetizer.cpp
    rtp/fragment.hpp
    rtp/fragment.cpp
    rtp/sender.hpp
    rtp/sender.cpp
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
add_subdirectory(rtp/tests)