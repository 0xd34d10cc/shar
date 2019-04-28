#include <cstring> // memcpy
#include <cassert> // assert

#include "header.hpp"
#include "byteorder.hpp"


namespace shar::rtcp {

Header::Header(std::uint8_t* data, std::size_t size) noexcept
  : Slice(data, size)
  {}

bool Header::valid() const noexcept {
  return (m_data != nullptr) && (m_size >= Header::MIN_SIZE);
}

const std::uint8_t* Header::data() const noexcept {
  return m_data;
}

std::uint8_t* Header::data() noexcept {
  return m_data;
}

std::size_t Header::size() const noexcept {
  return m_size;
}

std::uint8_t Header::version() const noexcept {
  assert(valid());
  return m_data[0] >> 6;
}

void Header::set_version(std::uint8_t version) noexcept {
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
    m_data[0] |= std::uint8_t{1 << 5};
  } else {
    m_data[0] &= ~std::uint8_t{1 << 5};
  }
}

std::uint8_t Header::nblocks() const noexcept {
  assert(valid());
  return m_data[0] & 0b11111;
}

void Header::set_nblocks(std::uint8_t nblocks) noexcept {
  assert(valid());
  assert(nblocks < 32);
  m_data[0] &= 0b11100000;
  m_data[0] |= nblocks;
}

std::uint8_t Header::packet_type() const noexcept {
  assert(valid());
  return m_data[1];
}

void Header::set_packet_type(std::uint8_t type) noexcept {
  assert(valid());
  m_data[1] = type;
}

std::uint16_t Header::length() const noexcept {
  assert(valid());
  return read_u16_big_endian(&m_data[2]);
}

void Header::set_length(std::uint16_t length) noexcept {
  assert(valid());
  const auto bytes = to_big_endian(length);
  std::memcpy(&m_data[2], bytes.data(), bytes.size());
}

std::size_t Header::packet_size() const noexcept {
  assert(valid());
  return (length() + 1) * sizeof(std::uint32_t);
}

Header Header::next() noexcept {
  assert(valid());
  std::uint16_t len = packet_size();
  if (len + Header::MIN_SIZE > m_size) {
    return Header{};
  }

  return Header{m_data + len, m_size - len};
}

}
