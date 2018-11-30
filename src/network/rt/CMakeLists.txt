add_library(rt
    rtp/packet.cpp 
    rtp/packet.hpp

    byteorder.cpp
    byteorder.hpp
)

target_include_directories(rt PRIVATE .)

enable_testing()
add_subdirectory(tests)