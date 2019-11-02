#pragma once

#include "int.hpp"

namespace shar {

struct Point {
  usize x;
  usize y;

  static Point origin() {
    return Point{0, 0};
  }
};

} // namespace shar
