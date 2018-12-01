#include <cassert> // assert
#include <cstring> // memcpy

#include "block.hpp"
#include "byteorder.hpp"


namespace shar::rtcp {

Block::Block(std::uint8_t* data, std::size_t size) noexcept
  : m_data(data)
  , m_size(size)
  {}

Block::Block(Block&& other) noexcept
  : m_data(other.m_data)
  , m_size(other.m_size) {
  other.m_data = nullptr;
  other.m_size = 0;
}

Block& Block::operator=(Block&& other) noexcept {
  if (this != &other) {
    m_data = other.m_data;
    m_size = other.m_size;

    other.m_data = nullptr;
    other.m_size = 0;
  }

  return *this;
}

bool Block::valid() const noexcept {
  return m_data != nullptr && m_size >= Block::MIN_SIZE;
}

std::uint32_t Block::stream_id() const noexcept {
  assert(valid());
  return read_u32_big_endian(m_data);
}

void Block::set_stream_id(std::uint32_t stream_id) noexcept {
  assert(valid());
  const auto bytes = to_big_endian(stream_id);
  std::memcpy(m_data, bytes.data(), bytes.size());
}

std::uint8_t Block::fraction_lost() const noexcept {
  assert(valid());
  return m_data[4];
}

void Block::set_fraction_lost(std::uint8_t lost) noexcept {
  assert(valid());
  m_data[4] = lost;
}

std::uint32_t Block::packets_lost() const noexcept {
  assert(valid());
  return (std::uint32_t{m_data[5]} << 16) |
         (std::uint32_t{m_data[6]} << 8)  |
         (std::uint32_t{m_data[7]} << 0);
}

void Block::set_packets_lost(std::uint32_t lost) noexcept {
  assert(valid());
  m_data[5] = static_cast<std::uint8_t>((lost & 0xff0000) >> 16);
  m_data[6] = static_cast<std::uint8_t>((lost & 0x00ff00) >> 8);
  m_data[7] = static_cast<std::uint8_t>((lost & 0x0000ff) >> 0);
}

std::uint32_t Block::last_sequence() const noexcept {
  assert(valid());
  return read_u32_big_endian(&m_data[8]);
}

void Block::set_last_sequence(std::uint32_t seq) noexcept {
  assert(valid());
  const auto bytes = to_big_endian(seq);
  std::memcpy(&m_data[8], bytes.data(), bytes.size());
}

std::uint32_t Block::jitter() const noexcept {
  assert(valid());
  return read_u32_big_endian(&m_data[12]);
}

void Block::set_jitter(std::uint32_t jitter) noexcept {
  assert(valid());
  const auto bytes = to_big_endian(jitter);
  std::memcpy(&m_data[12], bytes.data(), bytes.size());
}

std::uint32_t Block::last_sender_report_timestamp() const noexcept {
  assert(valid());
  return read_u32_big_endian(&m_data[16]);
}

void Block::set_last_sender_report_timestamp(std::uint32_t timestamp) noexcept {
  assert(valid());
  const auto bytes = to_big_endian(timestamp);
  std::memcpy(&m_data[16], bytes.data(), bytes.size());
}

std::uint32_t Block::delay_since_last_sender_report() const noexcept {
  assert(valid());
  return read_u32_big_endian(&m_data[20]);
}

void Block::set_delay_since_last_sender_report(std::uint32_t delay) noexcept {
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