add_library(net
    types.hpp

    url.hpp
    url.cpp

    dns.hpp
    dns.cpp

    sender.hpp
    sender_factory.hpp
    sender_factory.cpp

    receiver.hpp
    receiver_factory.hpp
    receiver_factory.cpp

    tcp/sender.hpp
    tcp/sender.cpp
    tcp/p2p_sender.hpp
    tcp/p2p_sender.cpp
    tcp/receiver.hpp
    tcp/receiver.cpp
    tcp/packet_parser.hpp
    tcp/packet_parser.cpp

    rtp/packet.cpp
    rtp/packet.hpp
    rtp/packetizer.hpp
    rtp/packetizer.cpp
    rtp/fragment.hpp
    rtp/fragment.cpp
    rtp/sender.hpp
    rtp/sender.cpp
    rtp/depacketizer.hpp
    rtp/depacketizer.cpp
    rtp/receiver.hpp
    rtp/receiver.cpp

    rtcp/header.hpp
    rtcp/header.cpp
    rtcp/block.hpp
    rtcp/block.cpp
    rtcp/sender_report.hpp
    rtcp/sender_report.cpp
    rtcp/receiver_report.hpp
    rtcp/receiver_report.cpp
    rtcp/source_description.hpp
    rtcp/source_description.cpp
    rtcp/source_items.hpp
    rtcp/source_items.cpp
    rtcp/bye.hpp
    rtcp/bye.cpp
    rtcp/app.hpp
    rtcp/app.cpp

    rtsp/request.hpp
    rtsp/request.cpp
    rtsp/response.hpp
    rtsp/response.cpp
    rtsp/parser.hpp
    rtsp/parser.cpp
    rtsp/header.hpp
    rtsp/header.cpp
    rtsp/server.hpp
    rtsp/server.cpp
    rtsp/error.hpp

    stun/message.hpp
    stun/message.cpp
    stun/attributes.hpp
    stun/attributes.cpp
    stun/error.hpp
    stun/request.hpp
    stun/request.cpp

    ice/candidate.hpp
    ice/candidate.cpp
    ice/forwarding.hpp
    ice/forwarding.cpp
    ice/client.hpp
    ice/client.cpp
)

target_include_directories(net
    PUBLIC ..
    PUBLIC SYSTEM ${CONAN_INCLUDE_DIRS_ASIO}
)

target_link_libraries(net
  PUBLIC common
  PUBLIC proto
  PUBLIC ${CONAN_LIBS_MINIUPNPC}
)

if (WIN32)
  target_link_libraries(net
    PUBLIC ws2_32
    PUBLIC iphlpapi
  )
else()
  target_link_libraries(net PUBLIC pthread)
endif()


target_compile_definitions(net PRIVATE ${SHAR_COMPILE_DEFINITIONS} ASIO_HAS_STD_STRING_VIEW)
target_compile_options(net PRIVATE ${SHAR_COMPILE_OPTIONS})

# tests
add_executable(nettest
    rtp/tests/packet.cpp
    rtp/tests/packetizer.cpp
    rtp/tests/depacketizer.cpp

    rtsp/tests/request.cpp
    rtsp/tests/response.cpp

    rtcp/tests/sender_report.cpp
    rtcp/tests/receiver_report.cpp
    rtcp/tests/source_description.cpp
    rtcp/tests/bye.cpp
    rtcp/tests/app.cpp

    stun/tests/message.cpp
)

target_include_directories(nettest
    PRIVATE ${CONAN_INCLUDE_DIRS_GTEST}
)

target_link_libraries(nettest
    PRIVATE net
    PRIVATE common
    PRIVATE ${CONAN_LIBS_GTEST}
)

target_compile_definitions(nettest PRIVATE ${SHAR_COMPILE_DEFINITIONS})
target_compile_options(nettest PRIVATE ${SHAR_COMPILE_OPTIONS})

add_test(NAME nettest COMMAND nettest)

# test STUN client
add_executable(stunc stun/stunc.cpp)

target_include_directories(stunc
    PRIVATE SYSTEM ${CONAN_INCLUDE_DIRS_ASIO}
)

target_link_libraries(stunc
    PRIVATE common
    PRIVATE net
)

target_compile_definitions(stunc PRIVATE ${SHAR_COMPILE_DEFINITIONS})
target_compile_options(stunc PRIVATE ${SHAR_COMPILE_OPTIONS})
