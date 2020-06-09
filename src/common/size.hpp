#pragma once

#include "int.hpp"


namespace shar {

class Size {
public:
  Size(usize height, usize width)
      : m_height(height)
      , m_width(width) {}

  Size(const Size&) = default;
  Size(Size&&) noexcept = default;
  Size& operator=(const Size&) = default;
  Size& operator=(Size&&) noexcept = default;
  ~Size() = default;

  bool operator==(const Size& rhs) const noexcept {
    return m_height == rhs.m_height && m_width == rhs.m_width;
  }

  bool operator!=(const Size& rhs) const noexcept {
    return !(*this == rhs);
  }

  static Size empty() {
    return Size{0, 0};
  }

  usize width() const noexcept {
    return m_width;
  }

  usize height() const noexcept {
    return m_height;
  }

  usize total_pixels() const noexcept {
    return width() * height();
  }

  bool is_empty() const noexcept {
    return m_height == 0 || m_width == 0;
  }

private:
  usize m_height;
  usize m_width;
};

}