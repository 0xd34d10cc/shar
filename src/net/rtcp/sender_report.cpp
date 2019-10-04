#include <cassert> // assert
#include <cstring> // memcpy

#include "sender_report.hpp"
#include "byteorder.hpp"


namespace shar::net::rtcp {

SenderReport::SenderReport(u8* data, usize size) noexcept
  : Header(data, size)
  {}

bool SenderReport::valid() const noexcept {
  return Header::valid() &&
         m_size >= SenderReport::MIN_SIZE &&
         m_size == packet_size() &&
         packet_type() == PacketType::SENDER_REPORT;
}

u32 SenderReport::stream_id() const noexcept {
  assert(valid());
  return read_u32_big_endian(m_data + Header::MIN_SIZE);
}

void SenderReport::set_stream_id(u32 stream_id) noexcept {
  assert(valid());
  const auto bytes = to_big_endian(stream_id);
  std::memcpy(m_data + Header::MIN_SIZE, bytes.data(), bytes.size());
}

u64 SenderReport::ntp_timestamp() const noexcept {
  assert(valid());
  return read_u64_big_endian(m_data + Header::MIN_SIZE + 4);
}

void SenderReport::set_ntp_timestamp(u64 timestamp) noexcept {
  assert(valid());
  const auto bytes = to_big_endian(timestamp);
  std::memcpy(m_data + Header::MIN_SIZE + 4, bytes.data(), bytes.size());
}

u32 SenderReport::rtp_timestamp() const noexcept {
  assert(valid());
  return read_u32_big_endian(m_data + Header::MIN_SIZE + 12);
}

void SenderReport::set_rtp_timestamp(u32 timestamp) noexcept {
  assert(valid());
  const auto bytes = to_big_endian(timestamp);
  std::memcpy(m_data + Header::MIN_SIZE + 12, bytes.data(), bytes.size());
}

u32 SenderReport::npackets() const noexcept {
  assert(valid());
  return read_u32_big_endian(m_data + Header::MIN_SIZE + 16);
}

void SenderReport::set_npackets(u32 npackets) noexcept {
  assert(valid());
  const auto bytes = to_big_endian(npackets);
  std::memcpy(m_data + Header::MIN_SIZE + 16, bytes.data(), bytes.size());
}

u32 SenderReport::nbytes() const noexcept {
  assert(valid());
  return read_u32_big_endian(m_data + Header::MIN_SIZE + 20);
}

void SenderReport::set_nbytes(u32 nbytes) noexcept {
  assert(valid());
  const auto bytes = to_big_endian(nbytes);
  std::memcpy(m_data + Header::MIN_SIZE + 20, bytes.data(), bytes.size());
}

Block SenderReport::block(usize index) noexcept {
  assert(valid());
  usize offset = SenderReport::MIN_SIZE + Block::MIN_SIZE * index;
  if (offset + Block::MIN_SIZE > m_size) {
    return Block{};
  }

  return Block{m_data + offset, m_size - offset};
}

}