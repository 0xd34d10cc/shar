#pragma once

#include <atomic>
#include <cstddef>

#include <GLFW/glfw3.h>

#include "texture.hpp"

namespace shar {

class Window {
public:
  using SystemWindow = GLFWwindow;

  Window(std::size_t width, std::size_t height);
  ~Window() noexcept;

  bool should_close() noexcept;
  void swap_buffers() noexcept;
  void poll_events() noexcept;
  void clear() noexcept;

  void draw_texture(const Texture& texture) noexcept;

private:
  static std::atomic<std::size_t> instances;
  static SystemWindow* create_window(std::size_t width, std::size_t height);

  SystemWindow* m_window;
};

}