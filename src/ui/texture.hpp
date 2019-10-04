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
  void set(Size size, const u8* data) noexcept;

  void bind() noexcept;
  void unbind() noexcept;

  // update part of texture
  void update(Point offset, Size size,
              const u8* data) noexcept;

private:
  usize m_id;
};

}