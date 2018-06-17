#pragma once

#include "disable_warnings_push.hpp"
#include <boost/asio.hpp>
#include "disable_warnings_pop.hpp"

#include "packet.hpp"
#include "processors/processor.hpp"
#include "queues/packets_queue.hpp"

namespace shar {


class PacketSender: public Processor {
public:
  PacketSender(PacketsQueue& input);
  PacketSender(const PacketSender&) = delete;
  PacketSender(PacketSender&&) = default;;
  ~PacketSender() = default;

  void run();

private:
  void send_packet(Packet&);

  PacketsQueue& m_packets;

  boost::asio::io_context      m_context;
  boost::asio::ip::tcp::socket m_socket;
};

} // namespace shar