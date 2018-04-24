#pragma once

#include <cstdint>
#include <memory>

#include <ScreenCapture.h>

namespace shar {

class Image { // in BGRA format
public:
  Image();
  Image(Image&&);
  ~Image() = default;

  void assign(const SL::Screen_Capture::Image& image);

  inline bool empty() const { return m_bytes.get() == nullptr || (size() == 0); }
  inline uint8_t* bytes() { return m_bytes.get(); }
  inline const uint8_t* bytes() const { return m_bytes.get(); }
  inline std::size_t size() const { return m_width * m_height; }
  inline std::size_t width() const { return m_width; }
  inline std::size_t height() const { return m_height; }

private:
  std::unique_ptr<uint8_t[]> m_bytes;

  std::size_t m_width;
  std::size_t m_height;
};

}