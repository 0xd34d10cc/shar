# platfor specific libraries for screen_capture_lite
if (WIN32)
    set(SC_PLATFORM_LIBS Dwmapi)
elseif (APPLE)
    find_package(Threads REQUIRED)
    find_library(corefoundation_lib CoreFoundation REQUIRED)
    find_library(cocoa_lib Cocoa REQUIRED)
    find_library(coremedia_lib CoreMedia REQUIRED)
    find_library(avfoundation_lib AVFoundation REQUIRED)
    find_library(coregraphics_lib CoreGraphics REQUIRED)
    find_library(corevideo_lib CoreVideo REQUIRED)

    set(SC_PLATFORM_LIBS
            ${corefoundation_lib}
            ${cocoa_lib}
            ${coremedia_lib}
            ${avfoundation_lib}
            ${coregraphics_lib}
            ${corevideo_lib})
else ()
    find_package(X11 REQUIRED)
    if (!X11_XTest_FOUND)
        message(FATAL_ERROR "X11 extensions are required, but not found!")
    endif ()
    if (!X11_Xfixes_LIB)
        message(FATAL_ERROR "X11 fixes extension is required, but not found!")
    endif ()
    find_package(Threads REQUIRED)
    set(SC_PLATFORM_LIBS
            ${X11_LIBRARIES}
            ${X11_Xfixes_LIB}
            ${X11_XTest_LIB}
            ${X11_Xinerama_LIB}
            ${CMAKE_THREAD_LIBS_INIT}
            )
endif ()