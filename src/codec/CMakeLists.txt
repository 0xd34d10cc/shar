add_library(codec
            ffmpeg/options.hpp
            ffmpeg/options.cpp
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
            decoder.hpp
            decoder.cpp
            )

target_include_directories(codec
    PUBLIC ..
    PUBLIC ../common
)

target_link_libraries(codec
    common
    ${CONAN_LIBS_FFMPEG}
    ${CONAN_LIBS_LIBX264}
)

target_compile_definitions(codec PRIVATE ${SHAR_COMPILE_DEFINITIONS})
target_compile_options(codec PRIVATE ${SHAR_COMPILE_OPTIONS})