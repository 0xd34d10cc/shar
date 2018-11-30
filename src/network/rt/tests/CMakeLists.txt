add_executable(rttest rtp.cpp rtcp.cpp)

target_include_directories(rttest 
    PRIVATE ..
    PRIVATE ${CONAN_INCLUDE_DIRS_GTEST}
)

target_link_libraries(rttest
    rt
    ${CONAN_LIBS_GTEST}
)

add_test(NAME rttest COMMAND rttest)