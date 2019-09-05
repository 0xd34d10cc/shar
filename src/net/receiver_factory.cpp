#include "receiver_factory.hpp"

#include "net/types.hpp"
#include "net/dns.hpp"
#include "tcp/receiver.hpp"
#include "rtp/receiver.hpp"


namespace shar::net {

std::unique_ptr<IPacketReceiver> create_receiver(Context context, Url url) {
  auto address = dns::resolve(url.host(), url.port());
  if (auto e = address.err()) {
    throw std::runtime_error("Failed to resolve " + url.host() + ": " + e.message());
  }

  context.m_logger.info("Resolved {} to {}",
                        url.host(), address->to_string());

  switch (url.protocol()) {
    case Protocol::TCP:
      return std::make_unique<tcp::PacketReceiver>(std::move(context),
                                                   *address,
                                                   url.port());
    case Protocol::RTP:
      return std::make_unique<rtp::Receiver>(std::move(context),
                                             *address,
                                             url.port());
    default:
      assert(false);
    case Protocol::RTSP:
      throw std::runtime_error("Unsupported protocol");
  }
}

}