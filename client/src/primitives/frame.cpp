#include "frame.hpp"


namespace shar {

Frame::Frame() noexcept
    : m_bytes(nullptr)
    , m_size(Size::empty()) {}

Frame::Frame(std::unique_ptr<uint8_t[]> raw_image, Size size)
    : m_bytes(std::move(raw_image))
    , m_size(size) {}

Frame::Frame(Frame&& from) noexcept
    : m_bytes(std::move(from.m_bytes))
    , m_size(from.m_size) {
  from.m_size = Size::empty();
}

Frame& Frame::operator=(Frame&& from) noexcept {
  if (this != &from) {
    m_bytes = std::move(from.m_bytes);
    m_size  = from.m_size;

    from.m_size = Size::empty();
  }
  return *this;
}

}