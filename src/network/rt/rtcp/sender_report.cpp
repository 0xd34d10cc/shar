#include <cassert> // assert
#include <cstring> // memcpy

#include "sender_report.hpp"
#include "byteorder.hpp"


namespace shar::rtcp {

SenderReport::SenderReport(std::uint8_t* data, std::size_t size) noexcept
  : Header(data, size)
  {}

bool SenderReport::valid() const noexcept {
  return Header::valid() && 
         m_size >= SenderReport::MIN_SIZE &&
         m_size == (Header::length() + 1) * sizeof(std::uint32_t);
}

std::uint64_t SenderReport::ntp_timestamp() const noexcept {
  assert(valid());
  return read_u64_big_endian(m_data + Header::MIN_SIZE);
}

void SenderReport::set_ntp_timestamp(std::uint64_t timestamp) noexcept {
  assert(valid());
  const auto bytes = to_big_endian(timestamp);
  std::memcpy(m_data + Header::MIN_SIZE, bytes.data(), bytes.size());
}

std::uint32_t SenderReport::rtp_timestamp() const noexcept {
  assert(valid());
  return read_u32_big_endian(m_data + Header::MIN_SIZE + 8);
}

void SenderReport::set_rtp_timestamp(std::uint32_t timestamp) noexcept {
  assert(valid());
  const auto bytes = to_big_endian(timestamp);
  std::memcpy(m_data + Header::MIN_SIZE + 8, bytes.data(), bytes.size());
}

std::uint32_t SenderReport::npackets() const noexcept {
  assert(valid());
  return read_u32_big_endian(m_data + Header::MIN_SIZE + 12);
}

void SenderReport::set_npackets(std::uint32_t npackets) noexcept {
  assert(valid());
  const auto bytes = to_big_endian(npackets);
  std::memcpy(m_data + Header::MIN_SIZE + 12, bytes.data(), bytes.size());
}

std::uint32_t SenderReport::nbytes() const noexcept {
  assert(valid());
  return read_u32_big_endian(m_data + Header::MIN_SIZE + 16);
}

void SenderReport::set_nbytes(std::uint32_t nbytes) noexcept {
  assert(valid());
  const auto bytes = to_big_endian(nbytes);
  std::memcpy(m_data + Header::MIN_SIZE + 16, bytes.data(), bytes.size());
}

Block SenderReport::block(std::size_t index) noexcept {
  assert(valid());
  std::size_t offset = SenderReport::MIN_SIZE + Block::MIN_SIZE * index;
  if (offset + Block::MIN_SIZE > m_size) {
    return Block{};
  }

  return Block{m_data + offset, m_size - offset};
}

}