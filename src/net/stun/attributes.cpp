#include <cassert>

#include "attributes.hpp"
#include "byteorder.hpp"


namespace shar::net::stun {

Attributes::Attributes(std::uint8_t* data, std::size_t size) noexcept
  : Slice(data, size)
  {}

void Attributes::reset() noexcept {
  m_pos = 0;
}

bool Attributes::read(Attribute& attr) noexcept {
  assert(valid());
  if (m_pos >= m_size || (m_size - m_pos) < 4) {
    return false;
  }

  std::uint8_t* p = m_data + m_pos;
  attr.type = read_u16_big_endian(p);
  attr.length = read_u16_big_endian(p + 2);
  attr.data = p + 4;

  std::size_t padding = (attr.length & 0b11) == 0 ? 0 : (4 - attr.length & 0b11);
  m_pos += 4 + attr.length + padding;
  assert(valid());
  return true;
}

bool Attributes::append(Attribute attr) noexcept {
  assert(valid());
  if (m_pos >= m_size || (m_size - m_pos) < 4 + attr.length) {
    return false;
  }

  std::uint8_t* p = m_data + m_pos;
  auto bytes = to_big_endian(attr.type);
  std::memcpy(p, bytes.data(), bytes.size());
  bytes = to_big_endian(attr.length);
  std::memcpy(p + 2, bytes.data(), bytes.size());
  if (attr.length != 0) {
    std::memcpy(p + 4, attr.data, attr.length);
  }

  std::size_t padding = (attr.length & 0b11) == 0 ? 0 : (4 - attr.length & 0b11);
  m_pos = 4 + attr.length + padding;
  assert(valid());
  return true;
}

bool Attributes::valid() const noexcept {
  return m_pos % 4 == 0;
}

}