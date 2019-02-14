#include <cassert>   // assert
#include <algorithm> // min

#include "packetizer.hpp"

namespace shar {

static const std::uint8_t PACKET_TYPE_FU_A = 28;

Packetizer::Packetizer(std::uint16_t mtu) noexcept
  : m_mtu(mtu)
{}

bool Packetizer::valid() const noexcept {
  return m_data <= m_nal_start && m_nal_start <= m_data + m_size &&
         m_data <= m_nal_end && m_nal_end <= m_data + m_size &&
         m_nal_start <= m_nal_end &&
         m_nal_start <= m_position && m_position <= m_nal_end;  
}

void Packetizer::set(std::uint8_t* data, std::size_t size) noexcept {
  m_data = data;
  m_size = size;

  m_nal_start = data;
  m_nal_end = data;
  m_position = data;
  m_last_packet_full_nal_unit = false;

  next_nal(); // move to start of NAL unit
}

void Packetizer::reset() noexcept {
  set(nullptr, 0);
}

Fragment Packetizer::next() noexcept {
  assert(valid());

  if (m_last_packet_full_nal_unit) {
    auto start = m_nal_start;
    m_last_packet_full_nal_unit = false;
    
    // remove start bit and set end bit
    *start = (*start & ~Fragment::START_FLAG_MASK) | Fragment::END_FLAG_MASK;
    return Fragment{start - 1, 2};
  }

  auto [start, end] = next_fragment();
  if (start == nullptr) {
    return Fragment();
  }

  std::uint8_t first = start == m_nal_start ? 1 : 0;
  std::uint8_t nri = *(m_nal_start - 1 + first) & Fragment::NRI_MASK;
  std::uint8_t nal_type = *m_nal_start & Fragment::NAL_TYPE_MASK;

  std::uint8_t last = 0;
  if (end == m_nal_end) {
    if (first == 1) {
      // from RFC6184: Start bit and End bit MUST NOT both be set
			//               to one in the same FU header
			// ... so we have to generate entire (though empty) packet just for end flag
      m_last_packet_full_nal_unit = true;
    } else {
      last = 1;
    }
  }

  std::uint8_t indicator = nri | PACKET_TYPE_FU_A;
  std::uint8_t header = static_cast<std::uint8_t>(first << std::uint8_t{7}) | 
                        static_cast<std::uint8_t>(last << std::uint8_t{6}) | 
                        nal_type;

  std::uint8_t* fragment = start - 2 + first;
  fragment[0] = indicator;
  fragment[1] = header;

  return Fragment{fragment, static_cast<std::size_t>(end - fragment)};
}

std::pair<std::uint8_t*, std::uint8_t*> Packetizer::next_fragment() noexcept {
  if (m_position == m_nal_end) {
    if (!next_nal()) {
      return {nullptr, nullptr};
    }
  }

  std::uint8_t* start = m_position;
  std::uint8_t* end = start + m_mtu + 1; // +1 for header
	
  if (end > m_nal_end) {
		end = m_nal_end;
	}

	m_position = end;
	return {start, end};
}

bool Packetizer::next_nal() noexcept {
  std::uint8_t* data_end = m_data + m_size;
  if (m_nal_end == data_end) {
    return false;
  }

  std::uint8_t* start = m_nal_end;

  // skip pattern
  while ((start < data_end) && *start == 0) {
    ++start;
  } 
  
  if (start >= data_end) {
    return false;
  }

  ++start; // skip 0x01

  // find 0x00 0x00 0x01 pattern
  std::uint8_t* end = start;
  while (end + 2 < data_end && (end[0] != 0 || end[1] != 0 || end[2] != 1)) {
    ++end;
  }

  if (end + 2 < data_end) {
    // or was it 0x00 0x00 0x00 0x01 ?
    m_nal_end = end[-1] == 0 ? end - 1 : end;
  } else {
    // it is last nal unit
    m_nal_end = data_end;
  }

  m_nal_start = start;
  m_position = m_nal_start;

  return true;
}

}