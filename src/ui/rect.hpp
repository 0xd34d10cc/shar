#pragma once

#include "point.hpp"
#include "size.hpp"


namespace shar {

class Rect {
public:
  static Rect empty() {
    return Rect(Point::origin(), Size::empty());
  }

  Rect(Point at, Size size)
    : m_at(at)
    , m_size(size)
  {}

  Rect(const Rect&) = default;
  Rect& operator=(const Rect&) = default;
  ~Rect() = default;

  bool contains(Point p) {
    return m_at.x <= p.x && p.x <= m_at.x + m_size.width() &&
           m_at.y <= p.y && p.y <= m_at.y + m_size.height();
  }

private:
  Point m_at;
  Size m_size;
};

}