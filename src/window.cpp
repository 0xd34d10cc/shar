#include <stdexcept>

#include <iostream> // DEBUG

#include "window.hpp"


namespace shar {

std::atomic<std::size_t> Window::instances{ 0 };

Window::Window()
  : m_window(create_window()) {
  ++instances;
}

Window::~Window() {
  --instances;
  
  // not sure if glfw can work with multiple window instances
  if (instances == 0) {
    glfwTerminate();
  }
}

bool Window::should_close() {
  return glfwWindowShouldClose(m_window);
}

void Window::poll_events() {
  glfwPollEvents();
}

void Window::swap_buffers() {
  glfwSwapBuffers(m_window);
}

void Window::clear() {
  glClear(GL_COLOR_BUFFER_BIT);
}

Window::SystemWindow* Window::create_window() {
  static int init_code = glfwInit();
    if (!init_code) {
      throw std::runtime_error("Failed to initialize GLFW");
    }

    SystemWindow* window = glfwCreateWindow(1920, 1080, "shar", NULL, NULL);
    if (!window) {
      throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    return window;
}


void Window::draw_texture(const Texture& texture) {
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