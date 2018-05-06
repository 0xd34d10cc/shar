#pragma once

#include <boost/asio.hpp>

#include "queue.hpp"
#include "packet.hpp"


namespace shar {

class PacketSender {
public:
  PacketSender();
  ~PacketSender() = default;

  void send(Packet packet);
  void run();
  void stop();
  bool is_running() const;

private:
  void send_packet(Packet&);

  std::atomic<bool> m_running;
  FixedSizeQueue<Packet, 256> m_packets;

  boost::asio::io_context m_context;
  boost::asio::ip::tcp::socket m_socket;
};

} // namespace shar