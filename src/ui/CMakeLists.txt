add_library(ui
    texture.hpp
    texture.cpp
    point.hpp
    rect.hpp
    gl_vtable.hpp
    gl_vtable.cpp
    state.hpp
    state.cpp
    renderer.hpp
    renderer.cpp
    window.hpp
    window.cpp
    message_box.hpp
    message_box.cpp
    button.hpp
    button.cpp
    nk.hpp
    nk.cpp
    text_edit.hpp
    text_edit.cpp
)

target_include_directories(ui
    PUBLIC ..
    PUBLIC ../common
    PRIVATE SYSTEM ${CONAN_INCLUDE_DIRS_SDL2}
)

find_package(OpenGL REQUIRED)

target_link_libraries(ui
    common
    ${CONAN_LIBS_SDL2}
    ${OPENGL_gl_LIBRARY}
)

target_compile_definitions(ui PRIVATE ${SHAR_COMPILE_DEFINITIONS})
target_compile_options(ui PRIVATE ${SHAR_COMPILE_OPTIONS})