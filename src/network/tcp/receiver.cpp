#include "receiver.hpp"
#include "time.hpp"


namespace shar::tcp {

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

  setup();

  while (!m_running.expired() && sender.connected()) {
    m_context.run_for(Milliseconds(250));
  }

  shutdown();
  teardown();

  m_sender = nullptr;
}

void PacketReceiver::shutdown() {
  m_running.cancel();
}

void PacketReceiver::process() {
  m_context.run_for(std::chrono::milliseconds(250));
}

void PacketReceiver::setup() {
  using Endpoint = asio::ip::tcp::endpoint;
  Endpoint endpoint {m_server_address, m_port};
  m_receiver.connect(endpoint);

  start_read();
}

void PacketReceiver::teardown() {
  // close the connection
  m_receiver.shutdown(Socket::shutdown_both);
  m_receiver.close();
}


void PacketReceiver::start_read() {
  m_receiver.async_read_some(
      asio::buffer(m_buffer.data(), m_buffer.size()),
      [this](const ErrorCode& ec, std::size_t received) {
        if (ec) {
          m_logger.error("Receiver failed: {}", ec.message());
          shutdown();
          return;
        }

        auto packets = m_reader.update(m_buffer, received);

        m_bytes_received.increment(static_cast<double>(received));
        m_packets_received.increment(static_cast<double>(packets.size()));
        for (auto& packet: packets) {
          m_sender->send(std::move(packet));
        }

        start_read();
      }
  );
}

}