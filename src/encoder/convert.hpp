#pragma once

#include <cstdint>
#include <memory>
#include <array>


namespace shar {
class Frame;
}

namespace shar::encoder {

using Bytes = std::unique_ptr<std::uint8_t[]>;

struct Slice {
  Bytes       data;
  std::size_t size;
};


Slice yuv420_to_bgra(const std::uint8_t* ys,
                     const std::uint8_t* us,
                     const std::uint8_t* vs,
                     std::size_t height, std::size_t width,
                     std::size_t y_pad, std::size_t uv_pad);


std::array<Slice, 3> bgra_to_yuv420(const Frame& image);

}