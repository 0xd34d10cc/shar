#include <stdexcept>
#include <iostream>

#include "disable_warnings_push.hpp"

#define GL_GLEXT_PROTOTYPES

#include <GLFW/glfw3.h>
#include "disable_warnings_pop.hpp"

#include "window.hpp"

namespace {

void GLAPIENTRY opengl_error_callback(GLenum /* source */,
                                      GLenum type,
                                      GLuint /* id */,
                                      GLenum severity,
                                      GLsizei /* length */,
                                      const GLchar* message,
                                      const void* /* userParam */) {
  std::cerr << "[GL]:" << (type == GL_DEBUG_TYPE_ERROR ? " !ERROR! " : "")
            << "type = " << type
            << ", severity = " << severity
            << ", message = " << message
            << std::endl;
}

}

namespace shar {

std::atomic<std::size_t> Window::instances {0};

Window::Window(Size size)
    : m_window(create_window(size))
    , m_size(size) {
  ++instances;
}

Window::~Window() noexcept {
  --instances;

  if (instances == 0) {
    glfwTerminate();
  }
}

bool Window::should_close() noexcept {
  return glfwWindowShouldClose(m_window) != 0;
}

void Window::poll_events() noexcept {
  glfwPollEvents();
}

void Window::swap_buffers() noexcept {
  glfwSwapBuffers(m_window);
}

void Window::clear() noexcept {
  glClear(GL_COLOR_BUFFER_BIT);
}

static void on_error(int code, const char* description) {
  std::cerr << "GLFW Error [" << code << "]: " << description << std::endl;
}

Window::SystemWindow* Window::create_window(Size size) {
  static int init_code = glfwInit();
  glfwSetErrorCallback(on_error);

  // TODO: disable in release
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

  if (!init_code) {
    throw std::runtime_error("Failed to initialize GLFW");
  }

  SystemWindow* window = glfwCreateWindow(static_cast<int>(size.width()),
                                          static_cast<int>(size.height()),
                                          "shar", nullptr, nullptr);
  if (!window) {
    throw std::runtime_error("Failed to create GLFW window");
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);
  std::cout << "OpenGL " << glGetString(GL_VERSION) << std::endl;
  glEnable(GL_TEXTURE_2D);

  // TODO: disable in release
  // enable debug output
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(opengl_error_callback, nullptr);

  return window;
}

std::size_t Window::width() const noexcept {
  return m_size.width();
}

std::size_t Window::height() const noexcept {
  return m_size.height();
}

Size Window::size() const noexcept {
  return m_size;
}

void Window::draw_texture(const Texture& /* texture */) noexcept {
  glBegin(GL_QUADS);

  glTexCoord2f(0, 0);
  glVertex3f(-1, 1, 0);

  glTexCoord2f(0, 1);
  glVertex3f(-1, -1, 0);

  glTexCoord2f(1, 1);
  glVertex3f(1, -1, 0);

  glTexCoord2f(1, 0);
  glVertex3f(1, 1, 0);

  glEnd();
}


}