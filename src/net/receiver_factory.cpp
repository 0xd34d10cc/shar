#include "receiver_factory.hpp"

#include "net/types.hpp"
#include "tcp/receiver.hpp"
#include "rtp/receiver.hpp"


namespace shar::net {

std::unique_ptr<IPacketReceiver> create_receiver(Context context, Url url) {
  udp::Endpoint endpoint;
  IOContext ioc;
  udp::Resolver resolver{ ioc };
  const auto port = std::to_string(url.port());
  for (auto entry : resolver.resolve(udp::v4(), url.host(), port)) {
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