#include "sender.hpp"
#include "net/rtp/packet.hpp"


namespace shar::net::rtp {

static const u16 MTU = 1000;

PacketSender::PacketSender(Context context, IpAddress ip, Port port)
    : Context(std::move(context))
    , m_endpoint(std::move(ip), port)
    , m_context()
    , m_socket(m_context)
    , m_packetizer(MTU)
    , m_sequence(0)
    , m_bytes_sent(0)
    , m_fragments_sent(0)
    {}

void PacketSender::run(Receiver<Unit> packets) {
    m_socket.open(udp::v4());

    // TODO: use port range instead of constant
    auto ip = IpAddress(IPv4({ 0, 0, 0, 0 }));
    auto endpoint = udp::Endpoint(ip, Port{44444});
    m_socket.bind(endpoint);

    auto sent = Metric(m_metrics, "bytes sent", Metrics::Format::Bytes);
    while (auto packet = packets.receive()) {
      if (m_running.expired()) {
        break;
      }

      sent += packet->size();
      set_packet(std::move(*packet));
      send();
    }

    shutdown();
    m_socket.close();
}

void PacketSender::shutdown() {
    m_running.cancel();
}

void PacketSender::set_packet(Unit packet) {
    m_current_packet = std::move(packet);
    m_packetizer.set(m_current_packet.data(), m_current_packet.size());

    m_bytes_sent = 0;
}

void PacketSender::send() {
  static const usize HEADER_SIZE = rtp::Packet::MIN_SIZE;

  std::array<u8, MTU + HEADER_SIZE> buffer;

  while (auto fragment = m_packetizer.next()) {
    // sleep for 1ms every 128 fragments
    if ((++m_fragments_sent & (128 - 1)) == 0) {
      std::this_thread::sleep_for(Milliseconds(1));
    }

    assert(buffer.size() >= fragment.size() + HEADER_SIZE);

    // setup packet
    std::memset(buffer.data(), 0, HEADER_SIZE);
    std::memcpy(buffer.data() + HEADER_SIZE, fragment.data(), fragment.size());

    rtp::Packet packet(buffer.data(), HEADER_SIZE + fragment.size());
    packet.set_version(2);
    packet.set_has_padding(false);
    packet.set_has_extensions(false);
    packet.set_contributors_count(0);
    packet.set_marked(false);
    packet.set_payload_type(96);
    packet.set_sequence(m_sequence++);
    packet.set_timestamp(m_current_packet.timestamp());
    packet.set_stream_id(0);

    ErrorCode ec;
    m_socket.send_to(span(packet), m_endpoint, 0, ec);

    if (ec) {
      m_logger.error("Failed to send rtp packet: {}", ec.message());
    }

  }
}

}