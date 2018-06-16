#pragma once

#include <atomic>
#include <cstddef>

#include "disable_warnings_push.hpp"
#include <GLFW/glfw3.h>
#include "disable_warnings_pop.hpp"

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

  std::size_t width() const noexcept;
  std::size_t height() const noexcept;

  void draw_texture(const Texture& texture) noexcept;

private:
  static std::atomic<std::size_t> instances;
  static SystemWindow* create_window(std::size_t width, std::size_t height);

  SystemWindow* m_window;
  std::size_t m_width;
  std::size_t m_height;
};

}