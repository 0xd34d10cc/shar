add_executable(networktest
    packetizer.cpp
    ../packetizer.cpp
    ../packetizer.hpp
    ../fragment.cpp
    ../fragment.cpp
)

target_include_directories(networktest
    PRIVATE ${PROJECT_SOURCE_DIR}/src/common
    PRIVATE ..
    PRIVATE ${CONAN_INCLUDE_DIRS_GTEST}
)

target_link_libraries(networktest
    ${CONAN_LIBS_GTEST}
)

add_test(NAME networktest COMMAND networktest)
