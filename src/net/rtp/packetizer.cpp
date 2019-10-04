#include <cassert>   // assert
#include <algorithm> // min

#include "packetizer.hpp"

namespace shar::net::rtp {

static const u8 PACKET_TYPE_FU_A = 28;

Packetizer::Packetizer(u16 mtu) noexcept
  : m_mtu(mtu)
{}

bool Packetizer::valid() const noexcept {
  return m_data <= m_nal_start && m_nal_start <= m_data + m_size &&
         m_data <= m_nal_end && m_nal_end <= m_data + m_size &&
         m_nal_start <= m_nal_end &&
         m_nal_start <= m_position && m_position <= m_nal_end;
}

void Packetizer::set(u8* data, usize size) noexcept {
  m_data = data;
  m_size = size;

  m_nal_start = data;
  m_nal_end = data;
  m_position = data;
  m_last_packet_full_nal_unit = false;

  bool found = next_nal(); // move to start of NAL unit
  assert(found);
}

void Packetizer::reset() noexcept {
  set(nullptr, 0);
}

Fragment Packetizer::next() noexcept {
  assert(valid());

  if (m_last_packet_full_nal_unit) {
    auto start = m_nal_start;
    assert(start - m_data >= 2);
    m_last_packet_full_nal_unit = false;

    // remove start bit and set end bit
    *start = (*start & ~Fragment::START_FLAG_MASK) | Fragment::END_FLAG_MASK;
    return Fragment{start - 1, 2};
  }

  u8* start = next_fragment();
  if (start == nullptr) {
    return Fragment();
  }
  assert(start - m_data >= 2);

  // For first fragment, data looks like this:
  //
  // 0x00 0x00 0x00 0x01 0xff 0xff 0xff 0xff
  // |_________________|  |     ^ actual h264 data starts here
  //         |           nal unit header, 'start' and 'm_nal_start' point here
  //   nal unit prefix
  //
  // both NRI and 'nal type' are stored in nal unit header
  //
  //
  // for following fragments:
  //
  // 0x00 0x00 0x00 0xff 0xff 0xff ... 0xff 0xff 0xff 0xff 0xff 0xff
  // |  prefix bytes  |   |   |___________|  |
  //                 /    |         |       'start' points here
  //   was 0x01 byte,     |   previous fragments
  //   now indicator      |
  //                 'm_nal_start', header is stored here
  //
  // ...with that covered, it is up to reader to understand the
  //  reasons behind magic offsets in code below
  u8 first = start == m_nal_start ? 1 : 0;
  u8 nri = *(m_nal_start - 1 + first) & Fragment::NRI_MASK;
  u8 nal_type = *m_nal_start & Fragment::NAL_TYPE_MASK;

  u8 last = 0;

  u8* end = start - 2 + first + m_mtu;
  if (end >= m_nal_end) {
    end = m_nal_end;

    if (first == 1) {
      // from RFC6184: Start bit and End bit MUST NOT both be set
      //               to one in the same FU header
      // ... so we have to generate entire (though empty) packet just for end flag
      m_last_packet_full_nal_unit = true;
    } else {
      last = 1;
    }
  }

  u8 indicator = nri | PACKET_TYPE_FU_A;
  u8 header = static_cast<u8>(first << u8{7}) |
                        static_cast<u8>(last << u8{6}) |
                        nal_type;

  u8* fragment = start - 2 + first;
  fragment[0] = indicator;
  fragment[1] = header;

  m_position = end;
  return Fragment{fragment, static_cast<usize>(end - fragment)};
}

u8* Packetizer::next_fragment() noexcept {
  if (m_position == m_nal_end) {
    if (!next_nal()) {
      return nullptr;
    }
  }
  return m_position;
}

bool Packetizer::next_nal() noexcept {
  u8* data_end = m_data + m_size;
  if (m_nal_end == data_end) {
    return false;
  }

  u8* start = m_nal_end;

  // skip pattern
  while ((start < data_end) && *start == 0) {
    ++start;
  }

  if (start >= data_end) {
    return false;
  }

  ++start; // skip 0x01

  // find 0x00 0x00 0x01 pattern
  u8* end = start;
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

} // namespace shar::net::rtp