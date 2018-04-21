#pragma once

#include <atomic>
#include <cstdint>

#include <GLFW/glfw3.h>

#include "texture.hpp"

namespace shar {

class Window {
public:
  using SystemWindow = GLFWwindow;

  Window();
  ~Window();

  bool should_close();
  void swap_buffers();
  void poll_events();
  void clear();

  void draw_texture(const Texture& texture);

private:
  static std::atomic<std::size_t> instances;
  static SystemWindow* create_window();

  SystemWindow* m_window;
};

}