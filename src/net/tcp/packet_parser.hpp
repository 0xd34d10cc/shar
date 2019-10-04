#pragma once

#include <vector>

#include "int.hpp"
#include "codec/ffmpeg/unit.hpp"


namespace shar::net::tcp {

using codec::ffmpeg::Unit;
using Buffer = std::vector<u8>;

class PacketParser {
public:
  PacketParser();
  std::vector<Unit> update(const Buffer& buffer, usize size);

private:
  enum class State {
    ReadingLength,
    ReadingContent
  };

  State m_state;
  usize m_packet_size;
  usize m_remaining;
  Buffer m_buffer;
};

}