add_library(common
            logger.hpp
            error_or.hpp
            env.hpp
            env.cpp
            config.hpp
            config.cpp
            context.hpp
            channel.hpp
            size.hpp
            time.hpp
            time.cpp
            disable_warnings_push.hpp
            disable_warnings_pop.hpp
            cancellation.hpp
            cancellation.cpp
            bytes.hpp
            bytes_ref.hpp
            byteorder.hpp
            byteorder.cpp
            bufwriter.hpp
            bufwriter.cpp
            newtype.hpp
            metrics.cpp
            metrics.hpp
            png_image.hpp
            png_image.cpp
            )

target_include_directories(common
    PUBLIC .
    PRIVATE SYSTEM ${CONAN_INCLUDE_DIRS_JSONFORMODERNCPP}
    PRIVATE SYSTEM ${CONAN_INCLUDE_DIRS_CLI11}
    PRIVATE SYSTEM ${CONAN_INCLUDE_DIRS_LIBPNG}
    PUBLIC SYSTEM ${CONAN_INCLUDE_DIRS_SPDLOG}
)

target_link_libraries(common
  PUBLIC ${CONAN_LIBS_FMT}
  PRIVATE ${CONAN_LIBS_LIBPNG}
  PRIVATE ${CONAN_LIBS_ZLIB}
)

target_compile_definitions(common PRIVATE ${SHAR_COMPILE_DEFINITIONS})
target_compile_options(common PRIVATE ${SHAR_COMPILE_OPTIONS})

