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
  : m_data(data)
  , m_size(size) {
  assert(valid());
}

Packet::Packet(Packet&& other) noexcept
  : m_data(other.m_data)
  , m_size(other.m_size) {
  other.m_data = nullptr;
  other.m_size = 0;
}

Packet& Packet::operator=(Packet&& other) noexcept {
  if (this != &other) {
    m_data = other.m_data;
    m_size = other.m_size;

    other.m_data = nullptr;
    other.m_size = 0;
  }

  return *this;
}

bool Packet::valid() const noexcept {
  return (m_data != nullptr) &&
         (m_size >= MIN_SIZE) &&
         // NOTE: this check is required only for contributors() method
         is_aligned<std::uint32_t>(m_data);
}

const std::uint8_t* Packet::data() const noexcept {
  return m_data;
}

std::size_t Packet::size() const noexcept {
  return m_size;
}

std::uint8_t Packet::version() const noexcept {
  assert(valid());
  return m_data[0] >> 6;
}

void Packet::set_version(std::uint8_t version) noexcept {
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
    m_data[0] |= std::uint8_t{1 << 5};
  } else {
    m_data[0] &= ~std::uint8_t{1 << 5};
  }
}

bool Packet::has_extensions() const noexcept {
  assert(valid());
  return (m_data[0] & (1 << 4)) != 0;
}

void Packet::set_has_extensions(bool has_extensions) noexcept {
  assert(valid());
  if (has_extensions) {
    m_data[0] |= std::uint8_t{1 << 4};
  } else {
    m_data[0] &= ~std::uint8_t{1 << 4};
  }
}

std::uint8_t Packet::contributors_count() const noexcept {
  assert(valid());
  return m_data[0] & 0b1111;
}

void Packet::set_contributors_count(std::uint8_t cc) noexcept {
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
    m_data[1] |= std::uint8_t{1 << 7};
  } else {
    m_data[1] &= ~std::uint8_t{1 << 7};
  }
}

std::uint8_t Packet::payload_type() const noexcept {
  assert(valid());
  return m_data[1] & 0b1111111;
}

void Packet::set_payload_type(std::uint8_t type) noexcept {
  assert(valid());
  assert(type < 128);
  m_data[1] &= 0b10000000;
  m_data[1] |= type;
}

std::uint16_t Packet::sequence() const noexcept {
  assert(valid());
  return read_u16_big_endian(&m_data[2]);
}

void Packet::set_sequence(std::uint16_t seq) noexcept {
  assert(valid());
  const auto bytes = to_big_endian(seq);
  std::memcpy(&m_data[2], bytes.data(), bytes.size());
}

std::uint32_t Packet::timestamp() const noexcept {
  assert(valid());
  return read_u32_big_endian(&m_data[4]);
}

void Packet::set_timestamp(std::uint32_t t) noexcept {
  assert(valid());
  const auto bytes = to_big_endian(t);
  std::memcpy(&m_data[4], bytes.data(), bytes.size());
}

std::uint32_t Packet::stream_id() const noexcept {
  assert(valid());
  return read_u32_big_endian(&m_data[8]);
}

void Packet::set_stream_id(std::uint32_t id) noexcept {
  assert(valid());
  const auto bytes = to_big_endian(id);
  std::memcpy(&m_data[8], bytes.data(), bytes.size());
}

std::uint32_t* Packet::contributors() noexcept {
  assert(valid());
  auto *ptr = m_data + MIN_SIZE;
  return reinterpret_cast<std::uint32_t *>(ptr);
}

std::size_t Packet::payload_size() const noexcept {
  assert(valid());
  return m_size - MIN_SIZE;
}

// FIXME: this implementation assumes that has_extensions() is always false
std::uint8_t* Packet::payload() noexcept {
  assert(valid());
  return m_data + MIN_SIZE +
         contributors_count() * sizeof(std::uint32_t);
}

} // namespace shar::rtp