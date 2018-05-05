#include "image.hpp"

namespace shar {

Image::Image() noexcept
  : m_width(0)
  , m_height(0)
  , m_bytes(nullptr)
{}

Image::Image(std::unique_ptr<uint8_t[]> raw_image, size_t height, size_t width)
  : m_width(width)
  , m_height(height)
  , m_bytes(std::move(raw_image)) 
{}

Image::Image(Image&& from) noexcept
  : m_width(from.m_width)
  , m_height(from.m_height)
  , m_bytes(std::move(from.m_bytes))
{
  from.m_width = 0;
  from.m_height = 0;
}

Image& Image::operator=(Image&& from) noexcept {
  if (this != &from) {
    m_bytes = std::move(from.m_bytes);
    m_height = from.m_height;
    m_width = from.m_width;

    from.m_width = 0;
    from.m_height = 0;
  }
  return *this;
}

void Image::assign(const SL::Screen_Capture::Image& image) noexcept {
  std::size_t width = Width(image);
  std::size_t height = Height(image);
  std::size_t pixels = width * height;
  int image_size = RowStride(image) * height;

  if (size() < pixels) {
    m_bytes = std::make_unique<uint8_t[]>(image_size);
  }

  m_width = width;
  m_height = height;

  SL::Screen_Capture::Extract(image, m_bytes.get(), image_size);
}

}