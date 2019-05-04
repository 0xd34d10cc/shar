#pragma once

#include <cstdint>
#include <cstddef>

#include "primitives/point.hpp"
#include "primitives/size.hpp"

namespace shar {

class Texture {
public:
  Texture(Size size) noexcept;
  ~Texture() noexcept;

  // this method reallocates and resizes texture
  void set(Size size, const std::uint8_t* data) noexcept;
  
  void bind() noexcept;
  void unbind() noexcept;

  // update part of texture
  void update(Point offset, Size size,
              const std::uint8_t* data) noexcept;

private:
  std::size_t m_id;
};

}