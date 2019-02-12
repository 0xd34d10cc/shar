#include "disable_warnings_push.hpp"
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include "disable_warnings_pop.hpp"

#include "context.hpp"
#include "network.hpp"
#include "consts.hpp"
#include "rtnet/rtp/packet.hpp"


namespace shar {

Network::Network(Context context, IpAddress ip, Port port)
    : Context(std::move(context))
    , m_running(false)
    , m_endpoint(std::move(ip), port)
    , m_context()
    , m_socket(m_context)
    , m_packetizer()
    , m_sequence(0)
    , m_bytes_sent(0) 
    {}

void Network::run(Receiver<Packet> packets) {
    m_running = true;

    if (!m_socket.is_open()) {
      m_socket.open(boost::asio::ip::udp::v4());
    }

    while (auto packet = packets.receive()) {

      if (!m_running) {
        break;
      }

      set_packet(std::move(*packet));
      m_context.reset();

      send();
      m_context.run();
      std::this_thread::sleep_for(std::chrono::microseconds(50));
    }

    shutdown();
    m_socket.close();
}

void Network::shutdown() {
    m_running = false;
}

void Network::set_packet(Packet packet) {
    m_current_packet = std::move(packet);
    m_packetizer.set(m_current_packet.data(),m_current_packet.size());

    m_bytes_sent = 0;
}

void Network::send() {

  std::array<std::uint8_t, 1200> buffer;
  while (auto fragment = m_packetizer.next()) {

    std::memcpy(buffer.data() + 16, fragment.data(), fragment.size());
    rtp::Packet packet(buffer.data(), fragment.size() + 16);
    packet.set_version(2);
    packet.set_has_padding(false);
    packet.set_has_extensions(false);
    packet.set_contributors_count(1);
    packet.set_marked(false);
    packet.set_payload_type(96);
    packet.set_sequence(m_sequence++);
    packet.set_timestamp(m_current_packet.timestamp());
    packet.set_stream_id(0);
    packet.contributors()[0] = 1;

    ErrorCode ec;
    m_socket.send_to(boost::asio::buffer(packet.data(), packet.size()), m_endpoint, 0, ec);
    if (ec) {
      m_logger.error("Something happened when we sent packet: {}", ec.message());
    }

  }

}

}