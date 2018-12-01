add_executable(rtnettest rtp.cpp rtcp.cpp)

target_include_directories(rtnettest
    PRIVATE ..
    PRIVATE ${CONAN_INCLUDE_DIRS_GTEST}
)

target_link_libraries(rtnettest
    rtnet
    ${CONAN_LIBS_GTEST}
)

add_test(NAME rtnettest COMMAND rtnettest)