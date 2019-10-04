#pragma once

#include <cstdint>
#include <utility>

#include "fragment.hpp"


namespace shar::net::rtp {

// H264 packetizer
class Packetizer {
public:
  Packetizer() noexcept = default;
  Packetizer(u16 mtu) noexcept;
  Packetizer(const Packetizer&) noexcept = default;
  Packetizer(Packetizer&&) noexcept = default;
  Packetizer& operator=(const Packetizer&) noexcept = default;
  Packetizer& operator=(Packetizer&&) noexcept = default;
  ~Packetizer() = default;

  void set(u8* data, usize size) noexcept;
  void reset() noexcept;

  // return next chunk of data
  Fragment next() noexcept;

private:
  bool valid() const noexcept;

  // returns nullptr if there are no more fragments
  u8* next_fragment() noexcept;

  // returns true on success
  bool next_nal() noexcept;

  u16 m_mtu{1100};

  u8* m_data{nullptr};
  usize   m_size{0};

  // current NAL unit in |m_data|
  u8* m_nal_start{nullptr};
  u8* m_nal_end{nullptr};

  // positon of current fragment inside current NAL unit
  u8* m_position{nullptr};

  // true if size of last packet from |m_data| was less than mtu
  // in this case we should send empty packet with E flag
  bool m_last_packet_full_nal_unit{false};
};

} // namespace shar::net::rtp