#include "processors/packet_receiver.hpp"
#include "network/consts.hpp"


namespace shar {

PacketReceiver::PacketReceiver(Context context, IpAddress server, std::uint16_t port, Sender<Packet> output)
    : Source(std::move(context), std::move(output))
    , m_reader()
    , m_buffer(4096, 0)
    , m_server_address(server)
    , m_port(port)
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
  Endpoint endpoint {m_server_address, m_port};
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