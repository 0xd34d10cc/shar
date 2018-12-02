#include <cassert> // assert

#include "bye.hpp"
#include "byteorder.hpp"


namespace shar::rtcp {

Bye::Bye(std::uint8_t* data, std::size_t size) noexcept
  : Header(data, size)
  {}

bool Bye::valid() const noexcept {
  return Header::valid() &&
         packet_type() == PacketType::BYE &&
         packet_size() >= Header::MIN_SIZE + nblocks() * sizeof(std::uint32_t);
}

std::uint32_t Bye::stream_id(std::size_t index) noexcept {
  assert(valid());
  return read_u32_big_endian(m_data + Header::MIN_SIZE + 4 * index);
}

bool Bye::has_reason() const noexcept {
  assert(valid());
  return packet_size() > Header::MIN_SIZE + nblocks() * sizeof(std::uint32_t);
}

std::uint8_t* Bye::reason() noexcept {
  assert(valid());
  assert(has_reason());
  return m_data + Header::MIN_SIZE + nblocks() * sizeof(std::uint32_t) + 1; 
}

std::uint8_t Bye::reason_len() const noexcept {
  assert(valid());
  assert(has_reason());
  std::size_t offset = Header::MIN_SIZE + nblocks() * sizeof(std::uint32_t);
  return m_data[offset];
}

void Bye::set_reason_len(std::uint8_t len) noexcept {
  assert(valid());
  assert(has_reason());
  std::size_t offset = Header::MIN_SIZE + nblocks() * sizeof(std::uint32_t);
  m_data[offset] = len;
}

}