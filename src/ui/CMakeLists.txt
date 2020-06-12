add_library(ui
    texture.hpp
    texture.cpp
    point.hpp
    rect.hpp
    gl_vtable.hpp
    gl_vtable.cpp
    state.hpp
    state.cpp
    nk.hpp
    nk.cpp
    renderer.hpp
    renderer.cpp
    window.hpp
    window.cpp
    controls/message_box.hpp
    controls/message_box.cpp
    controls/button.hpp
    controls/button.cpp
    controls/text_edit.hpp
    controls/text_edit.cpp
)

target_include_directories(ui
    PUBLIC ..
    PUBLIC SYSTEM ${CONAN_INCLUDE_DIRS_SDL2}
)

find_package(OpenGL REQUIRED)

target_link_libraries(ui
    PUBLIC common
    PUBLIC ${CONAN_LIBS_SDL2}
    PUBLIC ${OPENGL_gl_LIBRARY}
)

target_compile_definitions(ui PRIVATE ${SHAR_COMPILE_DEFINITIONS})
target_compile_options(ui PRIVATE ${SHAR_COMPILE_OPTIONS})