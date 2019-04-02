# Setup warnings
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    # using Clang
    set(ENABLED_WARNINGS
        -Werror
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
	)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # using GCC
    set(ENABLED_WARNINGS -Wall -Wextra -pedantic)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    # using Intel C++
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    # using Visual Studio C++
endif ()
