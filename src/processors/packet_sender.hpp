#pragma once

#include <unordered_map>

#include "disable_warnings_push.hpp"
#include <boost/asio.hpp>
#include "disable_warnings_pop.hpp"

#include "primitives/timer.hpp"
#include "packet.hpp"
#include "processors/sink.hpp"
#include "queues/packets_queue.hpp"


namespace shar {

class PacketSender : public Sink<PacketSender, PacketsQueue> {
public:
  PacketSender(PacketsQueue& input, boost::asio::ip::address ip);
  PacketSender(const PacketSender&) = delete;
  PacketSender(PacketSender&&) = default;
  ~PacketSender() = default;

  void setup();
  void process(Packet* packet);
  void teardown();


  using Socket = boost::asio::ip::tcp::socket;
  using Context = boost::asio::io_context;
  using Acceptor = boost::asio::ip::tcp::acceptor;
  using Clients = std::unordered_map<std::size_t /* fd */, Socket>;

private:
  void start_accepting();

  Timer                    m_metrics_timer;
  std::size_t              m_bps;
  boost::asio::ip::address m_ip;

  Clients  m_clients;
  Context  m_context;
  Socket   m_current_socket;
  Acceptor m_acceptor;
};

} // namespace shar