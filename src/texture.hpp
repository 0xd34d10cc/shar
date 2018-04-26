#pragma once

#include <cstdint>
#include <cstddef>

namespace shar {

class Texture {
public:
  Texture(std::size_t width, std::size_t height) noexcept;
  ~Texture() noexcept;

  // this method reallocates and resizes texture
  void set(std::size_t width, std::size_t height, const std::uint8_t* data) noexcept;
  
  void bind() noexcept;
  void unbind() noexcept;

  // update part of texture
  void update(std::size_t x_offset, std::size_t y_offset,
              std::size_t width,    std::size_t height,
              const std::uint8_t* data) noexcept;

private:
  std::size_t m_id;
};

}