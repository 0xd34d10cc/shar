#include "packet.hpp"
#include "byteorder.hpp" // to_big_endian, read_big_endian

#include <cassert> // assert
#include <cstring> // memcpy

namespace {
template <class T> bool is_aligned(const void *ptr) noexcept {
  auto iptr = reinterpret_cast<std::uintptr_t>(ptr);
  return !(iptr % alignof(T));
}
} // namespace

namespace shar::rtp {

Packet::Packet() 
  : m_buffer(nullptr)
  , m_size(0) 
  {}

Packet::Packet(std::uint8_t *data, std::size_t size)
  : m_buffer(data)
  , m_size(size) {
  assert(valid());
}

Packet::Packet(const Packet &other)
  : m_buffer(other.m_buffer)
  , m_size(other.m_size) 
  {}

Packet::Packet(Packet &&other)
  : m_buffer(other.m_buffer)
  , m_size(other.m_size) {
  other.m_buffer = nullptr;
  other.m_size = 0;
}

Packet &Packet::operator=(const Packet &other) {
  if (this != &other) {
    m_buffer = other.m_buffer;
    m_size = other.m_size;
  }

  return *this;
}

Packet &Packet::operator=(Packet &&other) {
  if (this != &other) {
    m_buffer = other.m_buffer;
    m_size = other.m_size;

    other.m_buffer = nullptr;
    other.m_size = 0;
  }

  return *this;
}

bool Packet::valid() const {
  // for now just check if header fits
  return (m_buffer != nullptr) && 
         (m_size >= MIN_HEADER_SIZE) &&
         is_aligned<std::uint32_t>(m_buffer);
}

const std::uint8_t *Packet::data() const { 
  return m_buffer; 
}

std::size_t Packet::size() const { 
  return m_size; 
}

std::uint8_t Packet::version() const {
  assert(valid());
  return m_buffer[0] >> 6;
}

void Packet::set_version(std::uint8_t version) {
  assert(valid());
  assert(version < 4);
  m_buffer[0] |= (version << 6);
}

bool Packet::has_padding() const {
  assert(valid());
  return (m_buffer[0] & (1 << 5)) != 0;
}

void Packet::set_has_padding(bool has_padding) {
  assert(valid());
  if (has_padding) {
    m_buffer[0] |= std::uint8_t{1 << 5};
  } else {
    m_buffer[0] &= ~std::uint8_t{1 << 5};
  }
}

bool Packet::has_extensions() const {
  assert(valid());
  return (m_buffer[0] & (1 << 4)) != 0;
}

void Packet::set_has_extensions(bool has_extensions) {
  assert(valid());
  if (has_extensions) {
    m_buffer[0] |= std::uint8_t{1 << 4};
  } else {
    m_buffer[0] &= ~std::uint8_t{1 << 4};
  }
}

std::uint8_t Packet::contributors_count() const {
  assert(valid());
  return m_buffer[0] & 0b1111;
}

void Packet::set_contributors_count(std::uint8_t cc) {
  assert(valid());
  assert(cc < 16);
  m_buffer[0] &= 0b11110000;
  m_buffer[0] |= cc;
}

bool Packet::marked() const {
  assert(valid());
  return m_buffer[1] & (1 << 7);
}

void Packet::set_marked(bool mark) {
  assert(valid());
  if (mark) {
    m_buffer[1] |= std::uint8_t{1 << 7};
  } else {
    m_buffer[1] &= ~std::uint8_t{1 << 7};
  }
}

std::uint8_t Packet::payload_type() const {
  assert(valid());
  return m_buffer[1] & 0b1111111;
}

void Packet::set_payload_type(std::uint8_t type) {
  assert(valid());
  assert(type < 128);
  m_buffer[1] &= 0b10000000;
  m_buffer[1] |= type;
}

std::uint16_t Packet::sequence() const {
  assert(valid());
  return read_u16_big_endian(&m_buffer[2]);
}

void Packet::set_sequence(std::uint16_t seq) {
  assert(valid());
  const auto bytes = to_big_endian(seq);
  std::memcpy(&m_buffer[2], bytes.data(), bytes.size());
}

// FIXME: assumes little-endian host
std::uint32_t Packet::timestamp() const {
  assert(valid());
  return read_u32_big_endian(&m_buffer[4]);
}

void Packet::set_timestamp(std::uint32_t t) {
  assert(valid());
  const auto bytes = to_big_endian(t);
  std::memcpy(&m_buffer[4], bytes.data(), bytes.size());
}

std::uint32_t Packet::stream_id() const {
  assert(valid());
  return read_u32_big_endian(&m_buffer[8]);
}

void Packet::set_stream_id(std::uint32_t id) {
  assert(valid());
  const auto bytes = to_big_endian(id);
  std::memcpy(&m_buffer[8], bytes.data(), bytes.size());
}

std::uint32_t *Packet::contributors() {
  assert(valid());
  auto *ptr = m_buffer + MIN_HEADER_SIZE;
  return reinterpret_cast<std::uint32_t *>(ptr);
}

std::size_t Packet::payload_size() const {
  assert(valid());
  return m_size - MIN_HEADER_SIZE;
}

// FIXME: this implementation assumes that has_extensions() is always false
std::uint8_t *Packet::payload() {
  assert(valid());
  return m_buffer + MIN_HEADER_SIZE +
         contributors_count() * sizeof(std::uint32_t);
}

} // namespace shar::rtp