#include "sender_factory.hpp"

#include "tcp/sender.hpp"
#include "tcp/p2p_sender.hpp"
#include "rtp/sender.hpp"


namespace shar::net {

std::unique_ptr<IPacketSender> create_sender(Context context, Url url) {
  switch (url.protocol()) {
    case Protocol::TCP:
      if (context.m_config->p2p) {
        return std::make_unique<tcp::P2PSender>(std::move(context), url.host(), url.port());
      }
      else {
        return std::make_unique<tcp::PacketSender>(std::move(context), url.host(), url.port());
      }
    case Protocol::RTP:
      return std::make_unique<rtp::PacketSender>(std::move(context), url.host(), url.port());
    default:
      assert(false);
    case Protocol::RTSP:
      throw std::runtime_error("Unsupported protocol");
  }
}

}