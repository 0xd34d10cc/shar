#pragma once

#include "context.hpp"
#include "cancellation.hpp"
#include "channel.hpp"
#include "packet_parser.hpp"
#include "metrics/gauge.hpp"
#include "net/receiver.hpp"
#include "net/types.hpp"


namespace shar::net::tcp {

class PacketReceiver
  : public IPacketReceiver
  , protected Context
{
public:
  PacketReceiver(Context context, IpAddress server, Port port);
  PacketReceiver(PacketReceiver&&) = default;

  void run(Sender<codec::ffmpeg::Unit> units) override;
  void shutdown() override;

private:
  void start_read();

  using Buffer = std::vector<std::uint8_t>;

  // NOTE: only valid inside run() call
  Sender<codec::ffmpeg::Unit>* m_sender{ nullptr };

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