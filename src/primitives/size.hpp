#pragma once

#include <cstddef>

namespace shar {

class Size {
public:
  Size(std::size_t height, std::size_t width)
      : m_height(height)
      , m_width(width) {}

  Size(const Size&) = default;
  Size& operator=(const Size&) = default;

  static Size empty() {
    return Size{0, 0};
  }

  std::size_t width() const noexcept {
    return m_width;
  }

  std::size_t height() const noexcept {
    return m_height;
  }

  std::size_t total_pixels() const noexcept {
    return width() * height();
  }

  bool is_empty() const noexcept {
    return m_height == 0 || m_width == 0;
  }

private:
  std::size_t m_height;
  std::size_t m_width;
};

}