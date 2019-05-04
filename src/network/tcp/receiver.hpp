#pragma once

#include <array>
#include <cstdint>
#include <system_error> // std::error_code

#include "disable_warnings_push.hpp"
#include <asio/ip/tcp.hpp>
#include "disable_warnings_pop.hpp"

#include "metrics/gauge.hpp"
#include "network/receiver.hpp"
#include "cancellation.hpp"
#include "context.hpp"
#include "channel.hpp"
#include "packet_parser.hpp"


namespace shar::tcp {

using IpAddress = asio::ip::address;
using Port      = const std::uint16_t;

class PacketReceiver
  : public IPacketReceiver
  , protected Context
{
public:
  PacketReceiver(Context context, IpAddress server, Port port);
  PacketReceiver(PacketReceiver&&) = default;

  void run(Sender<encoder::ffmpeg::Unit> units) override;
  void shutdown() override;

  void setup();
  void process();
  void teardown();

private:
  void start_read();

  using Buffer = std::vector<std::uint8_t>;
  using IOContext = asio::io_context;
  using Socket = asio::ip::tcp::socket;
  using ErrorCode = std::error_code;

  Sender<encoder::ffmpeg::Unit>* m_sender{ nullptr };

  Cancellation m_running;
  PacketParser m_reader;
  Buffer       m_buffer;
  IpAddress    m_server_address;
  Port         m_port;
  IOContext    m_context;
  Socket       m_receiver;

  metrics::Gauge m_packets_received;
  metrics::Gauge m_bytes_received;
};

}