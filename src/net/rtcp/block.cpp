#include <cassert> // assert
#include <cstring> // memcpy

#include "block.hpp"
#include "byteorder.hpp"


namespace shar::net::rtcp {

Block::Block(u8* data, usize size) noexcept
  : BytesRefMut(data, size)
  {}

bool Block::valid() const noexcept {
  return m_data != nullptr && m_size >= Block::MIN_SIZE;
}

u32 Block::stream_id() const noexcept {
  assert(valid());
  return read_u32_big_endian(m_data);
}

void Block::set_stream_id(u32 stream_id) noexcept {
  assert(valid());
  const auto bytes = to_big_endian(stream_id);
  std::memcpy(m_data, bytes.data(), bytes.size());
}

u8 Block::fraction_lost() const noexcept {
  assert(valid());
  return m_data[4];
}

void Block::set_fraction_lost(u8 lost) noexcept {
  assert(valid());
  m_data[4] = lost;
}

u32 Block::packets_lost() const noexcept {
  assert(valid());
  return (u32{m_data[5]} << 16) |
         (u32{m_data[6]} << 8)  |
         (u32{m_data[7]} << 0);
}

void Block::set_packets_lost(u32 lost) noexcept {
  assert(valid());
  m_data[5] = static_cast<u8>((lost & 0xff0000) >> 16);
  m_data[6] = static_cast<u8>((lost & 0x00ff00) >> 8);
  m_data[7] = static_cast<u8>((lost & 0x0000ff) >> 0);
}

u32 Block::last_sequence() const noexcept {
  assert(valid());
  return read_u32_big_endian(&m_data[8]);
}

void Block::set_last_sequence(u32 seq) noexcept {
  assert(valid());
  const auto bytes = to_big_endian(seq);
  std::memcpy(&m_data[8], bytes.data(), bytes.size());
}

u32 Block::jitter() const noexcept {
  assert(valid());
  return read_u32_big_endian(&m_data[12]);
}

void Block::set_jitter(u32 jitter) noexcept {
  assert(valid());
  const auto bytes = to_big_endian(jitter);
  std::memcpy(&m_data[12], bytes.data(), bytes.size());
}

u32 Block::last_sender_report_timestamp() const noexcept {
  assert(valid());
  return read_u32_big_endian(&m_data[16]);
}

void Block::set_last_sender_report_timestamp(u32 timestamp) noexcept {
  assert(valid());
  const auto bytes = to_big_endian(timestamp);
  std::memcpy(&m_data[16], bytes.data(), bytes.size());
}

u32 Block::delay_since_last_sender_report() const noexcept {
  assert(valid());
  return read_u32_big_endian(&m_data[20]);
}

void Block::set_delay_since_last_sender_report(u32 delay) noexcept {
  assert(valid());
  const auto bytes = to_big_endian(delay);
  std::memcpy(&m_data[20], bytes.data(), bytes.size());
}

Block Block::next() noexcept {
  assert(valid());

  if (m_size < Block::MIN_SIZE * 2) {
    return Block{};
  }

  return Block{m_data + Block::MIN_SIZE, m_size - Block::MIN_SIZE};
}

}
