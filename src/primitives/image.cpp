#include "image.hpp"

namespace shar {

Image::Image() noexcept
    : m_size(Size::empty())
    , m_bytes(nullptr) {}

Image::Image(std::unique_ptr<uint8_t[]> raw_image, Size size)
    : m_size(size)
    , m_bytes(std::move(raw_image)) {}

Image::Image(Image&& from) noexcept
    : m_size(from.m_size)
    , m_bytes(std::move(from.m_bytes)) {
  from.m_size = Size::empty();
}

Image& Image::operator=(Image&& from) noexcept {
  if (this != &from) {
    m_bytes = std::move(from.m_bytes);
    m_size  = from.m_size;

    from.m_size = Size::empty();
  }
  return *this;
}

Image& Image::operator=(const SL::Screen_Capture::Image& image) noexcept {
  std::size_t width  = static_cast<std::size_t>(Width(image));
  std::size_t height = static_cast<std::size_t>(Height(image));
  std::size_t pixels = width * height;

  const std::size_t PIXEL_SIZE = 4;

  // FIXME: this reallocates when we assign big picture -> small picture -> big picture
  // TODO: introduce |capacity| member to fix it
  if (total_pixels() < pixels) {
    m_bytes = std::make_unique<uint8_t[]>(pixels * PIXEL_SIZE);
  }

  m_size = Size(height, width);

  assert(m_bytes.get() != nullptr);
  SL::Screen_Capture::Extract(image, m_bytes.get(), pixels * PIXEL_SIZE);

  return *this;
}

}