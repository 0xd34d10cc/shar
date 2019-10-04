#include "source_description.hpp"

namespace shar::net::rtcp {

bool SourceDescription::valid() const noexcept {
  return Header::valid() && Header::packet_type() == PacketType::SOURCE_DESCRIPTION;
}

SourceDescription::SourceDescription(u8* data, usize size)
  : Header(data, size)
  {}

SourceItems SourceDescription::items() noexcept {
  if (Header::packet_size() <= Header::MIN_SIZE + 4 /* for stream_id */) {
    // corner case, empty packet
    return SourceItems{};
  }

  return SourceItems{m_data + Header::MIN_SIZE, m_size - Header::MIN_SIZE};
}

}