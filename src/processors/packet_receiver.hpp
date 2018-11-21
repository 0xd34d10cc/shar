#pragma once

#include <array>
#include <cstdint>

#include "disable_warnings_push.hpp"
#include <boost/asio.hpp>
#include "disable_warnings_pop.hpp"

#include "network/packet.hpp"
#include "network/packet_parser.hpp"
#include "processors/source.hpp"
#include "channels/bounded.hpp"


namespace shar {

using IpAddress = boost::asio::ip::address;
using Port      = const std::uint16_t;
class PacketReceiver : public Source<PacketReceiver, Sender<Packet>> {
public:
  using Base = Source<PacketReceiver, Sender<Packet>>;
  using Context = typename Base::Context;

  PacketReceiver(Context context, IpAddress server, Port port, Sender<Packet> output);
  PacketReceiver(PacketReceiver&&) = default;

  void setup();
  void process(FalseInput);
  void teardown();

private:
  void start_read();

  using Buffer = std::vector<std::uint8_t>;

  PacketParser                 m_reader;
  Buffer                       m_buffer;
  IpAddress                    m_server_address;
  Port                         m_port;
  boost::asio::io_context      m_context;
  boost::asio::ip::tcp::socket m_receiver;

  MetricId m_packets_received_metric;
  MetricId m_bytes_received_metric;
};

}