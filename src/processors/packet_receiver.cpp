#include "processors/packet_receiver.hpp"


namespace shar {

// The packet format is pretty simple: [content_length] [content]
// where [content_length] is 4-byte integer in little endian.
// [content] is just array of bytes
PacketReceiver::PacketReader::PacketReader()
    : m_state(State::ReadingLength)
    , m_packet_size(0)
    , m_remaining(4) // we need to read 4 more bytes to get length of first packet
    , m_buffer(4096, 0) {}

std::vector<shar::Packet> PacketReceiver::PacketReader::update(const Buffer& buffer, std::size_t size) {
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
        if (already_read + bytes_to_read >= m_buffer.size()) {
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

PacketReceiver::PacketReceiver(IpAddress server, Logger logger, MetricsPtr metrics, PacketsSender output)
    : Source("PacketReceiver", std::move(logger), std::move(metrics), std::move(output))
    , m_reader()
    , m_buffer(4096, 0)
    , m_server_address(server)
    , m_context()
    , m_receiver(m_context)
    , m_packets_received_metric()
    , m_bytes_received_metric() {}

void PacketReceiver::process(FalseInput) {
  m_context.run_for(std::chrono::milliseconds(250));
}

void PacketReceiver::setup() {
  m_packets_received_metric = m_metrics->add("PacketReceiver\tpackets", Metrics::Format::Count);
  m_bytes_received_metric   = m_metrics->add("PacketReceiver\tbytes", Metrics::Format::Bytes);

  using Endpoint = boost::asio::ip::tcp::endpoint;
  Endpoint endpoint {m_server_address, 1337};
  m_receiver.connect(endpoint);

  start_read();
}

void PacketReceiver::teardown() {
  // close the connection
  m_receiver.shutdown(boost::asio::socket_base::shutdown_both);
  m_receiver.close();

  m_metrics->remove(m_packets_received_metric);
  m_metrics->remove(m_bytes_received_metric);
}


void PacketReceiver::start_read() {
  m_receiver.async_read_some(
      boost::asio::buffer(m_buffer.data(), m_buffer.size()),
      [this](const boost::system::error_code& ec, std::size_t received) {
        if (ec) {
          m_logger.error("Receiver failed: {}", ec.message());
          Processor::stop();
          return;
        }

        m_metrics->increase(m_bytes_received_metric, received);

        auto packets = m_reader.update(m_buffer, received);
        m_metrics->increase(m_packets_received_metric, packets.size());
        for (auto& packet: packets) {
          output().send(std::move(packet));
        }

        start_read();
      }
  );
}

}