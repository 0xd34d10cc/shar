#pragma once

#include <vector>
#include <cstdint>
#include <cstdlib>

#include "network/packet.hpp"


namespace shar {

using Buffer = std::vector<std::uint8_t>;

class PacketParser {
public:
  PacketParser();

  std::vector<Packet> update(const Buffer& buffer, std::size_t size);

private:
  enum class State {
    ReadingLength,
    ReadingContent
  };

  State       m_state;
  std::size_t m_packet_size;
  std::size_t m_remaining;
  Buffer      m_buffer;
};

}