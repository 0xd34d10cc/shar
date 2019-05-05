#pragma once

#include <cstdint>
#include <cstddef>

#include "point.hpp"
#include "size.hpp"

namespace shar::ui {

class Texture {
public:
  explicit Texture(Size size) noexcept;
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