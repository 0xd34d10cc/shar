add_library(rt
    rtp/packet.cpp 
    rtp/packet.hpp

    rtcp/header.hpp
    rtcp/header.cpp

    byteorder.cpp
    byteorder.hpp
)

target_include_directories(rt PRIVATE .)

enable_testing()
add_subdirectory(tests)