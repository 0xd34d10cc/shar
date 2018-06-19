#include "packet_sender.hpp"

#include <iostream>


namespace shar {

PacketSender::PacketSender(PacketsQueue& input)
    : Processor("PacketSender")
    , m_metrics_timer(std::chrono::seconds(1))
    , m_bps(0)
    , m_packets(input)
    , m_clients()
    , m_context()
    , m_current_socket(m_context)
    , m_acceptor(m_context) // ???
{}

void PacketSender::run() {
  namespace ip = boost::asio::ip;
  ip::address_v4    localhost {{127, 0, 0, 1}};
//  ip::address_v4    any_interface {{0, 0, 0, 0}};
  ip::address       address {localhost};
  ip::tcp::endpoint endpoint {address, 1337};
  m_acceptor.open(endpoint.protocol());
  m_acceptor.set_option(ip::tcp::acceptor::reuse_address(true));
  m_acceptor.bind(endpoint);
  m_acceptor.listen(100);
  start_accepting();

  Processor::start();
  while (is_running()) {
    // report metrics
    if (m_metrics_timer.expired()) {
      std::cout << "bps: " << m_bps << std::endl;
      m_bps = 0;

      m_metrics_timer.restart();
    }

    // run acceptor for some time
    using namespace std::chrono_literals;
    m_context.run_for(5ms);

    if (!m_packets.empty()) {
      do {
        auto* packet = m_packets.get_next();
        m_bps += packet->size();

        for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
          const auto client_id = it->first;
          auto& client                   = it->second;

          // send size
          std::array<std::uint8_t, 4> size {
              static_cast<std::uint8_t>((packet->size() >> 0) & 0xff),
              static_cast<std::uint8_t>((packet->size() >> 8) & 0xff),
              static_cast<std::uint8_t>((packet->size() >> 16) & 0xff),
              static_cast<std::uint8_t>((packet->size() >> 24) & 0xff)
          };

          boost::system::error_code ec;
          std::size_t               sent = client.send(
              boost::asio::buffer(size.data(), size.size()),
              0 /* message flags */, ec
          );
          // FIXME: can actually send less than 4 bytes
          if (sent != size.size() || ec) {
            std::cerr << "Client #" << client_id << ": Failed to send packet size";
          }
          // NOTE: error will be handled below

          // send content
          std::size_t bytes_sent = 0;
          while (bytes_sent != packet->size()) {
            bytes_sent += client.send(
                boost::asio::buffer(packet->data() + bytes_sent,
                                    packet->size() - bytes_sent),
                0 /* message flags */, ec
            );

            if (ec) {
              std::cerr << "Client #" << client_id
                        << " [" << ec << "]: " << ec.message() << std::endl;
              client.shutdown(boost::asio::socket_base::shutdown_both);
              client.close();
              it = m_clients.erase(it);
              break; // go to next client
            }
          }
        }

        m_packets.consume(1);
      } while (!m_packets.empty());
    }

    m_packets.wait();
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

} //  namespace shar