# Setup warnings
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    # using Clang
    set(ENABLED_WARNINGS
        -Weverything
        -Wno-c++98-compat
        -Wno-c++98-compat-pedantic
        -Wno-padded
        -Wno-newline-eof
        -Wno-missing-braces
        -Wno-global-constructors
        -Wno-exit-time-destructors
        -Wno-unused-command-line-argument # because ccache
        -Wno-undefined-func-template # false positives
        -Wno-weak-vtables
        -Wno-covered-switch-default
        -Wno-undef # _MSC_VER is undefined
        -Wno-format-nonliteral # we have to trust |fmt| arg provided to avlog_callback
        -Wno-shadow-uncaptured-local
)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # using GCC
    set(ENABLED_WARNINGS -Wall -Wextra -pedantic)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    # using Intel C++
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    # using Visual Studio C++

    # CMake appends /W3 by default, and having /W3 followed by /W4 will result in
    # cl : Command line warning D9025 : overriding '/W3' with '/W4'.  Since this is
    # a command line warning and not a compiler warning, it cannot be suppressed except
    # by fixing the command line.
    string(REGEX REPLACE " /W[0-4]" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    string(REGEX REPLACE " /W[0-4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

    set(ENABLED_WARNINGS /W4 /wd4324 /D_CRT_SECURE_NO_WARNINGS)
endif ()
