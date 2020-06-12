#pragma once

#include <cstdint>
#include <memory>

#include "size.hpp"


namespace shar::codec {

using Bytes = std::unique_ptr<u8[]>;

struct Slice {
  Bytes data{ nullptr };
  usize size{ 0 };
};

struct YUVImage: Slice {
  usize y_size{ 0 };
  usize u_size{ 0 };
  usize v_size{ 0 };
};


Slice yuv420_to_bgra(const u8* ys,
                     const u8* us,
                     const u8* vs,
                     usize height, usize width,
                     usize y_pad, usize uv_pad);


YUVImage bgra_to_yuv420(const char* data, Size size);

}