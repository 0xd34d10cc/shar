#pragma once

#include <cstdint>
#include <memory>

#include "disable_warnings_push.hpp"
#include <ScreenCapture.h>
#include "disable_warnings_pop.hpp"


namespace shar {

class Image { // in BGRA format
public:
  Image() noexcept;
  Image(std::unique_ptr<std::uint8_t[]> raw_image, std::size_t height, std::size_t width);
  Image(Image&&) noexcept;
  Image& operator=(Image&&) noexcept;
  Image& operator=(const SL::Screen_Capture::Image& image) noexcept;
  ~Image() = default;


  inline bool empty() const noexcept { return m_bytes.get() == nullptr || size() == 0; }
  inline std::uint8_t* bytes() noexcept { return m_bytes.get(); }
  inline const std::uint8_t* bytes() const noexcept { return m_bytes.get(); }
  inline std::size_t size() const noexcept { return m_width * m_height; }
  inline std::size_t width() const noexcept { return m_width; }
  inline std::size_t height() const noexcept { return m_height; }

private:
  std::unique_ptr<uint8_t[]> m_bytes;

  std::size_t m_width;
  std::size_t m_height;
};

}