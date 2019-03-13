include(screencapture)

add_library(capture
            capture.hpp
            capture.cpp
            )

target_include_directories(capture
    PUBLIC ..
    PUBLIC ../common
    PUBLIC SYSTEM ${CONAN_INCLUDE_DIRS_SCREENCAPTURE}
)

target_link_libraries(capture
    PRIVATE common
    PUBLIC ${SC_PLATFORM_LIBS}
    PUBLIC ${CONAN_LIBS_SCREENCAPTURELITE}
)

target_compile_definitions(encoder PRIVATE ${SHAR_COMPILE_DEFINITIONS})
target_compile_options(encoder PRIVATE ${SHAR_COMPILE_OPTIONS})