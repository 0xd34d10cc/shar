#include "packet_sender.hpp"

#include <iostream>

namespace shar {

PacketSender::PacketSender()
  : m_running(false) 
  , m_packets()
  , m_context()
  , m_socket(m_context) // ???
{}

void PacketSender::send(Packet packet) {
  m_packets.push(std::move(packet));
}

void PacketSender::run() {
  auto localhost = boost::asio::ip::address_v4{{127, 0, 0, 1}};
  boost::asio::ip::address address{localhost};
  boost::asio::ip::tcp::endpoint endpoint{address, 1337};
  m_socket.connect(endpoint);

  m_running = true;
  while (m_running) {
    if (m_packets.empty()) {
      m_packets.wait();
    }

    assert(!m_packets.empty());
    std::size_t size = m_packets.size();
    for (std::size_t i = 0; i < size; ++i) {
      Packet* packet = m_packets.get(i);
      send_packet(*packet);
    }
    m_packets.consume(size);
  }
}

void PacketSender::stop() {
  m_running = false;
}

bool PacketSender::is_running() const {
  return m_running;
}

void PacketSender::send_packet(Packet& packet) {
  boost::system::error_code ec;
  m_socket.send(boost::asio::buffer(packet.data(), packet.size()), 0, ec);
  if (ec) {
    std::cerr << "network[" << ec << "]: " << ec.message() << std::endl;
  }
}

} //  namespace shar