#include <cassert> // assert

#include "bye.hpp"
#include "byteorder.hpp"


namespace shar::net::rtcp {

Bye::Bye(u8* data, usize size) noexcept
  : Header(data, size)
  {}

bool Bye::valid() const noexcept {
  return Header::valid() &&
         packet_type() == PacketType::BYE &&
         packet_size() >= Header::MIN_SIZE + nblocks() * sizeof(u32);
}

u32 Bye::stream_id(usize index) noexcept {
  assert(valid());
  return read_u32_big_endian(m_data + Header::MIN_SIZE + 4 * index);
}

bool Bye::has_reason() const noexcept {
  assert(valid());
  return packet_size() > Header::MIN_SIZE + nblocks() * sizeof(u32);
}

u8* Bye::reason() noexcept {
  assert(valid());
  assert(has_reason());
  return m_data + Header::MIN_SIZE + nblocks() * sizeof(u32) + 1;
}

u8 Bye::reason_len() const noexcept {
  assert(valid());
  assert(has_reason());
  usize offset = Header::MIN_SIZE + nblocks() * sizeof(u32);
  return m_data[offset];
}

void Bye::set_reason_len(u8 len) noexcept {
  assert(valid());
  assert(has_reason());
  usize offset = Header::MIN_SIZE + nblocks() * sizeof(u32);
  m_data[offset] = len;
}

}