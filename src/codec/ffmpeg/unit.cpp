#include <cassert>

#include "disable_warnings_push.hpp"
extern "C" {
#include <libavcodec/avcodec.h>
}
#include "disable_warnings_pop.hpp"

#include "unit.hpp"


namespace shar::codec::ffmpeg {

Unit::Unit(AVPacket* packet)
  : m_packet(packet)
  {}

Unit Unit::allocate() noexcept {
  return Unit(av_packet_alloc());
}

Unit Unit::from_data(std::uint8_t* data, std::size_t size) {
  auto unit = Unit::allocate();
  AVBufferRef* buffer = av_buffer_alloc(size);
  std::memcpy(buffer->data, data, size);
  assert(unit.raw()->buf == nullptr);
  unit.raw()->buf = buffer;
  unit.raw()->data = buffer->data;
  unit.raw()->size = buffer->size;
  return unit;
}

bool Unit::empty() const noexcept {
  return m_packet == nullptr ||
         m_packet->data == nullptr ||
         m_packet->size == 0;
}

std::uint8_t* Unit::data() noexcept {
  return m_packet ? m_packet->data : nullptr;
}

const std::uint8_t* Unit::data() const noexcept {
  return m_packet ? m_packet->data : nullptr;
}

std::size_t Unit::size() const noexcept {
  return m_packet ? static_cast<std::size_t>(m_packet->size) : 0;
}

std::uint32_t Unit::timestamp() const noexcept {
  return m_packet ? static_cast<std::uint32_t>(m_packet->pts) : 0;
}

Unit::Type Unit::type() const noexcept {
  bool is_idr = m_packet && (m_packet->flags & AV_PKT_FLAG_KEY) != 0;
  return is_idr ? Type::IDR : Type::Unknown;
}

AVPacket* Unit::raw() noexcept {
  return m_packet.get();
}

}
