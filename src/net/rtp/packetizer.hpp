#pragma once

#include <cstdint>
#include <utility>

#include "fragment.hpp"


namespace shar::net::rtp {

// H264 packetizer
class Packetizer {
public:
  Packetizer() noexcept = default;
  Packetizer(std::uint16_t mtu) noexcept;
  Packetizer(const Packetizer&) noexcept = default;
  Packetizer(Packetizer&&) noexcept = default;
  Packetizer& operator=(const Packetizer&) noexcept = default;
  Packetizer& operator=(Packetizer&&) noexcept = default;
  ~Packetizer() = default;

  void set(std::uint8_t* data, std::size_t size) noexcept;
  void reset() noexcept;

  // return next chunk of data
  Fragment next() noexcept;

private:
  bool valid() const noexcept;

  // returns nullptr if there are no more fragments
  std::uint8_t* next_fragment() noexcept;

  // returns true on success
  bool next_nal() noexcept;

  std::uint16_t m_mtu{1100};

  std::uint8_t* m_data{nullptr};
  std::size_t   m_size{0};

  // current NAL unit in |m_data|
  std::uint8_t* m_nal_start{nullptr};
  std::uint8_t* m_nal_end{nullptr};

  // positon of current fragment inside current NAL unit
  std::uint8_t* m_position{nullptr};

  // true if size of last packet from |m_data| was less than mtu
  // in this case we should send empty packet with E flag
  bool m_last_packet_full_nal_unit{false};
};

} // namespace shar::net::rtp