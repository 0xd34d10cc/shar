#include <stdexcept>

#include "window.hpp"

#include <iostream>

namespace shar {

std::atomic<std::size_t> Window::instances{ 0 };

Window::Window(std::size_t width, std::size_t height)
  : m_window(create_window(width, height)) {
  ++instances;
}

Window::~Window() noexcept {
  --instances;
  
  if (instances == 0) {
    glfwTerminate();
  }
}

bool Window::should_close() noexcept {
  return glfwWindowShouldClose(m_window);
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

Window::SystemWindow* Window::create_window(std::size_t width, std::size_t height) {
  static int init_code = glfwInit();
  glfwSetErrorCallback(on_error);

  if (!init_code) {
    throw std::runtime_error("Failed to initialize GLFW");
  }

  SystemWindow* window = glfwCreateWindow(width, height, "shar", NULL, NULL);
  if (!window) {
    throw std::runtime_error("Failed to create GLFW window");
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  return window;
}


void Window::draw_texture(const Texture& texture) noexcept {
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