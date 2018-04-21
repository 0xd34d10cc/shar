#pragma once

#include <cstdint>
#include <memory>

#include <ScreenCapture.h>

namespace shar {

class Image { // in BGRA format
public:
  Image()
    : m_width(0)
    , m_height(0)
    , m_bytes(nullptr)
  {}

  void assign(const SL::Screen_Capture::Image& image) {
    std::size_t width = Width(image); 
    std::size_t height = Height(image);
    std::size_t pixels = width * height;
    auto image_size = RowStride(image) * height;

    if (size() < pixels) {
      m_bytes = std::make_unique<uint8_t[]>(image_size);
    }

    m_width = width;
    m_height = height;

    SL::Screen_Capture::Extract(image, m_bytes.get(), image_size);
  }

  Image(Image&& from)
    : m_bytes(std::move(from.m_bytes))
    , m_width(from.m_width)
    , m_height(from.m_height)
  {
    from.m_width = 0;
    from.m_height = 0;
  }

  bool empty() const { return m_bytes.get() == nullptr; }
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