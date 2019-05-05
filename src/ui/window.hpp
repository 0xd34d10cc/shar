#pragma once

#include <atomic>
#include <cstddef>

#include "texture.hpp"
#include "logger.hpp"
#include "size.hpp"


struct GLFWwindow;

namespace shar::ui {

class Window {
public:
  using SystemWindow = GLFWwindow;

  Window(Size size, Logger logger);
  Window(Window&&) noexcept;
  Window(const Window&) = delete;
  Window& operator=(Window&&) noexcept;
  ~Window() noexcept;

  bool should_close() noexcept;
  void swap_buffers() noexcept;
  void poll_events() noexcept;
  void clear() noexcept;

  std::size_t width() const noexcept;
  std::size_t height() const noexcept;
  Size size() const noexcept;

  void draw_texture(const Texture& texture) noexcept;

private:
  void reset() noexcept;
  void lazy_init() noexcept;

  static std::atomic<std::size_t> instances;
  SystemWindow* create_window(Size size);

  SystemWindow* m_window{ nullptr };
  Size m_size;
  Logger m_logger;
};

}