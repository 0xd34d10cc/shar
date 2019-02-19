#pragma once

#include <cstdint>
#include <cstddef>
#include <chrono>
#include <memory>

#include "size.hpp"

namespace shar {

using Clock = std::chrono::high_resolution_clock;

class Frame { // in BGRA format
public:
  Frame() noexcept;
  Frame(std::unique_ptr<std::uint8_t[]> raw_image, shar::Size size, const Clock::time_point& time_stamp);
  Frame(const Frame&) = delete;
  Frame(Frame&&) noexcept;
  Frame& operator=(const Frame&) = delete;
  Frame& operator=(Frame&&) noexcept;
  ~Frame() = default;

  static const std::size_t PIXEL_SIZE = 4;

  bool empty() const noexcept {
    return m_bytes == nullptr || m_size.is_empty();
  }

  std::uint8_t* bytes() noexcept {
    return m_bytes.get();
  }

  const std::uint8_t* bytes() const noexcept {
    return m_bytes.get();
  }

  std::size_t total_pixels() const noexcept {
    return m_size.total_pixels();
  }

  std::size_t size_bytes() const noexcept {
    return total_pixels() * PIXEL_SIZE;
  }

  shar::Size size() const noexcept {
    return m_size;
  }

  std::size_t width() const noexcept {
    return m_size.width();
  }

  std::size_t height() const noexcept {
    return m_size.height();
  }

  Clock::time_point timestamp() const noexcept {
    return m_timestamp;
  }

private:
  std::unique_ptr<uint8_t[]> m_bytes;
  shar::Size                 m_size;
  Clock::time_point m_timestamp;
};

}