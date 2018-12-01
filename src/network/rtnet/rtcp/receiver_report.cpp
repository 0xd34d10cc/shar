#include <cassert> // assert

#include "receiver_report.hpp"

namespace shar::rtcp {

ReceiverReport::ReceiverReport(std::uint8_t* data, std::size_t size) noexcept
  : Header(data, size)
  {}

bool ReceiverReport::valid() const noexcept {
  return Header::valid() && 
         m_size >= ReceiverReport::MIN_SIZE &&
         m_size == packet_size();
}

Block ReceiverReport::block(std::size_t index) noexcept {
  assert(valid());
  std::size_t offset = ReceiverReport::MIN_SIZE + Block::MIN_SIZE * index; 
  if (offset + Block::MIN_SIZE > m_size) {
    return Block{};
  }

  return Block{m_data + offset, m_size - offset};
}

}