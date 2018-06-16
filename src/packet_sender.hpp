#pragma once

#include "disable_warnings_push.hpp"
#include <boost/asio.hpp>
#include "disable_warnings_pop.hpp"

#include "queue.hpp"
#include "packet.hpp"


namespace shar {

class PacketSender {
public:
  PacketSender();
  PacketSender(const PacketSender&) = delete;
  PacketSender(PacketSender&&) = default;;
  ~PacketSender() = default;

  void send(Packet packet);
  void run();
  void stop();
  bool is_running() const;

private:
  void send_packet(Packet&);

  std::atomic<bool>           m_running;
  FixedSizeQueue<Packet, 256> m_packets;

  boost::asio::io_context      m_context;
  boost::asio::ip::tcp::socket m_socket;
};

} // namespace shar