#include <algorithm>

#include "packet_parser.hpp"


namespace shar::net::tcp {


// The packet format is pretty simple: [content_length] [content]
// where [content_length] is 4-byte integer in little endian.
// [content] is just array of bytes
PacketParser::PacketParser()
    : m_state(State::ReadingLength)
    , m_packet_size(0)
    , m_remaining(4) // we need to read 4 more bytes to get length of first packet
    , m_buffer(4096, 0) {}

std::vector<Unit> PacketParser::update(const Buffer& buffer, usize size) {
  std::vector<Unit> packets;
  usize       bytes_read = 0;

  while (bytes_read != size) {
    usize bytes_to_read = std::min(size - bytes_read, m_remaining);
    switch (m_state) {
      case State::ReadingLength:
        for (usize i = 0; i < bytes_to_read; ++i) {
          usize offset = 8 * (4 - m_remaining);
          m_packet_size |= static_cast<usize>(buffer[bytes_read] << offset);
          ++bytes_read;
          --m_remaining;
        }

        // we read packet size
        if (m_remaining == 0) {
          m_remaining = m_packet_size;
          m_state     = State::ReadingContent;
        }
        break;

      case State::ReadingContent:
        usize already_read = m_packet_size - m_remaining;
        if (already_read + bytes_to_read >= m_buffer.size()) {
          m_buffer.resize(already_read + bytes_to_read, 0);
        }
        std::copy_n(buffer.data() + bytes_read, bytes_to_read,
                    m_buffer.data() + (m_packet_size - m_remaining));
        m_remaining -= bytes_to_read;
        bytes_read += bytes_to_read;

        // we read packet content
        if (m_remaining == 0) {
          packets.emplace_back(Unit::from_data(m_buffer.data(), m_packet_size));

          m_packet_size = 0;
          m_remaining   = 4;
          m_state       = State::ReadingLength;
        }

        break;
    }
  }

  return packets;
}

}