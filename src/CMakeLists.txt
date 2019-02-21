add_subdirectory(network)

add_executable(shar
               # application-level code
               app/main.cpp
               app/app.hpp
               app/app.cpp
               app/signal_handler.cpp
               app/signal_handler.hpp
               # common code
               common/counter.cpp
               common/counter.hpp
               common/metrics.cpp
               common/metrics.hpp
               common/logger.hpp
               common/config.hpp
               common/context.hpp
               common/channel.hpp
               common/size.hpp
               common/disable_warnings_push.hpp
               common/disable_warnings_pop.hpp
               common/newtype.hpp
               common/cancellation.hpp
               common/cancellation.cpp
               # encoder subsystem
               encoder/convert.cpp
               encoder/convert.hpp
               encoder/ffmpeg/options.hpp
               encoder/ffmpeg/options.cpp
               encoder/ffmpeg/avcontext.hpp
               encoder/ffmpeg/avcontext.cpp
               encoder/ffmpeg/codec.hpp
               encoder/ffmpeg/codec.cpp
               encoder/encoder.cpp
               encoder/encoder.hpp
               # capture subsystem
               capture/capture.cpp
               capture/capture.hpp
               capture/frame.cpp
               capture/frame.hpp
               )

target_include_directories(shar
                           PRIVATE .
                           PRIVATE ./common
                           PRIVATE SYSTEM ${CONAN_INCLUDE_DIRS_BOOST}
                           PRIVATE SYSTEM ${CONAN_INCLUDE_DIRS_FFMPEG}
                           PRIVATE SYSTEM ${CONAN_INCLUDE_DIRS_SPDLOG}
                           PRIVATE SYSTEM "${CONAN_INCLUDE_DIRS_PROMETHEUS-CPP}/push/include"
                           PRIVATE SYSTEM "${CONAN_INCLUDE_DIRS_PROMETHEUS-CPP}/pull/include"
                           PRIVATE SYSTEM "${CONAN_INCLUDE_DIRS_PROMETHEUS-CPP}/core/include"
                           )

target_link_libraries(shar
                      network
                      ${SC_PLATFORM_LIBS}
                      ${CONAN_LIBS_SCREENCAPTURELITE}
                      ${CONAN_LIBS_FFMPEG}
                      ${CONAN_LIBS_LIBX264}
                      ${CONAN_LIBS_BOOST}
                      ${CONAN_LIBS_FMT}
                      ${CONAN_LIBS_PROMETHEUS-CPP}
                      )

if (WIN32)
  target_link_libraries(shar ws2_32 iphlpapi)
endif ()

target_compile_definitions(shar PRIVATE ${SHAR_COMPILE_DEFINITIONS})
target_compile_options(shar PRIVATE ${SHAR_COMPILE_OPTIONS})

IF (NOT WIN32)
    message("-- Enabling debug info")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3 -fno-omit-frame-pointer")

    if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
      message("-- Enabling LTO")
      set_property(TARGET shar PROPERTY INTERPROCEDURAL_OPTIMIZATION True)
    endif ()
endif ()