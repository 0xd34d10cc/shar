#include "image.hpp"


namespace shar {

Image::Image() noexcept
    : m_bytes(nullptr)
    , m_size(Size::empty()) {}

Image::Image(std::unique_ptr<uint8_t[]> raw_image, Size size)
    : m_bytes(std::move(raw_image))
    , m_size(size) {}

Image::Image(Image&& from) noexcept
    : m_bytes(std::move(from.m_bytes))
    , m_size(from.m_size) {
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

}