#include "sender_factory.hpp"

#include "net/types.hpp"
#include "net/dns.hpp"
#include "tcp/sender.hpp"
#include "tcp/p2p_sender.hpp"
#include "rtp/sender.hpp"
#include "rtsp/server.hpp"


namespace shar::net {

std::unique_ptr<IPacketSender> create_sender(Context context, Url url) {
  auto address = dns::resolve(url.host(), url.port());
  if (auto e = address.err()) {
    throw std::runtime_error("Failed to resolve " + url.host() + ": " + e.message());
  }

  g_logger.info("Resolved {} to {}",
                        url.host(), address->to_string());

  switch (url.protocol()) {
    case Protocol::TCP:
      if (context.m_config->p2p) {
        return std::make_unique<tcp::P2PSender>(std::move(context),
                                                *address,
                                                url.port());
      }
      else {
        return std::make_unique<tcp::PacketSender>(std::move(context),
                                                   *address,
                                                   url.port());
      }
    case Protocol::RTP:
      return std::make_unique<rtp::PacketSender>(std::move(context),
                                                 *address,
                                                 url.port());
    case Protocol::RTSP:
      return std::make_unique<rtsp::Server>(std::move(context),
                                                    *address,
                                                    url.port());
    default:
      assert(false);
      throw std::runtime_error("Unsupported protocol");
  }
}

}