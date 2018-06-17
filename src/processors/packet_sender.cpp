#include "packet_sender.hpp"

#include <iostream>


namespace shar {

PacketSender::PacketSender(PacketsQueue& packets)
    : m_packets(packets)
    , m_context()
    , m_socket(m_context) // ???
{}

void PacketSender::run() {
  namespace ip = boost::asio::ip;

  ip::address_v4    localhost {{127, 0, 0, 1}};
  ip::address       address {localhost};
  ip::tcp::endpoint endpoint {address, 1337};
  m_socket.connect(endpoint);

  Processor::start();

  while (is_running()) {
    if (m_packets.empty()) {
      m_packets.wait();
    }

    std::size_t size = m_packets.size();
    assert(!m_packets.empty());
    for (std::size_t i = 0; i < size; ++i) {
      Packet* packet = m_packets.get(i);
      send_packet(*packet);
    }
    m_packets.consume(size);
  }
}

void PacketSender::send_packet(Packet& packet) {
  boost::system::error_code ec;
  m_socket.send(boost::asio::buffer(packet.data(), packet.size()), 0, ec);
  if (ec) {
    std::cerr << "network[" << ec << "]: " << ec.message() << std::endl;
  }
}

} //  namespace shar