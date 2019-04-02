add_library(rtnet
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
    rtcp/source_description.hpp
    rtcp/source_description.cpp
    rtcp/source_items.hpp
    rtcp/source_items.cpp
    rtcp/bye.hpp
    rtcp/bye.cpp
    rtcp/app.hpp
    rtcp/app.cpp

    byteorder.cpp
    byteorder.hpp
    slice.hpp
    slice.cpp
)

target_include_directories(rtnet PRIVATE . ../../common)
target_include_directories(rtnet PRIVATE SYSTEM ${CONAN_INCLUDE_DIRS_BOOST})
target_link_libraries(rtnet ${CONAN_LIBS_BOOST})

add_subdirectory(tests)
