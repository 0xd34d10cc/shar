add_executable(networktest
    packetizer.cpp
)

target_include_directories(networktest
    PRIVATE ..
    PRIVATE ${CONAN_INCLUDE_DIRS_GTEST}
)

target_link_libraries(networktest
    network
    ${CONAN_LIBS_GTEST}
)

add_test(NAME networktest COMMAND networktest)