#pragma once

#include <cstdint>
#include <optional>

#include "context.hpp"
#include "cancellation.hpp"
#include "channel.hpp"
#include "net/types.hpp"
#include "net/receiver.hpp"
#include "net/rtp/packet.hpp"
#include "net/rtp/depacketizer.hpp"


namespace shar::net::rtp {


class Receiver
  : public IPacketReceiver
  , protected Context
{
public:
  using Unit = codec::ffmpeg::Unit;
  using Output = Sender<Unit>;

  Receiver(Context context, IpAddress ip, Port port);

  void run(Output units) override;
  void shutdown() override;

private:
  std::optional<Unit> receive();
  std::optional<Unit> accept(const Packet& packet, const Fragment& fragment);

  // metrics
  usize m_received{ 0 };
  usize m_dropped{ 0 };

  Cancellation m_running;

  IOContext m_context;
  udp::Socket m_socket;
  udp::Endpoint m_endpoint;

  // rtp session state
  std::optional<udp::Endpoint> m_sender;
  u16 m_sequence{ 0 };
  u32 m_timestamp{ 0 }; // current timestamp
  bool m_drop{ true }; // true if drop occured
  Depacketizer m_depacketizer;
};

}
