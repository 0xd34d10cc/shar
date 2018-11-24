#include "convert.hpp"
#include "primitives/frame.hpp"


namespace shar::codecs {

static Slice alloc(std::size_t size) {
  return {std::make_unique<std::uint8_t[]>(size), size};
}


std::array<Slice, 3> bgra_to_yuv420(const shar::Frame& image) {
  Slice ys = alloc(image.total_pixels());
  Slice us = alloc(image.total_pixels() / 4);
  Slice vs = alloc(image.total_pixels() / 4);

  const auto luma = [](uint8_t r, uint8_t g, uint8_t b) {
    return static_cast<uint8_t >(((66 * r + 129 * g + 25 * b) >> 8) + 16);
  };

  const std::uint8_t* raw_image = image.bytes();
  std::size_t i  = 0;
  std::size_t ui = 0;

  for (std::size_t line = 0; line < image.height(); ++line) {
    if (line % 2 == 0) {
      for (std::size_t x = 0; x < image.width(); x += 2) {
        uint8_t r = raw_image[4 * i + 2];
        uint8_t g = raw_image[4 * i + 1];
        uint8_t b = raw_image[4 * i];

        uint8_t y = luma(r, g, b);
        auto u = static_cast<uint8_t>(((-38 * r + -74 * g + 112 * b) >> 8) + 128);
        auto v = static_cast<uint8_t>(((112 * r + -94 * g + -18 * b) >> 8) + 128);

        ys.data[i]  = y;
        us.data[ui] = u;
        vs.data[ui] = v;

        i++;
        ui++;

        r = raw_image[4 * i + 2];
        g = raw_image[4 * i + 1];
        b = raw_image[4 * i];

        y = luma(r, g, b);
        ys.data[i] = y;
        i++;
      }
    }
    else {
      for (size_t x = 0; x < image.width(); x += 1) {
        uint8_t r = raw_image[4 * i + 2];
        uint8_t g = raw_image[4 * i + 1];
        uint8_t b = raw_image[4 * i];

        uint8_t y = luma(r, g, b);
        ys.data[i] = y;
        i++;
      }
    }
  }

  return {std::move(ys), std::move(us), std::move(vs)};
}

Slice yuv420_to_bgra(const std::uint8_t* ys,
                     const std::uint8_t* us,
                     const std::uint8_t* vs,
                     std::size_t height, std::size_t width,
                     std::size_t y_pad, std::size_t uv_pad) {

  std::size_t y_width = width + y_pad;
  std::size_t u_width = width / 2 + uv_pad;
  std::size_t i       = 0;

  auto bgra = alloc(height * width * 4);

  for (std::size_t line = 0; line < height; ++line) {
    for (std::size_t coll = 0; coll < width; ++coll) {

      uint8_t y = ys[line * y_width + coll];
      uint8_t u = us[(line / 2) * u_width + (coll / 2)];
      uint8_t v = vs[(line / 2) * u_width + (coll / 2)];

      int c = y - 16;
      int d = u - 128;
      int e = v - 128;

      uint8_t r = static_cast<uint8_t>(shar::codecs::clamp((298 * c + 409 * e + 128) >> 8, 0, 255));
      uint8_t g = static_cast<uint8_t>(shar::codecs::clamp((298 * c - 100 * d - 208 * e + 128) >> 8, 0, 255));
      uint8_t b = static_cast<uint8_t>(shar::codecs::clamp((298 * c + 516 * d + 128) >> 8, 0, 255));

      bgra.data[i + 0] = b;
      bgra.data[i + 1] = g;
      bgra.data[i + 2] = r;
      bgra.data[i + 3] = 0;

      i += 4;
    }
  }

  return bgra;
}

}