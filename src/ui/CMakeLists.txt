add_library(ui
    display.hpp
    display.cpp
    window.hpp
    window.cpp
    texture.hpp
    texture.cpp
    point.hpp
    frame.hpp
)

target_include_directories(ui
    PUBLIC ..
    PUBLIC ../common
    PRIVATE SYSTEM ${CONAN_INCLUDE_DIRS_GLFW}
)

find_package(OpenGL REQUIRED)

target_link_libraries(ui
    common
    ${CONAN_LIBS_GLFW}
    ${OPENGL_gl_LIBRARY}
)

target_compile_definitions(ui PRIVATE ${SHAR_COMPILE_DEFINITIONS})
target_compile_options(ui PRIVATE ${SHAR_COMPILE_OPTIONS})