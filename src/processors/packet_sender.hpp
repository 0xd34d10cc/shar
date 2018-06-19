#pragma once

#include <unordered_map>

#include "disable_warnings_push.hpp"
#include <boost/asio.hpp>
#include "disable_warnings_pop.hpp"

#include "primitives/timer.hpp"
#include "packet.hpp"
#include "processors/processor.hpp"
#include "queues/packets_queue.hpp"


namespace shar {

class PacketSender : public Processor {
public:
  PacketSender(PacketsQueue& input);
  PacketSender(const PacketSender&) = delete;
  PacketSender(PacketSender&&) = default;
  ~PacketSender() = default;

  void run();

private:
  void start_accepting();

  using Socket = boost::asio::ip::tcp::socket;
  using Context = boost::asio::io_context;
  using Acceptor = boost::asio::ip::tcp::acceptor;
  using Clients = std::unordered_map<std::size_t /* fd */, Socket>;

  Timer m_metrics_timer;
  std::size_t m_bps;

  PacketsQueue& m_packets;
  Clients  m_clients;
  Context  m_context;
  Socket   m_current_socket;
  Acceptor m_acceptor;
};

} // namespace shar