#include <cassert> // assert
#include <cstring> // memcpy

#include "receiver_report.hpp"
#include "byteorder.hpp"


namespace shar::net::rtcp {

ReceiverReport::ReceiverReport(u8* data, usize size) noexcept
  : Header(data, size)
  {}

bool ReceiverReport::valid() const noexcept {
  return Header::valid() &&
         m_size >= ReceiverReport::MIN_SIZE &&
         m_size == packet_size() &&
         packet_type() == PacketType::RECEIVER_REPORT;
}

u32 ReceiverReport::stream_id() const noexcept {
  assert(valid());
  return read_u32_big_endian(&m_data[4]);
}

void ReceiverReport::set_stream_id(u32 stream_id) noexcept {
  assert(valid());
  const auto bytes = to_big_endian(stream_id);
  std::memcpy(&m_data[4], bytes.data(), bytes.size());
}

Block ReceiverReport::block(usize index) noexcept {
  assert(valid());
  usize offset = ReceiverReport::MIN_SIZE + Block::MIN_SIZE * index;
  if (offset + Block::MIN_SIZE > m_size) {
    return Block{};
  }

  return Block{m_data + offset, m_size - offset};
}

}