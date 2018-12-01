add_library(rt
    rtp/packet.cpp 
    rtp/packet.hpp

    rtcp/header.hpp
    rtcp/header.cpp
    rtcp/block.hpp
    rtcp/block.cpp
    rtcp/sender_report.hpp
    rtcp/sender_report.cpp
    rtcp/receiver_report.hpp
    rtcp/receiver_report.cpp

    byteorder.cpp
    byteorder.hpp
)

target_include_directories(rt PRIVATE .)

enable_testing()
add_subdirectory(tests)