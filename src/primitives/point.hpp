#pragma once

#include <cstddef>


namespace shar {

class Point {
public:
  Point(std::size_t x, std::size_t y)
      : m_x(x)
      , m_y(y) {}

  Point(const Point&) = default;

  static Point origin() {
    return Point {0, 0};
  }

  std::size_t x() const noexcept {
    return m_x;
  }

  std::size_t y() const noexcept {
    return m_y;
  }

private:
  std::size_t m_x;
  std::size_t m_y;
};

}