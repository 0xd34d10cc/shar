#include "packet_sender.hpp"

#include <type_traits>
#include <iostream>


namespace {
using Socket = shar::PacketSender::Socket;

template<typename T, std::size_t NBYTES = 4>
std::size_t send_little_endian(Socket& socket, T integer, boost::system::error_code& error_code) {
  static_assert(std::is_unsigned<T>::value);

  std::array<std::uint8_t, NBYTES> bytes;

  for (std::size_t i = 0; i < bytes.size(); ++i) {
    bytes[i] = static_cast<std::uint8_t>(integer & 0xff);
    integer >>= 8;
  }

  return socket.send(boost::asio::buffer(bytes.data(), bytes.size()),
                     0 /* message flags */, error_code);
}

bool send_packet(Socket& socket, shar::Packet& packet, boost::system::error_code& ec) {
//  std::cerr << "Sending packet of size " << packet.size() << std::endl;
  auto sent = send_little_endian<std::size_t, 4>(socket, packet.size(), ec);
  if (sent != 4 || ec) {
    return false;
  }

  // send content
  std::size_t bytes_sent = 0;
  while (bytes_sent != packet.size()) {
    bytes_sent += socket.send(
        boost::asio::buffer(packet.data() + bytes_sent,
                            packet.size() - bytes_sent),
        0 /* message flags */, ec
    );

    if (ec) {
      return false;
    }
  }

  return true;
}

}


namespace shar {

PacketSender::PacketSender(PacketsQueue& input, boost::asio::ip::address ip)
    : Sink("PacketSender", input), m_ip(ip)
    , m_metrics_timer(std::chrono::seconds(1))
    , m_bps(0)
    , m_clients()
    , m_context()
    , m_current_socket(m_context)
    , m_acceptor(m_context) // ???
{}

void PacketSender::process(Packet* packet) {
  // report metrics
  if (m_metrics_timer.expired()) {
//    std::cout << "bps: " << m_bps << std::endl;
    m_bps = 0;

    m_metrics_timer.restart();
  }

  // run acceptor for some time
  using namespace std::chrono_literals;
  m_context.run_for(5ms);

  m_bps += packet->size();

  for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
    const auto client_id = it->first;
    auto& client = it->second;

    boost::system::error_code ec;
    if (!send_packet(client, *packet, ec) || ec) {
      std::cerr << "Failed to send packet to Client #" << client_id
                << ": " << ec.message() << std::endl;
      it = m_clients.erase(it);
    }

  }
}

void PacketSender::start_accepting() {
  m_acceptor.async_accept(m_current_socket, [this](boost::system::error_code ec) {
    if (ec) {
      std::cerr << "Acceptor failed!" << std::endl;
      return;
    }

    std::size_t id = static_cast<std::size_t>(m_current_socket.native_handle());
    std::cout << "Client #" << id << ": connected" << std::endl;

    m_clients.emplace(id, std::move(m_current_socket));
    m_current_socket = Socket {m_context};

    // schedule another async_accept
    start_accepting();
  });
}

void PacketSender::setup() {
  namespace ip = boost::asio::ip;
  ip::tcp::endpoint endpoint {m_ip, 1337};
  m_acceptor.open(endpoint.protocol());
  m_acceptor.set_option(ip::tcp::acceptor::reuse_address(true));
  m_acceptor.bind(endpoint);
  m_acceptor.listen(100);
  start_accepting();
}

void PacketSender::teardown() {
  // disconnect all clients
}

} //  namespace shar