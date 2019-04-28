add_executable(rtnettest
    rtp.cpp

    rtcp/sender_report.cpp
    rtcp/receiver_report.cpp
    rtcp/source_description.cpp
    rtcp/bye.cpp
    rtcp/app.cpp

    rtsp/request.cpp 
    rtsp/response.cpp
)

target_include_directories(rtnettest
    PRIVATE ..
    PRIVATE ${PROJECT_SOURCE_DIR}/src/common
    PRIVATE ${CONAN_INCLUDE_DIRS_GTEST}
)

target_link_libraries(rtnettest
    rtnet
    ${CONAN_LIBS_GTEST}
) 

target_compile_definitions(rtnettest PRIVATE ${SHAR_COMPILE_DEFINITIONS})
target_compile_options(rtnettest PRIVATE ${SHAR_COMPILE_OPTIONS})

add_test(NAME rtnettest COMMAND rtnettest)