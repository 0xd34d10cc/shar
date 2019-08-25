#include "receiver_factory.hpp"

#include "disable_warnings_push.hpp"
#include <asio/ip/udp.hpp>
#include "disable_warnings_pop.hpp"

#include "tcp/receiver.hpp"
#include "rtp/receiver.hpp"


namespace shar::net {

using Resolver = asio::ip::udp::resolver;
using Endpoint = asio::ip::udp::endpoint;
using IOContext = asio::io_context;

std::unique_ptr<IPacketReceiver> create_receiver(Context context, Url url) {
  Endpoint endpoint;
  IOContext ioc;
  Resolver resolver{ ioc };
  const auto port = std::to_string(url.port());
  for (auto entry : resolver.resolve(asio::ip::udp::v4(), url.host(), port)) {
    if (entry.endpoint().address().is_v4()) {
      endpoint = entry;
      break;
    }
  }

  context.m_logger.info("Resolved {}:{} to {}:{}",
                        url.host(), url.port(),
                        endpoint.address().to_string(), endpoint.port());


  switch (url.protocol()) {
    case Protocol::TCP:
      return std::make_unique<tcp::PacketReceiver>(std::move(context),
                                                   endpoint.address(),
                                                   url.port());
    case Protocol::RTP:
      return std::make_unique<rtp::Receiver>(std::move(context),
                                             endpoint.address(),
                                             url.port());
    default:
      assert(false);
    case Protocol::RTSP:
      throw std::runtime_error("Unsupported protocol");
  }
}

}