add_library(encoder
            ffmpeg/options.hpp
            ffmpeg/options.cpp
            ffmpeg/avcontext.hpp
            ffmpeg/avcontext.cpp
            ffmpeg/codec.hpp
            ffmpeg/codec.cpp
            ffmpeg/unit.hpp
            ffmpeg/unit.cpp
            ffmpeg/frame.hpp
            ffmpeg/frame.cpp
            convert.cpp
            convert.hpp
            encoder.cpp
            encoder.hpp
            )

target_include_directories(encoder
    PUBLIC ..
    PUBLIC ../common
)

target_link_libraries(encoder
    common
    ${CONAN_LIBS_FFMPEG}
    ${CONAN_LIBS_LIBX264}
)

target_compile_definitions(encoder PRIVATE ${SHAR_COMPILE_DEFINITIONS})
target_compile_options(encoder PRIVATE ${SHAR_COMPILE_OPTIONS})