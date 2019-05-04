#pragma once


namespace shar {

struct Point {
  int x;
  int y;

  static Point origin() {
    return Point{0, 0};
  }
};

}