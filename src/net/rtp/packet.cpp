#include "packet.hpp"
#include "byteorder.hpp" // to_big_endian, read_big_endian

#include <cassert> // assert
#include <cstring> // memcpy


template <class T>
static bool is_aligned(const void *ptr) noexcept {
  auto iptr = reinterpret_cast<std::uintptr_t>(ptr);
  return !(iptr % alignof(T));
}

namespace shar::net::rtp {

Packet::Packet(u8* data, usize size) noexcept
  : BytesRefMut(data, size)
{
  assert(valid());
}

bool Packet::valid() const noexcept {
  return (m_data != nullptr) &&
         (m_size >= MIN_SIZE) &&
         // NOTE: this check is required only for contributors() method
         is_aligned<u32>(m_data);
}

u8 Packet::version() const noexcept {
  assert(valid());
  return m_data[0] >> 6;
}

void Packet::set_version(u8 version) noexcept {
  assert(valid());
  assert(version < 4);
  m_data[0] &= 0b111111; // reset top 2 bits
  m_data[0] |= (version << 6);
}

bool Packet::has_padding() const noexcept {
  assert(valid());
  return (m_data[0] & (1 << 5)) != 0;
}

void Packet::set_has_padding(bool has_padding) noexcept {
  assert(valid());
  if (has_padding) {
    m_data[0] |= u8{1 << 5};
  } else {
    m_data[0] &= ~u8{1 << 5};
  }
}

bool Packet::has_extensions() const noexcept {
  assert(valid());
  return (m_data[0] & (1 << 4)) != 0;
}

void Packet::set_has_extensions(bool has_extensions) noexcept {
  assert(valid());
  if (has_extensions) {
    m_data[0] |= u8{1 << 4};
  } else {
    m_data[0] &= ~u8{1 << 4};
  }
}

u8 Packet::contributors_count() const noexcept {
  assert(valid());
  return m_data[0] & 0b1111;
}

void Packet::set_contributors_count(u8 cc) noexcept {
  assert(valid());
  assert(cc < 16);
  m_data[0] &= 0b11110000;
  m_data[0] |= cc;
}

bool Packet::marked() const noexcept {
  assert(valid());
  return (m_data[1] & (1 << 7)) != 0;
}

void Packet::set_marked(bool mark) noexcept {
  assert(valid());
  if (mark) {
    m_data[1] |= u8{1 << 7};
  } else {
    m_data[1] &= ~u8{1 << 7};
  }
}

u8 Packet::payload_type() const noexcept {
  assert(valid());
  return m_data[1] & 0b1111111;
}

void Packet::set_payload_type(u8 type) noexcept {
  assert(valid());
  assert(type < 128);
  m_data[1] &= 0b10000000;
  m_data[1] |= type;
}

u16 Packet::sequence() const noexcept {
  assert(valid());
  return read_u16_big_endian(&m_data[2]);
}

void Packet::set_sequence(u16 seq) noexcept {
  assert(valid());
  const auto bytes = to_big_endian(seq);
  std::memcpy(&m_data[2], bytes.data(), bytes.size());
}

u32 Packet::timestamp() const noexcept {
  assert(valid());
  return read_u32_big_endian(&m_data[4]);
}

void Packet::set_timestamp(u32 t) noexcept {
  assert(valid());
  const auto bytes = to_big_endian(t);
  std::memcpy(&m_data[4], bytes.data(), bytes.size());
}

u32 Packet::stream_id() const noexcept {
  assert(valid());
  return read_u32_big_endian(&m_data[8]);
}

void Packet::set_stream_id(u32 id) noexcept {
  assert(valid());
  const auto bytes = to_big_endian(id);
  std::memcpy(&m_data[8], bytes.data(), bytes.size());
}

u32* Packet::contributors() noexcept {
  assert(valid());
  auto *ptr = m_data + MIN_SIZE;
  return reinterpret_cast<u32 *>(ptr);
}

usize Packet::payload_size() const noexcept {
  assert(valid());
  return m_size - MIN_SIZE;
}

// FIXME: this implementation assumes that has_extensions() is always false
u8* Packet::payload() noexcept {
  assert(valid());
  return m_data + MIN_SIZE +
         contributors_count() * sizeof(u32);
}

} // namespace shar::rtp
