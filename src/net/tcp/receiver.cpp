#include "receiver.hpp"
#include "time.hpp"


namespace shar::net::tcp {

PacketReceiver::PacketReceiver(Context context, IpAddress server, Port port)
    : Context(std::move(context))
    , m_reader()
    , m_buffer(4096, 0)
    , m_server_address(server)
    , m_port(port)
    , m_context()
    , m_receiver(m_context)
    , m_packets_received()
    , m_bytes_received() {}

void PacketReceiver::run(Sender<codec::ffmpeg::Unit> sender) {
  m_sender = &sender; // TODO: remove this hack
  m_bytes_received = m_metrics->add("Bytes received", Metrics::Format::Bytes);
  m_packets_received = m_metrics->add("Packets received", Metrics::Format::Count);
  Endpoint endpoint{ m_server_address, m_port };
  // FIXME: throws exception
  m_receiver.connect(endpoint);

  start_read();

  while (!m_running.expired() && sender.connected()) {
    m_context.run_for(Milliseconds(250));
  }

  shutdown();

  // close the connection
  m_receiver.shutdown(Socket::shutdown_both);
  m_receiver.close();

  m_sender = nullptr;
}

void PacketReceiver::shutdown() {
  m_running.cancel();
}

void PacketReceiver::start_read() {
  m_receiver.async_read_some(
      span(m_buffer.data(), m_buffer.size()),
      [this](const ErrorCode& ec, std::size_t received) {
        if (ec) {
          m_logger.error("Receiver failed: {}", ec.message());
          shutdown();
          return;
        }

        auto packets = m_reader.update(m_buffer, received);

        m_metrics->increase(m_bytes_received, received);
        m_metrics->increase(m_packets_received, packets.size());
        for (auto& packet: packets) {
          m_sender->send(std::move(packet));
        }

        start_read();
      }
  );
}

}