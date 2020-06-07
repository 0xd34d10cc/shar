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

  // update texture
  void update(Size size, const u8* data) noexcept;

  Size size() const noexcept {
    return m_size;
  }

private:
  usize m_id;
  Size m_size;
};

}