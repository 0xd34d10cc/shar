#include "frame.hpp"


namespace shar {

Frame::Frame() noexcept
    : m_bytes(nullptr)
    , m_size(Size::empty()) {}

Frame::Frame(std::unique_ptr<uint8_t[]> raw_image, Size size, Clock::time_point time_stamp)
    : m_bytes(std::move(raw_image))
    , m_size(size)
    , m_timestamp(time_stamp) {}

Frame::Frame(Frame&& from) noexcept
    : m_bytes(std::move(from.m_bytes))
    , m_size(from.m_size)
    , m_timestamp(std::move(from.m_timestamp)) {

  from.m_size = Size::empty();
}

Frame& Frame::operator=(Frame&& from) noexcept {
  if (this != &from) {
    m_bytes = std::move(from.m_bytes);
    m_size  = from.m_size;
    m_timestamp = std::move(from.m_timestamp);

    from.m_size = Size::empty();
  }
  return *this;
}

}