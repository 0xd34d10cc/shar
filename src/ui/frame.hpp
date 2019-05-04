#pragma once

#include <cstdint>
#include <memory>

#include "size.hpp"


namespace shar::ui {

// in BGRA
class Frame {
public:
  Frame() = default;
  Frame(std::unique_ptr<std::uint8_t[]> data, Size size);
  Frame(Frame&&) noexcept = default;
  Frame& operator=(Frame&&) noexcept = default;
  ~Frame() = default;

  std::uint8_t* bytes() noexcept { 
    return m_data.get();
  }

  const std::uint8_t* bytes() const noexcept {
    return m_data.get();
  }

  Size size() const noexcept {
    return m_size;
  }

private:
  std::unique_ptr<std::uint8_t[]> m_data;
  Size m_size;
};

}