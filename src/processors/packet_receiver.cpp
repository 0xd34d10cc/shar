#include <iostream>

#include "processors/packet_receiver.hpp"


using Buffer = std::vector<std::uint8_t>;

namespace {


// The packet format is pretty simple: [content_length] [content]
// where [content_length] is 4-byte integer in little endian.
// [content] is just array of bytes
struct PacketReader {
  enum class State {
    ReadingLength,
    ReadingContent
  };

  PacketReader()
      : m_state(State::ReadingLength)
      , m_packet_size(0)
      , m_remaining(4) // we need to read 4 more bytes to get length of first packet
      , m_buffer(4096, 0) {}

  std::vector<shar::Packet> update(const Buffer& buffer, std::size_t size) {
    std::vector<shar::Packet> packets;
    std::size_t               bytes_read = 0;

    while (bytes_read != size) {
      std::size_t bytes_to_read = std::min(size - bytes_read, m_remaining);
      switch (m_state) {
        case State::ReadingLength:
          for (std::size_t i = 0; i < bytes_to_read; ++i) {
            std::size_t offset = 8 * (4 - m_remaining);
            m_packet_size |= static_cast<std::size_t>(buffer[bytes_read] << offset);
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
          std::size_t already_read = m_packet_size - m_remaining;
          if (already_read + bytes_to_read > m_buffer.size()) {
            m_buffer.resize(already_read + bytes_to_read, 0);
          }
          std::copy_n(buffer.data() + bytes_read, bytes_to_read,
                      m_buffer.data() + (m_packet_size - m_remaining));
          m_remaining -= bytes_to_read;
          bytes_read += bytes_to_read;

          // we read packet content
          if (m_remaining == 0) {
            // sadly, std::vector don't have release() function so we have to do extra memcpy there
            auto packet = std::make_unique<std::uint8_t[]>(m_packet_size);
            std::copy_n(m_buffer.data(), m_packet_size, packet.get());
            packets.emplace_back(std::move(packet), m_packet_size);

            m_packet_size = 0;
            m_remaining   = 4;
            m_state       = State::ReadingLength;
          }

          break;
      }
    }

    return packets;
  }

  State       m_state;
  std::size_t m_packet_size;
  std::size_t m_remaining;
  Buffer      m_buffer;
};


}


namespace shar {

PacketReceiver::PacketReceiver(IpAddress server, PacketsQueue& output)
    : Processor("PacketReceiver")
    , m_packets(output)
    , m_server_address(boost::asio::ip::address_v4 {{server}})
    , m_context()
    , m_receiver(m_context) {}

void PacketReceiver::run() {
  using Endpoint = boost::asio::ip::tcp::endpoint;

  PacketReader reader;
  Buffer       buffer(4096, 0);
  Endpoint     endpoint {m_server_address, 1337};
  m_receiver.connect(endpoint);

  Processor::start();

  boost::system::error_code ec;
  while (is_running()) {
    std::size_t bytes_received = m_receiver.receive(
        boost::asio::buffer(buffer.data(), buffer.size()),
        0 /* message flags */, ec
    );

    if (ec) {
      std::cerr << "PacketReceiver[" << ec << "]: " << ec.message() << std::endl;
      // should we reconnect there or something?
      ec.clear();
      continue;
    }

    auto packets = reader.update(buffer, bytes_received);
    for (auto& packet: packets) {
      m_packets.push(std::move(packet));
    }

    ec.clear();
  }

  // close the connection
  m_receiver.shutdown(boost::asio::socket_base::shutdown_both);
  m_receiver.close();
}

}