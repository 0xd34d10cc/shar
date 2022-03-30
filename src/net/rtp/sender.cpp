#include "sender.hpp"

#include "packet.hpp"
#include "time.hpp"


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
    , m_client(m_context)
    {}

void PacketSender::connect() {
  // 1. Connect to shar server
  // 2. Create session
  // 3. Wait for connection notification
  // 4. ICE
  //
  // set ip = client ip
  // and monitor connectivity
  m_client.connect(
    IpAddress::from_string("127.0.0.1"), 1337,
    [](ErrorCode ec) {
      if (!ec) {
        LOG_INFO("Successfully connected to ICE server");
      }
      else {
        LOG_ERROR("Failed to connect to ICE server: {}", ec.message());
      }

      // TODO: push continuation task on m_context
    }
  );

  m_context.run_for(Seconds(3));
  if (!m_client.connected()) {
    // TODO: retry
    // TODO: notify GUI thread about connection issues
    throw std::runtime_error("Connection to ICE server timed out");
  }

  bool created = false;
  m_client.open_session(
    "kekus",
    [&created](ice::SessionID session) {
      LOG_INFO("Session created. ID: {}", session);
      created = true;
    },
    [](ice::ConnectionInfo info) {
      LOG_INFO("Client connection notification received: {}", info);

      // FIXME: gather candidates and serialize
      return ice::ConnectionInfo{};
    }
    );
  m_context.run_for(Seconds(3));

  if (!created) {
    throw std::runtime_error("CreateSession request timed out");
  }
}

void PacketSender::run(Receiver<Unit> packets) {
  connect();

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
      LOG_ERROR("Failed to send rtp packet: {}", ec.message());
    }

  }
}

}