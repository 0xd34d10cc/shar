#pragma once

#include <array>
#include <cstdint>

#include "disable_warnings_push.hpp"
#include <boost/asio.hpp>
#include "disable_warnings_pop.hpp"

#include "processors/processor.hpp"
#include "queues/packets_queue.hpp"


namespace shar {

using IpAddress = std::array<std::uint8_t, 4>;

class PacketReceiver : public Processor {
public:
  PacketReceiver(IpAddress server, PacketsQueue& output);


  void run();

private:
  PacketsQueue& m_packets;

  boost::asio::ip::address     m_server_address;
  boost::asio::io_context      m_context;
  boost::asio::ip::tcp::socket m_receiver;
};

}