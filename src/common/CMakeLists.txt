add_library(common
            logger.hpp
            options.hpp
            options.cpp
            context.hpp
            channel.hpp
            size.hpp
            disable_warnings_push.hpp
            disable_warnings_pop.hpp
            cancellation.hpp
            cancellation.cpp
            # metrics
            metrics/gauge.cpp
            metrics/gauge.hpp
            metrics/histogram.cpp
            metrics/histogram.hpp
            metrics/registry.cpp
            metrics/registry.hpp
            metrics/description.hpp
            )

target_include_directories(common
    PRIVATE .
    PRIVATE SYSTEM ${CONAN_INCLUDE_DIRS_JSONFORMODERNCPP}
    PRIVATE SYSTEM ${CONAN_INCLUDE_DIRS_CLI11}

    PUBLIC SYSTEM ${CONAN_INCLUDE_DIRS_SPDLOG}
    PUBLIC SYSTEM "${CONAN_INCLUDE_DIRS_PROMETHEUS-CPP}/push/include"
    PUBLIC SYSTEM "${CONAN_INCLUDE_DIRS_PROMETHEUS-CPP}/pull/include"
    PUBLIC SYSTEM "${CONAN_INCLUDE_DIRS_PROMETHEUS-CPP}/core/include"
)

target_link_libraries(common
  ${CONAN_LIBS_FMT}
  ${CONAN_LIBS_PROMETHEUS-CPP}
)

target_compile_definitions(common PRIVATE ${SHAR_COMPILE_DEFINITIONS})
target_compile_options(common PRIVATE ${SHAR_COMPILE_OPTIONS})

