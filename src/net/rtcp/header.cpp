#include <cstring> // memcpy
#include <cassert> // assert

#include "header.hpp"
#include "byteorder.hpp"


namespace shar::net::rtcp {

Header::Header(u8* data, usize size) noexcept
  : BytesRefMut(data, size)
  {}

bool Header::valid() const noexcept {
  return (m_data != nullptr) && (m_size >= Header::MIN_SIZE);
}

const u8* Header::data() const noexcept {
  return m_data;
}

u8* Header::data() noexcept {
  return m_data;
}

usize Header::size() const noexcept {
  return m_size;
}

u8 Header::version() const noexcept {
  assert(valid());
  return m_data[0] >> 6;
}

void Header::set_version(u8 version) noexcept {
  assert(valid());
  assert(version < 4);
  m_data[0] &= 0b111111;
  m_data[0] |= (version << 6);
}

bool Header::has_padding() const noexcept {
  assert(valid());
  return (m_data[0] & (1 << 5)) != 0;
}

void Header::set_has_padding(bool has_padding) noexcept {
  assert(valid());
  if (has_padding) {
    m_data[0] |= u8{1 << 5};
  } else {
    m_data[0] &= ~u8{1 << 5};
  }
}

u8 Header::nblocks() const noexcept {
  assert(valid());
  return m_data[0] & 0b11111;
}

void Header::set_nblocks(u8 nblocks) noexcept {
  assert(valid());
  assert(nblocks < 32);
  m_data[0] &= 0b11100000;
  m_data[0] |= nblocks;
}

u8 Header::packet_type() const noexcept {
  assert(valid());
  return m_data[1];
}

void Header::set_packet_type(u8 type) noexcept {
  assert(valid());
  m_data[1] = type;
}

u16 Header::length() const noexcept {
  assert(valid());
  return read_u16_big_endian(&m_data[2]);
}

void Header::set_length(u16 length) noexcept {
  assert(valid());
  const auto bytes = to_big_endian(length);
  std::memcpy(&m_data[2], bytes.data(), bytes.size());
}

usize Header::packet_size() const noexcept {
  assert(valid());
  return (usize{length()} + 1) * sizeof(u32);
}

Header Header::next() noexcept {
  assert(valid());
  u16 len = static_cast<u16>(packet_size());
  if (len + Header::MIN_SIZE > m_size) {
    return Header{};
  }

  return Header{m_data + len, m_size - len};
}

}
