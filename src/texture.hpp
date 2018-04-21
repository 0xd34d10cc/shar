#pragma once

#include <cstdint>

namespace shar {

class Texture {
public:
  Texture();
  ~Texture();

  // TODO: texture storage

  void set(std::size_t width, std::size_t height, const uint8_t* bytes);
  void bind();
  void unbind();

  // update part of texture
  void update(std::size_t x_offset, std::size_t y_offset,
              std::size_t width,    std::size_t height,
              const uint8_t* data);

private:
  std::size_t m_id;
};

}