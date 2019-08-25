#pragma once

#include <cstdint>
#include <optional>
#include <system_error>
#include <vector>

#include "disable_warnings_push.hpp"
#include <asio/ip/udp.hpp>
#include "disable_warnings_pop.hpp"

#include "time.hpp"
#include "context.hpp"
#include "cancellation.hpp"
#include "channel.hpp"
#include "net/receiver.hpp"
#include "net/rtp/packet.hpp"
#include "net/rtp/depacketizer.hpp"


namespace shar::net::rtp {

using IpAddress = asio::ip::address;
using Port = std::uint16_t;

class Receiver
  : public IPacketReceiver
  , protected Context
{
public:
  using IOContext = asio::io_context;
  using Socket = asio::ip::udp::socket;
  using Endpoint = asio::ip::udp::endpoint;
  using ErrorCode = std::error_code;
  using Unit = codec::ffmpeg::Unit;
  using Output = Sender<Unit>;

  Receiver(Context context, IpAddress ip, Port port);
  Receiver(Receiver&&) = default;

  void run(Output units) override;
  void shutdown() override;

private:
  std::optional<Unit> receive();
  std::optional<Unit> accept(const Packet& packet, const Fragment& fragment);

  // metrics
  std::size_t m_received{ 0 };
  std::size_t m_dropped{ 0 };

  Cancellation m_running;

  IOContext m_context;
  Socket m_socket;
  Endpoint m_endpoint;

  // rtp session state
  std::optional<Endpoint> m_sender;
  std::uint16_t m_sequence{ 0 };
  std::uint32_t m_timestamp{ 0 }; // current timestamp
  bool m_drop{ true }; // true if drop occured
  Depacketizer m_depacketizer;
};

}