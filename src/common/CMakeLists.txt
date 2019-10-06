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
            atomic_string.hpp
            atomic_string.cpp
            bufwriter.hpp
            bufwriter.cpp
            newtype.hpp
            metrics.cpp
            metrics.hpp
            )

target_include_directories(common
    PUBLIC .
    PRIVATE SYSTEM ${CONAN_INCLUDE_DIRS_JSONFORMODERNCPP}
    PRIVATE SYSTEM ${CONAN_INCLUDE_DIRS_CLI11}

    PUBLIC SYSTEM ${CONAN_INCLUDE_DIRS_SPDLOG}
)

target_link_libraries(common
  PUBLIC ${CONAN_LIBS_FMT}
)

target_compile_definitions(common PRIVATE ${SHAR_COMPILE_DEFINITIONS})
target_compile_options(common PRIVATE ${SHAR_COMPILE_OPTIONS})

